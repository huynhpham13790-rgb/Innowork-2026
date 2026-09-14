/* =============================================================================
 *  Đội Hủ Tiếu — AIoT InnoWorks 2026
 *  Test đường truyền: ESP32-S3-DevKitC-1 N16R8  ->  WISE-IoT (EdgeSync360/EdgeHub)
 *
 *  Chạy theo 2 GIAI ĐOẠN, đổi bằng #define STAGE bên dưới:
 *    STAGE 1 : bắn lên broker công cộng (HiveMQ). KHÔNG cần tài khoản WISE-IoT.
 *              -> Chứng minh: WiFi OK, MQTT OK, JSON đúng format, timestamp đúng.
 *    STAGE 2 : bắn lên WISE-IoT IoT Hub thật. Chỉ đổi thông tin đăng nhập,
 *              KHÔNG phải sửa logic. Nếu STAGE 1 chạy mà STAGE 2 hỏng
 *              => lỗi nằm ở credential/tenant, không phải ở code.
 *
 *  Dữ liệu: 8 nhiệt độ giả lập (pack 8S). Cell #5 được cho nóng dần lên
 *  để có sẵn một "bất thường" nhìn thấy trên dashboard.
 *
 *  Thư viện cần cài (Arduino IDE > Library Manager):
 *    - PubSubClient   (Nick O'Leary)
 *    - ArduinoJson    (Benoit Blanchon, v7)
 *  Board: "ESP32S3 Dev Module"  (esp32 by Espressif >= 3.0)
 * ============================================================================= */

#define STAGE 1          // <<<<<< ĐỔI 1 -> 2 khi đã có credential WISE-IoT

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <time.h>
#include "cell_ai.h"
#include "charge_cycle.h"
#include "cell_temp.h"

// ---------------------------------------------------------------- Nguồn nhiệt độ
// 1 = đọc 8 con DS18B20 thật · 0 = quay lại giả lập (khi tháo cảm biến ra)
// Giữ đường lùi này vì ngày thi mà dây tuột thì vẫn còn cái để diễn.
#define USE_REAL_TEMP 1
#define TEMP_PIN      4      // bus 1-Wire, khớp với các sketch trong test/

// ---------------------------------------------------------------- WiFi
const char* WIFI_SSID = "ICTU";
const char* WIFI_PASS = "";        // mạng mở, không mật khẩu

// Mạng mở thì phải gọi WiFi.begin(ssid) một tham số. Truyền chuỗi rỗng làm
// tham số mật khẩu khiến driver vẫn thương lượng theo kiểu có mã hoá và hỏng
// im lặng — nối mãi không được mà không báo lỗi gì.
static inline void wifiStart() {
  if (WIFI_PASS && WIFI_PASS[0]) WiFi.begin(WIFI_SSID, WIFI_PASS);
  else                           WiFi.begin(WIFI_SSID);
}
// LƯU Ý: ESP32-S3 chỉ bắt WiFi 2.4GHz. Wifi 5GHz sẽ không hiện/không nối được.

// ---------------------------------------------------------------- STAGE 2: WISE-IoT
// Cách A (khuyến nghị): dán Credential Key lấy từ portal, ESP tự gọi DCCS lấy
// host/port/user/pass. Đây đúng là việc mà EdgeAgent SDK làm bên trong.
#define WISE_USE_DCCS   1
const char* DCCS_API_URL  = "https://api-dccs-ensaas.education.wise-paas.com";
const char* DCCS_CRED_KEY = "DAN_CREDENTIAL_KEY_VAO_DAY";

// Cách B: nhập tay nếu portal cho thẳng thông tin broker
const char* MQTT_HOST_MANUAL = "";
const int   MQTT_PORT_MANUAL = 8883;
const char* MQTT_USER_MANUAL = "";
const char* MQTT_PASS_MANUAL = "";

// nodeId lấy ở portal (EdgeHub > Node). deviceId do mình tự đặt.
const char* NODE_ID   = "DAN_NODE_ID_VAO_DAY";   // dạng UUID
const char* DEVICE_ID = "BatteryPack01";

// ---------------------------------------------------------------- STAGE 1: broker riêng / công cộng
// Mặc định: broker công cộng HiveMQ, không cần tài khoản.
// PLAN B: đổi TEST_HOST thành IP server của đội + điền TEST_USER/TEST_PASS.
//   ví dụ: TEST_HOST = "203.0.113.45";  TEST_USER = "hutieu";  TEST_PASS = "matkhau";
// Để TEST_USER = "" thì nối ẩn danh (dùng cho HiveMQ công cộng).
// Broker của đội (Plan B). IP phải khớp LAN_IP trong planb_cloud/.env — ĐỔI
// MỖI KHI ĐỔI MẠNG. Xem IP máy chủ bằng: hostname -I
const char* TEST_HOST = "172.172.3.174";
const int   TEST_PORT = 1883;

// Mật khẩu KHÔNG nằm trong file này — nó bị commit lên GitHub.
// Xem arduino_secrets.example.h để biết cách tạo arduino_secrets.h.
#include "arduino_secrets.h"
const char* TEST_USER = SECRET_MQTT_USER;
const char* TEST_PASS = SECRET_MQTT_PASS;

// ---------------------------------------------------------------- Cấu hình chung
const int   NUM_CELLS      = 8;
const long  PUBLISH_MS     = 2000;    // 0.5 Hz cho lúc test
const long  HEARTBEAT_MS   = 60000;   // đúng mặc định của EdgeAgent

