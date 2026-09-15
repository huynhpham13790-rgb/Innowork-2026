#!/usr/bin/env python3
"""
Autoencoder CHỈ DÙNG ĐẶC TRƯNG TƯƠNG ĐỐI — thử giải bài toán sản xuất hàng loạt.

CÂU HỎI CẦN TRẢ LỜI
QĐ-032 phát hiện: mô hình 16 đặc trưng không chuyển giao sang pack khác, và
nguyên nhân là **bốn đặc trưng bối cảnh toàn pack** lệch tới 8,6 sd, trong khi
các đặc trưng tương đối giữa cell chỉ lệch ≤1,6 sd. Từ đó nảy ra câu hỏi thật
sự quan trọng: nếu mỗi pack phải hiệu chỉnh riêng thì **sản xuất hàng loạt kiểu
gì?**

Ý TƯỞNG
Vứt hẳn năm đặc trưng mang giá trị TUYỆT ĐỐI, chỉ giữ những đặc trưng nói về
**quan hệ giữa các cell trong cùng một pack**. Lý do vật lý: chênh lệch giữa
cell là đại lượng không phụ thuộc pack — "cell này nóng hơn phần còn lại 2 °C"
có nghĩa như nhau trên pack 8 cell 2,55 Ah lẫn pack 72 cell 5,2 Ah. Còn "pack
nóng hơn môi trường 12 °C" thì phụ thuộc hoàn toàn vào thiết kế tản nhiệt.

BỎ (mang giá trị tuyệt đối, gắn chặt với một pack cụ thể):
  4  T - amb          13 |I|/I_SCALE
  5  packT - amb      14 soc
  15 (T-25)/25

GIỮ (thuần tương đối, 11 đặc trưng):
  0 dev   1 dev_med   2 z   3 rank   6 spread
  7 dT_cell   8 dT_pack   9 dT_diff
  10 dev_ema   11 dev-dev_ema   12 roll_std

CÁI GIÁ PHẢI TRẢ, DỰ ĐOÁN TRƯỚC KHI CHẠY — ghi ra đây để không tự lừa mình sau:
mất bối cảnh nghĩa là mô hình không còn biết "đang sạc nặng" hay "đang nghỉ".
Cùng một mức chênh lệch 2 °C có thể bình thường lúc tải nặng và bất thường lúc
nghỉ. Nên tỉ lệ báo oan CÓ THỂ tăng. Nếu tăng nhiều thì đây là đường cụt, và
phải nói vậy.

PHẢI KIỂM CẢ HAI PHÍA, nếu không là tự lừa:
  (a) trên pack McMaster  -> có chuyển giao được không?
  (b) trên chính NASA     -> có MẤT khả năng phát hiện không?
Một mô hình chuyển giao tốt vì nó không phát hiện được gì cả thì vô dụng —
đúng cái bẫy đã ghi ở QĐ-030.

Chạy: ai/.venv/bin/python ai/train_ae_relative.py
"""
import os, sys, warnings
from pathlib import Path

os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")
warnings.filterwarnings("ignore")
import numpy as np

sys.path.insert(0, str(Path(__file__).parent))

HERE = Path(__file__).parent
PREP = HERE / "data" / "prepared"
MODELS = HERE / "models"
SEED = 1234
EPOCHS = 40
BATCH = 4096

REL = [0, 1, 2, 3, 6, 7, 8, 9, 10, 11, 12]     # 11 đặc trưng thuần tương đối
DROP = [4, 5, 13, 14, 15]


def main():
    import tensorflow as tf
    from tensorflow import keras
    from train_autoencoder import build_model, load

    tf.random.set_seed(SEED); np.random.seed(SEED)
    Xtr, _ = load("train")
    Xva, _ = load("val")
    sc = np.load(PREP / "scaler.npz")
    mu, sd = sc["mu"][REL], sc["sd"][REL]

    Ztr = (Xtr[:, REL] - mu) / sd
    Zva = (Xva[:, REL] - mu) / sd
    del Xtr, Xva
    print(f"train {Ztr.shape}  val {Zva.shape}  (11 dac trung thay vi 16)")

    model = build_model(len(REL))
    model.compile(optimizer=keras.optimizers.Adam(1e-3), loss="mse")
    model.fit(Ztr, Ztr, validation_data=(Zva, Zva),
              epochs=EPOCHS, batch_size=BATCH, shuffle=True, verbose=2,
              callbacks=[
                  keras.callbacks.EarlyStopping(patience=6, restore_best_weights=True,
                                                monitor="val_loss"),
                  keras.callbacks.ReduceLROnPlateau(patience=3, factor=0.5, min_lr=1e-5),
              ])

    errs = []
    for i in range(0, len(Zva), 100_000):
        b = Zva[i:i + 100_000]
        errs.append(np.mean((model.predict(b, batch_size=BATCH, verbose=0) - b) ** 2, axis=1))
    err_va = np.concatenate(errs)
    th = {f"p{q}": float(np.percentile(err_va, q)) for q in (95, 99, 99.9, 99.99)}

    MODELS.mkdir(exist_ok=True)
    model.save(MODELS / "cell_ae_rel.keras")
    np.savez(MODELS / "thresholds_rel.npz", **th)
    np.savez(MODELS / "scaler_rel.npz", mu=mu, sd=sd, idx=np.array(REL))
    print("\nnguong:", {k: round(v, 5) for k, v in th.items()})
    print(f"da ghi {MODELS/'cell_ae_rel.keras'}")


if __name__ == "__main__":
    main()
