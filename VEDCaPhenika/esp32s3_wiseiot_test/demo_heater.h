/* =============================================================================
 *  Sưởi một cell để diễn — có hạn mức cứng, cloud không nâng được.
 *
 *  VÌ SAO CẦN: kịch bản demo là "một cell nóng bất thường → Lớp 1 bắt được".
 *  Muốn diễn thì phải làm cho một cell nóng lên, và làm bằng tay thì không lặp
 *  lại được, không hẹn giờ được, và không dừng được từ xa.
 *
 *  ⚠️ ĐÂY LÀ MODULE DUY NHẤT TRONG FIRMWARE CHỦ ĐỘNG BƠM NHIỆT VÀO PACK PIN.
 *  Mọi thứ khác chỉ đọc. Nên nó được viết theo ba nguyên tắc, và cả ba đều đến
 *  từ chuyện đã xảy ra trên bàn thí nghiệm ngày 21/09 (QĐ-040):
 *
 *  1. CLOUD CHỈ ĐƯỢC XIN, THIẾT BỊ MỚI QUYẾT.
 *     Không có lệnh nào từ mạng nâng được các hạn mức dưới đây. Nút trên
 *     dashboard chỉ gửi "xin bật"; module tự kiểm rồi mới bật. Một nút bấm có
 *     thể nâng ngưỡng an toàn là một nút bấm đốt pack.
 *
 *  2. LỆNH XIN TỰ HẾT HẠN (công tắc chết người).
 *     Xin bật chỉ có hiệu lực DH_DEADMAN_MS rồi hết. Muốn sưởi tiếp thì phải
 *     xin lại. Mất mạng, sập Node-RED, đóng tab trình duyệt → sưởi TẮT, chứ
 *     không kẹt ở trạng thái bật lúc không ai nhìn.
 *
 *  3. HẠN MỨC THỜI GIAN KHÔNG ĐỌC CẢM BIẾN.
 *     Ngày 21/09 hệ an toàn của bench không cắt, vì nó chỉ nhìn nhiệt độ mà
 *     nhiệt độ lại đo ở chỗ không phải chỗ nóng nhất — đầu dò báo 47 °C trong
 *     khi thân điện trở đã bỏng tay, và thứ dừng thí nghiệm là BÀN TAY NGƯỜI.
 *     DH_MAX_ON_MS đếm thời gian, không đọc cảm biến: nó vẫn cắt khi đầu dò
 *     tuột, dán sai chỗ, hoặc tiếp xúc kém.
 *
 *  VÌ SAO TRẦN 50 °C CHỨ KHÔNG PHẢI 60:
 *  AL_T_CRIT = 60 °C là lớp bảo vệ CUỐI CÙNG. Nếu demo chạy tới 60 thì lớp cuối
 *  cùng thành lớp làm việc hằng ngày, và ta mất mất cái đệm. Thêm nữa, đo được
 *  ngày 21/09: cắt ở 40 °C mà nhiệt vẫn vọt tiếp lên 45,62 °C — ngưỡng cắt
 *  KHÔNG phải trần nhiệt. Trần 50 để vọt lố còn chỗ mà vọt.
 *
 *  VÌ SAO +6 °C LÀ ĐỦ: Lớp 1 bắt 100 % lỗi offset ở +2 °C (QĐ-039). Đẩy lên
 *  +6 là thừa gấp ba, mà vẫn an toàn tuyệt đối. Demo KHÔNG cần đến gần 60 °C.
 * ========================================================================== */
#pragma once
#include <stdint.h>
#include "pack_config.h"

#define DH_PIN            10        // D4184 #1 -> điện trở sưởi
#define DH_MAX_DELTA_C    6.0f      // dừng khi cell nóng nhất vượt trung vị pack bấy nhiêu
#define DH_RESUME_DELTA_C 4.5f      // bật lại khi tụt xuống dưới mức này (trễ, chống nhấp nháy)
#define DH_ABS_MAX_C      50.0f     // trần tuyệt đối — CỐ Ý thấp hơn AL_T_CRIT 60
#define DH_MAX_ON_MS      180000UL  // hạn mức thời gian; KHÔNG đọc cảm biến
#define DH_DEADMAN_MS     15000UL   // lệnh xin hết hiệu lực sau bấy nhiêu

enum DhState : uint8_t {
  DH_OFF = 0,     // không ai xin
  DH_HEATING,     // đang bơm nhiệt
  DH_HOLDING,     // đã tới +DH_MAX_DELTA_C, đang giữ (tắt, chờ nguội rồi bật lại)
  DH_BLOCKED,     // đã chốt vì chạm hạn mức an toàn — chỉ reset mới nhả
};

class DemoHeater {
 public:
  void begin();

  /* Xin bật trong `hold_ms` mili giây. Gọi lại để gia hạn. Đây là ĐIỂM VÀO DUY
     NHẤT từ bên ngoài (serial hoặc MQTT) — và nó chỉ XIN, không bật. */
  void request(uint32_t hold_ms = DH_DEADMAN_MS);

  /* Thu hồi lệnh xin ngay lập tức. */
  void stop();

  /* Gọi MỖI vòng lặp. temps = nhiệt các cell đã hiệu chỉnh; sensor_ok = bus còn
     đủ cảm biến khoẻ không. Tự quyết bật/tắt theo hạn mức. */
  void update(const float* temps, int n, bool sensor_ok);

  bool     on() const { return on_; }
  DhState  state() const { return st_; }
  const char* reason() const { return reason_; }
  /* Chênh lệch cell nóng nhất so với trung vị pack — số mà demo muốn đẩy lên. */
  float    delta() const { return delta_; }
  uint32_t secondsLeft() const;

 private:
  void  setPin(bool on);
  void  block(const char* why);

  bool     on_ = false;
  DhState  st_ = DH_OFF;
  const char* reason_ = "chua ai xin";
  float    delta_ = 0;
  uint32_t deadman_until_ = 0;
  uint32_t t_on_ = 0;          // lúc bật lần gần nhất
  uint32_t on_accum_ = 0;      // tổng thời gian đã bật trong phiên xin này
};
