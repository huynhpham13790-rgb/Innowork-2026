# LỘ TRÌNH TRIỂN KHAI — InnoWorks 2026 | Đội Hủ Tiếu

> ## ⚠️ TÀI LIỆU NÀY CÓ PHẦN ĐÃ LỖI THỜI — đọc cùng KHUNG_NGHIEN_CUU_v2
>
> Kiến trúc đã đổi sau khi viết file này. **Raspberry Pi đã bị BỎ** (lý do ở
> `KHUNG_NGHIEN_CUU_v2_HuTieu.md` §2), Lớp 2 chuyển lên **WISE-IoT cloud**.
> Mọi chỗ nhắc tới "Pi chạy LSTM" bên dưới đều đã được đánh dấu lại.
> Khi hai tài liệu mâu thuẫn, **KHUNG_NGHIEN_CUU_v2 và `docs/DECISION_LOG.md`
> là đúng**.


**Dự án:** Hệ thống giám sát nhiệt độ pin xe điện tích hợp AI nhúng — Dự đoán tuổi thọ (RUL) & phát hiện bất thường sớm
**Trường:** ĐH Công nghệ Thông tin và Truyền thông — ĐH Thái Nguyên (ICTU)
**Trạng thái:** Đã lọt Top 20 (qua Sơ loại)
**Cập nhật:** 15/08/2026

> Đọc lướt phần **0** (mốc thời gian) và **11** (checklist) trước. Các phần còn lại là chi tiết để tra khi làm.

---

## 0. Mốc thời gian & ràng buộc bắt buộc

| Mốc | Ngày | Ghi chú |
|---|---|---|
| Cập nhật gần nhất | 14/09/2026 | Ngày Bán kết chốt lại **26/09** (landing page BTC), trước đây ghi 15/09 |
| **Vòng Bán kết** | **26/09/2026** | Tại ĐH Phenikaa (Hà Nội) |
| **Vòng Chung kết** | **27/11/2026** | Tại ĐH Phenikaa |

**Hai ràng buộc "sống còn" từ thể lệ (đừng bỏ qua):**

1. **Chứng chỉ WISE-IoT bắt buộc:** đội qua Sơ loại phải tự học và lấy chứng chỉ WISE-IoT trên Advantech IoT Academy để đủ điều kiện vào Bán kết. → Làm NGAY trong tuần này.
2. **Phải có hoạt động trên tài khoản WISE-IoT (tối thiểu 1 Dashboard) chậm nhất 2 tuần trước Bán kết (~01/09/2026).** Không có hoạt động = bị hủy tài khoản và loại.
3. **Điểm Bán kết/Chung kết phụ thuộc lớn vào mức độ dùng WISE-IoT.** → WISE-IoT phải là lớp cloud thật, không phải để "cho có".

**Suy ra ưu tiên 1 tháng tới:** (a) lấy chứng chỉ, (b) đẩy được dữ liệu thật/giả lập lên WISE-IoT và dựng dashboard, (c) có 2 mô hình AI chạy được (dù chưa tối ưu). Tối ưu độ chính xác để dành cho giai đoạn Chung kết.

---

## 1. Phân vai 5 thành viên (gợi ý)

| Vai | Người phụ trách | Nhiệm vụ chính |
|---|---|---|
| **Team Lead + Business** | Phạm Văn Huynh (đội trưởng) | Điều phối, timeline, mô hình kinh doanh/ROI, thuyết trình |
| **AI/ML Engineer 1** | (bạn giỏi Python nhất) | Lớp 1 — Autoencoder phát hiện bất thường + xuất sang C cho ESP32 |
| **AI/ML Engineer 2** | | Lớp 2 — LSTM/CNN-LSTM dự đoán RUL + đánh giá |
| **Embedded/Hardware** | | ESP32-S3 + cảm biến + MQTT (~~Raspberry Pi~~ đã bỏ) |
| **Cloud/WISE-IoT + Demo** | | WISE-IoT dashboard, tích hợp MQTT, quay video demo, slide |

