#!/usr/bin/env python3
"""
Biến parquet UPC thành tập huấn luyện cho Lớp 1.

Các bước, và vì sao:
  1. Gộp Top+Bottom mỗi cell thành 1 giá trị  -> giả lập 1 con DS18B20/cell
  2. Hạ 2,5 Hz xuống 1 Hz                      -> đúng nhịp đọc DS18B20
  3. Lượng tử hoá về bước 0,0625 °C            -> đúng độ phân giải 12 bit
  4. Cắt pack 36 cell thành các "pack ảo 8 cell" liền kề
                                               -> đúng cấu hình 8S của đội
  5. Trích 16 đặc trưng tương đối cho từng cell (features.py)

Bước 4 quan trọng hơn vẻ ngoài của nó: thống kê pack (trung bình, độ lệch
chuẩn) tính trên 8 cell nhiễu hơn hẳn tính trên 36 cell. Train trên pack 36
rồi chạy trên pack 8 là mô hình gặp phân phối khác hẳn lúc học. Cắt thành
pack ảo 8 cell ngay từ khâu dữ liệu thì train và deploy khớp nhau.

Chạy: ai/.venv/bin/python ai/prepare_data.py
"""
import sys, warnings
from pathlib import Path
warnings.filterwarnings("ignore", message="Mean of empty slice")

import numpy as np
import pandas as pd

sys.path.insert(0, str(Path(__file__).parent))
from features import DS18B20_STEP, build_features

DATA = Path(__file__).parent / "data" / "upc_wltp"
OUT = Path(__file__).parent / "data" / "prepared"
PACK_SIZE = 8

# Dải nhiệt hợp lệ của pack thí nghiệm trong buồng nhiệt. Ngoài dải này là
# cảm biến hỏng chứ không phải pin nóng: bộ UPC có kênh P3S11 bị đứt, giá trị
# nhảy về ~0 hoặc bão hoà ~655,35 (= 0xFFFF/100 của thanh ghi 16 bit).
# Không lọc thì autoencoder học luôn cả lỗi cảm biến và coi đó là "bình thường".
T_VALID_LO, T_VALID_HI = 5.0, 80.0
BAD_CHANNEL_FRAC = 0.005    # kênh lỗi quá 0,5% số mẫu -> bỏ cả kênh trong chu kỳ đó

WARMUP = 300        # bỏ 5 phút đầu mỗi chu kỳ: EMA/cửa sổ trượt chưa ổn định
TIME_STRIDE = 10    # lấy 10 giây một mẫu

# Giới hạn số mẫu mỗi tập. Lấy hết thì 412 chu kỳ × 20.000 giây × 5 pack ảo
# × 8 cell ≈ 330 triệu mẫu ≈ 21 GB -> hết RAM. Mà autoencoder này chỉ có ~250
# tham số: vài triệu mẫu đã thừa, thêm nữa chỉ tốn thời gian chứ không tốt hơn.
# Vẫn đi qua TOÀN BỘ 412 chu kỳ để phủ hết dải lão hoá, chỉ lấy thưa ra thôi.
SAMPLE_CAP = {"train": 6_000_000, "val": 1_500_000, "test": 1_500_000}


def _subsample(chunks, metas, cap, rng):
    """Gộp rồi rút ngẫu nhiên xuống còn tối đa `cap` mẫu."""
    X = np.concatenate(chunks)
    M = np.concatenate(metas)
    if len(X) > cap:
        keep = rng.choice(len(X), cap, replace=False)
        keep.sort()                      # giữ thứ tự thời gian cho dễ soi
        X, M = X[keep], M[keep]
    return [X], [M]


def virtual_packs(valid_cells):
    """
    Cắt pack 36 cell thành các pack ảo 8 cell LIỀN KỀ trong cùng một nhánh.
    Cell cạnh nhau thì truyền nhiệt cho nhau — cắt ngẫu nhiên khắp pack sẽ tạo
    ra quan hệ nhiệt không có thật. Cửa sổ nào dính kênh hỏng thì bỏ.
    """
    packs = []
    for p in (1, 2, 3):
        for start in (1, 5):
            names = [f"P{p}S{s}" for s in range(start, start + PACK_SIZE)]
            if all(n in valid_cells for n in names):
                packs.append(names)
    return packs


