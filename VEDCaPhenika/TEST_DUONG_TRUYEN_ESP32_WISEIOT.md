# Test đường truyền ESP32-S3 → WISE-IoT

Đội Hủ Tiếu · 06/09/2026 · Bán kết **26/09** *(cập nhật 14/09: bản gốc ghi "còn 9 ngày" theo mốc cũ 15/09)*

Kèm sketch: `esp32s3_wiseiot_test/esp32s3_wiseiot_test.ino`

---

## 0. Việc chặn phải gỡ trước

Bạn trả lời là chưa có gì trong tay về tài khoản WISE-IoT. Đây là vấn đề lớn hơn con ESP nhiều, vì lộ trình cũ ghi rõ: tài khoản phải có ít nhất 1 Dashboard hoạt động trước 01/09, không có hoạt động thì **bị hủy tài khoản và loại**. Hôm nay đã 06/09.

Nên tách làm hai việc chạy song song, đừng làm tuần tự:

**Việc A — nhắn cho BTC ngay hôm nay.** Hỏi đúng bốn thứ: (1) tài khoản WISE-IoT của đội Hủ Tiếu còn hiệu lực không, (2) xin lại thông tin đăng nhập nếu chưa nhận được, (3) tenant/API URL của DCCS là gì, (4) đội được cấp mấy account. Hỏi qua kênh BTC và [Forum InnoWorks](https://forum.wise-paas.advantech.com/) cùng lúc.

**Việc B — cắm ESP chạy STAGE 1 ngay chiều nay.** Không cần chờ Việc A. Xem mục 2.

Lý do tách: nếu chờ credential mới bắt đầu code, và credential về vào ngày 12, bạn còn 3 ngày để debug cả firmware lẫn cloud cùng lúc. Còn nếu STAGE 1 xong trước, lúc credential về bạn chỉ đổi 4 dòng cấu hình.

---

## 1. Chuẩn bị Arduino IDE cho ESP32-S3

ESP32-S3 khác ESP32 đời đầu ở khâu nạp code, nên làm đúng thứ tự:

1. Cài Arduino IDE 2.x.
2. `File > Preferences > Additional boards manager URLs`, dán:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. `Tools > Board > Boards Manager`, tìm **esp32 by Espressif**, cài bản **3.x trở lên** (bản 2.x hỗ trợ S3 không đầy đủ).
4. `Tools > Board` chọn **ESP32S3 Dev Module**. Đặt thêm:
   - `USB CDC On Boot` → **Enabled** ← thiếu cái này thì Serial Monitor im lặng hoàn toàn, đây là chỗ hầu hết mọi người mắc lần đầu với S3
   - `Flash Size` → 16MB
   - `PSRAM` → **OPI PSRAM** (đúng cho bản N16R8)
   - `Partition Scheme` → 16M Flash (3MB APP/9.9MB FATFS)
5. `Tools > Manage Libraries`, cài **PubSubClient** (Nick O'Leary) và **ArduinoJson** (Benoit Blanchon, v7).
6. Cắm cáp vào cổng **USB** (không phải cổng UART) — DevKitC-1 có hai cổng USB-C, cổng `USB` là cổng có native USB của chip. Nếu máy không hiện COM port: giữ nút **BOOT**, nhấn nhả **RESET**, thả BOOT, rồi nạp.

---

## 2. STAGE 1 — chứng minh đường ống chạy, không cần WISE-IoT

Mở sketch, sửa `WIFI_SSID` / `WIFI_PASS`, để nguyên `#define STAGE 1`, nạp.

ESP sẽ nối WiFi → đồng bộ giờ NTP → bắn JSON lên broker công cộng HiveMQ mỗi 2 giây, đúng **y hệt topic và y hệt payload** mà WISE-IoT yêu cầu.

Xem kết quả: mở https://www.hivemq.com/demos/websocket-client/ → Connect → Subscribe topic `/wisepaas/scada/#`. Bạn sẽ thấy các gói `data` chảy về. Đối chiếu với Serial Monitor (115200 baud).

Qua được bước này nghĩa là đã chốt xong: WiFi ổn định, MQTT client hoạt động, JSON đúng cú pháp, timestamp UTC đúng định dạng, buffer đủ lớn. Bốn thứ này chiếm phần lớn thời gian debug. Sau đó mọi lỗi còn lại đều nằm phía cloud, và bạn biết chắc là vậy.

> Lưu ý riêng tư: HiveMQ là broker công cộng, ai cũng đọc được. Chỉ dùng dữ liệu giả lập ở bước này, đừng dùng khi có dữ liệu thật.

---

## 3. Giao thức WISE-IoT — phần tra được, để khỏi mò

Đây là câu hỏi bỏ ngỏ trong khung nghiên cứu ("chưa tìm được tài liệu chính thức xác nhận ESP32 nối trực tiếp được không"). Tớ đã giải nén SDK Python chính chủ của Advantech (`EdgeSync360-EdgeHub-Edge-Python-SDK`) và đọc mã nguồn. Kết luận: **SDK chỉ là lớp bọc mỏng quanh MQTT thuần**. Không có gì bí mật, không cần binary riêng. ESP32 nói chuyện trực tiếp được. Rủi ro kỹ thuật lớn nhất của phương án bỏ Raspberry Pi coi như đã gỡ.

**Topic** (`{nodeId}` là UUID lấy ở portal):

| Mục đích | Topic |
|---|---|
| Gửi dữ liệu | `/wisepaas/scada/{nodeId}/data` |
| Khai báo cấu hình tag | `/wisepaas/scada/{nodeId}/cfg` |
| Trạng thái kết nối + heartbeat | `/wisepaas/scada/{nodeId}/conn` |
| Nhận lệnh từ cloud | `/wisepaas/scada/{nodeId}/cmd` |
| Nhận phản hồi | `/wisepaas/scada/{nodeId}/ack` |

**Payload dữ liệu** — `ts` là ISO-8601 **UTC**, đúng 6 chữ số sau dấu chấm:

```json
{ "d": { "BatteryPack01": { "Cell01_Temp": 30.42, "Cell02_Temp": 31.07 } },
  "ts": "2026-09-06T09:15:22.410000Z" }
```

**Trạng thái**: `{"d":{"Con":1},...}` khi nối, `{"d":{"DsC":1},...}` khi ngắt chủ động, `{"d":{"UeD":1},...}` đặt làm Last Will, `{"d":{"Hbt":1},...}` mỗi 60 giây.

**Config** phải gửi trước, nếu không dashboard sẽ trống dù dữ liệu vẫn về. Cấu trúc `{"d":{"Action":1,"Scada":{nodeId:{"Type":0,"Hbt":60,"Device":{devId:{"Name":..,"Tag":{tagName:{"Type":1,"SH":80,"SL":0,"EU":"degC",...}}}}}}},"ts":..}`. Sketch đã sinh sẵn cho 8 cell.

**Lấy thông tin broker qua DCCS**: `GET {apiUrl}/v1/serviceCredentials/{credentialKey}` trả về `serviceHost` và `credential.protocols.mqtt+ssl.{port,username,password}`. Sketch tự làm bước này, bạn chỉ cần dán Credential Key.

Một điểm cần tự kiểm chứng khi có tài khoản: **`ts` do mình gửi có được cloud tôn trọng không, hay nó tự đóng dấu lúc nhận?** Việc này quyết định store-and-forward (đẩy bù dữ liệu sau khi mất mạng) có làm được không — mà đó là 90 giây đắt giá nhất trong kịch bản demo. Cách kiểm: gửi một gói với `ts` lùi lại 2 giờ, xem dashboard vẽ ở đâu.

---

## 4. STAGE 2 — nối thật

Khi có tài khoản: đăng nhập portal, vào **EdgeHub** (hoặc DataHub tùy phiên bản), tạo một **Node**, copy `nodeId` (UUID) và **Credential Key**. Rồi trong sketch:

```cpp
#define STAGE 2
const char* DCCS_API_URL  = "...";   // BTC cho, dạng https://api-dccs-ensaas.<tenant>.wise-paas.com
const char* DCCS_CRED_KEY = "...";
const char* NODE_ID       = "...";
```

Nạp lại. Không sửa gì khác.

---

## 5. Lỗi thường gặp

| Triệu chứng | Nguyên nhân gần như chắc chắn |
|---|---|
| Serial Monitor trống trơn | Chưa bật `USB CDC On Boot`, hoặc cắm nhầm cổng UART |
| WiFi không nối được | Router phát 5GHz. ESP32-S3 chỉ bắt 2.4GHz |
| `[NTP] ... THAT BAI` | Mạng chặn UDP/123. Dữ liệu sẽ mang timestamp 1970 và cloud vứt đi — phải sửa trước khi đi tiếp |
| `rc=4` | Sai username/password |
| `rc=5` | Credential đúng cú pháp nhưng không đủ quyền / sai tenant |
| `rc=-4` hoặc `-2` | Sai host/port, hoặc mạng trường chặn cổng 8883. Thử 4G điện thoại để loại trừ |
| `[CFG ] ... THAT BAI` | Quên `mqtt.setBufferSize()`. Mặc định PubSubClient chỉ 256 byte |
| Kết nối OK, dashboard trống | Chưa gửi config, hoặc `deviceId`/`tagName` trong data không khớp với config |
| Dữ liệu về nhưng dồn cục sai giờ | Cloud tự đóng dấu thời gian — xem ghi chú cuối mục 3 |

Về TLS: sketch đang dùng `setInsecure()` để bỏ qua kiểm tra chứng chỉ, cho nhanh. Chấp nhận được lúc test, nhưng đừng để nguyên khi demo trước giám khảo — nếu bị hỏi về bảo mật thì đó là điểm trừ dễ tránh. Sau khi chạy được, thay bằng `netTls.setCACert(...)` với CA thật.

---

## 6. Đề xuất thứ tự làm trong 3 ngày tới

Hôm nay: gửi yêu cầu cho BTC + chạy xong STAGE 1. Ngày mai: có credential thì chuyển STAGE 2 và dựng 1 dashboard bất kỳ (thỏa luôn ràng buộc "có hoạt động"). Ngày kia: kiểm chứng câu hỏi timestamp, rồi mới nghĩ tới cảm biến thật.

Nếu đến 09/09 vẫn chưa có credential, chuyển sang phương án dự phòng trong khung nghiên cứu: dùng Node-RED làm cầu MQTT, và trong slide kiến trúc vẽ tầng đó là "Gateway trạm — tùy chọn".

---

**Nguồn:**
- [Python Edge SDK Manual — Advantech WISE Learning Hub](https://learn.advantech.com/EdgeHub/Edge_SDK_WISE-PaaS_MQTT_Protocol/Python_Edge_SDK_Manual/)
- Mã nguồn gói `EdgeSync360-EdgeHub-Edge-Python-SDK` 1.0.2.1 (PyPI) — `Common/Topic.py`, `Model/MQTTMessage.py`, `EdgeAgent.py`
- [WISE-PaaS/2.0 Protocol — ESS-WIKI](http://ess-wiki.advantech.com.tw/view/WISE-PaaS/2.0_Protocol)
- [Forum InnoWorks / WISE Developer Community](https://forum.wise-paas.advantech.com/)
