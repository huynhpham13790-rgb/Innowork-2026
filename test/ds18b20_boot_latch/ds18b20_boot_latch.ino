/* =============================================================================
 *  Đo tần suất cờ "ký sinh" bị BẬT NHẦM lúc khởi động.
 *
 *  Phát hiện dẫn tới sketch này: trong DallasTemperature 4.0.6,
 *  `parasite = false` CHỈ nằm trong setOneWire(). Còn begin() thì:
 *
 *      if (!parasite && readPowerSupply(deviceAddress)) parasite = true;
 *
 *  Tức là cờ chỉ có chiều BẬT, không có chiều tắt, và nó được quyết định bằng
 *  MỘT lần hỏi duy nhất cho mỗi con, ngay lúc quét bus lúc khởi động.
 *
 *  Hệ quả: chỉ cần đúng một lần tiếp xúc chập chờn ở đúng khoảnh khắc quét bus
 *  là cờ bật, và **giữ nguyên suốt cả phiên chạy**. Thư viện sẽ xử lý cả 8 con
 *  theo kiểu ký sinh cho tới lúc reset, dù dây nối hoàn toàn đúng. Đó là giả
 *  thuyết giải thích vì sao cùng một sketch, cùng phần cứng, mà lần chạy này
 *  0,00% lỗi còn lần sau 35% lỗi — không phải suy giảm dần, mà là một cái CHỐT.
 *
 *  Sketch này khởi động thật nhanh rồi in đúng một dòng tóm tắt, để máy chủ
 *  reset board liên tục và đếm tỉ lệ bật nhầm.
 *
 *  Đọc kết quả:
 *    - PARA=0 ở mọi lần boot  -> cờ không bị bật nhầm, phải tìm nguyên nhân khác
 *    - PARA=1 ở vài lần boot  -> đúng là tiếp xúc chập chờn + cờ bị chốt
 *    - POLL>0 khi PARA=1      -> lúc đó thật sự có con báo ký sinh
 *    - POLL=0 khi PARA=1      -> báo nhầm thoáng qua, hỏi lại thì bình thường
 * ========================================================================== */
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 4

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000);
  delay(200);

  sensors.begin();
  uint8_t n = sensors.getDeviceCount();
  bool para = sensors.isParasitePowerMode();   // cờ đã bị chốt lúc begin()

  // Hỏi lại từng con NGAY SAU ĐÓ. Nếu cờ bật mà hỏi lại ra 0 thì lần hỏi lúc
  // begin() là báo nhầm thoáng qua, không phải dây nối sai.
  uint8_t poll = 0;
  DeviceAddress a;
  for (uint8_t i = 0; i < n && i < 8; i++)
    if (sensors.getAddress(a, i) && sensors.readPowerSupply(a)) poll++;

  // Đọc thử 3 vòng không chặn để xem có lỗi ngay không
  sensors.setWaitForConversion(false);
  uint8_t bad = 0, tot = 0;
  for (uint8_t c = 0; c < 3; c++) {
    sensors.requestTemperatures();
    delay(800);
    for (uint8_t i = 0; i < n && i < 8; i++) {
      if (!sensors.getAddress(a, i)) continue;
      float t = sensors.getTempC(a);
      tot++;
      if (t == DEVICE_DISCONNECTED_C || t == 85.0f || t < -50 || t > 125) bad++;
    }
  }

  Serial.printf("BOOT N=%u PARA=%d POLL=%u BAD=%u/%u\n", n, para ? 1 : 0, poll, bad, tot);
}

void loop() {}