def load_cycle(path):
    """Đọc 1 file parquet -> (temps 1Hz, ambient, current, soc) hoặc None."""
    df = pd.read_parquet(path)
    if "Timestamp" not in df.columns or len(df) < 300:
        return None

    cells = [f"P{p}S{s}" for p in (1, 2, 3) for s in range(1, 13)]
    top = [f"Temperature_Cell_Top_{c} [degC]" for c in cells]
    bot = [f"Temperature_Cell_Bottom_{c} [degC]" for c in cells]
    need = top + bot + ["Temperature_IN_Chamber [degC]",
                        "Current_Actual_Battery [A]",
                        "SoC_Actual_Battery [percent]"]
    if any(c not in df.columns for c in need):
        return None

    d = df[["Timestamp"] + need].set_index("Timestamp")
    d = d.resample("1s").mean().dropna()           # bước 2: hạ về 1 Hz
    if len(d) < 300:
        return None

    t_top = np.array(d[top].to_numpy(np.float32), copy=True)
    t_bot = np.array(d[bot].to_numpy(np.float32), copy=True)

    # Lọc cảm biến hỏng TRƯỚC khi gộp Top/Bottom — nếu gộp trước thì một kênh
    # hỏng sẽ kéo lệch giá trị của cell đó mà nhìn vào không thấy bất thường.
    for arr in (t_top, t_bot):
        arr[(arr < T_VALID_LO) | (arr > T_VALID_HI)] = np.nan

    temps = np.nanmean(np.stack([t_top, t_bot]), axis=0)   # bước 1: 1 cảm biến/cell

    # Kênh nào hỏng quá nhiều thì loại hẳn khỏi chu kỳ này
    bad_frac = np.isnan(temps).mean(axis=0)
    valid = [c for c, f in zip(cells, bad_frac) if f <= BAD_CHANNEL_FRAC]
    if len(valid) < PACK_SIZE:
        return None

    # Vài mẫu lỗi lẻ tẻ còn lại: lấp bằng giá trị liền trước (nhân quả, đúng
    # như firmware sẽ làm khi DS18B20 trả về lỗi CRC).
    df_t = pd.DataFrame(temps).ffill().bfill()
    temps = df_t.to_numpy(np.float32)

    # Chỉ đòi hỏi các kênh CÒN DÙNG phải sạch. Kênh đã bị loại thì để nguyên
    # NaN cũng được — virtual_packs() không bao giờ chọn tới nó. Kiểm tra trên
    # cả 36 kênh sẽ vứt oan mọi chu kỳ có 1 cảm biến chết hẳn (mất ~3/4 dữ liệu).
    vi = [i for i, c in enumerate(cells) if c in set(valid)]
    if not np.isfinite(temps[:, vi]).all():
        return None

    temps = np.round(temps / DS18B20_STEP) * DS18B20_STEP   # bước 3

    return (temps,
            d["Temperature_IN_Chamber [degC]"].to_numpy(np.float32),
            d["Current_Actual_Battery [A]"].to_numpy(np.float32),
            d["SoC_Actual_Battery [percent]"].to_numpy(np.float32),
            cells, valid)


def cycle_number(path):
    """Qtzl_Cycle_007_WLTP_partial_data.parquet -> 7"""
    try:
        return int(path.stem.split("_")[2])
    except (IndexError, ValueError):
        return -1


def main():
    files = sorted(DATA.glob("*.parquet"), key=cycle_number)
    if not files:
        sys.exit(f"Chưa có dữ liệu ở {DATA} — chạy download_upc.py trước")
    print(f"{len(files)} file, chu kỳ {cycle_number(files[0])}..{cycle_number(files[-1])}")

    # Chia theo CHU KỲ chứ không chia ngẫu nhiên theo dòng: dữ liệu chuỗi thời
    # gian mà chia ngẫu nhiên thì mẫu train và test cách nhau 1 giây, mô hình
    # chỉ việc nội suy -> điểm test đẹp giả tạo.
    n = len(files)
    splits = {"train": files[: int(n * 0.7)],
              "val":   files[int(n * 0.7): int(n * 0.85)],
              "test":  files[int(n * 0.85):]}

    OUT.mkdir(parents=True, exist_ok=True)
    rng = np.random.default_rng(42)
    for name, group in splits.items():
        cap = SAMPLE_CAP[name]
        per_cycle = max(cap // max(len(group), 1), 1000)
        X, meta = [], []
        for k, path in enumerate(group):
            r = load_cycle(path)
            if r is None:
                continue
            temps, amb, cur, soc, cells, valid = r
            idx = {c: i for i, c in enumerate(cells)}
            for names in virtual_packs(set(valid)):
                sub = temps[:, [idx[c] for c in names]]
                f = build_features(sub, amb, cur, soc)     # (T, 8, 16)
                # bỏ 5 phút đầu: EMA và cửa sổ trượt chưa ổn định
                f = f[WARMUP::TIME_STRIDE]
                X.append(f.reshape(-1, f.shape[-1]))
                meta.append(np.full(len(f) * PACK_SIZE, cycle_number(path)))
            # Gom dần rồi rút gọn, để không bao giờ giữ quá nhiều trong RAM.
            if sum(len(a) for a in X) > per_cycle * (k + 1) * 2:
                X, meta = _subsample(X, meta, per_cycle * (k + 1), rng)
            if (k + 1) % 40 == 0:
                print(f"    {name}: {k+1}/{len(group)} chu kỳ, "
                      f"{sum(len(a) for a in X):,} mẫu")
        if not X:
            print(f"  {name}: KHÔNG có dữ liệu hợp lệ")
            continue
        X, meta = _subsample(X, meta, cap, rng)
        X = X[0].astype(np.float32) if len(X) == 1 else np.concatenate(X).astype(np.float32)
        meta = meta[0] if len(meta) == 1 else np.concatenate(meta)
        np.savez_compressed(OUT / f"{name}.npz", X=X, cycle=meta)
        print(f"  {name}: {X.shape[0]:>9,} mẫu × {X.shape[1]} đặc trưng  "
              f"({len(group)} chu kỳ)")

    # Chuẩn hoá: CHỈ tính trên tập train, rồi áp cho val/test.
    # Tính trên toàn bộ dữ liệu là rò rỉ thông tin từ test sang train.
    tr = np.load(OUT / "train.npz")["X"]
    mu, sd = tr.mean(0), tr.std(0) + 1e-6
    np.savez(OUT / "scaler.npz", mu=mu, sd=sd)
    print(f"\nscaler lưu từ tập train: mu[0]={mu[0]:.3f} sd[0]={sd[0]:.3f}")


if __name__ == "__main__":
    main()
