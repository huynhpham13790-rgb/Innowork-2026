# Decision Log — đã cân nhắc gì, chọn gì, vì sao

Mỗi quyết định kỹ thuật đáng kể một dòng. Mục đích: 3 tuần nữa không ai phải tranh luận lại từ đầu, và người ngoài (giám khảo, thành viên mới, AI) hiểu được vì sao hệ thống có hình dạng hiện tại.

Format: **QĐ-xxx · ngày · trạng thái** — quyết định · lý do · cái đã đánh đổi.

---

### QĐ-001 · 06/09/2026 · Đã chốt
**Giữ nguyên giao thức WISE-PaaS làm ràng buộc số một, mọi phương án cloud phải thích ứng theo nó.**

Firmware đã bắn đúng topic + payload của Advantech, đối chiếu byte-for-byte với SDK gốc. Đây là thứ hầu hết các đội khác không có, và là bằng chứng trực quan nhất trước giám khảo Advantech rằng đội hiểu sản phẩm của họ.

*Đánh đổi:* loại bỏ nhiều nền tảng dựng nhanh hơn (ThingsBoard, Ubidots, Blynk) vì chúng khoá cứng format payload.

### QĐ-002 · 06/09/2026 · Đã chốt
**Plan B = tự dựng Mosquitto + Node-RED + InfluxDB + Grafana bằng Docker Compose.**

Không phải vì miễn phí, mà vì đây **đúng là bộ ruột WISE-PaaS chạy bên dưới**: dashboard của WISE-PaaS chính là Grafana, lưu trữ là InfluxDB. Đội không dựng thứ "na ná" mà dựng đúng các thành phần đó. Cho phép nói thẳng với giám khảo thay vì giấu.

*Các phương án đã loại:* ThingsBoard Cloud (bỏ free tier 2026; bản Community bắt đổi topic → sửa firmware), EMQX/HiveMQ Serverless (chỉ là broker, thiếu lưu trữ và dashboard — giải quyết 1/3 bài toán), Ubidots/Blynk (khoá format, trông như đồ án môn học).

*Đánh đổi:* phải tự vận hành một máy chủ, và phải tự lo phần bảo mật — chính chỗ này sinh ra QĐ-004.

### QĐ-003 · 06/09/2026 · Đã kiểm chứng
**Timestamp do thiết bị sinh, cloud phải tôn trọng.**

Đây là ẩn số lớn với WISE-IoT (chưa có tài khoản để thử). Với InfluxDB tự dựng thì đã **kiểm chứng được là có**: gói `ts` lùi 2 giờ được ghi đúng ở mốc 2 giờ trước. Nhờ vậy Plan B không chỉ chữa cháy mà còn gỡ nốt rủi ro kỹ thuật cuối cùng của kịch bản demo.

*Hệ quả:* màn store-and-forward chắc chắn diễn được trên Plan B, kể cả khi WISE-IoT có hành xử khác.

### QĐ-004 · 06/09/2026 · Đã chốt
**Node-RED (1880) và InfluxDB (8086) chỉ mở trên `127.0.0.1`; muốn vào từ xa thì dùng SSH tunnel.**

Kiểm chứng trực tiếp: với cấu hình cũ, deploy được một flow tuỳ ý vào Node-RED bằng một lệnh HTTP POST **không cần credential nào**. Node-RED không bật auth mặc định, mà flow chứa function node = chạy JavaScript tuỳ ý trên máy chủ. Ở Chặng 2 (VPS có IP công cộng) đây là mất máy chủ, không phải "rủi ro lý thuyết".

*Đánh đổi:* thêm một bước SSH tunnel khi cần sửa flow từ xa. Chấp nhận được — sửa flow là việc hiếm.

### QĐ-005 · 06/09/2026 · Đã chốt, có rủi ro tồn đọng
**Chấp nhận QoS 0, bù lại bằng store-and-forward ở tầng ứng dụng.**

PubSubClient chỉ publish được QoS 0. Đổi thư viện (sang AsyncMqttClient hoặc ESP-MQTT) sẽ có QoS 1 nhưng phải viết lại toàn bộ phần MQTT sát ngày thi.

