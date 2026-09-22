# KHUNG NGHIÊN CỨU LẠI — InnoWorks 2026 | Đội Hủ Tiếu

**Ngày:** 03/09/2026 · **Bán kết:** 26/09/2026 · **Chung kết:** 27/11/2026

> ⚠️ *Cập nhật 14/09/2026: ngày Bán kết đã chốt lại là **26/09** theo landing page BTC (trước đây tài liệu ghi 15/09). Mọi chỗ nói "12 ngày tới" trong bản gốc là tính từ 03/09 theo mốc cũ — tính lại theo 26/09 thì kế hoạch giãn ra 11 ngày. Xem `docs/NGUOI_DUNG_VA_KICH_BAN.md` §3 cho thứ tự việc hiện tại.*

> Đây là **khung tư duy**, không phải kết luận cuối. Mục tiêu: trả lời 4 câu hỏi cậu đặt ra, kèm dữ liệu để tự quyết.

⚠️ **Cảnh báo trước khi đọc tiếp:** lộ trình cũ ghi mốc bắt buộc "tài khoản WISE-IoT phải có ít nhất 1 Dashboard hoạt động trước 01/09". Hôm nay đã 03/09. Nếu chưa làm, đây là việc số 1 — mọi phân tích dưới đây vô nghĩa nếu tài khoản bị hủy.

---

## TÓM TẮT KHUYẾN NGHỊ (đọc cái này trước)

| Câu hỏi | Khuyến nghị ngắn |
|---|---|
| Có cần tầng Raspberry Pi? | **Sản phẩm bán ra: BỎ.** Bán kết: giữ tạm như "giàn giáo", nhưng đổi cách kể chuyện. Xem §1. |
| Giám sát 24/24? | **Có, nhưng không cùng một tần số.** 3 chế độ theo trạng thái xe. Xem §2. |
| Cần pin xe điện thật? | **Không cần mua/mượn pack thật.** Dùng pack nhỏ + điện trở sưởi. Xem §3. |
| AI để làm gì? | Không phải để "báo cháy nhanh hơn cảm biến nhiệt". Là để **so sánh tương đối giữa cell + chuẩn hóa theo ngữ cảnh + xu hướng**. Xem §4. |
| Bán cho ai? | **B2B đội xe / vận hành đổi pin** là số 1, không phải người dùng cá nhân. Xem §5. |

---

## §1. TẦNG RASPBERRY PI — CÓ THẬT SỰ CẦN KHÔNG?

### Vấn đề cốt lõi

Kiến trúc cũ: `ESP32 (Autoencoder) → UART → Raspberry Pi (LSTM RUL) → MQTT → WISE-IoT`

Câu hỏi thật sự không phải "Pi có chạy được không" (chạy được), mà là: **ai trả tiền cho con Pi?**

### So sánh chi phí trên mỗi xe

| Hạng mục | Kiến trúc 3 tầng | Kiến trúc 2 tầng (bỏ Pi) |
|---|---|---|
| MCU | ESP32 ~150k | ESP32-S3/C3 ~120–200k |
| Gateway | **Raspberry Pi 4 ~1.500.000–2.500.000đ** | — |
| Cảm biến (4×DS18B20 + INA226) | ~120k | ~120k |
| Vỏ + PCB + linh tinh | ~150k | ~150k |
| **BOM/xe** | **~2.000.000–3.000.000đ** | **~400.000–500.000đ** |
| Giá bán khả dĩ (BOM×2.5) | ~5–7 triệu | **~1–1,3 triệu** |

**Đối chiếu:** thay pin xe máy điện ở VN hiện tốn **1,5–12 triệu đồng**. Một thiết bị giám sát giá 5–7 triệu để bảo vệ cục pin 3 triệu là **vô lý về mặt kinh tế** — không ai mua. Thiết bị 1–1,3 triệu thì còn nói chuyện được.

Đây là lý do mạnh nhất để bỏ Pi.

### Nhưng bỏ Pi thì LSTM RUL chạy ở đâu?

Ba lựa chọn, xếp theo mức khuyến nghị:

**(A) Đưa RUL lên cloud (WISE-IoT) — KHUYẾN NGHỊ**
- RUL không cần real-time. Chạy 1 lần/chu kỳ sạc hoặc 1 lần/ngày là quá đủ.
- ESP32 chỉ cần đẩy lên ~10–20 con số tóm tắt sau mỗi chu kỳ sạc (dung lượng phóng, nhiệt độ đỉnh/trung bình, thời gian CC/CV, nội trở ước lượng). Vài trăm byte.
- Cloud chạy LSTM/CNN-LSTM thoải mái, không giới hạn RAM.
- ~~**Bonus lớn cho cuộc thi:** ... điểm bán kết phụ thuộc lớn vào mức độ dùng WISE-IoT.~~ **LẬP LUẬN NÀY ĐÃ BỊ BÁC BỎ 22/09/2026** — barem thật không chấm nền tảng cloud, xem `docs/BAREM_CHAM_BAN_KET.md`. Các lý do kỹ thuật ở trên vẫn đứng vững; chỉ lý do "ăn điểm" là sai.

**(B) Chạy RUL ngay trên ESP32**
- Nghiên cứu 2025–2026 cho thấy khả thi: LSTM lượng hóa động cho RUL pin đạt **97,8 KB, inference 1,72 giây** trên ESP32.
- Tốt hơn nữa: đổi LSTM → **1D-CNN hoặc TCN**. Cùng độ chính xác (sai số <1%) nhưng tốn **ít hơn ~35% RAM, ~25% Flash**, và inference **27,6 ms thay vì 2.038 ms**.
- → Nếu muốn "wow" về Edge AI thuần túy ở Chung kết, đây là hướng. Nhưng rủi ro kỹ thuật cao hơn cho 12 ngày tới.

