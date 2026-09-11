#!/usr/bin/env python3
"""
Lớp 2 — biến dữ liệu NASA PCoE (.mat) thành bảng đặc trưng theo chu kỳ.

QUYẾT ĐỊNH QUAN TRỌNG: chỉ dùng đặc trưng của PHA SẠC.

Vì sao không dùng dung lượng phóng làm đầu vào: dung lượng chính là thứ dùng
để tính nhãn RUL, đưa nó vào đầu vào là rò rỉ đáp án. Mà quan trọng hơn, đo
được dung lượng thật đòi hỏi phóng kiệt pin theo dòng cố định — điều không bao
giờ xảy ra với một chiếc xe điện đang chạy ngoài đường.

Pha sạc thì ngược lại: ngày nào người dùng cũng cắm sạc, và ESP32 đo được đủ
điện áp, dòng, nhiệt độ suốt quá trình đó. Nên câu chuyện sản phẩm là:
**"đoán tuổi thọ pin từ cách nó sạc"** — dùng được thật, không chỉ trên giấy.

Cái bẫy đã gặp: thời gian CC thô KHÔNG dùng được, vì nó phụ thuộc pin còn bao
nhiêu lúc cắm sạc (đo được 11 phút ở chu kỳ này và 38 phút ở chu kỳ khác, không
phải do lão hoá). Nên mọi đặc trưng thời gian ở đây đều đo trên một KHOẢNG ĐIỆN
ÁP CỐ ĐỊNH — bắt đầu tính từ lúc pin đạt mốc điện áp đã chọn, không tính từ lúc
cắm dây.

Chạy: ai/.venv/bin/python ai/nasa_prepare.py
"""
import sys
from pathlib import Path

import numpy as np
import pandas as pd
import scipy.io as sio

HERE = Path(__file__).parent
RAW = HERE / "data" / "nasa_raw"
OUT = HERE / "data" / "nasa_cycles.csv"

BATTERIES = ["B0005", "B0006", "B0007", "B0018"]   # 4 pin chuẩn, cùng 24 °C
EOL_FRAC = 0.80          # hết đời khi dung lượng còn 80% ban đầu
V_LO, V_HI = 3.9, 4.15   # khoảng điện áp cố định để đo thời gian sạc
V_CV = 4.19              # coi như đã vào pha CV
I_START = 0.10           # dòng vượt mức này = bắt đầu sạc (A)
# Coi là sạc xong khi dòng tụt dưới mức này. Chọn 0,2 A (~C/10 với cell 2 Ah)
# chứ KHÔNG phải 0,02 A của giao thức phòng thí nghiệm NASA. Lý do là tính
# khả dụng ngoài đời, không phải vì điểm số: mọi bộ sạc thương mại đều ngắt
# quanh C/20..C/10, và cái đuôi dòng rất thấp phía sau dài bao lâu là do cài
# đặt của bộ sạc chứ không phải do sức khoẻ viên pin. Lấy tới 0,02 A là nhét
# đặc tính của thiết bị đo vào đặc trưng của pin.
I_CV_END = 0.20

FEATURES = ["t_v_interval", "t_cv", "i_cv_mean", "t_charge_total",
            "T_max_ch", "T_mean_ch", "T_rise_ch", "v_start", "dvdt_cc"]


def find_mat(name):
    hits = list(RAW.rglob(f"{name}.mat"))
    return hits[0] if hits else None


