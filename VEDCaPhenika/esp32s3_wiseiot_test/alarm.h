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
/* Còi SFM-27 qua module D4184 #2. Trước 21/09 chỗ này là -1 ("còi đang đặt mua")
   và không ai sửa lại sau khi còi về — nên firmware chính báo động mà LOA IM.
   Chỉ lộ ra khi chạy trọn kịch bản đầu-cuối, không lộ ra ở bất kỳ bench nào.
   Còi đã nghiệm thu ở T4 (bring-up 20/09, tai người xác nhận). */
#define AL_PIN_BUZZER    5
#define AL_PIN_LED_R    -1      // LED đỏ rời; -1 => dùng LED RGB trên board
#define AL_PIN_LED_G    -1
#define AL_PIN_MUTE      0      // nút BOOT sẵn có trên DevKitC-1 (nhấn = mức 0)

/* Phải GIỮ nút đủ lâu mới đảo tắt tiếng — không bắt sườn xuống.
 *
 * VÌ SAO: GPIO0 cũng là chân mà mạch tự động nạp trên board điều khiển qua
 * DTR/RTS. Mở cổng serial (Serial Monitor của Arduino IDE, hay bất cứ script
 * nào) sẽ kéo GPIO0 xuống một nhịp ngắn, và bản bắt-sườn-xuống đọc cái đó y
 * hệt một cú nhấn nút. Đo ngày 21/09: mở cổng 3 lần thì 2 lần đảo trạng thái.
 * Ngẫu nhiên nên còn khó tìm hơn là luôn xảy ra.
 *
 * Hậu quả nếu để nguyên: ai mở Serial Monitor lúc demo là còi có thể đã tắt
 * tiếng mà không ai biết — báo động vẫn leo mức, đèn vẫn đỏ, chỉ có tiếng là
 * không kêu. Đúng họ hàng với lỗi AL_PIN_BUZZER = -1 tìm ra hôm trước.
 *
 * ĐO LẠI 21/09 SAU KHI SỬA LẦN 1: giả định "xung DTR chỉ vài chục ms" là SAI.
 * Đo thật: GPIO0 bị giữ thấp tới 998 ms, nên riêng ngưỡng giữ 600 ms không
 * chặn được gì. Nhật ký còn cho thấy mở cổng serial làm board RESET — dòng
 * [BLE] quảng bá hiện ra trước dòng [ALRM] — nên cú nhấn giả luôn rơi vào
 * ngay sau khi khởi động, chứ không rải rác giữa chừng.
 *
 * Vì vậy dùng HAI lớp, mỗi lớp chặn một kiểu:
 *   AL_MUTE_ARM_MS  — khoá nút hẳn trong 3 s đầu. Diệt đúng cửa sổ mà mạch tự
 *                     động nạp còn đang giữ GPIO0. Không ai nhấn nút tắt tiếng
 *                     trong 3 giây đầu bật máy; nếu có thì nhấn lại là xong.
 *   AL_MUTE_HOLD_MS — sau khi mở khoá thì vẫn đòi giữ, chặn nhiễu lẻ tẻ.
 *
 * Lưu ý: pollMute() chạy theo nhịp gAlarm.update(), tức 1 Hz, nên độ phân giải
 * của phép giữ chỉ khoảng 1 giây. Đó là lý do nhật ký in "giu nut 998 ms" chứ
 * không phải 600. Đủ dùng cho một cái nút bấm tay. */
/* Khoảng hợp lệ của một cú nhấn tay. Hai mốc này chỉ có nghĩa khi pollButton()
   được gọi mỗi vòng loop(); ở nhịp 1 Hz thì độ phân giải ~1 s nuốt mất cả
   khoảng — đó đúng là lý do cú nhấn thứ hai của người dùng biến mất 21/09.

   HAI MỐC NÀY LÀ SỐ ĐO, KHÔNG PHẢI SỐ ĐOÁN (test/dtr_mute_check.py + nhật ký
   chẩn đoán, 21/09):
     - nhấn bình thường  : 199, 232, 254, 205, 241, 259 ms  -> đều phải ăn
     - giữ "khoảng 1 giây": 1735 ms                          -> cũng phải ăn
   Bản đầu đặt tối thiểu 600 ms đã LOẠI SẠCH mọi cú nhấn bình thường, và trần
   1500 ms loại nốt cú giữ lâu. Người ta luôn giữ lâu hơn mình nghĩ.
   120..2500 ms ôm trọn cả hai mà vẫn đủ hẹp: một phiên serial ngắn hơn 2,5 s
   gần như không xảy ra, và cửa boot đã bị khoá riêng bằng mute_seen_up_. */
