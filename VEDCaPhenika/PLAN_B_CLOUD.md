# Plan B — cloud thay Advantech, không phải sửa lại firmware

Đội Hủ Tiếu · 06/09/2026 · file cấu hình kèm theo trong thư mục `planb_cloud/`

---

## Nguyên tắc chọn: giữ nguyên giao thức WISE-PaaS

Bạn đã có một tài sản mà hầu hết các đội khác không có: firmware ESP32-S3 bắn đúng topic `/wisepaas/scada/{nodeId}/data` và đúng payload `{"d":{...},"ts":...}`, đã đối chiếu byte-for-byte với SDK chính chủ của Advantech. Đừng vứt nó đi chỉ vì tạm thời chưa có tài khoản.

Nên tiêu chí số một của Plan B không phải là "nền tảng nào nhiều tính năng nhất", mà là **nền tảng nào cho phép ESP32 giữ nguyên không sửa một dòng nào**. Bất kỳ phương án nào bắt bạn đổi format payload đều là bước lùi: mất luôn phần chứng minh với giám khảo Advantech rằng bạn hiểu sản phẩm của họ, và khi tài khoản WISE-IoT về thì lại phải sửa ngược.

---

## Các phương án đã cân nhắc

| Phương án | Vì sao loại / giữ |
|---|---|
| **ThingsBoard Cloud** | Bản hosted đã bỏ free tier từ 2026, rẻ nhất ~$10/tháng. Bản Community tự host thì miễn phí nhưng bắt payload theo topic `v1/devices/me/telemetry` → **phải sửa firmware**. Loại. |
| **EMQX Serverless / HiveMQ Cloud** | Free tier thật (EMQX cho 1 triệu session-minute + 1GB/tháng, thừa cho 1 con ESP). Nhưng **chỉ là broker** — không có lưu trữ, không có dashboard. Chỉ giải quyết 1/3 bài toán. |
| **Ubidots STEM / Blynk / ThingSpeak** | Dựng nhanh nhất, nhưng khoá cứng format payload và trông như đồ án môn học. Trước giám khảo Advantech thì đây là điểm trừ. Loại. |
| **Grafana Cloud + InfluxDB Cloud (managed)** | Free tier ổn, nhưng vẫn cần một tiến trình chạy đâu đó để chuyển MQTT → DB. Đã phải có máy chạy rồi thì tự host luôn cả bộ cho gọn. |
| **Tự dựng: Mosquitto + Node-RED + InfluxDB + Grafana** | ✅ **Chọn cái này.** |

---

## Vì sao tự dựng lại là phương án mạnh nhất ở đây

Không phải vì nó miễn phí. Mà vì **đây đúng là bộ ruột mà WISE-PaaS chạy bên dưới**: Dashboard của WISE-PaaS chính là Grafana, lớp lưu trữ là InfluxDB/PostgreSQL, lớp nhận bản tin là một MQTT broker. Bạn không dựng một thứ "na ná" — bạn dựng đúng các thành phần đó, chỉ thiếu phần đóng gói thương mại của Advantech.

Điều đó cho bạn một câu nói trước giám khảo mà không đội nào bắt bẻ được:

> "Firmware của chúng em nói đúng giao thức WISE-PaaS. Trong lúc chờ cấp tài khoản, chúng em chạy tạm chính stack mà WISE-PaaS dựng trên đó — Grafana và InfluxDB. Khi có tài khoản, chuyển sang là đổi cấu hình, không phải viết lại."

Và có một phần thưởng kèm theo, xem mục "Câu hỏi timestamp" bên dưới — nó gỡ nốt rủi ro cuối cùng của kịch bản demo.

---

## Chạy ở đâu

Dùng **cùng một file `docker-compose.yml`** cho cả hai chặng, nên không có công nào bị bỏ đi:

**Chặng 1 — trên laptop của bạn, làm ngay chiều nay.** ESP32 và laptop cùng mạng WiFi, trỏ `TEST_HOST` vào IP LAN của laptop (kiểu `192.168.1.x`). Đủ để dựng dashboard và chạy thử toàn bộ. Khoảng 30 phút.

**Chặng 2 — trên máy chủ có IP công cộng, trước ngày thi vài hôm.** Lý do phải làm bước này: WiFi hội trường thường chặn hoặc NAT lung tung, và bạn không muốn phát hiện điều đó lúc 8 giờ sáng ngày thi. Hai lựa chọn:

- **Oracle Cloud Always Free** — 2 OCPU / 12GB RAM ARM, miễn phí vĩnh viễn, chọn region Singapore (region Mỹ hay hết chỗ). Thừa sức. Đổi lại: đăng ký cần thẻ tín dụng và đôi khi phải chờ có chỗ trống.
- **VPS trả tiền ~120–150k/tháng** (Vultr/DigitalOcean Singapore, hoặc AZDIGI/Vietnix cho ping thấp). Kinh phí đã xin được rồi, và đây là chỗ đáng tiêu tiền nhất: nó mua sự chắc chắn cho ngày thi.

