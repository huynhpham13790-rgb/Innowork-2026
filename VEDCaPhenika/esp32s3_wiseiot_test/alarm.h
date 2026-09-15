/* =============================================================================
 *  Báo động tại chỗ — còi + đèn.
 *
 *  VÌ SAO CẦN MODULE NÀY:
 *  Trước module này, hệ chỉ cảnh báo qua Serial và dashboard. Cả hai đều giả
 *  định có người đang nhìn màn hình. Kịch bản nguy hiểm nhất của pack pin là
 *  lúc sạc qua đêm trong nhà xe — không có màn hình nào được nhìn, và mạng thì
 *  đúng lúc đó cũng có thể mất. Cảnh báo phải phát ra ngay tại cục pin, bằng
 *  thứ đánh thức được người: âm thanh.
 *
 *  BỐN QUYẾT ĐỊNH THIẾT KẾ:
 *
 *  1. BỐN MỨC LEO THANG, không phải một cờ bật/tắt.
 *     "Đang theo dõi" khác "đã báo động" khác "quá nóng, cắt ngay". Gộp cả ba
 *     vào một cái còi kêu hay không kêu là ném đi thông tin mà người dùng cần
 *     để quyết định làm gì. Xem docs/HAI_LOP_AI_HOAT_DONG_THE_NAO.md.
 *
 *  2. MỨC NGUY KỊCH TỰ CHỐT, và nhả bằng TRỄ (hysteresis).
 *     Nhiệt độ dao động quanh đúng ngưỡng 60 °C sẽ làm còi kêu ngắt quãng liên
 *     tục — người nghe sẽ tưởng là lỗi vặt rồi rút điện cho đỡ ồn. Đã vượt 60
 *     thì phải tụt xuống dưới 55 mới hạ mức. Cùng một bài học với CT_MIN_CONSEC_OK
 *     ở cell_temp.h: ngưỡng đơn luôn sinh ra nhấp nháy.
 *
 *  3. CÓ NÚT TẮT CÒI, và tắt còi KHÔNG tắt đèn.
 *     Một cái còi không tắt được sẽ bị người ta rút dây — và lần sau nó không
 *     còn bảo vệ được ai nữa. Cho tắt tiếng, nhưng đèn vẫn đỏ và dashboard vẫn
 *     báo, nên sự cố không bị xoá khỏi tầm mắt. Tắt tiếng TỰ HUỶ khi mức báo
 *     động leo lên cao hơn — người dùng tắt tiếng cảnh báo AI không có nghĩa là
 *     họ đồng ý tắt luôn cảnh báo 60 °C sau đó.
 *
 *  4. KHÔNG CHẶN. Không delay(), không tone() chặn. Vòng lặp chính phải gọi
 *     mqtt.loop() đều đặn; một cái còi kêu bằng delay() là rớt kết nối.
 *
 *  PHẦN CỨNG: còi và LED rời chưa về. Module chạy được ngay bây giờ bằng LED
 *  RGB có sẵn trên ESP32-S3-DevKitC-1 (GPIO48). Khi linh kiện về thì chỉ sửa
 *  số chân ở dưới — logic không đổi, và đã được thử trước đó hàng giờ.
 * ========================================================================== */
#pragma once
#include <stdint.h>

/* --- Chân phần cứng ---------------------------------------------------------
 * Đặt -1 = chưa có linh kiện đó, module tự bỏ qua. Nhờ vậy cùng một firmware
 * chạy được cả trên bàn (chưa có còi) lẫn trên pack thật, không cần #ifdef rải
 * khắp nơi. */
#define AL_PIN_BUZZER   -1      // còi: đang đặt mua, chưa về
#define AL_PIN_LED_R    -1      // LED đỏ rời; -1 => dùng LED RGB trên board
#define AL_PIN_LED_G    -1
#define AL_PIN_MUTE      0      // nút BOOT sẵn có trên DevKitC-1 (nhấn = mức 0)

/* Còi chủ động (active) chỉ cần cấp điện là kêu. Còi thụ động (passive) phải
 * đưa xung vào mới kêu. Mua nhầm loại là chuyện rất hay xảy ra, nên hỗ trợ cả
 * hai — nếu cắm vào mà im, đổi số này trước khi nghi ngờ còi hỏng. */
#define AL_BUZZER_ACTIVE  1
#define AL_BUZZER_HZ      2730  // gần đỉnh cộng hưởng của còi thụ động phổ biến

/* Ngưỡng cứng, độc lập hoàn toàn với AI. Đây là lớp bảo vệ cuối cùng và không
 * bao giờ được bỏ đi vì "đã có AI" — autoencoder bỏ sót kiểu trôi nhiệt chậm
 * mà đều trên cả pack (xem ai/README.md). */
#define AL_T_CRIT        60.0f
#define AL_T_CRIT_CLEAR  55.0f  // trễ 5 °C, xem quyết định (2) ở trên

enum AlarmLevel : uint8_t {
  AL_OK       = 0,   // xanh lá, sáng nhẹ — "hệ đang sống và thấy bình thường"
  AL_WATCH    = 1,   // vàng nháy chậm  — AI thấy lệch nhưng chưa đủ lâu
  AL_ALARM    = 2,   // đỏ nháy + còi ngắt quãng — AI đã xác nhận bất thường
  AL_CRITICAL = 3,   // đỏ liên tục + còi liên tục — vượt ngưỡng cứng
};

/* Ngoài bốn mức trên còn một trạng thái riêng: MẤT CẢM BIẾN. Nó không nằm
 * trong thang leo thang vì nó không phải "pin nguy hiểm hơn" — nó là "tôi
 * không còn biết pin thế nào". Hai thứ đó đòi hỏi hành động khác nhau, nên
 * báo bằng màu khác (xanh dương nháy) chứ không trộn vào mức đỏ. */

class Alarm {
 public:
  void begin();

  /* Gọi mỗi vòng lặp, không chặn. Ba đầu vào là ba nguồn sự thật độc lập:
       ai_alarm   — Lớp 1 đã xác nhận bất thường
       ai_watch   — có cell đang vượt ngưỡng nhưng chưa đủ lâu
       t_max      — nhiệt độ cell nóng nhất (NAN nếu không đọc được)
       sensor_bad — không đủ cảm biến khoẻ
     Tách riêng để đường ngưỡng cứng KHÔNG đi qua AI: AI hỏng thì nó vẫn chạy. */
  void update(bool ai_alarm, bool ai_watch, float t_max, bool sensor_bad);

  AlarmLevel level() const { return lvl_; }
  bool  muted() const { return muted_; }
  /* Số lần leo lên mức ALARM trở lên kể từ lúc khởi động — đẩy lên dashboard
     để biết đêm qua có gì xảy ra mà không ai ở đó nghe. */
  uint32_t eventCount() const { return n_events_; }
  const char* levelName() const;

 private:
  void  setLed(uint8_t r, uint8_t g, uint8_t b);
  void  setBuzzer(bool on);
  void  pollMute();

  AlarmLevel lvl_ = AL_OK;
  bool     crit_latched_ = false;
  bool     sensor_bad_   = false;
  bool     muted_        = false;
  bool     buzz_on_      = false;
  uint32_t n_events_     = 0;
  uint32_t t_blink_      = 0;
  bool     blink_        = false;
  uint32_t t_mute_dbnc_  = 0;
  bool     mute_prev_    = true;   // nút nhả = mức cao (có điện trở kéo lên)
};
