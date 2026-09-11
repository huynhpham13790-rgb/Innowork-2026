#!/usr/bin/env python3
"""
Lớp 2 — Dự báo RUL (số chu kỳ còn lại tới khi pin còn 80% dung lượng).

GIAO THỨC ĐÁNH GIÁ: leave-one-battery-out. Train trên 3 pin, thử trên pin thứ
4 chưa từng thấy. Đây là cách duy nhất trung thực khi chỉ có 4 pin.

Chia ngẫu nhiên theo chu kỳ sẽ cho điểm rất đẹp và hoàn toàn vô nghĩa: chu kỳ
40 và 41 của cùng một pin gần như giống hệt nhau, mô hình chỉ việc nội suy.
Ngoài đời thì luôn là "pin mới chưa từng thấy", nên phải thử đúng như vậy.

Bài toán khó thật: 4 pin có tuổi thọ rất khác nhau (60, 77, 105, 123 chu kỳ),
nên đoán RUL tuyệt đối cho một pin lạ là đoán cả tuổi thọ tổng của nó.

So 4 phương pháp, cùng giao thức:
  B1. Trung bình RUL của tập train    — baseline ngu nhất, để biết sàn
  B2. Hồi quy tuyến tính 1 đặc trưng  — chỉ dùng t_cv (r=-0,86)
  B3. Ridge trên cả 9 đặc trưng
  B4. Gradient Boosting
  M.  LSTM trên cửa sổ 10 chu kỳ gần nhất  (kiến trúc mà tài liệu đề xuất)

Chạy: ai/.venv/bin/python ai/train_rul.py
"""
import os, sys
from pathlib import Path

os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")
import numpy as np
import pandas as pd

sys.path.insert(0, str(Path(__file__).parent))
from nasa_prepare import FEATURES, OUT as CSV

HERE = Path(__file__).parent
MODELS = HERE / "models"
WINDOW = 10          # LSTM nhìn lại 10 chu kỳ sạc gần nhất
SEED = 7


def windows(df, mu, sd):
    """(n, WINDOW, n_feat) + nhãn RUL + SOH, theo thứ tự thời gian của 1 pin."""
    X = ((df[FEATURES].to_numpy(np.float32) - mu) / sd)
    r = df["rul"].to_numpy(np.float32)
    s = df["soh"].to_numpy(np.float32)
    xs, ys, ss = [], [], []
    for i in range(WINDOW - 1, len(df)):
        xs.append(X[i - WINDOW + 1: i + 1])
        ys.append(r[i]); ss.append(s[i])
    if not xs:
        return (np.zeros((0, WINDOW, len(FEATURES)), np.float32),
                np.zeros(0, np.float32), np.zeros(0, np.float32))
    return np.array(xs, np.float32), np.array(ys, np.float32), np.array(ss, np.float32)


def build_lstm(n_feat):
    from tensorflow import keras
    return keras.Sequential([
        keras.layers.Input((WINDOW, n_feat)),
        keras.layers.LSTM(24),
        keras.layers.Dense(16, activation="relu"),
        keras.layers.Dropout(0.2),      # 4 pin thôi -> rất dễ học thuộc
        keras.layers.Dense(1),
    ], name="rul_lstm")


