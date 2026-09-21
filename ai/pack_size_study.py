#!/usr/bin/env python3
"""
Đổi pack từ 8 cell xuống 6 cell thì Lớp 1 yếu đi bao nhiêu? (QĐ-038)

VÌ SAO PHẢI ĐO, KHÔNG ĐƯỢC SUY LUẬN
Autoencoder chạy trên TỪNG CELL với 11 đặc trưng, nên đổi số cell không đụng
gì tới kích thước mạng — chuyện đó đã chắc. Nhưng trong 11 đặc trưng có nhiều
cái tính TRÊN CẢ PACK (trung bình, trung vị, độ lệch chuẩn, thứ hạng, độ
rộng), và thống kê của chúng đổi khi số cell đổi:

  - cell lỗi nằm trong 6 con thì nó tự kéo trung bình pack lên mạnh hơn khi
    nằm trong 8 con  =>  dev = T - mean nhỏ đi  =>  tự che mình nhiều hơn
  - nó cũng tự thổi phồng độ lệch chuẩn pack nhiều hơn  =>  zscore nhỏ đi
  - độ rộng max-min của 6 mẫu lành nhỏ hơn của 8 mẫu  =>  đổi phân bố nền

Mô hình được huấn luyện với PACK_SIZE = 8 (prepare_data.py), và ngưỡng 1,0707
cùng con số "0 báo động giả" đều chốt ở N = 8. Chạy ở N = 6 là dùng ngoài điều
kiện huấn luyện — đúng cảnh báo mà sensor_count_study.py đã ghi sẵn.

THIẾT KẾ PHÉP SO SÁNH — chỗ dễ làm sai nhất
Pack 6 cell được lấy là **6 cell ĐẦU của đúng pack 8 cell đó**, không phải cắt
ở chỗ khác. Nhờ vậy mọi khác biệt đo được đều đến từ THỐNG KÊ PACK, không phải
từ việc hai bên nhìn vào những cell khác nhau. Lỗi cũng tiêm vào cùng một cell
(chỉ số < 6) để hai bên thấy đúng một sự kiện vật lý.

Chạy: <python co numpy/scipy/tensorflow> ai/pack_size_study.py
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
from validate_mcmaster import persist_mask
from train_ae_relative import REL
from evaluate import FAULT_START, MAX_TEST_CYCLES

HERE = Path(__file__).parent
MODELS = HERE / "models"

WARMUP = 300
# 30 giây, giống FIRMWARE (operating_point_rel.npz: persist_s = 30), không phải
# 10 như eval_ae_relative.py dùng. Con số nào cũng được miễn là HAI BÊN dùng
# chung, nhưng lấy đúng số của firmware thì kết quả đọc thẳng ra thực tế.
PERSIST = int(os.environ.get("PERSIST_S", 30))
SIZES = tuple(int(x) for x in os.environ.get("SIZES","8,6").split(","))
MAGNITUDES = [0.5, 1.0, 2.0, 3.0, 5.0]


def main():
    from tensorflow import keras
    ae = keras.models.load_model(MODELS / "cell_ae_rel.keras", compile=False)
    s = np.load(MODELS / "scaler_rel.npz")
    mu, sd = s["mu"], s["sd"]
    th = float(np.load(MODELS / "operating_point_rel.npz")["threshold"])
    print(f"mo hinh: 11 dac trung thuan tuong doi, nguong dang dung = {th:.5f}, "
          f"giu lien tuc {PERSIST}s\n")

    def score(feats):
        T, N, _ = feats.shape
        z = (feats.reshape(-1, feats.shape[-1])[:, REL] - mu) / sd
        return np.mean((ae.predict(z, batch_size=8192, verbose=0) - z) ** 2, 1).reshape(T, N)

    # ---- gom các đoạn pack-ảo 8 cell từ phần TEST của bộ UPC ---------------
    files = sorted(DATA.glob("*.parquet"), key=cycle_number)
    segs = []
    for path in files[int(len(files) * 0.85):][:MAX_TEST_CYCLES]:
        r = load_cycle(path)
        if r is None:
            continue
        temps, amb, cur, soc, cells, valid = r
        idx = {c: i for i, c in enumerate(cells)}
        for names in virtual_packs(set(valid)):
            sub = temps[:, [idx[c] for c in names]]
            if len(sub) >= FAULT_START + 600:
                segs.append((sub, amb, cur, soc))
    print(f"{len(segs)} doan pack-ao 8 cell tu {MAX_TEST_CYCLES} chu ky test\n")
    if not segs:
        sys.exit("khong co du lieu — kiem tra ai/data/upc_wltp/")

    FT.I_SCALE = 20.0

    # ---- (1) nền sạch: báo oan và ngưỡng mà chính dữ liệu đòi hỏi ----------
    print("=" * 74)
    print("(1) NEN SACH — bao oan o nguong dang dung, va nguong p99.9 thuc te")
    print("=" * 74)
    print(f"{'N cell':<9}{'bao oan @1.0707':>18}{'p99.9 cua diem':>18}{'p99.99':>12}")
    base = {}
    for N in SIZES:
        fp = tot = 0
        pool = []
        for temps, amb, cur, soc in segs:
            f = build_features(temps[:, :N], amb, cur, soc)[WARMUP:]
            sc = score(f)
            fl = persist_mask(sc > th, PERSIST)
            fp += int(fl.sum()); tot += fl.size
            # lấy thưa để giữ RAM: phân vị không cần toàn bộ điểm
            pool.append(sc.ravel()[::7])
            del f, sc, fl
        allsc = np.concatenate(pool)
        p999, p9999 = np.percentile(allsc, [99.9, 99.99])
        base[N] = dict(fp=100*fp/max(tot,1), p999=p999, p9999=p9999, n=tot)
        print(f"{N:<9}{base[N]['fp']:>17.4f}%{p999:>18.5f}{p9999:>12.5f}")

    # ---- (2) phát hiện lỗi, cùng một cell, cùng một biên độ ----------------
    print("\n" + "=" * 74)
    print("(2) PHAT HIEN LOI — tiem vao CUNG MOT CELL o ca hai cau hinh")
    print("=" * 74)
    rng = np.random.default_rng(7)
    # chọn trước cell lỗi cho từng đoạn, chỉ số < 6 để pack 6 cell cũng chứa nó
    faulty = [int(rng.integers(6)) for _ in segs]

    for kind in ("offset", "ramp", "drift"):
        print(f"\n--- loi kieu {kind} ---")
        print(f"{'bien do':<10}" + "".join(f"{'N='+str(N):>22}" for N in SIZES))
        for mag in MAGNITUDES:
            cols = []
            for N in SIZES:
                hit = tot = 0; lat = []
                for (temps, amb, cur, soc), cell in zip(segs, faulty):
                    t = temps[:, :N]
                    if kind == "offset":
                        t2 = FT.inject_offset(t, cell, FAULT_START, mag)
                    elif kind == "ramp":
                        t2 = FT.inject_ramp(t, cell, FAULT_START, mag / 10.0)
                    else:
                        t2 = FT.inject_drift(t, cell, mag)
                    f = build_features(t2, amb, cur, soc)[WARMUP:]
                    fl = persist_mask(score(f) > th, PERSIST)
                    col = fl[FAULT_START - WARMUP:, cell]
                    tot += 1
                    if col.any():
                        hit += 1; lat.append(int(np.argmax(col)))
                    del f, fl, t2
                med = f"{np.median(lat):.0f}s" if lat else "-"
                cols.append(f"{100*hit/max(tot,1):>10.0f}%  tre {med:>6}")
            print(f"{mag:<10.1f}" + "".join(f"{c:>22}" for c in cols))

    print("\n" + "=" * 74)
    print("Doc ket qua:")
    print("  - Neu bao oan o N=6 van ~0 va ti le phat hien tut it  -> giu nguyen 1.0707")
    print("  - Neu bao oan tang  -> nang nguong len p99.9 cua N=6 roi do lai phat hien")
    print("  - Neu phat hien tut nhieu o bien do nho -> phai huan luyen lai voi N=6")
    print("=" * 74)


if __name__ == "__main__":
    main()
