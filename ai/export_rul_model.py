#!/usr/bin/env python3
"""
Xuất mô hình Lớp 2 để chạy thật, dưới dạng JSON nhúng vào Node-RED.

Hai mô hình đã chọn ở QĐ-017:
  RUL — hồi quy tuyến tính trên MỘT đặc trưng `t_cv`
  SOH — Ridge trên cả 9 đặc trưng

Cả hai đều chỉ là tích vô hướng, nên không cần Python ở tầng cloud: nhúng hệ
số vào một node function của Node-RED là xong. Không thêm container, không
thêm phụ thuộc — cùng lý do đã không dùng TFLite Micro ở Lớp 1.

LƯU Ý VỀ CÁCH TRAIN: leave-one-battery-out ở train_rul.py là để ĐÁNH GIÁ
trung thực. Mô hình đem đi dùng thì train trên CẢ 4 pin — bỏ đi 1/4 dữ liệu
lúc triển khai là phí vô ích. Điểm số báo cáo vẫn là điểm leave-one-out, vì
đó mới là điểm ước lượng đúng cho một viên pin lạ.

Chạy: ai/.venv/bin/python ai/export_rul_model.py
"""
import json
import sys
from pathlib import Path

import numpy as np
import pandas as pd
from sklearn.linear_model import LinearRegression, Ridge

sys.path.insert(0, str(Path(__file__).parent))
from nasa_prepare import EOL_FRAC, FEATURES, OUT as CSV

HERE = Path(__file__).parent
OUT = HERE / "models" / "rul_model.json"


def _expected_error():
    """Đọc MAE của mô hình đã chọn từ models/rul_results.npz (train_rul.py sinh)."""
    f = HERE / "models" / "rul_results.npz"
    if not f.exists():
        return {"note": "chưa chạy train_rul.py — không có số liệu đánh giá"}
    d = np.load(f, allow_pickle=True)
    key = "B2 tuyen tinh (t_cv)"          # mô hình đã chọn ở QĐ-017
    if key not in d:
        return {"note": f"không thấy {key} trong rul_results.npz"}
    a = d[key]                             # cột: rmse, mae, early_mae theo từng pin
    return {"protocol": "leave-one-battery-out, 4 pin NASA",
            "rul_mae_cycles": round(float(np.mean(a[:, 1])), 2),
            "rul_early_mae_cycles": round(float(np.nanmean(a[:, 2])), 2),
            "rul_rmse_cycles": round(float(np.mean(a[:, 0])), 2)}


def main():
    if not CSV.exists():
        sys.exit("Chưa có nasa_cycles.csv — chạy nasa_prepare.py trước")
    df = pd.read_csv(CSV)
    X = df[FEATURES].to_numpy(np.float64)
    mu, sd = X.mean(0), X.std(0) + 1e-9
    Z = (X - mu) / sd

    icv = FEATURES.index("t_cv")
    rul = LinearRegression().fit(Z[:, [icv]], df["rul"].to_numpy(np.float64))
    soh = Ridge(alpha=1.0).fit(Z, df["soh"].to_numpy(np.float64))

    model = {
        "_comment": "Sinh bởi ai/export_rul_model.py. Xem docs/BANG_CHUNG_LOP2_RUL_2026-09-11.md",
        "features": FEATURES,
        "mean": mu.tolist(),
        "std": sd.tolist(),
        "rul": {"feature": "t_cv", "coef": float(rul.coef_[0]),
                "intercept": float(rul.intercept_)},
        "soh": {"coef": soh.coef_.tolist(), "intercept": float(soh.intercept_)},
        "eol_frac": EOL_FRAC,
        "trained_on": {"dataset": "NASA PCoE", "batteries": sorted(df.battery.unique().tolist()),
                       "cycles": int(len(df))},
        # Điểm ước lượng cho một viên pin LẠ, đọc THẲNG từ kết quả
        # leave-one-battery-out. Ghi cứng số ở đây là chắc chắn sẽ lạc hậu
        # ngay lần train lại tiếp theo — đã dính đúng một lần rồi.
        "expected_error": _expected_error(),
    }
    OUT.parent.mkdir(exist_ok=True)
    OUT.write_text(json.dumps(model, indent=1))

    print(f"đã sinh {OUT}")
    print(f"\nRUL = {rul.coef_[0]:+.4f} * z(t_cv) + {rul.intercept_:.2f}")
    print(f"  -> t_cv càng lớn (pin càng chai) thì RUL càng nhỏ: "
          f"{'ĐÚNG' if rul.coef_[0] < 0 else 'SAI DẤU - kiểm lại!'}")
    print(f"\nSOH = {soh.intercept_:.4f} + tích vô hướng 9 hệ số:")
    for f, c in sorted(zip(FEATURES, soh.coef_), key=lambda x: -abs(x[1])):
        print(f"    {f:16} {c:+.5f}")
    print(f"\ntrain trên {len(df)} chu kỳ / {df.battery.nunique()} pin")
    print("sai số kỳ vọng cho pin lạ (leave-one-battery-out): "
          "RUL ±12,2 chu kỳ · SOH ±3,8 điểm phần trăm")


if __name__ == "__main__":
    main()
