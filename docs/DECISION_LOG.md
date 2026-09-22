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

### QĐ-010 · 06/09/2026 · Đã chốt, phát hiện trên phần cứng thật
**Sau khi nối lại MQTT, chờ 20 giây rồi mới đẩy bù (`LINK_GRACE_MS`).**

Phát hiện khi chạy thật trên board: ngắt broker 60 giây rồi bật lại thì ESP32 nối lại sau ~5 giây, nhưng **Node-RED mất tới 15 giây** mới nối lại. ESP bắn nguyên 33 gói buffer vào một broker chưa có ai subscribe — MQTT QoS 0 không lưu cho subscriber offline nên mất sạch, trong khi log phía ESP vẫn báo `day bu 33 goi, con lai 0`. Nhìn Serial thì tưởng thành công, nhìn Grafana mới thấy thủng lỗ 78 giây.

Đây là mặt trái nguy hiểm của chính cơ chế store-and-forward: nó gom dữ liệu lại rồi bắn hết đúng vào thời điểm mong manh nhất.

*Cách sửa:* sau khi nối lại thì vẫn tiếp tục ghi vào spool thêm 20 giây, hết khoảng đó mới đẩy bù một lượt. Giữ nguyên thứ tự, không mất gói.
*Đánh đổi:* dữ liệu lên chậm hơn ~20 giây sau mỗi lần nối lại. Với demo và với giám sát pin thì không đáng kể.
*Đã kiểm chứng:* chạy lại đúng kịch bản đó trên board thật → 83 điểm liên tục, không còn lỗ hổng nào.

### QĐ-011 · 06/09/2026 · Ghi nhận phần cứng
**Nạp firmware qua cổng COM bằng cáp USB-A sang USB-C, không dùng cáp C-to-C.**

Cổng COM của board thiếu điện trở CC 5.1k nên cáp C-to-C **không cấp được nguồn** (cắm vào không có đèn). Cáp A-to-C thì chạy bình thường: chip CH343 (`1a86:55d3`) lên đúng, tự vào mode nạp qua DTR/RTS, không phải bấm BOOT/RESET.

Cổng USB native cũng nạp được nhưng **không tự vào mode nạp**, phải bấm nút bằng tay.

*Ghi lại vì:* ngày thi mà cầm nhầm cáp là mất 20 phút loay hoay. **Mang theo đúng sợi A-to-C đã thử.**

### QĐ-012 · 11/09/2026 · Đã chốt
**Mô hình nhìn TỪNG CELL qua 16 đặc trưng tương đối, không nhìn cả pack.**

Bộ UPC có pack 36 cell, phần cứng của đội có 8 cell. Cho mô hình ăn thẳng
"nhiệt độ 36 cell" thì train xong không deploy được. Thay vào đó mỗi cell được
mô tả bằng 16 con số kiểu *"cell này lệch khỏi phần còn lại của pack ra sao"*.

*Được gì:* train trên pack 36, chạy trên pack 8 không sửa gì; mỗi thời điểm
cho ra N mẫu huấn luyện thay vì 1; mô hình chỉ đúng cell nào bất thường; và
chỉ cần 356 tham số. Dữ liệu cũng được ép về đúng phần cứng trước khi train
(1 cảm biến/cell, 1 Hz, bước lượng tử 0,0625 °C của DS18B20).

*Đánh đổi:* mô hình không thấy được kiểu lỗi mà CẢ pack cùng bất thường — vì
mọi đặc trưng đều là tương đối. Đây là lý do ngưỡng cứng 60 °C phải giữ.

### QĐ-013 · 11/09/2026 · Đã chốt, khác với kế hoạch ban đầu
**Không dùng TFLite Micro. Firmware tự nhân 4 lớp dense bằng tay.**

Kế hoạch ban đầu là INT8 + TFLite Micro để lọt giới hạn 50 KB. Nhưng mô hình
chỉ có 356 tham số: float32 đã là **1,4 KB**, nhỏ hơn giới hạn 35 lần. Lượng
tử hoá để "cho vừa" là giải quyết một vấn đề không tồn tại.

Forward pass viết tay hết ~30 dòng C, không phụ thuộc thư viện nào, và **kiểm
chứng được khớp từng số với Python**. TFLite Micro trên Arduino-ESP32 kéo theo
hàng trăm KB flash, một arena bộ nhớ phải tự chỉnh, cộng sai số lượng tử hoá
phải đi giải trình — rủi ro không đáng rước 4 ngày trước bán kết.

*Số đo thực tế:* AI thêm 5,5 KB flash và 2,9 KB RAM vào firmware.
*Vẫn giữ:* bản INT8 .tflite trong ai/models/ nếu sau này muốn đổi hướng.

### QĐ-014 · 11/09/2026 · Đã chốt
**Hạng nhiệt trong pack dùng HẠNG TRUNG BÌNH khi hoà, không phá hoà bằng chỉ số.**

`np.argsort` dùng quicksort (không ổn định) nên khi hai cell bằng nhau thì kết
quả tuỳ nội bộ numpy — mà sau khi lượng tử hoá về bước 0,0625 °C thì bằng nhau
xảy ra rất thường xuyên, và firmware không tài nào tái tạo được.

Phá hoà bằng chỉ số cell cũng sai: đánh số cell là do mình đặt, không mang
thông tin vật lý nào, nên cell 0 luôn bị xếp dưới là thiên lệch vô nghĩa.

*Chốt:* `rank[i] = (số cell lạnh hơn + (số cell bằng − 1)/2) / (N−1)`.
Xác định, không phụ thuộc thứ tự đánh số, và viết bằng C chỉ vài dòng.

### QĐ-015 · 11/09/2026 · Đã chốt
**Độ lệch chuẩn trượt tính lại từ vòng đệm mỗi bước, KHÔNG dùng tổng chạy.**

Lần đầu viết bằng công thức `E[x²] − E[x]²` với tổng chạy float32. Test đối
chiếu bắt được lệch **1,3e-3** so với Python trong khi các đặc trưng khác lệch
cỡ 1e-9. Nguyên nhân: nhiệt độ ~25 bình phương rồi cộng 60 mẫu ra ~37.500,
trong khi phương sai thật chỉ ~0,001 → float32 mất sạch chữ số có nghĩa. Tổng
chạy còn tích luỹ sai số vô hạn theo thời gian vì cứ cộng vào rồi trừ ra.

*Chốt:* tính lại hai lượt (trung bình rồi phương sai) từ vòng đệm, tích luỹ
bằng double. 60×8 phép tính mỗi giây — không đáng kể. Sau khi sửa: lệch 5e-10.

### QĐ-016 · 11/09/2026 · Đã chốt
**Lớp 2 chỉ dùng đặc trưng PHA SẠC, không dùng dung lượng phóng.**

Hai lý do. Thứ nhất, nhãn RUL được tính TỪ dung lượng — đưa dung lượng vào đầu
vào là cho mô hình nhìn trộm đáp án. Thứ hai, quan trọng hơn: đo dung lượng
thật đòi hỏi phóng kiệt pin theo dòng cố định, việc không bao giờ xảy ra với
một chiếc xe điện đang chạy ngoài đường.

Pha sạc thì ngày nào cũng có, và ESP32 đo được đủ điện áp/dòng/nhiệt độ.

*Bẫy đã gặp:* thời gian sạc CC **thô** không dùng được — đo ra 11 phút ở chu kỳ
này, 38 phút ở chu kỳ khác, nhưng do pin còn bao nhiêu lúc cắm sạc chứ không
phải do lão hoá. Nên mọi đặc trưng thời gian đo trên **khoảng điện áp cố định**
(3,9 → 4,15 V), tính từ lúc đạt mốc điện áp chứ không từ lúc cắm dây.

### QĐ-017 · 11/09/2026 · Đã chốt, NGƯỢC với kế hoạch ban đầu
**Lớp 2 dùng hồi quy tuyến tính 1 đặc trưng, KHÔNG dùng LSTM.**

Tài liệu đề xuất LSTM/CNN-LSTM. Đã làm cả hai và so bằng leave-one-battery-out.
Kết quả (MAE, số chu kỳ): tuyến tính trên `t_cv` **12,2** · GBM 17,2 · LSTM
20,3 · Ridge 9 đặc trưng 27,0 · baseline đoán trung bình 25,8.

**Mô hình 2 tham số đánh bại LSTM 3.500 tham số.** Ridge còn tệ hơn cả baseline.

*Nguyên nhân, đã kiểm chứng:* bốn pin có tuổi thọ 60/77/105/123 chu kỳ. B0006
(60) ngắn hơn mọi pin trong tập train, và đúng là chỗ mọi mô hình phức tạp sụp
(LSTM 42,8 · GBM 35,7 · Ridge 32,4 · tuyến tính 6,4). LSTM không học "pin già
thì pha CV dài ra" — nó học thuộc tuổi thọ của đúng 3 viên pin được cho xem.

*Giới hạn của kết luận này:* chỉ đúng ở quy mô 4 viên pin. Có 50 viên thì LSTM
rất có thể thắng. Phải nói là *"với dữ liệu chúng em có"*, không phải *"LSTM
vô dụng"* — nói kiểu sau là sai và sẽ bị hỏi lại.

*Hệ quả cho slide:* SOH (sai số 3,8 điểm phần trăm) đáng tin hơn RUL và cũng là
con số khách hàng định giá xe cũ thật sự cần. Nên dẫn bằng SOH.

### QĐ-018 · 11/09/2026 · Đã chốt
**Mô hình Lớp 2 chạy trong Node-RED bằng hệ số nhúng sẵn, không dựng dịch vụ riêng.**

RUL là hồi quy tuyến tính 2 tham số, SOH là Ridge 10 tham số — cả hai chỉ là
tích vô hướng. Nhúng hệ số vào một node function là xong. Dựng thêm container
Python chỉ để nhân 10 số là tự rước thêm một thứ có thể hỏng vào ngày thi.

*Ranh giới thiết bị / cloud:* ESP32 chỉ gửi 9 con số tóm tắt mỗi chu kỳ sạc,
cloud tính. Nhờ vậy đổi mô hình không phải nạp lại firmware — cần thiết vì mô
hình còn phải hiệu chỉnh lại cho pack thật.

### QĐ-019 · 11/09/2026 · Đã chốt
**Mọi dự đoán Lớp 2 phải kèm cờ cảnh báo NGOẠI SUY.**

Phát hiện khi chạy thật: mô hình tuyến tính **không bao giờ từ chối trả lời**.
Cho nó đầu vào cách trung bình huấn luyện 5 độ lệch chuẩn, nó vẫn trả về một
con số trông hợp lý. Lần chạy đầu RUL ra 0 và SOH ra 105% mà không có gì báo
là đang ngoại suy — chỉ vì profile sạc mô phỏng chưa đúng thực tế.

*Chốt:* node tính kiểm |z| > 3 cho từng đặc trưng, cảnh báo vào log và ghi cờ
`Extrapolating` lên dashboard. Đây là thứ cần nhất lúc gắn pack thật, vì khi đó
đầu vào chắc chắn khác bộ NASA.

### QĐ-020 · 11/09/2026 · Đã chốt
**Ngưỡng kết thúc sạc lấy 0,2 A (~C/10), không lấy 0,02 A của NASA.**

0,02 A là giao thức phòng thí nghiệm. Bộ sạc thương mại ngắt quanh C/20–C/10,
và cái đuôi dòng rất thấp phía sau dài bao lâu là do **cài đặt bộ sạc** chứ
không phải do sức khoẻ pin. Lấy tới 0,02 A là nhét đặc tính thiết bị đo vào
đặc trưng của pin.

*Lưu ý trung thực:* lựa chọn này cũng cho điểm tốt hơn (MAE 12,1 vs 16,8),
nhưng lý do chọn là khả dụng ngoài đời — nếu chọn vì điểm thì đó là tinh chỉnh
trên tập test.

### QĐ-021 · 14/09/2026 · Đã chốt
**Giữ Wi-Fi làm kênh chính lên cloud. BLE là kênh BỔ SUNG, làm sau Bán kết.**

Cô gợi ý ESP32 chạy BLE bắn thẳng lên điện thoại thay vì qua Wi-Fi/gateway.
Hai điều cần tách:

- *Về gateway:* kiến trúc hiện tại **không có gateway** — Pi đã bỏ từ §1. Đường
  đi là ESP32 → Wi-Fi → cloud, một chặng.
- *Về tiết kiệm điện:* BLE tiết kiệm thật (~10 mA so với ~100 mA), nhưng trên
  pack 86 Wh thì Wi-Fi liên tục vẫn trụ ~11 ngày, trong khi xe được sạc mỗi
  1–3 ngày. **Điện không phải ràng buộc quyết định** — đừng lấy nó làm lý do
  chính trên sân khấu, sẽ bị hỏi ngược.

Lý do thật sự để không đổi:
1. BLE chỉ chạy khi có người cầm điện thoại đứng gần — mà lúc nguy hiểm nhất
   là **sạc qua đêm không ai trông**.
2. BLE không thay được cloud, chỉ đổi người đưa thư; quản lý đội xe vẫn cần
   dashboard.
