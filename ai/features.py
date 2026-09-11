#!/usr/bin/env python3
"""
Trích đặc trưng cho Lớp 1 — phát hiện cell bất thường.

Ý TƯỞNG CỐT LÕI, đọc kỹ chỗ này trước khi sửa bất cứ thứ gì:

Bộ UPC có pack 36 cell, phần cứng của đội có 8 cell. Nếu mô hình nhận thẳng
"nhiệt độ 36 cell" làm đầu vào thì train xong không deploy được. Nên mô hình
KHÔNG nhìn cả pack — nó nhìn **từng cell một**, qua một vector 16 đặc trưng mô
tả *cell này lệch khỏi phần còn lại của pack như thế nào*.

Hệ quả tốt:
  - Train trên pack 36 cell, chạy trên pack 8 cell, không phải sửa gì.
  - Mỗi thời điểm cho ra N mẫu huấn luyện (N = số cell) -> dữ liệu dồi dào.
  - Mô hình chỉ ra ĐÚNG cell nào bất thường, không chỉ nói "pack có vấn đề".
  - Mô hình cực nhỏ: 16 đầu vào, chạy 8 lần mỗi giây trên ESP32.

ĐỒNG BỘ VỚI PHẦN CỨNG THẬT (xem PHAN_CUNG_VA_KIEN_TRUC_HuTieu.md):
  - Đội dùng 8× DS18B20, MỖI CELL MỘT CON, không có cảm biến Top/Bottom riêng.
    -> ở đây gộp Top+Bottom của bộ UPC thành 1 giá trị, giả lập đúng phần cứng.
  - DS18B20 12 bit -> bước 0,0625 °C, và đọc ở 1 Hz.
    -> hạ tần số bộ UPC từ 2,5 Hz xuống 1 Hz và lượng tử hoá đúng bước đó.
  Không làm mấy bước này thì mô hình học trên dữ liệu mịn hơn thực tế, ra sân
  khấu gặp dữ liệu thô hơn là hỏng.
"""
import numpy as np

# ---- hằng số phải KHỚP với firmware khi viết lại bằng C -------------------
FS_HZ        = 1.0      # tần số lấy mẫu sau khi hạ
DS18B20_STEP = 0.0625   # độ phân giải DS18B20 ở 12 bit
DT_WIN       = 10       # cửa sổ tính tốc độ đổi nhiệt (giây)
STD_WIN      = 60       # cửa sổ tính độ dao động (giây)
EMA_TAU      = 300      # hằng số thời gian EMA chậm (giây)
I_SCALE      = 20.0     # dòng chuẩn hoá (A) — pack thí nghiệm cỡ này
N_FEATURES   = 16

FEATURE_NAMES = [
    "dev_mean",      # 0  T_cell - trung bình pack        <- tín hiệu chính
    "dev_median",    # 1  T_cell - trung vị pack          <- ít bị 1 cell hỏng kéo lệch
    "zscore",        # 2  dev / độ lệch chuẩn pack
    "rank",          # 3  thứ hạng nhiệt trong pack, chuẩn hoá 0..1
    "t_minus_amb",   # 4  T_cell - nhiệt môi trường
    "pack_minus_amb",# 5  trung bình pack - môi trường
    "spread",        # 6  max - min của pack
    "dT_cell",       # 7  tốc độ đổi nhiệt của cell (degC/phút)
    "dT_pack",       # 8  tốc độ đổi nhiệt trung bình pack
    "dT_diff",       # 9  (7)-(8) <- cell nóng NHANH HƠN phần còn lại
    "dev_ema",       # 10 EMA chậm của dev -> lệch kiểu trôi từ từ
    "dev_shock",     # 11 dev - dev_ema -> lệch đột ngột
    "t_std",         # 12 độ dao động nhiệt của cell
    "current",       # 13 |dòng| chuẩn hoá
    "soc",           # 14 SoC 0..1
    "t_abs",         # 15 mức nhiệt tuyệt đối, (T-25)/25
]
assert len(FEATURE_NAMES) == N_FEATURES


def _ema(x, tau, axis=0):
    """
    EMA nhân quả (chỉ dùng quá khứ). Trên ESP32 đây là đúng 1 dòng mỗi bước:
        acc += a * (T_now - acc)
    Ở đây dùng lfilter cho nhanh, nhưng kết quả giống hệt vòng lặp đó
    (đã kiểm chứng bằng test_features.py).
    """
    from scipy.signal import lfilter
    a = 1.0 / max(tau, 1.0)
    zi = np.full((1,) + x.shape[1:], 1.0 - a, dtype=np.float64) * x[0]
    y, _ = lfilter([a], [1.0, -(1.0 - a)], x, axis=0, zi=zi)
    return y.astype(x.dtype)


def _roll_std(x, win):
    """
    Độ lệch chuẩn trượt, nhân quả, cửa sổ `win` bước. x: (T, N).
    Vectorised hoàn toàn — bản vòng lặp Python chậm gấp ~100 lần, tiền xử lý
    412 chu kỳ sẽ mất hàng giờ thay vì vài phút.
    """
    T = x.shape[0]
    z = np.zeros((1,) + x.shape[1:], dtype=np.float64)
    c1 = np.concatenate([z, np.cumsum(x, axis=0, dtype=np.float64)])
    c2 = np.concatenate([z, np.cumsum(np.square(x, dtype=np.float64), axis=0)])
    i = np.arange(T)
    lo = np.maximum(0, i - win + 1)
    n = (i - lo + 1).astype(np.float64)[:, None]
    s1 = c1[i + 1] - c1[lo]
    s2 = c2[i + 1] - c2[lo]
    return np.sqrt(np.maximum(s2 / n - (s1 / n) ** 2, 0)).astype(x.dtype)


