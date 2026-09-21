/* =============================================================================
 *  Bring-up phần cứng đợt 3 — Đội Hủ Tiếu, AIoT InnoWorks 2026
 *
 *  MỤC ĐÍCH: nghiệm thu từng mảnh phần cứng mới lắp (INA226, 2 module D4184,
 *  điện trở sưởi 20 Ω, còi SFM-27) TRƯỚC KHI dán bất cứ thứ gì lên cell thật.
 *  Đây là sketch chẩn đoán, KHÔNG phải firmware sản phẩm — nó không nối WiFi,
 *  không gửi MQTT, để khi có lỗi thì chỉ có một nghi phạm.
 *
 *  PHẦN CỨNG (đợt này):
 *    DS18B20 ×8   GPIO4   1-Wire, pull-up 4,7 kΩ, KHÔNG parasitic
 *    INA226       GPIO8 SDA / GPIO9 SCL, addr 0x40, shunt onboard R100 = 100 mΩ
 *                 => toàn thang ±81,92 mV / 0,1 Ω = ±0,819 A
 *    D4184 #1     GPIO10  đóng/cắt điện trở sưởi 20 Ω, nguồn 12 V
 *    D4184 #2     GPIO5   đóng/cắt còi SFM-27
 *
 *  HAI QUY TẮC AN TOÀN, cố ý viết ngay đầu setup():
 *    1. pinMode + digitalWrite(LOW) cho GPIO10 và GPIO5 là lệnh ĐẦU TIÊN,
 *       trước cả Serial.begin(). Lý do: chân ESP32 lúc reset ở trạng thái thả
 *       nổi; nếu Serial.begin() hoặc USB CDC kẹt vài trăm ms thì trong khoảng
 *       đó gate MOSFET có thể bị kéo lên và điện trở sưởi bắt đầu nóng mà
 *       chương trình chưa chạy tới dòng nào điều khiển nó.
 *    2. delay(5000) cuối setup() — 5 giây để người kịp rút điện nếu ngửi thấy
 *       khét, thấy khói, hoặc thấy còi kêu ngay lúc khởi động.
 *
 *  VÌ SAO GỘP T1..T9 VÀO MỘT SKETCH CÓ MENU, không tách 9 file:
 *  mỗi lần nạp lại là một lần cắm rút, và mỗi lần cắm rút là một cơ hội tuột
 *  dây — đúng chế độ hỏng mà T5/T6 đang đi tìm. Gõ một ký tự vào Serial
 *  Monitor rẻ hơn nạp lại 9 lần.
 *
 *  DÙNG: mở Serial Monitor 115200, gõ số 1..9 rồi Enter. Gõ 0 = dừng mọi
 *  thứ và tắt cả sưởi lẫn còi.
 *
 *  Thư viện: OneWire, DallasTemperature (đã có sẵn trong Arduino/libraries).
 *  INA226 KHÔNG dùng thư viện ngoài — nói chuyện thẳng với thanh ghi qua Wire,
 *  cùng tinh thần với pack_meter.cpp: thư viện che mất chỗ hỏng, mà chỗ hỏng
 *  là thứ duy nhất sketch này đi tìm.
 * ========================================================================== */

#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "ina_reading.h"

// ------------------------------------------------------------------ Chân cắm
#define PIN_ONEWIRE   4
#define PIN_SDA       8
#define PIN_SCL       9
#define PIN_HEATER   10      // D4184 #1 -> điện trở sưởi 20 Ω / 12 V
#define PIN_BUZZER    5      // D4184 #2 -> còi SFM-27

// ------------------------------------------------------------------ Ngưỡng an toàn
// 60,0 °C giống AL_T_CRIT trong alarm.h — cùng một con số, cùng một lý do.
// Hạ tạm xuống 35,0 để test T7 bằng thân nhiệt, XONG PHẢI TRẢ VỀ 60,0.
static float  SAFE_T_CUT   = 60.0f;
#define SAFE_STALE_MS  5000   // quá 5 s không có số đọc mới => coi như quá nhiệt

// ------------------------------------------------------------------ INA226
#define INA_ADDR      0x40
#define INA_REG_CONF  0x00
#define INA_REG_SHUNT 0x01
#define INA_REG_BUS   0x02
#define INA_REG_PWR   0x03
#define INA_REG_CUR   0x04
#define INA_REG_CAL   0x05
#define INA_REG_MFR   0xFE    // phải đọc ra 0x5449 = "TI"
#define INA_REG_DIE   0xFF    // phải đọc ra 0x2260

#define INA_R_SHUNT   0.1f    // ohm — shunt R100 hàn sẵn trên module
#define INA_SHUNT_LSB 2.5e-6f // V/bit, cố định theo datasheet
#define INA_BUS_LSB   1.25e-3f// V/bit, cố định theo datasheet

// CURRENT_LSB chọn 1,0 A / 32768 = 30,5175 µA để khớp với cách gọi
// setMaxCurrentShunt(1.0, 0.1) trong phiếu test. Lưu ý: 1,0 A chỉ là hệ số
// thang đo, phần cứng vẫn chỉ đo được tới 0,819 A (giới hạn ±81,92 mV của
// chân shunt). Vượt ngưỡng đó là thanh ghi bão hoà chứ không phải số đo thật.
static const float INA_CUR_LSB = 1.0f / 32768.0f;

static bool ina_present = false;

// ------------------------------------------------------------------ DS18B20
#define DS_MAX 12            // dò được bao nhiêu thì báo bấy nhiêu; KHÔNG ghim 8
OneWire oneWire(PIN_ONEWIRE);
DallasTemperature ds(&oneWire);
static DeviceAddress ds_rom[DS_MAX];
static int  ds_n = 0;
static float ds_t[DS_MAX];

// ------------------------------------------------------------------ Trạng thái
static bool     heater_on   = false;
static bool     latched     = false;   // T7: đã cắt vì quá nhiệt, chỉ reset mới nhả
static uint32_t t_last_read = 0;       // lần cuối đọc được nhiệt độ hợp lệ
static double   energy_j    = 0;       // T8: tích phân P·dt