// ---------------------------------------------------------------- Store-and-forward
// Mất mạng thì gói dữ liệu được ghi xuống flash, nối lại thì đẩy bù lên theo
// đúng thứ tự, giữ nguyên `ts` gốc. Đây là màn 90 giây trong kịch bản demo:
// rút mạng -> ESP đệm -> cắm lại -> dữ liệu bù về đúng vị trí thời gian.
const char* SPOOL_PATH      = "/spool.jsonl";   // mỗi dòng là 1 payload JSON
const char* SPOOL_TMP_PATH  = "/spool.tmp";
const size_t SPOOL_MAX_BYTES = 512UL * 1024UL;  // ~1.5 ngày ở nhịp 2s; thừa cho demo
const int   FLUSH_BATCH      = 25;              // đẩy bù mỗi lượt bấy nhiêu gói,
                                                // rồi trả quyền cho loop() -> không nghẽn
const long  RECONNECT_MS     = 5000;            // nhịp thử nối lại, KHÔNG chặn loop

// Sau khi nối lại, ĐỪNG tin đường truyền ngay. Lý do đã trả giá bằng một lần
// test hỏng: khi broker vừa sống lại, ESP32 nối lại sau 5s nhưng phía subscriber
// (Node-RED) mất tới 15s mới nối lại. ESP bắn nguyên buffer vào broker chưa có
// ai nghe -> MQTT QoS 0 không lưu cho subscriber offline -> mất sạch, mà log
// phía ESP vẫn báo "đẩy bù OK". Nên trong khoảng này vẫn ghi tiếp vào spool,
// hết khoảng mới đẩy bù một lượt. Đổi lại: dữ liệu lên chậm hơn vài chục giây.
const long  LINK_GRACE_MS    = 20000;

// -----------------------------------------------------------------------------
WiFiClient        netPlain;
WiFiClientSecure  netTls;
PubSubClient      mqtt;

String  gHost, gUser, gPass;
int     gPort = 1883;
bool    gUseTls = false;

char topicData[160], topicConn[160], topicCfg[160], topicCmd[160], topicAck[160];

unsigned long lastPublish = 0, lastHeartbeat = 0, lastReconnect = 0, lastAi = 0;
const long AI_PERIOD_MS = 1000;   // AI chạy đúng 1 Hz — đặc trưng phụ thuộc nhịp này
unsigned long linkTrustedAt = 0;   // trước mốc này thì vẫn đệm, chưa đẩy bù
unsigned long spoolDropped = 0;   // số gói bị bỏ vì flash đầy (báo cho biết là có mất)
bool   fsReady = false;
float cellBias[NUM_CELLS];
float faultRise = 0.0f;   // độ nóng cộng dồn của cell lỗi
const int FAULT_CELL = 5; // cell "hỏng" (1-based)

// ---------------------------------------------------------------- Lớp 1: AI on-device
// Autoencoder phát hiện cell bất thường. Trọng số nằm trong cell_ae_weights.h,
// sinh ra từ ai/export_c_model.py. Ngưỡng và quy tắc giữ liên tục cũng ở đó.
void publishAnomaly(const CellAIResult& r);      // định nghĩa bên dưới
void publishCycleSummary(const ChargeSummary& s);
CellAI gAI;

// ---------------------------------------------------------------- Lớp 2: tóm tắt chu kỳ sạc
// ESP32 KHÔNG chạy mô hình RUL. Nó chỉ quan sát quá trình sạc rồi gửi 9 con số
// lên cloud; Node-RED nhân với hệ số đã train ra RUL/SOH. Xem charge_cycle.h.
ChargeCycle gCharge;
int  gSimCycle = 0;                 // số chu kỳ sạc đã MÔ PHỎNG
const long SIM_CHARGE_EVERY_MS = 45000;   // cứ 45s lại mô phỏng 1 chu kỳ sạc
unsigned long lastSimCharge = 0;
float  gCellTemp[AI_N_CELLS];     // nhiệt 8 cell của bước hiện tại
bool   gAlarmLatched = false;     // đã báo động rồi thì không spam lại
int    gAlarmCell    = -1;

// ---------------------------------------------------------------- Cảm biến thật
CellTemp gTemp;
bool     gTempOk = false;         // begin() có đủ 8 cảm biến không
unsigned long lastTempPrint = 0;
const long TEMP_PRINT_MS = 30000;
void publishSensorHealth();

/* Nạp gCellTemp[] cho bước hiện tại. Tách riêng để runAI() và publishData()
   dùng CHUNG một bộ số — bản cũ mỗi hàm tự sinh số ngẫu nhiên riêng, nên nhiệt
   độ đẩy lên dashboard không phải nhiệt độ mà AI nhìn thấy. Với dữ liệu giả
   thì không ai để ý; với dữ liệu thật thì đó là hai sự thật mâu thuẫn nhau. */
void sampleCellTemps() {
#if USE_REAL_TEMP
  if (gTemp.status().ready) {
    const float* t = gTemp.temps();
    for (int i = 0; i < AI_N_CELLS; i++) gCellTemp[i] = t[i];
    return;
  }
  // Chưa có lượt đọc nào (vài giây đầu) — để nguyên, runAI() sẽ tự bỏ qua.
  for (int i = 0; i < AI_N_CELLS; i++) gCellTemp[i] = NAN;
#else
  faultRise += 0.05f;
  if (faultRise > 18.0f) faultRise = 0.0f;   // reset để demo lặp lại
  for (int i = 0; i < AI_N_CELLS; i++) {
    float v = 30.0f + cellBias[i] + random(-30, 31) / 100.0f;
    if (i + 1 == FAULT_CELL) v += faultRise;
    gCellTemp[i] = roundf(v * 16.0f) / 16.0f;   // bước 0,0625 °C như DS18B20
  }
#endif
}