def charge_features(d):
    """Một chu kỳ sạc -> dict đặc trưng, hoặc None nếu chu kỳ hỏng."""
    V = np.asarray(d["Voltage_measured"], float).ravel()
    I = np.asarray(d["Current_measured"], float).ravel()
    T = np.asarray(d["Temperature_measured"], float).ravel()
    t = np.asarray(d["Time"], float).ravel()

    # Chu kỳ rác: bộ NASA có vài chu kỳ chỉ 5 mẫu, điện áp nhảy lên 4,99 V.
    # Không lọc thì chúng thành ngoại lai kéo lệch cả mô hình.
    if len(t) < 100 or t[-1] < 600 or V.max() > 4.5 or V.max() < 4.1:
        return None
    if not np.isfinite(V).all() or not np.isfinite(T).all():
        return None

    def first_at(mask):
        idx = np.argmax(mask)
        return t[idx] if mask.any() else np.nan

    # CỬA SỔ SẠC: từ lúc dòng bắt đầu chạy tới lúc pin no.
    # Định nghĩa này phải giống hệt bên firmware (charge_cycle.cpp) và phải
    # dùng được ngoài đời. Bản đầu lấy "toàn bộ mảng ghi được" (t[-1]) —
    # ngoài đời con số đó phụ thuộc lúc nào người dùng rút sạc, tức là nhiễu
    # thuần tuý. Test so C với Python đã bắt đúng chỗ này.
    i_on = first_at(I > I_START)
    if not np.isfinite(i_on):
        return None
    t_cv0 = first_at(V >= V_CV)
    after_cv = (t >= t_cv0) if np.isfinite(t_cv0) else np.zeros_like(t, bool)
    t_end = first_at(after_cv & (I < I_CV_END))
    if not np.isfinite(t_end):
        t_end = t[-1]                      # chưa no thì lấy tới hết bản ghi

    win = (t >= i_on) & (t <= t_end)
    if win.sum() < 50:
        return None

    tw, Vw, Iw, Tw = t[win], V[win], I[win], T[win]

    def first_at_w(mask):
        idx = np.argmax(mask)
        return tw[idx] if mask.any() else np.nan

    t_lo, t_hi = first_at_w(Vw >= V_LO), first_at_w(Vw >= V_HI)
    if not (np.isfinite(t_lo) and np.isfinite(t_hi)) or t_hi <= t_lo:
        return None

    cv = (tw >= t_cv0) if np.isfinite(t_cv0) else np.zeros_like(tw, bool)
    in_cc = (Vw >= V_LO) & (Vw <= V_HI)

    return {
        # thời gian đi hết khoảng 3,9 -> 4,15 V. Đây là chỉ số sức khoẻ kinh
        # điển, và không phụ thuộc pin còn bao nhiêu lúc cắm sạc.
        "t_v_interval": t_hi - t_lo,
        "t_cv": (t_end - t_cv0) if np.isfinite(t_cv0) else 0.0,
        "i_cv_mean": float(Iw[cv].mean()) if cv.sum() > 3 else 0.0,
        "t_charge_total": float(t_end - i_on),
        "T_max_ch": float(Tw.max()),
        "T_mean_ch": float(Tw.mean()),
        "T_rise_ch": float(Tw.max() - Tw[0]),
        "v_start": float(Vw[0]),
        "dvdt_cc": float(np.polyfit(tw[in_cc], Vw[in_cc], 1)[0]) if in_cc.sum() > 5 else np.nan,
    }


def process(name):
    p = find_mat(name)
    if p is None:
        print(f"  {name}: KHÔNG tìm thấy .mat")
        return None
    cycles = sio.loadmat(p, simplify_cells=True)[name]["cycle"]

    # Ghép: mỗi chu kỳ sạc lấy dung lượng của LẦN PHÓNG NGAY SAU nó.
    rows, pending = [], None
    for c in cycles:
        if c["type"] == "charge":
            f = charge_features(c["data"])
            pending = f
        elif c["type"] == "discharge" and pending is not None:
            cap = float(np.asarray(c["data"]["Capacity"], float).ravel()[0])
            if 0.1 < cap < 5.0:
                rows.append({**pending, "capacity": cap})
            pending = None

    if len(rows) < 20:
        print(f"  {name}: chỉ {len(rows)} chu kỳ dùng được -> bỏ")
        return None

    df = pd.DataFrame(rows)
    df["battery"] = name
    df["cycle_idx"] = np.arange(len(df))

    # Dung lượng ban đầu: trung vị 5 chu kỳ đầu (1 chu kỳ đơn lẻ dễ nhiễu)
    c0 = float(df["capacity"].head(5).median())
    df["soh"] = df["capacity"] / c0
    eol_thr = EOL_FRAC * c0

    # RUL = còn bao nhiêu chu kỳ nữa thì dung lượng lần ĐẦU tụt dưới 80%.
    below = np.where(df["capacity"].to_numpy() < eol_thr)[0]
    if len(below) == 0:
        print(f"  {name}: chưa bao giờ xuống dưới {EOL_FRAC:.0%} -> bỏ "
              f"(nhãn RUL sẽ bị cắt cụt, không train được trung thực)")
        return None
    eol = int(below[0])
    df["rul"] = eol - df["cycle_idx"]
    df = df[df["rul"] >= 0].copy()      # bỏ phần sau EOL: ngoài phạm vi bài toán

    print(f"  {name}: {len(df):>3} chu kỳ dùng được | C0={c0:.3f} Ah | "
          f"EOL ở chu kỳ {eol} | SOH {df['soh'].iloc[0]:.3f} -> {df['soh'].iloc[-1]:.3f}")
    return df


def main():
    if not RAW.exists():
        sys.exit(f"Chưa có dữ liệu ở {RAW}")
    print("đọc dữ liệu NASA PCoE:")
    dfs = [d for d in (process(b) for b in BATTERIES) if d is not None]
    if not dfs:
        sys.exit("không pin nào dùng được")

    df = pd.concat(dfs, ignore_index=True)
    before = len(df)
    df = df.dropna(subset=FEATURES)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    df.to_csv(OUT, index=False)

    print(f"\ntổng {len(df)} chu kỳ từ {df['battery'].nunique()} pin "
          f"(bỏ {before-len(df)} chu kỳ thiếu đặc trưng) -> {OUT}")
    print("\ntương quan đặc trưng với RUL (|r| càng lớn càng có ích):")
    for f in FEATURES:
        r = df[f].corr(df["rul"])
        bar = "#" * int(abs(r) * 30)
        print(f"  {f:16} r={r:+.3f}  {bar}")


if __name__ == "__main__":
    main()