/* ===========================================================================
 *  Lớp truy cập thấp
 * ======================================================================== */
static void heaterSet(bool on) {
  // latched = đã từng vượt ngưỡng. Sau đó KHÔNG có lệnh nào bật lại được sưởi
  // cho tới khi nhấn reset. Chốt cứng ở đây, không ở chỗ gọi, để không thể
  // quên ở một nhánh nào đó.
  if (latched) on = false;
  heater_on = on;
  digitalWrite(PIN_HEATER, on ? HIGH : LOW);
}

static void buzzerSet(bool on) {
  // SFM-27 là còi CHỦ ĐỘNG: cấp điện là kêu, tần số cố định trong thân còi.
  // Dùng tone() ở đây là vô nghĩa — không đổi được cao độ, mà lại chiếm một
  // bộ định thời phần cứng.
  digitalWrite(PIN_BUZZER, on ? HIGH : LOW);
}

static bool inaWrite16(uint8_t reg, uint16_t val) {
  Wire.beginTransmission(INA_ADDR);
  Wire.write(reg);
  Wire.write((uint8_t)(val >> 8));
  Wire.write((uint8_t)(val & 0xFF));
  return Wire.endTransmission() == 0;
}

static bool inaRead16(uint8_t reg, uint16_t& out) {
  Wire.beginTransmission(INA_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)INA_ADDR, (uint8_t)2) != 2) return false;
  out = ((uint16_t)Wire.read() << 8) | Wire.read();
  return true;
}

static InaReading inaRead() {
  InaReading r = {false, 0, 0, 0, 0, 0};
  uint16_t rs, rb, rc, rp;
  if (!inaRead16(INA_REG_SHUNT, rs)) return r;
  if (!inaRead16(INA_REG_BUS,   rb)) return r;
  if (!inaRead16(INA_REG_CUR,   rc)) return r;
  if (!inaRead16(INA_REG_PWR,   rp)) return r;
  int16_t s = (int16_t)rs;             // shunt có dấu: đảo IN+/IN- ra số âm
  int16_t c = (int16_t)rc;
  r.shunt_mv       = s * INA_SHUNT_LSB * 1000.0f;
  r.bus_v          = rb * INA_BUS_LSB;
  r.current_ma     = (s * INA_SHUNT_LSB / INA_R_SHUNT) * 1000.0f;
  r.current_reg_ma = c * INA_CUR_LSB * 1000.0f;
  r.power_w        = rp * (25.0f * INA_CUR_LSB);   // POWER_LSB = 25 × CURRENT_LSB
  r.valid = true;
  return r;
}

static bool inaBegin() {
  uint16_t mfr = 0, die = 0;
  if (!inaRead16(INA_REG_MFR, mfr) || !inaRead16(INA_REG_DIE, die)) return false;
  // Một địa chỉ I2C có ACK KHÔNG chứng minh đó là INA226 — rất nhiều chip khác
  // cũng nằm ở 0x40. Kiểm hai mã định danh, giống cách pack_meter.cpp kiểm
  // DEVICE_ID của INA228.
  if (mfr != 0x5449 || die != 0x2260) {
    Serial.printf("  ✗ Chip ở 0x40 KHÔNG phải INA226: MFR=0x%04X DIE=0x%04X\n", mfr, die);
    return false;
  }
  // AVG=16 mẫu, VBUSCT=VSHCT=1,1 ms, chế độ liên tục shunt+bus.
  // Một lượt = 2 × 1,1 ms × 16 ≈ 35 ms. Đây chính là lý do KHÔNG được PWM
  // nhanh cái điện trở sưởi: băm 1 kHz thì mỗi lượt đo trung bình hoá trên
  // ~35 chu kỳ băm, ra một con số không ứng với trạng thái nào cả.
  if (!inaWrite16(INA_REG_CONF, 0x4527)) return false;
  // CAL = 0,00512 / (CURRENT_LSB × R_SHUNT)
  uint16_t cal = (uint16_t)(0.00512f / (INA_CUR_LSB * INA_R_SHUNT) + 0.5f);
  if (!inaWrite16(INA_REG_CAL, cal)) return false;
  Serial.printf("  ✓ INA226 MFR=0x%04X DIE=0x%04X CAL=%u (CURRENT_LSB=%.4f mA)\n",
                mfr, die, cal, INA_CUR_LSB * 1000.0f);
  return true;
}

/* ---------------------------------------------------------------------------
 *  Đọc toàn bộ DS18B20 một lượt (CÓ chặn ~750 ms).
 *  Sketch này được phép chặn — nó không giữ kết nối MQTT nào. Firmware thật
 *  thì không, và đó là lý do cell_temp.cpp tồn tại.
 *  Trả về số kênh đọc HỎNG (CRC sai, -127 mất kết nối, hoặc 85,0 = giá trị
 *  sau reset nghĩa là lệnh convert không tới nơi).
 * ------------------------------------------------------------------------ */
static int dsReadAll() {
  ds.requestTemperatures();
  int bad = 0;
  for (int i = 0; i < ds_n; i++) {
    float t = ds.getTempC(ds_rom[i]);
    ds_t[i] = t;
    if (t == DEVICE_DISCONNECTED_C || t == 85.0f || t < -40.0f || t > 125.0f) bad++;
  }
  if (bad < ds_n) t_last_read = millis();
  return bad;
}

static float dsMax() {
  float m = NAN;
  for (int i = 0; i < ds_n; i++) {
    float t = ds_t[i];
    if (t == DEVICE_DISCONNECTED_C || t == 85.0f || t < -40.0f || t > 125.0f) continue;
    if (isnan(m) || t > m) m = t;
  }
  return m;
}

/* ---------------------------------------------------------------------------
 *  Ngưỡng cắt an toàn — T7.
 *  Gọi sau MỌI lượt đọc nhiệt độ, kể cả trong các test khác. Cắt sưởi ở dòng
 *  đầu tiên, in log sau. Mạng, màn hình, MQTT đều có thể đợi; cell thì không.
 *  MẤT CẢM BIẾN CŨNG LÀ QUÁ NHIỆT: không đọc được nghĩa là không biết, mà
 *  không biết thì phải giả định điều xấu nhất — nếu không, cách dễ nhất để
 *  qua mặt hệ an toàn này là làm hỏng một cảm biến.
 * ------------------------------------------------------------------------ */
