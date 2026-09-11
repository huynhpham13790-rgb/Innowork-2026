/* =============================================================================
 *  Lớp 2 (phần trên thiết bị) — tóm tắt một chu kỳ SẠC thành 9 con số.
 *
 *  ESP32 không chạy mô hình RUL. Nó chỉ quan sát suốt quá trình sạc rồi gửi
 *  lên cloud ~9 con số; cloud nhân với hệ số đã train sẵn ra RUL và SOH.
 *  Vì sao chia như vậy: RUL không cần real-time (1 lần/chu kỳ sạc là đủ), và
 *  để cloud tính thì đổi mô hình không phải nạp lại firmware — quan trọng khi
 *  mô hình còn phải hiệu chỉnh lại cho pack thật (xem cảnh báo bên dưới).
 *
 *  9 đặc trưng phải KHỚP thứ tự trong ai/nasa_prepare.py FEATURES.
 *
 *  ================== CẢNH BÁO VỀ PHẠM VI ÁP DỤNG ==========================
 *  Hệ số hiện tại train trên NASA PCoE: pin 18650 ĐƠN, 2 Ah, sạc 1,5 A (~0,75C).
 *  Pack của đội là 8S cell LG HG2 3 Ah. Hai thứ khác nhau về:
 *    - điện áp: pack ~33,6 V, nên phải quy về điện áp TRUNG BÌNH MỖI CELL
 *    - tốc độ sạc: t_cv phụ thuộc mạnh vào tỉ lệ dòng sạc / dung lượng
 *  => Đường ống dữ liệu chạy đúng, nhưng CON SỐ RUL chưa dùng được cho pack
 *     thật cho tới khi hiệu chỉnh lại trên chính pack đó. Đừng đưa con số này
 *     lên slide như thể đã đo trên pin của đội.
 * ========================================================================== */
#pragma once
#include <stdint.h>

#define CC_N_FEATURES 9
#define CC_N_CELLS    8      // pack 8S: quy điện áp pack về trung bình mỗi cell

// Ngưỡng tính theo ĐIỆN ÁP MỖI CELL, khớp ai/nasa_prepare.py
#define CC_V_LO     3.90f    // mốc bắt đầu tính khoảng điện áp cố định
#define CC_V_HI     4.15f    // mốc kết thúc
#define CC_V_CV     4.19f    // coi như đã vào pha giữ áp (CV)
#define CC_I_START  0.10f    // dòng vượt mức này = bắt đầu sạc (A)
#define CC_I_END    0.20f    // dòng tụt dưới mức này = sạc xong (~C/10). KHÔNG dùng
                             // 0,02 A của NASA: đuôi dòng thấp dài bao lâu là do
                             // bộ sạc, không phải do sức khoẻ pin.
#define CC_MIN_S    600      // chu kỳ ngắn hơn 10 phút coi là hỏng, bỏ

struct ChargeSummary {
  bool  valid;
  float f[CC_N_FEATURES];    // đúng thứ tự FEATURES trong nasa_prepare.py
  uint32_t duration_s;
};

extern const char* CC_FEATURE_NAMES[CC_N_FEATURES];

class ChargeCycle {
 public:
  void begin();

  /* Gọi 1 Hz với điện áp PACK (V), dòng sạc (A, dương = đang nạp),
     nhiệt độ pack (°C). Trả về summary.valid = true đúng một lần, ở giây
     kết thúc chu kỳ sạc. */
  ChargeSummary update(float pack_voltage, float current, float temperature);

  bool charging() const { return active_; }

 private:
  bool     active_ = false;
  uint32_t t_ = 0;                 // giây kể từ lúc bắt đầu sạc

  float v_start_ = 0;
  float t_lo_ = -1, t_hi_ = -1;    // thời điểm chạm CC_V_LO / CC_V_HI
  float t_cv0_ = -1;               // thời điểm vào CV

  float T_first_ = 0, T_max_ = 0;
  double T_sum_ = 0;
  uint32_t T_n_ = 0;

  double i_cv_sum_ = 0;            // để tính dòng trung bình pha CV
  uint32_t i_cv_n_ = 0;

  // hồi quy tuyến tính V theo t trong khoảng [CC_V_LO, CC_V_HI] -> dvdt_cc
  double sx_ = 0, sy_ = 0, sxx_ = 0, sxy_ = 0;
  uint32_t sn_ = 0;

  void reset_();
  ChargeSummary finish_();
};
