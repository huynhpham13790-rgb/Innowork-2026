#!/usr/bin/env python3
"""
Đánh giá Lớp 1: Autoencoder ĐẤU với 3 baseline đơn giản.

KHUNG_NGHIEN_CUU §4 yêu cầu đúng việc này, và nói thẳng: "Nếu Autoencoder
không thắng, phải biết TRƯỚC khi lên sân khấu." Script này để trả lời câu đó
một cách trung thực, kể cả khi câu trả lời không đẹp.

Baseline:
  B1. Ngưỡng cứng 60 °C                  — cách mọi BMS đang làm
  B2. Lệch so với trung bình pack > 5 °C — cách "thông minh" rẻ tiền nhất
  B3. Isolation Forest                   — máy học cổ điển, không cần nhãn

Lỗi được tiêm tổng hợp vào dữ liệu test (dữ liệu gốc toàn bình thường nên
không có nhãn thật). Ba kiểu, mô phỏng ba cơ chế hỏng khác nhau:
  offset — tiếp xúc kém/điện trở cao: cell lệch hẳn lên một mức rồi giữ nguyên
  ramp   — tiền đề thermal runaway: nhiệt tăng không dừng
  drift  — lão hoá: trôi rất chậm suốt cả chu kỳ, khó thấy nhất

QUY TẮC CHỐNG BÁO ĐỘNG GIẢ: một mẫu vượt ngưỡng chưa tính là báo động. Phải
vượt liên tục PERSIST giây. Thực tế cảm biến luôn có nhiễu, không có quy tắc
này thì mọi phương pháp đều báo động liên tục.

Chạy: ai/.venv/bin/python ai/evaluate.py
"""
import os, sys, warnings
from pathlib import Path

os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")
warnings.filterwarnings("ignore")
import numpy as np

sys.path.insert(0, str(Path(__file__).parent))
from features import build_features
from prepare_data import DATA, cycle_number, load_cycle, virtual_packs
import features as FT

HERE = Path(__file__).parent
MODELS = HERE / "models"
PREP = HERE / "data" / "prepared"

PERSIST = 10          # số giây phải vượt ngưỡng liên tục mới tính là báo động
WARMUP = 300          # bỏ 5 phút đầu (EMA chưa ổn định)
FAULT_START = 600     # tiêm lỗi từ giây thứ 600
MAGNITUDES = [0.5, 1.0, 2.0, 3.0, 5.0]   # °C — cho ra đường "phát hiện theo độ lớn"
MAX_TEST_CYCLES = 8


def persist_mask(flag, k):
    """True tại t nếu flag đúng liên tục trong k bước tính tới t."""
    if k <= 1:
        return flag
    out = flag.copy()
    run = np.zeros(flag.shape[1:], dtype=int)
    for t in range(flag.shape[0]):
        run = np.where(flag[t], run + 1, 0)
        out[t] = run >= k
    return out


class Detectors:
    def __init__(self):
        import tensorflow as tf
        from tensorflow import keras
        self.ae = keras.models.load_model(MODELS / "cell_ae.keras", compile=False)
        sc = np.load(PREP / "scaler.npz")
        self.mu, self.sd = sc["mu"], sc["sd"]
        th = np.load(MODELS / "thresholds.npz")
        self.ae_th = float(th["p99.9"])
        self.iforest = None

    def ae_score(self, feats):
        """feats (T,N,16) -> sai số tái tạo (T,N)"""
        T, N, F = feats.shape
        z = (feats.reshape(-1, F) - self.mu) / self.sd
        rec = self.ae.predict(z, batch_size=8192, verbose=0)
        return np.mean((rec - z) ** 2, axis=1).reshape(T, N)

    def fit_iforest(self, feats_normal):
        from sklearn.ensemble import IsolationForest
        X = feats_normal.reshape(-1, feats_normal.shape[-1])
        if len(X) > 200_000:
            X = X[np.random.default_rng(0).choice(len(X), 200_000, replace=False)]
        self.iforest = IsolationForest(n_estimators=100, contamination=0.001,
                                       random_state=0, n_jobs=-1)
        self.iforest.fit((X - self.mu) / self.sd)

    def iforest_score(self, feats):
        T, N, F = feats.shape
        z = (feats.reshape(-1, F) - self.mu) / self.sd
        return (-self.iforest.score_samples(z)).reshape(T, N)


