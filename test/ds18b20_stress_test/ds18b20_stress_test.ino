/* =============================================================================
 *  Kiểm tra 8 cảm biến DS18B20 — bản đo đạc, không phải bản "xem cho vui".
 *
 *  Sketch bench trước (ds18b20_bench_test) trả lời được "có nhìn thấy cảm biến
 *  không". Sketch này trả lời 4 câu khó hơn, là những câu quyết định có dám
 *  cho Lớp 1 ăn số thật hay không:
 *
 *    1. NHIỄU NỀN     — mỗi kênh dao động bao nhiêu khi nhiệt độ đứng yên?
 *    2. LỆCH GIỮA CON — 8 con cùng một chỗ có ra cùng một số không?
 *    3. ĐỘ BỀN BUS    — ép đọc liên tục, có mất gói / sai CRC không?
 *    4. THỜI GIAN ĐỌC — mất bao lâu, có chặn vòng lặp 1 Hz không?
 *
 *  BA KHÁC BIỆT SO VỚI SKETCH BENCH — đều là chỗ bench làm sai kiểu âm thầm:
 *
 *  (a) Đọc theo ĐỊA CHỈ ROM, không theo index.
 *      `getTempCByIndex(i)` trả về con thứ i theo thứ tự dò bus. Thứ tự đó do
 *      giá trị ROM quyết định, nên chỉ cần thay một con cảm biến là toàn bộ
 *      ánh xạ "kênh i ↔ cell nào" đổi hết — mà không có lỗi nào báo. Với Lớp 1
 *      thì đó là hoán vị dữ liệu 8 cell, hỏng hoàn toàn nhưng vẫn chạy.
 *
 *  (b) Đọc KHÔNG CHẶN (setWaitForConversion(false)).
 *      Chuyển đổi 12 bit tốn ~750 ms. Bản chặn sẽ giữ CPU từng ấy mỗi lần đọc,
 *      trong khi firmware chính phải gọi mqtt.loop() đều đặn — chặn 750 ms mỗi
 *      giây là rớt keep-alive. Đây cũng là mẫu code sẽ bê thẳng sang firmware.
 *
 *  (c) ĐẾM LỖI, không chỉ in số.
 *      Một lần đọc lỗi trong 10 000 lần là thứ không thể thấy bằng mắt khi
 *      nhìn log chạy. Phải đếm.
 *
 *  Chạy: nạp rồi để yên ~5 phút, KHÔNG chạm vào đầu dò. Bảng thống kê tự in
 *  mỗi 30 s. Muốn thử độ bền cơ học thì lắc dây / ngoáy breadboard sau khi đã
 *  có bảng đầu tiên, rồi xem cột CRC/DISC có nhảy không.
 * ========================================================================== */
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS   4
#define N_PROBES       8
#define REPORT_MS      30000
#define RESOLUTION     12        // 0,0625 °C — khớp mức lượng tử hoá dữ liệu train

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

struct Probe {
  DeviceAddress rom;
  uint32_t n_ok = 0, n_crc = 0, n_disc = 0, n_reset85 = 0;
  double   sum = 0, sum2 = 0;          // double: xem QĐ-015, float32 cộng dồn
  float    vmin = 1000, vmax = -1000;  // bị mất chữ số ở đuôi
  float    last = NAN;
  uint32_t n_jump = 0;                 // số lần nhảy > 1 °C giữa 2 lần đọc liền
};

Probe probes[N_PROBES];
uint8_t nFound = 0;

// Trạng thái đọc không chặn
bool     converting = false;
uint32_t convStart = 0;
uint32_t cycles = 0, convMsSum = 0, convMsMax = 0;
uint32_t tReport = 0, tStart = 0;

void printRom(const DeviceAddress a) {
  for (uint8_t i = 0; i < 8; i++) Serial.printf("%02X", a[i]);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  delay(300);

  Serial.println("\n============================================================");
  Serial.println("  DS18B20 x8 — do nhieu nen, lech giua con, do ben bus");
  Serial.println("============================================================");

  sensors.begin();
  nFound = sensors.getDeviceCount();
  Serial.printf("[INFO] GPIO %d — tim thay %d thiet bi\n", ONE_WIRE_BUS, nFound);

  // Bus ký sinh (parasite power) chỉ có 1 dây dữ liệu, thời gian chuyển đổi
  // khắt khe hơn nhiều và dễ lỗi khi nhiều cảm biến cùng chuyển đổi. Phải biết
  // mình đang ở chế độ nào trước khi kết luận về độ bền.
  Serial.printf("[INFO] Che do nguon: %s\n",
                sensors.isParasitePowerMode() ? "KY SINH (2 day) - canh giac"
                                              : "CAP NGUON RIENG (3 day) - tot");

  if (nFound != N_PROBES) {
    Serial.printf("[CANH BAO] Mong doi %d, thay %d. Kiem tra dien tro keo len "
                  "4,7k va tiep xuc breadboard.\n", N_PROBES, nFound);
  }
  if (nFound == 0) { Serial.println("[DUNG] Khong co cam bien nao."); return; }
  if (nFound > N_PROBES) nFound = N_PROBES;

  Serial.println("\n--- Dia chi ROM (dan nhan len tung day theo thu tu nay) ---");
  for (uint8_t i = 0; i < nFound; i++) {
    if (!sensors.getAddress(probes[i].rom, i)) {
      Serial.printf("P%02d: KHONG lay duoc dia chi\n", i + 1);
      continue;
    }
    sensors.setResolution(probes[i].rom, RESOLUTION);
    Serial.printf("const uint8_t PROBE_%02d[8] = { ", i + 1);
    for (uint8_t k = 0; k < 8; k++)
      Serial.printf("0x%02X%s", probes[i].rom[k], k < 7 ? ", " : " };\n");
  }

  // Mấu chốt (b): từ đây requestTemperatures() trả về ngay, không đợi.
  sensors.setWaitForConversion(false);

  Serial.println("\nDang do... de yen ~5 phut, KHONG cham vao dau do.");
  Serial.println("Bang thong ke in moi 30 giay.\n");
  tStart = tReport = millis();
}

