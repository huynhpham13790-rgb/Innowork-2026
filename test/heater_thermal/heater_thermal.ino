/* =============================================================================
 *  Đặc tính nhiệt của điện trở sưởi + điều nhiệt vòng kín.
 *
 *  MỤC ĐÍCH: trả lời ba câu mà kịch bản demo và ngưỡng an toàn phụ thuộc vào,
 *  và cả ba đều PHẢI đo chứ không suy ra được:
 *    1. Bơm 7,1 W thì nhiệt lên nhanh cỡ nào? -> demo 90 giây có khả thi không
 *    2. Cắt rồi nhiệt còn VỌT thêm bao nhiêu? -> ngưỡng 60 °C có đủ an toàn
 *       khi dán lên pin thật không, hay phải hạ xuống
 *    3. Tắt thì nguội bao lâu?                -> giữa hai lần demo phải chờ bao lâu
 *
 *  Câu 2 là câu quan trọng nhất. Cắt ở 60 °C mà nhiệt còn vọt thêm 15 °C thì
 *  ngưỡng 60 KHÔNG đủ an toàn — và chỉ đo mới biết.
 *
 *  ⚠️ VÌ SAO PHÉP ĐO ĐỘ VỌT LẠI CẮT Ở 50 °C CHỨ KHÔNG PHẢI 60:
 *  DS18B20 chỉ chịu tới 125 °C, còn bề mặt điện trở sứ 10 W chạy 7,1 W có thể
 *  lên 150–200 °C. Đo độ vọt ở ngưỡng THẤP rồi suy ra, an toàn hơn nhiều so
 *  với đo ở đúng 60 rồi cầu mong nó không vọt quá xa. Ngưỡng cứng 60 °C vẫn
 *  chạy song song và vẫn chốt, y như trong firmware thật.
 *
 *  ⚠️ CẢM BIẾN TRÊN ĐIỆN TRỞ PHẢI LÀ P07 HOẶC P08 — hai con nằm NGOÀI cấu hình
 *  6S. Lấy một trong sáu sợi của pack đi quấn vào điện trở là Lớp 1 mất một
 *  cell và dữ liệu thành rác.
 *
 *  KHÔNG PWM NHANH: INA226 trung bình hoá 16 mẫu × 1,1 ms ≈ 35 ms, băm 1 kHz
 *  thì mỗi lượt đo trải trên ~35 chu kỳ băm, ra một con số không ứng với trạng
 *  thái nào. Đóng cắt tối thiểu 5 giây, xem DWELL_MS.
 *
 *  DÙNG: Serial 115200.
 *    a = đo đặc tính nhiệt (sưởi tới 50 °C, cắt, theo dõi vọt lố + nguội)
 *    b = điều nhiệt vòng kín giữ ở đích, 5 phút
 *    0 = tắt khẩn
 * ========================================================================== */
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define PIN_ONEWIRE   4
#define PIN_SDA       8
#define PIN_SCL       9
#define PIN_HEATER   10
#define PIN_BUZZER    5

/* Cảm biến quấn trên điện trở: P07. Đổi sang P08 nếu quấn con kia.
   TUYỆT ĐỐI không đặt thành một trong sáu ROM của pack. */
static const uint8_t ROM_HEATER[8] = { 0x28, 0x4D, 0x49, 0x04, 0x00, 0x00, 0x00, 0x35 };

#define T_HARD_CUT    60.0f   // ngưỡng cứng, giống AL_T_CRIT — chốt, không nhả
/* 40 °C, không phải 50. Đo thật (21/09): bơm 7,3 W trong 113 s chỉ đưa đầu dò
   quấn ngoài lên 46,94 °C — chưa tới 50 nên không lần nào cắt, trong khi THÂN
   điện trở đã nóng tới mức không sờ được. 40 °C tới trong ~75 s, đủ để đo độ
   vọt lố mà không phải nướng cái điện trở thêm hai phút. */
#define T_TRIP_TEST   40.0f

