#include "ble_view.h"

#if !USE_BLE
/* Biên dịch rỗng: không kéo Bluedroid vào, firmware y hệt bản chưa có BLE. */
bool BleView::begin(const char*) { return false; }
void BleView::update(const float*, const CellAIResult&, AlarmLevel, bool, bool,
                     bool, float, float, float, int, bool, bool) {}
bool BleView::connected() const { return false; }
int  BleView::clientCount() const { return 0; }
#else

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2901.h>
#include <BLE2902.h>
#include <math.h>

/* UUID tự đặt. Bốn byte đầu là "HUTI" trong ASCII (0x48 0x55 0x54 0x49) để khi
   nhìn danh sách quét giữa một rừng thiết bị lạ thì nhận ra ngay của mình. */
#define UUID_SVC   "48555449-4555-4d53-0001-000000000000"
#define UUID_STATE "48555449-4555-4d53-0002-000000000000"
#define UUID_TEMPS "48555449-4555-4d53-0003-000000000000"
#define UUID_AI    "48555449-4555-4d53-0004-000000000000"
#define UUID_PACK  "48555449-4555-4d53-0005-000000000000"
#define UUID_SYS   "48555449-4555-4d53-0006-000000000000"

static const int N_CHAR = 5;
static BLEServer         *sServer = nullptr;
static BLECharacteristic *sCh[N_CHAR] = {nullptr};
static String             sLast[N_CHAR];

/* Quảng bá lại ngay khi thợ ngắt kết nối. Thiếu chỗ này thì người thứ hai tới
   kiểm pack sẽ KHÔNG QUÉT THẤY GÌ CẢ — thiết bị vẫn chạy, vẫn báo động, chỉ là
   vô hình. Đó là kiểu hỏng khó đoán nhất vì nó trông hệt như hỏng phần cứng. */
class SrvCb : public BLEServerCallbacks {
  void onConnect(BLEServer*) override {
    Serial.println("[BLE ] co dien thoai ket noi");
  }
  void onDisconnect(BLEServer* s) override {
    Serial.println("[BLE ] ngat ket noi - quang ba lai");
    s->startAdvertising();
  }
};
static SrvCb sSrvCb;

