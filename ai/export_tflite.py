#!/usr/bin/env python3
"""
Xuất autoencoder sang TFLite INT8 + file .h nhúng thẳng vào firmware ESP32.

Vì sao INT8 chứ không float32: ESP32-S3 không có FPU cho vector, nhân số
nguyên nhanh hơn hẳn, và mô hình nhỏ đi 4 lần. Yêu cầu trong KHUNG_NGHIEN_CUU
là <50 KB — phần dưới sẽ kiểm tra và báo lỗi nếu vượt.

Lượng tử hoá cần "dữ liệu đại diện" để đo dải giá trị thật của từng lớp.
Dùng đúng dữ liệu train đã chuẩn hoá, KHÔNG dùng dữ liệu ngẫu nhiên — lấy
ngẫu nhiên thì dải sai, sai số lượng tử hoá vọt lên và mô hình mất tác dụng.

Chạy: ai/.venv/bin/python ai/export_tflite.py
"""
import os, sys
from pathlib import Path

os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")
import numpy as np
import tensorflow as tf
from tensorflow import keras

HERE = Path(__file__).parent
MODELS = HERE / "models"
PREP = HERE / "data" / "prepared"
SIZE_LIMIT = 50 * 1024
N_CALIB = 2000


def main():
    model = keras.models.load_model(MODELS / "cell_ae.keras", compile=False)
    sc = np.load(PREP / "scaler.npz")
    mu, sd = sc["mu"], sc["sd"]

    X = np.load(PREP / "val.npz")["X"]
    rng = np.random.default_rng(0)
    calib = ((X[rng.choice(len(X), N_CALIB, replace=False)] - mu) / sd).astype(np.float32)
    del X

    def rep_data():
        for i in range(len(calib)):
            yield [calib[i:i + 1]]

    conv = tf.lite.TFLiteConverter.from_keras_model(model)
    conv.optimizations = [tf.lite.Optimize.DEFAULT]
    conv.representative_dataset = rep_data
    # ép toàn bộ sang int8: TFLite Micro trên ESP32 không có kernel float
    conv.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    conv.inference_input_type = tf.int8
    conv.inference_output_type = tf.int8
    blob = conv.convert()

    path = MODELS / "cell_ae_int8.tflite"
    path.write_bytes(blob)
    print(f"TFLite INT8: {len(blob):,} byte ({len(blob)/1024:.1f} KB)")
    if len(blob) > SIZE_LIMIT:
        sys.exit(f"VƯỢT giới hạn {SIZE_LIMIT/1024:.0f} KB")
    print(f"  -> đạt yêu cầu <{SIZE_LIMIT/1024:.0f} KB")

    # ---- so sánh INT8 với float32: lượng tử hoá có làm hỏng mô hình không? --
    interp = tf.lite.Interpreter(model_content=blob)
    interp.allocate_tensors()
    ind, outd = interp.get_input_details()[0], interp.get_output_details()[0]
    in_s, in_z = ind["quantization"]
    out_s, out_z = outd["quantization"]

    test = calib[:500]
    err_f = np.mean((model.predict(test, verbose=0) - test) ** 2, axis=1)
    err_q = []
    for row in test:
        q = np.clip(np.round(row / in_s + in_z), -128, 127).astype(np.int8)
        interp.set_tensor(ind["index"], q[None, :])
        interp.invoke()
        r = (interp.get_tensor(outd["index"])[0].astype(np.float32) - out_z) * out_s
        err_q.append(float(np.mean((r - row) ** 2)))
    err_q = np.array(err_q)

    corr = float(np.corrcoef(err_f, err_q)[0, 1])
    print(f"\nsai số tái tạo float32 vs INT8:")
    print(f"  tương quan           {corr:.4f}")
    print(f"  lệch tuyệt đối TB    {np.abs(err_f - err_q).mean():.5f}")
    print(f"  float32 trung bình   {err_f.mean():.5f}")
    print(f"  INT8    trung bình   {err_q.mean():.5f}")
    if corr < 0.95:
        sys.exit("CẢNH BÁO: INT8 lệch quá nhiều so với float32 -> ngưỡng sẽ sai")

    # ---- sinh file .h để #include thẳng vào sketch -------------------------
    op = MODELS / "operating_point.npz"
    thr, persist = (float(np.load(op)["threshold"]), int(np.load(op)["persist_s"])) \
        if op.exists() else (float(np.load(MODELS / "thresholds.npz")["p99.9"]), 30)

    h = [
        "// Tự sinh bởi ai/export_tflite.py — ĐỪNG sửa tay.",
        "// Autoencoder phát hiện cell bất thường, 16->8->4->8->16, INT8.",
        "#pragma once",
        "#include <stdint.h>",
        "",
        f"// Ngưỡng báo động và quy tắc giữ liên tục, chọn bằng tune_threshold.py",
        f"#define AE_THRESHOLD   {thr:.6f}f",
        f"#define AE_PERSIST_S   {persist}",
        f"#define AE_IN_SCALE    {in_s:.8f}f",
        f"#define AE_IN_ZERO     {int(in_z)}",
        f"#define AE_OUT_SCALE   {out_s:.8f}f",
        f"#define AE_OUT_ZERO    {int(out_z)}",
        "",
        "// Trung bình / độ lệch chuẩn để chuẩn hoá 16 đặc trưng.",
        "// PHẢI khớp với ai/data/prepared/scaler.npz, sai là mô hình vô nghĩa.",
        "const float AE_FEAT_MEAN[16] = {" + ", ".join(f"{v:.6f}f" for v in mu) + "};",
        "const float AE_FEAT_STD[16]  = {" + ", ".join(f"{v:.6f}f" for v in sd) + "};",
        "",
        f"const unsigned int g_cell_ae_len = {len(blob)};",
        "alignas(16) const unsigned char g_cell_ae[] = {",
    ]
    for i in range(0, len(blob), 12):
        h.append("  " + ", ".join(f"0x{b:02x}" for b in blob[i:i + 12]) + ",")
    h.append("};")

    hp = MODELS / "cell_ae_model.h"
    hp.write_text("\n".join(h) + "\n")
    print(f"\nđã sinh {hp} ({hp.stat().st_size/1024:.0f} KB nguồn)")
    print(f"  ngưỡng={thr:.4f}  giữ={persist}s")


if __name__ == "__main__":
    main()