def main():
    import tensorflow as tf
    from tensorflow import keras
    from sklearn.linear_model import Ridge, LinearRegression
    from sklearn.ensemble import GradientBoostingRegressor

    if not CSV.exists():
        sys.exit("Chưa có nasa_cycles.csv — chạy nasa_prepare.py trước")
    df = pd.read_csv(CSV)
    bats = sorted(df["battery"].unique())
    print(f"{len(df)} chu kỳ, {len(bats)} pin: {', '.join(bats)}")
    print(f"tuổi thọ từng pin: " +
          ", ".join(f"{b}={int(df[df.battery==b].rul.max())+1}" for b in bats))

    methods = ["B1 trung binh", "B2 tuyen tinh (t_cv)", "B3 Ridge 9 dac trung",
               "B4 GradientBoosting", "M  LSTM cua so 10"]
    res = {m: [] for m in methods}       # (pin, rmse, mae, early_mae)
    soh_res = []

    for held in bats:
        tr = df[df.battery != held]
        te = df[df.battery == held]

        mu = tr[FEATURES].to_numpy(np.float32).mean(0)
        sd = tr[FEATURES].to_numpy(np.float32).std(0) + 1e-6

        Xtr = (tr[FEATURES].to_numpy(np.float32) - mu) / sd
        Xte = (te[FEATURES].to_numpy(np.float32) - mu) / sd
        ytr, yte = tr["rul"].to_numpy(np.float32), te["rul"].to_numpy(np.float32)

        preds = {}
        preds["B1 trung binh"] = np.full(len(yte), ytr.mean())

        icv = FEATURES.index("t_cv")
        lr = LinearRegression().fit(Xtr[:, [icv]], ytr)
        preds["B2 tuyen tinh (t_cv)"] = lr.predict(Xte[:, [icv]])

        preds["B3 Ridge 9 dac trung"] = Ridge(alpha=1.0).fit(Xtr, ytr).predict(Xte)
        preds["B4 GradientBoosting"] = GradientBoostingRegressor(
            n_estimators=200, max_depth=3, random_state=SEED).fit(Xtr, ytr).predict(Xte)

        # ---- LSTM ----
        tf.random.set_seed(SEED); np.random.seed(SEED)
        wx, wy = [], []
        for b in bats:
            if b == held:
                continue
            a, c, _ = windows(df[df.battery == b], mu, sd)
            if len(a): wx.append(a); wy.append(c)
        WX, WY = np.concatenate(wx), np.concatenate(wy)
        TX, TY, TS = windows(te, mu, sd)

        m = build_lstm(len(FEATURES))
        m.compile(optimizer=keras.optimizers.Adam(3e-3), loss="mse")
        m.fit(WX, WY, epochs=200, batch_size=32, verbose=0,
              callbacks=[keras.callbacks.EarlyStopping(
                  monitor="loss", patience=25, restore_best_weights=True)])
        lstm_pred = m.predict(TX, verbose=0).ravel()

        # LSTM chỉ dự đoán được từ chu kỳ thứ WINDOW trở đi -> so trên cùng đoạn
        off = len(yte) - len(TY)
        for name, p in preds.items():
            preds[name] = p[off:]
        preds["M  LSTM cua so 10"] = lstm_pred
        yte_al = yte[off:]

        life = yte_al.max() + 1
        early = (yte_al >= 0.5 * life) & (yte_al <= 0.75 * life)   # 25–50% vòng đời

        for name, p in preds.items():
            rmse = float(np.sqrt(np.mean((p - yte_al) ** 2)))
            mae = float(np.mean(np.abs(p - yte_al)))
            emae = float(np.mean(np.abs(p[early] - yte_al[early]))) if early.any() else np.nan
            res[name].append((held, rmse, mae, emae))

        # ---- SOH bằng chính Ridge, gần như miễn phí và đây mới là thứ khách
        #      hàng định giá xe cũ cần ----
        rs = Ridge(alpha=1.0).fit(Xtr, tr["soh"].to_numpy(np.float32))
        ps = rs.predict(Xte)
        soh_res.append((held, float(np.sqrt(np.mean((ps - te["soh"].to_numpy()) ** 2)))))

    # ---------------------------------------------------------------- báo cáo
    print(f"\n{'='*86}")
    print("RUL — leave-one-battery-out (sai số tính bằng SỐ CHU KỲ, càng nhỏ càng tốt)")
    print(f"{'='*86}")
    hdr = f"{'phương pháp':24}" + "".join(f"{b:>10}" for b in bats) + f"{'TB MAE':>11}"
    print(hdr); print("-" * len(hdr))
    for name in methods:
        maes = [r[2] for r in res[name]]
        row = "".join(f"{r[2]:9.1f}" for r in res[name])
        print(f"  {name:22}{row}{np.mean(maes):10.1f}")

    print(f"\n{'sai số dự đoán SỚM (ở 25–50% vòng đời) — con số quan trọng nhất':<86}")
    print("-" * len(hdr))
    for name in methods:
        es = [r[3] for r in res[name] if np.isfinite(r[3])]
        row = "".join(f"{r[3]:9.1f}" if np.isfinite(r[3]) else f"{'--':>9}"
                      for r in res[name])
        print(f"  {name:22}{row}{np.mean(es) if es else float('nan'):10.1f}")

    print(f"\nSOH (Ridge) — RMSE theo tỉ lệ dung lượng còn lại:")
    for b, r in soh_res:
        print(f"  {b}: {r:.4f}  (~{100*r:.1f} điểm phần trăm)")
    print(f"  trung bình: {np.mean([r for _, r in soh_res]):.4f}")

    best = min(methods, key=lambda m: np.mean([r[2] for r in res[m]]))
    print(f"\nTốt nhất theo MAE: {best}")
    MODELS.mkdir(exist_ok=True)
    np.savez(MODELS / "rul_results.npz",
             **{m: np.array([(r[1], r[2], r[3]) for r in res[m]]) for m in methods})
    print(f"đã lưu {MODELS/'rul_results.npz'}")


if __name__ == "__main__":
    main()
