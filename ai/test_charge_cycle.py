#!/usr/bin/env python3
"""
So bản C (charge_cycle.cpp) với bản Python (nasa_prepare.charge_features).

Cùng lý do như test_c_vs_python.py ở Lớp 1: mô hình được train trên đặc trưng
do Python tính; firmware tính lệch đi là cloud nhân hệ số với đầu vào sai, ra
một con số RUL trông rất hợp lý nhưng vô nghĩa. Không crash, không log.

Cách làm: lấy một chu kỳ sạc THẬT từ bộ NASA, bơm từng mẫu vào bản C, rồi so
9 đặc trưng với bản Python.

Chạy: ai/.venv/bin/python ai/test_charge_cycle.py
"""
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np
import scipy.io as sio

HERE = Path(__file__).parent
FW = HERE.parent / "VEDCaPhenika" / "esp32s3_wiseiot_test"
sys.path.insert(0, str(HERE))

HARNESS = r"""
#include "charge_cycle.h"
#include <stdio.h>
int main(int argc, char** argv) {
  FILE* f = fopen(argv[1], "r");
  int T; if (fscanf(f, "%d", &T) != 1) return 1;
  ChargeCycle cc; cc.begin();
  for (int i = 0; i < T; i++) {
    float v, a, t;
    if (fscanf(f, "%f %f %f", &v, &a, &t) != 3) return 1;
    ChargeSummary s = cc.update(v, a, t);
    if (s.valid) {
      for (int k = 0; k < CC_N_FEATURES; k++) printf("%.9g ", s.f[k]);
      printf("\n");
    }
  }
  fclose(f);
  return 0;
}
"""


def main():
    from nasa_prepare import FEATURES, RAW, charge_features

    mats = list(RAW.rglob("B0005.mat"))
    if not mats:
        sys.exit("Chưa có dữ liệu NASA — chạy phần tải trong ai/README.md")
    cycles = sio.loadmat(mats[0], simplify_cells=True)["B0005"]["cycle"]
    charges = [c for c in cycles if c["type"] == "charge"]

    # lấy vài chu kỳ trải đều vòng đời, bỏ chu kỳ rác
    picks = []
    for idx in (0, len(charges) // 3, 2 * len(charges) // 3, len(charges) - 2):
        d = charges[idx]["data"]
        if charge_features(d) is not None:
            picks.append((idx, d))
    if not picks:
        sys.exit("không có chu kỳ sạc nào dùng được")

    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        (td / "h.cpp").write_text(HARNESS)
        exe = td / "h"
        r = subprocess.run(
            ["g++", "-std=c++17", "-O2", "-I", str(FW), "-o", str(exe),
             str(td / "h.cpp"), str(FW / "charge_cycle.cpp")],
            capture_output=True, text=True)
        if r.returncode:
            print(r.stderr); sys.exit("biên dịch C THẤT BẠI")

        fails = 0
        for idx, d in picks:
            V = np.asarray(d["Voltage_measured"], float).ravel()
            I = np.asarray(d["Current_measured"], float).ravel()
            T = np.asarray(d["Temperature_measured"], float).ravel()
            t = np.asarray(d["Time"], float).ravel()

            # Bộ NASA lấy mẫu không đều; bản C chạy đúng 1 Hz. Nội suy về 1 Hz
            # cho đúng cách firmware sẽ thấy dữ liệu.
            grid = np.arange(0, t[-1], 1.0)
            Vg, Ig, Tg = (np.interp(grid, t, a) for a in (V, I, T))
            # firmware nhận điện áp PACK -> nhân 8 để mô phỏng pack 8S
            Vpack = Vg * 8.0
            # thêm vài giây dòng 0 ở cuối để bản C chốt sổ chu kỳ.
            # Phải nối cho CẢ lưới thời gian, nếu không bản Python nhận mảng
            # lệch độ dài (đúng lỗi đã gặp lần chạy đầu).
            pad = 5
            Vpack = np.r_[Vpack, [Vpack[-1]] * pad]
            Ig = np.r_[Ig, [0.0] * pad]
            Tg = np.r_[Tg, [Tg[-1]] * pad]
            Vg = np.r_[Vg, [Vg[-1]] * pad]
            grid = np.r_[grid, grid[-1] + np.arange(1, pad + 1)]

            inp = td / "in.txt"
            with open(inp, "w") as fh:
                fh.write(f"{len(Vpack)}\n")
                for k in range(len(Vpack)):
                    fh.write(f"{Vpack[k]:.9g} {Ig[k]:.9g} {Tg[k]:.9g}\n")

            out = subprocess.run([str(exe), str(inp)], capture_output=True, text=True)
            if out.returncode or not out.stdout.strip():
                print(f"  chu kỳ #{idx}: bản C KHÔNG cho ra summary nào"); fails += 1; continue
            cvals = [float(x) for x in out.stdout.strip().splitlines()[-1].split()]

            # bản Python chạy trên CÙNG lưới 1 Hz
            py = charge_features({"Voltage_measured": Vg, "Current_measured": Ig,
                                  "Temperature_measured": Tg, "Time": grid})
            if py is None:
                print(f"  chu kỳ #{idx}: bản Python loại chu kỳ này"); continue

            print(f"\nchu kỳ sạc #{idx} ({len(grid)} giây):")
            for k, name in enumerate(FEATURES):
                a, b = py[name], cvals[k]
                scale = max(abs(a), 1.0)
                ok = abs(a - b) < 2e-3 * scale
                fails += not ok
                print(f"  {name:16} python={a:12.4f}  C={b:12.4f}  "
                      f"{'OK' if ok else '<<< LỆCH'}")

    print("\n" + ("=== CO %d CHO LECH ===" % fails if fails
                  else "=== C VA PYTHON KHOP ==="))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
