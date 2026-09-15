#!/usr/bin/env python3
"""
Kiểm chứng Lớp 1 trên MẤT CÂN BẰNG THẬT — bộ dữ liệu pack McMaster.

VÌ SAO CẦN
Cho tới nay mọi ca lỗi của Lớp 1 đều do CHÍNH MÌNH TIÊM vào dữ liệu bình
thường (`evaluate.py`, `sensor_count_study.py`). Điều đó kiểm được rằng mô hình
nhạy với thứ mình nghĩ là lỗi — nhưng không kiểm được rằng lỗi thật trông giống
thứ mình tưởng tượng. Đây là lỗ hổng lớn nhất còn lại trong câu chuyện AI, và
là câu giám khảo dễ hỏi nhất.

BỘ DỮ LIỆU
M. Naguib, P. Kollmeyer, J. Chen, A. Emadi, "Battery Pack with Introduced
Faults Dataset - Air Cooled SBLimotive 5Ah", Borealis Data, 2025. CC-BY 4.0.
doi:10.5683/SP3/THZTJC
Pack thật 72 cell nối tiếp lấy từ xe hybrid, đo qua Orion BMS, 1 Hz, buồng
nhiệt 15/25 °C. Lỗi nhiệt được gây ra CỐ Ý: chặn dòng khí, chặn một module,
tắt quạt. Nhãn nằm ngay trong tên file.

ĐÂY LÀ PHÉP THỬ "ZERO-SHOT": mô hình KHÔNG được huấn luyện lại, KHÔNG chỉnh
ngưỡng. Dùng đúng `cell_ae.keras` và đúng ngưỡng p99.9 đã nạp vào firmware.
Kém thuyết phục hơn thì đã phải huấn luyện lại, và như vậy là kiểm chính mình.

BA CHỖ BẤT LỢI CHO MÔ HÌNH, PHẢI NÓI RA TRƯỚC:

 1. ĐỘ PHÂN GIẢI 1 °C. Orion BMS báo nhiệt độ làm tròn tới 1 °C. DS18B20 của
    đội là 0,0625 °C — mịn gấp 16 lần. Mọi đặc trưng lệch/độ lệch chuẩn đều bị
    lượng tử hoá thô. Mô hình được huấn luyện trên dữ liệu 0,0625 °C nên đây là
    điều kiện tệ hơn hẳn thực tế của đội.

 2. DÒNG LỚN GẤP 10 LẦN. Pack hybrid này chạy tới ±190 A; pack của đội sạc
    1,9 A. Đặc trưng 13 là |I|/I_SCALE với I_SCALE=20 A — giữ nguyên thì giá
    trị bắn ra ~9 thay vì ~0,1, hoàn toàn ngoài phân bố huấn luyện. Nên chuẩn
    hoá lại theo tốc độ C tương đương (xem I_SCALE_MC bên dưới). Đây là thích
    nghi cần thiết chứ không phải nới tay: cùng một đại lượng vật lý, khác đơn
    vị quy chiếu. Script chạy CẢ HAI cách để thấy nó ảnh hưởng bao nhiêu.

 3. PACK 72 CELL, KHÔNG PHẢI 8. Cắt thành 9 pack-ảo 8 cell liền kề. Giả định:
    cell liền kề về điện thì cũng liền kề về vị trí — đúng với module pin trụ
    xếp dãy, nhưng là giả định.

MỘT ĐIỀU LÀM BỘ DỮ LIỆU NÀY ĐẶC BIỆT GIÁ TRỊ: nó kiểm được cả thứ Lớp 1 NÓI LÀ
KHÔNG LÀM ĐƯỢC. `Fanoffon` tắt quạt cả pack => nóng ĐỀU => lệch tương đối giữa
các cell không đổi => Lớp 1 phải MÙ, đúng như TH-4 trong
docs/HAI_LOP_AI_HOAT_DONG_THE_NAO.md đã ghi từ trước. Còn `ModuleBlocked` chặn
một module => tạo chênh lệch không gian => Lớp 1 phải THẤY. Một bộ dữ liệu kiểm
được cả hai chiều thì đáng tin hơn hẳn bộ chỉ kiểm chiều thuận.

Chạy: ai/.venv/bin/python ai/validate_mcmaster.py
"""
import os, sys, warnings, re
from pathlib import Path

os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")
warnings.filterwarnings("ignore")
import numpy as np
import scipy.io as sio

sys.path.insert(0, str(Path(__file__).parent))
import features as FT
from features import build_features

HERE = Path(__file__).parent
MODELS = HERE / "models"
PREP = HERE / "data" / "prepared"
MCM = HERE / "data" / "mcmaster" / "Drive Cycle Data"

PERSIST = 10     # giống firmware và evaluate.py: phải vượt liên tục 10 s
WARMUP = 300     # bỏ 5 phút đầu, EMA chưa ổn định
N_VPACK = 8      # cắt 72 cell thành 9 pack-ảo 8 cell