def evaluate():
    files = sorted(DATA.glob("*.parquet"), key=cycle_number)
    n = len(files)
    test_files = files[int(n * 0.85):][:MAX_TEST_CYCLES]
    print(f"đánh giá trên {len(test_files)} chu kỳ test (chu kỳ "
          f"{cycle_number(test_files[0])}..{cycle_number(test_files[-1])})")

    det = Detectors()

    # ---- gom dữ liệu sạch của tập test để (a) fit IForest (b) đo báo động giả
    # Máy dev chỉ còn ~2,5 GB RAM nên KHÔNG giữ đặc trưng của mọi đoạn cùng lúc
    # (60 đoạn × 20.000 giây × 8 cell × 16 = ~600 MB, chưa kể IForest nhân đôi).
    # Đổi lại: tính đặc trưng vài lần, mỗi lần chỉ giữ đúng một đoạn.
    clean_sets = []
    for path in test_files:
        r = load_cycle(path)
        if r is None:
            continue
        temps, amb, cur, soc, cells, valid = r
        idx = {c: i for i, c in enumerate(cells)}
        for names in virtual_packs(set(valid)):
            sub = temps[:, [idx[c] for c in names]]
            if len(sub) < FAULT_START + 600:
                continue
            clean_sets.append((sub, amb, cur, soc))
    print(f"{len(clean_sets)} đoạn pack-ảo dùng được")
    if not clean_sets:
        sys.exit("không có đoạn nào đủ dài")

    # Lượt 0 — lấy mẫu thưa để fit IsolationForest
    rng0 = np.random.default_rng(0)
    samp = []
    for c in clean_sets:
        f = build_features(*c)[WARMUP::20].reshape(-1, 16)
        samp.append(f[rng0.choice(len(f), min(4000, len(f)), replace=False)])
    det.fit_iforest(np.concatenate(samp)[:, None, :])
    del samp

    # Lượt 0b — ngưỡng IForest lấy trên chính dữ liệu sạch, cùng phân vị p99.9
    # như AE, để so sánh công bằng chứ không ưu ái bên nào.
    if_scores = []
    for c in clean_sets:
        f = build_features(*c)[WARMUP::20]
        if_scores.append(det.iforest_score(f).ravel())
    if_th = float(np.percentile(np.concatenate(if_scores), 99.9))
    del if_scores

    methods = ["AE (Autoencoder)", "B1 nguong cung 60C", "B2 lech TB >5C", "B3 IsolationForest"]

    def flags_for(feats, temps):
        """Trả về dict method -> mảng cờ báo động (T,N) đã áp quy tắc liên tục."""
        t = temps[WARMUP:]
        dev = t - t.mean(axis=1, keepdims=True)
        raw = {
            "AE (Autoencoder)":   det.ae_score(feats) > det.ae_th,
            "B1 nguong cung 60C": t > 60.0,
            "B2 lech TB >5C":     dev > 5.0,
            "B3 IsolationForest": det.iforest_score(feats) > if_th,
        }
        return {k: persist_mask(v, PERSIST) for k, v in raw.items()}

    # ---- báo động giả trên dữ liệu sạch ---------------------------------
    fp = {m: 0 for m in methods}
    n_clean = 0
    for c in clean_sets:
        feats = build_features(*c)[WARMUP:]      # chỉ giữ 1 đoạn tại một thời điểm
        f = flags_for(feats, c[0])
        for m in methods:
            fp[m] += int(f[m].sum())
        n_clean += feats.shape[0] * feats.shape[1]
        del feats, f

    # ---- phát hiện trên dữ liệu đã tiêm lỗi ------------------------------
    rng = np.random.default_rng(7)
    results = {m: {} for m in methods}
    for kind in ("offset", "ramp", "drift"):
        for mag in MAGNITUDES:
            det_cnt = {m: 0 for m in methods}
            lat = {m: [] for m in methods}
            total = 0
            for temps, amb, cur, soc in clean_sets:
                cell = int(rng.integers(temps.shape[1]))
                if kind == "offset":
                    t2 = FT.inject_offset(temps, cell, FAULT_START, mag)
                elif kind == "ramp":
                    # mag °C mỗi 10 phút -> tốc độ tăng chậm, giống tiền TR
                    t2 = FT.inject_ramp(temps, cell, FAULT_START, mag / 10.0)
                else:
                    t2 = FT.inject_drift(temps, cell, mag)
                f2 = build_features(t2, amb, cur, soc)[WARMUP:]
                fl = flags_for(f2, t2)
                del f2, t2
                on = FAULT_START - WARMUP          # chỉ số bắt đầu lỗi
                total += 1
                for m in methods:
                    col = fl[m][on:, cell]
                    if col.any():
                        det_cnt[m] += 1
                        lat[m].append(int(np.argmax(col)))
            for m in methods:
                results[m][(kind, mag)] = (
                    det_cnt[m] / max(total, 1),
                    float(np.median(lat[m])) if lat[m] else float("nan"),
                )

    # ---- in bảng ----------------------------------------------------------
    print(f"\n{'='*78}\nBÁO ĐỘNG GIẢ trên dữ liệu bình thường "
          f"({n_clean:,} mẫu-cell, phải vượt ngưỡng {PERSIST}s liên tục)\n{'='*78}")
    for m in methods:
        rate = fp[m] / n_clean
        # quy ra "bao lâu một lần" cho pack 8 cell chạy 1 Hz
        per_hour = rate * 8 * 3600
        print(f"  {m:22} {fp[m]:>8,} mẫu = {100*rate:7.4f}%   ~{per_hour:6.1f} lần/giờ")

    for kind in ("offset", "ramp", "drift"):
        title = {"offset": "OFFSET (lệch hẳn, °C)",
                 "ramp":   "RAMP (tăng dần, °C mỗi 10 phút)",
                 "drift":  "DRIFT (trôi chậm cả chu kỳ, °C)"}[kind]
        print(f"\n{'='*78}\nTỈ LỆ PHÁT HIỆN — {title}\n{'='*78}")
        print(f"  {'phương pháp':22}" + "".join(f"{m:>10}" for m in MAGNITUDES))
        for m in methods:
            row = "".join(f"{100*results[m][(kind,mag)][0]:9.0f}%" for mag in MAGNITUDES)
            print(f"  {m:22}{row}")
        print(f"  {'-- độ trễ (giây) --':22}")
        for m in methods:
            row = ""
            for mag in MAGNITUDES:
                l = results[m][(kind, mag)][1]
                row += f"{'  --':>10}" if np.isnan(l) else f"{l:9.0f}s"
            print(f"  {m:22}{row}")

    np.savez(MODELS / "eval_results.npz",
             fp={m: fp[m] for m in methods}, n_clean=n_clean,
             results={str(k): v for m in methods for k, v in results[m].items()})
    print(f"\nngưỡng dùng: AE={det.ae_th:.6f}  IForest={if_th:.4f}  "
          f"quy tắc liên tục={PERSIST}s")


if __name__ == "__main__":
    evaluate()