/* ⚠️ HẠN MỨC ĐỘC LẬP VỚI CẢM BIẾN — bài học đắt nhất của buổi 21/09.
 * Lần chạy đầu, hệ an toàn chỉ nhìn NHIỆT ĐỘ, mà nhiệt độ lại đo ở đầu dò quấn
 * ngoài chứ không phải chỗ nóng nhất. Đầu dò báo 47 °C trong khi thân điện trở
 * đã bỏng tay. Ngưỡng nhiệt không bao giờ chạm, sưởi cứ chạy, và thứ dừng thí
 * nghiệm lại là BÀN TAY NGƯỜI chứ không phải chương trình.
 *
 * Một hệ an toàn chỉ nhìn một đại lượng thì mù đúng theo cách đại lượng đó mù.
 * Hai hạn mức dưới đây KHÔNG đọc cảm biến nhiệt: chúng đếm thời gian và đếm
 * năng lượng đã bơm vào. Cảm biến tuột, dán sai chỗ, hay tiếp xúc kém thì
 * chúng vẫn cắt. 1200 J ở 7,3 W ≈ 165 giây. */
#define MAX_HEAT_MS   165000
#define MAX_HEAT_J    1200.0f
#define T_TARGET      45.0f   // đích của điều nhiệt vòng kín
#define T_HYST         2.0f   // trễ: tắt ở đích, bật lại khi tụt quá bấy nhiêu
#define DWELL_MS       5000   // tối thiểu giữ mỗi trạng thái, để INA226 đo được
#define STALE_MS       5000   // quá bấy nhiêu không đọc được -> coi như quá nhiệt

#define INA_ADDR 0x40
static const float INA_R_SHUNT = 0.1f, INA_SHUNT_LSB = 2.5e-6f, INA_BUS_LSB = 1.25e-3f;

OneWire oneWire(PIN_ONEWIRE);
DallasTemperature ds(&oneWire);

static bool     latched = false;
static bool     heater_on = false;
static uint32_t t_heat_on = 0;      // lúc bật sưởi lần gần nhất
static double   j_heat_start = 0;   // Joule tại lúc bật
static uint32_t t_last_ok = 0;
static double   energy_j = 0;
static bool     ina_ok = false;

// ------------------------------------------------------------------ phần cứng
static void heaterSet(bool on) {
  if (latched) on = false;          // chốt cứng tại đây, không ở chỗ gọi
  if (on && !heater_on) { t_heat_on = millis(); j_heat_start = energy_j; }
  heater_on = on;
  digitalWrite(PIN_HEATER, on ? HIGH : LOW);
}

static bool inaRead16(uint8_t reg, uint16_t& out) {
  Wire.beginTransmission(INA_ADDR); Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)INA_ADDR, (uint8_t)2) != 2) return false;
  out = ((uint16_t)Wire.read() << 8) | Wire.read();
  return true;
}
static bool inaWrite16(uint8_t reg, uint16_t v) {
  Wire.beginTransmission(INA_ADDR); Wire.write(reg);
  Wire.write(v >> 8); Wire.write(v & 0xFF);
  return Wire.endTransmission() == 0;
}
static void inaPower(float& i_ma, float& p_w) {
  uint16_t rs, rb;
  if (!ina_ok || !inaRead16(0x01, rs) || !inaRead16(0x02, rb)) { i_ma = p_w = NAN; return; }
  const float vsh = (int16_t)rs * INA_SHUNT_LSB;
  i_ma = vsh / INA_R_SHUNT * 1000.0f;
  p_w  = (rb * INA_BUS_LSB) * (vsh / INA_R_SHUNT);
}

// ------------------------------------------------------------------ nhiệt độ
static float t_heater = NAN, t_ref = NAN;

/* Đọc một lượt: nhiệt trên điện trở + nhiệt "nền" (trung vị các con còn lại).
   Lấy TRUNG VỊ các con khác làm nền thay vì một con cố định: đỡ phụ thuộc vào
   việc con nào đang nằm ở đâu, và một con hỏng không kéo lệch cả phép đo. */
static void readTemps() {
  ds.requestTemperatures();
  t_heater = ds.getTempC((uint8_t*)ROM_HEATER);
  if (!(t_heater > -40 && t_heater < 125 && t_heater != 85.0f)) t_heater = NAN;
  else t_last_ok = millis();

  float v[12]; int m = 0;
  for (int i = 0; i < ds.getDeviceCount() && m < 12; i++) {
    DeviceAddress a;
    if (!ds.getAddress(a, i)) continue;
    if (memcmp(a, ROM_HEATER, 8) == 0) continue;
    float t = ds.getTempC(a);
    if (t > -40 && t < 125 && t != 85.0f) v[m++] = t;
  }
  if (m == 0) { t_ref = NAN; return; }
  for (int i = 1; i < m; i++) { float x = v[i]; int j = i-1; while (j>=0 && v[j]>x) { v[j+1]=v[j]; j--; } v[j+1]=x; }
  t_ref = (m % 2) ? v[m/2] : 0.5f*(v[m/2-1]+v[m/2]);
}

