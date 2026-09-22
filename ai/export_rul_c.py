#!/usr/bin/env python3
"""
Xuất mô hình Lớp 2 thành header C để ESP32 tự tính RUL/SOH khi MẤT MẠNG.

VÌ SAO CÓ FILE NÀY, VÀ VÌ SAO NÓ PHẢI LÀ SCRIPT
Cùng bộ hệ số giờ chạy ở HAI nơi: Node-RED trên cloud (rul_predict.js) và
firmware trên chip. Hai bản chép tay sẽ lệch nhau — không phải nếu, mà là khi.
Và lúc lệch thì không ai biết, vì cả hai đều trả về một con số trông hợp lý.

Nên cả hai đều sinh từ MỘT nguồn: ai/models/rul_model.json. Sửa mô hình thì
chạy lại export_rul_model.py rồi chạy file này, không bao giờ gõ số bằng tay.

VÌ SAO CHO CHIP TỰ TÍNH (QĐ-043)
Trước đây Lớp 2 chỉ chạy trên cloud, với lý do "để đổi mô hình không phải nạp
lại firmware" — lý do đó VẪN ĐÚNG và cloud vẫn là nơi giữ bản chuẩn. Nhưng
thêm bản trên chip thì thiết bị vẫn trả lời được khi mất mạng, và barem chấm
riêng "hệ thống chạy được khi mất mạng". Giá phải trả: 10 phép nhân cộng, chạy
MỘT LẦN mỗi chu kỳ sạc.

Chạy: python3 ai/export_rul_c.py
"""
import json
from pathlib import Path

HERE = Path(__file__).parent
SRC = HERE / "models" / "rul_model.json"
OUT = HERE.parent / "VEDCaPhenika" / "esp32s3_wiseiot_test" / "rul_model.h"


def arr(name, vals):
    body = ",\n  ".join(f"{v:.10g}f" for v in vals)
    return f"static const float {name}[RUL_N_FEAT] = {{\n  {body}\n}};\n"


def main():
    m = json.loads(SRC.read_text())
    feats = m["features"]
    err = m.get("expected_error", {})
    tr = m.get("trained_on", {})

    h = f'''/* =============================================================================
 *  Hệ số Lớp 2 (RUL/SOH) — SINH TỰ ĐỘNG, ĐỪNG SỬA TAY.
 *
 *  Sinh bởi: ai/export_rul_c.py, từ ai/models/rul_model.json
 *  Cùng nguồn với VEDCaPhenika/planb_cloud/nodered/rul_predict.js — sửa mô
 *  hình thì chạy lại export_rul_model.py rồi export_rul_c.py, ĐỪNG gõ số vào
 *  đây. Hai bản chép tay sẽ lệch nhau, và lúc lệch thì cả hai đều trả về một
 *  con số trông hợp lý nên không ai phát hiện.
 *
 *  Mô hình (QĐ-017): RUL = hồi quy tuyến tính trên MỘT đặc trưng `{m["rul"]["feature"]}`;
 *  SOH = Ridge trên cả 9. Đã thử LSTM ~3.500 tham số và nó THUA mô hình 2 tham
 *  số này (MAE 20,3 so với 12,2 chu kỳ) — xem QĐ-017 trước khi đổi.
 *
 *  Train trên: {tr.get("dataset","?")} — {len(tr.get("batteries",[]))} pin, {tr.get("cycles","?")} chu kỳ.
 *  Sai số kỳ vọng (leave-one-battery-out): RUL ±{err.get("rul_mae_cycles","?")} chu kỳ.
 *
 *  ⚠️ Hệ số mượn của NASA (18650 đơn, 2,0 Ah, sạc 0,75C). Pack đội là 2,55 Ah
 *     nên PHẢI sạc ở 1,9 A mới cùng tốc độ C — xem cảnh báo trong charge_cycle.h.
 * ========================================================================== */
#pragma once

#define RUL_N_FEAT {len(feats)}

/* Thứ tự PHẢI khớp FEATURES trong ai/nasa_prepare.py và ChargeSummary::f[]:
   {", ".join(feats)} */

{arr("RUL_MEAN", m["mean"])}
{arr("RUL_STD", m["std"])}
{arr("SOH_COEF", m["soh"]["coef"])}
static const float SOH_INTERCEPT = {m["soh"]["intercept"]:.10g}f;

/* RUL chỉ dùng MỘT đặc trưng, nên lưu sẵn chỉ số của nó thay vì dò tên lúc chạy. */
#define RUL_FEAT_IDX  {feats.index(m["rul"]["feature"])}   /* {m["rul"]["feature"]} */
static const float RUL_COEF      = {m["rul"]["coef"]:.10g}f;
static const float RUL_INTERCEPT = {m["rul"]["intercept"]:.10g}f;

/* Ngoài ±3 độ lệch chuẩn là NGOÀI DẢI HUẤN LUYỆN. Mô hình tuyến tính không bao
   giờ từ chối trả lời — nó vẫn ra một con số trông hợp lý. Cờ này là thứ duy
   nhất phân biệt "pin sắp hỏng" với "mô hình đang đoán mò". */
#define RUL_Z_OUTLIER  3.0f

#define RUL_EOL_FRAC   {m["eol_frac"]:.10g}f
'''
    OUT.write_text(h)
    print(f"da ghi {OUT}  ({len(feats)} dac trung)")


if __name__ == "__main__":
    main()