**(C) Giữ Pi**
- Chỉ hợp lý khi **1 Pi phục vụ nhiều xe**: trạm đổi pin, gara, kho đội xe. 1 Pi gom 20–50 module ESP32 qua BLE/ESP-NOW/Wi-Fi. Lúc đó chi phí Pi chia cho 20 xe = 75k/xe → chấp nhận được.
- **Đây mới là cách kể chuyện đúng về con Pi:** nó không phải thiết bị trên xe, nó là *tùy chọn* cho khách hàng đội xe muốn hoạt động offline khi mất mạng.

### Lợi / hại tóm tắt

**Bỏ Pi — lợi:**
- BOM giảm ~5 lần → sản phẩm mới có cửa bán
- Ít điểm hỏng hóc, ít nguồn điện, ít dây, dễ đóng hộp chống rung/chống nước
- Pi 4 ăn 3–6W liên tục — không thể gắn trên xe máy điện chạy pin
- Không cần quản lý OS, cập nhật, thẻ SD hỏng (thẻ SD Pi hỏng là lỗi kinh điển)
- ~~Đẩy tính toán lên WISE-IoT → **tăng điểm thi**~~ — **SAI**, barem không chấm chuyện này (`docs/BAREM_CHAM_BAN_KET.md`)

**Bỏ Pi — hại:**
- ESP32 nối trực tiếp WISE-IoT IoT Hub cần MQTT over TLS + đúng định dạng payload của WISE-PaaS — **cần thử nghiệm sớm, có rủi ro tốn thời gian**. Đây là rủi ro kỹ thuật lớn nhất của phương án này.
- Mất khả năng "chạy AI nặng khi mất mạng" (nhưng lớp phát hiện bất thường vẫn chạy on-device — phần quan trọng vẫn offline)
- Node-RED/EdgeSense trên Pi là con đường dễ nhất để đẩy dữ liệu lên WISE-IoT nếu bí

### Khuyến nghị chiến thuật cho 12 ngày tới

Đừng chọn dứt khoát ngay. Làm song song:

1. **Ngay hôm nay:** cử 1 người thử ESP32 → MQTT/TLS → WISE-IoT IoT Hub. Cho deadline cứng: **3 ngày**. Nếu chạy → bỏ Pi luôn.
2. Nếu 3 ngày không xong → dùng Pi làm cầu MQTT tạm cho Bán kết, nhưng **trong slide kiến trúc, vẽ Pi là "Gateway trạm — tùy chọn"**, và có 1 slide BOM riêng cho phiên bản thương mại 2 tầng.
3. Cách này biến điểm yếu thành điểm mạnh: giám khảo thấy đội **hiểu chênh lệch giữa nguyên mẫu và sản phẩm** — đó chính là thứ đội vô địch 2025 thắng nhờ có (ROI cụ thể).

---

## §2. GIÁM SÁT 24/24 HAY THEO LÚC?

### Dữ liệu để quyết

Thống kê quốc tế về **thời điểm/trạng thái** khi xe điện cháy:
- **~31%** khi **đỗ ngoài trời**
- **~29%** khi **đang chạy**
- **~25%** trong **hầm/bãi ngầm**
- **~15–18%** khi **đang sạc** (thêm ~2% trong 1 giờ sau khi rút sạc)

**Kết luận thẳng:** phần lớn sự cố xảy ra **khi xe đang đỗ**, tức là đúng lúc một hệ thống "chỉ bật khi chạy/sạc" đã ngủ. → **Bắt buộc phải có giám sát 24/24.** Nếu bỏ chế độ đỗ, cậu vứt đi hơn nửa giá trị của sản phẩm.

Nhưng 24/24 **không có nghĩa là lấy mẫu 1Hz suốt ngày**. Pin sẽ cạn.

### Đề xuất: 3 chế độ thích ứng (adaptive duty cycle)

| Chế độ | Tần số lấy mẫu | AI chạy gì | Đẩy cloud | Lý do |
|---|---|---|---|---|
| **Đang sạc** | 1 Hz | Autoencoder mỗi mẫu | 1 lần/30s | Trạng thái năng lượng cao nhất; ở 100% SOC cường độ cháy tăng vọt (31–38 kW/Ah so với ổn định ở 0–75%) |
| **Đang chạy** | 0,2–1 Hz | Autoencoder mỗi mẫu | 1 lần/60s | Tải động, dòng cao, rung xóc |
| **Đang đỗ** | 1 mẫu/60s, **wake-on-event** | Chỉ kiểm tra dT/dt + chênh lệch cell | 1 lần/15 phút (heartbeat) | Tiết kiệm pin; nhưng sự cố khi đỗ thường phát triển chậm nên 60s là đủ |

**Cơ chế wake-on-event khi đỗ:** nếu bất kỳ cell nào có dT/dt vượt ngưỡng, HOẶC chênh lệch giữa cell nóng nhất và lạnh nhất > X°C → **nhảy ngay sang 1 Hz + gửi cảnh báo**. Đây là điểm kỹ thuật đáng khoe: hệ thống tự leo thang.

**Lớp RUL:** chạy **1 lần/chu kỳ sạc** (hoặc 1 lần/ngày). Không cần liên tục. Đây cũng là lý do nữa để đẩy nó lên cloud (§1).

### Bài toán ngân sách điện — cần tính và đưa vào slide

Đây là con số giám khảo sẽ hỏi: *"thiết bị của em có làm cạn pin xe không?"*

- ESP32 deep sleep: ~10 µA · active + Wi-Fi TX: ~120–160 mA
- Cần tính: mAh/ngày ở chế độ đỗ → so với dung lượng pack (vd. 60V–20Ah ≈ 1.200 Wh)
- **Mục tiêu để tuyên bố:** thiết bị tiêu thụ < 0,1%/ngày dung lượng pack.
- → **Việc cần làm:** đo thật bằng INA226 rồi lấy số thật. Đừng ước lượng.