static BLECharacteristic* mkChar(BLEService* svc, const char* uuid,
                                 const char* label) {
  BLECharacteristic* c = svc->createCharacteristic(
      uuid, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  // 0x2902: bắt buộc có thì điện thoại mới bật được notify.
  c->addDescriptor(new BLE2902());
  // 0x2901: nhãn người đọc được. Không có nó thì nRF Connect chỉ hiện UUID và
  // thợ phải tra bảng — mất hẳn cái lợi "mở lên là hiểu".
  BLE2901* d = new BLE2901();
  d->setDescription(label);
  c->addDescriptor(d);
  return c;
}

bool BleView::begin(const char* device_id) {
  const uint32_t heap0 = ESP.getFreeHeap();

  BLEDevice::init(BLE_DEV_NAME);
  /* Xin MTU lớn để gói notify chứa đủ chuỗi nhiệt độ 6 cell. Đây chỉ là ĐỀ
     NGHỊ — điện thoại mới là bên chốt, nên vẫn không được phụ thuộc vào nó
     (xem ghi chú MTU trong ble_view.h). */
  BLEDevice::setMTU(247);

  sServer = BLEDevice::createServer();
  sServer->setCallbacks(&sSrvCb);

  BLEService* svc = sServer->createService(UUID_SVC);
  sCh[0] = mkChar(svc, UUID_STATE, "Trang thai");
  sCh[1] = mkChar(svc, UUID_TEMPS, "Nhiet do tung cell (degC)");
  sCh[2] = mkChar(svc, UUID_AI,    "Lop 1 - cell nghi ngo");
  sCh[3] = mkChar(svc, UUID_PACK,  "Dien ap / dong / SoC");
  sCh[4] = mkChar(svc, UUID_SYS,   "Tinh trang he thong");
  for (int i = 0; i < N_CHAR; i++) { sLast[i] = ""; sCh[i]->setValue("dang khoi dong"); }
  svc->start();

  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(UUID_SVC);
  adv->setScanResponse(true);   // tên thiết bị đi ở gói phụ, chừa chỗ cho UUID
  BLEDevice::startAdvertising();

  ready_ = true;
  Serial.printf("[BLE ] quang ba \"%s\" (%s), RAM ton %u byte, con trong %u\n",
                BLE_DEV_NAME, device_id,
                (unsigned)(heap0 - ESP.getFreeHeap()), (unsigned)ESP.getFreeHeap());
  Serial.println("[BLE ] doc bang nRF Connect - CHI DOC, khong co lenh ghi nao");
  return true;
}

bool BleView::connected()  const { return sServer && sServer->getConnectedCount() > 0; }
int  BleView::clientCount() const { return sServer ? (int)sServer->getConnectedCount() : 0; }

void BleView::put(int idx, const char* s) {
  if (sLast[idx] == s) return;          // không đổi thì không làm phiền
  sLast[idx] = s;
  sCh[idx]->setValue(sLast[idx]);
  if (connected()) sCh[idx]->notify();
}

static const char* levelText(AlarmLevel l) {
  switch (l) {
    case AL_OK:       return "BINH THUONG";
    case AL_WATCH:    return "THEO DOI - AI thay lech";
    case AL_ALARM:    return "BAO DONG - cell bat thuong";
    default:          return "NGUY KICH - vuot nguong cung";
  }
}

void BleView::update(const float* temps, const CellAIResult& ai, AlarmLevel lvl,
                     bool muted, bool quiet,
                     bool meter_ok, float pack_v, float pack_a, float soc_pct,
                     int n_healthy, bool wifi_ok, bool cloud_ok) {
  if (!ready_) return;
  char b[200];

  /* Trạng thái PHẢI kèm chuyện còi có đang bị bịt miệng không. Tắt tiếng mà
     không ai thấy chính là chế độ hỏng nguy hiểm nhất của cả khối báo động:
     đèn vẫn đỏ, mức vẫn leo, chỉ tiếng là không kêu — và người đứng cạnh pack
     lại là người duy nhất có thể nhận ra. Đo ngày 21/09: mở cổng serial làm
     đảo tắt tiếng 2/3 lần, không một dấu hiệu nào ra bên ngoài. */
  snprintf(b, sizeof(b), "%s%s%s", levelText(lvl),
           muted ? "  [COI DANG TAT TIENG]" : "",
           quiet ? "  [bip thua]" : "");
  put(0, b);

  /* Nhiệt độ. Cảm biến hỏng thì in "--" chứ KHÔNG in số cũ: thợ đang đứng cạnh
     pack và sẽ hành động theo con số này, nên số cũ mà trông như số mới là
     nguy hiểm hơn hẳn việc không có số. */
  int n = 0;
  for (int i = 0; i < AI_N_CELLS && n < (int)sizeof(b) - 12; i++) {
    n += snprintf(b + n, sizeof(b) - n, isnan(temps[i]) ? "%d:-- " : "%d:%.1f ",
                  i + 1, temps[i]);
  }
  put(1, b);

  /* Lớp 1. Lúc chưa đủ cửa sổ lịch sử thì nói thẳng là CHƯA CHẠY, không im
     lặng hiện "khong co bat thuong" — im lặng vì chưa biết và im lặng vì đã
     kiểm tra xong là hai chuyện hoàn toàn khác nhau. */
  if (!ai.valid) {
    snprintf(b, sizeof(b), "AI dang khoi dong, chua ket luan");
  } else if (ai.alarm) {
    snprintf(b, sizeof(b), "CELL %d - diem %.2f / nguong %.2f - da giu %us",
             ai.worst_cell + 1, ai.worst_score, (double)CellAI::threshold(),
             ai.run_s[ai.worst_cell]);
  } else if (ai.run_s[ai.worst_cell] > 0) {
    snprintf(b, sizeof(b), "cell %d dang vuot (%.2f), moi giu %us/%us",
             ai.worst_cell + 1, ai.worst_score, ai.run_s[ai.worst_cell],
             CellAI::persist_s());
  } else {
    snprintf(b, sizeof(b), "khong co cell bat thuong (cao nhat %.2f / %.2f)",
             ai.worst_score, (double)CellAI::threshold());
  }
  put(2, b);

  if (meter_ok) snprintf(b, sizeof(b), "%.2f V  %.3f A  SoC ~%.0f%%",
                         pack_v, pack_a, soc_pct);
  else          snprintf(b, sizeof(b), "chua do duoc (khong thay INA)");
  put(3, b);

  snprintf(b, sizeof(b), "cam bien %d/%d  wifi %s  cloud %s  chay %lu phut",
           n_healthy, AI_N_CELLS, wifi_ok ? "OK" : "mat",
           cloud_ok ? "OK" : "mat", (unsigned long)(millis() / 60000UL));
  put(4, b);
}

#endif  // USE_BLE