static bool safetyCheck(bool verbose = true) {
  float tmax   = dsMax();
  bool  stale  = (millis() - t_last_read) > SAFE_STALE_MS;
  bool  trip   = false;
  const char* why = "";

  if (isnan(tmax))              { trip = true; why = "KHÔNG kênh nào đọc được"; }
  else if (tmax >= SAFE_T_CUT)  { trip = true; why = "vượt ngưỡng nhiệt"; }
  else if (stale)               { trip = true; why = "quá 5 s không có số đọc mới"; }

  if (trip && !latched) {
    digitalWrite(PIN_HEATER, LOW);   // cắt TRƯỚC, không qua heaterSet()
    heater_on = false;
    latched   = true;
    buzzerSet(true);
    if (verbose) {
      Serial.printf("\n*** CẮT AN TOÀN *** %s (t_max=%.2f °C, ngưỡng %.1f)\n",
                    why, tmax, SAFE_T_CUT);
      Serial.println("    Sưởi đã cắt và CHỐT. Còi kêu liên tục.");
      Serial.println("    Nhấn nút RESET trên board để nhả chốt.");
    }
  }
  return trip;
}

/* ===========================================================================
 *  T1 — Quét I2C
 * ======================================================================== */
static void test1() {
  Serial.println("\n===== T1 — Quét bus I2C (SDA=8, SCL=9) =====");
  int n = 0;
  for (uint8_t a = 0x08; a < 0x78; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  thấy thiết bị ở 0x%02X\n", a);
      n++;
    }
  }
  if (n == 0) {
    Serial.println("  ✗ KHÔNG thấy thiết bị nào.");
    Serial.println("    => sai dây SDA/SCL, thiếu VCC, hoặc thiếu GND chung.");
    Serial.println("    DỪNG, đừng chạy T2/T3 — mọi số đọc sẽ là số rác.");
    return;
  }
  ina_present = inaBegin();
  Serial.printf("  KẾT LUẬN: %d thiết bị, INA226 ở 0x40 = %s\n",
                n, ina_present ? "CÓ" : "KHÔNG");
}

/* ===========================================================================
 *  TD — Quét 3 trạng thái gate  (gõ 'd')
 *
 *  VÌ SAO CẦN RIÊNG MỘT TEST: T2 đo lúc gate LOW, T3 đo lúc gate HIGH, nhưng
 *  hai test chạy cách nhau nên nếu nguồn chập chờn thì hai số không so được
 *  với nhau. Ở đây ba trạng thái đo liên tiếp trong một lần chạy, cùng một
 *  nguồn, cùng một bộ lọc — chênh lệch giữa chúng mới là bằng chứng.
 *
 *  Trạng thái thứ ba (thả nổi, pinMode INPUT) là để soi cái điện trở kéo
 *  xuống trên module D4184. Ba kết quả, ba kết luận khác hẳn nhau:
 *
 *    LOW=0, HIGH=cao                 -> module chạy đúng, logic thuận.
 *    LOW=cao, HIGH=0                 -> trigger đấu ĐẢO: chân tín hiệu đang
 *                                       nằm ở phía ÂM của cặp trigger, 3V3 ở
 *                                       phía dương. Sửa bằng dây, không phải
 *                                       bằng code.
 *    cả ba đều cao                   -> dòng KHÔNG đi qua MOSFET (tải đấu vào
 *                                       cặp cực VÀO), hoặc MOSFET chập D-S.
 *    thả nổi = cao, LOW = 0          -> không có điện trở kéo xuống; chân
 *                                       ESP32 lúc reset thả nổi => sưởi bật
 *                                       vài trăm ms mỗi lần khởi động.
 * ======================================================================== */
static void avgCurrent(const char* label, float& i_ma, float& bus_v) {
  // Bỏ 5 mẫu đầu: INA226 đang trung bình hoá 16 mẫu, ngay sau khi đổi trạng
  // thái thì số đọc còn trộn cả trạng thái cũ lẫn mới.
  for (int i = 0; i < 5; i++) { inaRead(); delay(50); }
  double si = 0, sv = 0;
  int n = 0;
  for (int i = 0; i < 20; i++) {
    InaReading r = inaRead();
    if (r.valid) { si += r.current_ma; sv += r.bus_v; n++; }
    delay(50);
  }
  i_ma  = n ? si / n : NAN;
  bus_v = n ? sv / n : NAN;
  Serial.printf("  %-22s I=%7.1f mA   bus=%6.3f V   (%d mẫu)\n", label, i_ma, bus_v, n);
}

