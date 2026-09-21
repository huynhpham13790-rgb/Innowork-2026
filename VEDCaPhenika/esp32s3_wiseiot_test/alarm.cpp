#include <Arduino.h>
#include <math.h>
#include "alarm.h"

/* Nhịp nháy theo mức. Chọn khác nhau có chủ đích: người ở xa không đọc được
 * màu (đèn nhỏ, ban ngày, hoặc mù màu đỏ-lục — khoảng 8 % nam giới) nhưng vẫn
 * phân biệt được nhanh hay chậm. Màu là kênh phụ, nhịp mới là kênh chính. */
static const uint16_t BLINK_WATCH_MS    = 1000;  // chậm, không gây lo lắng
static const uint16_t BLINK_ALARM_MS    = 250;   // nhanh, rõ ràng là có chuyện
static const uint16_t BLINK_SENSORBAD_MS = 600;

void Alarm::begin() {
  if (AL_PIN_BUZZER >= 0) {
#if AL_BUZZER_ACTIVE
    pinMode(AL_PIN_BUZZER, OUTPUT);
    digitalWrite(AL_PIN_BUZZER, LOW);
#else
    ledcAttach(AL_PIN_BUZZER, AL_BUZZER_HZ, 10);
    ledcWrite(AL_PIN_BUZZER, 0);
#endif
  }
  if (AL_PIN_LED_R >= 0) { pinMode(AL_PIN_LED_R, OUTPUT); digitalWrite(AL_PIN_LED_R, LOW); }
  if (AL_PIN_LED_G >= 0) { pinMode(AL_PIN_LED_G, OUTPUT); digitalWrite(AL_PIN_LED_G, LOW); }

  // Nút BOOT đã có điện trở kéo lên trên board, nhưng khai báo INPUT_PULLUP
  // vẫn đúng và cần thiết nếu sau này dời sang chân khác.
  if (AL_PIN_MUTE >= 0) {
    pinMode(AL_PIN_MUTE, INPUT_PULLUP);
    /* Không cần nạp mức ban đầu nữa: pollMute() bây giờ đòi GIỮ nút liên tục
       AL_MUTE_HOLD_MS mới đảo, nên mọi mức lúc khởi động — kể cả GPIO0 còn
       đang thấp sau khi nạp — chỉ làm bộ đếm bắt đầu chạy rồi bị huỷ ngay khi
       chân lên cao. Không còn đường nào để một xung ngắn thành một cú nhấn. */
    delay(5);                              // cho điện trở kéo lên kịp ổn định
  }
  t_begin_ = millis();

  setLed(0, 0, 0);

  // Tự kiểm lúc khởi động: chớp đỏ-vàng-xanh một lượt. Không phải để cho đẹp —
  // đây là cách duy nhất biết đèn còn sống. Một đèn báo động cháy bóng trông
  // giống hệt một đèn báo "mọi thứ bình thường" khi mức đang là OK và đèn tắt.
  const uint8_t selftest[3][3] = { {60,0,0}, {60,40,0}, {0,60,0} };
  for (auto &c : selftest) { setLed(c[0], c[1], c[2]); delay(200); }
  setLed(0, 0, 0);
}

void Alarm::setLed(uint8_t r, uint8_t g, uint8_t b) {
  if (AL_PIN_LED_R >= 0) {
    digitalWrite(AL_PIN_LED_R, r ? HIGH : LOW);
    if (AL_PIN_LED_G >= 0) digitalWrite(AL_PIN_LED_G, g ? HIGH : LOW);
    return;
  }
#ifdef RGB_BUILTIN
  // LED trên board là WS2812 nên sáng chói kể cả ở giá trị thấp; giữ dưới 80
  // để nhìn được trong phòng mà không loá khi quay video demo.
  neopixelWrite(RGB_BUILTIN, r, g, b);
#else
  (void)r; (void)g; (void)b;
#endif
}

void Alarm::setBuzzer(bool on) {
  if (on == buzz_on_) return;          // tránh ghi chân mỗi vòng lặp vô ích
  buzz_on_ = on;
  if (AL_PIN_BUZZER < 0) return;
#if AL_BUZZER_ACTIVE
  digitalWrite(AL_PIN_BUZZER, on ? HIGH : LOW);
#else
  ledcWrite(AL_PIN_BUZZER, on ? 512 : 0);   // 50 % chu kỳ = to nhất
#endif
}

