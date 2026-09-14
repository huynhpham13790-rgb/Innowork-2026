/* =============================================================================
 *  Bench Lớp 1 chạy trên CẢM BIẾN THẬT — không cần WiFi, không cần cloud.
 *
 *  Dùng đúng cell_temp.* và cell_ai.* của firmware chính (symlink, không phải
 *  bản sao — sửa firmware là bench đổi theo, không bao giờ lệch nhau).
 *
 *  Mục đích: kiểm hai thứ mà chỉ cảm biến thật mới trả lời được.
 *
 *  A. BẢNG OFFSET CÓ THẬT SỰ CÓ TÁC DỤNG KHÔNG?
 *     Chạy bench này khi 8 đầu dò đang **bó cụm nhúng trong nước**, tức là cả
 *     8 con thật sự cùng một nhiệt độ. Khi đó:
 *        độ rộng THÔ        = sai số chế tạo (kỳ vọng ~0,37 °C)
 *        độ rộng ĐÃ HIỆU CHỈNH = kỳ vọng < 0,1 °C
 *     Nếu bản hiệu chỉnh không nhỏ hơn hẳn thì bảng offset sai hoặc ánh xạ ROM
 *     bị lệch — phải biết trước khi dán lên pin, không phải sau.
 *
 *  B. LỚP 1 CÓ IM LẶNG KHI MỌI THỨ BÌNH THƯỜNG KHÔNG?
 *     Cả 8 cell cùng nhiệt độ là trạng thái "khoẻ" rõ ràng nhất có thể dựng
 *     được. AI mà báo động ở đây thì nó sẽ báo động suốt ngày ngoài đời.
 *     Đây là phép thử báo động giả rẻ nhất và thật nhất.
 *
 *  Muốn thử tiếp: nhấc MỘT đầu dò ra khỏi nước, cầm trong tay. Nó sẽ ấm lên
 *  vài độ và Lớp 1 phải chỉ đúng kênh đó.
 * ========================================================================== */
#include "cell_temp.h"
#include "cell_ai.h"

#define TEMP_PIN 4

CellTemp gTemp;
CellAI   gAI;

unsigned long lastAi = 0, lastReport = 0;
uint32_t nSamples = 0, nAlarm = 0;
float sumSpreadCal = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  delay(300);

  Serial.println("\n============================================================");
  Serial.println("  Bench Lop 1 tren CAM BIEN THAT (khong can WiFi)");
  Serial.println("============================================================");

  if (!gTemp.begin(TEMP_PIN)) {
    Serial.println("[LOI] khong du 8 cam bien — ket qua ben duoi KHONG dung.");
  }
  gAI.begin();

  Serial.printf("\nNguong bao dong Lop 1: %.4f | phai giu lien tuc %u giay\n",
                (double)CellAI::threshold(), CellAI::persist_s());
  Serial.println("\nDat ca 8 dau do trong NUOC (cung mot nhiet do) roi doc ket qua.\n");
}

void loop() {
  gTemp.update();                       // không chặn

  if (millis() - lastAi < 1000) return; // Lớp 1 chạy đúng 1 Hz
  lastAi = millis();
  if (!gTemp.status().ready) return;

  const float* t = gTemp.temps();

  // Độ rộng SAU hiệu chỉnh (cái mà AI thật sự nhìn thấy)
  float lo = t[0], hi = t[0];
  for (int i = 1; i < CT_N; i++) { if (t[i] < lo) lo = t[i]; if (t[i] > hi) hi = t[i]; }
  float spreadCal = hi - lo;

  // Độ rộng THÔ — cộng offset ngược lại để biết nếu KHÔNG hiệu chỉnh thì sao
  float rlo = t[0] + DS_OFFSET[0], rhi = rlo;
  for (int i = 1; i < CT_N; i++) {
    float r = t[i] + DS_OFFSET[i];
    if (r < rlo) rlo = r; if (r > rhi) rhi = r;
  }
  float spreadRaw = rhi - rlo;

  CellAIResult r = gAI.update(t, 28.0f, 0.0f, 80.0f);
  if (!r.valid) { Serial.println("[AI] dang khoi dong..."); return; }

  nSamples++;
  sumSpreadCal += spreadCal;
  if (r.alarm) nAlarm++;

  if (millis() - lastReport >= 10000) {
    lastReport = millis();
    Serial.printf("\n--- sau %lu mau ---\n", nSamples);
    Serial.print("nhiet do da hieu chinh:");
    for (int i = 0; i < CT_N; i++) Serial.printf(" %.3f", t[i]);
    Serial.println();
    Serial.printf("do rong THO        : %.4f C  (neu KHONG hieu chinh)\n", spreadRaw);
    Serial.printf("do rong DA HIEU CHINH: %.4f C  <- AI nhin thay cai nay\n", spreadCal);
    Serial.printf("  -> hieu chinh thu hep %.2f lan\n",
                  spreadCal > 0.001f ? spreadRaw / spreadCal : 0.0f);
    Serial.printf("diem cao nhat: cell %d = %.4f (nguong %.4f)\n",
                  r.worst_cell + 1, r.worst_score, (double)CellAI::threshold());
    Serial.printf("bao dong: %lu/%lu mau (%.2f%%)%s\n", nAlarm, nSamples,
                  100.0 * nAlarm / nSamples,
                  nAlarm ? "  <-- BAO DONG GIA, xem lai!" : "  <- tot, im lang");
    gTemp.printStatus();
  }
}