static void testGate() {
  Serial.println("\n===== TD — Quét 3 trạng thái gate GPIO10 =====");
  if (!ina_present) { Serial.println("  ✗ chưa nhận INA226, chạy T1 trước."); return; }
  if (latched)      { Serial.println("  ✗ đang chốt an toàn, nhấn RESET trước."); return; }

  InaReading r0 = inaRead();
  if (!r0.valid)      { Serial.println("  ✗ lỗi I2C."); return; }
  if (r0.bus_v < 6.0f) {
    Serial.printf("  ✗ bus = %.2f V — CHƯA CẮM 12 V. Cắm adapter rồi chạy lại.\n", r0.bus_v);
    return;
  }

  float i_low, i_high, i_float, v;
  pinMode(PIN_HEATER, OUTPUT); digitalWrite(PIN_HEATER, LOW);
  Serial.printf("  (digitalRead lúc LOW = %d)\n", digitalRead(PIN_HEATER));
  avgCurrent("gate LOW (phải tắt)", i_low, v);

  digitalWrite(PIN_HEATER, HIGH);
  avgCurrent("gate HIGH (phải bật)", i_high, v);

  digitalWrite(PIN_HEATER, LOW);
  pinMode(PIN_HEATER, INPUT);           // thả nổi — soi điện trở kéo xuống
  avgCurrent("gate THẢ NỔI", i_float, v);

  /* Đọc ngược mức trên chân GPIO10 — phép đo dứt điểm cho câu hỏi "chân này
     có đang bị cái gì bên ngoài giữ không".
     Phải đọc ở chế độ INPUT: khi pinMode(OUTPUT), arduino-esp32 tắt luôn bộ
     đệm vào, nên digitalRead lúc đó trả về giá trị KHÔNG tin được — đây là
     chỗ rất dễ kết luận sai.
     Ba lần đọc với ba kiểu kéo nội:
       thả nổi -> mức do mạch ngoài quyết định
       kéo xuống nội (~45 kΩ) mà vẫn đọc 1 => có nguồn ngoài khoẻ đang ghim
          chân này lên cao. ESP32 kéo xuống không thắng nổi.
       kéo lên nội mà đọc 0 => có mạch ngoài ghim xuống đất. */
  int lv_float = digitalRead(PIN_HEATER);
  pinMode(PIN_HEATER, INPUT_PULLDOWN); delay(5);
  int lv_pd = digitalRead(PIN_HEATER);
  pinMode(PIN_HEATER, INPUT_PULLUP);   delay(5);
  int lv_pu = digitalRead(PIN_HEATER);
  Serial.printf("  mức trên GPIO10: thả nổi=%d  kéo-xuống-nội=%d  kéo-lên-nội=%d\n",
                lv_float, lv_pd, lv_pu);

  pinMode(PIN_HEATER, OUTPUT); digitalWrite(PIN_HEATER, LOW);   // về an toàn

  Serial.println("  --- kết luận ---");
  const float ON = 50.0f;   // mA, ngưỡng coi là "đang dẫn"
  bool on_low = fabs(i_low) > ON, on_high = fabs(i_high) > ON, on_flt = fabs(i_float) > ON;
  if (!on_low && on_high)
    Serial.println("  ✓ Module đóng cắt ĐÚNG, logic thuận. T2/T3 chạy được.");
  else if (on_low && !on_high)
    Serial.println("  ⚠ ĐẢO LOGIC: chân GPIO10 đang ở phía ÂM của cặp trigger.\n"
                   "     Đổi lại dây: GPIO10 -> chân tín hiệu (+), GND ESP32 -> chân âm.\n"
                   "     KHÔNG sửa bằng cách đảo trong code — lúc reset chân thả nổi\n"
                   "     sẽ thành BẬT sưởi, đúng thứ ta đang cố tránh.");
  else if (on_low && on_high)
    Serial.println("  ✗ Dẫn ở MỌI trạng thái => dòng không đi qua MOSFET (tải đấu vào\n"
                   "     cặp cực VÀO thay vì cực RA), hoặc MOSFET chập D-S.");
  else
    Serial.println("  ✗ KHÔNG dẫn ở trạng thái nào => hở mạch tải, hoặc gate không\n"
                   "     bao giờ đủ áp (thiếu GND chung giữa ESP32 và nguồn 12 V).");
  if (lv_pd == 1)
    Serial.println("  ✗✗ GPIO10 vẫn đọc MỨC CAO khi đã bật điện trở kéo xuống trong chip.\n"
                   "     => có nguồn 3V3 bên ngoài đang GHIM CỨNG chân này. Gate không\n"
                   "        bao giờ xuống được, nên MOSFET dẫn vĩnh viễn.\n"
                   "     => và mỗi lần firmware ghi LOW là ESP32 đang đấu ngắn mạch\n"
                   "        chân ra của nó xuống đất — RÚT dây 3V3 khỏi cọc trigger.");
  else if (lv_pd == 0 && lv_pu == 1)
    Serial.println("  ✓ GPIO10 tự do, không bị mạch ngoài ghim. Lỗi nằm ở đường công suất.");
  if (on_flt && !on_low)
    Serial.println("  ⚠ Thả nổi = ĐANG DẪN: module thiếu điện trở kéo xuống. Chân ESP32\n"
                   "     thả nổi lúc reset => mỗi lần khởi động sưởi bật vài trăm ms.\n"
                   "     Cần thêm điện trở 10 kΩ từ GPIO10 xuống GND.");
}

/* ===========================================================================
 *  T2 — Đọc INA226 lúc nghỉ (cần đã cắm 12 V, GPIO10 = LOW)
 * ======================================================================== */
static void test2() {
  Serial.println("\n===== T2 — INA226 lúc nghỉ (sưởi TẮT) =====");
  if (!ina_present) { Serial.println("  ✗ chưa nhận INA226, chạy T1 trước."); return; }
  heaterSet(false);
  delay(300);   // chờ bộ lọc trung bình 16 mẫu nạp đầy sau khi trạng thái đổi
  for (int i = 0; i < 5; i++) {
    InaReading r = inaRead();
    if (!r.valid) { Serial.println("  ✗ lỗi I2C khi đọc"); return; }
    Serial.printf("  bus=%.3f V  shunt=%.3f mV  I=%.1f mA (reg %.1f mA)  P=%.3f W\n",
                  r.bus_v, r.shunt_mv, r.current_ma, r.current_reg_ma, r.power_w);
    delay(200);
  }
  InaReading r = inaRead();
  Serial.println("  --- chẩn đoán ---");
  if (r.bus_v < 1.0f)
    Serial.println("  ✗ bus ≈ 0 V => chân VBS chưa nối vào phía 12 V, hoặc chưa cắm adapter.");
  else if (r.bus_v < 10.5f || r.bus_v > 13.5f)
    Serial.printf("  ⚠ bus = %.2f V, không phải ~12 V => kiểm nguồn.\n", r.bus_v);
  else
    Serial.println("  ✓ bus ≈ 12 V, đúng kỳ vọng.");
  if (fabs(r.current_ma) > 5.0f)
    Serial.printf("  ✗ dòng lúc nghỉ = %.1f mA (kỳ vọng ~0) => MOSFET rò hoặc đang dẫn.\n",
                  r.current_ma);
  else
    Serial.println("  ✓ dòng lúc nghỉ ≈ 0 mA.");
}