3. Điểm Bán kết phụ thuộc lớn vào mức độ dùng WISE-IoT. Làm dữ liệu lên cloud
   đứt quãng là tự bỏ điểm.
4. BLE hợp nhất với người dùng cá nhân — mà §5 xếp hạng 3 và khuyến nghị không
   làm khách hàng chính. Chọn BLE làm trung tâm là âm thầm đổi khách mục tiêu.

**Không lấy "sắp tới ngày thi" làm lý do.** Kiến trúc chọn vì kịp deadline là
kiến trúc phải làm lại ngay sau đó. Ba lý do trên đúng bất kể còn bao nhiêu
ngày. BLE vẫn nên làm — bản tối giản qua GATT, đọc bằng nRF Connect, không cần
viết app — nhưng xếp sau việc thay nốt các phần còn đang mô phỏng.

Phân tích đầy đủ và phân vai 3 nhóm người dùng:
`docs/NGUOI_DUNG_VA_KICH_BAN.md`.

### QĐ-022 · 14/09/2026 · Đã chốt
**Phải hiệu chỉnh offset 8 cảm biến DS18B20 trước khi cho Lớp 1 ăn số thật.**

Đo 60 s với cả 8 đầu dò trong cùng khối không khí: nhiễu mỗi kênh chỉ
0,00–0,03 °C, nhưng **sai lệch giữa các kênh là 0,575 °C** (P05 cao nhất
+0,41 °C). Nằm trong dải ±0,5 °C của datasheet nên cảm biến không hỏng.

Vấn đề: Lớp 1 nhìn **chênh lệch tương đối giữa cell** (QĐ-012), nên một sai
lệch cố định trông y hệt "cell này lúc nào cũng nóng hơn". Theo chính
`models/eval_results.npz`, AE phát hiện lỗi offset 0,5 °C với tỉ lệ **41,7 %**
→ cắm thẳng vào là có ~4/10 khả năng P05 báo động giả vĩnh viễn.

Cách làm: đo trung bình mỗi kênh ~60 s trong môi trường cân bằng, lưu
`offset[i] = mean[i] − mean(all)` vào firmware, trừ đi khi đọc.
**Phải đo lại SAU khi dán đầu dò lên pin** — lúc đó sai số còn gồm tiếp xúc
nhiệt của từng mối dán, và đó mới là phần lớn. Bảng đo hiện tại (đầu dò để rời
ngoài không khí) chỉ dùng để chứng minh vấn đề tồn tại, không dùng để nạp.

Kèm theo: đọc cảm biến phải **không chặn**
(`setWaitForConversion(false)`) — chuyển đổi 12 bit tốn ~750 ms, chặn từng ấy
mỗi giây thì `mqtt.loop()` rớt keep-alive.

Bằng chứng: `docs/BANG_CHUNG_CAM_BIEN_2026-09-14.md`.

> ⛔ **ĐÍNH CHÍNH 14/09 (cùng ngày) — quyết định này nói quá.**
> Bản đầu khẳng định "sai lệch 0,575 °C, **bắt buộc** hiệu chỉnh". Cả con số
> lẫn chữ "bắt buộc" đều chưa có cơ sở.
>
> Đo thêm ở hai lần chạy sạch khác nhau thì bảng offset **xáo trộn hoàn toàn**:
> P04 đổi **0,359 °C**, P07 đổi 0,211 °C, P02 đổi 0,205 °C. Sai số chế tạo của
> cảm biến là **hằng số vật lý**, không thể đổi; nhiễu đọc chỉ 0,03 °C nên cũng
> không phải nhiễu.
>
> → Cái đo được **không phải sai số cảm biến**, mà là chênh lệch nhiệt độ THẬT
> giữa các vị trí 8 đầu dò đang nằm. Để rời mỗi cái một chỗ thì mỗi cái ở trong
> một luồng không khí khác nhau — chênh 0,3 °C giữa hai điểm cách nhau 20 cm
> trong phòng là bình thường.
>
> **Không khí không phải môi trường hiệu chuẩn hợp lệ.** Hiện **chưa biết** 8
> con này có lệch nhau hay không, vì phép đo bị môi trường lấn át hoàn toàn.

Quy trình đo đúng: **bó cả 8 đầu dò thành một cụm, nhúng vào cốc nước** ở nhiệt
độ phòng, khuấy, đợi 10 phút rồi mới đo 5 phút. Nước dẫn nhiệt hơn không khí
hàng trăm lần nên mới ép được cả 8 con về cùng một nhiệt độ thật. **Làm hai
lần; hai bảng phải khớp trong ~0,05 °C** thì mới tin.

Nếu độ rộng đo ra < 0,1 °C thì **bỏ qua hiệu chỉnh**. Chỉ khi cỡ 0,5 °C mới cần
làm, vì lúc đó autoencoder bắt lỗi offset 0,5 °C ở tỉ lệ 41,7 %
(`models/eval_results.npz`) — tức là báo động giả vĩnh viễn.

### QĐ-023 · 14/09/2026 · Đã chốt *(đã sửa lại trong ngày — xem phần đính chính)*
**Không tin cờ `isParasitePowerMode()`. Phải hỏi nguồn từng con bằng `readPowerSupply(rom)`.**

> ⛔ **ĐÍNH CHÍNH — bản đầu của quyết định này SAI.**
> Bản đầu viết: *"8 cảm biến đang chạy nguồn ký sinh, phải đấu lại 3 dây"*.
> Thực tế đã đấu 3 dây đúng từ đầu (hàng VCC, hàng GND, hàng tín hiệu, trở
> 4,7 kΩ). Tớ kết luận về phần cứng chỉ từ **một dòng log**, không kiểm chứng.
> Giữ lại nguyên văn sai lầm này vì nó có ích hơn là xoá đi.

Sự thật đo được (`test/ds18b20_power_diag`): hỏi riêng từng con 200 lần, tổng
**1 600 lần hỏi, 0 lần báo ký sinh**. Cả 8 con đều có nguồn riêng.

Vậy vì sao `isParasitePowerMode()` từng trả về `true`? Đọc mã
DallasTemperature 4.0.6:

```cpp
// parasite = false CHỈ nằm trong setOneWire(), begin() không bao giờ tắt nó
if (!parasite && readPowerSupply(deviceAddress)) parasite = true;
```

Cờ **chỉ có chiều bật**, và quyết định bằng **một lần hỏi duy nhất** cho mỗi
con lúc quét bus. Một lần nhiễu thoáng qua đúng khoảnh khắc đó là cờ bật và giữ
nguyên **suốt cả phiên chạy** — từ đó thư viện xử lý cả 8 con theo kiểu ký sinh
dù dây nối đúng hoàn toàn.

Đây là lý do lỗi có hình dạng **chốt** chứ không suy giảm dần: hoặc 0 %, hoặc
~33 %, không có giá trị ở giữa.

Quy tắc rút ra: **một cờ tổng hợp của cả bus không bao giờ đủ để kết luận về
phần cứng.** Có API hỏi từng thiết bị thì phải dùng.

### QĐ-024 · 14/09/2026 · Đã chốt
**Firmware phải ĐẾM và BÁO lỗi đọc cảm biến lúc chạy, không được nuốt im lặng.**

Đã gặp bus lỗi **31,86 %** rồi **35,42 %** trên hai lần chạy liên tiếp, sau đó
**không tái hiện được** qua 25 lần khởi động và 2 lần chạy 5 phút — không ai
sửa gì phần cứng ở giữa. Lỗi chập chờn, nghi phạm số một là tiếp xúc breadboard.

Không truy được nguyên nhân không có nghĩa là bỏ qua. Ngược lại: trên xe thật
có rung động thì dây **chắc chắn** sẽ có lúc tiếp xúc kém, nên phần mềm phải
sống chung với nó. Với một hệ giám sát an toàn, đọc sai 1/3 số lần mà **không
báo gì cả** là chế độ hỏng tệ nhất có thể có — tệ hơn cả chết hẳn, vì chết hẳn
thì còn biết mà sửa.

Yêu cầu với firmware chính:
1. Đếm `DISC` / CRC / giá trị 85,0 °C **theo từng kênh**, không gộp.
2. Đẩy tỉ lệ lỗi lên cloud như một tag bình thường (không đổi data contract —
   chỉ thêm tag mới trong `d`), và vẽ lên dashboard.
3. Một kênh vượt ngưỡng lỗi thì **loại kênh đó khỏi đầu vào Lớp 1** và báo
   "cảm biến hỏng", chứ không đưa số rác cho AI.
4. Kiểm nguồn bằng `readPowerSupply(rom)` từng con lúc khởi động (QĐ-023), ghi
   log nếu có con nào báo ký sinh.

Cũng vì lý do này: **bỏ breadboard trước khi gắn lên pack** — hàn thẳng hoặc
dùng terminal block bắt vít.

### QĐ-025 · 14/09/2026 · Đã chốt
**Hiệu chuẩn offset cảm biến trong NƯỚC, lúc rời pack. TUYỆT ĐỐI không hiệu
chuẩn lại sau khi đã dán lên pack.**

Đây là chỗ tớ (AI) đã khuyên **ngược** ở bản đầu — bản đầu viết *"đo lại lần
cuối sau khi dán lên pack, bảng đó mới là bảng nạp vào firmware"*. Lời khuyên
đó nguy hiểm, và lý do đáng ghi lại.

Cái ta cần sửa là **sai số của dụng cụ đo** — mỗi con DS18B20 lệch một ít do
chế tạo (đo được 0,3665 °C, datasheet ±0,5 °C). Sai số đó là hằng số, đo trong
nước là đo đúng nó.

Nếu hiệu chuẩn lúc cảm biến đã dán lên pack thì trong số đo có thêm hai thứ
khác: chênh lệch nhiệt độ **thật** giữa các cell, và chất lượng tiếp xúc của
từng mối dán. Trừ đi cả cụm đó thì hỏng theo hai đường:

1. **Xoá mất chính tín hiệu cần tìm.** Lớp 1 phát hiện bất thường bằng chênh
   lệch tương đối giữa các cell. Cân bằng phẳng chênh lệch đó bằng hiệu chuẩn
   là làm mù mô hình.
2. **Nguy hiểm hơn: đóng băng một lỗi có sẵn thành "bình thường".** Nếu lúc
   hiệu chuẩn đã có một cell tiếp xúc kém đang nóng hơn, phép hiệu chuẩn ghi
   nhận cái nóng đó là mức nền của cell ấy — và từ đó **vĩnh viễn không bao giờ
   phát hiện được nữa**. Đúng cell nguy hiểm nhất lại là cell bị làm mù.

Còn chênh lệch nhiệt độ thật giữa các vị trí trong pack thì **để nguyên cho mô
hình thấy** — Lớp 1 train trên dữ liệu pack thật (bộ UPC 36 cell) nên đã gặp
kiểu chênh lệch không gian đó rồi. Thứ nó chưa từng gặp là sai số **dụng cụ**,
và đó đúng là thứ duy nhất nên trừ đi.

Quy tắc chung: **chỉ hiệu chuẩn cái thuộc về dụng cụ đo, không bao giờ hiệu
chuẩn cái thuộc về đối tượng đo.**

Quy trình đã kiểm chứng: bó 8 đầu dò thành cụm, nhúng nước nhiệt độ phòng,
khuấy, đợi ổn định, chạy `ds18b20_stress_test` 5 phút. **Làm hai lần**; hai
bảng phải khớp trong 0,05 °C. Thực tế đạt 0,025 °C. Chỉ làm lại khi thay cảm
biến.

### QĐ-026 · 14/09/2026 · Đã chốt
**Pack thật là 8S × 18650 2,55 Ah, không phải LG HG2 3 Ah. Và cách kiểm chứng
Lớp 2 đổi từ "lão hoá pack" sang "mượn pin đã chai sẵn".**

*Đính chính thông số:* tài liệu ghi pack là LG HG2 3 Ah — đó là **kế hoạch mua**,
không phải thứ đã mua. Thực tế: **18650, 2,55 Ah**, 8 viên, 2 đế 4 cell nối
tiếp (8S). Đã sửa ở `charge_cycle.h`, `ai/README.md`,
`docs/NGUOI_DUNG_VA_KICH_BAN.md` (phép tính Wh cũng đổi: 86 → 73 Wh).

**Việc này làm Lớp 2 khả thi hơn hẳn so với đánh giá trước đó.** NASA PCoE dùng
**cùng loại cell 18650**, 2,0 Ah. Lệch dung lượng chỉ 27 %, không phải "khác
hẳn dòng cell" như giả định cũ. Dáng đường sạc do hoá học + hình dạng cell
quyết định, nên mượn hệ số NASA giờ có cơ sở vật lý, không còn là chắp vá.

**Điều kiện bắt buộc: sạc ở cùng tốc độ C.**
```
NASA : 1,5 A / 2,00 Ah = 0,75C
đội  : 0,75 × 2,55     = 1,9 A
```
Sạc ở dòng khác thì `t_cv` và `dvdt_cc` lệch **hệ thống** — sai theo một hướng
cố định, kiểu sai khó phát hiện nhất vì kết quả vẫn trông hợp lý.

### Vì sao KHÔNG thể hiệu chuẩn RUL trên pack của đội

