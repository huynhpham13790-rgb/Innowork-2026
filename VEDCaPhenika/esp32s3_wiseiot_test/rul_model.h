/* =============================================================================
 *  Hệ số Lớp 2 (RUL/SOH) — SINH TỰ ĐỘNG, ĐỪNG SỬA TAY.
 *
 *  Sinh bởi: ai/export_rul_c.py, từ ai/models/rul_model.json
 *  Cùng nguồn với VEDCaPhenika/planb_cloud/nodered/rul_predict.js — sửa mô
 *  hình thì chạy lại export_rul_model.py rồi export_rul_c.py, ĐỪNG gõ số vào
 *  đây. Hai bản chép tay sẽ lệch nhau, và lúc lệch thì cả hai đều trả về một
 *  con số trông hợp lý nên không ai phát hiện.
 *
 *  Mô hình (QĐ-017): RUL = hồi quy tuyến tính trên MỘT đặc trưng `t_cv`;
 *  SOH = Ridge trên cả 9. Đã thử LSTM ~3.500 tham số và nó THUA mô hình 2 tham
 *  số này (MAE 20,3 so với 12,2 chu kỳ) — xem QĐ-017 trước khi đổi.
 *
 *  Train trên: NASA PCoE — 4 pin, 364 chu kỳ.
 *  Sai số kỳ vọng (leave-one-battery-out): RUL ±12.07 chu kỳ.
 *
 *  ⚠️ Hệ số mượn của NASA (18650 đơn, 2,0 Ah, sạc 0,75C). Pack đội là 2,55 Ah
 *     nên PHẢI sạc ở 1,9 A mới cùng tốc độ C — xem cảnh báo trong charge_cycle.h.
 * ========================================================================== */
#pragma once

#define RUL_N_FEAT 9

/* Thứ tự PHẢI khớp FEATURES trong ai/nasa_prepare.py và ChargeSummary::f[]:
   t_v_interval, t_cv, i_cv_mean, t_charge_total, T_max_ch, T_mean_ch, T_rise_ch, v_start, dvdt_cc */

static const float RUL_MEAN[RUL_N_FEAT] = {
  2074.847525f,
  2813.042973f,
  0.6288362623f,
  5566.027104f,
  30.1086116f,
  26.71054261f,
  2.271116423f,
  3.615197398f,
  0.0001119070904f
};

static const float RUL_STD[RUL_N_FEAT] = {
  289.1027485f,
  171.679238f,
  0.02205949217f,
  358.4939484f,
  1.936197358f,
  0.7524074021f,
  1.58607215f,
  0.1599310514f,
  2.428700949e-05f
};

static const float SOH_COEF[RUL_N_FEAT] = {
  -0.01955931644f,
  -0.05180911098f,
  0.01009103183f,
  0.07254137301f,
  0.003699165545f,
  -0.008248802943f,
  0.004887006816f,
  0.003572543595f,
  0.04071775721f
};

static const float SOH_INTERCEPT = 0.912627511f;

/* RUL chỉ dùng MỘT đặc trưng, nên lưu sẵn chỉ số của nó thay vì dò tên lúc chạy. */
#define RUL_FEAT_IDX  1   /* t_cv */
static const float RUL_COEF      = -26.96290061f;
static const float RUL_INTERCEPT = 48.30494505f;

/* Ngoài ±3 độ lệch chuẩn là NGOÀI DẢI HUẤN LUYỆN. Mô hình tuyến tính không bao
   giờ từ chối trả lời — nó vẫn ra một con số trông hợp lý. Cờ này là thứ duy
   nhất phân biệt "pin sắp hỏng" với "mô hình đang đoán mò". */
#define RUL_Z_OUTLIER  3.0f

#define RUL_EOL_FRAC   0.8f