# Tốc độ C tương đương: pack đội 2,55 Ah với I_SCALE 20 A  <->  cell 5,2 Ah ở
# đây. 20 × (5,2/2,55) ≈ 40,8 A. Xem chú thích (2) ở đầu file.
I_SCALE_MC = 20.0 * (5.2 / 2.55)

# Nhãn lấy từ tên file, theo README của bộ dữ liệu.
FAULT = {
    "UDDS_Blocked_25C_with_TEM":            ("chặn dòng khí",        "cục bộ"),
    "HWFET_15C_ModuleBlocked_Long_with_TEM_data": ("chặn một module", "cục bộ"),
    "Fanoffon_15C_Long_with_TEM_data":      ("tắt quạt cả pack",     "toàn cục"),
}


def persist_mask(flag, k):
    if k <= 1:
        return flag
    out = flag.copy()
    run = np.zeros(flag.shape[1:], dtype=int)
    for t in range(flag.shape[0]):
        run = np.where(flag[t], run + 1, 0)
        out[t] = run >= k
    return out


def load(path):
    d = sio.loadmat(path)
    temps = d["CellTemp_R"].astype(np.float64)            # (T,72)
    cur = d["Current_Smooth"].ravel().astype(np.float64)  # (T,)
    soc = d["SOC_R2"].astype(np.float64).mean(axis=1)     # (T,) trung bình pack
    # Nhiệt độ buồng lấy từ tên file — đây là số THẬT của thí nghiệm, không phải
    # hằng số bịa: buồng nhiệt được giữ ở đúng mức đó.
    amb_c = 15.0 if "_15C" in path.name else 25.0
    amb = np.full(len(temps), amb_c)
    return temps, amb, cur, soc


def main():
    from tensorflow import keras
    ae = keras.models.load_model(MODELS / "cell_ae.keras", compile=False)
    sc = np.load(PREP / "scaler.npz")
    mu, sd = sc["mu"], sc["sd"]
    ae_th = float(np.load(MODELS / "thresholds.npz")["p99.9"])
    print(f"mô hình: cell_ae.keras, ngưỡng p99.9 = {ae_th:.5f} (KHÔNG chỉnh lại)")

    global _CTX_OFF, _CTX
    _CTX_OFF = False

    def score(feats):
        T, N, F = feats.shape
        z = (feats.reshape(-1, F) - mu) / sd
        if _CTX_OFF:
            z[:, _CTX] = 0.0      # ép về đúng trung bình của tập huấn luyện
        rec = ae.predict(z, batch_size=8192, verbose=0)
        return np.mean((rec - z) ** 2, axis=1).reshape(T, N)

    files = sorted(MCM.glob("*.mat"))
    if not files:
        sys.exit(f"không thấy dữ liệu ở {MCM}")

    # Bốn đặc trưng BỐI CẢNH TOÀN PACK: T-amb, packT-amb, |I|/scale, soc.
    # Chúng giống hệt nhau ở mọi cell trong pack, nên khi lệch phân bố thì đẩy
    # sai số tái tạo của TẤT CẢ các cell lên cùng lúc — đó chính là cơ chế làm
    # hỏng phép chuyển giao, xem KQ-02 trong bằng chứng.
    _CTX = [4, 5, 13, 14]

    for i_scale, ctx_off, label in (
            (I_SCALE_MC, False, "A. I_SCALE chuẩn hoá theo tốc độ C"),
            (20.0,       False, "B. I_SCALE giữ nguyên 20 A (đối chứng)"),
            (I_SCALE_MC, True,  "C. Vô hiệu hoá 4 đặc trưng bối cảnh toàn pack")):
        FT.I_SCALE = i_scale
        _CTX_OFF = ctx_off
        print(f"\n{'='*82}\n{label}  (I_SCALE = {i_scale:.1f} A)\n{'='*82}")
        print(f"{'file':<44}{'loại':<12}{'% pack-ảo báo':>16}{'% thời gian':>12}")

        for path in files:
            stem = path.stem
            kind, scope = FAULT.get(stem, ("bình thường", "-"))
            temps, amb, cur, soc = load(path)
            if len(temps) < WARMUP + 600:
                continue

            n_vp = temps.shape[1] // N_VPACK
            hit, tot_flag, tot_pts = 0, 0, 0
            for g in range(n_vp):
                sub = temps[:, g * N_VPACK:(g + 1) * N_VPACK]
                f = build_features(sub, amb, cur, soc)[WARMUP:]
                fl = persist_mask(score(f) > ae_th, PERSIST)
                if fl.any():
                    hit += 1
                tot_flag += int(fl.sum())
                tot_pts += fl.size
                del f, fl

            tag = kind if kind == "bình thường" else f"{kind}"
            print(f"{stem[:43]:<44}{scope:<12}{100*hit/n_vp:>14.0f}% "
                  f"{100*tot_flag/max(tot_pts,1):>11.3f}%")


if __name__ == "__main__":
    main()