/* Đọc nút tắt tiếng: phải GIỮ đủ AL_MUTE_HOLD_MS mới đảo, và mỗi lần giữ chỉ
 * đảo MỘT lần (mute_fired_) — không thì giữ nút một giây sẽ đảo vài chục lần.
 * Lý do dùng nhấn-giữ thay vì bắt sườn xuống: xem chú thích ở AL_MUTE_HOLD_MS. */
void Alarm::pollMute() {
  if (AL_PIN_MUTE < 0) return;
  const uint32_t t = millis();

  /* Khoá nút trong 3 s đầu: đây đúng là cửa sổ mà mạch tự động nạp còn giữ
     GPIO0 xuống sau khi mở cổng serial (đo 21/09: giữ tới 998 ms, kèm reset
     board). Bỏ cả cửa sổ đi thì không cần đoán xung dài bao nhiêu nữa. */
  if (t - t_begin_ < AL_MUTE_ARM_MS) { t_mute_down_ = 0; return; }

  const bool down = !digitalRead(AL_PIN_MUTE);   // nhấn = LOW (có kéo lên)

  /* CHỐT QUAN TRỌNG NHẤT: chưa từng thấy nút ở trạng thái NHẢ thì không tính
     cú nhấn nào hết.

     Vì sao cần: DTR của cổng USB ghì GPIO0 xuống NGAY TỪ LÚC KHỞI ĐỘNG và giữ
     suốt phiên, nên nếu chỉ dựa vào "giữ rồi nhả" thì lúc đóng Serial Monitor
     sẽ sinh ra đúng một cú nhả trông y hệt người vừa buông tay — đo 21/09:
     khoảng giữ ~3 s, lọt gọn vào khoảng hợp lệ 0,6–5 s. Bản trước của tớ
     không chặn được chỗ này, và phép thử cũng không thấy vì board reset lúc
     mở cổng khiến hai đầu đo bằng nhau.

     Đòi thấy mức NHẢ trước thì cửa đó đóng hẳn: chân bị ghì từ lúc boot sẽ
     không bao giờ qua được vạch này, còn người dùng thật thì nút vốn đang nhả
     nên qua ngay ở vòng lặp đầu tiên. */
  if (!mute_seen_up_) {
    if (!down) mute_seen_up_ = true;   // đã thấy nhả: từ giờ mới nhận nhấn
    t_mute_down_ = 0;
    return;
  }

  if (down) {                                   // đang giữ: chỉ ghi mốc
    if (t_mute_down_ == 0) t_mute_down_ = t;
    return;
  }
  if (t_mute_down_ == 0) return;                // vốn đang nhả, không có gì

  const uint32_t held = t - t_mute_down_;
  t_mute_down_ = 0;

  /* Đảo lúc NHẢ, và chỉ khi thời gian giữ nằm trong khoảng của một bàn tay
     người. Chặn được cả hai kiểu nhấn giả:
       - giữ quá NGẮN : nhiễu điện
       - giữ quá DÀI  : không phải người, mà là máy đang ghì chân xuống
     Vế thứ hai mới là vế quan trọng. Đo 21/09 bằng test/dtr_mute_check.py:
     GPIO0 cũng là chân DTR của cổng USB, nên hễ có ai mở Serial Monitor là
     chân bị kéo thấp LIÊN TỤC cho tới lúc đóng cổng — không phải một xung.
     Không bao giờ có cú nhả thì không bao giờ đảo. */
  if (held < AL_MUTE_HOLD_MS || held > AL_MUTE_MAX_MS) return;

  muted_ = !muted_;
  Serial.printf("[ALRM] %s coi (giu nut %lu ms)\n",
                muted_ ? "TAT TIENG" : "BAT LAI TIENG", (unsigned long)held);
}

void Alarm::setMuted(bool m) {
  if (muted_ == m) return;
  muted_ = m;
  Serial.printf("[ALRM] %s coi (lenh tu xa)\n", m ? "TAT TIENG" : "BAT LAI TIENG");
}

void Alarm::setQuiet(bool q) {
  if (quiet_ == q) return;
  quiet_ = q;
  Serial.printf("[ALRM] bip thua: %s (lenh tu xa)\n", q ? "BAT" : "TAT");
}