Và dù chọn gì, **ngày thi vẫn phát WiFi từ điện thoại** thay vì dùng mạng hội trường.

---

## Dựng lên

Trong thư mục `planb_cloud/` đã có sẵn 5 file. Cần cài Docker Desktop (Windows) hoặc Docker Engine (Linux) trước.

> **Đừng đổi image tag trong `docker-compose.yml`.** Hai cái đã ghim là ghim có lý do: `eclipse-mosquitto:2.0.22` vì tag `2` nay trỏ sang 2.1.x đã bỏ chỉ thị `password_file`, và `influxdb:2.7` vì tag `latest` đã sang InfluxDB 3 — bản 3 **bỏ hẳn Flux**, tức là mọi câu query ở bước 6 sẽ chết.

1. Đổi tên `.env.example` thành `.env`, sửa hết các mật khẩu và sinh `INFLUX_TOKEN` bằng lệnh ghi trong file. Token này là thứ Node-RED dùng để ghi vào InfluxDB.

2. Tạo tài khoản MQTT — **làm bước này trước khi `up`**, thiếu file `passwd` là Mosquitto khởi động rồi chết ngay. Chạy trong thư mục `planb_cloud/`:

   ```
   # PowerShell (mặc định của Docker Desktop trên Windows)
   docker run --rm -v "${PWD}/mosquitto:/m" eclipse-mosquitto:2.0.22 mosquitto_passwd -c -b /m/passwd hutieu MAT_KHAU_CUA_BAN

   # cmd.exe thì đổi ${PWD} thành %cd% ; Linux/macOS thì đổi thành $PWD
   ```

   Nếu chỉ chạy trong LAN nhà và muốn nhanh, mở `mosquitto/mosquitto.conf` làm theo ghi chú cuối file để bật ẩn danh — nhưng **tuyệt đối không để ẩn danh khi đưa lên máy chủ công cộng**, cổng 1883 mở toang là ai cũng bơm rác vào được.

3. `docker compose pull` rồi `docker compose up -d`. Tách hai lệnh để nếu có image nào tải hỏng thì báo lỗi rõ ràng ngay, thay vì lẫn vào log khởi động.

   **Kiểm ngay, đừng bỏ qua:** `docker compose ps` — cả bốn container phải ở trạng thái `running`. Nếu `hutieu-mosquitto` bị `restarting` thì chạy `docker compose logs mosquitto`, gần như chắc chắn là quên bước 2. Đây là cái bẫy tốn thời gian nhất, vì phía ESP32 chỉ báo `rc=-2` chung chung và bạn sẽ đi tìm nhầm chỗ.

   **Trên Windows còn một bước nữa:** mở Windows Defender Firewall → Inbound Rules → New Rule → Port → TCP **1883** → Allow. Không mở thì ESP32 không vào được laptop dù cùng WiFi. Đây là chỗ kẹt phổ biến nhất ở Chặng 1.

4. Mở Node-RED ở `http://localhost:1880` → menu góc phải → **Import** → dán nội dung `nodered/flow_wisepaas_to_influx.json` → **Deploy**. Rồi double-click node "Mosquitto noi bo" → tab **Security** → điền `hutieu` + mật khẩu vừa tạo → Deploy lại. (Mật khẩu không nằm trong file flow, đó là lý do phải điền tay.)

   > **Ở Chặng 2 thì `localhost:1880` sẽ không mở được từ máy bạn — và đó là cố ý.** Node-RED không có mật khẩu mặc định, ai vào được cổng 1880 là deploy được function node, tức là chạy code tuỳ ý trên máy chủ. Nên `docker-compose.yml` đã ghim cổng 1880 và 8086 vào `127.0.0.1`. Muốn sửa flow từ xa thì mở đường hầm SSH rồi vào `http://localhost:1880` như thường:
   >
   > ```
   > ssh -N -L 1880:localhost:1880 -L 8086:localhost:8086 user@ip_cua_vps
   > ```

5. Mở Grafana ở `http://localhost:3000`, đăng nhập bằng `GRAFANA_USER`/`GRAFANA_PASSWORD` trong `.env`. Thêm data source **InfluxDB**, chọn query language **Flux**, URL `http://influxdb:8086`, Organization `hutieu`, Token là `INFLUX_TOKEN`, Default bucket `battery`.

6. Panel đầu tiên — 8 đường nhiệt độ cell:

   ```flux
   from(bucket: "battery")
     |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
     |> filter(fn: (r) => r._measurement == "cell")
     |> filter(fn: (r) => r._field == "value")
     |> aggregateWindow(every: v.windowPeriod, fn: mean, createEmpty: false)
   ```

   Dòng `_field == "value"` chưa cần thiết lúc này nhưng cứ để sẵn: hôm nào bạn thêm một tag kiểu chuỗi (trạng thái, mã lỗi) thì `mean` sẽ báo lỗi nếu thiếu nó.