/* ===========================================================================
 *  T3 — Burst sưởi 3 s ON / 7 s OFF, đo dòng 10 lần/giây
 *  Kỳ vọng ON: ~60 mV / ~597 mA / ~11,94 V / ~7,1 W.  OFF: ~0.
 * ======================================================================== */
static void test3(int cycles = 3) {
  Serial.println("\n===== T3 — Burst sưởi + đo dòng =====");
  if (!ina_present) { Serial.println("  ✗ chưa nhận INA226, chạy T1 trước."); return; }
  if (latched)      { Serial.println("  ✗ đang chốt an toàn, nhấn RESET trước."); return; }
  Serial.println("  ON 3 s / OFF 7 s. Ctrl: gõ 0 để dừng khẩn.");
  // Nạp lại mốc thời gian đọc nhiệt độ TRƯỚC khi vào vòng có sưởi.
  // Đồng hồ "quá 5 s không có số mới" vẫn chạy trong lúc board ngồi ở menu,
  // mà lúc đó không ai đọc cảm biến — không nạp lại thì test nào khởi động
  // sau 5 giây ngồi không cũng bị cắt oan ngay dòng đầu.
  dsReadAll();
  Serial.println("  ms\tstate\tshunt_mV\tI_mA\tbus_V\tP_W");

  for (int c = 0; c < cycles; c++) {
    for (int phase = 0; phase < 2; phase++) {
      bool on = (phase == 0);
      heaterSet(on);
      uint32_t t0 = millis();
      uint32_t dur = on ? 3000 : 7000;
      float pmax = 0, imax = 0;
      double e0 = energy_j;
      uint32_t t_prev = millis();
      while (millis() - t0 < dur) {
        if (Serial.available() && Serial.read() == '0') { heaterSet(false); Serial.println("  DỪNG KHẨN"); return; }
        InaReading r = inaRead();
        uint32_t now = millis();
        if (r.valid) {
          energy_j += r.power_w * (now - t_prev) / 1000.0;   // T8: tích phân P·dt
          if (r.power_w > pmax) pmax = r.power_w;
          if (fabs(r.current_ma) > fabs(imax)) imax = r.current_ma;
          Serial.printf("  %lu\t%s\t%.3f\t%.1f\t%.3f\t%.3f\n",
                        (unsigned long)(now - t0), on ? "ON " : "OFF",
                        r.shunt_mv, r.current_ma, r.bus_v, r.power_w);
        }
        t_prev = now;
        delay(100);   // 10 Hz. KHÔNG nhanh hơn: một lượt đo INA226 mất ~35 ms.
      }
      if (on) {
        Serial.printf("  >> chu kỳ %d: I_đỉnh=%.1f mA, P_đỉnh=%.2f W, E=%.2f J\n",
                      c + 1, imax, pmax, energy_j - e0);
        Serial.println("  --- chẩn đoán ---");
        float a = fabs(imax);
        if (a < 20.0f)
          Serial.println("  ✗ ~0 mA lúc ON => mối hàn nguội, dây lỏng, hoặc gate không lên.");
        else if (imax < 0)
          Serial.println("  ✗ dòng ÂM => đảo IN+ / IN− trên module INA226.");
        else if (a > 900.0f)
          Serial.println("  ✗✗ ≥0,9 A: hai điện trở đang mắc SONG SONG chứ không nối tiếp — RÚT ĐIỆN NGAY.");
        else if (a > 500.0f && a < 700.0f)
          Serial.println("  ✓ ~600 mA, đúng kỳ vọng cho 20 Ω nối tiếp ở 12 V.");
        else
          Serial.printf("  ⚠ %.0f mA — ngoài khoảng trông đợi 500–700 mA, kiểm lại trị số điện trở.\n", a);
      }
    }
  }
  heaterSet(false);
  Serial.printf("  Tổng năng lượng đã bơm: %.2f J\n", energy_j);
}

/* ===========================================================================
 *  T4 — Còi
 * ======================================================================== */
static void test4() {
  Serial.println("\n===== T4 — Còi SFM-27 trên GPIO5 =====");
  Serial.println("  3 tiếng ngắn 100 ms...");
  for (int i = 0; i < 3; i++) { buzzerSet(true); delay(100); buzzerSet(false); delay(300); }
  delay(500);
  Serial.println("  kêu liền 2 s...");
  buzzerSet(true); delay(2000); buzzerSet(false);
  Serial.println("  xong. Nghe thấy đúng 3 tiếng ngắn rồi 1 tiếng dài = ĐẠT.");
  Serial.println("  Im hoàn toàn => kiểm 12 V của còi (qua D4184 #2) và GND chung.");
}

/* ===========================================================================
 *  T5 — Nhiễu còi lên bus 1-Wire  ⭐
 *  Chạy 2 lượt 60 s, đếm lỗi đọc. Còi là tải cảm ứng đóng cắt ngay cạnh một
 *  bus 1-Wire dài và trở kháng cao — đây đúng là kiểu hỏng "vẫn ra số, chỉ là
 *  số sai" mà cả dự án này được dựng lên để chống.
 * ======================================================================== */
static int test5_pass(bool buzz, int secs) {
  uint32_t t0 = millis();
  int rounds = 0, bad_total = 0;
  uint32_t t_buz = millis();
  bool bz = false;
  while (millis() - t0 < (uint32_t)secs * 1000) {
    if (buzz && millis() - t_buz > 500) {   // kêu ngắt quãng 0,5 s
      bz = !bz; buzzerSet(bz); t_buz = millis();
    }
    bad_total += dsReadAll();
    rounds++;
  }
  buzzerSet(false);
  Serial.printf("  %s: %d lượt × %d kênh = %d phép đọc, %d lỗi (%.2f %%)\n",
                buzz ? "CÓ còi " : "KHÔNG còi", rounds, ds_n, rounds * ds_n,
                bad_total, rounds * ds_n ? 100.0 * bad_total / (rounds * ds_n) : 0.0);
  return bad_total;
}