> Ai cũng nên học chứng chỉ WISE-IoT (mỗi đội được cấp tối đa 5 tài khoản). Có thể chia đôi: 2 người "cày" AI, 3 người lo phần cứng + cloud + business trong tháng đầu.

---

## 2. Các giai đoạn & task cụ thể (có deadline)

### Phase 0 — Khởi động (15/08 → 20/08)
- [ ] Đăng ký/nhận tài khoản WISE-IoT từ BTC; tất cả đăng nhập được.
- [ ] Đăng ký khóa WISE-IoT trên IoT Academy, bắt đầu học (xem Phần 7).
- [ ] Lập repo Git chung (GitHub), chia nhánh theo vai.
- [ ] Tải sẵn 3 dataset lõi (NASA, CALCE, EV thermal) về máy (xem Phần 5).
- [ ] Chốt danh sách linh kiện, đặt mua (xem Phần 8).

### Phase 1 — Dữ liệu & mô hình "baseline" (20/08 → 31/08)
- [ ] Tiền xử lý NASA B0005/6/7/18: trích chuỗi nhiệt độ, điện áp, dòng, dung lượng theo chu kỳ.
- [ ] Baseline Lớp 2 (RUL): train 1 LSTM đơn giản trên NASA, đo RMSE/MAE. Mục tiêu: chạy end-to-end, chưa cần đẹp.
- [ ] Baseline Lớp 1 (bất thường): train Autoencoder trên dữ liệu "bình thường", xác định ngưỡng lỗi tái tạo.
- [ ] **Dựng Dashboard WISE-IoT đầu tiên** (dữ liệu giả lập/CSV cũng được) — để thỏa ràng buộc "có hoạt động trước 01/09".

### Phase 2 — Đưa AI lên thiết bị biên (01/09 → 10/09)
- [x] ~~Convert Autoencoder → TFLite → INT8~~ → **ĐÃ XONG 11/09 theo hướng khác**: xuất trọng số float32 (1,4 KB) ra file .h, firmware tự nhân. Không cần TFLite Micro. Xem `docs/DECISION_LOG.md` QĐ-013.
- [ ] Đọc cảm biến thật (NTC/DS18B20 + INA226), chạy inference on-device, bật LED/còi khi bất thường.
- [x] ~~Raspberry Pi chạy LSTM, nhận dữ liệu từ MCU qua UART/I2C~~ → **BỎ Pi.** Lớp 2 chạy trên cloud, ESP32 publish thẳng lên IoT Hub.
- [x] ~~Pi publish qua MQTT → WISE-IoT IoT Hub~~ → **ESP32 publish thẳng**, đã chạy được (xem `docs/BANG_CHUNG_PHAN_CUNG_2026-09-06.md`).

### Phase 3 — Tích hợp WISE-IoT hoàn chỉnh (10/09 → 14/09) — *cho Bán kết*
- [ ] Dashboard WISE-IoT hiển thị nhiệt độ real-time từng cell + chỉ số RUL + cảnh báo.
- [ ] Kịch bản demo 3–5 phút: mô phỏng 1 cell nóng bất thường → cảnh báo → dashboard cập nhật.
- [ ] Slide + video demo dự phòng (phòng khi mạng lỗi tại hội trường).
- [ ] Chuẩn bị phần business/ROI (Phần 9).

### Phase 4 — Bán kết (26/09)
- [ ] Thuyết trình + demo trực tiếp. Nhấn mạnh: Edge AI hoạt động offline + WISE-IoT làm cloud.

### Phase 5 — Nâng cấp cho Chung kết (16/09 → 26/11)
- [ ] Tăng độ chính xác: CNN-LSTM/attention cho RUL; cá nhân hóa Autoencoder theo từng cụm pin.
- [ ] Thu thập dữ liệu thật từ pin xe điện thật (3–5 ngày vận hành) để fine-tune Autoencoder.
- [ ] Hoàn thiện vỏ hộp module plug-and-play, đo chi phí thực tế (BOM).
- [ ] Kiểm thử độ bền, đo độ trễ cảnh báo, thống kê false-positive/negative.
- [ ] Hoàn thiện mô hình kinh doanh + chỉ số ROI cụ thể.

