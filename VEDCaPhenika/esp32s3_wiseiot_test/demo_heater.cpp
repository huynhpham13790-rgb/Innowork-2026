#include <Arduino.h>
#include <math.h>
#include "demo_heater.h"

void DemoHeater::begin() {
  // LOW trước mọi thứ khác. Chân ESP32 lúc reset thả nổi; module D4184 có điện
  // trở kéo xuống (đã đo 21/09: thả nổi = 0,0 mA) nhưng không được dựa vào đó.
  pinMode(DH_PIN, OUTPUT);
  digitalWrite(DH_PIN, LOW);
  on_ = false; st_ = DH_OFF; reason_ = "chua ai xin";
}

void DemoHeater::setPin(bool on) {
  on_ = on;
  digitalWrite(DH_PIN, on ? HIGH : LOW);
}

void DemoHeater::block(const char* why) {
  setPin(false);
  st_ = DH_BLOCKED;
  reason_ = why;
  deadman_until_ = 0;
  Serial.printf("[HEAT] CHAN: %s — nhan RESET de nha\n", why);
}

void DemoHeater::request(uint32_t hold_ms) {
  if (st_ == DH_BLOCKED) {            // đã chốt thì không lệnh nào mở lại được
    Serial.println("[HEAT] bo qua lenh xin: dang bi chan, nhan RESET");
    return;
  }
  if (hold_ms > DH_DEADMAN_MS) hold_ms = DH_DEADMAN_MS;   // cloud không nới được
  deadman_until_ = millis() + hold_ms;
  if (st_ == DH_OFF) { on_accum_ = 0; }                   // phiên mới, đếm lại
}

void DemoHeater::stop() {
  setPin(false);
  deadman_until_ = 0;
  if (st_ != DH_BLOCKED) { st_ = DH_OFF; reason_ = "da thu hoi lenh"; }
}

uint32_t DemoHeater::secondsLeft() const {
  const uint32_t now = millis();
  return (deadman_until_ > now) ? (deadman_until_ - now) / 1000 : 0;
}

void DemoHeater::update(const float* temps, int n, bool sensor_ok) {
  const uint32_t now = millis();

  // cộng dồn thời gian đã bật TRƯỚC mọi quyết định, để hạn mức không bị bỏ sót
  if (on_) on_accum_ += (now - t_on_);
  t_on_ = now;

  if (st_ == DH_BLOCKED) { setPin(false); return; }

  /* --- KHÔNG AI XIN VÀ ĐANG TẮT => không có gì để bảo vệ, nằm im, KHÔNG chốt.
     Đây là lỗi đã mắc và đã sửa ngay trong ngày 21/09: bản đầu kiểm mọi hạn mức
     ở mọi vòng lặp, nên ngay lúc khởi động — khi cảm biến chưa có số đọc đầu
     tiên và gCellTemp[] còn toàn NAN — module tự chốt vĩnh viễn, dù sáu cảm
     biến đều khoẻ và chẳng ai định sưởi gì cả.
     Bài học: hệ an toàn phải phân biệt "đang nguy hiểm" với "đang khởi động".
     Chốt trên một trạng thái quá độ bình thường thì chỉ tổ làm người ta tìm
     cách vô hiệu hoá nó. --- */
  const bool wanted = (int32_t)(deadman_until_ - now) > 0;
  if (!wanted && !on_) {
    setPin(false);
    if (st_ != DH_OFF) { st_ = DH_OFF; reason_ = "lenh xin het han"; }
    return;
  }

  /* --- hạn mức 3: thời gian. KHÔNG đọc cảm biến, nên vẫn cắt khi đầu dò tuột,
     dán sai chỗ, hay tiếp xúc kém. Xem chú thích đầu demo_heater.h. --- */
  if (on_accum_ > DH_MAX_ON_MS) { block("qua han muc thoi gian"); return; }

  /* --- mất cảm biến = coi như quá nhiệt. Không đọc được nghĩa là không biết,
     mà không biết thì phải giả định điều xấu nhất — nếu không, cách dễ nhất để
     qua mặt hệ này là làm hỏng một cảm biến. --- */
  if (!sensor_ok) { block("mat cam bien"); return; }

  // trung vị pack: dùng trung vị chứ không phải trung bình, vì chính cell đang
  // bị sưởi sẽ kéo lệch trung bình và làm nó trông đỡ nóng đi
  float v[PACK_N_CELLS]; int m = 0;
  float tmax = -1000.0f;
  for (int i = 0; i < n && m < PACK_N_CELLS; i++) {
    if (isnan(temps[i])) continue;
    v[m++] = temps[i];
    if (temps[i] > tmax) tmax = temps[i];
  }
  if (m < 3) { block("khong du cell doc duoc"); return; }
  for (int i = 1; i < m; i++) { float x=v[i]; int j=i-1; while (j>=0 && v[j]>x) { v[j+1]=v[j]; j--; } v[j+1]=x; }
  const float med = (m % 2) ? v[m/2] : 0.5f*(v[m/2-1]+v[m/2]);
  delta_ = tmax - med;

  // --- hạn mức 1: trần tuyệt đối, cố ý thấp hơn AL_T_CRIT ---
  if (tmax >= DH_ABS_MAX_C) { block("cham tran tuyet doi 50 degC"); return; }

  // --- lệnh xin hết hạn giữa chừng: tắt ngay (công tắc chết người) ---
  if (!wanted) {
    setPin(false);
    if (st_ != DH_OFF) { st_ = DH_OFF; reason_ = "lenh xin het han"; }
    return;
  }

  /* --- hạn mức 2: chênh lệch đủ cho Lớp 1 rồi thì thôi, có TRỄ.
     Lớp 1 bắt 100 % offset ở +2 °C, nên +6 là thừa gấp ba. Trễ 1,5 °C để không
     nhấp nháy quanh đúng ngưỡng — cùng bài học với AL_T_CRIT_CLEAR. --- */
  if (delta_ >= DH_MAX_DELTA_C) {
    setPin(false); st_ = DH_HOLDING; reason_ = "da du chenh lech, dang giu";
  } else if (delta_ < DH_RESUME_DELTA_C || st_ == DH_HEATING) {
    setPin(true);  st_ = DH_HEATING; reason_ = "dang bom nhiet";
  }
}