static void test5() {
  Serial.println("\n===== T5 — Nhiễu còi lên bus 1-Wire =====");
  if (ds_n == 0) { Serial.println("  ✗ chưa thấy DS18B20 nào, chạy T6 trước."); return; }
  Serial.println("  Lượt 1 — còi IM (60 s)");
  int a = test5_pass(false, 60);
  Serial.println("  Lượt 2 — còi KÊU NGẮT QUÃNG (60 s)");
  int b = test5_pass(true, 60);
  Serial.println("  --- chẩn đoán ---");
  if (b <= a + 2)
    Serial.println("  ✓ Còi không làm hỏng bus 1-Wire. Không cần chống nhiễu thêm.");
  else
    Serial.printf("  ✗ Lỗi tăng %d -> %d. Cần: tụ 100 µF + 100 nF ngay rail 3V3 của\n"
                  "     cụm cảm biến, HOẶC tách thời gian — không cho còi kêu trong\n"
                  "     lúc DS18B20 đang chuyển đổi (750 ms).\n", a, b);
}

/* ===========================================================================
 *  T6 — In ROM 64-bit của từng DS18B20
 *  Thứ tự dò bus KHÔNG cố định giữa các lần khởi động. Để index tự động thì
 *  cell #3 hôm nay có thể thành cell #5 ngày mai và không có lỗi nào báo —
 *  Lớp 1 vẫn chạy, vẫn ra số, chỉ là sai cell. Chép bảng dưới đây vào
 *  ds18b20_offsets.h theo ĐÚNG vị trí vật lý đã dán nhãn.
 * ======================================================================== */
static void test6() {
  Serial.println("\n===== T6 — Địa chỉ ROM của các DS18B20 =====");
  ds.begin();
  ds_n = 0;
  oneWire.reset_search();
  DeviceAddress a;
  while (ds_n < DS_MAX && oneWire.search(a)) {
    if (OneWire::crc8(a, 7) != a[7]) { Serial.println("  ⚠ bỏ qua một ROM sai CRC"); continue; }
    memcpy(ds_rom[ds_n], a, 8);
    ds_n++;
  }
  Serial.printf("  Tìm thấy %d cảm biến trên GPIO%d\n", ds_n, PIN_ONEWIRE);
  ds.setResolution(12);

  /* Đo thời gian một lượt chuyển đổi và đọc lại độ phân giải THẬT từ từng
     con, thay vì tin rằng setResolution(12) đã có tác dụng.
     Vì sao quan trọng đến thế: 12 bit = 0,0625 °C, 10 bit = 0,25 °C. Sai số
     chế tạo của cả dàn đo được là 0,326 °C, và Lớp 1 sống bằng chênh lệch
     tương đối cỡ đó — chạy ở 10 bit thì bước lượng tử nuốt mất tín hiệu, mà
     nhiệt độ in ra vẫn trông hoàn toàn bình thường. */
  uint32_t t_conv = millis();
  dsReadAll();
  t_conv = millis() - t_conv;
  Serial.printf("  Một lượt đọc %d kênh mất %lu ms (12 bit cần ≥750 ms)\n",
                ds_n, (unsigned long)t_conv);
  float t_fast[DS_MAX];
  memcpy(t_fast, ds_t, sizeof(t_fast));

  /* Đo lại bằng đường CHẬM để xem đường nhanh có trả về số cũ không.
     setCheckForConversion(false) bỏ kiểu hỏi-cảm-biến-xong-chưa và ép chờ đủ
     750 ms theo datasheet. Nếu hai đường cho cùng số thì đường nhanh đang
     đọc đúng lượt hiện tại; nếu lệch thì mọi số đọc đều trễ một nhịp — vô
     hại khi bench, nhưng là nhãn sai khi lấy dữ liệu huấn luyện. */
  ds.setCheckForConversion(false);
  uint32_t t_slow = millis();
  dsReadAll();
  t_slow = millis() - t_slow;
  // Trả về false — đó là mặc định của bench (đặt trong setup). Bản trước trả
  // về true ở đây và vô tình ghi đè mất thiết lập của setup, khiến mọi test
  // chạy sau T6 lại đi đường nhanh.
  ds.setCheckForConversion(false);
  float dmax = 0;
  for (int i = 0; i < ds_n; i++) dmax = max(dmax, fabsf(ds_t[i] - t_fast[i]));
  Serial.printf("  Đường chờ đủ 750 ms mất %lu ms; lệch lớn nhất so với đường nhanh: %.4f °C\n",
                (unsigned long)t_slow, dmax);
  for (int i = 0; i < ds_n; i++) {
    Serial.printf("  [%d] { ", i);
    for (int j = 0; j < 8; j++) Serial.printf("0x%02X%s", ds_rom[i][j], j < 7 ? ", " : "");
    Serial.printf(" },   // %.4f °C, %d bit\n", ds_t[i], ds.getResolution(ds_rom[i]));
  }
  if (ds_n == 0)
    Serial.println("  ✗ Không thấy con nào => kiểm pull-up 4,7 kΩ, VDD 3V3, GND chung.");
}

/* ===========================================================================
 *  T7 — Ngưỡng cắt an toàn  ⭐⭐  BẮT BUỘC trước khi dán điện trở lên cell
 *  Cách test: gõ 7, sketch hạ ngưỡng xuống 35 °C, bật sưởi, và chờ. Nắm tay
 *  vào một đầu dò cho lên thân nhiệt. Phải thấy cắt + còi + chốt.
 *  Xong test, ngưỡng TỰ TRẢ VỀ 60,0 °C ở cuối hàm — không để người phải nhớ.
 * ======================================================================== */