---

## 3. Các phần cần nghiên cứu (đọc để hiểu, không cần thành chuyên gia)

1. **BMS & an toàn pin Li-ion:** cơ chế quá nhiệt (thermal runaway), các giai đoạn (pre-vent, vent, flame), vì sao ngưỡng cố định là không đủ.
2. **SOH vs RUL:** State of Health (sức khỏe hiện tại, %) và Remaining Useful Life (số chu kỳ còn lại). Định nghĩa EOL = còn 80% dung lượng thiết kế.
3. **Autoencoder cho phát hiện bất thường:** ý tưởng "học cái bình thường → lỗi tái tạo cao = bất thường"; cách chọn ngưỡng (percentile của reconstruction error).
4. **LSTM / CNN-LSTM cho chuỗi thời gian:** cửa sổ trượt (sliding window), dự báo nhiều bước.
5. **TinyML & lượng hóa:** INT8 quantization, TFLite Micro, giới hạn RAM/Flash của MCU.
6. **Truyền thông IoT:** MQTT (topic, QoS), UART/I2C giữa MCU↔Pi, Wi-Fi/BLE.
7. **WISE-IoT:** IoT Hub, Dashboard (Grafana), lưu trữ (InfluxDB), EdgeSense/Node-RED.
8. **Chỉ số đánh giá:** RMSE, MAE cho RUL; Precision/Recall/F1 cho phát hiện bất thường; độ trễ cảnh báo.

---

## 4. Hướng AI đề xuất (chi tiết kỹ thuật)

### Lớp 1 — Phát hiện bất thường (Autoencoder, chạy trên MCU)

- **Đầu vào (features):** cửa sổ trượt các giá trị nhiệt độ từng cell, tốc độ tăng nhiệt (dT/dt), điện áp, dòng. Ví dụ vector 8–16 chiều.
- **Kiến trúc:** Autoencoder dày (dense) nhỏ, ví dụ `16 → 8 → 4 → 8 → 16`. Nếu cần bắt quan hệ thời gian mạnh hơn → **LSTM-Autoencoder** (nhưng nặng hơn, cân nhắc để trên Pi).
- **Huấn luyện:** chỉ dùng dữ liệu **vận hành bình thường**. Loss = MSE tái tạo.
- **Ngưỡng phát hiện:** đặt tại percentile 95–99 của reconstruction error trên tập validation bình thường. Lỗi vượt ngưỡng liên tục N mẫu → cảnh báo.
- **Triển khai (ĐÃ LÀM, khác kế hoạch):** Keras → trọng số float32 → C array → firmware tự nhân. Mô hình 1,4 KB, nhỏ hơn mục tiêu 50 KB tới 35 lần nên INT8 là không cần thiết. Xem `docs/DECISION_LOG.md` QĐ-013 và `ai/README.md`.
- **Mẹo:** dùng **Edge Impulse** để rút ngắn thời gian nếu nhóm chưa quen quy trình quantize thủ công (xuất trực tiếp `.tflite`/thư viện Arduino).

### Lớp 2 — Dự đoán RUL (LSTM/CNN-LSTM, chạy trên **WISE-IoT cloud**, không phải Pi)