// ============================================================ Tiện ích thời gian
// EdgeHub yêu cầu ISO-8601 UTC, đúng format "%Y-%m-%dT%H:%M:%S.%fZ" (6 số lẻ giây).
String isoTimestampUtc() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  struct tm t;
  gmtime_r(&tv.tv_sec, &t);
  char buf[40];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d.%06ldZ",
           t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
           t.tm_hour, t.tm_min, t.tm_sec, (long)tv.tv_usec);
  return String(buf);
}

// Giờ đã hợp lệ chưa? Nếu NTP chưa xong, ts sẽ ra năm 1970 và cloud vứt dữ liệu
// đi (InfluxDB trả HTTP 422 vì ngoài retention). Thà không gửi còn hơn gửi rác.
bool timeIsValid() {
  time_t now = 0;
  time(&now);
  return now >= 1700000000;
}

void syncTime() {
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  Serial.print("[NTP] dong bo gio");
  time_t now = 0;
  int tries = 0;
  while (now < 1700000000 && tries < 40) {   // chờ tới khi gio hop le
    delay(500); Serial.print(".");
    time(&now); tries++;
  }
  Serial.println(now < 1700000000 ? " THAT BAI" : " OK -> " + isoTimestampUtc());
  // Nếu NTP fail: timestamp sẽ sai năm 1970, WISE-IoT sẽ vứt dữ liệu đi
  // hoặc vẽ điểm ở tận 1970 trên dashboard. Đây là lỗi hay gặp nhất.
}

// ============================================================ Store-and-forward
void spoolBegin() {
  fsReady = LittleFS.begin(true);        // true = tự format nếu chưa có filesystem
  if (!fsReady) { Serial.println("[FS  ] LittleFS hong -> mat mang la mat du lieu"); return; }
  File f = LittleFS.open(SPOOL_PATH, FILE_READ);
  size_t sz = f ? f.size() : 0;
  if (f) f.close();
  Serial.printf("[FS  ] LittleFS OK, spool dang co %u byte\n", (unsigned)sz);
}

// Flash gần đầy thì bỏ nửa cũ đi, giữ nửa mới. Dữ liệu mới đáng giá hơn dữ liệu
// cũ trong bài toán giám sát pin, và cách này tránh phải viết ring buffer thật.
void spoolCompact() {
  File in = LittleFS.open(SPOOL_PATH, FILE_READ);
  if (!in) return;
  size_t half = in.size() / 2;
  in.seek(half);
  in.readStringUntil('\n');              // bỏ nốt dòng bị cắt dở
  File out = LittleFS.open(SPOOL_TMP_PATH, FILE_WRITE);
  if (!out) { in.close(); return; }
  while (in.available()) {
    String line = in.readStringUntil('\n');
    if (line.length()) { out.println(line); }
  }
  in.close(); out.close();
  LittleFS.remove(SPOOL_PATH);
  LittleFS.rename(SPOOL_TMP_PATH, SPOOL_PATH);
  Serial.println("[FS  ] spool day -> da bo nua cu");
}

void spoolAppend(const String& payload) {
  if (!fsReady) { spoolDropped++; return; }
  File f = LittleFS.open(SPOOL_PATH, FILE_APPEND);
  if (!f) { spoolDropped++; return; }
  if (f.size() > SPOOL_MAX_BYTES) { f.close(); spoolCompact(); f = LittleFS.open(SPOOL_PATH, FILE_APPEND); }
  if (!f) { spoolDropped++; return; }
  f.println(payload);
  f.close();
}

size_t spoolSize() {
  if (!fsReady) return 0;
  File f = LittleFS.open(SPOOL_PATH, FILE_READ);
  size_t sz = f ? f.size() : 0;
  if (f) f.close();
  return sz;
}

// Đẩy bù tối đa FLUSH_BATCH gói. Gói nào chưa gửi được thì giữ nguyên trong file
// theo đúng thứ tự, nên rớt mạng giữa chừng cũng không mất và không đảo thứ tự.
// Trả về true khi spool đã sạch.
bool spoolFlush() {
  if (millis() < linkTrustedAt) return false;      // còn trong khoảng chờ subscriber
  if (!fsReady || spoolSize() == 0) return true;

  File in = LittleFS.open(SPOOL_PATH, FILE_READ);
  if (!in) return true;
  File out = LittleFS.open(SPOOL_TMP_PATH, FILE_WRITE);
  if (!out) { in.close(); return false; }

  int sent = 0, kept = 0;
  bool stillSending = true;
  while (in.available()) {
    String line = in.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    if (stillSending && sent < FLUSH_BATCH && mqtt.connected()) {
      if (mqtt.publish(topicData, line.c_str())) { sent++; continue; }
      stillSending = false;              // gửi hỏng -> phần còn lại giữ lại hết
    } else if (sent >= FLUSH_BATCH) {
      stillSending = false;              // hết lượt, để dành cho vòng loop sau
    }
    out.println(line);
    kept++;
  }
  in.close(); out.close();
  LittleFS.remove(SPOOL_PATH);
  LittleFS.rename(SPOOL_TMP_PATH, SPOOL_PATH);

  if (sent) Serial.printf("[BUFF] day bu %d goi, con lai %d\n", sent, kept);
  return kept == 0;
}

