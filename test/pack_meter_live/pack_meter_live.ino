/* =============================================================================
 *  Kiểm PackMeter trên CHIP THẬT — khác hẳn pack_meter_bench (chạy khô, chỉ
 *  kiểm phép giải mã thanh ghi của INA228 bằng số tự tính tay).
 *
 *  Câu hỏi test này trả lời: đường TỰ NHẬN CHIP của QĐ-037 có nhận đúng con
 *  đang cắm không, và số đọc ra có khớp với số mà sketch bring-up đã đo bằng
 *  đường truy cập thô không. Hai đường viết độc lập, nên khớp nhau là bằng
 *  chứng thật chứ không phải chép lại hành vi của nhau.
 *
 *  Chân I2C giống bring-up: SDA=8, SCL=9.
 * ========================================================================== */
#include "pack_meter.h"

#define PIN_SDA 8
#define PIN_SCL 9

PackMeter meter;

void setup() {
  Serial.begin(115200);
  uint32_t t = millis();
  while (!Serial && millis() - t < 3000) { }
  Serial.println("\n===== PackMeter tren chip that =====");

  if (!meter.begin(PIN_SDA, PIN_SCL)) {
    Serial.println("✗ begin() TRUOT — khong nhan duoc chip nao.");
    return;
  }
  Serial.printf("✓ Nhan chip: %s, toan thang +-%.3f A\n",
                meter.chipName(), meter.currentFullScale());
}

void loop() {
  if (!meter.present()) { delay(2000); return; }
  PackMeasurement m = meter.read();
  /* In CẢ KHI valid=false, kèm lý do. Với pack 8S thì PM_V_MIN = 15,0 V, mà
     bàn test đang chạy nguồn 12 V — nên valid=false ở đây là ĐÚNG THIẾT KẾ,
     không phải lỗi. In số thô ra để phân biệt "chip không đọc được" với
     "đọc được nhưng ngoài khoảng hợp lý của pack 8S". */
  Serial.printf("valid=%d  V=%7.3f  cell_v=%6.3f  I=%8.1f mA  Q=%.5f Ah  "
                "die=%s  loi=%lu  bao_hoa=%lu\n",
                m.valid, m.voltage, m.cell_v, m.current * 1000.0f, m.charge_ah,
                isnan(m.die_temp) ? "NAN" : String(m.die_temp, 2).c_str(),
                (unsigned long)meter.errorCount(),
                (unsigned long)meter.saturationCount());
  delay(1000);
}
