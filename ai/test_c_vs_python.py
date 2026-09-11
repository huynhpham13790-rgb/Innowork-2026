#!/usr/bin/env python3
"""
So bản C (firmware) với bản Python (lúc train) — PHẢI khớp từng phần tử.

Đây là test quan trọng nhất của cả phần AI. Mô hình được train trên đặc trưng
do features.py sinh ra; nếu firmware tính ra con số khác dù chỉ một chút, thì
autoencoder đang nhận đầu vào lệch khỏi phân phối lúc học, và ngưỡng báo động
trở nên vô nghĩa. Kiểu lỗi đó không làm firmware crash, không có log, chỉ
lặng lẽ làm mô hình sai — đúng loại lỗi tệ nhất.

Cách làm: biên dịch cell_ai.cpp trên PC cùng một chương trình nhỏ, cho cả hai
bên ăn cùng một chuỗi dữ liệu, rồi so 16 đặc trưng và sai số tái tạo.

Chạy: ai/.venv/bin/python ai/test_c_vs_python.py
"""
import os, subprocess, sys, tempfile
from pathlib import Path

os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")
import numpy as np

HERE = Path(__file__).parent
FW = HERE.parent / "VEDCaPhenika" / "esp32s3_wiseiot_test"
sys.path.insert(0, str(HERE))

HARNESS = r"""
#include "cell_ai.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char** argv) {
  FILE* f = fopen(argv[1], "r");
  int T; if (fscanf(f, "%d", &T) != 1) return 1;
  CellAI ai; ai.begin();
  for (int t = 0; t < T; t++) {
    float temps[AI_N_CELLS], amb, cur, soc;
    for (int i = 0; i < AI_N_CELLS; i++) if (fscanf(f, "%f", &temps[i]) != 1) return 1;
    if (fscanf(f, "%f %f %f", &amb, &cur, &soc) != 3) return 1;
    CellAIResult r = ai.update(temps, amb, cur, soc);
    for (int i = 0; i < AI_N_CELLS; i++) {
      float v[AI_N_FEAT]; ai.features_of(i, v);
      for (int k = 0; k < AI_N_FEAT; k++) printf("%.9g ", v[k]);
      printf("%.9g\n", r.score[i]);
    }
  }
  fclose(f);
  return 0;
}
"""


def main():
    from features import build_features, DS18B20_STEP
    import tensorflow as tf
    from tensorflow import keras

    if not (FW / "cell_ae_weights.h").exists():
        sys.exit("Thiếu cell_ae_weights.h — chạy ai/export_c_model.py trước")

    # ---- sinh chuỗi dữ liệu thử, có cả một cell nóng dần ---------------------
    rng = np.random.default_rng(3)
    T, N = 400, 8
    temps = 25 + np.cumsum(rng.normal(0, 0.03, (T, N)), axis=0).astype(np.float32)
    temps[:, 2] += np.clip(np.arange(T) - 150, 0, None) * 0.01      # cell 2 nóng dần
    temps = np.round(temps / DS18B20_STEP) * DS18B20_STEP           # như DS18B20
    # ép nhiều giá trị bằng nhau để test đúng nhánh xử lý hoà hạng
    temps[:50, 4] = temps[:50, 5]
    amb = np.full(T, 24.5, np.float32)
    cur = rng.normal(0, 6, T).astype(np.float32)
    soc = np.linspace(95, 30, T).astype(np.float32)

    with tempfile.TemporaryDirectory() as td:
        td = Path(td)
        (td / "h.cpp").write_text(HARNESS)
        exe = td / "h"
        r = subprocess.run(
            ["g++", "-std=c++17", "-O2", "-I", str(FW), "-o", str(exe),
             str(td / "h.cpp"), str(FW / "cell_ai.cpp")],
            capture_output=True, text=True)
        if r.returncode:
            print(r.stderr); sys.exit("biên dịch C THẤT BẠI")

        inp = td / "in.txt"
        with open(inp, "w") as fh:
            fh.write(f"{T}\n")
            for t in range(T):
                fh.write(" ".join(f"{v:.9g}" for v in temps[t]))
                fh.write(f" {amb[t]:.9g} {cur[t]:.9g} {soc[t]:.9g}\n")

        out = subprocess.run([str(exe), str(inp)], capture_output=True, text=True)
        if out.returncode:
            print(out.stderr); sys.exit("chạy C THẤT BẠI")

    c = np.array([[float(x) for x in line.split()]
                  for line in out.stdout.strip().splitlines()], dtype=np.float64)
    c_feat = c[:, :16].reshape(T, N, 16)
    c_score = c[:, 16].reshape(T, N)

    # ---- bản Python ---------------------------------------------------------
    py_feat = build_features(temps, amb, cur, soc).astype(np.float64)

    model = keras.models.load_model(HERE / "models" / "cell_ae.keras", compile=False)
    sc = np.load(HERE / "data" / "prepared" / "scaler.npz")
    z = (py_feat.reshape(-1, 16) - sc["mu"]) / sc["sd"]
    rec = model.predict(z, batch_size=8192, verbose=0)
    py_score = np.mean((rec - z) ** 2, axis=1).reshape(T, N)

    # ---- so sánh ------------------------------------------------------------
    import features as FT
    fails = 0
    print(f"so {T} bước × {N} cell\n")
    print(f"{'đặc trưng':16} {'lệch tối đa':>14} {'lệch tương đối':>16}")
    for k, name in enumerate(FT.FEATURE_NAMES):
        a, b = py_feat[:, :, k], c_feat[:, :, k]
        d = np.abs(a - b).max()
        scale = max(np.abs(a).max(), 1e-9)
        ok = d < 2e-4 * max(scale, 1.0)
        fails += not ok
        print(f"{name:16} {d:14.3e} {d/scale:16.3e}  {'OK' if ok else '<<< LỆCH'}")

    ds = np.abs(py_score - c_score).max()
    ok = ds < 1e-4
    fails += not ok
    print(f"\n{'sai số tái tạo':16} {ds:14.3e}  {'OK' if ok else '<<< LỆCH'}")

    # cùng một cell bị chấm là tệ nhất?
    same = (py_score.argmax(1) == c_score.argmax(1)).mean()
    print(f"{'cùng cell tệ nhất':16} {100*same:13.2f}%  "
          f"{'OK' if same > 0.999 else '<<< KHAC'}")
    fails += not (same > 0.999)

    print("\n" + ("=== CO %d CHO LECH ===" % fails if fails
                  else "=== C VA PYTHON KHOP HOAN TOAN ==="))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