// ============================================================ WiFi
// KHÔNG chặn. Nếu chặn ở đây thì lúc mất mạng loop() đứng luôn, không lấy mẫu
// được và store-and-forward thành vô nghĩa — đó chính là bug của bản trước.
void ensureWifi() {
  static bool announced = false;
  if (WiFi.status() == WL_CONNECTED) {
    if (!announced) {
      Serial.println("[WiFi] OK  IP=" + WiFi.localIP().toString() +
                     "  RSSI=" + String(WiFi.RSSI()) + "dBm");
      announced = true;
    }
    return;
  }
  announced = false;
  static unsigned long lastTry = 0;
  if (millis() - lastTry < RECONNECT_MS) return;
  lastTry = millis();
  Serial.printf("[WiFi] chua co mang, thu noi lai %s ...\n", WIFI_SSID);
  WiFi.disconnect();
  wifiStart();
}

// ============================================================ DCCS
// GET {apiUrl}/v1/serviceCredentials/{credentialKey}
// Trả về serviceHost + credential.protocols.mqtt / mqtt+ssl {port,username,password}
bool fetchCredentialFromDccs() {
  WiFiClientSecure https;
  https.setInsecure();                 // bỏ qua kiểm tra CA cho lần test đầu
  HTTPClient http;
  String url = String(DCCS_API_URL) + "/v1/serviceCredentials/" + DCCS_CRED_KEY;
  Serial.println("[DCCS] GET " + url);

  if (!http.begin(https, url)) { Serial.println("[DCCS] http.begin that bai"); return false; }
  int code = http.GET();
  if (code != 200) {
    Serial.printf("[DCCS] HTTP %d - kiem tra lai API URL va Credential Key\n", code);
    Serial.println(http.getString());
    http.end();
    return false;
  }
  String body = http.getString();
  http.end();

  JsonDocument doc;
  if (deserializeJson(doc, body)) { Serial.println("[DCCS] JSON hong"); return false; }

  gHost = doc["serviceHost"].as<String>();
  JsonObject p = gUseTls ? doc["credential"]["protocols"]["mqtt+ssl"]
                         : doc["credential"]["protocols"]["mqtt"];
  if (p.isNull()) { Serial.println("[DCCS] khong co protocol tuong ung"); return false; }

  gPort = p["port"] | 0;
  gUser = p["username"].as<String>();
  gPass = p["password"].as<String>();

  Serial.printf("[DCCS] host=%s port=%d user=%s\n", gHost.c_str(), gPort, gUser.c_str());
  return gPort != 0;
}

// ============================================================ MQTT
void buildTopics() {
  snprintf(topicData, sizeof(topicData), "/wisepaas/scada/%s/data", NODE_ID);
  snprintf(topicConn, sizeof(topicConn), "/wisepaas/scada/%s/conn", NODE_ID);
  snprintf(topicCfg,  sizeof(topicCfg),  "/wisepaas/scada/%s/cfg",  NODE_ID);
  snprintf(topicCmd,  sizeof(topicCmd),  "/wisepaas/scada/%s/cmd",  NODE_ID);
  snprintf(topicAck,  sizeof(topicAck),  "/wisepaas/scada/%s/ack",  NODE_ID);
}

void onMqttMessage(char* topic, byte* payload, unsigned int len) {
  Serial.printf("[MQTT<-] %s : ", topic);
  for (unsigned int i = 0; i < len; i++) Serial.write(payload[i]);
  Serial.println();
}

// Gửi Config: khai báo Node + Device + 8 tag analog. Chỉ cần gửi 1 lần sau khi
// kết nối; không có bước này WISE-IoT sẽ không biết tag nào tồn tại và
// dashboard sẽ trống dù dữ liệu vẫn về.
void publishConfig() {
  JsonDocument doc;
  JsonObject d     = doc["d"].to<JsonObject>();
  d["Action"]      = 1;                               // 1 = Create
  JsonObject scada = d["Scada"].to<JsonObject>();
  JsonObject node  = scada[NODE_ID].to<JsonObject>();
  node["Type"]     = 0;                               // 0 = Gateway
  node["Hbt"]      = HEARTBEAT_MS / 1000;
  JsonObject dev   = node["Device"][DEVICE_ID].to<JsonObject>();
  dev["Name"]      = "Pack pin 8S";
  dev["Type"]      = "BatteryPack";
  dev["Desc"]      = "Pack thi nghiem 8 cell LG HG2";
  JsonObject tags  = dev["Tag"].to<JsonObject>();

  for (int i = 1; i <= NUM_CELLS; i++) {
    char name[16]; snprintf(name, sizeof(name), "Cell%02d_Temp", i);
    JsonObject t = tags[name].to<JsonObject>();
    t["Type"] = 1;          // 1 = Analog
    t["Desc"] = "Nhiet do cell";
    t["RO"]   = 1;          // read only
    t["Ary"]  = 0;
    t["SH"]   = 80;         // span high
    t["SL"]   = 0;          // span low
    t["EU"]   = "degC";
    t["IDF"]  = 3;
    t["FDF"]  = 2;
  }
  doc["ts"] = isoTimestampUtc();

  String out; serializeJson(doc, out);
  bool ok = mqtt.publish(topicCfg, out.c_str());
  Serial.printf("[CFG ] %d bytes -> %s\n", out.length(), ok ? "OK" : "THAT BAI (tang buffer?)");
}

