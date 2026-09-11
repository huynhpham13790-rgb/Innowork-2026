#!/usr/bin/env python3
"""
Xuất autoencoder thành file .h chứa trọng số float32, để firmware tự nhân tay.

VÌ SAO KHÔNG DÙNG TFLite Micro (đây là một quyết định, xem DECISION_LOG QĐ-013):

Mô hình có 356 tham số, đúng 4 lớp dense 16->8->4->8->16. Ở cỡ đó:
  - float32 chỉ tốn 356*4 = 1,4 KB, đã nhỏ hơn giới hạn 50 KB tới 35 lần.
    Lượng tử hoá INT8 để "cho vừa" là giải quyết một vấn đề không tồn tại.
  - Forward pass viết tay hết ~30 dòng C, không phụ thuộc thư viện nào.
    TFLite Micro trên Arduino-ESP32 kéo theo hàng trăm KB flash, một
    arena bộ nhớ phải tự chỉnh, và một đống rủi ro tích hợp — đúng thứ không
    nên rước vào người 4 ngày trước bán kết.
  - Nhân tay thì kiểm chứng được KHỚP TỪNG SỐ với Python (test_c_vs_python.sh).
    Qua TFLite còn thêm sai số lượng tử hoá phải đi giải trình.
  - Tốc độ không phải vấn đề: 356 phép nhân-cộng × 8 cell = 2.848 phép/giây.
    ESP32-S3 chạy 240 MHz làm việc đó trong chưa tới 1 phần nghìn giây.

Bản INT8 .tflite vẫn được giữ trong models/ nếu sau này muốn đổi hướng.

Chạy: ai/.venv/bin/python ai/export_c_model.py
"""
import os
from pathlib import Path

os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")
import numpy as np
from tensorflow import keras

HERE = Path(__file__).parent
MODELS = HERE / "models"
PREP = HERE / "data" / "prepared"
OUT = HERE.parent / "VEDCaPhenika" / "esp32s3_wiseiot_test" / "cell_ae_weights.h"


def cfloat(v):
    """
    Literal float hợp lệ trong C. Phải cẩn thận: '%.8g' của số 0 cho ra "0",
    mà "0f" KHÔNG phải literal C (trình biên dịch báo 'unable to find numeric
    literal operator'). Số nguyên như 1 cũng thành "1f" -> hỏng y hệt.
    """
    s = f"{float(v):.9g}"
    if "." not in s and "e" not in s and "E" not in s and "inf" not in s:
        s += ".0"
    return s + "f"


def arr(name, a):
    flat = np.asarray(a, np.float32).ravel()
    body = ",\n  ".join(", ".join(cfloat(v) for v in flat[i:i + 8])
                        for i in range(0, len(flat), 8))
    return f"const float {name}[{len(flat)}] = {{\n  {body}\n}};\n"


def main():
    model = keras.models.load_model(MODELS / "cell_ae.keras", compile=False)
    sc = np.load(PREP / "scaler.npz")
    op = np.load(MODELS / "operating_point.npz")

    lines = [
        "// Tự sinh bởi ai/export_c_model.py — ĐỪNG sửa tay.",
        "// Autoencoder phát hiện cell bất thường: 16 -> 8 -> 4 -> 8 -> 16",
        "// Trọng số float32, firmware tự nhân (xem cell_ai.cpp).",
        "#pragma once",
        "",
        "#define AE_N_IN      16",
        f"#define AE_THRESHOLD {cfloat(op['threshold'])}"
        "   // sai số tái tạo vượt mức này = bất thường",
        f"#define AE_PERSIST_S {int(op['persist_s'])}"
        "        // phải vượt liên tục bấy nhiêu giây mới báo động",
        "",
        "// Chuẩn hoá đầu vào. PHẢI khớp ai/data/prepared/scaler.npz.",
        arr("AE_FEAT_MEAN", sc["mu"]),
        arr("AE_FEAT_STD", sc["sd"]),
    ]

    for i, layer in enumerate([l for l in model.layers if l.weights]):
        W, b = layer.get_weights()
        lines.append(f"// lớp {i}: {layer.name}  {W.shape[0]} -> {W.shape[1]}"
                     f"  ({'relu' if i < 3 else 'linear'})")
        lines.append(f"#define AE_L{i}_IN  {W.shape[0]}")
        lines.append(f"#define AE_L{i}_OUT {W.shape[1]}")
        # Lưu theo thứ tự [out][in] để vòng lặp trong C đọc liên tiếp bộ nhớ
        lines.append(arr(f"AE_W{i}", W.T))
        lines.append(arr(f"AE_B{i}", b))

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text("\n".join(lines))
    n_par = sum(w.size for l in model.layers for w in l.get_weights())
    print(f"đã sinh {OUT}")
    print(f"  {n_par} tham số = {n_par*4} byte float32 ({n_par*4/1024:.1f} KB)")
    print(f"  ngưỡng {float(op['threshold']):.4f}, giữ {int(op['persist_s'])}s")
    print(f"  nguồn .h: {OUT.stat().st_size/1024:.0f} KB")


if __name__ == "__main__":
    main()
