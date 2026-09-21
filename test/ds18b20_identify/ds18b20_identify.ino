/* =============================================================================
 *  Gán nhãn đầu dò DS18B20 bằng cốc nước lạnh.
 *
 *  BÀI TOÁN: tám sợi dây giống hệt nhau, không biết sợi nào là ROM nào. Mà
 *  ánh xạ "sợi dây ↔ cell" là thứ KHÔNG suy ra được từ phần mềm, và sai nó thì
 *  Lớp 1 vẫn chạy, vẫn ra số, chỉ là chỉ đúng cell sai — không có lỗi nào báo.
 *
 *  CÁCH DÙNG: nhúng MỘT đầu dò vào cốc nước lạnh, đọc tên nó hiện ra trên màn
 *  hình, lấy giấy dán số lên sợi đó. Nhấc ra, chờ ấm lại, nhúng sợi tiếp theo.
 *
 *  VÌ SAO SO VỚI TRUNG VỊ CHỨ KHÔNG SO VỚI NGƯỠNG TUYỆT ĐỐI:
 *  ngưỡng kiểu "dưới 20 °C là đang nhúng" phụ thuộc vào nước lạnh tới đâu và
 *  phòng nóng tới đâu — hôm nay đúng, mai sai. Trung vị của cả dàn là mốc tự
 *  trôi theo nhiệt độ phòng, nên chỉ cần nước lạnh hơn phòng vài độ là nhận
 *  ra. Dùng TRUNG VỊ chứ không dùng trung bình vì chính cái kênh đang nhúng sẽ
 *  kéo lệch trung bình, làm chính nó trông đỡ lạnh đi.
 *
 *  Đối chiếu với bảng DS_ROM trong ds18b20_offsets.h để gọi thẳng tên P01..P08.
 *  ROM nào không có trong bảng thì báo "LẠ" kèm mã đầy đủ để chép vào bảng.
 * ========================================================================== */
#include <OneWire.h>
#include <DallasTemperature.h>
#include "ds18b20_offsets.h"

#define PIN_ONEWIRE 4
#define DS_MAX      12
#define DIP_DELTA   (-2.0f)   // lạnh hơn trung vị bấy nhiêu thì coi là đang nhúng

OneWire oneWire(PIN_ONEWIRE);
DallasTemperature ds(&oneWire);

static DeviceAddress rom[DS_MAX];
static float  t[DS_MAX];
static float  dev_prev[DS_MAX];   // chênh lệch so với trung vị ở lượt trước
static bool   have_prev = false;
static int    n = 0;
static int    last_dip = -1;

/* Tên theo bảng đã hiệu chuẩn. Bảng DS_ROM chỉ còn 6 dòng khi PACK_N_CELLS=6,
   nên hai con P07/P08 phải tra riêng — chúng vẫn tồn tại về mặt vật lý, chỉ là
   không nằm trong cấu hình pack hiện tại. */
static const uint8_t ROM_P07[8] = { 0x28, 0x4D, 0x49, 0x04, 0x00, 0x00, 0x00, 0x35 };
static const uint8_t ROM_P08[8] = { 0x28, 0x7F, 0xCD, 0x01, 0x00, 0x00, 0x00, 0xE3 };

static bool romEq(const uint8_t* a, const uint8_t* b) { return memcmp(a, b, 8) == 0; }

static void nameOf(const uint8_t* r, char* out, size_t cap) {
  for (int i = 0; i < DS_N_PROBES; i++)
    if (romEq(r, DS_ROM[i])) { snprintf(out, cap, "P%02d", i + 1); return; }
  if (romEq(r, DS_ROM_AMBIENT)) { snprintf(out, cap, "MOI TRUONG"); return; }
  if (romEq(r, ROM_P07)) { snprintf(out, cap, "P07 (ngoai pack 6S)"); return; }
  if (romEq(r, ROM_P08)) { snprintf(out, cap, "P08 (ngoai pack 6S)"); return; }
  snprintf(out, cap, "LA");
}

static void printRom(const uint8_t* r) {
  for (int j = 0; j < 8; j++) Serial.printf("%02X%s", r[j], j < 7 ? " " : "");
}

