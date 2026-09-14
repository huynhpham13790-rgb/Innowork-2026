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
 *  ================== PHẠM VI ÁP DỤNG (cập nhật 14/09/2026) ================
 *  Hệ số hiện tại train trên NASA PCoE: pin 18650 ĐƠN, 2,0 Ah, sạc 1,5 A (0,75C).
 *  Pack của đội: 8S × 18650 **2,55 Ah** (2 đế 4 cell nối tiếp).
 *
 *  Tin tốt: CÙNG loại cell 18650, dung lượng chỉ lệch 27 % — gần hơn nhiều so
 *  với giả định cũ trong tài liệu (LG HG2 3 Ah, nay đã biết là sai). Nghĩa là
 *  dáng đường sạc của hai bên tương đồng, và việc mượn hệ số NASA có cơ sở
 *  hơn hẳn.
 *
 *  ĐIỀU KIỆN để sự tương đồng đó thành thật: phải sạc ở CÙNG TỐC ĐỘ C.
 *    NASA: 1,5 A / 2,00 Ah = 0,75C
 *    đội : cần 0,75 × 2,55 = **1,9 A**
 *  Sạc ở dòng khác đi thì t_cv và dvdt_cc lệch hệ thống, và mô hình sai theo
 *  một hướng cố định — kiểu sai khó phát hiện nhất.
 *
 *  Vẫn còn: điện áp pack ~33,6 V nên phải quy về TRUNG BÌNH MỖI CELL (đã làm,
 *  xem CC_N_CELLS).
 *
 *  => Đường ống chạy đúng. Con số RUL vẫn nên trình bày là "mô hình hiệu chỉnh
 *     trên bộ chuẩn NASA, cùng loại cell 18650", KHÔNG phải "đo trên pin của
 *     chúng em". Xem docs/DECISION_LOG.md QĐ-026.
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