static void test7() {
  Serial.println("\n===== T7 — Ngưỡng cắt an toàn =====");
  if (ds_n == 0) { Serial.println("  ✗ chưa thấy DS18B20 nào, chạy T6 trước."); return; }
  /* 31,0 °C chứ không phải 35,0 như phiếu test ghi ban đầu.
     Đo thật: nắm tay vào đầu dò 2 phút chỉ đưa được lên 32,31 °C rồi nguội —
     vỏ kim loại đầu dò không bao giờ đạt được thân nhiệt lõi. Ngưỡng 35 °C là
     ngưỡng KHÔNG THỂ VỚI TỚI, nên test sẽ luôn "trượt" mà chẳng nói lên điều
     gì về hệ an toàn.
     31,0 nằm giữa: cao hơn con nóng nhất lúc nằm không (28,4 °C) đủ để không
     tự kích, thấp hơn 32,31 °C đủ để tay người với tới. */
  SAFE_T_CUT = 31.0f;
  Serial.println("  Ngưỡng tạm hạ xuống 31,0 °C (35,0 là mức tay người không với tới).");
  Serial.println("  NẮM TAY vào một đầu dò bất kỳ. Tối đa 120 s.");
  // Nạp lại mốc thời gian đọc nhiệt độ TRƯỚC khi vào vòng có sưởi.
  // Đồng hồ "quá 5 s không có số mới" vẫn chạy trong lúc board ngồi ở menu,
  // mà lúc đó không ai đọc cảm biến — không nạp lại thì test nào khởi động
  // sau 5 giây ngồi không cũng bị cắt oan ngay dòng đầu.
  dsReadAll();
  heaterSet(true);
  uint32_t t0 = millis();
  while (millis() - t0 < 120000 && !latched) {
    dsReadAll();
    safetyCheck();
    float m = dsMax();
    Serial.printf("  t_max=%.2f °C  sưởi=%s\n", m, heater_on ? "ON" : "off");
    if (Serial.available() && Serial.read() == '0') break;
  }
  heaterSet(false);
  if (latched) {
    Serial.println("  ✓ ĐẠT: đã cắt, đã chốt, còi kêu.");
    Serial.println("    Ngưỡng trả về 60,0 °C. Nhấn RESET để nhả chốt và tắt còi.");
  } else {
    Serial.println("  ✗ TRƯỢT: hết 120 s mà không cắt. KHÔNG được dán điện trở lên cell.");
  }
  SAFE_T_CUT = 60.0f;
}

/* ===========================================================================
 *  T8 — Tích phân năng lượng
 *  Đây là NHÃN GỐC để tự gán nhãn dataset: cửa sổ có dòng = "đã bơm nhiệt",
 *  không dòng = bình thường. Không có nó thì mọi mẻ dữ liệu sưởi đều phải gán
 *  nhãn bằng tay theo trí nhớ, và trí nhớ thì sai.
 * ======================================================================== */
static void test8() {
  Serial.println("\n===== T8 — Tích phân năng lượng theo cửa sổ ON =====");
  if (!ina_present) { Serial.println("  ✗ chưa nhận INA226, chạy T1 trước."); return; }
  if (latched)      { Serial.println("  ✗ đang chốt an toàn, nhấn RESET trước."); return; }
  Serial.println("  win\tbat_dau_ms\tket_thuc_ms\tJ\tP_tb_W");
  // Nạp lại mốc thời gian đọc nhiệt độ TRƯỚC khi vào vòng có sưởi.
  // Đồng hồ "quá 5 s không có số mới" vẫn chạy trong lúc board ngồi ở menu,
  // mà lúc đó không ai đọc cảm biến — không nạp lại thì test nào khởi động
  // sau 5 giây ngồi không cũng bị cắt oan ngay dòng đầu.
  dsReadAll();
  for (int w = 0; w < 3; w++) {
    uint32_t t_on = millis();
    double e0 = energy_j;
    heaterSet(true);
    /* Lấy mẫu công suất 20 ms/lần, ĐỘC LẬP với nhịp đọc DS18B20.
     * Bản đầu gọi dsReadAll() ngay trong vòng tích phân; hàm đó chặn ~750 ms
     * nên bước tích phân thành 750 ms, và vì dùng giá trị ĐẦU khoảng nên cả
     * khoảng đầu tiên (lúc sưởi vừa bật, INA226 còn đang trung bình hoá
     * trạng thái cũ) bị tính thành ~0 W. Kết quả thiếu ~12 %, và luôn thiếu
     * về một phía — kiểu sai số tệ nhất cho một nhãn dataset, vì nó không tự
     * triệt tiêu khi lấy trung bình nhiều mẻ.
     * Dùng hình thang thay vì bậc thang để không phụ thuộc vào việc lấy giá
     * trị đầu hay cuối khoảng. */
    float p_prev = 0;
    uint32_t t_prev = t_on, t_ds = t_on;
    while (millis() - t_on < 5000) {
      InaReading r = inaRead();
      uint32_t now = millis();
      if (r.valid) {
        energy_j += 0.5 * (p_prev + r.power_w) * (now - t_prev) / 1000.0;
        p_prev = r.power_w;
      }
      t_prev = now;
      // đọc nhiệt độ thưa hơn hẳn, chỉ để ngưỡng cắt an toàn còn số mới
      // KHÔNG đặt lại t_prev sau lượt đọc này: 750 ms đó sưởi vẫn đang bật,
      // bỏ đi là lại thiếu năng lượng. Hình thang bắc qua khoảng trống vẫn
      // đúng vì công suất trong pha ON gần như hằng số (đo được: 7,10 W ±0,01).
      if (now - t_ds > 1500) { dsReadAll(); t_ds = millis(); }
      if (safetyCheck()) { heaterSet(false); return; }
      delay(20);
    }
    heaterSet(false);
    uint32_t t_off = millis();
    double dj = energy_j - e0;
    Serial.printf("  %d\t%lu\t%lu\t%.2f\t%.3f\n", w + 1,
                  (unsigned long)t_on, (unsigned long)t_off, dj,
                  dj / ((t_off - t_on) / 1000.0));
    /* Nghỉ giữa hai cửa sổ để nhiệt lan ra. KHÔNG dùng delay() trần: 5 giây
       không đọc cảm biến là đúng bằng ngưỡng "mất cảm biến" của safetyCheck,
       nên nó sẽ cắt oan ngay đầu cửa sổ sau. Hệ an toàn phải được NUÔI liên
       tục, kể cả lúc đang không làm gì — đó cũng là lý do firmware thật không
       bao giờ được có delay() dài trong vòng lặp chính. */
    uint32_t t_rest = millis();
    while (millis() - t_rest < 5000) { dsReadAll(); safetyCheck(); }
  }
  Serial.printf("  Tổng cộng dồn từ lúc khởi động: %.2f J\n", energy_j);
}