/* Ngưỡng cứng, chạy song song và ĐỘC LẬP với mọi vòng điều khiển ở dưới.
   Mất cảm biến cũng cắt: không đọc được nghĩa là không biết, mà không biết thì
   phải giả định điều xấu nhất — nếu không, cách dễ nhất để qua mặt hệ an toàn
   này là làm hỏng một cảm biến. */
static bool hardCut() {
  const bool stale = (millis() - t_last_ok) > STALE_MS;
  const bool hot   = !isnan(t_heater) && t_heater >= T_HARD_CUT;
  /* Hai hạn mức KHÔNG đọc nhiệt độ — xem chú thích ở MAX_HEAT_MS. */
  const bool too_long = heater_on && (millis() - t_heat_on) > MAX_HEAT_MS;
  const bool too_much = heater_on && (energy_j - j_heat_start) > MAX_HEAT_J;
  if (too_long || too_much) {
    digitalWrite(PIN_HEATER, LOW); heater_on = false; latched = true;
    digitalWrite(PIN_BUZZER, HIGH);
    Serial.printf("\n*** CAT THEO HAN MUC *** %s — da bom %.0f J trong %.0f s,"
                  " cam bien moi bao %.2f °C\n",
                  too_long ? "qua thoi gian" : "qua nang luong",
                  energy_j - j_heat_start, (millis()-t_heat_on)/1000.0, t_heater);
    Serial.println("    Han muc nay KHONG doc cam bien nhiet. No cat ke ca khi dau do");
    Serial.println("    tuot, dan sai cho, hoac tiep xuc kem. Nhan RESET de nha.");
    return true;
  }
  if ((hot || stale || isnan(t_heater)) && !latched) {
    digitalWrite(PIN_HEATER, LOW);    // cắt TRƯỚC, in log SAU
    heater_on = false; latched = true;
    digitalWrite(PIN_BUZZER, HIGH);
    Serial.printf("\n*** CAT CUNG *** %s (T=%.2f, nguong %.1f)\n",
                  hot ? "vuot nguong" : "mat cam bien", t_heater, T_HARD_CUT);
    Serial.println("    Da chot. Nhan RESET de nha.");
  }
  return latched;
}

static void logLine(uint32_t t0, const char* st) {
  float i_ma, p_w; inaPower(i_ma, p_w);
  Serial.printf("%6lu\t%s\t%7.3f\t%7.3f\t%7.2f\t%8.1f\t%6.3f\t%8.2f",
                (unsigned long)((millis()-t0)/100)*100/1000, st,
                t_heater, t_ref, isnan(t_heater)||isnan(t_ref) ? NAN : t_heater-t_ref,
                i_ma, p_w, energy_j);
  /* In chênh lệch của TỪNG đầu dò so với nền. Cần cái này để biết đầu dò nào
     thật sự đang được hơ — nhìn một con số trung bình thì không phân biệt được
     "đã quấn đúng chỗ" với "quấn vào con điện trở không có điện". */
  for (int i = 0; i < ds.getDeviceCount(); i++) {
    DeviceAddress a;
    if (!ds.getAddress(a, i)) continue;
    float t = ds.getTempC(a);
    if (!(t > -40 && t < 125 && t != 85.0f)) { Serial.printf("\t%02X:xx", a[1]); continue; }
    Serial.printf("\t%02X:%+.2f", a[1], isnan(t_ref) ? NAN : t - t_ref);
  }
  Serial.println();
}

/* Vòng lấy mẫu chung: đọc nhiệt, cộng dồn năng lượng, kiểm an toàn.
   Trả về false nếu phải dừng. Tích phân theo HÌNH THANG với nhịp riêng, không
   bám vào nhịp 1-Wire (chặn ~850 ms) — cùng bài học với T8 của bring-up. */