---

## §3. CÓ CẦN PIN XE ĐIỆN THẬT KHÔNG?

### Trả lời ngắn: **KHÔNG** — và đừng làm thế.

Lý do:
1. **An toàn.** Tạo thermal runaway thật trong phòng lab trường là chuyện tuyệt đối không nên. Một pack xe máy điện chứa ~1.000–2.000 Wh; cháy là không dập được bằng bình chữa cháy thường.
2. **Chi phí & thời gian.** Pack thật 3–12 triệu, chưa kể phải phóng/sạc nhiều chu kỳ mới thấy suy giảm — mà suy giảm pin cần **hàng trăm chu kỳ, tức nhiều tháng**. Không kịp cho 27/11.
3. **Không cần thiết để chứng minh.** Cái cần chứng minh là *thuật toán phát hiện đúng*, không phải *pin thật cháy đúng*.

### Phương án 3 tầng (làm cả 3, không chọn 1)

**Tầng A — Nguyên mẫu vật lý (bắt buộc, làm ngay)**
- Pack nhỏ **4–8 cell 18650 hoặc LiFePO4** tháo từ pack laptop cũ / pin xe đạp điện cũ. Chi phí ~200–500k.
- Gắn 4 cảm biến DS18B20, mỗi cell 1 con. INA226 đo dòng/áp tổng.
- **Cách tạo "cell bất thường" AN TOÀN:** dán một **điện trở công suất (5–10Ω, 5W)** áp sát 1 cell, cấp nguồn ngoài để sưởi cell đó lên 45–55°C. Cell không bị lạm dụng, không phóng quá dòng, không đâm chọc. Lặp lại được vô hạn lần, an toàn tuyệt đối, và **tái hiện đúng hiện tượng cần phát hiện: một cell nóng bất thường so với các cell còn lại**.
- Đây là kỹ thuật chuẩn trong nghiên cứu (mô phỏng lạm dụng nhiệt bằng nguồn nhiệt ngoài) — hoàn toàn bảo vệ được trước giám khảo.
- Bổ sung: dùng **túi chườm nóng / máy sấy tóc** cho demo live nếu muốn kịch tính hơn, có kiểm soát.

**Tầng B — Dữ liệu thật (nếu tiếp cận được, không bắt buộc)**
- Không cần *sở hữu* pin xe điện. Cần *đọc* dữ liệu từ một chiếc đang chạy thật.
- Hướng: mượn xe máy điện của thành viên/người quen, gắn module **đọc-only** vào các điểm đo nhiệt bên ngoài vỏ pack + kẹp dòng non-invasive. **Không mở pack, không can thiệp BMS.**
- 3–5 ngày chạy thật → có profile nhiệt độ trong điều kiện VN (nắng 35°C+, tải leo dốc, sạc qua đêm). Đây là **dữ liệu không dataset công khai nào có** → chính là điểm khác biệt để khoe.
- Hoặc: liên hệ 1 gara/cửa hàng xe điện ở Thái Nguyên xin đo trên xe khách để bảo dưỡng.

**Tầng C — Dataset công khai (cho phần train)**
- Xem §4. Đây là nơi lấy dữ liệu suy giảm dài hạn mà đội không thể tự tạo ra kịp.

### Việc PHẢI làm trong phần này
- [ ] Mua pack nhỏ + điện trở sưởi (< 700k) — làm tuần này
- [ ] Viết kịch bản demo: bình thường 3 phút → bật điện trở → hệ thống phát hiện → cảnh báo. Đo **độ trễ phát hiện tính bằng giây** → đây là con số bán hàng.
- [ ] Chuẩn bị câu trả lời cho câu hỏi chắc chắn bị hỏi: *"sao không thử trên pin thật?"* → "vì an toàn, và vì hiện tượng cần phát hiện — chênh lệch nhiệt bất thường giữa các cell — được tái hiện đầy đủ bằng nguồn nhiệt ngoài, đúng phương pháp trong các nghiên cứu về lạm dụng nhiệt."

---

## §4. AI Ở ĐÂY CÓ TÁC DỤNG GÌ THẬT SỰ?

### Trước hết: nói thật về giới hạn

Cần thẳng thắn nội bộ, vì giám khảo sẽ vặn:

> **Mọi BMS đều đã có ngưỡng nhiệt cứng (vd. ngắt ở 60°C). Nếu AI chỉ để báo khi nhiệt > 60°C thì AI vô dụng.**

Và tệ hơn: nếu chỉ dựa vào **nhiệt độ**, thời gian cảnh báo trước thermal runaway thật rất ngắn — nghiên cứu cho thấy gradient nhiệt cho lead time **cỡ giây đến vài chục giây** trước khi runaway. Phát hiện bằng khí (H₂, DMC) hay EIS mới cho 6,8–22,5 phút. Đội **không nên** hứa "cảnh báo trước 20 phút" nếu chỉ dùng cảm biến nhiệt — sẽ bị bắt bài.

### Vậy AI thắng ở đâu? — 4 chỗ, xếp theo giá trị bán hàng

**(1) So sánh tương đối giữa các cell — giá trị cao nhất, dễ chứng minh nhất**
- Cell #3 nóng hơn 4°C so với 3 cell còn lại **dưới cùng một tải**, dù tuyệt đối mới 35°C.
- Ngưỡng cứng **không bao giờ** bắt được cái này. Autoencoder học "mẫu hình bình thường của cả pack" thì bắt được ngay.
- → **Đây là câu chuyện chính của sản phẩm.** Không phải "phát hiện cháy", mà "**phát hiện cell yếu trước khi nó thành vấn đề**".