Hiệu chuẩn cần các cặp *(đặc trưng sạc, tuổi thọ còn lại thật)*. Vế sau chỉ
biết được sau khi lão hoá pack tới mốc còn 80 % dung lượng — hàng trăm chu kỳ,
tính bằng tháng. Đây là **bản chất bài toán**, không phải chuyện thiếu thời
gian hay thiếu chăm chỉ. Không có mẹo nào rút ngắn được.

### Đường vòng: mượn pin đã chai sẵn

Không lão hoá được pin thì **lấy pin đã chai rồi**. Mua/xin một nhúm 18650 cũ
(pin laptop tháo ra là nguồn rẻ nhất) có độ chai khác nhau, rồi với **từng viên**:

1. Đo dung lượng thật — xả dòng cố định, tích phân dòng theo thời gian.
   → đây là **SOH thật**, nhãn ground-truth.
2. Ghi đường sạc của chính viên đó → 9 đặc trưng.
3. So SOH mô hình đoán với SOH đo được.

Biến bài toán **hàng tháng** thành bài toán **một buổi chiều**. Chỗ then chốt:
mô hình cần pin ở nhiều mức sức khoẻ khác nhau, chứ không cần chính pin của đội
phải già đi.

*Giới hạn phải nói rõ:* pin cũ tháo ra thường khác model, khác hoá học, nên
phương sai lớn hơn một bộ dữ liệu phòng thí nghiệm. Và cách này kiểm được
**SOH**, còn RUL thì chỉ suy ra gián tiếp từ xu hướng SOH.

*Vì sao vẫn đáng làm:* SOH mới là phần có nội dung vật lý; RUL chỉ là phép ngoại
suy xu hướng SOH. Kiểm được SOH trên pin thật là đã kiểm được phần lõi.

### Thứ tự ưu tiên cho Lớp 2

1. Đo 1–2 chu kỳ sạc **thật** ở 1,9 A → chứng minh 9 đặc trưng đo được ngoài
   đời và ra đúng dải (cần INA228 + đo áp pack).
2. Mượn pin chai để kiểm SOH như trên.
3. Kiểm chéo trên bộ dữ liệu thứ hai có nhãn dung lượng thật (CALCE / Oxford).
   **Bộ UPC đã tải KHÔNG dùng được** cho việc này: cả 410 file đều là bản
   `partial_data`, chu kỳ đo dung lượng chỉ còn đoạn dòng ±2 A trong 10 giờ với
   SoC đứng yên ở 99 %; tích phân ra 0,008 Ah, vô lý với pack EV. Bộ này vẫn
   tốt cho Lớp 1 — và đó đúng là việc đã dùng nó.

### QĐ-027 · 14/09/2026 · Đã chốt — trả lời phản biện "đo từng cell để làm gì?"
**Lớp 1 đo từng cell vì AN TOÀN và CHẨN ĐOÁN, không phải để thay lẻ từng cell.
Lớp 2 vốn đã là mức PACK, không phải mức cell.**

Phản biện từ thành viên trong đội, và nó đúng chỗ: *"phát hiện được một cell
nóng thì thay mỗi cell đó à hay thay cả cục? Nếu 7 cell 85 % mà 1 cell 80 %,
thay mỗi cell 80 % thì vài hôm nữa 7 cell kia cũng phải thay."*

#### Đính chính sự thật: Lớp 2 KHÔNG đo từng cell

`charge_cycle.cpp` nhận **điện áp PACK** rồi chia cho 8 để quy về điện áp trung
bình mỗi cell (`pack_voltage / CC_N_CELLS`). Đầu ra là **một** `RUL_Cycles` và
**một** `SOH_Percent` cho cả pack. Không có RUL theo từng cell, chưa bao giờ có.

Nên kịch bản "7 cell 85 %, 1 cell 80 %" không phải thứ hệ thống hiện tại sinh
ra. Phản biện đúng về nguyên tắc nhưng nhắm sai đối tượng.

*Vì sao mức pack là đúng:* pack nối tiếp có dung lượng dùng được **bằng dung
lượng của cell yếu nhất** — cell yếu chạm ngưỡng cắt trước, cả pack dừng theo.
Nên SOH pack đã tự động phản ánh cell yếu nhất. Đo riêng từng cell rồi báo cáo
8 con số RUL là vừa thừa vừa gây hiểu nhầm.

#### Lớp 1: hành động KHÔNG phải "thay cell đó"

Khi Lớp 1 báo một cell nóng bất thường, việc phải làm là **ngắt sạc, cách ly
pack, đưa ra xa người** — ngay lập tức, và hoàn toàn không liên quan tới bài
toán kinh tế thay thế. Đây là lý do tồn tại số một, và nó đứng vững bất kể sau
đó thay cell hay thay pack.

Vì sao phải đo từng cell mới làm được: một cell trong tám nóng lên chỉ kéo
**trung bình pack** lên khoảng 1/8 mức bất thường. Cảm biến đo trung bình sẽ
thấy tín hiệu bị pha loãng 8 lần — đúng lúc cần nhạy nhất.

#### Giá trị thứ hai: phân biệt hai kiểu hỏng khác nhau

| Dấu hiệu | Nguyên nhân | Việc phải làm |
|---|---|---|
| **Một** cell lệch hẳn | lỗi sản xuất, mối hàn xấu, hỏng cục bộ | bảo hành / sửa cell đó / phản hồi nhà cung cấp |
| **Cả tám** cùng xuống đều | lão hoá bình thường | hết đời, thay pack |

Hai tình huống này cần hai hành động hoàn toàn khác nhau, và **không thể phân
biệt nếu chỉ đo pack**. Đây là giá trị chẩn đoán, độc lập với giá trị an toàn.

#### Còn chuyện thay lẻ thì sao — tài liệu đã nói hớ

Bạn trong đội đúng ở chỗ: **cắm một cell MỚI TINH vào pack đã chai là sai.**
Cell mới dung lượng cao hơn, 7 cell cũ vẫn là nút thắt, và chênh lệch còn làm
mất cân bằng nặng thêm.

Nhưng kết luận "vậy phải thay cả pack" cũng không đúng. Tài liệu nghiên cứu về
sửa chữa pack nói rõ hai điều:

1. Thay toàn bộ cell vì **một** cell hỏng sớm là **không khả thi về kinh tế**
   với pack lớn — nên phải có phương án khác.
2. Phương án đúng là **giữ kho cell đã lão hoá ở nhiều mức khác nhau, rồi chọn
   cell có độ chai KHỚP với pack** để thay.

Chiếu vào kịch bản 7 cell 85 % + 1 cell 80 %: không thay bằng cell 100 %, mà
thay bằng cell **~85 %**. Pack từ 80 % (bị cell yếu nhất chặn) lên 85 %. Rẻ,
và có thật.

Mà muốn làm được việc đó thì **bắt buộc phải biết sức khoẻ từng cell** — cả
của pack đang sửa lẫn của kho cell dự trữ. Đây chính là đầu vào mà Lớp 1 tạo ra.

*Ai làm được việc này:* đúng khách hàng số 1 ở §5 — đơn vị vận hành đội xe và
trạm đổi pin. Họ sở hữu hàng trăm pack, có đồ nghề, và tân trang tập trung.
Người dùng cá nhân thì không, và đó là một lý do nữa để không chọn B2C.

*Nguồn:* Journal of Remanufacturing (2020), "Battery pack remanufacturing
process up to cell level"; Batteries 6(3):39, "Cell Replacement Strategies for
Lithium Ion Battery Packs".

#### Hệ quả cho cách trình bày

Câu **sai**: *"AI của chúng em chỉ ra cell nào cần thay."*
Câu **đúng**: *"Lớp 1 phát hiện cell bất thường để NGẮT SẠC kịp thời, và để
phân biệt lỗi một cell với lão hoá toàn pack. Lớp 2 báo sức khoẻ ở mức PACK,
vì pack nối tiếp sống chết theo cell yếu nhất."*

---

## Ẩn số còn treo

| Ẩn số | Chặn việc gì | Gỡ bằng cách nào |
|---|---|---|
| WISE-IoT có tôn trọng `ts` thiết bị gửi không? | Độ chắc chắn của màn demo trên cloud Advantech | Chờ tài khoản, bắn 1 gói backdate. Plan B đã có đường lùi. |
| WiFi hội trường có chặn/NAT cổng 1883 không? | Kịch bản ngày thi | Đã quyết: phát WiFi từ điện thoại, không dùng mạng hội trường. |
| ~~Store-and-forward chạy thật trên board ra sao?~~ | **Đã gỡ 06/09** — chạy thật thành công, xem `BANG_CHUNG_PHAN_CUNG_2026-09-06.md`. Tìm ra thêm QĐ-010. | |
| Mạng 4G / WiFi phát từ điện thoại có ổn không? | Kịch bản ngày thi | Thử trước, cùng lúc với AC-04.6 trên VPS. |

---

### QĐ-028 · 15/09/2026 · Đã chốt — báo động tại chỗ có 4 mức, không phải một cờ bật/tắt

**Bối cảnh.** Trước hôm nay hệ chỉ cảnh báo qua Serial và dashboard. Cả hai đều
giả định có người đang nhìn màn hình. Kịch bản nguy hiểm nhất của pack pin lại
là lúc sạc qua đêm trong nhà xe — không ai nhìn màn hình, và mạng thì đúng lúc
đó cũng có thể mất.

**Quyết định.** `alarm.cpp` với 4 mức leo thang (OK / THEO DÕI / BÁO ĐỘNG /
NGUY KỊCH) cộng một trạng thái riêng cho MẤT CẢM BIẾN.

**Bốn điểm đáng ghi lại, vì mỗi cái đều là một chỗ dễ làm sai:**

1. **Không gộp ba mức vào một cái còi kêu/không kêu.** "Đang theo dõi" khác
   "đã báo động" khác "cắt ngay" — gộp lại là ném đi đúng thứ người dùng cần để
   quyết định làm gì.

2. **Ngưỡng cứng 60 °C đi đường RIÊNG, không qua AI.** `Alarm::update()` nhận
   `t_max` như một tham số độc lập với `ai_alarm`. Nhờ vậy autoencoder sai hoàn
   toàn thì ngưỡng cứng vẫn kêu. Đã kiểm: TH-E cho `ai_alarm=false` mà vẫn lên
   NGUY KỊCH.

3. **Trễ 5 °C (60 lên / 55 xuống).** Cùng bài học với `CT_MIN_CONSEC_OK` ở
   `cell_temp.h`: ngưỡng đơn luôn sinh nhấp nháy. Nhiệt độ dao động quanh đúng
   60 °C sẽ làm còi kêu ngắt quãng, và người nghe sẽ tưởng lỗi vặt rồi rút điện
   cho đỡ ồn — tức là ngưỡng đơn tự phá hoại chính nó.

4. **Có nút tắt tiếng, và tắt tiếng KHÔNG tắt đèn.** Một cái còi không tắt được
   sẽ bị rút dây, và lần sau nó không bảo vệ được ai nữa. Tắt tiếng tự huỷ khi
   mức leo lên cao hơn: người dùng tắt tiếng cảnh báo AI không có nghĩa là họ
   đồng ý im lặng khi sau đó pin vượt 60 °C.

**MẤT CẢM BIẾN báo màu xanh dương, không phải đỏ.** "Tôi không còn biết pin thế
nào" là trạng thái khác hẳn "pin đang nguy hiểm" — hai thứ đòi hỏi hành động
khác nhau. Trộn vào cùng màu đỏ sẽ dạy người dùng bỏ qua màu đỏ.

**Bằng chứng.** `test/alarm_bench` — 11/11 phép kiểm đạt, xem
`docs/BANG_CHUNG_BAO_DONG_2026-09-15.md`. alarm.cpp/h trong bench là **symlink**
tới file firmware thật, không phải bản sao, nên bench không thể phân kỳ.

**Chỗ tớ suýt làm sai.** Bản đầu gọi `gAlarm.update()` sau hai nhánh thoát sớm
của `runAI()`. Nghĩa là mất cả bus cảm biến → `runAI()` return → đèn ĐỨNG HÌNH
ở trạng thái cũ → mất hết cảm biến trông y hệt mọi thứ bình thường. Đúng kiểu
hỏng âm thầm mà QĐ-024 được viết ra để chống. Đã sửa: mọi nhánh thoát đều phải
nuôi `gAlarm.update()`.

**Phần cứng.** Còi và LED rời đang đặt mua. `AL_PIN_BUZZER -1` nghĩa là "chưa
có, bỏ qua", nên cùng một firmware chạy được cả trên bàn lẫn trên pack. Logic
đã được thử trước hàng giờ bằng LED RGB sẵn trên DevKitC-1; linh kiện về chỉ
sửa số chân.

---

### QĐ-029 · 15/09/2026 · Rủi ro — IP máy chủ đổi theo DHCP của trường

Hôm nay IP máy chủ nhảy từ `172.172.3.174` sang `172.172.5.2`, làm ESP32 mất
MQTT với `rc=-2`. Đã sửa `LAN_IP` trong `.env` và `TEST_HOST` trong firmware,
dựng lại container mosquitto, kết nối lại OK.