static float p_prev = 0; static uint32_t t_prev = 0;
static bool tick() {
  float i_ma, p_w; inaPower(i_ma, p_w);
  const uint32_t now = millis();
  if (!isnan(p_w)) { energy_j += 0.5*(p_prev+p_w)*(now-t_prev)/1000.0; p_prev = p_w; }
  t_prev = now;
  if (Serial.available() && Serial.read() == '0') { heaterSet(false); Serial.println("DUNG KHAN"); return false; }
  return true;
}

static void header() {
  Serial.println("\n  t_s\tstate\tT_suoi\tT_nen\tchenh\tI_mA\tP_W\tJ");
}

// ===========================================================================
//  a — Đặc tính nhiệt: sưởi tới T_TRIP_TEST, cắt, theo dõi vọt lố rồi nguội
// ===========================================================================
static void modeA() {
  Serial.println("\n===== a) DAC TINH NHIET =====");
  readTemps();
  if (isnan(t_heater)) { Serial.println("✗ khong doc duoc cam bien tren dien tro"); return; }
  if (latched) { Serial.println("✗ dang chot, nhan RESET"); return; }
  Serial.printf("Suoi lien tuc toi %.1f °C roi cat. Bat dau tu %.2f °C.\n", T_TRIP_TEST, t_heater);
  header();

  const uint32_t t0 = millis();
  t_prev = t0; p_prev = 0;
  float t_start = t_heater;
  uint32_t t_trip = 0; float t_at_trip = NAN, t_peak = -999;

  heaterSet(true);
  // --- pha 1: đi lên ---
  while (millis() - t0 < 600000) {
    readTemps();
    if (hardCut() || !tick()) return;
    logLine(t0, heater_on ? "ON " : "off");
    if (!isnan(t_heater) && t_heater >= T_TRIP_TEST) {
      heaterSet(false);
      t_trip = millis(); t_at_trip = t_heater;
      Serial.printf(">>> CAT o %.2f °C sau %.1f s\n", t_at_trip, (t_trip-t0)/1000.0);
      break;
    }
  }
  if (t_trip == 0) { heaterSet(false); Serial.println("✗ 10 phut chua toi nguong — kiem tra suoi"); return; }

  // --- pha 2: vọt lố + nguội ---
  Serial.println("--- da cat, theo doi vot lo va nguoi 180 s ---");
  const uint32_t t1 = millis();
  float t_63 = NAN; uint32_t t_tau = 0;
  while (millis() - t1 < 180000) {
    readTemps();
    if (hardCut() || !tick()) return;
    logLine(t0, "off");
    if (!isnan(t_heater)) {
      if (t_heater > t_peak) t_peak = t_heater;
      // hằng số thời gian: thời điểm nguội được 63 % quãng từ đỉnh về nền
      if (isnan(t_63) && !isnan(t_ref) && t_peak > t_ref + 1.0f) {
        const float target = t_peak - 0.632f*(t_peak - t_ref);
        if (t_heater <= target) { t_63 = t_heater; t_tau = millis() - t1; }
      }
    }
  }

  Serial.println("\n--- KET LUAN ---");
  Serial.printf("  Toc do len   : %.2f -> %.2f °C trong %.1f s  = %.3f °C/s\n",
                t_start, t_at_trip, (t_trip-t0)/1000.0,
                (t_at_trip-t_start)/((t_trip-t0)/1000.0));
  Serial.printf("  VOT LO       : dinh %.2f °C, vuot nguong cat %.2f °C\n",
                t_peak, t_peak - t_at_trip);
  if (t_tau) Serial.printf("  Hang so nguoi: ~%.0f s (ve 63%% quang tu dinh xuong nen)\n", t_tau/1000.0);
  else       Serial.println("  Hang so nguoi: chua nguoi du trong 180 s");
  Serial.printf("  Nang luong   : %.1f J\n", energy_j);
  Serial.println("\n  Doc: neu VOT LO > 5 °C thi nguong cung 60 °C KHONG du an toan khi");
  Serial.println("  dan len pin that — phai ha nguong xuong, vi nhiet con di tiep sau khi cat.");
}

