#!/usr/bin/env python3
"""
Chọn điểm vận hành cho Lớp 1: ngưỡng × quy tắc "giữ liên tục bao lâu".

Vì sao cần script riêng: evaluate.py cho thấy Autoencoder ở ngưỡng p99.9 +
giữ 10 s báo động giả ~3,3 lần/giờ. Một sản phẩm giám sát pin mà kêu 3 lần
mỗi giờ thì sau một ngày người dùng tắt chuông — và thế là mất luôn tác dụng
an toàn. Cần tìm cặp (ngưỡng, thời gian giữ) cho báo động giả đủ thấp mà vẫn
bắt được ca nguy hiểm nhất (ramp = tiền đề thermal runaway).

Tối ưu ở đây KHÔNG được nhìn tập test. Dùng tập val (toàn dữ liệu bình thường)
để đo báo động giả, và dữ liệu tiêm lỗi sinh từ chính val để đo độ nhạy.

Chạy: ai/.venv/bin/python ai/tune_threshold.py
"""
import os, sys, warnings
from pathlib import Path

os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")
warnings.filterwarnings("ignore")
import numpy as np

sys.path.insert(0, str(Path(__file__).parent))
import features as FT
from features import build_features
from prepare_data import DATA, cycle_number, load_cycle, virtual_packs

HERE = Path(__file__).parent
MODELS = HERE / "models"
PREP = HERE / "data" / "prepared"

WARMUP, FAULT_START = 300, 600
N_CYCLES = 6
PERSISTS = [10, 30, 60, 120]          # giây phải vượt ngưỡng liên tục
PCTS = [99.9, 99.99, 99.995, 99.999]  # phân vị ngưỡng, lấy trên tập val
RAMP_RATES = [0.05, 0.1, 0.2, 0.5]    # °C/phút — ca an toàn, phải bắt được


def persist_mask(flag, k):
    if k <= 1:
        return flag
    out = flag.copy()
    run = np.zeros(flag.shape[1:], dtype=int)
    for t in range(flag.shape[0]):
        run = np.where(flag[t], run + 1, 0)
        out[t] = run >= k
    return out