void publishConnState(int con) {
  JsonDocument doc;
  doc["d"][con ? "Con" : "DsC"] = 1;
  doc["ts"] = isoTimestampUtc();
  String out; serializeJson(doc, out);
  mqtt.publish(topicConn, (const uint8_t*)out.c_str(), out.length(), true); // retained
}

void publishHeartbeat() {
  JsonDocument doc;
  doc["d"]["Hbt"] = 1;
  doc["ts"] = isoTimestampUtc();
  String out; serializeJson(doc, out);
  mqtt.publish(topicConn, out.c_str());
  Serial.println("[HBT ] .");
}

// Payload dữ liệu — đây là thứ quyết định dashboard có vẽ được hay không:
// { "d": { "<deviceId>": { "<tag>": <so>, ... } }, "ts": "...Z" }
// ============================================================ Lớp 1: AI on-device
// Chạy 1 Hz. Đọc nhiệt 8 cell (hiện đang giả lập — khi gắn DS18B20 thật thì
// chỉ thay chỗ đọc), cho qua autoencoder, và báo động nếu một cell vượt ngưỡng
// đủ lâu.
//
// LƯU Ý AN TOÀN: đây là lớp phát hiện SỚM cell bất thường, KHÔNG phải bộ dự
// báo cháy nổ. Nó bỏ sót kiểu trôi nhiệt rất chậm (xem ai/README.md), nên
// ngưỡng cứng 60 °C bên dưới PHẢI giữ, không được bỏ đi vì "đã có AI".
void runAI() {
  sampleCellTemps();
  // Chưa có số đọc hợp lệ (vài giây đầu, hoặc mất cả bus) thì không cho AI ăn
  // số rác. Thà không có kết quả còn hơn có kết quả sai.
  if (isnan(gCellTemp[0])) return;

  // ⚠️ Ba con số dưới đây VẪN LÀ GIẢ ĐỊNH — chưa có phần cứng đo.
  // ambient: cần một con DS18B20 thứ 9 đo môi trường (8 con hiện có đều dán
  //   lên cell). Lấy hằng số là chấp nhận được vì đặc trưng Lớp 1 chủ yếu là
  //   tương đối giữa các cell, nhưng phải nói rõ khi trình bày.
  // current/soc: cần INA228 + đo điện áp pack.
  const float ambient = 28.0f;
  const float current = 0.0f;
  const float soc     = 80.0f;

  CellAIResult r = gAI.update(gCellTemp, ambient, current, soc);
  if (!r.valid) return;           // còn trong giai đoạn khởi động

  // --- lớp an toàn độc lập với AI: ngưỡng cứng, không bao giờ được bỏ ---
  for (int i = 0; i < AI_N_CELLS; i++) {
    if (gCellTemp[i] >= 60.0f) {
      Serial.printf("[SAFE] CELL %d = %.2f degC >= 60 - NGUONG CUNG\n", i + 1, gCellTemp[i]);
    }
  }

  if (r.alarm && !gAlarmLatched) {
    gAlarmLatched = true;
    gAlarmCell = r.worst_cell;
    Serial.printf("[AI  ] *** BAT THUONG *** cell %d, diem %.3f (nguong %.3f), "
                  "giu %us\n", r.worst_cell + 1, r.worst_score,
                  (double)CellAI::threshold(), r.run_s[r.worst_cell]);
    publishAnomaly(r);
  } else if (!r.alarm && gAlarmLatched) {
    gAlarmLatched = false;
    Serial.println("[AI  ] het bat thuong");
    publishAnomaly(r);
  }
}

// Đẩy kết quả AI lên cloud theo ĐÚNG data contract hiện có: vẫn là
// {"d":{"<device>":{...}},"ts":...} nên Node-RED/WISE-IoT không phải sửa gì.
// Xem docs/DATA_CONTRACT.md — giá trị phải là số, nên trạng thái mã hoá bằng 0/1.
void publishAnomaly(const CellAIResult& r) {
  if (!timeIsValid()) return;
  JsonDocument doc;
  JsonObject dev = doc["d"][DEVICE_ID].to<JsonObject>();
  dev["AI_Alarm"]      = r.alarm ? 1 : 0;
  dev["AI_WorstCell"]  = r.worst_cell + 1;         // 1-based cho người đọc
  dev["AI_WorstScore"] = round(r.worst_score * 1000) / 1000.0;
  for (int i = 0; i < AI_N_CELLS; i++) {
    char name[16]; snprintf(name, sizeof(name), "AI_Score%02d", i + 1);
    dev[name] = round(r.score[i] * 1000) / 1000.0;
  }
  doc["ts"] = isoTimestampUtc();

  String out; serializeJson(doc, out);
  bool linkReady = mqtt.connected() && millis() >= linkTrustedAt;
  if (!(linkReady && mqtt.publish(topicData, out.c_str()))) {
    spoolAppend(out);        // mất mạng thì cảnh báo cũng phải được đệm lại
  }
  Serial.printf("[AI  ] gui ket qua: %s\n", out.c_str());
}