// ===========================================================================
//  b — Điều nhiệt vòng kín: giữ quanh T_TARGET
// ===========================================================================
static void modeB() {
  Serial.println("\n===== b) DIEU NHIET VONG KIN =====");
  readTemps();
  if (isnan(t_heater)) { Serial.println("✗ khong doc duoc cam bien tren dien tro"); return; }
  if (latched) { Serial.println("✗ dang chot, nhan RESET"); return; }
  Serial.printf("Dich %.1f °C, tre %.1f °C, giu toi thieu %d s moi trang thai. Chay 5 phut.\n",
                T_TARGET, T_HYST, DWELL_MS/1000);
  header();

  const uint32_t t0 = millis();
  t_prev = t0; p_prev = 0;
  uint32_t t_switch = 0;
  int n_sw = 0; float t_min = 999, t_max = -999; bool settled = false;

  while (millis() - t0 < 300000) {
    readTemps();
    if (hardCut() || !tick()) return;

    /* Bật/tắt có TRỄ và có thời gian giữ tối thiểu.
       Trễ để không nhấp nháy quanh đúng đích — cùng bài học với
       AL_T_CRIT_CLEAR trong alarm.h. Thời gian giữ để INA226 còn kịp đo. */
    if (!isnan(t_heater) && millis() - t_switch > DWELL_MS) {
      if (heater_on && t_heater >= T_TARGET)            { heaterSet(false); t_switch = millis(); n_sw++; }
      else if (!heater_on && t_heater < T_TARGET-T_HYST) { heaterSet(true);  t_switch = millis(); n_sw++; }
    }
    if (!settled && !isnan(t_heater) && t_heater >= T_TARGET) settled = true;
    if (settled && !isnan(t_heater)) { t_min = min(t_min, t_heater); t_max = max(t_max, t_heater); }
    logLine(t0, heater_on ? "ON " : "off");
  }
  heaterSet(false);
  Serial.println("\n--- KET LUAN ---");
  if (settled) Serial.printf("  Dao dong quanh dich: %.2f .. %.2f °C (bien do %.2f), %d lan dong cat\n",
                             t_min, t_max, t_max-t_min, n_sw);
  else         Serial.println("  Chua bao gio toi dich — suoi yeu hoac dich dat qua cao");
  Serial.printf("  Nang luong: %.1f J\n", energy_j);
}

// ===========================================================================
void setup() {
  pinMode(PIN_HEATER, OUTPUT); digitalWrite(PIN_HEATER, LOW);
  pinMode(PIN_BUZZER, OUTPUT); digitalWrite(PIN_BUZZER, LOW);

  Serial.begin(115200);
  uint32_t w = millis(); while (!Serial && millis()-w < 3000) {}

  Wire.begin(PIN_SDA, PIN_SCL, 400000);
  inaWrite16(0x00, 0x4527);
  inaWrite16(0x05, (uint16_t)(0.00512f/((1.0f/32768.0f)*INA_R_SHUNT)+0.5f));
  uint16_t mfr=0, die=0;
  ina_ok = inaRead16(0xFE,mfr) && inaRead16(0xFF,die) && mfr==0x5449 && die==0x2260;

  ds.begin();
  ds.setResolution(12);
  ds.setWaitForConversion(true);
  ds.setCheckForConversion(false);   // ép chờ đủ 750 ms, xem QĐ-037
  t_last_ok = millis();

  Serial.println("\n\n=== Dac tinh nhiet dien tro suoi — Doi Hu Tieu ===");
  Serial.printf("INA226: %s · %d cam bien tren bus\n", ina_ok?"OK":"KHONG THAY", ds.getDeviceCount());
  Serial.print("Cam bien tren dien tro: ");
  for (int i=0;i<8;i++) Serial.printf("%02X%s", ROM_HEATER[i], i<7?" ":"\n");
  readTemps();
  if (isnan(t_heater)) Serial.println("⚠️ KHONG doc duoc con do — kiem tra lai ROM va day");
  else Serial.printf("Dang o %.2f °C, nen %.2f °C\n", t_heater, t_ref);
  Serial.printf("Nguong cung %.1f °C luon bat, khong the tat.\n", T_HARD_CUT);
  Serial.println("5 giay de rut dien neu co gi bat thuong...");
  delay(5000);
  Serial.println("\nMENU: a = dac tinh nhiet · b = dieu nhiet vong kin · 0 = tat");
}

void loop() {
  if (!Serial.available()) { delay(20); return; }
  switch (Serial.read()) {
    case 'a': case 'A': modeA(); break;
    case 'b': case 'B': modeB(); break;
    case '0': heaterSet(false); digitalWrite(PIN_BUZZER, LOW); Serial.println("da tat."); break;
    default: return;
  }
  Serial.println("\n> san sang (a / b / 0)");
}