- **Đầu vào:** đặc trưng theo chu kỳ (dung lượng phóng, thời gian sạc CC/CV, nhiệt độ trung bình/đỉnh, nội trở nếu có).
- **KẾT QUẢ THỰC TẾ (11/09):** đã thử cả LSTM, Gradient Boosting, Ridge và hồi quy tuyến tính. **Tuyến tính 1 đặc trưng thắng** (MAE 12,2 chu kỳ vs LSTM 20,3) vì chỉ có 4 viên pin để học. Xem `docs/BANG_CHUNG_LOP2_RUL_2026-09-11.md`. Phần dưới giữ lại làm hướng mở rộng khi có nhiều pin hơn.
- **Kiến trúc gợi ý ban đầu (theo mức độ):**
  - Cơ bản: LSTM 1–2 lớp → Dense → giá trị RUL.
  - Tốt hơn: **CNN-LSTM** (CNN trích đặc trưng cục bộ, LSTM học phụ thuộc dài) — nhiều repo tham khảo ở Phần 5.
  - Nâng cao (Chung kết): thêm attention/Transformer.
- **Nhãn:** RUL = số chu kỳ còn lại đến khi dung lượng < 80%. ✅ đã làm.
- **Đầu vào:** CHỈ đặc trưng pha sạc, không dùng dung lượng phóng (rò rỉ nhãn + không đo được ngoài đời) — QĐ-016.
- **Đánh giá:** RMSE, MAE, và "early prediction error" (dự đoán sớm ở 25–50% vòng đời).
- **Tần suất chạy:** 1 lần mỗi chu kỳ sạc → không cần GPU. Mô hình cuối chỉ 2 tham số, chạy vài mili-giây.

### Các phương án thay thế / dự phòng
- Bất thường: **Isolation Forest / One-Class SVM** (nhẹ, dễ, làm baseline so sánh).
- RUL: **Gaussian Process Regression** hoặc **CEEMDAN + Transformer** (nếu muốn "wow" ở Chung kết).

---

## 5. Dữ liệu — nguồn, link & cách dùng

> **Chiến lược:** dùng dataset công khai để train mô hình RUL và làm baseline Autoencoder. Dữ liệu **bất thường nhiệt thật** rất hiếm → dùng dataset thermal-runaway + tự sinh dữ liệu bất thường (mô phỏng cell nóng) cho Lớp 1.

### A. Dự đoán RUL / lão hóa pin
- **NASA PCoE (lõi chính, B0005/B0006/B0007/B0018):** 4 pin 18650, có nhiệt độ–điện áp–dòng–dung lượng theo chu kỳ. Trang gốc: https://www.nasa.gov/intelligent-systems-division/discovery-and-systems-health/pcoe/pcoe-data-set-repository/
  - **Bản dễ tải trên Kaggle (khuyên dùng):**
    - https://www.kaggle.com/datasets/patrickfleith/nasa-battery-dataset
    - https://www.kaggle.com/datasets/ckskaggle/li-ion-battery-dataset-from-nasa-pcoe
- **CALCE (ĐH Maryland):** nhiều loại cell (LCO/LFP/NMC), có test theo nhiều nhiệt độ (-40/-5/25/50°C). https://calce.umd.edu/battery-data
- **Multi-stage aging dataset (Nature Scientific Data 2024):** dữ liệu lão hóa nhiều giai đoạn, sạch, hiện đại. https://www.nature.com/articles/s41597-024-03859-z
- **Tổng hợp nhiều dataset pin:** https://github.com/jonathanwvd/awesome-industrial-datasets/blob/master/markdown/li-ion_battery_aging_datasets.md

### B. Bất thường nhiệt / thermal runaway (cho Lớp 1)
- **EV Battery Charging & Thermal Runaway (Kaggle, 1 phút/mẫu, có nhiệt độ+điện áp+dòng+BMS):** rất hợp với bài toán của đội. https://www.kaggle.com/datasets/zara2099/ev-battery-charging-and-thermal-runaway-dataset
- **ORNL — thermal runaway do tác động cơ học (có nhãn mức độ 0–100):** https://www.sciencedirect.com/science/article/pii/S2352340924005766
- **Battery Failure Databank (Nature Energy) — hàng trăm test thermal runaway:** https://www.nature.com/articles/s41560-024-01497-8