Đây là chuyện sẽ lặp lại, và nếu lặp vào ngày thi thì mất luôn màn demo. Ba
đường xử lý, **chưa chốt cái nào** vì đụng tới thế trận bảo mật:
- Mở `0.0.0.0:1883` thay vì ghim IP — ràng buộc 4 trong CLAUDE.md cho phép
  (1883 có auth), và miễn nhiễm hoàn toàn với DHCP. Đổi lại là mở ra mọi giao
  diện, kể cả các mạng Docker.
- Dùng mDNS (`hutieu.local`) — không phải sửa gì khi IP đổi, nhưng nhiều WiFi
  công cộng chặn multicast.
- Giữ nguyên, và đưa "kiểm IP" thành một bước trong checklist ngày thi.

Ngày thi đã quyết phát WiFi từ điện thoại (xem phần Ẩn số), lúc đó IP do điện
thoại cấp và ổn định hơn — nên rủi ro này chủ yếu ảnh hưởng giai đoạn phát
triển. Cần người chốt.

---

### QĐ-030 · 15/09/2026 · Đã chốt — giữ 8 cảm biến ở bản lab; 4 là mức rút tối đa

**Bối cảnh.** Đội tranh luận: 8 cảm biến vướng và không kinh tế, chia mỗi cảm
biến cho 2–4 cell có được không? Cả hai phía đều chưa có số.

**Đã đo** (`ai/sensor_count_study.py`, 24 đoạn pack-ảo, chu kỳ 349–356). Lỗi
tiêm vào MỘT CELL THẬT rồi mới lấy trung bình nhóm — đúng thứ tự vật lý.

**Quyết định.** Bản lab giữ **8 cảm biến**. Bản thương mại có thể rút về **4**
nếu nói rõ mất gì. **2 cảm biến bị loại.**

**Ba con số quyết định:**

1. **Trễ phát hiện `ramp` (tiền đề thermal runaway) ở 2 °C/10 phút:**
   8 cảm biến 1 142 s · 4 cảm biến 2 993 s (×2,6) · 2 cảm biến 8 063 s (×7).
   Với sự cố đang leo thang, hơn 100 phút chênh lệch là khoảng cách giữa ngắt
   sạc kịp và ngắt sạc khi đã quá muộn.

2. **`offset` 5 °C:** 8 cảm biến 100 %, 4 cảm biến 96 %, **2 cảm biến 0 %.**
   Cấu hình 2 cảm biến không phát hiện được lỗi tiếp xúc ở bất kỳ độ lớn nào
   đã thử. Đó là hiệu ứng pha loãng: 5 °C trên một cell chia cho 4 còn 1,25 °C.

3. **Chỉ đích danh:** 8 → đúng 1 cell; 4 → 2 cell; 2 → 4 cell. Chiến lược thay
   cell cùng tuổi ở QĐ-027 phụ thuộc vào việc biết cell nào.

**Cái bẫy phải ghi lại.** Tỉ lệ báo oan GIẢM khi bớt cảm biến: 0,0360 % (8) →
0,0005 % (4) → 0,0001 % (2). Nhìn qua tưởng ít cảm biến là tốt hơn. Sai — đó là
triệu chứng của sự mù, không phải của độ chính xác. Cùng phép pha loãng làm nó
không thấy lỗi thật cũng làm nó không thấy nhiễu. Một hệ không bao giờ báo động
có tỉ lệ báo oan 0 % hoàn hảo. **Không bao giờ đọc tỉ lệ báo oan tách rời khỏi
tỉ lệ phát hiện.**

**Nghiên cứu này còn lạc quan ở ba chỗ** (trung bình nhóm là trường hợp tốt; AE
huấn luyện trên 8 kênh nên số N=2 là giới hạn trên; bỏ qua dẫn nhiệt giữa cell)
— cả ba đều nghiêng về làm cấu hình ít cảm biến trông đẹp hơn thực tế, nên kết
luận loại bỏ N=2 càng vững. Chi tiết:
`docs/BANG_CHUNG_SO_CAM_BIEN_2026-09-15.md`.

---

### QĐ-031 · 15/09/2026 · Đã chốt — INA228 đo luôn điện áp pack, không mua gì thêm

**Câu hỏi.** Mua gì để đo điện áp pack?

**Trả lời: không cần mua gì.** INA228 đo điện áp bus tới 85 V bằng ADC 20 bit;
pack 8S tối đa 33,6 V nằm gọn trong dải. Cầu chia áp thì phi tuyến theo nhiệt
độ và làm hỏng độ chính xác; ADS1115 là thừa. Suýt khuyên mua ADS1115 vì ADC
của ESP32 phi tuyến và nhiễu — nhưng INA228 làm tốt hơn hẳn và đã đặt rồi.

**Tiện thể:** thanh ghi CHARGE của INA228 tự tích phân dòng theo thời gian —
đúng phép đếm coulomb cần để đo dung lượng THẬT làm đáp án chấm điểm Lớp 2
(QĐ-026). Một con chip giải cả hai việc.

**Chọn dải shunt RỘNG (ADCRANGE=0, ±163,84 mV), không phải dải hẹp.** Dải hẹp
cho độ phân giải gấp 4 nhưng với shunt 0,015 Ω chỉ đo tới 2,73 A — sạc 1,9 A
thì vừa, nhưng xả sẽ TRÀN, và số tràn là một con số sai trông hoàn toàn hợp lý.
Đổi lại độ phân giải chỉ còn ~21 µA, vẫn thừa thãi.

**Viết driver TRƯỚC khi linh kiện về, và kiểm được luôn.** Phần giải mã thanh
ghi tách thành hàm thuần (`pm_decode24`, `pm_decode40`), không đụng I2C, nên
bench chạy được mà không cần chip: 14/14 đạt. Hai lỗi kinh điển của INA228 —
quên dịch phải 4 bit (mọi số gấp 16 lần) và quên mở rộng dấu 20 bit (dòng xả
thành số dương khổng lồ) — đều cho ra con số TRÔNG NHƯ SỐ ĐO, không thể phát
hiện bằng mắt khi nhìn log. Nếu đợi có chip mới thử, ta sẽ mất hàng giờ nghi
ngờ dây nối và nguồn trước khi nghĩ tới phần mềm.

**Hai chỗ còn nợ, phải nói ra:**
- `PM_R_SHUNT` chưa đo lại. Mọi giá trị dòng tỉ lệ thẳng với nó; module ghi
  0,015 Ω nhưng dung sai ±1 % và vết hàn thêm vài mΩ. Chưa đo thì số Lớp 2 chỉ
  đúng tới ~5 %. (AC-06.33)
- SOC hiện là xấp xỉ tuyến tính từ điện áp, sai >20 điểm phần trăm ở vùng
  30–70 % vì đường OCV lithium phẳng ở giữa. Vẫn tốt hơn hằng số 80,0 cũ vì ít
  nhất nó biến thiên đúng chiều. Làm đúng cần đếm coulomb hiệu chỉnh bằng OCV
  lúc pin nghỉ. (AC-06.34)

---

### QĐ-032 · 15/09/2026 · **Kết quả âm tính** — Lớp 1 không chuyển giao sang pack khác

**Đã làm.** Kiểm Lớp 1 trên bộ dữ liệu pack THẬT có lỗi gây ra cố ý: McMaster
"Battery Pack with Introduced Faults", 72 cell nối tiếp, 1 Hz, CC-BY 4.0,
doi:10.5683/SP3/THZTJC. Zero-shot: không huấn luyện lại, không chỉnh ngưỡng.

**Kết quả: TRƯỢT.** Mô hình báo động trên gần như mọi file. File BÌNH THƯỜNG
US06_15C bị báo 98,7 % thời gian — cao hơn cả file LỖI UDDS_Blocked (12,4 %).
Tỉ lệ báo bám theo mức khắc nghiệt của chu trình lái và nhiệt độ buồng, **không
bám theo nhãn lỗi**. Đó là dấu vân tay của lệch phân bố, không phải của phát
hiện.

**Nguyên nhân đã khoanh được — đây là phần có giá trị nhất.** Đo độ lệch của
từng đặc trưng so với tập huấn luyện:
- Bốn đặc trưng **bối cảnh toàn pack** (`T-amb`, `packT-amb`, `|I|/I_SCALE`,
  `soc`) lệch tới **8,6 sd**.
- Các đặc trưng **tương đối giữa cell** (`dev`, `z`, `rank`, `dev_ema`,
  `roll_std`) chỉ lệch ≤1,6 sd — **chuyển giao tốt**, dù pack khác hẳn.

Vì bốn đặc trưng bối cảnh **giống hệt nhau ở mọi cell**, khi lệch phân bố chúng
đẩy sai số tái tạo của TẤT CẢ các cell lên cùng lúc. Đó là cơ chế làm 100 %
pack-ảo báo động.

**Vô hiệu hoá bốn đặc trưng đó** đưa báo oan ở 25 °C về đúng 0 % — nhưng cũng
**bỏ sót hoàn toàn** lỗi thật `UDDS_Blocked_25C` (0 %), và ở 15 °C thì mọi file
vẫn báo bất kể nhãn. Nên phép sửa này không cứu được kết quả; nó chỉ xác nhận
đúng chỗ hỏng.

**Hai giả thuyết cho ca trượt, CHƯA tách được:** (a) Orion BMS chỉ có độ phân
giải 1 °C, mà `UDDS_Blocked` chỉ làm độ rộng nhiệt tăng 3→4 °C — tín hiệu bằng
đúng một bước lượng tử; DS18B20 của đội mịn gấp 16 lần. (b) ngưỡng và bộ chuẩn
hoá thuộc về pack NASA. Đừng nói đã biết nguyên nhân.

**Một dự đoán của chính mình đã sai.** TH-4 nói pack nóng đều thì Lớp 1 mù;
`Fanoffon` lại bị báo nhiều nhất. Vì "Fanoffon" là tắt-rồi-bật-lại quạt nhiều
lần, tạo chênh lệch không gian thật. TH-4 chưa bị bác bỏ nhưng **vẫn chưa được
kiểm** — đã sửa lại tài liệu cho đúng.

**Ảnh hưởng tới lời nói trước giám khảo.** Không được nói *"AI phát hiện được
bất thường nhiệt trên pack pin nói chung"*. Được nói *"mô hình phải hiệu chỉnh
trên chính pack sẽ giám sát; chúng em đã tìm ra lý do cụ thể"*. Với bài dự thi
thì giới hạn này không chặn đường — hệ giám sát chính pack của đội — nhưng lời
quảng cáo phải khớp với thứ đã chứng minh được.

Chi tiết: `docs/BANG_CHUNG_KIEM_CHUNG_LOP1_THAT_2026-09-15.md`.

---

### QĐ-033 · 15/09/2026 · **ĐÃ CHỐT 15/09** — autoencoder thuần tương đối

**Câu hỏi khởi nguồn.** QĐ-032 kết luận mô hình phải hiệu chỉnh riêng cho từng
pack. Vậy **sản xuất hàng loạt kiểu gì?**

**Ý tưởng.** Vấn đề không phải "mỗi pack một khác", mà là đang **trộn hai loại
thông tin có tính chất khác hẳn nhau vào cùng một mô hình**. Đặc trưng tương đối
giữa cell là phổ quát ("cell này nóng hơn phần còn lại 2 °C" có nghĩa như nhau
trên mọi pack). Đặc trưng bối cảnh tuyệt đối thì gắn chặt với thiết kế tản
nhiệt — và cũng **không cần học**, vì "pack nóng hơn môi trường 25 °C" là một
ngưỡng, không phải một mẫu.

Bỏ 5 đặc trưng tuyệt đối (`T-amb`, `packT-amb`, `|I|/I_SCALE`, `soc`,
`(T-25)/25`), giữ 11 đặc trưng thuần tương đối.

**Dự đoán ghi trước khi chạy: báo oan sẽ tăng, phát hiện sẽ giảm một ít.**
**Dự đoán này SAI.** Kết quả ngược lại hoàn toàn.

**Kết quả trên chính dữ liệu NASA — tốt hơn toàn diện:**
- Báo oan 0,0360 % → **0,0106 %** (thấp hơn 3,4 lần)
- `offset` 2,0 °C: 79 % → **100 %**
- `ramp` 2,0 °C trễ: 1 142 s → **844 s**
- `drift` 5,0 °C: 46 % → **88 %** — điểm yếu nặng nhất, cải thiện gần gấp đôi

Giải thích: nút thắt chỉ 4 chiều. Với 16 đặc trưng, mô hình phải tiêu dung
lượng ít ỏi đó để tái tạo 5 đặc trưng **giống hệt nhau ở mọi cell**, tức không
mang thông tin phân biệt cell nào. Bỏ đi thì cả 4 chiều dành trọn cho cấu trúc
tương đối.

**Kết quả chuyển giao sang pack McMaster:** file lỗi 0,360–1,262 % thời gian,
file bình thường tệ nhất 0,065 % — **tách biệt ~19 lần**, đã dùng được. Cả 5
file bình thường ở 25 °C: đúng 0 %. (So với mô hình cũ: file bình thường
US06_15C bị báo 98,7 %, cao hơn cả file lỗi.)

**Vẫn còn một ca trượt:** `UDDS_Blocked_25C` im lặng hoàn toàn. File đó chỉ làm
độ rộng nhiệt tăng 3→4 °C, đúng một bước lượng tử của Orion BMS.