// ============================================================ Lớp 2: chu kỳ sạc
// Gửi 9 con số tóm tắt một chu kỳ sạc. Vẫn đi qua ĐÚNG data contract hiện có
// (docs/DATA_CONTRACT.md) nên Node-RED và WISE-IoT không phải sửa gì về đường
// truyền — chỉ thêm một nhánh xử lý khi thấy tag CYC_*.
void publishCycleSummary(const ChargeSummary& s) {
  if (!timeIsValid()) return;
  JsonDocument doc;
  JsonObject dev = doc["d"][DEVICE_ID].to<JsonObject>();
  for (int i = 0; i < CC_N_FEATURES; i++) {
    char name[24];
    snprintf(name, sizeof(name), "CYC_%s", CC_FEATURE_NAMES[i]);
    dev[name] = round(s.f[i] * 10000) / 10000.0;
  }
  dev["CYC_duration_s"] = (double)s.duration_s;
  doc["ts"] = isoTimestampUtc();

  String out; serializeJson(doc, out);
  bool linkReady = mqtt.connected() && millis() >= linkTrustedAt;
  if (!(linkReady && mqtt.publish(topicData, out.c_str()))) spoolAppend(out);
  Serial.printf("[CYC ] chu ky sac #%d xong sau %us ao -> gui %d dac trung\n",
                gSimCycle, s.duration_s, CC_N_FEATURES);
}

/* Đẩy sức khoẻ cảm biến lên cloud (QĐ-024).
   Vẫn đúng data contract: {"d":{"<device>":{...}},"ts":...}, chỉ thêm tag mới,
   giá trị đều là số. Node-RED/WISE-IoT không phải sửa gì.

   Vì sao đáng một gói riêng: đã đo được bus lỗi 35 % mà không có dấu hiệu nào,
   và đã gặp cảnh tuột một dây mà chương trình vẫn chạy với 6 cảm biến. Nếu
   dashboard chỉ có 8 đường nhiệt độ thì không ai nhận ra — các đường vẫn đẹp,
   chỉ là chúng không còn là sự thật. */
void publishSensorHealth() {
  if (!timeIsValid()) return;
  JsonDocument doc;
  JsonObject dev = doc["d"][DEVICE_ID].to<JsonObject>();

  const CellTempStatus& s = gTemp.status();
  dev["Sensor_Healthy"]   = s.n_healthy;          // phải luôn bằng 8
  dev["Sensor_Found"]     = s.n_found;
  dev["Sensor_ErrPct"]    = round(gTemp.errorRate() * 1000) / 1000.0;
  // Bitmask kênh hỏng: 0 là mọi thứ bình thường, khác 0 là có vấn đề.
  uint8_t mask = 0;
  for (int i = 0; i < CT_N; i++) if (!s.healthy[i]) mask |= (1 << i);
  dev["Sensor_FaultMask"] = mask;

  doc["ts"] = isoTimestampUtc();
  String out; serializeJson(doc, out);
  bool linkReady = mqtt.connected() && millis() >= linkTrustedAt;
  if (!(linkReady && mqtt.publish(topicData, out.c_str()))) spoolAppend(out);
}

// ------------------------------------------------------------ MÔ PHỎNG SẠC
// TẠM THỜI: chưa có mạch sạc thật nên sinh một chu kỳ CC-CV tổng hợp, chạy ở
// "thời gian ảo" (mỗi vòng lặp = 1 giây ảo) để một chu kỳ 3 tiếng gói gọn
// trong vài giây thật. Khi có phần cứng thật thì bỏ hàm này và gọi
// gCharge.update() 1 Hz với số đo thật từ INA228 + ADC + DS18B20.
//
// Pin được cho "già đi" theo từng chu kỳ mô phỏng: pha CV dài dần ra, đúng
// quy luật vật lý mà mô hình đã học -> RUL trên dashboard sẽ giảm dần.
void simulateChargeCycle() {
  gSimCycle++;
  // Hằng số HIỆU CHỈNH theo dải thật của bộ NASA mà mô hình đã học:
  // t_cv chạy từ ~2470 giây (pin mới) tới ~3170 giây (pin sắp hết đời).
  // Với profile CV suy giảm mũ thì t_cv = tau * ln(I0/I_END) = tau * 2,015,
  // nên tau phải nằm trong khoảng 1224..1573.
  // Không hiệu chỉnh thì đầu vào rơi ngoài dải huấn luyện và RUL ra số âm —
  // đã gặp đúng lỗi đó ở lần chạy đầu, mô hình trả -6 chu kỳ ngay từ chu kỳ 1.
  // Pha CC chia 3 đoạn, KHÔNG dâng tuyến tính. Pin lithium có vùng điện áp
  // phẳng ở giữa: lên nhanh tới ~3,9 V, bò rất chậm qua 3,9–4,15 V, rồi lên
  // nốt. Bản đầu cho dâng tuyến tính -> t_v_interval và dvdt_cc rơi ngoài dải
  // huấn luyện, và chính node cảnh báo ngoại suy bên Node-RED đã bắt được.
  // Các con số dưới đây khớp trung bình của bộ NASA (xem ai/data/nasa_cycles.csv).
  const float t_a = 300.0f;       // 3,60 -> 3,90 V
  const float t_b = 2074.0f;      // 3,90 -> 4,15 V  (NASA: t_v_interval TB 2074s)
  const float t_c = 379.0f;       // 4,15 -> 4,20 V
  const float t_cc = t_a + t_b + t_c;
  const float tau_cv = 1224.0f + 18.0f * gSimCycle;     // CV dài dần khi pin già
  const float I0 = 1.5f;

  for (int t = 0; t < 20000; t++) {
    float v_cell, i;
    if (t < t_a)              { v_cell = 3.60f + 0.30f * (t / t_a);            i = I0; }
    else if (t < t_a + t_b)   { v_cell = 3.90f + 0.25f * ((t - t_a) / t_b);    i = I0; }
    else if (t < t_cc)        { v_cell = 4.15f + 0.05f * ((t - t_a - t_b) / t_c); i = I0; }
    else {                                              // CV: giữ áp, dòng giảm mũ
      v_cell = 4.20f;
      i = I0 * expf(-(t - t_cc) / tau_cv);
    }
    // Nhiệt độ: hiệu chỉnh để T_mean rơi quanh 26,7 °C như bộ NASA
    const float temp = 25.5f + 1.8f * (i / I0);
    ChargeSummary s = gCharge.update(v_cell * CC_N_CELLS, i, temp);
    if (s.valid) { publishCycleSummary(s); return; }
    if (i < CC_I_END && t > t_cc) break;
  }
  Serial.println("[CYC ] mo phong khong ra summary hop le");
}