### C. Cách dùng nhanh
1. Bắt đầu với **NASA (Kaggle)** cho Lớp 2 — dễ tải, nhiều code mẫu.
2. Dùng **EV Thermal Runaway (Kaggle)** cho Lớp 1 — lấy đoạn "bình thường" để train, đoạn "runaway" để test phát hiện.
3. Tự sinh thêm dữ liệu bất thường: lấy chuỗi nhiệt bình thường rồi cộng nhiễu/đường tăng nhiệt dốc để mô phỏng cell lỗi → tăng dữ liệu test.

---

## 6. Quy trình train cụ thể (pipeline mẫu)

```
1. Môi trường:   Python 3.10, tensorflow, keras, numpy, pandas, scikit-learn, matplotlib
2. Tiền xử lý:   - đọc .mat/.csv → DataFrame theo chu kỳ
                 - chuẩn hóa (StandardScaler / MinMax), LƯU scaler để dùng lại trên thiết bị
                 - tạo sliding window (vd 20–50 bước) cho LSTM
3. Chia dữ liệu: train/val/test theo PIN (không trộn chu kỳ cùng pin để tránh rò rỉ)
4. Train:        - Lớp 1: Autoencoder chỉ trên dữ liệu bình thường, EarlyStopping theo val_loss
                 - Lớp 2: LSTM/CNN-LSTM, loss=MSE, optimizer=Adam
5. Đánh giá:     - Lớp 1: chọn ngưỡng theo percentile; tính Precision/Recall/F1 trên tập có nhãn bất thường
                 - Lớp 2: RMSE, MAE; vẽ đường RUL dự đoán vs thực tế
6. Xuất mô hình: - Lớp 1: trọng số float32 → C array; kiểm chứng bản C khớp bản Python từng số (`ai/test_c_vs_python.py`)
                 - Lớp 2: chạy trên cloud nên không cần lượng hoá, giữ nguyên float32
7. Kiểm thử on-device: đo RAM/Flash, độ trễ inference, so sánh output PC vs MCU
```

**Repo tham khảo (đọc code, đừng copy nguyên):**
- Autoencoder anomaly TinyML (ESP32, có notebook convert TFLite): https://github.com/ShawnHymel/tinyml-example-anomaly-detection
- RUL bằng Autoencoder + LSTM/CNN trên NASA + UNIBO: https://github.com/MichaelBosello/battery-rul-estimation
- CNN-LSTM RUL: https://github.com/huzaifi18/RUL_prediction
- 2 tầng LSTM (SOH → RUL): https://github.com/MoHoss007/Li-Ion-Battery-RUL-SOH-Prediction
- LSTM-Autoencoder cho RUL: https://github.com/sukrialfian/Remaining-Useful-Life-Prediction-Of-Lithium-Ion-Battery-Based-on-LSTM-Autoencoder-Method
- CNN-LSTM multi-channel: https://github.com/mayankbaluni/Battery_Remaining_Useful_Life
- Nhập môn ML trên Arduino: https://docs.arduino.cc/tutorials/nano-33-ble-sense/get-started-with-machine-learning/

---

## 7. WISE-IoT — học, video & cách tích hợp

### Học & lấy chứng chỉ (LÀM TRƯỚC — bắt buộc)
- Advantech IoT Academy (đăng ký khóa miễn phí): https://academy.advantech.com/en/courses
- WISE-PaaS Core Level I (tổng quan kiến trúc): https://academy.advantech.com/catalog/info/id:499
- WISE-IoTSuite/Dashboard Level I (dựng dashboard kéo-thả): https://academy.advantech.com/catalog/info/id:1597

### Video YouTube hướng dẫn (link từ kết quả tìm kiếm — kiểm tra lại khi mở)
- WISE-PaaS Data Visualization (Dashboard) Introduction: https://www.youtube.com/watch?v=VN4heB2t_Po
- Advantech's WISE-PaaS 3.0 | Introduction: https://www.youtube.com/watch?v=NiUjC_EPVfw
- Advantech WISE-PaaS Industrial IoT Cloud Platform: https://www.youtube.com/watch?v=y7LrX5Zdibk
- Playlist "WISE-IoT Explained | Powered by WISE-Edge": https://www.youtube.com/playlist?list=PLfSyeb6482zhmqS3xba48jwzDnhULf8_i