def _rate_per_min(x, win):
    """Tốc độ đổi nhiệt degC/phút, tính bằng sai phân lùi qua `win` giây."""
    out = np.zeros_like(x)
    out[win:] = (x[win:] - x[:-win]) / (win / 60.0)
    return out


def build_features(temps, ambient, current, soc):
    """
    temps   : (T, N) nhiệt độ từng cell, °C, đã ở 1 Hz
    ambient : (T,)   nhiệt độ môi trường
    current : (T,)   dòng pack, A
    soc     : (T,)   phần trăm 0..100

    Trả về (T, N, 16) — mỗi cell tại mỗi thời điểm là một vector 16 chiều.
    """
    T, N = temps.shape
    eps = 1e-6

    pack_mean = temps.mean(axis=1, keepdims=True)          # (T,1)
    pack_med  = np.median(temps, axis=1, keepdims=True)
    pack_std  = temps.std(axis=1, keepdims=True)
    spread    = (temps.max(axis=1) - temps.min(axis=1))[:, None]

    dev     = temps - pack_mean
    dev_med = temps - pack_med
    z       = dev / (pack_std + eps)

    # Thứ hạng nhiệt trong pack, chuẩn hoá về 0..1, dùng HẠNG TRUNG BÌNH khi hoà.
    #   rank[i] = ( số cell lạnh hơn i  +  (số cell bằng i - 1)/2 ) / (N-1)
    #
    # Phải định nghĩa tường minh vì hai lý do:
    #  1. np.argsort dùng quicksort (không ổn định) -> hoà nhau thì kết quả tuỳ
    #     nội bộ numpy, mà sau khi lượng tử hoá về bước 0,0625 °C thì hoà nhau
    #     xảy ra rất thường xuyên. Firmware không thể tái tạo được.
    #  2. Phá hoà bằng chỉ số cell (cell 0 luôn xếp dưới) là thiên lệch vô
    #     nghĩa: đánh số cell là do mình đặt, không mang thông tin vật lý nào.
    # Hạng trung bình xử lý được cả hai, và viết bằng C chỉ vài dòng.
    A = temps[:, :, None]                      # (T, i, 1)
    B = temps[:, None, :]                      # (T, 1, j)
    n_less = (B < A).sum(2)
    n_eq = (B == A).sum(2)                     # luôn >= 1 (chính nó)
    rank = (n_less + (n_eq - 1) / 2.0) / max(N - 1, 1)

    amb = ambient[:, None]
    dT_cell = _rate_per_min(temps, DT_WIN)
    dT_pack = _rate_per_min(np.repeat(pack_mean, N, axis=1), DT_WIN)
    dev_ema = _ema(dev, EMA_TAU, axis=0)

    f = np.empty((T, N, N_FEATURES), dtype=np.float32)
    f[:, :, 0]  = dev
    f[:, :, 1]  = dev_med
    f[:, :, 2]  = z
    f[:, :, 3]  = rank
    f[:, :, 4]  = temps - amb
    f[:, :, 5]  = np.repeat(pack_mean - amb, N, axis=1)
    f[:, :, 6]  = np.repeat(spread, N, axis=1)
    f[:, :, 7]  = dT_cell
    f[:, :, 8]  = dT_pack
    f[:, :, 9]  = dT_cell - dT_pack
    f[:, :, 10] = dev_ema
    f[:, :, 11] = dev - dev_ema
    f[:, :, 12] = _roll_std(temps, STD_WIN)
    f[:, :, 13] = np.repeat(np.abs(current)[:, None] / I_SCALE, N, axis=1)
    f[:, :, 14] = np.repeat(soc[:, None] / 100.0, N, axis=1)
    f[:, :, 15] = (temps - 25.0) / 25.0
    return f


# ---------------------------------------------------------------- tiêm lỗi
def inject_offset(temps, cell, start, delta, ramp=60):
    """Cell bị lệch hẳn lên `delta` °C — mô phỏng tiếp xúc kém / điện trở cao."""
    out = temps.copy()
    T = out.shape[0]
    prof = np.clip((np.arange(T) - start) / max(ramp, 1), 0, 1) * delta
    out[:, cell] += prof
    return out


def inject_ramp(temps, cell, start, rate_per_min):
    """Nhiệt tăng dần không dừng — tiền đề thermal runaway."""
    out = temps.copy()
    T = out.shape[0]
    t = np.clip(np.arange(T) - start, 0, None) / 60.0
    out[:, cell] += t * rate_per_min
    return out


def inject_drift(temps, cell, total_delta):
    """Trôi rất chậm suốt cả chu kỳ — kiểu lão hoá, khó thấy nhất."""
    out = temps.copy()
    T = out.shape[0]
    out[:, cell] += np.linspace(0, total_delta, T)
    return out
