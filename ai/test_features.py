#!/usr/bin/env python3
"""
Test cho features.py. Chạy: ai/.venv/bin/python ai/test_features.py

Vì sao cần: hai hàm _ema và _roll_std đã được vectorise cho nhanh, và bản
vectorise rất dễ sai lệch một bước so với bản vòng lặp — mà lệch một bước ở
đây nghĩa là đặc trưng "nhìn trộm tương lai", điểm đánh giá đẹp giả tạo còn
firmware thì không tài nào tái tạo được. Test này so trực tiếp với bản vòng
lặp ngây thơ, đúng bản mà firmware sẽ viết lại bằng C.
"""
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).parent))
import features as FT

fails = 0


def check(cond, name):
    global fails
    print(f"  {'PASS' if cond else 'FAIL'}  {name}")
    if not cond:
        fails += 1


def ema_ref(x, tau):
    """Bản ngây thơ — đây chính là đoạn sẽ viết trong firmware."""
    a = 1.0 / tau
    out = np.empty_like(x)
    acc = x[0].astype(np.float64).copy()
    for i in range(len(x)):
        acc = acc + a * (x[i] - acc)
        out[i] = acc
    return out


def std_ref(x, w):
    out = np.zeros_like(x)
    for i in range(len(x)):
        out[i] = x[max(0, i - w + 1): i + 1].std(axis=0)
    return out


def main():
    rng = np.random.default_rng(0)
    T, N = 800, 8
    temps = (25 + np.cumsum(rng.normal(0, 0.02, (T, N)), axis=0)).astype(np.float32)
    amb = np.full(T, 25.0, np.float32)
    cur = rng.normal(0, 5, T).astype(np.float32)
    soc = np.linspace(90, 20, T).astype(np.float32)

    print("=== bản vectorise phải khớp bản vòng lặp ===")
    check(np.abs(FT._ema(temps, 300) - ema_ref(temps, 300)).max() < 1e-4, "EMA khớp")
    check(np.abs(FT._roll_std(temps, 60) - std_ref(temps, 60)).max() < 1e-4, "rolling std khớp")

    print("\n=== tính nhân quả: đặc trưng KHÔNG được nhìn tương lai ===")
    # Sửa dữ liệu ở nửa sau; đặc trưng ở nửa đầu phải không đổi một chút nào.
    f1 = FT.build_features(temps, amb, cur, soc)
    t2 = temps.copy()
    t2[T // 2:, 0] += 10.0
    f2 = FT.build_features(t2, amb, cur, soc)
    check(np.array_equal(f1[:T // 2], f2[:T // 2]),
          "đổi dữ liệu tương lai không làm đổi đặc trưng quá khứ")

    print("\n=== hình dạng và tính lành mạnh ===")
    check(f1.shape == (T, N, FT.N_FEATURES), f"shape = (T, N, {FT.N_FEATURES})")
    check(np.isfinite(f1).all(), "không có NaN/Inf")
    check(abs(f1[:, :, 0].sum(axis=1)).max() < 1e-3,
          "dev_mean cộng trên các cell phải bằng 0 (theo định nghĩa)")

    print("\n=== tiêm lỗi phải làm đặc trưng phản ứng đúng hướng ===")
    hot = FT.inject_offset(temps, 3, 200, 5.0, ramp=1)
    fh = FT.build_features(hot, amb, cur, soc)
    check(fh[400:, 3, 0].mean() > f1[400:, 3, 0].mean() + 3.0,
          "offset +5C -> dev_mean của đúng cell đó tăng mạnh")
    others = [c for c in range(N) if c != 3]
    check(fh[400:, others, 0].mean() < f1[400:, others, 0].mean(),
          "các cell còn lại bị kéo xuống (vì trung bình pack tăng)")
    check(fh[400:, 3, 3].mean() > 0.9, "cell nóng nhất -> rank ~ 1.0")

    ramp = FT.inject_ramp(temps, 5, 200, 2.0)     # 2 degC/phút
    fr = FT.build_features(ramp, amb, cur, soc)
    check(fr[400:, 5, 9].mean() > 1.0,
          "ramp -> dT_diff (nóng nhanh hơn pack) dương rõ rệt")

    print("\n=== khớp phần cứng ===")
    check(FT.FS_HZ == 1.0, "1 Hz — đúng nhịp đọc DS18B20")
    check(abs(FT.DS18B20_STEP - 0.0625) < 1e-9, "bước 0,0625 degC — DS18B20 12 bit")

    print("\n" + ("=== CO %d TEST HONG ===" % fails if fails else "=== TAT CA TEST PASS ==="))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