### Tài liệu & hỗ trợ kỹ thuật
- Quick Start Guide (v3.0): https://docs.wise-paas.advantech.com/en/Guides_and_API_References/Industrial_IoT_Cloud_Platform/Quick_Start_Guide/1590568453944497947/v3.0.0
- Tài liệu Dashboard: https://docs.wise-paas.advantech.com/en/Guides_and_API_References/ApplicationServices/Dashboard/1605799235152848843/v3.0.21
- WISE-PaaS/EdgeSense (Node-RED để nối thiết bị ↔ cloud): https://ess-wiki.advantech.com.tw/view/WISE-PaaS/EdgeSense
- Forum kỹ thuật riêng cho InnoWorks (hỏi HQ, tải tài liệu): https://forum.wise-paas.advantech.com/

### Luồng tích hợp đề xuất
```
ESP32-S3 (Autoencoder float32, 1,4 KB — Lớp 1, on-device)
   │  MQTT trực tiếp, topic /wisepaas/scada/{nodeId}/data
   ▼
WISE-IoT IoT Hub (RabbitMQ)   [Plan B: Mosquitto + Node-RED]
                                          │
                                          ├─► Lưu trữ: InfluxDB (time-series)
                                          └─► WISE-IoT Dashboard (Grafana):
                                              nhiệt độ/cell, RUL, cảnh báo, báo cáo
```
- Học trước cách publish MQTT từ ESP32/Pi (broker Mosquitto) rồi trỏ sang WISE-IoT IoT Hub. Tham khảo MQTT ESP32: https://randomnerdtutorials.com/esp32-mqtt-publish-subscribe-arduino-ide/
- Nếu bí, dùng EdgeSense/Node-RED làm cầu nối (kéo-thả) thay vì code thuần.

---

## 8. Phần cứng — danh sách & nơi mua (VN)

| Linh kiện | Vai trò | Giá tham khảo |
|---|---|---|
| ESP32 / STM32F4 | MCU chạy Autoencoder | 150.000–300.000đ |
| ~~Raspberry Pi 4 (2–4GB)~~ | ~~Edge AI chạy LSTM~~ | ~~1,5–2,5 triệu~~ — **ĐÃ BỎ**, tiết kiệm được khoản này |
| Cảm biến nhiệt NTC / DS18B20 | Đo nhiệt từng cell | ~20.000đ/cái |
| IC INA226 | Đo dòng/áp (I2C) | ~40.000đ/cái |
| Pin/cell mẫu + đế, dây, breadboard | Dựng nguyên mẫu | — |
| Còi/LED | Cảnh báo cục bộ | — |

Mua ở: các cửa hàng linh kiện điện tử VN (Hshop, Nshop, ICdayroi, Điện Tử Tương Lai...). **Tổng nguyên mẫu < 3,5 triệu.** Đặt sớm để không kẹt tiến độ Phase 2.

---

## 9. Tiêu chí chấm & mẹo ghi điểm

- **WISE-IoT chiếm phần lớn điểm Bán kết/Chung kết** → dashboard phải đẹp, dữ liệu chạy thật, dùng đúng dịch vụ WISE-IoT (IoT Hub + Dashboard).
- **Mô hình kinh doanh + ROI rõ ràng:** đội vô địch 2025 (VGU) thắng nhờ ROI cụ thể. Chuẩn bị con số: chi phí module ~300–500k + 50k/tháng SaaS vs. chi phí thay pin 3–10 triệu → tiết kiệm/ngăn cháy.
- **Edge AI thật:** nhấn mạnh hệ thống chạy được **khi mất mạng** (inference tại thiết bị) — điểm khác biệt so với giải pháp chỉ-cloud.
- **Demo trực quan:** có kịch bản "cell nóng bất thường → cảnh báo sớm" chạy live.
- **Số liệu định lượng:** độ trễ cảnh báo (phút), độ chính xác RUL (RMSE), false alarm rate.