---

## Đổi gì trong sketch

**Trước hết, một cài đặt board dễ chọn nhầm:** Tools → Partition Scheme → **`8M with spiffs (3MB APP/1.5MB SPIFFS)`**. Board là N16R8 nên bản năng sẽ chọn scheme 16M cho "hết flash", nhưng cả hai scheme 16M đều là **FATFS, không có phân vùng SPIFFS** — mà store-and-forward mount LittleFS trên phân vùng `spiffs`. Chọn nhầm là mất tính năng đệm, và mất một cách lặng lẽ ngay từ lúc boot.

Rồi ba dòng, ở phần STAGE 1:

```cpp
const char* TEST_HOST = "192.168.1.50";   // IP laptop, hoặc IP public của VPS
const char* TEST_USER = "hutieu";
const char* TEST_PASS = "mat_khau_vua_tao";
```

Để `TEST_USER = ""` thì nối ẩn danh như cũ (dùng cho HiveMQ công cộng). Tớ đã sửa sẵn phần `mqtt.connect()` để nhận tài khoản và đăng ký luôn Last Will ở STAGE 1 — trước đây chỉ STAGE 2 mới có.

Lưu ý: nếu điền `TEST_USER` mà để `TEST_PASS` rỗng thì broker trả `rc=4` hoặc `rc=5`. Điền cả hai, hoặc để trống cả hai.

`NODE_ID` cứ để nguyên chuỗi placeholder cũng chạy, vì nó chỉ là một đoạn trong topic. Nhưng nên đặt một UUID thật ngay từ giờ để lúc chuyển sang WISE-IoT không phải nhớ lại.

---

## Câu hỏi timestamp — Plan B trả lời luôn được

Trong khung nghiên cứu có một ẩn số chưa gỡ được: **WISE-IoT có tôn trọng `ts` do thiết bị gửi lên không, hay tự đóng dấu lúc nhận?** Câu này quyết định phần demo store-and-forward — 90 giây đắt nhất trong bài thuyết trình: rút mạng, ESP32 đệm dữ liệu, cắm lại, dữ liệu bù về đúng vị trí thời gian.

Với InfluxDB của chính mình thì câu trả lời chắc chắn là **có** — và điều này **đã được kiểm chứng thật**, không còn là suy luận: gửi một gói `ts` lùi 2 tiếng thì InfluxDB ghi đúng ở mốc 2 tiếng trước, trong khi các gói cùng lượt nằm ở hiện tại. Log và số liệu cụ thể trong `docs/BANG_CHUNG_KIEM_THU_2026-09-06.md` §2.

Phần đệm phía ESP32 cũng đã có: mất mạng thì dữ liệu ghi xuống LittleFS, nối lại thì đẩy bù đúng thứ tự và giữ nguyên `ts` gốc. Logic này có bộ test chạy trên PC không cần board — `esp32s3_wiseiot_test/test/run_test.sh`. Việc còn thiếu là diễn tập trên board thật.

Nói cách khác, Plan B không chỉ là phương án chữa cháy — nó gỡ nốt rủi ro kỹ thuật cuối cùng của kịch bản demo, việc mà bản thân WISE-IoT chưa chắc làm được cho tới khi bạn có tài khoản để thử.

---

## Slide kiến trúc nên vẽ thế nào

Vẫn vẽ WISE-IoT ở tầng cloud, nhưng thêm một hộp nhỏ ghi "lớp cloud hiện tại: Grafana + InfluxDB, cùng giao thức". Không giấu, không nói vòng. Giám khảo Advantech nhìn payload là nhận ra format của họ ngay, và việc bạn chủ động dựng được tầng thay thế trong 1 ngày là điểm cộng về năng lực kỹ thuật chứ không phải điểm trừ về việc chưa có tài khoản.

---

## Khi tài khoản WISE-IoT về

Đổi `#define STAGE 1` thành `2`, điền DCCS API URL + Credential Key + Node ID. Hết. Bộ Plan B vẫn giữ lại chạy song song làm dự phòng cho ngày thi — nếu cloud Advantech trục trặc lúc demo, bạn đổi một dòng là quay về.

---

**Nguồn:**
- [ThingsBoard Pricing](https://thingsboard.io/pricing/) · [Subscription Plans](https://thingsboard.io/docs/paas/reference/subscriptions/)
- [EMQX — Forever Free Serverless MQTT](https://www.emqx.com/en/blog/how-to-get-a-forever-free-serverless-mqtt-service) · [EMQX Pricing](https://www.emqx.com/en/pricing)
- [Oracle Cloud Always Free Resources](https://docs.oracle.com/en-us/iaas/Content/FreeTier/freetier_topic-Always_Free_Resources.htm) · [Oracle cắt free tier A1 còn 2 OCPU/12GB, 06/2026](https://www.infoq.com/news/2026/07/oracle-cloud-free-tier-limits/)
