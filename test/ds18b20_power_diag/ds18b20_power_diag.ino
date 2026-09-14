/* =============================================================================
 *  Chẩn đoán nguồn DS18B20 — tìm ĐÚNG con nào đang báo ký sinh.
 *
 *  Vì sao cần sketch riêng: `isParasitePowerMode()` chỉ trả về một bit cho CẢ
 *  BUS. Nhìn vào thư viện (DallasTemperature.cpp) thì thấy vì sao nó vô dụng
 *  cho việc sửa lỗi:
 *
 *      if (!parasite && readPowerSupply(deviceAddress)) parasite = true;
 *
 *  Chỉ cần MỘT con báo ký sinh là cờ chung bật, và thư viện đổi cách xử lý cho
 *  cả 8 con. Nên "bus báo ký sinh" KHÔNG có nghĩa là quên nối VDD cả dãy — rất
 *  có thể chỉ một chân VDD tiếp xúc kém trên breadboard.
 *
 *  May là `readPowerSupply(rom)` hỏi được TỪNG con. Sketch này khai thác đúng
 *  chỗ đó, và trả lời 3 câu:
 *
 *    1. Con nào báo ký sinh? (hỏi riêng từng ROM)
 *    2. Nó báo ỔN ĐỊNH hay CHẬP CHỜN? — hỏi 200 lần mỗi con.
 *         ổn định 100% ký sinh  -> chân VDD của con đó KHÔNG nối / nối nhầm
 *         chập chờn             -> tiếp xúc kém trên breadboard (lỗi hay gặp nhất)
 *         ổn định 0% ký sinh    -> con đó bình thường
 *    3. Đọc CHẶN so với KHÔNG CHẶN thì tỉ lệ lỗi khác nhau thế nào?
 *         Nếu chặn thì sạch mà không chặn thì hỏng -> đúng là do ký sinh, vì ở
 *         chế độ ký sinh thư viện phải giữ dây ở mức cao suốt lúc chuyển đổi.
 *         Nếu cả hai đều hỏng -> lỗi đấu dây/điện trở kéo lên, không phải nguồn.
 *
 *  Chạy: nạp rồi để yên, KHÔNG chạm vào dây. ~2 phút là xong, tự in kết luận.
 * ========================================================================== */
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS  4
#define N_PROBES      8
#define N_POWER_POLL  200      // số lần hỏi nguồn mỗi con
#define N_READ_CYCLE  40       // số vòng đọc mỗi chế độ

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DeviceAddress rom[N_PROBES];
uint8_t nFound = 0;

void printRom(const uint8_t* a) {
  for (uint8_t i = 0; i < 8; i++) Serial.printf("%02X", a[i]);
}

/* Đọc N_READ_CYCLE vòng ở một chế độ, trả về số lần đọc lỗi. */
uint32_t runMode(bool blocking, uint32_t &okOut) {
  sensors.setWaitForConversion(blocking);
  uint32_t bad = 0, ok = 0;

  for (uint16_t c = 0; c < N_READ_CYCLE; c++) {
    sensors.requestTemperatures();
    if (!blocking) delay(800);          // chờ thủ công cho đủ thời gian chuyển đổi

    for (uint8_t i = 0; i < nFound; i++) {
      float t = sensors.getTempC(rom[i]);
      if (t == DEVICE_DISCONNECTED_C || t == 85.0f || t < -50 || t > 125) bad++;
      else ok++;
    }
  }
  okOut = ok;
  return bad;
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  delay(300);

  Serial.println("\n============================================================");
  Serial.println("  DS18B20 — chan doan nguon, tim dung con bao ky sinh");
  Serial.println("============================================================");

  sensors.begin();
  nFound = sensors.getDeviceCount();
  Serial.printf("[INFO] Tim thay %d thiet bi tren GPIO %d\n", nFound, ONE_WIRE_BUS);
  Serial.printf("[INFO] Co chung cua ca bus: %s\n",
                sensors.isParasitePowerMode() ? "KY SINH" : "CAP NGUON RIENG");
  if (nFound == 0) { Serial.println("[DUNG] khong co cam bien."); return; }
  if (nFound > N_PROBES) nFound = N_PROBES;

  for (uint8_t i = 0; i < nFound; i++) sensors.getAddress(rom[i], i);

  // ---------- CAU 1 + 2: hoi rieng tung con, nhieu lan ----------
  Serial.printf("\n===== HOI NGUON TUNG CON (%d lan moi con) =====\n", N_POWER_POLL);
  Serial.println("Kenh  ROM                Bao ky sinh   Ket luan");

  uint8_t nBad = 0;
  for (uint8_t i = 0; i < nFound; i++) {
    uint16_t para = 0;
    for (uint16_t k = 0; k < N_POWER_POLL; k++) {
      if (sensors.readPowerSupply(rom[i])) para++;
      delayMicroseconds(200);
    }
    float pct = 100.0f * para / N_POWER_POLL;

    Serial.printf("P%02d   ", i + 1);
    printRom(rom[i]);
    Serial.printf("   %5.1f%%      ", pct);

    if (pct == 0.0f)        Serial.println("OK - co nguon rieng");
    else if (pct >= 99.0f) { Serial.println("*** VDD KHONG NOI (on dinh) ***"); nBad++; }
    else                   { Serial.println("*** TIEP XUC CHAP CHON ***");      nBad++; }
  }

  // ---------- CAU 3: chan vs khong chan ----------
  Serial.println("\n===== DOC CHAN vs KHONG CHAN =====");
  uint32_t okB = 0, okN = 0;
  uint32_t badB = runMode(true,  okB);
  uint32_t badN = runMode(false, okN);

  float pB = 100.0f * badB / (okB + badB);
  float pN = 100.0f * badN / (okN + badN);
  Serial.printf("CHAN      : %lu tot, %lu loi (%.2f%%)\n", okB, badB, pB);
  Serial.printf("KHONG CHAN: %lu tot, %lu loi (%.2f%%)\n", okN, badN, pN);

  // ---------- ket luan ----------
  Serial.println("\n===== KET LUAN =====");
  if (nBad == 0) {
    Serial.println("Ca 8 con deu bao co nguon rieng.");
    Serial.println("-> Neu co chung van bao KY SINH thi la do lan quet luc begin(),");
    Serial.println("   goi lai sensors.begin() sau khi nguon da on dinh.");
  } else {
    Serial.printf("Co %d con co van de VDD — xem dong co dau *** o tren.\n", nBad);
    Serial.println("-> CHI can sua dung may con do, KHONG phai dau lai ca day.");
    Serial.println("-> 'TIEP XUC CHAP CHON' = chan cam long tren breadboard:");
    Serial.println("   rut ra cam lai sang lo khac, hoac han thang.");
  }

  if (pB < 1.0f && pN > 5.0f)
    Serial.println("Doc CHAN sach ma KHONG CHAN hong -> dung la do ky sinh.");
  else if (pB > 5.0f && pN > 5.0f)
    Serial.println("CA HAI che do deu hong -> loi day/dien tro keo len, KHONG phai nguon.\n"
                   "   Thu ha 4,7k xuong 2,2k, rut ngan day, kiem tra mass chung.");
  else if (pB < 1.0f && pN < 1.0f)
    Serial.println("Ca hai che do deu sach o lan chay nay. Bus dang o ranh gioi —\n"
                   "   phai chay lai nhieu lan moi ket luan duoc.");
  Serial.println("====================\n");
}

void loop() {}
