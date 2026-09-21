#include "ble_view.h"

#if !USE_BLE
/* Biên dịch rỗng: không kéo Bluedroid vào, firmware y hệt bản chưa có BLE. */
bool BleView::begin(const char*) { return false; }
void BleView::update(const float*, const CellAIResult&, AlarmLevel, bool, bool,
                     bool, float, float, float, int, bool, bool) {}
bool BleView::connected() const { return false; }
int  BleView::clientCount() const { return 0; }
void BleView::onCommand(void (*)(bool), void (*)(bool)) {}
void BleView::tick() {}
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
#define UUID_DIAG  "48555449-4555-4d53-0007-000000000000"
#define UUID_CMD   "48555449-4555-4d53-0008-000000000000"

static const int N_CHAR = 6;
static BLEServer         *sServer = nullptr;
static BLECharacteristic *sCh[N_CHAR] = {nullptr};
static String             sLast[N_CHAR];

static void (*sMuteFn)(bool)  = nullptr;
static void (*sQuietFn)(bool) = nullptr;
static uint32_t sMuteUntil    = 0;      // 0 = không có hẹn giờ đang chạy

/* Đặc tính lệnh. Chỉ nhận ĐÚNG bốn chuỗi — danh sách trắng, không phân tích
   cú pháp gì cả. Không có chỗ cho lệnh lạ lọt vào, và thêm lệnh mới thì phải
   sửa đúng chỗ này nên không thể vô tình mở thêm một đường điều khiển.

   Sưởi CỐ Ý không có mặt: nó là thứ duy nhất bơm năng lượng vào pack, nên vẫn
   chỉ đi qua MQTT có tài khoản kèm công tắc chết người (QĐ-040, QĐ-042). */
class CmdCb : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) override {
    String v = c->getValue();
    v.trim();
    const char* reply = "lenh la - chi nhan: mute on|mute off|quiet on|quiet off";

    if (v == "mute on"  && sMuteFn)  {
      sMuteFn(true);
      sMuteUntil = millis() + BLE_MUTE_TTL_MS;   // tự hết hạn
      reply = "da tat tieng - TU BAT LAI sau 5 phut";
    } else if (v == "mute off" && sMuteFn)  {
      sMuteFn(false);  sMuteUntil = 0;  reply = "da bat lai tieng";
    } else if (v == "quiet on"  && sQuietFn) {
      sQuietFn(true);  reply = "da chuyen sang bip thua";
    } else if (v == "quiet off" && sQuietFn) {
      sQuietFn(false); reply = "da tat bip thua";
    }
    Serial.printf("[BLE ] lenh \"%s\" -> %s\n", v.c_str(), reply);
    c->setValue(reply);      // đọc lại đặc tính là thấy kết quả thật
  }
};
static CmdCb sCmdCb;

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

  /* Ghép đôi có mã PIN. MITM = bắt buộc nhập mã, bonding = nhớ máy đã ghép nên
     lần sau khỏi nhập lại. Không có mấy dòng này thì đặc tính lệnh ở dưới chỉ
     là một cái cửa khoá bằng dây chun.
     (Không gọi setEncryptionLevel(): hàm đó chỉ có khi backend là Bluedroid,
      mà core 3.3.11 đang chạy NimBLE. Mức mã hoá do quyền đặt trên từng
      đặc tính quyết định — xem setAccessPermissions() ở dưới.) */
  BLESecurity::setPassKey(true, BLE_PASSKEY);   // true = mã CỐ ĐỊNH, in trên nhãn
  BLESecurity::setAuthenticationMode(true, true, true);   // bonding, MITM, SC
  BLESecurity::setCapability(ESP_IO_CAP_OUT);             // thiết bị hiện mã
  BLESecurity::setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  BLESecurity::setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  sServer = BLEDevice::createServer();
  sServer->setCallbacks(&sSrvCb);

  BLEService* svc = sServer->createService(UUID_SVC);
  sCh[0] = mkChar(svc, UUID_STATE, "Trang thai");
  sCh[1] = mkChar(svc, UUID_TEMPS, "Nhiet do tung cell (degC)");
  sCh[2] = mkChar(svc, UUID_AI,    "Lop 1 - cell nghi ngo");
  sCh[3] = mkChar(svc, UUID_PACK,  "Dien ap / dong / SoC");
  sCh[4] = mkChar(svc, UUID_SYS,   "Tinh trang he thong");
  sCh[5] = mkChar(svc, UUID_DIAG,  "So do da dung de chan doan");

  /* Đặc tính DUY NHẤT ghi được, và nó đòi liên kết đã mã hoá + xác thực. Điện
     thoại chưa ghép đôi ghi vào sẽ bị từ chối ngay ở tầng GATT, không tới được
     hàm onWrite. Đây là lớp chặn số 1 của QĐ-042. */
  /* Quyền phải nằm trong THUỘC TÍNH lúc tạo, không phải qua setAccessPermissions().
     Bản đầu của tớ gọi setAccessPermissions() — mà hàm đó có thân rỗng khi
     backend là NimBLE (BLECharacteristic.cpp dòng 167: thân nằm trong
     #ifdef CONFIG_BLUEDROID_ENABLED). Tức lớp bảo vệ số 1 KHÔNG TỒN TẠI, và
     test/ble_write_check.py bắt được ngay: máy chưa ghép đôi tắt được còi.
     Đặt cả hai đường để đúng với cả hai backend. */
  BLECharacteristic* cmd = svc->createCharacteristic(
      UUID_CMD, BLECharacteristic::PROPERTY_READ
              | BLECharacteristic::PROPERTY_WRITE
              | BLECharacteristic::PROPERTY_WRITE_ENC      // đòi liên kết mã hoá
              | BLECharacteristic::PROPERTY_WRITE_AUTHEN); // đòi đã xác thực PIN
  cmd->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED |
                            ESP_GATT_PERM_WRITE_ENC_MITM);
  cmd->setCallbacks(&sCmdCb);
  cmd->setValue("mute on | mute off | quiet on | quiet off");
  { BLE2901* d = new BLE2901();
    d->setDescription("Lenh (can ghep doi) - tat tieng coi");
    cmd->addDescriptor(d); }
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

