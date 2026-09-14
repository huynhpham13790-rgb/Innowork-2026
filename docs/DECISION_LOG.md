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

---

## Ẩn số còn treo

| Ẩn số | Chặn việc gì | Gỡ bằng cách nào |
|---|---|---|
| WISE-IoT có tôn trọng `ts` thiết bị gửi không? | Độ chắc chắn của màn demo trên cloud Advantech | Chờ tài khoản, bắn 1 gói backdate. Plan B đã có đường lùi. |
| WiFi hội trường có chặn/NAT cổng 1883 không? | Kịch bản ngày thi | Đã quyết: phát WiFi từ điện thoại, không dùng mạng hội trường. |
| ~~Store-and-forward chạy thật trên board ra sao?~~ | **Đã gỡ 06/09** — chạy thật thành công, xem `BANG_CHUNG_PHAN_CUNG_2026-09-06.md`. Tìm ra thêm QĐ-010. | |
| Mạng 4G / WiFi phát từ điện thoại có ổn không? | Kịch bản ngày thi | Thử trước, cùng lúc với AC-04.6 trên VPS. |