**(2) Chuẩn hóa theo ngữ cảnh**
- 45°C khi đang sạc nhanh giữa trưa hè Hà Nội = **bình thường**.
- 45°C khi xe đỗ trong hầm lúc 2h sáng = **rất bất thường**.
- Ngưỡng cứng phải chọn 1 con số cho cả 2 → hoặc báo động giả liên tục, hoặc bỏ sót. AI ăn thêm đầu vào (dòng, nhiệt môi trường, trạng thái sạc/chạy/đỗ) thì phân biệt được.

**(3) Xu hướng theo chu kỳ — nơi lead time thật sự dài**
- Nghiên cứu về nhận diện "cell nguy cơ TR" bằng giám sát dài hạn cho **lead time tối thiểu 14 ngày**.
- 14 ngày là con số **bán được**: đủ để đưa xe đi thay pin theo lịch, không phải "cứu hoả".
- → Nên định vị sản phẩm quanh con số này, không phải quanh vài chục giây.

**(4) RUL / SOH — giá trị kinh tế, không phải an toàn**
- Trả lời "pin còn dùng được bao lâu" → phục vụ định giá xe cũ, lập kế hoạch thay pin, quyết định second-life.
- Đây là thứ khách hàng B2B trả tiền đều đặn hàng tháng.

### Cách phân bổ lại hai lớp AI (sau khi bỏ Pi)

| Lớp | Chạy ở đâu | Mô hình | Chu kỳ | Vai trò |
|---|---|---|---|---|
| **Lớp 1 — Bất thường** | ESP32 (on-device, float32, **1,4 KB**) | Autoencoder dense 16→8→4→8→16 | 1 Hz | An toàn, chạy offline |
| **Lớp 2 — RUL/SOH** | **WISE-IoT cloud** | ~~LSTM / CNN-LSTM~~ → **hồi quy tuyến tính** (đã thử LSTM, thua — QĐ-017) | 1 lần/chu kỳ sạc | Kinh tế, làm dày phần WISE-IoT |

**Baseline bắt buộc phải có để so sánh** (giám khảo sẽ hỏi "sao không dùng cách đơn giản hơn?"):
- Ngưỡng cứng 60°C
- Ngưỡng chênh lệch cell cố định (vd. ΔT > 5°C)
- Isolation Forest / One-Class SVM
→ Bảng so sánh Precision/Recall/độ trễ giữa 3 baseline và Autoencoder. **Nếu Autoencoder không thắng, phải biết trước khi lên sân khấu.**

### Dataset — xem §8 (đã rà soát lại và SỬA ngày 03/09)

⚠️ Phần dataset trong bản đầu có **hai lỗi nghiêm trọng**. Đã viết lại thành **§8** ở cuối tài liệu. Đọc §8, bỏ qua danh sách cũ.

---

## §5. BÁN CHO AI? (cậu bảo tớ đề xuất)

### Xếp hạng theo khả năng bán được

**#1 — Đơn vị vận hành đội xe & trạm đổi pin (B2B) — KHUYẾN NGHỊ CHÍNH**
- Ai: đội giao hàng, xe ôm công nghệ, doanh nghiệp cho thuê xe điện, đơn vị vận hành đổi pin.
- **Vì sao đây là khách tốt nhất:** trong mô hình đổi pin (VinFast, Selex, Honda, Yamaha, Yadea đều đã có xe đổi pin ở VN), **pin thuộc sở hữu của đơn vị vận hành, không phải người lái**. Nghĩa là: người chịu chi phí hỏng pin = người có quyền mua thiết bị = **cùng một pháp nhân**. Đây là điều kiện tiên quyết để bán được.
- Đau ở đâu: pin hỏng sớm, không biết pin nào sắp chết, một pin cháy trong kho là mất cả kho.
- ROI cụ thể: thiết bị ~1–1,3 triệu + SaaS ~30–50k/tháng vs. thay pin **1,5–12 triệu**. Chỉ cần kéo dài tuổi thọ 10% hoặc tránh 1 lần thay không cần thiết là hòa vốn.
- Bán theo lô hàng trăm module → có quy mô.

**#2 — Gara / trạm bảo dưỡng / định giá xe cũ (B2B, doanh thu phụ)**
- Ai: gara xe điện, cửa hàng mua bán xe cũ, đơn vị bảo hiểm/thẩm định.
- Sản phẩm khác: **không gắn cố định trên xe** — là thiết bị cầm tay cắm vào đo 15–30 phút → xuất "chứng nhận sức khỏe pin".
- Ưu: 1 thiết bị phục vụ hàng trăm xe → khách sẵn sàng trả cao hơn. Không cần lo ngân sách điện, không cần lo 24/24.
- Nhược: thị trường nhỏ hơn, và mô hình này **không dùng được lớp phát hiện bất thường real-time** → chỉ dùng lớp SOH/RUL.

**#3 — Người dùng cá nhân (B2C) — KHÔNG khuyến nghị làm khách hàng chính**
- Ưu: câu chuyện cảm động nhất khi thuyết trình ("bảo vệ gia đình khỏi cháy nổ").
- Nhược nặng: mức sẵn sàng chi trả rất thấp; ai mua xe mới đã có bảo hành; ai đi xe cũ thì không có tiền mua thiết bị 1 triệu; chi phí bán hàng/hỗ trợ trên đầu người quá cao.
- **Cách dùng:** để mở đầu pitch (gây chú ý), nhưng **mô hình kinh doanh phải là #1**. Nói rõ với giám khảo là đội hiểu sự khác biệt này — đó là dấu hiệu trưởng thành.

### Khung mô hình doanh thu để điền số
```
Doanh thu = (Phần cứng bán 1 lần) + (SaaS/tháng × số module × 12)
Chi phí   = BOM + lắp ráp + hạ tầng cloud + hỗ trợ
Điểm hòa vốn khách hàng = ? tháng
Giá trị mang lại cho khách = (số pin cứu được × giá pin) + (rủi ro cháy tránh được × xác suất)
```
→ **Việc cần làm:** phỏng vấn ít nhất **3 đơn vị đội xe/gara thật ở Thái Nguyên hoặc Hà Nội** trước Chung kết. Một câu trích dẫn thật từ khách hàng trong slide có sức nặng hơn mọi ước tính.