void setup() {
  Serial.begin(115200);
  uint32_t w = millis();
  while (!Serial && millis() - w < 3000) { }

  ds.begin();
  ds.setWaitForConversion(true);
  ds.setCheckForConversion(false);   // ép chờ đủ 750 ms, xem QĐ-037
  ds.setResolution(12);

  oneWire.reset_search();
  DeviceAddress a;
  while (n < DS_MAX && oneWire.search(a)) {
    if (OneWire::crc8(a, 7) != a[7]) continue;
    memcpy(rom[n], a, 8); n++;
  }

  Serial.printf("\n\n===== Gan nhan dau do DS18B20 — thay %d con tren GPIO%d =====\n",
                n, PIN_ONEWIRE);
  bool missing = false;
  for (int i = 0; i < DS_N_PROBES; i++) {
    bool found = false;
    for (int k = 0; k < n; k++) if (romEq(rom[k], DS_ROM[i])) found = true;
    if (!found) {
      Serial.printf("  ✗ THIEU P%02d  (", i + 1); printRom(DS_ROM[i]); Serial.println(")");
      missing = true;
    }
  }
  bool amb = false;
  for (int k = 0; k < n; k++) if (romEq(rom[k], DS_ROM_AMBIENT)) amb = true;
  if (!amb) { Serial.print("  ✗ THIEU cam bien MOI TRUONG  ("); printRom(DS_ROM_AMBIENT); Serial.println(")"); }
  for (int k = 0; k < n; k++) {
    char nm[24]; nameOf(rom[k], nm, sizeof(nm));
    if (!strcmp(nm, "LA")) { Serial.print("  ? ROM LA khong co trong bang: "); printRom(rom[k]); Serial.println(); }
  }
  if (!missing && amb) Serial.println("  ✓ Du mat tat ca dau do trong bang.");

  Serial.println("\nNhung MOT dau do vao coc nuoc lanh. Doc ten hien ra roi dan giay.");
  Serial.println("Nhac ra, cho am lai, nhung soi tiep theo.\n");
}

void loop() {
  ds.requestTemperatures();
  float v[DS_MAX]; int m = 0;
  for (int i = 0; i < n; i++) {
    t[i] = ds.getTempC(rom[i]);
    if (t[i] > -40 && t[i] < 125 && t[i] != 85.0f) v[m++] = t[i];
  }
  if (m < 3) { Serial.println("  (chua du kenh doc duoc)"); delay(500); return; }

  // trung vị: sắp xếp chèn, m nhỏ nên không cần gì phức tạp hơn
  for (int i = 1; i < m; i++) {
    float x = v[i]; int j = i - 1;
    while (j >= 0 && v[j] > x) { v[j + 1] = v[j]; j--; }
    v[j + 1] = x;
  }
  const float med = (m % 2) ? v[m / 2] : 0.5f * (v[m / 2 - 1] + v[m / 2]);

  /* Chọn sợi đang nhúng.
     Không lấy đơn thuần "kênh lạnh nhất": sợi VỪA NHẤC RA còn lạnh hàng chục
     giây, nên có lúc hai ba sợi cùng lạnh và kênh lạnh nhất lại là sợi đã làm
     xong. Phân biệt bằng CHIỀU BIẾN THIÊN — sợi đang trong nước thì lạnh DẦN,
     sợi vừa nhấc ra thì ấm DẦN. Đã gặp thật: P08 (-6,31 đang lạnh dần) mới là
     sợi trong nước, trong khi P07 (-7,75 -> -3,56, đang ấm lên) là sợi cũ.
     Chỉ khi không sợi nào đang lạnh dần thì mới đành lấy sợi lạnh nhất. */
  float dev[DS_MAX];
  int dip = -1, dip_any = -1;
  float best_fall = -0.05f, worst = DIP_DELTA, worst_any = DIP_DELTA;
  for (int i = 0; i < n; i++) {
    // -127 (mất kết nối) và 85,0 (chưa chuyển đổi) là MÃ LỖI, không phải nhiệt
    // độ. Không loại thì -127 luôn thắng cuộc thi "lạnh nhất" và cướp kết luận.
    if (!(t[i] > -40 && t[i] < 125 && t[i] != 85.0f)) { dev[i] = NAN; continue; }
    dev[i] = t[i] - med;
    if (dev[i] >= DIP_DELTA) continue;
    if (dev[i] < worst_any) { worst_any = dev[i]; dip_any = i; }
    if (!have_prev || isnan(dev_prev[i])) continue;
    const float trend = dev[i] - dev_prev[i];
    if (trend < best_fall && dev[i] < worst) { best_fall = trend; worst = dev[i]; dip = i; }
  }
  if (dip < 0) { dip = dip_any; worst = worst_any; }
  for (int i = 0; i < n; i++) dev_prev[i] = dev[i];
  have_prev = true;

  Serial.printf("trung vi %.2f °C | ", med);
  for (int i = 0; i < n; i++) {
    char nm[24]; nameOf(rom[i], nm, sizeof(nm));
    Serial.printf("%s %+.2f  ", nm, t[i] - med);
  }
  Serial.println();

  if (dip >= 0 && dip != last_dip) {
    char nm[24]; nameOf(rom[dip], nm, sizeof(nm));
    Serial.println("  ----------------------------------------------------");
    Serial.printf("  >>> DANG NHUNG: %s   (lanh hon trung vi %.2f °C)\n", nm, worst);
    for (int i = 0; i < n; i++) {
      if (i == dip || isnan(dev[i]) || dev[i] >= DIP_DELTA) continue;
      char o[24]; nameOf(rom[i], o, sizeof(o));
      Serial.printf("      (bo qua %s: %+.2f — dang am len, la soi vua nhac ra)\n", o, dev[i]);
    }
    Serial.print("      ROM: "); printRom(rom[dip]); Serial.println();
    Serial.println("      -> Dan giay so nay len soi day dang cam trong nuoc.");
    Serial.println("  ----------------------------------------------------");
  }
  if (dip < 0 && last_dip >= 0)
    Serial.println("  (da nhac ra — nhung soi tiep theo)");
  last_dip = dip;
  delay(200);
}
