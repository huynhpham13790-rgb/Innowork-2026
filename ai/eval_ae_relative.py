#!/usr/bin/env python3
"""
Đánh giá autoencoder "thuần tương đối" — kiểm CẢ HAI PHÍA.

(a) McMaster: có chuyển giao sang pack khác được không?
(b) NASA: có MẤT khả năng phát hiện không?

Phải kiểm cả hai. Một mô hình chuyển giao tốt vì nó không phát hiện được gì thì
vô dụng — đúng cái bẫy "báo oan thấp vì mù" đã ghi ở QĐ-030.

Chạy: ai/.venv/bin/python ai/eval_ae_relative.py
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
from validate_mcmaster import persist_mask, load as load_mcm, FAULT, WARMUP, PERSIST, N_VPACK, I_SCALE_MC, MCM
from train_ae_relative import REL
from evaluate import FAULT_START, MAX_TEST_CYCLES

HERE = Path(__file__).parent
MODELS = HERE / "models"
MAGNITUDES = [0.5, 1.0, 2.0, 3.0, 5.0]


def main():
    from tensorflow import keras
    ae = keras.models.load_model(MODELS / "cell_ae_rel.keras", compile=False)
    s = np.load(MODELS / "scaler_rel.npz")
    mu, sd = s["mu"], s["sd"]
    th = float(np.load(MODELS / "thresholds_rel.npz")["p99.9"])
    print(f"mo hinh thuan tuong doi: 11 dac trung, nguong p99.9 = {th:.5f}\n")

    def score(feats):
        T, N, _ = feats.shape
        z = (feats.reshape(-1, feats.shape[-1])[:, REL] - mu) / sd
        return np.mean((ae.predict(z, batch_size=8192, verbose=0) - z) ** 2, 1).reshape(T, N)

    # ---------------- (a) chuyển giao sang pack McMaster --------------------
    print("=" * 80)
    print("(a) CHUYEN GIAO: pack McMaster 72 cell, loi gay co y, ZERO-SHOT")
    print("=" * 80)
    FT.I_SCALE = I_SCALE_MC     # không còn dùng tới, nhưng giữ cho nhất quán
    print(f"{'file':<44}{'loai':<12}{'% pack-ao bao':>15}{'% thoi gian':>13}")
    for p in sorted(MCM.glob("*.mat")):
        kind, scope = FAULT.get(p.stem, ("binh thuong", "-"))
        temps, amb, cur, soc = load_mcm(p)
        if len(temps) < WARMUP + 600:
            continue
        nvp = temps.shape[1] // N_VPACK
        hit = tf_ = tp = 0
        for g in range(nvp):
            f = build_features(temps[:, g*N_VPACK:(g+1)*N_VPACK], amb, cur, soc)[WARMUP:]
            fl = persist_mask(score(f) > th, PERSIST)
            hit += int(fl.any()); tf_ += int(fl.sum()); tp += fl.size
            del f, fl
        print(f"{p.stem[:43]:<44}{scope:<12}{100*hit/nvp:>13.0f}% {100*tf_/max(tp,1):>12.3f}%")

    # ---------------- (b) có mất khả năng phát hiện trên NASA không? --------
    print("\n" + "=" * 80)
    print("(b) GIU DUOC KHA NANG PHAT HIEN? pack NASA, loi tiem tong hop")
    print("=" * 80)
    FT.I_SCALE = 20.0
    files = sorted(DATA.glob("*.parquet"), key=cycle_number)
    segs = []
    for path in files[int(len(files)*0.85):][:MAX_TEST_CYCLES]:
        r = load_cycle(path)
        if r is None:
            continue
        temps, amb, cur, soc, cells, valid = r
        idx = {c: i for i, c in enumerate(cells)}
        for names in virtual_packs(set(valid)):
            sub = temps[:, [idx[c] for c in names]]
            if len(sub) >= FAULT_START + 600:
                segs.append((sub, amb, cur, soc))
    print(f"{len(segs)} doan pack-ao\n")

    fp = n = 0
    for c in segs:
        f = build_features(*c)[WARMUP:]
        fl = persist_mask(score(f) > th, PERSIST)
        fp += int(fl.sum()); n += fl.size
        del f, fl
    print(f"bao oan tren du lieu sach: {100*fp/max(n,1):.4f}%\n")

    rng = np.random.default_rng(7)
    for kind in ("offset", "ramp", "drift"):
        print(f"--- loi kieu {kind} ---")
        for mag in MAGNITUDES:
            hit = tot = 0; lat = []
            for temps, amb, cur, soc in segs:
                cell = int(rng.integers(temps.shape[1]))
                if kind == "offset":
                    t2 = FT.inject_offset(temps, cell, FAULT_START, mag)
                elif kind == "ramp":
                    t2 = FT.inject_ramp(temps, cell, FAULT_START, mag/10.0)
                else:
                    t2 = FT.inject_drift(temps, cell, mag)
                f = build_features(t2, amb, cur, soc)[WARMUP:]
                fl = persist_mask(score(f) > th, PERSIST)
                col = fl[FAULT_START-WARMUP:, cell]
                tot += 1
                if col.any():
                    hit += 1; lat.append(int(np.argmax(col)))
                del f, fl, t2
            med = f"{np.median(lat):.0f}s" if lat else "-"
            print(f"  {mag:>4.1f} degC : {100*hit/max(tot,1):>3.0f}%  tre {med}")


if __name__ == "__main__":
    main()