#define AL_MUTE_HOLD_MS  120     // giữ ít hơn => nhiễu, bỏ qua
#define AL_MUTE_MAX_MS   2500    // giữ lâu hơn => máy ghì chân, KHÔNG phải người
#define AL_MUTE_ARM_MS   3000    // khoá hẳn nút trong 3 s đầu sau khởi động

/* Còi chủ động (active) chỉ cần cấp điện là kêu. Còi thụ động (passive) phải
 * đưa xung vào mới kêu. Mua nhầm loại là chuyện rất hay xảy ra, nên hỗ trợ cả
 * hai — nếu cắm vào mà im, đổi số này trước khi nghi ngờ còi hỏng. */
#define AL_BUZZER_ACTIVE  1
#define AL_BUZZER_HZ      2730
#define AL_QUIET_BEEP_MS  90    // độ dài tiếng bíp ở chế độ bíp thưa  // gần đỉnh cộng hưởng của còi thụ động phổ biến

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

  /* Đọc nút tắt tiếng. PHẢI gọi MỖI VÒNG loop(), không phải theo nhịp 1 Hz
     của update().
     Lý do: một cú nhấn tay chỉ kéo dài vài trăm ms. Lấy mẫu 1 Hz thì cú nhấn
     có thể lọt trọn vẹn vào giữa hai lần đọc và KHÔNG BAO GIỜ được nhìn thấy —
     người dùng bấm, không có gì xảy ra, và không có cách nào biết vì sao.
     Đã xảy ra thật 21/09: cú nhấn thứ nhất (giữ ~1 s) ăn, cú thứ hai bị bỏ
     lọt hoàn toàn. */
  void pollButton() { pollMute(); }

  AlarmLevel level() const { return lvl_; }
  bool  muted() const { return muted_; }
  bool  quiet() const { return quiet_; }

  /* Tắt tiếng / bật lại từ XA (nút trên dashboard). Đi qua đúng cờ mà nút bấm
     tại chỗ dùng, nên mọi quy tắc cũ vẫn áp dụng — đặc biệt là "leo thang thì
     huỷ tắt tiếng". Không có đường tắt nào khác vào còi. */
  void setMuted(bool m);

  /* Bíp THƯA: kêu một nhịp ngắn mỗi chu kỳ thay vì kêu suốt nửa chu kỳ.
     ⚠️ KHÔNG phải giảm âm lượng. SFM-27 là còi CHỦ ĐỘNG — mạch dao động nằm
     trong thân còi, chỉ có hai trạng thái có điện / không điện. Băm PWM nguồn
     của nó không làm nhỏ tiếng mà chỉ chặt tiếng thành từng đoạn. Thứ giảm
     được là mức gây khó chịu, không phải decibel. Nói đúng tên để sau này
     không ai trông đợi nhầm.
     KHÔNG áp dụng ở mức NGUY KỊCH: ngưỡng cứng 60 °C là lớp bảo vệ cuối cùng,
     nó phải kêu hết cỡ. */
  void setQuiet(bool q);
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
  bool     quiet_        = false;
  bool     buzz_on_      = false;
  uint32_t n_events_     = 0;
  uint32_t t_blink_      = 0;
  bool     blink_        = false;
  uint32_t t_begin_      = 0;      // mốc begin(), để khoá nút lúc mới khởi động
  bool     mute_seen_up_ = false;  // đã từng thấy nút ở trạng thái NHẢ chưa
  uint32_t t_mute_down_  = 0;      // lúc bắt đầu giữ nút; 0 = đang nhả
};