void loop() {
  if (nFound == 0) return;

  if (!converting) {
    sensors.requestTemperatures();      // không chặn
    convStart = millis();
    converting = true;
    return;
  }

  // 12 bit cần ~750 ms. Chưa tới hạn thì thoát ngay — đây chính là chỗ firmware
  // chính sẽ chạy mqtt.loop() và mọi việc khác thay vì ngồi đợi.
  if (millis() - convStart < 760) return;

  uint32_t dt = millis() - convStart;
  convMsSum += dt; if (dt > convMsMax) convMsMax = dt;
  converting = false;
  cycles++;

  for (uint8_t i = 0; i < nFound; i++) {
    Probe &p = probes[i];
    float t = sensors.getTempC(p.rom);   // mấu chốt (a): theo ROM, không theo index

    if (t == DEVICE_DISCONNECTED_C) { p.n_disc++; continue; }
    // 85,0 chẵn là giá trị thanh ghi lúc bật nguồn — nghĩa là đọc trước khi
    // chuyển đổi xong, hoặc cảm biến vừa bị reset vì sụt áp.
    if (t == 85.0f) { p.n_reset85++; continue; }
    if (t < -50.0f || t > 125.0f) { p.n_crc++; continue; }

    if (!isnan(p.last) && fabsf(t - p.last) > 1.0f) p.n_jump++;
    p.last = t;
    p.n_ok++;
    p.sum += t; p.sum2 += (double)t * t;
    if (t < p.vmin) p.vmin = t;
    if (t > p.vmax) p.vmax = t;
  }

  if (millis() - tReport >= REPORT_MS) { report(); tReport = millis(); }
}

void report() {
  Serial.printf("\n===== SAU %lu giay, %lu vong doc =====\n",
                (millis() - tStart) / 1000, cycles);
  Serial.printf("Thoi gian chuyen doi: TB %lu ms, max %lu ms\n",
                cycles ? convMsSum / cycles : 0, convMsMax);
  Serial.println("Kenh  TrungBinh   Nhieu(std)   Min     Max    | OK    CRC DISC R85 JUMP");

  double gsum = 0; uint8_t gn = 0;
  double mean[N_PROBES];

  for (uint8_t i = 0; i < nFound; i++) {
    Probe &p = probes[i];
    if (p.n_ok < 2) {
      Serial.printf("P%02d   (khong du mau)                       | %-5lu %-3lu %-4lu %-3lu\n",
                    i + 1, p.n_ok, p.n_crc, p.n_disc, p.n_reset85);
      mean[i] = NAN;
      continue;
    }
    double mu = p.sum / p.n_ok;
    double var = p.sum2 / p.n_ok - mu * mu;
    if (var < 0) var = 0;                        // chống sai số trừ hai số gần nhau
    mean[i] = mu; gsum += mu; gn++;
    Serial.printf("P%02d   %8.4f   %8.4f   %6.2f %6.2f | %-5lu %-3lu %-4lu %-3lu %-4lu\n",
                  i + 1, mu, sqrt(var), p.vmin, p.vmax,
                  p.n_ok, p.n_crc, p.n_disc, p.n_reset85, p.n_jump);
  }

  if (gn == 0) return;
  double gmean = gsum / gn;
  double omin = 1e9, omax = -1e9;

  Serial.printf("\nTrung binh toan pack: %.4f C\n", gmean);
  Serial.println("Bang OFFSET — day la so se nap vao firmware:");
  Serial.println("// offset[i] = trung binh kenh i - trung binh toan pack");
  Serial.printf("const float T_OFFSET[%d] = {", nFound);
  for (uint8_t i = 0; i < nFound; i++) {
    double off = isnan(mean[i]) ? 0.0 : mean[i] - gmean;
    if (!isnan(mean[i])) { if (off < omin) omin = off; if (off > omax) omax = off; }
    Serial.printf(" %+.4ff%s", off, i < nFound - 1 ? "," : " };\n");
  }

  double spread = omax - omin;
  Serial.printf("\nDO RONG LECH GIUA CAC CON: %.4f C\n", spread);
  if (spread < 0.1)
    Serial.println("  -> Rat deu. Co the BO QUA hieu chinh offset.");
  else if (spread < 0.3)
    Serial.println("  -> Lech vua phai. Nen hieu chinh, chua den muc nguy hiem.");
  else
    Serial.println("  -> BAT BUOC hieu chinh. Lop 1 nhin chenh lech tuong doi giua "
                   "cell,\n     nen lech co dinh nay trong y het mot cell luon nong hon.");

  uint32_t tot_err = 0, tot_ok = 0;
  for (uint8_t i = 0; i < nFound; i++) {
    tot_err += probes[i].n_crc + probes[i].n_disc + probes[i].n_reset85;
    tot_ok  += probes[i].n_ok;
  }
  Serial.printf("\nDO BEN BUS: %lu doc tot, %lu loi (%.4f%%)\n",
                tot_ok, tot_err, tot_ok + tot_err ? 100.0 * tot_err / (tot_ok + tot_err) : 0.0);
  Serial.println(tot_err == 0 ? "  -> Bus sach tuyet doi."
                              : "  -> Co loi. Kiem tra dien tro keo len, do dai day, tiep xuc.");
  Serial.println("========================================\n");
}