**Kiến trúc ba tầng cho bản thương mại:** (1) ngưỡng cứng 60 °C, không học, y
hệt mọi máy; (2) autoencoder thuần tương đối, nạp giống hệt nhau khi xuất
xưởng; (3) bối cảnh tuyệt đối xử lý bằng luật vật lý viết tay. Muốn thêm thì
hiệu chỉnh ngưỡng tại chỗ bằng vài giờ vận hành bình thường — autoencoder không
cần nhãn nên tự động hoàn toàn. ⚠️ Chỉ làm trên pack đã biết là tốt, nếu không
thì lỗi sẵn có bị ghi thành "bình thường" vĩnh viễn (cùng bẫy với QĐ-025).

**ĐÃ CHỐT và ĐÃ CHUYỂN.** Người phụ trách duyệt 15/09.

**Điểm vận hành mới, dò bằng `tune_threshold.py --rel` — cùng script, cùng dữ
liệu val, cùng tiêu chí chọn với bản cũ, nên so được trực tiếp:**

| | 16 đặc trưng | 11 thuần tương đối |
|---|---|---|
| Ngưỡng p99.9 | 1,8474 | **1,0707** |
| Phải giữ liên tục | 60 s | **30 s** |
| Trễ ở ramp chậm nhất (0,05 °C/phút) | 138 phút | **104 phút** |
| Báo động giả | 0,0329/giờ = 1 lần/1,3 ngày | **0 lần trên 6 123 080 mẫu-cell** |

Số 0 báo động giả KHÔNG được nói thành "không bao giờ báo oan". Với 0 sự kiện
trên n mẫu, chặn trên 95 % là 3/n (quy tắc số 3) → hiếm hơn 1 lần mỗi 3 ngày.

**Đã làm:** `cell_ae_weights.h` xuất lại (271 tham số, 1,1 KB — nhỏ hơn bản cũ),
`AI_N_FEAT` 16→11, `cell_ai.cpp` bỏ 5 đặc trưng, dashboard Grafana sửa ngưỡng
và mô tả, `ai/README.md` + `HAI_LOP_AI_HOAT_DONG_THE_NAO.md` cập nhật.

**Kiểm:** `test_c_vs_python.py` — C khớp Python trên cả 11 đặc trưng, lệch tối
đa 3,6e-07, sai số tái tạo lệch 1,8e-06, cùng cell tệ nhất 100 %. Test này đã
được sửa để đọc số đặc trưng từ `AI_N_FEAT` thay vì viết cứng 16 — con số viết
cứng làm test vỡ đúng lúc nó cần chạy nhất.

**Chưa kiểm được:** chạy thật trên pack của đội, vì đầu dò còn chưa dán lên pin.
Mọi con số ở trên đều đo trên dữ liệu NASA.

Chi tiết: `docs/BANG_CHUNG_AE_TUONG_DOI_2026-09-15.md`.

---

### QĐ-034 · 15/09/2026 · Đã chốt — ESP32 tự tìm broker, thôi ghim IP (khép QĐ-029)

Máy chủ chạy lúc ở trường, lúc ở nhà, lúc qua hotspot điện thoại. Ghim IP nghĩa
là mỗi lần đổi mạng phải nạp lại firmware, và quên đúng hôm thi thì mất demo.

**Ba đường, thử theo thứ tự** (`broker_find.cpp`) — vì không đường nào tin được
một mình:
1. **mDNS** hỏi dịch vụ `_mqtt._tcp`; avahi tự cập nhật địa chỉ khi IP đổi nên
   viết một lần là xong. Nhưng **nhiều WiFi công cộng chặn multicast**, và hội
   trường thi rất có thể là một trong số đó.
2. **IP lần trước nối được**, lưu trong LittleFS — sống sót qua khởi động lại
   khi mDNS bị chặn.
3. **IP biên dịch sẵn** — đường duy nhất chắc chắn có khi tới mạng mới lần đầu.

Chỉ ghi nhớ địa chỉ **sau khi đã nối thành công** (nhớ địa chỉ chưa kiểm chứng
là tự đặt bẫy cho lần sau), và chỉ ghi khi **khác** giá trị đang lưu (flash có
số lần ghi hữu hạn; nối lại MQTT xảy ra hàng chục lần mỗi giờ khi sóng yếu).

Đã chạy thật: mDNS không thấy (chưa cài file avahi service), tự lùi sang địa
chỉ đã lưu, `[MQTT] KET NOI OK`.

**Phía máy chủ:** `planb_cloud/start.sh` tự dò IP LAN rồi ghi vào `.env` — thôi
sửa tay. Lời giải sạch hơn là `sudo systemctl disable --now mosquitto` rồi xoá
`docker-compose.override.yml`, khi đó compose gốc bind `0.0.0.0:1883` và không
còn phụ thuộc IP; nhưng việc đó cần quyền sudo của người dùng.

---

### QĐ-035 · 15/09/2026 · Đã xác minh — mạng trường ICTU có client isolation

Máy chủ `172.172.5.2` **không ping được** ESP32 `172.172.3.18` (100 % mất gói),
dù hai bên cùng subnet `/19`. Cùng lúc đó máy chủ tự nối được `172.172.5.2:1883`
bình thường, và avahi xác nhận đang quảng bá đúng (trả lời truy vấn mDNS 197
byte). Nên broker không hỏng, cấu hình không sai — **AP của trường chặn thiết
bị nói chuyện trực tiếp với nhau.**

Điều này cũng giải thích luôn vì sao mDNS im: cùng một cơ chế chặn.

Không sửa được bằng code — đó là chính sách của thiết bị mạng. Đội đã chốt từ
trước là **ngày thi phát WiFi từ điện thoại**; chuyện này là bằng chứng thực tế
cho quyết định đó chứ không phải sự cố mới.

Lưu ý: AP khác trong cùng trường (dải `172.172.10.x`) thì KHÔNG chặn — đã nối
MQTT thành công từ đó. Nên hành vi phụ thuộc từng AP, đừng kết luận "mạng
trường luôn hỏng" hay "mạng trường dùng được".

**Việc phải nhớ:** `arduino_secrets.h` hiện để ICTU ở đầu danh sách cho tiện
phát triển. **Trước ngày thi phải đảo `Huynh` lên đầu** — đã ghi cảnh báo ngay
trong file.

---

### QĐ-036 · 19/09/2026 · Đã chốt — pack thí nghiệm xuống 6 cell, không còn 8

Người phụ trách đổi cấu hình pack thí nghiệm từ **8S sang 6S** (19/09). Hồ sơ
bán kết và bộ slide đã viết theo 6 cell.

**Lập luận ở QĐ/PHAN_CUNG "vì sao 8 chứ không phải 4" vẫn đứng vững với 6:** hỏng
một cell thì còn **5 mẫu tham chiếu**, đủ để nói chuyện thống kê (4 cell chỉ còn
3 — đó mới là chỗ bị vặn). Gradient theo vị trí vẫn xuất hiện với 6 viên xếp
hàng, nên Lớp 1 vẫn có mẫu hình không tầm thường để học.

**Một hệ quả có lợi:** 6S sạc đầy là **25,2 V**, nằm dưới trần 26 V của INA219.
Nghĩa là ràng buộc "INA219 không dùng được" ở `PHAN_CUNG_VA_KIEN_TRUC` chỉ còn
đúng với bản 8S. Vẫn nên dùng INA228 như QĐ-031 vì nó đo luôn điện áp pack và
đằng nào lên xe thật cũng cần, nhưng nếu INA228 về muộn thì INA219 **không còn
là đường cụt**.

**⚠️ Firmware hiện vẫn viết cứng cho 8 kênh — CHƯA SỬA.** Năm chỗ phải đổi cùng
lúc, đổi thiếu một chỗ là lệch chỉ số cell âm thầm:

| File | Hằng số | Hiện tại |
|---|---|---|
| `cell_ai.h` | `AI_N_CELLS` | 8 |
| `ds18b20_offsets.h` | `DS_N_PROBES` + bảng ROM | 8 |
| `charge_cycle.h` | `CC_N_CELLS` | 8 |
| `pack_meter.h` | `PM_N_CELLS`, `PM_V_MIN` | 8 · 15,0 V (1,9 V/cell) → 6S phải là **11,4 V** |
| `cell_temp.h` | phần kiểm "phải đủ 8 cảm biến" | 8 |

Trọng số autoencoder **không phải xuất lại**: mô hình chạy trên **từng cell một**
với 11 đặc trưng, số cell chỉ ảnh hưởng tới cách tính các đại lượng tham chiếu
(trung bình, median, rank, spread) — không ảnh hưởng tới kích thước mạng. Đây là
lợi ích trực tiếp của QĐ-033.

**Phải kiểm lại sau khi sửa:** `test/run_test.sh` và `test_c_vs_python.py` với
`AI_N_CELLS = 6`; `alarm_bench` không phụ thuộc số cell nên không đổi.

**Bổ sung 20/09 — dashboard Grafana đã chỉnh theo 6 cell, và sửa một lỗi hiển thị
thật.** `grafana/make_dashboard.py`: tiêu đề "8S"→"6S", regex `Cell0[1-8]`→`0[1-6]`,
`Sensor_Healthy` đổi max 8→6 (trước đó 6/6 cảm biến khoẻ lại hiện màu cam vì ngưỡng
xanh đặt ở 8).

Lỗi thật tìm ra khi dựng thử để chụp ảnh: panel **"Cell nóng nhất"** hiện 6 con số
chồng lên nhau thay vì một. Nguyên nhân: trong Flux, `max()` tính **theo từng bảng**,
mà mỗi tag là một bảng — nên ra 6 giá trị. Phải `last()` trước (mỗi tag một dòng), rồi
`group()` gộp lại, rồi mới `max()`. Thứ tự `group() |> last() |> max()` cũng sai: gộp
trước thì `last()` chỉ còn một dòng và `max()` vô nghĩa. Đã sửa và kiểm bằng dữ liệu
seed: cell nóng nhất hiện đúng 36,3 °C.

Bar gauge cũng được đặt `displayName: ${__field.labels.tag}`, nếu không Grafana ghi
tên hàng là `_value Cell01_Temp`.

Ảnh chụp dashboard dùng cho hồ sơ và slide: `docs/slide_assets/dashboard_real.png` —
**dashboard thật, số liệu mô phỏng**, và mọi chú thích đều ghi rõ điều đó.

---

## QĐ-037 — Bring-up phần cứng đợt 3 (INA226, D4184 ×2, sưởi 20 Ω, còi SFM-27)

**Ngày:** 20/09/2026 · **Bằng chứng:** `docs/BANG_CHUNG_BRINGUP_PHAN_CUNG_2026-09-20.md`
· **Sketch:** `test/hw_bringup_6s/`

**Kết quả:** T1, T2, T3, T4, T5, T6, T7, T8, T9 — ĐẠT toàn bộ.

**Đổi chip đo dòng: INA228 → INA226.** Phần cứng thực tế mua về là INA226 với
shunt onboard R100 = 100 mΩ, không phải INA228/0,015 Ω như `pack_meter.h` đang
mô tả. Hệ quả cần biết: toàn thang chỉ còn **±0,819 A** (±81,92 mV / 0,1 Ω),
đủ cho sưởi 0,62 A nhưng **không đủ** để đo dòng sạc/xả pack thật. `pack_meter.cpp`
chưa sửa theo — xem "việc còn lại".

**Bốn quyết định kỹ thuật rút ra từ số đo, không phải từ suy luận:**

1. **Cọc trigger D4184 KHÔNG được cấp nguồn riêng.** Đấu 3,3 V vào đó ghim cứng
   gate lên cao: MOSFET dẫn vĩnh viễn, và mỗi lệnh `digitalWrite(LOW)` là một
   lần ESP32 ngắn mạch chân ra của nó. Chỉ `TRIG → GPIO`, `GND → GND ESP32`,
   và GND đó phải nối chung cực âm 12 V ở `VIN−`.

2. **Đo mức chân GPIO phải ở chế độ INPUT.** Khi `pinMode(OUTPUT)`,
   arduino-esp32 tắt bộ đệm vào nên `digitalRead` trả về giá trị không tin
   được. Phép đo dứt điểm là bật điện trở kéo xuống nội rồi đọc: vẫn ra mức cao
   ⇒ có nguồn ngoài ghim chân.

3. **Tích phân năng lượng phải dùng quy tắc hình thang và nhịp lấy mẫu độc lập
   với 1-Wire.** Bản bậc-thang-trái với bước 750 ms (vì gọi `dsReadAll()` trong
   vòng tích phân) cho kết quả thiếu 12 %, **luôn lệch một chiều** — loại sai số
   không tự triệt tiêu khi lấy trung bình nhiều mẻ, tức là Lớp 2 sẽ học đúng cái
   lệch đó. Sau khi sửa: 35,11/35,10/35,11 J, tản 0,03 %.

4. **Hệ an toàn phải được nuôi liên tục, kể cả lúc đang không làm gì.** Đồng hồ
   "quá 5 s không có số đọc mới" vẫn chạy trong lúc chương trình ngồi ở menu
   hoặc `delay()`, nên nó cắt oan hai lần. Cùng một lý do khiến firmware thật
   cấm `delay()` dài trong vòng lặp chính.