void publishData() {
  // Chưa có giờ chuẩn thì gói nào gửi lên cũng bị cloud vứt. Bỏ qua lượt này.
  if (!timeIsValid()) { Serial.println("[DATA] bo qua - NTP chua dong bo"); return; }

  // Chưa có số đọc thật thì không gửi gì. Gửi số bịa lên dashboard rồi sau đó
  // giải thích "lúc đó cảm biến chưa sẵn sàng" là cách nhanh nhất để mất tin.
  if (isnan(gCellTemp[0])) { Serial.println("[DATA] bo qua - cam bien chua san sang"); return; }

  JsonDocument doc;
  JsonObject dev = doc["d"][DEVICE_ID].to<JsonObject>();

  // Dùng đúng bộ số mà runAI() vừa nhìn thấy — xem sampleCellTemps().
  for (int i = 1; i <= NUM_CELLS; i++) {
    char name[16]; snprintf(name, sizeof(name), "Cell%02d_Temp", i);
    dev[name] = round(gCellTemp[i - 1] * 100) / 100.0;
  }
  doc["ts"] = isoTimestampUtc();

  String out; serializeJson(doc, out);

  // Còn mạng thì gửi thẳng; mất mạng (hoặc gửi hỏng) thì ghi xuống flash để
  // đẩy bù sau. `ts` đã nằm sẵn trong payload nên gói đệm giữ đúng thời điểm
  // lấy mẫu, không phải thời điểm đẩy lên.
  bool linkReady = mqtt.connected() && millis() >= linkTrustedAt;
  bool ok = linkReady && mqtt.publish(topicData, out.c_str());
  if (ok) {
    Serial.printf("[DATA] OK   %s\n", out.c_str());
  } else {
    spoolAppend(out);
    Serial.printf("[DATA] DEM  (spool %u byte) %s\n", (unsigned)spoolSize(), out.c_str());
  }
}

// MỘT lần thử nối, không chặn. Gọi lại theo nhịp RECONNECT_MS từ loop().
bool mqttTryConnect() {
    // Last Will: nếu ESP mất điện/rớt mạng, broker tự báo cho cloud biết.
    JsonDocument will;
    will["d"]["UeD"] = 1;
    will["ts"] = isoTimestampUtc();
    String willPayload; serializeJson(will, willPayload);

    String clientId = "HuTieu-S3-" + String((uint32_t)ESP.getEfuseMac(), HEX);

    Serial.printf("[MQTT] noi %s:%d ...\n", gHost.c_str(), gPort);
    bool ok;
#if STAGE == 2
    ok = mqtt.connect(clientId.c_str(), gUser.c_str(), gPass.c_str(),
                      topicConn, 0, true, willPayload.c_str());
#else
    if (strlen(TEST_USER) > 0) {
      // Broker riêng của đội (Plan B) — có tài khoản, và đăng ký luôn Last Will
      ok = mqtt.connect(clientId.c_str(), TEST_USER, TEST_PASS,
                        topicConn, 0, true, willPayload.c_str());
    } else {
      // Broker công cộng — nối ẩn danh
      ok = mqtt.connect(clientId.c_str(), nullptr, nullptr,
                        topicConn, 0, true, willPayload.c_str());
    }
#endif
    if (ok) {
      Serial.println("[MQTT] KET NOI OK");
      mqtt.subscribe(topicCmd);
      mqtt.subscribe(topicAck);
      publishConnState(1);
      publishConfig();
      delay(1000);            // cho cloud kịp ghi nhận config trước khi bắn data
      publishHeartbeat();
      lastHeartbeat = millis();
      linkTrustedAt = millis() + LINK_GRACE_MS;
      Serial.printf("[BUFF] cho %lds cho subscriber nói lai roi moi day bu\n", LINK_GRACE_MS / 1000);
      size_t pending = spoolSize();
      if (pending) Serial.printf("[BUFF] co %u byte cho day bu\n", (unsigned)pending);
      return true;
    }

    Serial.printf("[MQTT] that bai, rc=%d ", mqtt.state());
    switch (mqtt.state()) {
      case -4: Serial.println("(timeout - sai host/port hoac firewall)"); break;
      case -2: Serial.println("(connect failed - TLS/DNS hong)");         break;
      case  4: Serial.println("(SAI USER/PASSWORD)");                     break;
      case  5: Serial.println("(KHONG DU QUYEN - sai tenant/credential)");break;
      default: Serial.println();
    }
    return false;
}

