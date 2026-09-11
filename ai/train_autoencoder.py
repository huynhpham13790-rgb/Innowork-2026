#!/usr/bin/env python3
"""
Lớp 1 — Autoencoder phát hiện cell bất thường.

Kiến trúc 16 -> 8 -> 4 -> 8 -> 16 (đúng như KHUNG_NGHIEN_CUU §4).

Cách hoạt động: chỉ train trên dữ liệu BÌNH THƯỜNG. Mô hình học cách nén rồi
dựng lại vector 16 đặc trưng của một cell khoẻ. Gặp cell bất thường, nó dựng
lại sai -> sai số tái tạo lớn -> báo động. Không cần một mẫu lỗi nào để train,
đó là lý do chọn autoencoder thay vì bộ phân loại: dữ liệu lỗi pin thật gần
như không thể thu thập, mà dữ liệu bình thường thì thừa.

Ngưỡng báo động lấy theo phân vị trên tập validation (toàn dữ liệu bình
thường), KHÔNG lấy theo tập test — lấy theo test là tự chấm điểm cho mình.

Chạy: ai/.venv/bin/python ai/train_autoencoder.py
"""
import os, sys
from pathlib import Path

os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")
import numpy as np
import tensorflow as tf
from tensorflow import keras

HERE = Path(__file__).parent
PREP = HERE / "data" / "prepared"
MODELS = HERE / "models"
SEED = 1234
EPOCHS = 40
BATCH = 4096

# Mô hình này có 356 tham số. 6 triệu mẫu không làm nó tốt hơn 1 triệu mẫu,
# chỉ làm hết RAM (máy dev chỉ còn ~2,5 GB trống). Lấy ngẫu nhiên có seed cố
# định từ tập đã chuẩn bị -> vẫn phủ đủ 287 chu kỳ, mà nhẹ hơn 6 lần.
MAX_SAMPLES = {"train": 1_000_000, "val": 300_000}


def build_model(n_in=16):
    """
    Cố tình để nút thắt 4 chiều: ép mô hình học *cấu trúc chung* của một cell
    khoẻ chứ không học thuộc lòng. Nút thắt rộng quá thì autoencoder tái tạo
    được cả cell lỗi -> mất luôn khả năng phát hiện.
    """
    return keras.Sequential([
        keras.layers.Input((n_in,)),
        keras.layers.Dense(8, activation="relu", name="enc1"),
        keras.layers.Dense(4, activation="relu", name="bottleneck"),
        keras.layers.Dense(8, activation="relu", name="dec1"),
        keras.layers.Dense(n_in, activation="linear", name="out"),
    ], name="cell_anomaly_ae")


def load(split):
    """Đọc rồi rút ngay xuống cỡ vừa RAM, giải phóng mảng lớn trước khi trả về."""
    d = np.load(PREP / f"{split}.npz")
    X, cyc = d["X"], d["cycle"]
    cap = MAX_SAMPLES.get(split)
    if cap and len(X) > cap:
        keep = np.random.default_rng(SEED).choice(len(X), cap, replace=False)
        keep.sort()
        X, cyc = X[keep].copy(), cyc[keep].copy()
    d.close()
    return X, cyc


def main():
    tf.random.set_seed(SEED); np.random.seed(SEED)
    if not (PREP / "train.npz").exists():
        sys.exit("Chưa có dữ liệu — chạy prepare_data.py trước")

    Xtr, _ = load("train")
    Xva, _ = load("val")
    sc = np.load(PREP / "scaler.npz")
    mu, sd = sc["mu"], sc["sd"]

    Ztr = (Xtr - mu) / sd
    Zva = (Xva - mu) / sd
    print(f"train {Ztr.shape}  val {Zva.shape}")

    model = build_model(Ztr.shape[1])
    model.compile(optimizer=keras.optimizers.Adam(1e-3), loss="mse")
    model.summary()

    model.fit(
        Ztr, Ztr,
        validation_data=(Zva, Zva),
        epochs=EPOCHS, batch_size=BATCH, shuffle=True, verbose=2,
        callbacks=[
            keras.callbacks.EarlyStopping(patience=6, restore_best_weights=True,
                                          monitor="val_loss"),
            keras.callbacks.ReduceLROnPlateau(patience=3, factor=0.5, min_lr=1e-5),
        ],
    )

    # Ngưỡng: lấy phân vị của sai số tái tạo trên tập VALIDATION bình thường.
    # p99.9 nghĩa là chấp nhận ~1 báo động giả trên 1000 mẫu-cell. Ở 8 cell,
    # 1 Hz thì tương đương khoảng 1 lần mỗi ~2 phút — vẫn nhiều, nên lúc chạy
    # thật còn phải cộng thêm điều kiện "giữ liên tục K giây" (xem evaluate.py).
    # Tính sai số theo từng lô để không nhân đôi mảng trong RAM
    errs = []
    for i in range(0, len(Zva), 100_000):
        b = Zva[i:i + 100_000]
        errs.append(np.mean((model.predict(b, batch_size=BATCH, verbose=0) - b) ** 2, axis=1))
    err_va = np.concatenate(errs)
    th = {f"p{q}": float(np.percentile(err_va, q)) for q in (95, 99, 99.9, 99.99)}

    MODELS.mkdir(exist_ok=True)
    model.save(MODELS / "cell_ae.keras")
    np.savez(MODELS / "thresholds.npz", **th,
             err_val_mean=float(err_va.mean()), err_val_max=float(err_va.max()))
    print("\nngưỡng theo phân vị sai số trên tập val (bình thường):")
    for k, v in th.items():
        print(f"   {k:8} {v:.6f}")
    print(f"\nđã lưu {MODELS/'cell_ae.keras'}")


if __name__ == "__main__":
    main()