---

## §6. VIỆC PHẢI LÀM — 12 NGÀY TỚI (03/09 → 26/09)

**Ưu tiên tuyệt đối**
- [ ] **HÔM NAY:** kiểm tra tài khoản WISE-IoT còn sống không; dựng 1 Dashboard bất kỳ (dữ liệu giả cũng được). Đây là điều kiện tồn tại.
- [ ] Xác nhận cả đội đã có chứng chỉ WISE-IoT.

**Quyết định kiến trúc (deadline 06/09)**
- [ ] 1 người thử ESP32 → MQTT/TLS → WISE-IoT IoT Hub. 3 ngày. Kết quả quyết định bỏ Pi hay không.

**Kỹ thuật**
- [ ] Mua pack 4–8 cell + điện trở sưởi 5–10Ω/5W (< 700k)
- [x] ~~Autoencoder chạy trên ESP32~~ → **ĐÃ XONG 11/09**, float32 1,4 KB (không dùng INT8/TFLite — xem `docs/DECISION_LOG.md` QĐ-013). Còn lại: thử với cell bị sưởi THẬT khi có DS18B20
- [ ] Bảng so sánh Autoencoder vs 3 baseline (ngưỡng cứng / ΔT cố định / Isolation Forest)
- [ ] LSTM RUL trên NASA — chỉ cần chạy end-to-end, có RMSE
- [ ] Đo ngân sách điện thật của module ở chế độ đỗ

**Thuyết trình**
- [ ] Slide kiến trúc **2 tầng** (Pi là tùy chọn trạm, nếu còn)
- [ ] Slide BOM + giá bán + ROI đối chiếu chi phí thay pin 1,5–12 triệu
- [ ] Slide "vì sao AI, ngưỡng cứng không đủ" — dùng ví dụ cell #3 ở §4(1)
- [ ] Kịch bản demo 3–5 phút + video dự phòng
- [ ] **Chuẩn bị trả lời trung thực về lead time** — đừng hứa 20 phút

---

## §7bis. ĐÍNH CHÍNH — hai chỗ tớ đã nói sai ở bản đầu

Rà soát lại ngày 03/09 phát hiện:

1. **Tớ nói "không có dataset công khai nào có nhiệt độ tương đối giữa nhiều cell" — SAI.** Có, và có bộ rất tốt. Xem §8. Điều này thực ra là **tin tốt**: đội có thể train Lớp 1 trên dữ liệu thật ngay, không phải chờ tự thu thập.
2. **Bộ Kaggle "EV Battery Charging & Thermal Runaway" mà lộ trình cũ chọn làm dataset chính cho Lớp 1 — nên BỎ.** Lý do ở §8.

Hệ quả: dữ liệu tự thu thập (§3 Tầng B) vẫn có giá trị, nhưng **không còn là nền móng bắt buộc** — nó trở thành phần "chứng minh trên điều kiện VN" bổ sung. Bớt được một rủi ro tiến độ lớn.

---

## §7. NHỮNG CHỖ TỚ CHƯA CHẮC / CẦN KIỂM CHỨNG THÊM

