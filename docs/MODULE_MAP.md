# Module Map — chức năng nằm ở đâu

Mục đích: người mới (hoặc AI) cần sửa một chức năng thì biết mở file nào, và biết sửa nó sẽ đụng tới đâu.

---

## Sơ đồ đường đi dữ liệu

```
ESP32-S3 ──MQTT/1883──► Mosquitto ──sub──► Node-RED ──HTTP──► InfluxDB ──Flux──► Grafana
   │                    (có auth)          (chuyển đổi)       (lưu 90d)      (dashboard)
   └── mất mạng ──► LittleFS spool ──► đẩy bù khi nối lại (giữ nguyên ts)
```

Khi có tài khoản WISE-IoT: bốn hộp bên phải được thay bằng WISE-IoT, ESP32 **không đổi** (xem `docs/DATA_CONTRACT.md` §5).

---

## Firmware — `VEDCaPhenika/esp32s3_wiseiot_test/esp32s3_wiseiot_test.ino`

Một file duy nhất, cố ý. Đây là sketch *test đường truyền*, không phải firmware sản phẩm; gộp một file để nạp nhanh và đọc hết trong một lượt.

| Vùng | Hàm | Ghi chú |
|---|---|---|
| **Số cell** | `pack_config.h` → `PACK_N_CELLS` | **Nguồn sự thật DUY NHẤT.** `AI_N_CELLS`, `DS_N_PROBES`, `CC_N_CELLS`, `PM_N_CELLS`, `NUM_CELLS`, `PM_V_MIN/MAX` đều suy ra từ đây. Đang là **6**; **8 là bản dự phòng**, đổi một dòng. Số khác 6/8 bị `#error` chặn. Xem QĐ-038. |
| Cấu hình | các `const` đầu file | `STAGE 1/2` quyết định bắn đi đâu. Sửa WiFi/broker ở đây. |
| Thời gian | `isoTimestampUtc()`, `syncTime()`, `timeIsValid()` | `timeIsValid()` chặn không cho gửi khi NTP chưa xong. |
| **Store-and-forward** | `spoolBegin/Append/Compact/Flush/Size()` | Đệm xuống LittleFS khi mất mạng. Có test riêng. |
| Mạng | `ensureWifi()`, `mqttTryConnect()` | **Không chặn.** Chặn ở đây là loop() đứng và mất luôn khả năng đệm. |
| Cloud WISE-IoT | `fetchCredentialFromDccs()` | Chỉ dùng ở STAGE 2. Lấy host/user/pass từ Credential Key. |
| Giao thức | `buildTopics()`, `publishConfig/Data/ConnState/Heartbeat()` | Sửa mấy hàm này là **đụng data contract** → phải đọc `docs/DATA_CONTRACT.md` trước. |
| Vòng chính | `setup()`, `loop()` | `loop()` luôn lấy mẫu đúng nhịp bất kể có mạng hay không. |

**Cài đặt build:** Board `ESP32S3 Dev Module`, Partition Scheme **`8M with spiffs (3MB APP/1.5MB SPIFFS)`**.
⚠️ Không chọn các scheme `16M Flash (... FATFS)` — chúng không có phân vùng SPIFFS nên `LittleFS.begin()` sẽ fail và mất toàn bộ store-and-forward. Chi tiết: `docs/DECISION_LOG.md` QĐ-006.

Thư viện: `PubSubClient`, `ArduinoJson` v7.

## Test firmware — `VEDCaPhenika/esp32s3_wiseiot_test/test/`

| File | Vai trò |
|---|---|
| `run_test.sh` | Cắt code spool **thật** từ `.ino` rồi biên dịch cùng test. Chạy trên PC, không cần board. |
| `test_spool.cpp` | Giả lập LittleFS + MQTT, kiểm 5 nhóm tình huống mất mạng. |

Chạy: `./test/run_test.sh`. Sửa logic spool mà không chạy lại cái này là đang bay mù.

## Bench phần cứng — `test/`

Sketch chạy **trên board thật**, không phải test trên PC. Mỗi thư mục nghiệm thu
một mảnh phần cứng, để khi hỏng thì chỉ có một nghi phạm.

| Thư mục | Nghiệm thu cái gì |
|---|---|
| `ds18b20_stress_test/` · `ds18b20_power_diag/` · `ds18b20_boot_latch/` | Bus 1-Wire, nguồn, hiệu chuẩn offset |
| `cell_temp_ai_bench/` | `cell_temp` + `cell_ai` chạy cùng nhau |
| `alarm_bench/` | Bốn mức báo động (QĐ-028) |
| `pack_meter_bench/` | Giải mã thanh ghi INA228 — **chạy khô, không cần chip** |
| `pack_meter_live/` | `PackMeter` trên **chip thật**: kiểm đường tự nhận INA226/INA228 (QĐ-037) |
| **`hw_bringup_6s/`** | **Đợt 3: INA226 + 2×D4184 + sưởi 20 Ω + còi SFM-27.** Menu serial T1..T9 + `d`. Bằng chứng: `docs/BANG_CHUNG_BRINGUP_PHAN_CUNG_2026-09-20.md` |

Nạp `hw_bringup_6s` (board cắm cổng USB-Serial-JTAG, `/dev/ttyACM0`):

```
FQBN="esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=default,PSRAM=opi,FlashSize=16M"
```

⚠️ `CDCOnBoot=cdc` **nạp được, chạy được, nhưng serial câm tịt**. Máy này không
có `arduino-cli` trên PATH; bản đi kèm nằm trong AppImage của Arduino IDE
(`--appimage-extract`).

## Tầng cloud Plan B — `VEDCaPhenika/planb_cloud/`

| File | Vai trò | Sửa khi nào |
|---|---|---|
| `docker-compose.yml` | 4 container: mosquitto, influxdb, nodered, grafana | Đổi port, đổi version (đọc comment ghim version trước) |
| `.env.example` | Mẫu secret. Copy thành `.env` | Không commit `.env` |
| `mosquitto/mosquitto.conf` | Bắt buộc đăng nhập MQTT | Bật ẩn danh chỉ khi chạy LAN nhà |
| `mosquitto/passwd` | Sinh bằng `mosquitto_passwd`, **không commit** | Đổi mật khẩu MQTT |
| `nodered/flow_wisepaas_to_influx.json` | Flow chạy thật, import qua UI | Đổi luồng xử lý |
| `nodered/convert.js` | Bản đọc được của node "WISE-PaaS → line protocol" | Đổi cách map payload → DB |
| `nodered/check_influx_response.js` | Bản đọc được của node "Kiểm tra kết quả ghi" | Đổi cách báo lỗi ghi |

⚠️ `convert.js` và `check_influx_response.js` là **bản chép để đọc**; bản chạy thật nằm trong JSON. Sửa file `.js` thì phải đồng bộ lại vào JSON, nếu không sẽ sửa một đằng chạy một nẻo.

## Tài liệu

| Nhóm | Ở đâu |
|---|---|
| Điều khiển AI, gate, DoD | `CLAUDE.md` |
| Hợp đồng dữ liệu, module map, test, quyết định, RTM | `docs/` |
| Nghiên cứu, phần cứng, lộ trình, Plan B | `VEDCaPhenika/*.md` |