**Sửa phiếu test: ngưỡng thử T7 là 31 °C, không phải 35 °C.** Đo thật: nắm tay
vào đầu dò 2 phút chỉ lên được 32,31 °C rồi nguội. 35 °C là ngưỡng không thể
với tới, nên test đó luôn "trượt" mà không nói gì về hệ an toàn.

**Đã thực hiện 21/09 — `pack_meter` chạy được cả hai chip, tự nhận.**
Quyết định của đội: dùng INA226 trước, INA228 về thì đổi vào. Thay vì sửa qua
sửa lại, `PackMeter::begin()` đọc mã định danh và cấu hình theo con đang cắm.
Giao diện công khai (`PackMeasurement`) **không đổi**, nên
`esp32s3_wiseiot_test.ino` không phải sửa một dòng nào.

Thử INA228 **trước** trong chuỗi nhận dạng: thanh ghi `0x3F` không tồn tại trên
INA226 nên đọc ra giá trị không xác định, còn `0xFE/0xFF` của INA228 lại là
thanh ghi hợp lệ khác — thứ tự ngược lại dễ nhận nhầm hơn.

Ba khác biệt của INA226 và cách xử lý:

| | INA228 | INA226 |
|---|---|---|
| Toàn thang dòng | ±10,9 A (shunt 0,015 Ω) | **±0,819 A** (shunt 0,1 Ω) |
| Đếm coulomb | thanh ghi `CHARGE` trong chip | **tích phân bằng phần mềm** |
| Nhiệt độ chip | có | không → `die_temp = NAN` |

- **Quyết định (5) mới — loại số bão hoà.** ±0,819 A là giới hạn chân shunt;
  vượt qua thì thanh ghi dừng ở giá trị lớn nhất và trả về một con số trông
  hoàn toàn hợp lý. `readIna226()` so với 99 % toàn thang rồi loại, đếm riêng
  bằng `saturationCount()` — "dòng vượt thang đo" và "hỏng dây" đòi hỏi hai
  hành động khác nhau nên không gộp vào một bộ đếm.
- **Đếm coulomb phần mềm** bỏ qua lượt đầu (chưa có mốc thời gian, lấy Δt từ
  `millis()=0` sẽ cộng một cục Ah bịa) và bỏ qua Δt ≥ 60 s (lỡ nhịp quá lâu
  thì con số thành bịa). Hạn chế phải biết: chỉ tích phân những lúc `read()`
  được gọi, nên ở nhịp thưa hơn 1 Hz con số Ah bắt đầu sai.
- **Dòng tính từ điện áp shunt**, không đọc thanh ghi `CURRENT`. Hai đường cho
  cùng kết quả (đo được: lệch 0,1 mA trên 605 mA) nhưng đường này không phụ
  thuộc thanh ghi `CAL`.

Kiểm chứng: `test/pack_meter_live/` chạy trên chip thật — nhận đúng INA226
(`die_temp = NAN`, chỉ có trên đường INA226) và loại đúng số ngoài khoảng hợp
lý. Firmware chính, `pack_meter_bench` và `run_test.sh` đều biên dịch/chạy lại
sạch.

**Việc còn lại (chưa làm, cần người quyết):**
- `PM_N_CELLS`/`PM_V_MIN` vẫn là 8 cell / 15,0 V (QĐ-036 chưa thực hiện).
- Cảm biến môi trường `DS_ROM_AMBIENT` và cảm biến **P05** hiện không có trên
  bus. Còn 7/9 đầu dò.
- Điện trở shunt của cả hai module đều **chưa đo lại** — sai số tuyệt đối ~5 %.

---

## QĐ-038 — Thực hiện 6S, và gom số cell về một nguồn sự thật

**Ngày:** 21/09/2026 · **Thực hiện QĐ-036** (đã chốt 19/09 nhưng chưa sửa code)
· Người phụ trách xác nhận: pack chạy 6S, **8S giữ làm bản dự phòng**.

### Không sửa năm chỗ — bỏ hẳn khả năng sửa sót

QĐ-036 liệt kê năm hằng số ở bốn file và cảnh báo "đổi thiếu một chỗ là lệch
chỉ số cell âm thầm". Sửa tay năm chỗ thì lần sau đổi lại vẫn đúng rủi ro ấy.
Thay vào đó: thêm `pack_config.h` giữ **`PACK_N_CELLS`** duy nhất, mọi hằng số
khác suy ra từ nó.

| Trước | Sau |
|---|---|
| `AI_N_CELLS 8` | `= PACK_N_CELLS` |
| `DS_N_PROBES 8` | `= PACK_N_CELLS` |
| `CC_N_CELLS 8` | `= PACK_N_CELLS` |
| `PM_N_CELLS 8` | `= PACK_N_CELLS` |
| `PM_V_MIN 15.0` · `PM_V_MAX 35.0` | `= PACK_V_MIN/MAX`, suy ra từ số cell |
| **`NUM_CELLS 8`** | `= PACK_N_CELLS` + `static_assert` |

Đổi cấu hình pack giờ là sửa **một dòng**. `PACK_N_CELLS` khác 6 hoặc 8 thì
`#error` chặn ngay lúc biên dịch — vì bảng ROM và bảng offset chỉ có hai bản.

### ⚠️ Phát hiện: QĐ-036 đếm thiếu, có SÁU chỗ chứ không phải năm

`NUM_CELLS` trong `esp32s3_wiseiot_test.ino` **không có trong danh sách**, và
nó là chỗ nguy hiểm nhất. `publishData()` chạy chỉ số tới `NUM_CELLS` để đọc
`gCellTemp[]` — mảng có kích thước `AI_N_CELLS`. Hạ `AI_N_CELLS` xuống 6 theo
đúng danh sách QĐ-036 mà giữ `NUM_CELLS = 8` thì vòng lặp **đọc tràn mảng** và
đẩy hai cell rác lên dashboard, không có lỗi nào báo.

Đây chính là lý do gom về một nguồn tốt hơn là sửa theo danh sách: danh sách có
thể thiếu, ràng buộc biên dịch thì không.

### Bảng offset 6 cell phải CĂN LẠI GỐC, không được cắt bớt

Bảng gốc căn theo trung bình **8** kênh (tổng 8 hệ số = 0). Giữ nguyên 6 hệ số
đầu thì tổng thành +0,0406 °C ⇒ phép hiệu chỉnh dịch nhiệt độ trung bình của
cả pack đi +0,0068 °C. Vô hại với Lớp 1 (nhìn chênh lệch tương đối) nhưng phá
tính chất "hiệu chỉnh không đụng giá trị tuyệt đối" — mà **ngưỡng cứng 60 °C
lại đọc giá trị tuyệt đối**. Đã trừ mỗi hệ số đi 0,006767. Chênh lệch giữa các
kênh không đổi (độ rộng P03−P02 vẫn 0,3260 °C) nên **không phải hiệu chuẩn lại
bằng nước**.

### Giả định phải người xác nhận

Lấy **P01..P06**, bỏ P07/P08 — chọn theo thứ tự đánh số, **không phải kết quả
đo**. Nếu sáu đầu dò đang dán lên pack không phải P01..P06 thì phải sửa
`DS_ROM[]` cho khớp nhãn thật. Ghi chú: **P05 hiện không có trên bus**
(bằng chứng bring-up 20/09), phải cắm lại trước khi lấy dữ liệu.

### Kiểm chứng

| Phép kiểm | Kết quả |
|---|---|
| Biên dịch 6S | ✅ 85 % flash |
| Biên dịch 8S (bản dự phòng) | ✅ |
| `PACK_N_CELLS = 7` | ✅ bị `#error` chặn, không biên dịch ra |
| `ai/test_c_vs_python.py` ở 6 cell | ✅ 11/11 đặc trưng lệch ≤ 2,4e-7; sai số tái tạo lệch 1,4e-6; 100 % bước chỉ đúng cùng cell tệ nhất |
| `test/run_test.sh` (spool) | ✅ toàn bộ PASS |
| `pack_meter_bench`, `pack_meter_live`, `cell_temp_ai_bench`, `alarm_bench` | ✅ biên dịch |

**Sửa luôn một lỗi trong chính bộ test:** `test_c_vs_python.py` ghim `N = 8`.
Cái test dùng để gác việc đổi số cell lại là chỗ đầu tiên bị lệch. Giờ nó đọc
`PACK_N_CELLS` thẳng từ `pack_config.h`.

Payload MQTT: chỉ đổi **số lượng** trường `Cell0X_Temp` (6 thay vì 8) và hai
chuỗi mô tả `Name`/`Desc`. Topic và cấu trúc `{"d":{...},"ts":...}` **không
đổi** — ràng buộc cứng số 1 của `CLAUDE.md` được giữ nguyên.

---

## QĐ-039 — Kiểm Lớp 1 ở 6 cell: giữ nguyên mô hình và ngưỡng; và một mâu thuẫn trong số liệu công bố

**Ngày:** 21/09/2026 · **Công cụ:** `ai/pack_size_study.py` (viết mới)

### Câu hỏi

QĐ-038 hạ pack xuống 6 cell. Mạng không phải đổi (11 đặc trưng/cell, kích thước
độc lập với N) — chuyện đó đã chắc. Nhưng nhiều trong 11 đặc trưng tính **trên
cả pack**, nên thống kê của chúng dịch khi N đổi: cell lỗi nằm trong 6 con thì
tự kéo trung bình và độ lệch chuẩn pack lên mạnh hơn khi nằm trong 8 con, tức
là **tự che mình nhiều hơn**. Mô hình lại huấn luyện ở `PACK_SIZE = 8`, và
ngưỡng 1,0707 chốt ở N = 8.

Ước lượng bằng tay trước khi đo: `zscore` yếu đi ~16 %. **Phải đo, vì đó mới là
suy luận.**

### Thiết kế phép đo

Pack 6 cell lấy là **6 cell ĐẦU của đúng pack 8 cell đó**, lỗi tiêm vào **cùng
một cell**. Nhờ vậy mọi khác biệt đều đến từ thống kê pack, không phải từ việc
hai bên nhìn vào cell khác nhau. 24 đoạn pack-ảo từ 8 chu kỳ test của bộ UPC.

### Kết quả — KHÔNG phải huấn luyện lại, KHÔNG phải đổi ngưỡng

| N cell | báo động giả @1,0707 | p99.9 của điểm |
|---|---|---|
| 8 | 0,0000 % | 1,17955 |
| 6 | 0,0000 % | 1,17975 |

Nền sạch **không dịch chuyển** — hai ngưỡng p99.9 lệch nhau 0,02 %.

| Lỗi | N=8 | N=6 |
|---|---|---|
| offset 2 °C | 100 %, trễ 75 s | 100 %, trễ 77 s |
| offset 3 °C | 100 %, trễ 62 s | 100 %, trễ 62 s |
| ramp 1 °C/10ph | 100 %, trễ 3212 s | 100 %, trễ **3436 s** |
| ramp 5 °C/10ph | 100 %, trễ 261 s | 100 %, trễ **286 s** |
| drift 5 °C | 67 % | 67 % |

**Cái giá thật: chậm hơn 8–12 % với lỗi kiểu ramp.** Tỉ lệ phát hiện không đổi.
Đúng chiều đã dự đoán nhưng **nhẹ hơn nhiều** con 16 % tính tay — vì mô hình
còn 9 đặc trưng khác đỡ, không chỉ sống bằng `zscore`.

Drift 3 °C ra 25 % (N=8) so với 17 % (N=6): đó là **6 ca so với 4 ca trên 24
đoạn** — nhiễu thống kê, đừng đọc thành suy giảm.

### ⚠️ Phát hiện phụ, nghiêm trọng hơn câu hỏi gốc

Hai con số đinh đang dùng trong hồ sơ và slide **đến từ hai cấu hình khác nhau
và không thể cùng đúng một lúc**:

| | giữ 10 s | giữ 30 s (**firmware thật**) |
|---|---|---|
| Báo động giả | **0,0189 %** | **0,0000 %** |
| offset 2 °C | 100 % | 100 % |
| drift 5 °C | **83 %** | **67 %** |

- *"0 báo động giả / 6.123.080 mẫu-cell"* chỉ đúng ở **30 giây**
- *"drift 5 °C: 88 %"* đo ở **10 giây** — mà ở 10 giây báo động giả **không còn
  là 0**

Gốc: `eval_ae_relative.py` nhập `PERSIST = 10` từ `validate_mcmaster.py`, trong
khi `operating_point_rel.npz` ghi `persist_s = 30` và `cell_ai.cpp` cũng giữ 30
giây. Hai đường số liệu chạy song song mà không ai đối chiếu.

**Nói công bằng:** phép đo này ra 83 % còn hồ sơ ghi 88 % — chênh do cỡ mẫu
khác (24 đoạn / 8 chu kỳ). Kết luận **không phải** "88 % sai", mà là **"88 %
được đo ở cấu hình không chạy trên thiết bị"**.

