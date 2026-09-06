# Bằng chứng kiểm thử — 06/09/2026

Kiểm chứng chặng **ESP32 → cloud** của Plan B. Toàn bộ stack được dựng thật bằng Docker, ESP32 được giả lập bằng `mosquitto_pub` bắn đúng payload mà `esp32s3_wiseiot_test.ino` sinh ra, rồi query ngược InfluxDB để đối chiếu.

Ghi lại theo yêu cầu của `CLAUDE.md` §Definition of Done: *AI-generated không có nghĩa là done, phải có bằng chứng chạy được.*

**Chưa kiểm được:** mọi thứ cần ESP32 thật. Chi tiết ở cuối file.

---

## §1 — Đường truyền thông suốt

4 container `running`, Node-RED nối được broker có auth:

```
NAME               STATUS
hutieu-grafana     Up 18 seconds
hutieu-influxdb    Up 18 seconds
hutieu-mosquitto   Up 18 seconds
hutieu-nodered     Up 18 seconds
[info] [mqtt-broker:Mosquitto noi bo] Connected to broker: nodered-hutieu@mqtt://mosquitto:1883
```

Bắn 3 gói dữ liệu 8 cell → query lại InfluxDB, đủ 24 điểm, đúng device và tag, cell #5 tách khỏi nhóm đúng như firmware cố ý tạo:

```
_time,_value,device,tag
2026-09-06T14:18:45.69Z,30.07,BatteryPack01,Cell01_Temp
2026-09-06T14:18:45.69Z,38.09,BatteryPack01,Cell05_Temp   <- cell "lỗi"
2026-09-06T14:18:45.69Z,29.89,BatteryPack01,Cell08_Temp
```

Query Flux trong `PLAN_B_CLOUD.md` bước 6 chạy đúng, không phải sửa.

Gói `conn` (`{"d":{"Con":1}}`) bị `convert.js` bỏ qua đúng thiết kế — không sinh rác trong DB, không làm chết flow.

→ **AC-01.1, 01.2, 01.3 đạt.**

## §2 — Timestamp do thiết bị quyết định

Bắn một gói có `ts` lùi 2 giờ so với các gói còn lại:

```
2026-09-06T12:37:26.466Z,99.99    <- gói backdate, ghi đúng ở mốc 2 giờ trước
2026-09-06T14:37:26.466Z,30.45
2026-09-06T14:37:28.466Z,29.62
```

InfluxDB tôn trọng `ts` của thiết bị, **không** tự đóng dấu lúc nhận.

→ **AC-02.1 đạt.** Đây là nền của màn demo store-and-forward, và là câu trả lời cho ẩn số mà bản thân WISE-IoT chưa chắc chắn (xem `DECISION_LOG.md` QĐ-003).

## §3 — Store-and-forward

Firmware biên dịch sạch cho board thật:

```
esp32:esp32:esp32s3:PartitionScheme=default_8MB,FlashSize=16M
Sketch uses 1015980 bytes (30%) of program storage space. Maximum is 3342336 bytes.
Global variables use 47424 bytes (14%) of dynamic memory.
```

17 test logic đệm/đẩy bù, chạy bằng `./test/run_test.sh` (script cắt code spool **thật** từ `.ino` nên không thể xanh giả):

```
Da cat 92 dong code spool that tu .ino
TEST 1: mat mang -> dem, noi lai -> day bu DUNG THU TU        5/5 PASS
TEST 2: rot mang GIUA CHUNG luc dang day bu                   3/3 PASS
TEST 3: nhieu hon FLUSH_BATCH -> chia nhieu luot              2/2 PASS
TEST 4: flash day -> compact, giu du lieu MOI                 5/5 PASS
TEST 5: spool rong thi flush khong lam gi                     2/2 PASS
=== TAT CA TEST PASS ===
```

→ **AC-03.1 → 03.5 đạt** ở mức logic. AC-03.6 (board thật) chưa làm.

