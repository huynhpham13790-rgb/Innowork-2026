#!/usr/bin/env python3
"""
Cần bao nhiêu cảm biến nhiệt? — đo cái giá phải trả khi bớt cảm biến.

VÌ SAO CÓ SCRIPT NÀY
Trong đội đang có tranh luận: 8 cảm biến cho 8 cell thì vướng víu và không kinh
tế; chia mỗi cảm biến cho 2 hoặc 4 cell có được không? Cả hai phía đều đúng ở
một điểm. Câu "gắn 8 con cho chắc" không phải một lập luận, và câu "bớt đi cho
gọn" cũng vậy. Script này biến tranh luận thành một con số: **bớt cảm biến thì
tỉ lệ phát hiện tụt bao nhiêu, ở mức lỗi nào.**

MÔ HÌNH VẬT LÝ
Một cảm biến dán giữa k cell đọc được xấp xỉ TRUNG BÌNH của k cell đó. Nên lỗi
nằm gọn trong một cell bị pha loãng đúng k lần tại điểm đo. Lỗi vẫn được tiêm
vào cell thật trước, rồi mới lấy trung bình nhóm — đúng thứ tự vật lý, chứ
không phải tiêm thẳng vào giá trị nhóm (làm thế là tự cho mình điểm cao).

BA CHỖ MÔ HÌNH NÀY CÒN LẠC QUAN — phải nói ra, đừng để giám khảo tìm ra hộ:
 1. Trung bình là trường hợp TỐT. Cảm biến thật dán lệch về một cell sẽ nhạy
    với cell đó và gần như mù với cell kia — tệ hơn trung bình.
 2. Autoencoder được huấn luyện trên thống kê của pack 8 kênh. Chạy nó với 2
    kênh là dùng ngoài điều kiện huấn luyện; "trung bình pack" của 2 kênh nhiễu
    hơn hẳn. Kết quả N=2 vì thế nên đọc như giới hạn trên.
 3. Bỏ qua dẫn nhiệt giữa các cell, vốn còn làm nhoè lỗi thêm nữa.

VÀ MỘT THỨ BẢNG NÀY KHÔNG ĐO ĐƯỢC: khả năng CHỈ ĐÍCH DANH. 8 cảm biến chỉ ra
đúng 1 trong 8 cell. 2 cảm biến chỉ thu hẹp được còn 4 cell — tức là vẫn phải
tháo cả cụm ra dò tay. Cột "chỉ đích danh" ở cuối bảng là chỗ đó.

Chạy: ai/.venv/bin/python ai/sensor_count_study.py
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
from evaluate import persist_mask, PERSIST, WARMUP, FAULT_START, MAX_TEST_CYCLES

HERE = Path(__file__).parent
MODELS = HERE / "models"
PREP = HERE / "data" / "prepared"

CONFIGS = [8, 4, 2]                     # số cảm biến trên pack 8 cell
MAGNITUDES = [0.5, 1.0, 2.0, 3.0, 5.0]  # °C


def group_avg(temps, n_sensors):
    """(T,8) -> (T,n_sensors): mỗi cảm biến đọc trung bình nhóm cell liền kề.

    Nhóm theo cell LIỀN KỀ vì đó là cách duy nhất lắp được trong thực tế — một
    cảm biến không thể vừa chạm cell 1 vừa chạm cell 8."""
    T, N = temps.shape
    k = N // n_sensors
    return temps.reshape(T, n_sensors, k).mean(axis=2)


def main():
    import tensorflow as tf
    from tensorflow import keras
    ae = keras.models.load_model(MODELS / "cell_ae.keras", compile=False)
    sc = np.load(PREP / "scaler.npz")
    mu, sd = sc["mu"], sc["sd"]
    ae_th = float(np.load(MODELS / "thresholds.npz")["p99.9"])

    def score(feats):
        T, N, F = feats.shape
        z = (feats.reshape(-1, F) - mu) / sd
        rec = ae.predict(z, batch_size=8192, verbose=0)
        return np.mean((rec - z) ** 2, axis=1).reshape(T, N)

    files = sorted(DATA.glob("*.parquet"), key=cycle_number)
    test_files = files[int(len(files) * 0.85):][:MAX_TEST_CYCLES]

    segs = []
    for path in test_files:
        r = load_cycle(path)
        if r is None:
            continue
        temps, amb, cur, soc, cells, valid = r
        idx = {c: i for i, c in enumerate(cells)}
        for names in virtual_packs(set(valid)):
            sub = temps[:, [idx[c] for c in names]]
            if sub.shape[1] == 8 and len(sub) >= FAULT_START + 600:
                segs.append((sub, amb, cur, soc))
    print(f"{len(segs)} đoạn pack-ảo 8 cell dùng được "
          f"(chu kỳ {cycle_number(test_files[0])}..{cycle_number(test_files[-1])})")
    if not segs:
        sys.exit("không có đoạn nào đủ dài")

    # --- báo động giả: bớt cảm biến có làm hệ kêu oan nhiều hơn không? -------
    # Phải đo, vì một cấu hình "phát hiện tốt" mà kêu oan liên tục thì vô dụng:
    # người dùng sẽ tắt nó đi trong tuần đầu.
    fp = {n: 0 for n in CONFIGS}
    n_pts = {n: 0 for n in CONFIGS}
    for temps, amb, cur, soc in segs:
        for n in CONFIGS:
            f = build_features(group_avg(temps, n), amb, cur, soc)[WARMUP:]
            fl = persist_mask(score(f) > ae_th, PERSIST)
            fp[n] += int(fl.sum())
            n_pts[n] += fl.size
            del f, fl

    # --- phát hiện --------------------------------------------------------
    rng = np.random.default_rng(7)
    res = {}
    for kind in ("offset", "ramp", "drift"):
        for mag in MAGNITUDES:
            hit = {n: 0 for n in CONFIGS}
            lat = {n: [] for n in CONFIGS}
            tot = 0
            for temps, amb, cur, soc in segs:
                cell = int(rng.integers(8))
                if kind == "offset":
                    t2 = FT.inject_offset(temps, cell, FAULT_START, mag)
                elif kind == "ramp":
                    t2 = FT.inject_ramp(temps, cell, FAULT_START, mag / 10.0)
                else:
                    t2 = FT.inject_drift(temps, cell, mag)
                tot += 1
                for n in CONFIGS:
                    g = cell // (8 // n)          # cell lỗi rơi vào nhóm nào
                    f = build_features(group_avg(t2, n), amb, cur, soc)[WARMUP:]
                    fl = persist_mask(score(f) > ae_th, PERSIST)
                    col = fl[FAULT_START - WARMUP:, g]
                    if col.any():
                        hit[n] += 1
                        lat[n].append(int(np.argmax(col)))
                    del f, fl
                del t2
            for n in CONFIGS:
                res[(kind, mag, n)] = (hit[n] / max(tot, 1),
                                       float(np.median(lat[n])) if lat[n] else float("nan"))

    # ---- in bảng ----------------------------------------------------------
    print(f"\n{'='*76}\nBÁO ĐỘNG GIẢ trên dữ liệu bình thường\n{'='*76}")
    print(f"{'Số cảm biến':<14}{'Tỉ lệ báo oan':>16}")
    for n in CONFIGS:
        print(f"{n:<14}{100*fp[n]/max(n_pts[n],1):>15.4f}%")

    print(f"\n{'='*76}\nTỈ LỆ PHÁT HIỆN theo số cảm biến (trễ trung vị, giây)\n{'='*76}")
    for kind in ("offset", "ramp", "drift"):
        print(f"\n--- lỗi kiểu {kind} ---")
        print(f"{'độ lớn':<10}" + "".join(f"{str(n)+' cảm biến':>20}" for n in CONFIGS))
        for mag in MAGNITUDES:
            row = f"{mag:<10.1f}"
            for n in CONFIGS:
                r, l = res[(kind, mag, n)]
                row += f"{100*r:>13.0f}% {'' if np.isnan(l) else f'({l:.0f}s)':>6}"
            print(row)

    print(f"\n{'='*76}\nCHỈ ĐÍCH DANH — thứ bảng trên KHÔNG đo được\n{'='*76}")
    for n in CONFIGS:
        print(f"{n} cảm biến: thu hẹp được còn {8//n} cell "
              f"({'đúng 1 cell' if n == 8 else f'phải tháo ra dò tay {8//n} cell'})")

    np.savez(MODELS / "sensor_count_study.npz",
             configs=np.array(CONFIGS),
             fp_rate=np.array([fp[n]/max(n_pts[n],1) for n in CONFIGS]),
             keys=np.array([f"{k}|{m}|{n}" for (k, m, n) in res]),
             rates=np.array([res[k][0] for k in res]),
             latency=np.array([res[k][1] for k in res]))
    print(f"\nđã ghi {MODELS/'sensor_count_study.npz'}")


if __name__ == "__main__":
    main()