**Quyết định: chỉ công bố cột 30 giây**, vì đó là cột tự nhất quán và đúng với
thứ chạy trên board. Cụ thể: giữ "offset 2 °C = 100 %", giữ "0 báo động giả",
**sửa "drift 5 °C" từ 88 % xuống ~67 %**.

**Việc còn lại:** thống nhất `PERSIST` về 30 trong `eval_ae_relative.py` và
`validate_mcmaster.py` để hai đường số liệu không lệch nhau nữa.

---

## QĐ-040 — Đặc tính nhiệt của điện trở sưởi, và hai lỗ hổng trong cách nghĩ về hệ an toàn

**Ngày:** 21/09/2026 · **Công cụ:** `test/heater_thermal/`
· **Bằng chứng:** `docs/BANG_CHUNG_BRINGUP_PHAN_CUNG_2026-09-20.md`

Đầu dò **P07** (nằm ngoài cấu hình 6S) quấn lên điện trở sứ 20 Ω / 10 W (đo
được ~18,4 Ω), sưởi bằng 12 V qua D4184, đo dòng bằng INA226.

### Số đo

| | |
|---|---|
| Tốc độ lên | **0,115 °C/s** (28,81 → 40,00 °C trong 97,7 s, 720 J) |
| Cắt tại | 40,00 °C, t = 97 s |
| **Đỉnh** | **45,62 °C tại t = 157 s — 60 giây SAU khi cắt** |
| **Vọt lố** | **+5,62 °C** |
| Hằng số nguội | **τ ≈ 359 s** — về trong 1 °C của nền cần ~18 phút |

### ⚠️ Lỗ hổng 1 — ngưỡng cắt KHÔNG phải là trần nhiệt

Cắt ở 40 °C nhưng nhiệt vẫn đi tiếp 60 giây nữa và dừng ở 45,62 °C. Nhiệt đã
nằm trong khối sứ vẫn tiếp tục lan ra đầu dò sau khi ngắt điện.

Chiếu sang firmware: `AL_T_CRIT = 60,0 °C` nghĩa là **cắt ở 60**, không phải
**không bao giờ quá 60**. Với động học kiểu này thì đỉnh thật sẽ là ~65,6 °C.

**KHÔNG đổi `AL_T_CRIT` ngay** — đó là kiến trúc đã chốt ở QĐ-028, và con số
5,62 °C này đo trên **điện trở + đầu dò quấn ngoài**, không phải trên cell. Cell
18650 có khối nhiệt lớn hơn nhiều và đầu dò sẽ chạm trực tiếp vỏ cell, nên vọt
lố ở đó gần như chắc chắn khác. **Phải đo lại sau khi dán đầu dò lên pack**, rồi
người mới quyết có hạ ngưỡng không.

Thứ đã chắc chắn ngay bây giờ: **một ngưỡng cắt luôn cần biên dự phòng**, và
biên đó phải đo chứ không đoán.

### ⚠️ Lỗ hổng 2 — hệ an toàn chỉ nhìn một đại lượng thì mù theo đúng cách đại lượng đó mù

Lần chạy đầu (ngưỡng test 50 °C) **không bao giờ cắt**: bơm 7,3 W trong 113 s mà
đầu dò mới lên 46,94 °C. Trong khi đó **thân điện trở đã nóng tới mức người phụ
trách rụt tay** và tự rút adapter. Chênh lệch giữa bề mặt điện trở và đầu dò
quấn ngoài ước cỡ 50–80 °C.

Không có gì hỏng (10 W chạy 7,3 W là trong định mức, đầu dò thấy tối đa 46,9 °C
so với giới hạn 125 °C). Nhưng **thứ dừng thí nghiệm là bàn tay người, không
phải chương trình** — đó mới là chỗ sai.

Gốc của nó là lỗi thiết kế: hệ an toàn chỉ nhìn **nhiệt độ**, mà nhiệt độ lại đo
ở một chỗ không phải chỗ nóng nhất.

**Đã sửa — thêm hạn mức KHÔNG đọc cảm biến:** cắt sau **165 s** hoặc **1200 J**
đã bơm, bất kể nhiệt độ nói gì (`MAX_HEAT_MS`, `MAX_HEAT_J`). Hạn mức này vẫn cắt
khi đầu dò tuột, dán sai chỗ, hay tiếp xúc kém — đúng những chế độ hỏng mà một
ngưỡng nhiệt đơn thuần không thấy.

**Nên cân nhắc đưa nguyên tắc này vào firmware thật**, không chỉ bench: hiện
`alarm.cpp` cũng chỉ có ngưỡng nhiệt và trạng thái mất cảm biến. Người quyết.

### Hệ quả cho kịch bản demo — và một cái bẫy dễ mất điểm

Ước đầu τ ≈ 407 s lấy từ **2 điểm trên 122 s**, mới nguội 26 % quãng đường —
ngoại suy yếu, đã trình bày chắc chắn hơn mức dữ liệu cho phép. Đo lại với mốc
dài gấp đôi (54 % quãng đường, 4,6 phút): **τ ≈ 359 s**, lệch 12 % so với ước
đầu. Kết luận "~20 phút" vẫn đứng, chính xác hơn là **~18 phút**.

⚠️ **Sờ tay KHÔNG kiểm được chuyện này.** Lúc người phụ trách thấy "đã nguội
rồi", đầu dò đọc **35,25 °C** trong khi nền **26,4 °C** — còn cao hơn nền
8,9 °C, mới đi được nửa đường. Da người ~33 °C nên 35 °C sờ vào thấy gần như
trung tính. Bàn tay là dụng cụ tốt ở dải nóng (chính nó đã phát hiện điện trở
quá nhiệt khi máy còn mù, xem lỗ hổng 2) nhưng **mù ở dải ấm**.

**Cái bẫy:** Lớp 1 nhìn chênh lệch TƯƠNG ĐỐI giữa các cell. Diễn xong lần một
rồi diễn lại ngay thì cell vừa sưởi vẫn ấm hơn 5 cell kia vài độ, và một trong
hai chuyện xấu sẽ xảy ra:

- hệ **báo động ngay khi vừa bật** — trông oai nhưng là báo giả, và không trả
  lời được câu "sao nó báo trước khi các bạn sưởi?"
- hoặc EMA của `dev` đã kịp coi mức ấm đó là nền bình thường, nên khi sưởi thật
  thì **báo chậm hơn, hoặc không báo**

**Quy tắc cho ngày thi:** giữa hai lần diễn, chờ tới khi cell vừa sưởi về trong
~1 °C so với các cell còn lại. Con số đó **đọc thẳng trên dashboard**, không
được dựa vào sờ tay.
- Tốc độ lên 0,115 °C/s là **trên điện trở**. Trên cell 18650 khối nhiệt lớn hơn
  nhiều nên sẽ chậm hơn hẳn. Demo 90 giây chỉ chốt được **sau khi đo lại trên
  pack thật**.

---

## QĐ-041 — BLE cho thợ kỹ thuật: chỉ đọc, và nút tắt tiếng dời khỏi kiểu bắt sườn

**21/09/2026 · Đã chốt, đã đo trên phần cứng**

### Làm gì

Thêm `ble_view.h/.cpp`: một dịch vụ GATT quảng bá tên `HuTieu-BMS`, 5 đặc tính
**chỉ đọc + notify**, mỗi cái có nhãn tiếng Việt (descriptor `0x2901`) để đọc
được bằng **nRF Connect**, không phải viết app. Đây đúng là bản tối giản mà
QĐ-021 đã hẹn làm.

### Phục vụ ai — và không phục vụ ai

`docs/NGUOI_DUNG_VA_KICH_BAN.md` xếp BLE vào **NGƯỜI DÙNG B: thợ kỹ thuật đứng
cạnh pack**. Không phải người lái xe — tài liệu nói thẳng người lái gần như
không bao giờ mở app, giao diện của họ là còi và đèn. Vì phục vụ B nên nội dung
hiển thị là **đủ 6 cell + điểm AI + ngưỡng**, tức đủ để chẩn đoán tại chỗ, chứ
không phải một đèn xanh/đỏ.

Wi-Fi vẫn là kênh chính (QĐ-021 không đổi). BLE khởi động **sau cùng** trong
`setup()`: Bluedroid ngốn ~80 KB heap, và nếu phải tranh RAM thì MQTT phải
thắng — hỏng BLE là mất tiện nghi, hỏng MQTT là mất phần được chấm điểm.

### Không có đặc tính nào ghi được — và điều đó được KIỂM, không chỉ được định

BLE quảng bá công khai, không xác thực. Một đặc tính ghi được là cho bất kỳ ai
trong bán kính 10 m bật sưởi của một pack lithium, không mật khẩu, không dấu
vết. Đường điều khiển đã có và nó đi qua MQTT có tài khoản, cộng công tắc chết
người 15 s ở phía thiết bị (QĐ-040).

`test/ble_check.py` kiểm tự động chuyện này thay vì tin vào ý định, cùng ba thứ
khác mà nhìn nRF Connect không chứng minh được: đọc đủ 5 đặc tính, notify có
bắn khi số đổi, và **có quảng bá lại sau khi ngắt kết nối không** — thiếu vế
cuối thì người thứ hai tới kiểm pack sẽ không quét thấy gì, thiết bị vẫn chạy
nhưng vô hình. Kết quả 21/09: **cả bốn đạt.**

### Lỗi tìm ra khi làm: cổng USB tự bấm nút tắt tiếng

`AL_PIN_MUTE = 0`, mà GPIO0 cũng là chân DTR của mạch nạp trên board. Đo được:

| Cấu hình cổng | Dòng serial đọc được | Tắt tiếng bị đảo |
|---|---|---|
| `dtr=True, rts=False` | 4 | **có, 6/6 lần** |
| `dtr=False, rts=False` | 25 | không |

Hai phát hiện, cái thứ hai quan trọng hơn:

1. Script đọc serial của đội vẫn dùng `dtr=True` — **sai suốt từ đầu**, và nó
   còn làm mất dữ liệu (4 dòng so với 25).
2. `dtr=True` **ghì GPIO0 xuống liên tục suốt phiên**, không phải một xung, và
   mở cổng còn làm board reset. Nên bất kỳ ai mở Serial Monitor rồi đóng lại là
   **còi bị tắt tiếng** — đèn vẫn đỏ, mức vẫn leo, chỉ tiếng là không kêu.
   Cùng họ với lỗi `AL_PIN_BUZZER = -1` tìm ra ngày 21/09.

**Ba lần sửa, hai lần đầu sai — ghi lại vì cách sai mới là bài học:**

- *Lần 1* — đòi giữ nút 600 ms, với lập luận "xung DTR chỉ vài chục ms". Đo
  lại: **998 ms**. Lập luận sai nên bản vá vô dụng.
- *Lần 2* — khoá nút 3 s đầu sau khởi động. Vẫn trượt 6/6, vì chân không bị
  nhấn lúc khởi động mà bị **giữ vĩnh viễn**.
- *Lần 3, đạt* — đảo lúc **NHẢ**, và chỉ nhận cú nhấn sau khi đã **thấy nút ở
  trạng thái nhả ít nhất một lần**. Chân bị ghì từ lúc boot không bao giờ qua
  được vạch đó; người dùng thật thì nút vốn đang nhả nên qua ngay vòng đầu.

**Phép thử cũng từng sai và đó là phần đáng nhớ nhất.** Bản đầu của
`test/dtr_mute_check.py` đọc dòng `[ALRM]` trên chính serial và báo "6/6 đạt" —
sai, vì cú đảo xảy ra đúng lúc **đóng cổng**, tức sau khi đã thôi đọc. Phải
quan sát bằng kênh không đụng vào GPIO0, nên bản hiện tại **đọc trạng thái qua
BLE**. Chạy lại: 3/3 đạt.

### Hệ quả thiết kế được rút ra

Trạng thái tắt tiếng bây giờ **hiện trên đặc tính `Trang thai` của BLE**
(`[COI DANG TAT TIENG]`). Một cú tắt tiếng mà không ai thấy là chế độ hỏng nguy
hiểm nhất của cả khối báo động, và người đứng cạnh pack lại chính là người duy
nhất có cơ hội nhận ra.

### Lần vá thứ tư: nút an toàn trước máy, nhưng chết với người

Ba lần vá trên mới chỉ chứng minh **cổng USB không bấm hộ được**. Chưa ai kiểm
vế còn lại: *người bấm thì có ăn không?* Đo ra: **không**.

Hai lỗi, cả hai đều do chọn số bằng cảm tính thay vì bằng phép đo:

1. **`pollMute()` chạy ở nhịp 1 Hz** (nó được gọi trong `Alarm::update()`).
   Cú nhấn tay chỉ kéo dài vài trăm ms nên lọt trọn vào giữa hai lần lấy mẫu và
   **không bao giờ được nhìn thấy**. → tách ra `Alarm::pollButton()`, gọi **mỗi
   vòng `loop()`**, cùng lý do với `gHeater.update()` ở QĐ-040.