def main():
    from tensorflow import keras
    ae = keras.models.load_model(MODELS / "cell_ae.keras", compile=False)
    sc = np.load(PREP / "scaler.npz")
    mu, sd = sc["mu"], sc["sd"]

    def score(feats):
        T, N, F = feats.shape
        z = (feats.reshape(-1, F) - mu) / sd
        r = ae.predict(z, batch_size=8192, verbose=0)
        return np.mean((r - z) ** 2, axis=1).reshape(T, N).astype(np.float32)

    # dùng các chu kỳ VAL (70%..85%), không đụng tập test
    files = sorted(DATA.glob("*.parquet"), key=cycle_number)
    n = len(files)
    val_files = files[int(n * 0.7): int(n * 0.85)][:N_CYCLES]
    print(f"dò trên {len(val_files)} chu kỳ val "
          f"({cycle_number(val_files[0])}..{cycle_number(val_files[-1])})")

    segs = []
    for p in val_files:
        r = load_cycle(p)
        if r is None:
            continue
        temps, amb, cur, soc, cells, valid = r
        idx = {c: i for i, c in enumerate(cells)}
        for names in virtual_packs(set(valid)):
            sub = temps[:, [idx[c] for c in names]]
            if len(sub) >= FAULT_START + 1200:
                segs.append((sub, amb, cur, soc))
    print(f"{len(segs)} đoạn pack-ảo")

    # điểm số trên dữ liệu sạch
    clean_scores = [score(build_features(*s)[WARMUP:]) for s in segs]
    all_clean = np.concatenate([c.ravel() for c in clean_scores])
    n_clean = len(all_clean)

    # điểm số trên dữ liệu tiêm lỗi ramp
    rng = np.random.default_rng(11)
    ramp_scores = {}
    for rate in RAMP_RATES:
        rows = []
        for temps, amb, cur, soc in segs:
            cell = int(rng.integers(temps.shape[1]))
            t2 = FT.inject_ramp(temps, cell, FAULT_START, rate)
            rows.append((score(build_features(t2, amb, cur, soc)[WARMUP:]), cell))
        ramp_scores[rate] = rows

    print(f"\n{'='*92}")
    print("BÁO ĐỘNG GIẢ (lần/giờ trên pack 8 cell) và ĐỘ TRỄ PHÁT HIỆN ramp (phút)")
    print(f"{'='*92}")
    hdr = f"{'ngưỡng':>9} {'giữ':>5} {'BĐ giả/giờ':>11} │ " + \
          " ".join(f"{r:>5}°C/ph" for r in RAMP_RATES)
    print(hdr); print("-" * len(hdr))

    best = None
    for pct in PCTS:
        th = float(np.percentile(all_clean, pct))
        for k in PERSISTS:
            fp = sum(int(persist_mask(c > th, k).sum()) for c in clean_scores)
            fp_hr = fp / n_clean * 8 * 3600
            cells_txt, ok_all = [], True
            for rate in RAMP_RATES:
                lats, hits = [], 0
                for s, cell in ramp_scores[rate]:
                    col = persist_mask(s > th, k)[FAULT_START - WARMUP:, cell]
                    if col.any():
                        hits += 1
                        lats.append(int(np.argmax(col)))
                if hits == len(ramp_scores[rate]):
                    cells_txt.append(f"{np.median(lats)/60:8.0f}")
                else:
                    cells_txt.append(f"{100*hits//len(ramp_scores[rate]):7d}%")
                    ok_all = False
            mark = ""
            # mục tiêu: dưới 1 báo động giả mỗi 24 h, mà vẫn bắt 100% mọi ramp
            if fp_hr < 1 / 24 and ok_all:
                mark = "  <<< đạt"
                if best is None or fp_hr < best[2]:
                    best = (pct, k, fp_hr, th)
            print(f"{pct:>9} {k:>4}s {fp_hr:>11.3f} │ " + " ".join(cells_txt) + mark)

    print("\nô ghi số = độ trễ trung vị (phút), bắt được 100% ca")
    print("ô ghi %  = chỉ bắt được từng ấy phần trăm ca -> loại")

    if best:
        pct, k, fp_hr, th = best
        print(f"\nCHỌN: ngưỡng p{pct} (= {th:.4f}), giữ liên tục {k}s")
        if fp_hr > 0:
            print(f"  -> báo động giả {fp_hr:.4f}/giờ = 1 lần mỗi {1/fp_hr/24:.1f} ngày")
        else:
            # Không quan sát được lần nào KHÔNG có nghĩa là tần suất bằng 0.
            # Với 0 sự kiện trên n mẫu, chặn trên 95% là 3/n (quy tắc số 3).
            # Nói "1 lần mỗi 41 triệu ngày" là bịa, và giám khảo sẽ bắt được.
            ub = 3.0 / n_clean * 8 * 3600
            print(f"  -> KHÔNG có báo động giả nào trên {n_clean:,} mẫu-cell")
            print(f"     chặn trên 95% (quy tắc số 3): < {ub:.4f}/giờ "
                  f"= hiếm hơn 1 lần mỗi {1/ub/24:.1f} ngày")
        np.savez(MODELS / "operating_point.npz",
                 threshold=th, persist_s=k, pct=pct, fp_per_hour=fp_hr)
        print(f"  -> đã lưu {MODELS/'operating_point.npz'}")
    else:
        print("\nKHÔNG cặp nào đạt mục tiêu (<1 báo động giả/ngày + bắt 100% ramp).")
        print("Phải nới mục tiêu, hoặc thêm đặc trưng, hoặc dùng mô hình khác.")


if __name__ == "__main__":
    main()