// ============================================================ setup / loop
void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n\n===== Hu Tieu | Test duong truyen ESP32-S3 -> WISE-IoT =====");
  Serial.printf("STAGE = %d\n", STAGE);

  randomSeed(esp_random());
  for (int i = 0; i < NUM_CELLS; i++) cellBias[i] = random(-80, 81) / 100.0f;

  spoolBegin();
  gAI.begin();
  gCharge.begin();

#if USE_REAL_TEMP
  gTempOk = gTemp.begin(TEMP_PIN);
  if (!gTempOk) {
    // KHÔNG dừng máy: an toàn tại chỗ vẫn phải chạy với số cảm biến còn lại.
    // Nhưng phải kêu to, vì đây đúng là chế độ hỏng im lặng đã dính lúc đo
    // hiệu chuẩn — chương trình chạy ngon lành với 6 cảm biến, không báo gì.
    Serial.println("[TEMP] *** THIEU CAM BIEN - so lieu KHONG day du ***");
  }
#endif
  Serial.print("[WiFi] noi toi "); Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  wifiStart();
  // CHỜ CÓ HẠN, không chờ vô hạn. Bản cũ quay vòng mãi trong setup(): mạng
  // hỏng là board treo, Lớp 1 không chạy, còi không kêu — trong khi an toàn
  // tại chỗ đúng ra KHÔNG được phụ thuộc đường truyền. Hết giờ thì đi tiếp,
  // ensureWifi() trong loop() sẽ tự thử lại nền.
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 20000) {
    delay(500); Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() != WL_CONNECTED)
    Serial.println("[WiFi] CHUA NOI DUOC - chay tiep offline, se tu thu lai");
  else
    Serial.printf("[WiFi] OK, IP %s, RSSI %d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
  ensureWifi();
  syncTime();
  buildTopics();

#if STAGE == 2
  gUseTls = true;                       // WISE-IoT dùng MQTT over TLS (8883)
  #if WISE_USE_DCCS
    while (!fetchCredentialFromDccs()) { Serial.println("[DCCS] thu lai sau 5s"); delay(5000); }
  #else
    gHost = MQTT_HOST_MANUAL; gPort = MQTT_PORT_MANUAL;
    gUser = MQTT_USER_MANUAL; gPass = MQTT_PASS_MANUAL;
  #endif
  netTls.setInsecure();                 // BƯỚC 1 bỏ qua kiểm tra CA cho nhanh.
                                        // Xong rồi hãy nạp CA thật (xem file .md).
  mqtt.setClient(netTls);
#else
  gHost = TEST_HOST; gPort = TEST_PORT;
  mqtt.setClient(netPlain);
  Serial.println("[i] Mo https://www.hivemq.com/demos/websocket-client/ ,");
  Serial.printf ("    Connect roi Subscribe topic:  /wisepaas/scada/%s/#\n", NODE_ID);
#endif

  mqtt.setServer(gHost.c_str(), gPort);
  mqtt.setCallback(onMqttMessage);
  mqtt.setBufferSize(4096);   // BẮT BUỘC: mặc định 256 byte, payload config sẽ bị cắt
  mqtt.setKeepAlive(90);

  mqttTryConnect();
  lastReconnect = millis();
}

void loop() {
  unsigned long now = millis();

  ensureWifi();   // không chặn

  // Thử nối lại theo nhịp, cũng không chặn. Trong lúc chưa nối được, phần
  // lấy mẫu bên dưới vẫn chạy bình thường và dữ liệu chảy vào spool.
  if (!mqtt.connected() && WiFi.status() == WL_CONNECTED &&
      now - lastReconnect >= RECONNECT_MS) {
    lastReconnect = now;
    mqttTryConnect();
  }

  if (mqtt.connected()) {
    mqtt.loop();
    spoolFlush();   // đẩy bù tối đa FLUSH_BATCH gói mỗi vòng
  }

#if USE_REAL_TEMP
  // Máy trạng thái đọc cảm biến: gọi mỗi vòng, không chặn. Tự lo việc phát
  // lệnh chuyển đổi rồi quay lại lấy kết quả sau ~760 ms.
  gTemp.update();

  if (now - lastTempPrint >= TEMP_PRINT_MS) {
    lastTempPrint = now;
    gTemp.printStatus();
    publishSensorHealth();
  }
#endif

  // AI chạy 1 Hz, độc lập với nhịp gửi dữ liệu. Chạy cả khi MẤT MẠNG —
  // đây là lý do đặt AI on-device: an toàn không được phụ thuộc đường truyền.
  if (now - lastAi >= AI_PERIOD_MS) {
    lastAi = now;
    runAI();
  }

  // Lớp 2: mô phỏng một chu kỳ sạc theo nhịp, chỉ khi đã có mạng để gửi
  if (now - lastSimCharge >= SIM_CHARGE_EVERY_MS && timeIsValid()) {
    lastSimCharge = now;
    simulateChargeCycle();
  }

  if (now - lastPublish >= PUBLISH_MS)    { lastPublish   = now; publishData(); }
  if (now - lastHeartbeat >= HEARTBEAT_MS){ lastHeartbeat = now;
                                            if (mqtt.connected()) publishHeartbeat(); }

  if (spoolDropped) {
    Serial.printf("[BUFF] CANH BAO: da bo %lu goi vi flash day\n", spoolDropped);
    spoolDropped = 0;
  }
}