void Alarm::update(bool ai_alarm, bool ai_watch, float t_max, bool sensor_bad) {
  pollMute();

  /* --- Đường ngưỡng cứng: KHÔNG phụ thuộc AI, xử lý trước mọi thứ khác ---
     Nếu cell_ai.cpp có lỗi và không bao giờ báo động, nhánh này vẫn chạy.
     Đó chính là lý do nó là một đường riêng chứ không phải một điều kiện ghép
     vào ai_alarm. */
  if (!isnan(t_max)) {
    if (t_max >= AL_T_CRIT)            crit_latched_ = true;
    else if (t_max < AL_T_CRIT_CLEAR)  crit_latched_ = false;   // trễ 5 °C
  }

  const AlarmLevel prev = lvl_;
  if      (crit_latched_) lvl_ = AL_CRITICAL;
  else if (ai_alarm)      lvl_ = AL_ALARM;
  else if (ai_watch)      lvl_ = AL_WATCH;
  else                    lvl_ = AL_OK;
  sensor_bad_ = sensor_bad;

  if (lvl_ != prev) {
    // Leo thang thì huỷ tắt tiếng — xem quyết định (3) ở alarm.h. Người dùng
    // tắt tiếng cảnh báo AI không có nghĩa là họ chấp nhận im lặng khi sau đó
    // pin vượt 60 °C.
    // Leo thang huỷ CẢ tắt tiếng lẫn bíp thưa. Người dùng chấp nhận nghe ít
    // hơn khi AI nghi ngờ, không có nghĩa là họ chấp nhận nghe ít hơn khi pin
    // đã vượt 60 °C.
    if (lvl_ > prev) { muted_ = false; quiet_ = false; }
    if (lvl_ >= AL_ALARM && prev < AL_ALARM) n_events_++;
    Serial.printf("[ALRM] %s -> %s%s\n",
                  prev == AL_OK ? "OK" : (prev == AL_WATCH ? "THEO DOI" :
                  (prev == AL_ALARM ? "BAO DONG" : "NGUY KICH")),
                  levelName(), muted_ ? " (dang tat tieng)" : "");
  }

  /* --- Nháy và kêu ---------------------------------------------------- */
  uint16_t period = 0;
  switch (lvl_) {
    case AL_WATCH:    period = BLINK_WATCH_MS; break;
    case AL_ALARM:    period = BLINK_ALARM_MS; break;
    case AL_CRITICAL: period = 0;              break;   // sáng/kêu liên tục
    case AL_OK:       period = sensor_bad_ ? BLINK_SENSORBAD_MS : 0; break;
  }
  const uint32_t t = millis();
  if (period && t - t_blink_ >= period) { t_blink_ = t; blink_ = !blink_; }
  if (!period) blink_ = true;

  switch (lvl_) {
    case AL_CRITICAL: setLed(80, 0, 0);                          break;
    case AL_ALARM:    setLed(blink_ ? 80 : 0, 0, 0);             break;
    case AL_WATCH:    setLed(blink_ ? 70 : 0, blink_ ? 45 : 0, 0); break;
    case AL_OK:
      // Mất cảm biến: xanh DƯƠNG, không phải đỏ. "Tôi không biết" là một trạng
      // thái khác hẳn "pin nguy hiểm", và trộn hai cái vào một màu sẽ dạy người
      // dùng bỏ qua màu đỏ. Xem TH-3 trong HAI_LOP_AI_HOAT_DONG_THE_NAO.md.
      if (sensor_bad_) setLed(0, 0, blink_ ? 80 : 0);
      else             setLed(0, 12, 0);   // xanh mờ = "còn sống, đang canh"
      break;
  }

  // Còi chỉ kêu từ mức BAO DONG trở lên. Mức THEO DOI cố tình im: nếu mỗi lần
  // AI hơi nghi là còi kêu thì chỉ sau vài đêm người ta sẽ tắt vĩnh viễn.
  /* Còi chỉ kêu từ mức BAO DONG trở lên (xem chú thích ngay trên).
     Chế độ bíp thưa rút tiếng xuống còn AL_QUIET_BEEP_MS đầu mỗi chu kỳ —
     nhưng CHỈ ở mức BAO DONG. Mức NGUY KỊCH là ngưỡng cứng 60 °C, lớp bảo vệ
     cuối cùng; cho phép làm nó thưa đi là mở lại đúng cánh cửa mà quyết định
     (3) ở alarm.h đóng lại. */
  const bool quiet_gate = (!quiet_ || lvl_ >= AL_CRITICAL ||
                           (t - t_blink_) < AL_QUIET_BEEP_MS);
  setBuzzer(lvl_ >= AL_ALARM && !muted_ && blink_ && quiet_gate);
}

const char* Alarm::levelName() const {
  switch (lvl_) {
    case AL_CRITICAL: return "NGUY KICH";
    case AL_ALARM:    return "BAO DONG";
    case AL_WATCH:    return "THEO DOI";
    default:          return sensor_bad_ ? "OK (thieu cam bien)" : "OK";
  }
}