*Rủi ro còn lại:* broker nuốt gói thì ESP không biết, dữ liệu mất im lặng. Store-and-forward chỉ cứu trường hợp *mất kết nối* (phát hiện được), không cứu trường hợp *gói rơi khi vẫn đang kết nối*. Với bài toán demo nhịp 2s thì chấp nhận được. Ghi lại đây để không ai tưởng là đã kín.

### QĐ-006 · 06/09/2026 · Đã chốt
**Partition scheme `8M with spiffs (3MB APP/1.5MB SPIFFS)`, không dùng scheme 16M.**

Board là ESP32-S3 N16R8 nên theo bản năng sẽ chọn scheme 16M cho "hết flash". Nhưng cả hai scheme 16M của core Espressif đều là **FATFS**, không có phân vùng SPIFFS — mà `LittleFS.begin()` mount phân vùng nhãn `spiffs`. Chọn nhầm là store-and-forward chết lặng lẽ ngay từ boot.

*Số liệu:* với scheme này firmware chiếm 30% của 3MB app, còn 1.5MB SPIFFS (thừa cho spool 512KB) và còn nhiều chỗ cho mô hình AI sau này.

### QĐ-007 · 06/09/2026 · Đã chốt
**Spool đầy thì bỏ dữ liệu cũ, giữ dữ liệu mới.**

Với giám sát an toàn pin, nhiệt độ 10 phút trước đáng giá hơn nhiệt độ 30 giờ trước. Cách làm: khi vượt 512KB thì cắt bỏ nửa cũ của file (compaction), thay vì viết ring buffer thật.

*Đánh đổi:* compaction là một lần ghi lại nửa file, tốn thời gian — nhưng chỉ xảy ra sau ~1,5 ngày mất mạng liên tục, tình huống không có trong kịch bản demo.

### QĐ-008 · 06/09/2026 · Đã chốt
**Ghim cứng version cho cả 4 image Docker.**

`eclipse-mosquitto:2.0.22` (tag `2` nay trỏ 2.1.x đã bỏ `password_file`), `influxdb:2.7` (tag `latest` đã sang InfluxDB 3, bỏ hẳn Flux → mọi query Grafana chết), `nodered/node-red:4.1`, `grafana/grafana:12.1`.

*Lý do gộp chung:* thứ duy nhất tệ hơn một hệ thống hỏng là một hệ thống tự hỏng vào đêm trước ngày thi vì có ai đó `docker compose pull`.

### QĐ-009 · 06/09/2026 · Đã chốt
**Lỗi ghi InfluxDB phải kêu to trong `docker compose logs`, không chỉ hiện ở sidebar Node-RED.**

Kiểm chứng: với cấu hình cũ, token sai (HTTP 401) hoặc ts năm 1970 (HTTP 422) đều làm mất dữ liệu mà **không có một dòng log nào**, container vẫn xanh, Grafana chỉ đơn giản là trống. Ngày thi mà không mở sẵn tab Node-RED thì không ai biết.

*Cách sửa:* thêm node "Kiểm tra kết quả ghi" đọc `statusCode` và `node.error()` kèm chẩn đoán theo từng mã lỗi; chặn sớm ts vô lý ngay trong `convert.js`.

---

## Ẩn số còn treo

| Ẩn số | Chặn việc gì | Gỡ bằng cách nào |
|---|---|---|
| WISE-IoT có tôn trọng `ts` thiết bị gửi không? | Độ chắc chắn của màn demo trên cloud Advantech | Chờ tài khoản, bắn 1 gói backdate. Plan B đã có đường lùi. |
| WiFi hội trường có chặn/NAT cổng 1883 không? | Kịch bản ngày thi | Đã quyết: phát WiFi từ điện thoại, không dùng mạng hội trường. |
| Store-and-forward chạy thật trên board ra sao? | AC-03.6 — mục quan trọng nhất còn lại | Cần board thật, diễn tập rút mạng 60 giây. |