/* ===========================================================================
 *  T9 — Chạy gộp 5 phút
 *  Câu hỏi cần trả lời: 1-Wire (chặn 750 ms) có làm hỏng giao dịch I2C không,
 *  có tràn bộ nhớ không, có bị watchdog reset không, thời gian mỗi vòng có ổn
 *  định không. Ba thứ này chỉ lộ ra khi chạy dài, không lộ ra ở test 10 giây.
 * ======================================================================== */
static void test9() {
  Serial.println("\n===== T9 — Chạy gộp 5 phút =====");
  if (ds_n == 0) { Serial.println("  ✗ chưa thấy DS18B20 nào, chạy T6 trước."); return; }
  uint32_t t0 = millis(), n = 0, loop_max = 0, i2c_err = 0, ds_err = 0;
  uint32_t heap0 = ESP.getFreeHeap();
  // Nạp lại mốc thời gian đọc nhiệt độ TRƯỚC khi vào vòng có sưởi.
  // Đồng hồ "quá 5 s không có số mới" vẫn chạy trong lúc board ngồi ở menu,
  // mà lúc đó không ai đọc cảm biến — không nạp lại thì test nào khởi động
  // sau 5 giây ngồi không cũng bị cắt oan ngay dòng đầu.
  dsReadAll();
  uint32_t t_heat = millis();
  bool on = false;
  while (millis() - t0 < 300000) {
    uint32_t ls = millis();
    ds_err += dsReadAll();
    if (ina_present) { InaReading r = inaRead(); if (!r.valid) i2c_err++; }
    if (safetyCheck()) break;
    // sưởi 10 s / nghỉ 20 s, và còi bíp 1 lần mỗi vòng sưởi — để ba thiết bị
    // thật sự hoạt động ĐỒNG THỜI, chứ không phải lần lượt từng cái
    if (millis() - t_heat > (on ? 10000u : 20000u)) {
      on = !on; heaterSet(on); t_heat = millis();
      if (on) { buzzerSet(true); delay(60); buzzerSet(false); }
    }
    uint32_t ld = millis() - ls;
    if (ld > loop_max) loop_max = ld;
    n++;
    if (n % 20 == 0)
      Serial.printf("  %lus  vòng=%lu  vòng_max=%lu ms  heap=%lu (Δ%+ld)  lỗi_ds=%lu lỗi_i2c=%lu\n",
                    (unsigned long)((millis() - t0) / 1000), (unsigned long)n,
                    (unsigned long)loop_max, (unsigned long)ESP.getFreeHeap(),
                    (long)ESP.getFreeHeap() - (long)heap0,
                    (unsigned long)ds_err, (unsigned long)i2c_err);
    if (Serial.available() && Serial.read() == '0') break;
  }
  heaterSet(false); buzzerSet(false);
  Serial.printf("  KẾT THÚC: %lu vòng, vòng chậm nhất %lu ms, heap %lu -> %lu,\n"
                "            lỗi DS18B20 %lu, lỗi I2C %lu\n",
                (unsigned long)n, (unsigned long)loop_max,
                (unsigned long)heap0, (unsigned long)ESP.getFreeHeap(),
                (unsigned long)ds_err, (unsigned long)i2c_err);
  Serial.println("  Đạt khi: không reset, heap không tụt dần, vòng_max < 1500 ms.");
}

/* ===========================================================================
 *  setup / loop
 * ======================================================================== */
void setup() {
  // ---- Quy tắc an toàn 1: hai chân công suất về LOW trước MỌI thứ khác ----
  pinMode(PIN_HEATER, OUTPUT); digitalWrite(PIN_HEATER, LOW);
  pinMode(PIN_BUZZER, OUTPUT); digitalWrite(PIN_BUZZER, LOW);

  Serial.begin(115200);
  uint32_t tw = millis();
  while (!Serial && millis() - tw < 3000) { }

  Wire.begin(PIN_SDA, PIN_SCL, 400000);
  ds.begin();
  ds.setResolution(12);
  ds.setWaitForConversion(true);
  /* Ép chờ đủ 750 ms theo datasheet thay vì hỏi cảm biến "xong chưa".
     Đo được: đường hỏi-xong-chưa trả về sau 90 ms — bất khả thi cho chuyển
     đổi 12 bit, nên số đọc có thể là của lượt trước. Ở bench thì 750 ms là
     rẻ, mà đổi lại không còn phải nghi ngờ số liệu.
     Firmware thật KHÔNG đi đường này: cell_temp.cpp tự đếm CT_CONV_MS=760 ms
     bằng máy trạng thái không chặn, vì nó còn phải gọi mqtt.loop() đều đặn. */
  ds.setCheckForConversion(false);
  t_last_read = millis();

  Serial.println("\n\n=== Bring-up phần cứng đợt 3 — Đội Hủ Tiếu ===");
  Serial.println("Sưởi GPIO10 và còi GPIO5 đang TẮT.");
  Serial.println("5 giây để rút điện nếu có gì bất thường...");
  delay(5000);   // ---- Quy tắc an toàn 2 ----

  test6();          // dò cảm biến trước, các test khác đều cần ds_n
  test1();          // rồi dò I2C — hai cái này không cần 12 V

  Serial.println("\nMENU: 1..9 = chạy test tương ứng · d = quét 3 trạng thái gate · 0 = tắt hết");
  Serial.println("Cần cắm 12 V: T2, T3, T8 (và phần sưởi của T7, T9).");
}

void loop() {
  if (!Serial.available()) { delay(20); return; }
  int c = Serial.read();
  switch (c) {
    case '1': test1(); break;
    case '2': test2(); break;
    case '3': test3(); break;
    case '4': test4(); break;
    case '5': test5(); break;
    case '6': test6(); break;
    case '7': test7(); break;
    case '8': test8(); break;
    case '9': test9(); break;
    case 'd': case 'D': testGate(); break;
    case '0':
      heaterSet(false); buzzerSet(false);
      Serial.println("  đã tắt sưởi và còi.");
      break;
    default: return;
  }
  Serial.println("\n> sẵn sàng (1..9, 0=tắt)");
}