2. **Khoảng hợp lệ đặt sai hoàn toàn.** Nhật ký chẩn đoán từ tay người thật:

   | Kiểu nhấn | Đo được | Ngưỡng cũ 600–5000 ms |
   |---|---|---|
   | bình thường ×6 | 199, 232, 254, 205, 241, 259 ms | **loại sạch** |
   | giữ "khoảng 1 giây" | 1735 ms | ăn |
   | giữ vừa | 595 ms | **loại** |

   Tức ngưỡng tối thiểu 600 ms của lần vá 1 **đã giết nút thật ngay từ đầu**,
   và chuyện đó bị che suốt ba lần vá vì không ai đo vế người dùng.
   → chốt **120–2500 ms**, là số đo chứ không phải số đoán.

**Và nút giờ tự khai khi từ chối:** `bo qua cu nham 1735 ms (chi nhan
120..2500 ms)`. Một cái nút im lặng không ăn là thứ không gỡ được — người bấm,
không có gì xảy ra, và không có cách nào biết là quá ngắn, quá dài, hay chưa
tới tay chương trình. Chính vì thiếu dòng này mà mất hai vòng đo.

Kiểm lại sau khi sửa: nút thật ăn 7/7 cú nhấn bình thường, và
`test/dtr_mute_check.py` vẫn **3/3 đạt** — nới khoảng không mở lại cửa cũ.

### Bài học chung, đáng giá hơn cả cái nút

Một lớp an toàn có **hai vế**: không kích hoạt sai, và **vẫn kích hoạt đúng khi
cần**. Ba lần vá đầu chỉ kiểm vế thứ nhất, nên tạo ra một cái nút tắt tiếng an
toàn tuyệt đối trước nhiễu và vô dụng trước người. Cùng dạng với lỗi
`AL_PIN_BUZZER = -1`: mọi thứ trông đúng, chỉ có chức năng là không tồn tại.
Mỗi phép vá cho một lớp an toàn phải kèm phép đo cho **cả hai vế**.

### Còn nợ

- Nút BOOT vẫn là chân chung với DTR. Bản vá phần mềm đã đủ an toàn, nhưng nếu
  có board rời thì **dời nút tắt tiếng sang chân khác** vẫn sạch hơn.

---

## QĐ-042 — Phân loại DẠNG bất thường, và mở đúng một đường ghi qua BLE

**21/09/2026 · Đã chốt, đã đo trên phần cứng**

### (A) Lớp 1 nói được DẠNG, không chỉ nói điểm số

Trước: BLE và dashboard chỉ đưa `cell 3, điểm 26,97`. Thợ đứng cạnh pack không
làm gì được với con số đó — bảng tra TH-1/2/3 nằm trong tài liệu, trên máy tính,
không nằm trong tay họ.

Nay: `cell_ai` tra bảng đó ngay trên thiết bị, từ ba đặc trưng **đã tính sẵn**
(`dev_mean`, `dT_diff`, `dev_shock`) — không thêm phép tính nào đáng kể.

| Dạng | Điều kiện | Việc phải làm |
|---|---|---|
| TH-1 `AIP_FAST` | `dT_diff ≥ 1,0` °C/ph **hoặc** `shock ≥ 1,5` °C | Ngắt sạc, ngắt tải, cách ly pack |
| TH-2 `AIP_WARM` | `dev ≥ 1,5` °C, không vọt | Ghi sổ, kiểm lúc bảo dưỡng |
| TH-3 `AIP_COLD` | `dev ≤ −2,0` °C | Kiểm mối nối và cảm biến |

**Vì sao đáng làm:** phân biệt TH-1 với TH-2 là khác biệt giữa *"cách ly pack
ngay"* và *"mai xem"*. Đó là quyết định đắt nhất mà thợ phải ra tại chỗ.

**Ba ràng buộc, đều quan trọng ngang nhau:**

1. **Đây là TRA BẢNG, không phải bộ phân loại được huấn luyện.** Autoencoder
   chưa bao giờ được dạy tên của bất kỳ lỗi nào, và đội không có dữ liệu gán
   nhãn để dạy. Trước giám khảo phải nói đúng: *"phát hiện bất thường và phân
   loại dạng, không chẩn đoán nguyên nhân"*.
2. **Không đụng gì tới quyết định báo động.** Báo động vẫn do autoencoder +
   ngưỡng cứng 60 °C quyết. Ngưỡng tra bảng có sai thì lời khuyên kém sắc, chứ
   không làm hệ bỏ sót hay báo oan.
3. **Ba con số thô được đưa ra cùng lời khuyên**, trên cả BLE lẫn trang web.
   Một lời khuyên không kiểm được thì đến lúc nó sai sẽ không ai phát hiện.

**Thứ tự kiểm không được đảo:** LẠNH trước (dễ bỏ sót nhất), rồi NHANH, rồi ẤM.
Xếp NHANH sau ẤM là hạ một TH-1 xuống TH-2 — biến "cách ly ngay" thành "mai
xem".

Nghiệm thu trên phần cứng: đốt cell 3 →
`CELL 3 - NONG LEN NHANH (TH-1) | NGAT SAC, ngat tai, CACH LY PACK`,
`lech +9,53 °C | nhanh hon pack +2,19 °C/phut | dot ngot +7,60 °C`.
Cell 3 ấm ổn định → `TH-2`. Cell 3 về nền → `chua ro dang`.

### (B) Mở đúng MỘT đường ghi qua BLE — và trả giá cho nó

QĐ-041 chốt "không có đặc tính ghi nào", lý do vẫn đúng nguyên: BLE quảng bá
công khai, một đặc tính ghi tự do là cho bất kỳ ai trong 10 m tắt còi của pack
lithium. Nhưng bắt người dùng chạy về chỗ pack bấm nút BOOT mới tắt được còi là
thiết kế tồi. Mở, kèm **bốn** lớp chặn:

1. **Ghép đôi + mã PIN 6 số** (`BLE_PASSKEY`, in trên nhãn thiết bị).
2. **Chỉ `mute`/`quiet`, không có `heat`.** Sưởi là thứ duy nhất bơm năng lượng
   vào pack nên vẫn chỉ đi qua MQTT có tài khoản + công tắc chết người (QĐ-040).
   Kẻ ghép đôi được cũng không làm nóng được pack.
3. **Tự hết hạn sau 5 phút** (`BLE_MUTE_TTL_MS`). Im lặng không bao giờ vĩnh
   viễn — và người dùng thật khỏi quên bật lại, đúng cái bẫy gặp ngày 21/09.
4. **Không tắt được tiếng ở mức NGUY KỊCH.**

### Lỗi tìm ra khi làm (B): lớp bảo mật đầu tiên KHÔNG TỒN TẠI

Bản đầu dùng `cmd->setAccessPermissions(ESP_GATT_PERM_WRITE_ENC_MITM)`. Biên
dịch sạch, không cảnh báo gì. Nhưng `test/ble_write_check.py` — nối mà **không**
ghép đôi rồi ghi `mute on` — cho kết quả:

```
truoc khi ghi : BINH THUONG
sau khi ghi   : BINH THUONG  [COI DANG TAT TIENG]
✗ TRUOT — may CHUA GHEP DOI van tat duoc coi
```

Nguyên nhân: core 3.3.11 chạy **NimBLE**, mà `BLECharacteristic::setAccessPermissions()`
có **thân rỗng** khi không phải Bluedroid (`BLECharacteristic.cpp:167`, thân nằm
trong `#ifdef CONFIG_BLUEDROID_ENABLED`). Hàm chạy, không báo lỗi, và không làm
gì cả.

Sửa: đặt quyền vào **thuộc tính lúc tạo đặc tính** —
`PROPERTY_WRITE_ENC | PROPERTY_WRITE_AUTHEN` — đây mới là đường NimBLE đọc.
Giữ luôn cả `setAccessPermissions()` để đúng với cả hai backend.

**Phép thử cũng phải sửa:** bản đầu không có hạn giờ nên khi thiết bị bắt đầu
đòi ghép đôi, BlueZ ngồi chờ PIN và phép thử **treo 10 phút** mà không kết luận
được gì. Thêm hạn giờ 20 giây, và coi *treo* là **ĐẠT** — vì nghĩa là ghi không
đi qua được. Kết quả sau khi sửa: `✓ DAT — ghi bi CHAN`.

### Bài học

Lặp lại đúng bài học của QĐ-041 ở một chỗ khác: **một lớp bảo vệ chưa được đo
thì chưa tồn tại**. Lần này nó còn im lặng hơn — hàm có thật, biên dịch sạch,
và không làm gì. Không có phép thử thì nó đã lên sân khấu nguyên vẹn dưới dạng
một dòng bình luận nói rằng hệ thống an toàn.

---

## QĐ-043 — Lớp 2 chạy ở CẢ HAI nơi: cloud giữ bản chuẩn, chip có bản dự phòng

**22/09/2026 · Đã chốt, đã kiểm bằng test đối chiếu**

### Bối cảnh: một lập luận cũ vừa bị bác bỏ

QĐ ban đầu đặt Lớp 2 ở cloud với **hai** lý do. Lý do thứ hai —
*"cloud thành nơi tính toán thật, đúng thứ WISE-IoT được chấm điểm"* — **SAI**.
Barem thật của BTC (`docs/BAREM_CHAM_BAN_KET.md`) không nhắc tới WISE-IoT một
lần nào. Xem thêm: barem còn chấm riêng *"tính khả thi và mức độ hoàn thiện"*
15 điểm, mục nặng nhất.

Lý do thứ nhất thì **vẫn đứng vững**: đổi hệ số không phải nạp lại firmware.
Điều này quan trọng thật, vì hệ số đang mượn của NASA và **chắc chắn phải hiệu
chỉnh** cho pack thật (`charge_cycle.h`). Chỉ chip tính thì mỗi lần chỉnh hệ số
là phải đi tới từng thiết bị ngoài hiện trường.

### Quyết định

Chạy **cả hai**. Cloud vẫn là bản chuẩn và vẫn đổi được không cần nạp lại
firmware; chip có bản dự phòng để trả lời được khi mất mạng và để app BLE có số.

Giá phải trả: **10 phép nhân cộng, một lần mỗi chu kỳ sạc** (vài tiếng). So với
Lớp 1 — autoencoder 271 tham số chạy 1 Hz — đây là hạt cát. Và giữ cloud tốn
**0 công**: `rul_predict.js` đã chạy sẵn, ESP32 đã gửi `CYC_*` sẵn. Gỡ đi mới
là làm thêm việc.

### Chỗ nguy hiểm nhất, và cách chặn

Cùng một mô hình chạy hai nơi thì **sẽ lệch nhau** — không phải nếu, mà là khi.
Và lúc lệch thì không ai biết, vì cả hai đều trả về một con số trông hợp lý.

Chặn bằng hai lớp:

1. **Hệ số chung một nguồn.** `ai/export_rul_c.py` sinh `rul_model.h` từ chính
   `ai/models/rul_model.json` mà `rul_predict.js` dùng. Không bao giờ gõ số tay.
2. **Logic được đối chiếu tự động.** `ai/test_rul_c_vs_js.py` sinh 500 chu kỳ
   sạc ngẫu nhiên — **có cả trường hợp ngoài dải huấn luyện**, vì chỉ thử điểm
   ở giữa thì không bao giờ chạm tới nhánh cờ ngoại suy — rồi chạy qua cả hai
   bản và đòi khớp:

   ```
   500 chu ky sac ngau nhien
     lech RUL lon nhat : 0.000032 chu ky
     lech SOH lon nhat : 0.000066 diem phan tram
     co ngoai suy lech : 0 truong hop
   DAT — chip va cloud ra cung mot so
   ```

   Chênh lệch còn lại đúng bằng sai số float32 của chip so với float64.

Firmware còn gửi kèm kết quả tính trên chip dưới tiền tố `ONB_*`, để dashboard
hiện **cả hai** — lệch nhau là nhìn ra ngay, thay vì tin là chúng giống nhau.

### Lịch sử chu kỳ trên flash

**RUL không cần lịch sử để tính** — mô hình chỉ đọc đặc trưng của chu kỳ hiện
tại (`t_cv`). Lịch sử chỉ để vẽ **xu hướng**, mà xu hướng mới là thứ phân biệt
TH-6 (*"SOH tụt dần đều, lão hoá tự nhiên"*) với TH-7 (*"SOH tụt đột ngột, có
cell hỏng"*) — hai trường hợp dẫn tới hai hành động khác hẳn nhau.

Chi phí: 32 byte/chu kỳ. Cả đời pin (~1000 chu kỳ) hết ~32 KB trên phân vùng
1,5 MB. Chip chứa được **toàn bộ lịch sử cả đời quả pin** mà không đánh đổi gì.

Ghi nối đuôi rồi cắt đầu, **không** ghi đè tại chỗ theo vòng: vòng tiết kiệm hơn
nhưng khi file hỏng giữa chừng thì không còn cách nào biết bản ghi nào mới. Ghi
nối đuôi thì hỏng là mất đuôi chứ không mất nghĩa.

### Vẫn chưa thay đổi: con số Lớp 2 CHƯA CÓ THẬT

Chuyển tính toán xuống chip làm nó **chạy offline**, không làm nó **thật hơn**.
Chu kỳ sạc vẫn đang mô phỏng vì chưa có bộ sạc CC/CV 25,2 V. Trên sân khấu vẫn
phải nói đúng như `HAI_LOP_AI_HOAT_DONG_THE_NAO.md` §giới hạn đã ghi.