void BleView::onCommand(void (*mute_fn)(bool), void (*quiet_fn)(bool)) {
  sMuteFn = mute_fn; sQuietFn = quiet_fn;
}

/* Cho tắt tiếng từ BLE tự hết hạn. Lớp chặn số 3: im lặng không bao giờ vĩnh
   viễn, kể cả khi bị lạm dụng — và người dùng thật cũng khỏi quên bật lại,
   đúng cái bẫy đã gặp ngày 21/09 (tắt tiếng còn nguyên sang lần demo sau). */
void BleView::tick() {
  if (!sMuteUntil || (int32_t)(sMuteUntil - millis()) > 0) return;
  sMuteUntil = 0;
  if (sMuteFn) { sMuteFn(false); Serial.println("[BLE ] het han tat tieng - coi bat lai"); }
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
  char b[256];

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
    /* Kèm DẠNG và VIỆC PHẢI LÀM, không chỉ con số. Thợ đứng cạnh pack không
       tra được bảng TH-1/2/3 từ một con số điểm; mà phân biệt TH-1 với TH-2
       chính là khác biệt giữa "cách ly pack ngay" và "ghi sổ, mai kiểm". */
    snprintf(b, sizeof(b), "CELL %d - %s | %s | diem %.2f/%.2f, giu %us",
             ai.worst_cell + 1, aiPatternName(ai.pattern),
             aiPatternAction(ai.pattern), ai.worst_score,
             (double)CellAI::threshold(), ai.run_s[ai.worst_cell]);
  } else if (ai.run_s[ai.worst_cell] > 0) {
    snprintf(b, sizeof(b), "cell %d dang vuot (%.2f) - %s - moi giu %us/%us",
             ai.worst_cell + 1, ai.worst_score, aiPatternName(ai.pattern),
             ai.run_s[ai.worst_cell], CellAI::persist_s());
  } else {
    snprintf(b, sizeof(b), "khong co cell bat thuong (cao nhat %.2f / %.2f)",
             ai.worst_score, (double)CellAI::threshold());
  }
  put(2, b);

  if (meter_ok) snprintf(b, sizeof(b), "%.2f V  %.3f A  SoC ~%.0f%%",
                         pack_v, pack_a, soc_pct);
  else          snprintf(b, sizeof(b), "chua do duoc (khong thay INA)");
  put(3, b);

  /* Ba con số đã dùng để tra ra DẠNG ở trên. Đưa ra ngoài để thợ KIỂM được
     lời khuyên thay vì phải tin nó — một lời khuyên không kiểm được thì đến
     lúc nó sai sẽ không ai phát hiện. */
  snprintf(b, sizeof(b), "lech %+.2f degC | nhanh hon pack %+.2f degC/phut | "
           "dot ngot %+.2f degC", ai.dev, ai.dt_diff, ai.shock);
  put(5, ai.valid ? b : "chua co so (AI dang khoi dong)");

  snprintf(b, sizeof(b), "cam bien %d/%d  wifi %s  cloud %s  chay %lu phut",
           n_healthy, AI_N_CELLS, wifi_ok ? "OK" : "mat",
           cloud_ok ? "OK" : "mat", (unsigned long)(millis() / 60000UL));
  put(4, b);
}

#endif  // USE_BLE