**Một chỗ AI làm sai, đã sửa:** lần chạy đầu TEST 4 báo đỏ ở tiêu chí "gói cũ nhất đã bị bỏ". Truy ra thì **lỗi ở test chứ không ở firmware**: test chỉ ghi 6000 gói ≈ 330KB, chưa chạm ngưỡng 512KB nên `spoolCompact()` không bao giờ chạy — nghĩa là bốn tiêu chí còn lại của TEST 4 đang xanh một cách vô nghĩa. Đã sửa test để tính số gói theo `SPOOL_MAX_BYTES`, và thêm hẳn một tiêu chí kiểm lại **tiền đề của chính test** ("compact đã chạy thật sự chưa"). Bài học: một test xanh mà không kiểm tra tiền đề của nó thì không chứng minh được gì.

## §4 — An toàn và khả năng phát hiện lỗi

**Trước khi sửa** — Node-RED phơi ra không mật khẩu:

```
GET http://localhost:1880/flows  -> HTTP 200      (không cần credential)
GET /auth/login -> {}                              (auth không bật)
```
Đã deploy được một flow tuỳ ý vào Node-RED bằng một lệnh HTTP POST duy nhất. Flow chứa function node = chạy JavaScript trên máy chủ. Trên VPS công cộng đây là mất máy chủ.

**Sau khi sửa** — 1880 và 8086 rút về localhost, chỉ MQTT và Grafana (đều có auth) ra ngoài:

```
hutieu-nodered:   127.0.0.1:1880->1880/tcp
hutieu-influxdb:  127.0.0.1:8086->8086/tcp
hutieu-mosquitto: 0.0.0.0:1883->1883/tcp
hutieu-grafana:   0.0.0.0:3000->3000/tcp
```

**Trước khi sửa** — lỗi ghi DB im lặng hoàn toàn. Bắn gói `ts` năm 1970 (kịch bản NTP fail): InfluxDB trả **HTTP 422** và vứt dữ liệu, Node-RED **không log một dòng nào**, container vẫn xanh, Grafana chỉ đơn giản là trống. Token sai cho **HTTP 401**, cũng im lặng y hệt. Nguyên nhân: node debug đặt `tosidebar` nên chỉ hiện trong tab trình duyệt.

**Sau khi sửa** — cả hai trường hợp đều kêu to trong `docker compose logs nodered`, kèm chẩn đoán đúng nguyên nhân:

```
[warn]  [function:WISE-PaaS -> line protocol] Bo goi: ts=1970-01-01T00:00:12.000000Z
        (nam 1970?). NTP tren ESP32 chua dong bo.

[error] [function:Kiem tra ket qua ghi] Influx ghi that bai HTTP 401
        -> SAI INFLUX_TOKEN (kiem tra .env va restart nodered)
        {"code":"unauthorized","message":"unauthorized access"}
```

→ **AC-02.2, AC-04.1, 04.2, 04.3, 04.5 đạt.**

## §5 — Secret

`.gitignore` chặn `.env`, `mosquitto/passwd`, file tạm và `.rar`. Commit đầu tiên đã kiểm: không có secret nào lọt vào.

→ **AC-04.4 đạt.**

---

## Chưa kiểm được — nói thẳng

| Việc | Vì sao chưa | Ai cần làm gì |
|---|---|---|
| Firmware chạy trên ESP32 thật | Không có board trong lần kiểm này | Nạp và xem Serial (AC-01.6) |
| **Rút mạng 60s rồi cắm lại, xem dữ liệu bù về trên Grafana** | như trên | **Việc quan trọng nhất còn lại** (AC-03.6) |
| Dashboard Grafana | Chưa dựng panel | Làm theo `PLAN_B_CLOUD.md` bước 5–6 (AC-01.4) |
| Stack trên VPS công cộng | Mới chạy local | Trước ngày thi vài hôm (AC-04.6) |
| WISE-IoT thật | Chưa có tài khoản | Xong thì chạy lại §2 trên cloud Advantech |

**Kết luận:** chặng ESP → cloud đã thông và đã vá được 4 lỗi (2 lỗi im lặng, 1 lỗ hổng bảo mật, 1 tính năng thiếu). Nhưng toàn bộ bằng chứng ở trên dừng ở mức **logic đúng và cloud đúng** — chưa có một lần nào chạy trên phần cứng thật. Đừng nhầm cái này với "đã sẵn sàng demo".