---

## 10. Rủi ro & phương án dự phòng

| Rủi ro | Dự phòng |
|---|---|
| Không kịp lấy chứng chỉ WISE-IoT | Ưu tiên tuyệt đối tuần này; chia người học song song |
| Thiếu dữ liệu bất thường thật | Tự sinh dữ liệu bất thường + dùng dataset thermal-runaway |
| Mô hình quá nặng cho MCU | Giảm kích thước, INT8, hoặc chuyển bớt sang Pi; dùng Edge Impulse |
| Mạng lỗi khi demo | Quay sẵn video demo + chạy local dashboard dự phòng |
| Phần cứng về trễ | Bắt đầu bằng dữ liệu giả lập/CSV để không chặn phần AI & cloud |

---

## 11. Checklist nhanh trước Bán kết (26/09)

- [ ] Cả đội có chứng chỉ WISE-IoT.
- [ ] Tài khoản WISE-IoT có ít nhất 1 Dashboard hoạt động **trước 01/09**.
- [ ] Autoencoder chạy trên ESP32/STM32, phát hiện bất thường + cảnh báo LED/còi.
- [ ] LSTM RUL chạy trên Raspberry Pi, xuất giá trị hợp lý.
- [ ] Pi đẩy dữ liệu qua MQTT lên WISE-IoT, Dashboard hiển thị real-time.
- [ ] Kịch bản demo live 3–5 phút + video dự phòng.
- [ ] Slide: vấn đề → giải pháp → kiến trúc → demo → business/ROI.
- [ ] Số liệu: độ trễ cảnh báo, RMSE của RUL, chi phí BOM.

---

## Nguồn tham khảo

**Cuộc thi & WISE-IoT**
- Trang InnoWorks 2026: https://giaiphap.advantech.com.vn/aiot-innoworks/
- IoT Academy: https://academy.advantech.com/en/courses
- Docs WISE-PaaS: https://docs.wise-paas.advantech.com/en
- Forum InnoWorks: https://forum.wise-paas.advantech.com/

**Dataset**
- NASA PCoE: https://www.nasa.gov/intelligent-systems-division/discovery-and-systems-health/pcoe/pcoe-data-set-repository/
- NASA (Kaggle): https://www.kaggle.com/datasets/patrickfleith/nasa-battery-dataset
- CALCE: https://calce.umd.edu/battery-data
- EV Thermal Runaway (Kaggle): https://www.kaggle.com/datasets/zara2099/ev-battery-charging-and-thermal-runaway-dataset
- Multi-stage aging (Nature): https://www.nature.com/articles/s41597-024-03859-z
- Battery Failure Databank: https://www.nature.com/articles/s41560-024-01497-8
- Danh mục dataset pin: https://github.com/jonathanwvd/awesome-industrial-datasets/blob/master/markdown/li-ion_battery_aging_datasets.md

**Code / mô hình**
- TinyML anomaly (ShawnHymel): https://github.com/ShawnHymel/tinyml-example-anomaly-detection
- battery-rul-estimation (Bosello): https://github.com/MichaelBosello/battery-rul-estimation
- RUL_prediction (huzaifi18): https://github.com/huzaifi18/RUL_prediction
- SOH→RUL LSTM (MoHoss007): https://github.com/MoHoss007/Li-Ion-Battery-RUL-SOH-Prediction
- LSTM-Autoencoder RUL (sukrialfian): https://github.com/sukrialfian/Remaining-Useful-Life-Prediction-Of-Lithium-Ion-Battery-Based-on-LSTM-Autoencoder-Method

*Lưu ý: các link YouTube/GitHub lấy từ kết quả tìm kiếm ngày 15/08/2026 — nên mở kiểm tra lại nội dung trước khi dựa vào.*
