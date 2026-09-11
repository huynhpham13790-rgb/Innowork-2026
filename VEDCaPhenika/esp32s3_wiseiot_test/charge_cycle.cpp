#include "charge_cycle.h"
#include <math.h>

// PHẢI khớp thứ tự FEATURES trong ai/nasa_prepare.py — cloud nhân hệ số theo
// đúng thứ tự này, đảo một chỗ là kết quả sai mà không báo lỗi gì.
const char* CC_FEATURE_NAMES[CC_N_FEATURES] = {
  "t_v_interval", "t_cv", "i_cv_mean", "t_charge_total",
  "T_max_ch", "T_mean_ch", "T_rise_ch", "v_start", "dvdt_cc"
};

void ChargeCycle::reset_() {
  t_ = 0;
  v_start_ = 0; t_lo_ = -1; t_hi_ = -1; t_cv0_ = -1;
  T_first_ = 0; T_max_ = -1000.0f; T_sum_ = 0; T_n_ = 0;
  i_cv_sum_ = 0; i_cv_n_ = 0;
  sx_ = sy_ = sxx_ = sxy_ = 0; sn_ = 0;
}

void ChargeCycle::begin() { active_ = false; reset_(); }

ChargeSummary ChargeCycle::finish_() {
  ChargeSummary s;
  s.valid = false;
  s.duration_s = t_;
  for (int i = 0; i < CC_N_FEATURES; i++) s.f[i] = NAN;

  // Chu kỳ quá ngắn, hoặc không đi hết khoảng điện áp cần đo -> bỏ.
  // Sạc dở chừng rồi rút ra là chuyện thường, đừng gửi rác lên cloud.
  if (t_ < CC_MIN_S || t_lo_ < 0 || t_hi_ < 0 || t_hi_ <= t_lo_) return s;

  float dvdt = 0.0f;
  if (sn_ > 5) {
    double den = sn_ * sxx_ - sx_ * sx_;
    if (fabs(den) > 1e-9) dvdt = (float)((sn_ * sxy_ - sx_ * sy_) / den);
  }

  s.f[0] = t_hi_ - t_lo_;                                   // t_v_interval
  s.f[1] = (t_cv0_ >= 0) ? (float)t_ - t_cv0_ : 0.0f;       // t_cv
  s.f[2] = i_cv_n_ ? (float)(i_cv_sum_ / i_cv_n_) : 0.0f;   // i_cv_mean
  s.f[3] = (float)t_;                                       // t_charge_total
  s.f[4] = T_max_;                                          // T_max_ch
  s.f[5] = T_n_ ? (float)(T_sum_ / T_n_) : 0.0f;            // T_mean_ch
  s.f[6] = T_max_ - T_first_;                               // T_rise_ch
  s.f[7] = v_start_;                                        // v_start
  s.f[8] = dvdt;                                            // dvdt_cc
  s.valid = true;
  return s;
}

ChargeSummary ChargeCycle::update(float pack_voltage, float current,
                                  float temperature) {
  ChargeSummary none;
  none.valid = false;
  none.duration_s = t_;

  // Mô hình train trên pin ĐƠN, nên quy điện áp pack về trung bình mỗi cell.
  // Đây là xấp xỉ: pack thật có thể lệch cell, nhưng BMS đã cân bằng nên sai
  // số nhỏ so với sai số của chính mô hình (±12 chu kỳ).
  const float v = pack_voltage / (float)CC_N_CELLS;

  if (!active_) {
    if (!(current > CC_I_START)) return none;
    // Bắt đầu một chu kỳ sạc. KHÔNG return ở đây — phải xử lý luôn chính mẫu
    // này, vì bên Python cửa sổ sạc BAO GỒM mẫu đầu tiên có dòng. Bản đầu
    // return sớm nên lệch đúng 1 giây, test so C/Python đã bắt được.
    active_ = true;
    reset_();
    v_start_ = v;
    T_first_ = temperature;
  }

  // ---- đang sạc ----
  T_max_ = temperature > T_max_ ? temperature : T_max_;
  T_sum_ += temperature; T_n_++;

  if (t_lo_ < 0 && v >= CC_V_LO) t_lo_ = (float)t_;
  if (t_hi_ < 0 && v >= CC_V_HI) t_hi_ = (float)t_;
  if (t_cv0_ < 0 && v >= CC_V_CV) t_cv0_ = (float)t_;

  if (v >= CC_V_LO && v <= CC_V_HI) {    // tích luỹ cho hồi quy dvdt
    sx_ += t_; sy_ += v; sxx_ += (double)t_ * t_; sxy_ += (double)t_ * v; sn_++;
  }
  if (t_cv0_ >= 0) { i_cv_sum_ += current; i_cv_n_++; }

  // Kết thúc: đã vào CV và dòng đã tụt đủ thấp -> pin no.
  if (t_cv0_ >= 0 && current < CC_I_END) {
    ChargeSummary s = finish_();
    active_ = false;
    return s;
  }
  // Hoặc người dùng rút sạc giữa chừng -> chốt sổ luôn, finish_() sẽ tự loại
  // nếu chu kỳ chưa đủ dữ liệu.
  if (current < CC_I_END) {
    ChargeSummary s = finish_();
    active_ = false;
    return s;
  }

  t_++;
  return none;
}