1. **ESP32 nối trực tiếp WISE-IoT IoT Hub** — tớ chưa tìm được tài liệu chính thức xác nhận Advantech hỗ trợ trực tiếp không cần SDK riêng. **Cần hỏi trên [forum InnoWorks](https://forum.wise-paas.advantech.com/) ngay hôm nay.** Đây là ẩn số lớn nhất.
2. **Thống kê cháy nổ** ở §2 là số quốc tế, không phải VN. Nếu tìm được số của Cục PCCC thì thuyết phục hơn nhiều với giám khảo Việt.
3. **Giá Raspberry Pi và linh kiện** lấy từ lộ trình cũ (15/08) — nên kiểm lại giá hiện tại.
4. **Con số "lead time 14 ngày"** đến từ một nghiên cứu cụ thể trên hệ thống lưu trữ năng lượng, chưa chắc chuyển được sang xe máy điện. **Phải đọc bài gốc trước khi đưa lên slide.**

---

## §8. DATASET — BẢN RÀ SOÁT LẠI (03/09)

Tiêu chí lọc: bài toán cốt lõi của đội là **so sánh nhiệt độ giữa các cell trong cùng pack**. Nên câu hỏi duy nhất quan trọng là: *dataset này có đo nhiệt độ RIÊNG cho từng cell không, hay chỉ 1 con số cho cả pack?* Rất nhiều bộ nổi tiếng trượt tiêu chí này.

| Dataset | Nhiệt độ cấp cell? | Chi tiết | Có nhãn lỗi? | Dùng cho |
|---|---|---|---|---|
| **UPC WLTP Pack Cycling** (Sci Data 12:1942, 2025) | ✅ **Tốt nhất** | **2 cảm biến NTC 10kΩ mỗi cell** (đầu + và −) × 36 cell = 72 kênh + 2 ambient. ≥2 Hz. Chu trình lái WLTP thật. 410 chu kỳ. Parquet, ~800 MB. DOI `10.34810/data2395` | ❌ Toàn dữ liệu bình thường | **Lớp 1 (train) + Lớp 2 (RUL)** |
| **Stanford/Warwick parallel module** | ✅ | Thermocouple từng cell + cảm biến Hall, 1s. **54 điều kiện cố ý gây mất cân bằng** (điện trở kết nối, mức lão hóa, nhiệt độ khác nhau). Mendeley `zh58byr53c`, DOI `10.1016/j.dib.2024.110227` | ✅ Có nhãn theo điều kiện | **Lớp 1 (validation/test)** |
| **NASA PCoE B0005/6/7/18** | ❌ 1 cell đơn | Chuẩn ngành, nhiều code mẫu, dễ tải | ❌ | **Lớp 2 (RUL) — bắt đầu từ đây** |
| **CALCE** | ❌ | Test ở −40/−5/25/50°C | ❌ | Lớp 2 + phần chuẩn hóa theo nhiệt môi trường |
| **Multi-stage aging** (Sci Data 2024) | ❌ | Sạch, hiện đại | ❌ | Lớp 2 nâng cao (Chung kết) |
| **Multi-modal Thermal Runaway** (Sci Data 13:1084, 2026) | ⚠️ Không | 3 can nhiệt K nhưng **lấy trung bình thành 1 tín hiệu/cell**. Chỉ **8 thí nghiệm**. 10 Hz. OSF `10.17605/OSF.IO/C2HNQ` | ✅ TR thật, có mốc venting | **Chỉ để tra ngưỡng vật lý** — không đủ để train |
| **ZJU-o LFP (BESS)** | ✅ | 28 module × 16 cell = 1.792 chuỗi, 1 phút/mẫu, 400+ chu kỳ | ? | ⛔ **Chưa có link tải công khai** — chỉ mô tả trong phụ lục preprint. Muốn dùng phải email tác giả. |
| ~~Kaggle "EV Battery Charging & Thermal Runaway"~~ | ❌ | **BỎ** — xem bên dưới | — | — |

### ⛔ Vì sao phải bỏ bộ Kaggle zara2099

Lộ trình cũ chọn bộ này làm dataset chính cho Lớp 1. Kiểm tra kỹ thì có nhiều dấu hiệu là **dữ liệu sinh tự động (synthetic)**:
- Chỉ **93 KB** cho cả một bộ "giám sát thermal runaway thời gian thực"
- Không ghi nguồn gốc, không tác giả, không mô tả thiết bị đo
- Có cột `Notes` bình luận bằng chữ và cột `TR_Probability` đã dọn sẵn — thật thì không ai có sẵn xác suất TR
- **Chí mạng: chỉ có `MaxTemp_C` / `MinTemp_C` / `AvgTemp_C` cho cả pack, KHÔNG có nhiệt độ từng cell.**

Điểm cuối là lý do đủ để loại. Bài toán của đội là "cell #3 nóng hơn các cell còn lại" — bộ này về mặt cấu trúc không thể biểu diễn được điều đó. Train trên nó là xây nhà trên cát, và nếu giám khảo hỏi nguồn gốc dữ liệu thì rất khó đỡ.

### Quy trình dùng dataset — đề xuất cụ thể

```
LỚP 1 (phát hiện cell bất thường)
  Train  ← UPC WLTP (36 cell × 2 NTC, toàn bộ dữ liệu bình thường)
           → Autoencoder học "mẫu hình nhiệt bình thường của cả pack"
  Test A ← Tiêm lỗi tổng hợp vào UPC WLTP: cộng offset nhiệt vào 1 cell,
           tạo dốc tăng nhiệt, drift chậm → có nhãn 100%, tính P/R/F1
  Test B ← Stanford/Warwick (mất cân bằng THẬT, có nhãn theo điều kiện)
           → đây là bằng chứng mạnh nhất, dùng số liệu này lên slide
  Test C ← Pack tự làm + điện trở sưởi (§3) → demo live

LỚP 2 (RUL)
  Train  ← NASA PCoE (dễ, nhanh, có baseline để so)
  Mở rộng ← UPC WLTP (410 chu kỳ có đường suy giảm dung lượng)
  Kiểm chéo ← CALCE (nhiều mức nhiệt môi trường)
```

**Điểm mạnh khi trình bày:** đội train trên dữ liệu bình thường và **kiểm thử trên mất cân bằng thật của một nghiên cứu độc lập** — đó là cách làm đúng chuẩn, mạnh hơn nhiều so với "tải một bộ Kaggle có sẵn nhãn".

**Việc cần làm ngay:**
- [ ] Tải UPC WLTP từ CORA.RDR (DOI 10.34810/data2395) — ~800 MB, cần thời gian
- [ ] Tải Stanford/Warwick từ Mendeley `zh58byr53c`
- [ ] Kiểm tra giấy phép: bài báo CC BY-NC-ND, **dữ liệu có thể khác giấy phép** — đọc kỹ trước khi dùng cho mục đích thương mại. Với cuộc thi/học thuật thì không sao.
- [ ] Lưu ý kỹ thuật: UPC WLTP thiếu chu kỳ 1–124, và có cờ `Balancing` khi lệch >100 mV — phải xử lý khi tiền xử lý, không thì mô hình học nhầm.

---

## §9. KẾT NỐI — "ĐI RA ĐƯỜNG THÌ LẤY MẠNG Ở ĐÂU?"

Câu hỏi này đúng và quan trọng. Nhưng nó chứa một giả định cần gỡ trước.

### Gỡ giả định: ESP32 không cần mạng để CHẠY, chỉ cần mạng để BÁO

Đây chính là lý do tồn tại của Edge AI, và là điểm bán hàng lớn nhất của đội:

| Chức năng | Cần mạng? |
|---|---|
| Đọc cảm biến nhiệt | ❌ |
| Chạy Autoencoder phát hiện cell bất thường | ❌ |
| **Kêu còi / nháy LED / rung cảnh báo tại chỗ** | ❌ |
| Ghi log vào bộ nhớ trong | ❌ |
| Gửi cảnh báo tới điện thoại chủ xe khi ở xa | ✅ |
| Đẩy dữ liệu lên WISE-IoT, chạy RUL, vẽ dashboard | ✅ |

→ **An toàn — thứ quan trọng nhất — hoạt động 100% offline.** Mất mạng không làm hệ thống mù. Nó chỉ làm chậm việc báo cho người ở xa.

Nếu chỉ nhớ một câu từ phần này thì là câu trên. Đây là câu trả lời cho giám khảo, và cũng là điểm khác biệt so với mọi giải pháp chỉ-cloud.

### Bốn phương án kết nối

**(A) Store-and-forward — nền tảng, BẮT BUỘC có, chi phí 0đ**
- ESP32 ghi log vào flash nội bộ khi offline. Về tới vùng Wi-Fi đã biết (nhà, kho, trạm sạc) → tự động đẩy toàn bộ lên.
- **Tính thử dung lượng:** 4 cell × 1 mẫu/60s ở chế độ đỗ ≈ 5.760 mẫu/ngày × ~20 byte ≈ **115 KB/ngày**. ESP32 có 4 MB flash → đệm được **hơn 30 ngày**. Thoải mái.
- Kể cả khi đã có 4G, vẫn nên có lớp này làm dự phòng khi mất sóng.
- ⚠️ **Cần kiểm chứng:** WISE-IoT IoT Hub có nhận dữ liệu quá khứ kèm timestamp riêng không, hay nó tự đóng dấu thời gian lúc nhận? Nếu nó tự đóng dấu thì dữ liệu backfill sẽ dồn cục sai. **Hỏi forum InnoWorks ngay.**

**(B) BLE qua điện thoại chủ xe — hợp nhất với xe máy điện cá nhân, chi phí 0đ**
- ESP32 có sẵn BLE, không tốn thêm linh kiện.
- Khi đang chạy xe, điện thoại chủ xe ở ngay đó → app nhận qua BLE rồi chuyển tiếp lên cloud bằng 4G của điện thoại.
- Khi chủ xe đi khỏi → quay về (A) + còi tại chỗ.
- Đây đúng là cách các thiết bị theo dõi xe đạp điện thương mại đang làm.
- Nhược: phải viết app, và phụ thuộc chủ xe có mở app không.

**(C) 4G LTE Cat-1 — cho khách hàng đội xe (B2B)**
- Module A7670C (SIMCOM, LTE Cat-1, UART) hoặc kit ESP32-S3 + A7670 bán sẵn ở VN.
- Cat-1 đủ dùng và rẻ hơn nhiều Cat-4; dự án này chỉ đẩy vài trăm byte mỗi lần.
- **Phủ sóng di động VN đã đạt 99,8%** → về cơ bản chỗ nào đỗ xe cũng có sóng.
- 🚨 **BẪY CHẾT NGƯỜI — đọc kỹ:** **Việt Nam tắt sóng 2G toàn quốc từ 15/9/2026** — tức là **11 ngày trước Bán kết 26/09**. Mọi module 2G/GSM (SIM800L, SIM900...) sẽ **thành cục sắt**. Rất nhiều tutorial và module giá rẻ trên mạng vẫn là 2G. **Tuyệt đối không dùng.** Chỉ mua module 4G LTE.
  - *Bonus:* đưa chi tiết này vào slide. Nó cho thấy đội hiểu bối cảnh hạ tầng VN 2026 — giám khảo Việt sẽ đánh giá cao.
- Chi phí: module + SIM data. Đẩy giá module lên → chỉ hợp lý cho đội xe.
- ⚠️ Ngân sách điện: 4G phát sóng có đỉnh dòng ~0,5–2A. Cần tụ đệm và phải tính lại phần điện ở §2.

**(D) Wi-Fi tại trạm đổi pin / kho đội xe — "miễn phí" và rất hợp mô hình kinh doanh**
- Trong mô hình đổi pin, **cục pin tự quay về trạm đều đặn**. Trạm có Wi-Fi. Về tới trạm là trút hết dữ liệu.
- Kết hợp với khách hàng #1 ở §5 thì gần như không cần 4G: pin về trạm mỗi 1–2 ngày, mà RUL chỉ cần cập nhật 1 lần/ngày.
- Chi phí kết nối: **0đ/module/tháng**. Đây là lợi thế cạnh tranh thật sự, nên nói rõ trong phần kinh doanh.

### Đề xuất: 3 gói sản phẩm

| Gói | Kết nối | Ai mua | Giá tham khảo |
|---|---|---|---|
| **Basic** | Còi tại chỗ + store-and-forward qua Wi-Fi nhà + BLE tới điện thoại | Cá nhân | ~400–500k |
| **Fleet** | Basic + Wi-Fi trạm/kho | Đội xe, trạm đổi pin | ~500–600k, phí SaaS |
| **Pro** | Fleet + 4G LTE Cat-1 luôn online | Đội xe cần cảnh báo tức thì | ~900k–1,2tr + cước SIM |

Cách này giải quyết được mâu thuẫn ở §1: giữ giá vào cửa thấp, ai cần realtime thì trả thêm.

### Biến điểm yếu thành màn demo mạnh nhất

Kịch bản demo cho Bán kết:

```
1. Dashboard WISE-IoT đang chạy, dữ liệu real-time chảy      (30s)
2. RÚT WI-FI trước mặt giám khảo                              → "giờ xe đang chạy ngoài đường"
3. Bật điện trở sưởi cell #3
4. ESP32 vẫn PHÁT HIỆN và KÊU CÒI — dù đang mất mạng hoàn toàn  ← khoảnh khắc ăn tiền
5. Cắm lại Wi-Fi
6. Dữ liệu của quãng offline TỰ ĐỘNG dồn lên dashboard,
   sự kiện bất thường hiện đúng vị trí trên trục thời gian
```

Màn này chứng minh cùng lúc: Edge AI chạy thật, store-and-forward chạy thật, WISE-IoT tích hợp thật. Và nó trả lời trước câu hỏi mà mọi giám khảo đều sẽ nghĩ tới. Đáng để dành 90 giây trong 5 phút demo.

**Việc cần làm:**
- [ ] Hỏi forum InnoWorks về backfill timestamp trên IoT Hub (chặn kịch bản demo ở bước 6)
- [ ] Cài store-and-forward vào firmware ESP32 (SPIFFS/LittleFS ring buffer)
- [ ] Nếu mua module 4G: **kiểm tra kỹ là 4G LTE, không phải 2G**
- [ ] Thêm 1 slide "Thiết kế cho thực tế VN: không giả định lúc nào cũng có mạng"

---

## Nguồn

**Dataset (rà soát 03/09)**
- [UPC WLTP Pack Cycling Dataset — Nature Scientific Data 2025](https://www.nature.com/articles/s41597-025-06229-5) · dữ liệu: DOI `10.34810/data2395`
- [Multi-modal thermal runaway dataset — Nature Scientific Data 2026](https://www.nature.com/articles/s41597-026-07857-1) · OSF `10.17605/OSF.IO/C2HNQ`
- [Iontech — danh mục tài nguyên giám sát pin mã nguồn mở](https://github.com/shiyunliu-battery/Iontech)
- [NASA PCoE (Kaggle)](https://www.kaggle.com/datasets/patrickfleith/nasa-battery-dataset) · [gốc NASA](https://www.nasa.gov/intelligent-systems-division/discovery-and-systems-health/pcoe/pcoe-data-set-repository/)
- [CALCE](https://calce.umd.edu/battery-data)
- [Multi-stage aging (Nature SD 2024)](https://www.nature.com/articles/s41597-024-03859-z)
- [Danh mục dataset pin tổng hợp](https://github.com/jonathanwvd/awesome-industrial-datasets/blob/master/markdown/li-ion_battery_aging_datasets.md)

**Kết nối & hạ tầng mạng VN**
- [VNPT — 15/9/2026 hệ thống 2G ngừng hoạt động tại Việt Nam](https://vnpt.vn/gioi-thieu/tin-tuc/15-9-2026-he-thong-2g-se-ngung-hoat-dong-tai-viet-nam.html)
- [Viettel — Việt Nam sẽ chỉ còn 4G và 5G từ 2026](https://vieteltelecom.vn/tat-song-2g-3g-phat-trien-4g-5g/)
- [Thanh Niên — lộ trình tắt sóng 2G](https://thanhnien.vn/con-9-thang-nua-se-tat-song-2g-tai-viet-nam-185251212102955025.htm)
- [Makerfabs ESP32-S3 4G LTE Cat-1 (A7670)](https://www.makerfabs.com/esp32s3-4g-lte.html)
- [Module SIM 4G LTE Cat.1 A7670C — bán tại VN](https://caka.vn/module-sim-4g-lte-cat-1-a7670c-simcom-ho-tro-iot-giao-tiep-uart)
- [Lightbug — theo dõi đội xe đạp điện qua BLE/4G](https://lightbug.io/use/bike-ebike-fleet-tracking/)

**Cháy nổ & cảnh báo sớm**
- [EV Fire Safe — nghiên cứu cháy khi sạc](https://www.evfiresafe.com/research-ev-fire-charging)
- [EV battery fire risk: statistical analysis](https://www.evinfrastructurenews.com/ev-battery/fire-risk-statistics)
- [Advances in Early Warning of Thermal Runaway (Wiley 2025)](https://advanced.onlinelibrary.wiley.com/doi/10.1002/adsr.202400165)
- [Early Detection of Li-Ion TR Using Commercial Diagnostics (IOP)](https://iopscience.iop.org/article/10.1149/1945-7111/ad2440)
- [On-Board Implementation of TR Detection (MDPI Energies)](https://www.mdpi.com/1996-1073/19/3/858)
- [Early Warning for TR High-Risk Cells (MDPI)](https://www.mdpi.com/2077-1312/14/7/684)

**TinyML / AI trên vi điều khiển**
- [Embedded strategy for battery module states using tiny ML (ScienceDirect)](https://www.sciencedirect.com/science/article/pii/S2352152X26003373)
- [TinyML RUL cho pin LiPo UAV (Sensors 2025)](https://pmc.ncbi.nlm.nih.gov/articles/PMC12196908/)
- [LSTM vs 1D-CNN cho TinyML (arXiv)](https://arxiv.org/html/2603.04860)
- [So sánh TFLite trên ESP32/Arduino (GitHub)](https://github.com/lbedogni/tinyMLproject)

**Thị trường**
- [Đổi pin xe máy điện VinFast — bảng giá 2026](https://news-vn.imotorbike.com/2026/05/doi-pin-xe-may-dien-vinfast-bao-nhieu-tien/)
- [Danh sách xe máy điện đổi pin tại VN](https://www.24h.com.vn/xe-may-xe-dap/danh-sach-xe-may-dien-doi-pin-tai-thi-truong-viet-nam-c748a1776292.html)
- [Xe điện thay pin có đắt không (2026)](https://www.thegioixedien.com.vn/xe-dien-thay-pin-co-dat-khong-nhung-dong-xe-de-thay-pin-moi-nhat-2026)
- [EV Aftermarket Report 2026](https://www.globenewswire.com/news-release/2026/04/28/3282950/28124/en/electric-vehicle-aftermarket-global-business-report-2026-a-272-5-billion-market-by-2030-driven-by-aging-ev-fleet-growth-battery-centric-maintenance-models-rise-of-digital-service-e.html)
- [volytica — giám sát pin cho e-mobility](https://www.volytica.com/solutions/e-mobility/)

**WISE-IoT**
- [Python Edge SDK Manual (Advantech)](https://learn.advantech.com/EdgeHub/Edge_SDK_WISE-PaaS_MQTT_Protocol/Python_Edge_SDK_Manual/)
- [Forum InnoWorks](https://forum.wise-paas.advantech.com/)

*Các link lấy từ tìm kiếm ngày 03/09/2026 — mở kiểm tra lại trước khi dựa vào.*
