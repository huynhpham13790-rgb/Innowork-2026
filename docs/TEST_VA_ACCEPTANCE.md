# Test & Acceptance Criteria — thế nào là "chạy đúng"

Không có mục nào ở đây được tick bằng cảm giác. Mỗi mục phải có **bằng chứng chạy được** ghi vào `docs/BANG_CHUNG_KIEM_THU_*.md`.

Ký hiệu: ✅ đã kiểm chứng · ⬜ chưa làm · 🔶 làm rồi nhưng chưa có board thật

---

## AC-01 — Đường truyền ESP → cloud thông suốt

| # | Tiêu chí | Cách kiểm | Trạng thái |
|---|---|---|---|
| 1.1 | 4 container đều `running` | `docker compose ps` | ✅ 06/09 |
| 1.2 | Node-RED nối được Mosquitto có auth | log `Connected to broker` | ✅ 06/09 |
| 1.3 | Payload đúng contract → ghi được vào InfluxDB | query Flux thấy đủ 8 tag | ✅ 06/09 |
| 1.4 | Grafana vẽ được 8 đường nhiệt độ | dashboard `hutieu-pin`, panel + ô cảnh báo ngưỡng | ✅ 06/09 |
| 1.5 | Firmware biên dịch cho ESP32-S3 | `arduino-cli compile` | ✅ 06/09 |
| 1.6 | ESP32 thật bắn lên và thấy trên dashboard | nạp board, 800 điểm / 100 mỗi cell, không mất gói | ✅ 06/09 |

## AC-02 — Timestamp do thiết bị quyết định (nền của màn demo)

| # | Tiêu chí | Cách kiểm | Trạng thái |
|---|---|---|---|
| 2.1 | Gói có `ts` lùi 2 giờ được vẽ đúng ở vị trí 2 giờ trước | bắn 1 gói backdate, query Influx | ✅ 06/09 |
| 2.2 | Gói `ts` năm 1970 (NTP fail) bị chặn kèm cảnh báo rõ | bắn gói ts 1970, xem `docker compose logs nodered` | ✅ 06/09 |
| 2.3 | ESP32 không gửi khi NTP chưa đồng bộ | `timeIsValid()` chặn trong `publishData()` | ✅ 06/09 (NTP đồng bộ OK trên board) |
| 2.4 | WISE-IoT thật có tôn trọng `ts` không | **ẩn số** — chỉ trả lời được khi có tài khoản | ⬜ chờ tài khoản |

## AC-03 — Store-and-forward (90 giây đắt nhất của bài demo)

| # | Tiêu chí | Cách kiểm | Trạng thái |
|---|---|---|---|
| 3.1 | Mất mạng → dữ liệu ghi xuống flash, không mất | `test/run_test.sh` TEST 1 | ✅ 06/09 |
| 3.2 | Nối lại → đẩy bù **đúng thứ tự gốc** | TEST 1 | ✅ 06/09 |
| 3.3 | Rớt mạng giữa lúc đang đẩy bù → phần còn lại giữ nguyên, không mất không đảo | TEST 2 | ✅ 06/09 |
| 3.4 | Đệm nhiều hơn 1 lô → chia nhiều lượt, không nghẽn `loop()` | TEST 3 | ✅ 06/09 |
| 3.5 | Flash đầy → bỏ dữ liệu cũ, giữ dữ liệu mới, không phình vô hạn | TEST 4 | ✅ 06/09 |
| 3.6 | **Trên board thật:** ngắt 60s, nối lại, dữ liệu bù về đúng vị trí thời gian | 83 điểm liên tục, 0 lỗ hổng | ✅ 06/09 |
| 3.7 | Không đẩy bù trước khi subscriber kịp nối lại | TEST 6 + chạy thật trên board | ✅ 06/09 |

## AC-04 — An toàn khi đưa lên máy chủ công cộng

| # | Tiêu chí | Cách kiểm | Trạng thái |
|---|---|---|---|
| 4.1 | Node-RED (1880) không lắng nghe trên 0.0.0.0 | `docker compose ps` thấy `127.0.0.1:1880` | ✅ 06/09 |
| 4.2 | InfluxDB (8086) không lắng nghe trên 0.0.0.0 | như trên | ✅ 06/09 |
| 4.3 | MQTT bắt buộc đăng nhập | `allow_anonymous false` + nối thử không user → bị từ chối | ✅ 06/09 |
| 4.4 | Không có secret nào trong git | `git log -p` không thấy `.env`, `passwd` | ✅ 06/09 |
| 4.5 | Lỗi ghi DB hiện trong `docker compose logs`, không im lặng | ép token sai, xem log | ✅ 06/09 |
| 4.6 | Chạy thử trên VPS công cộng thật | dựng lên rồi bắn từ ESP qua 4G | ⬜ trước ngày thi vài hôm |

## AC-05 — Sẵn sàng ngày thi

| # | Tiêu chí | Trạng thái |
|---|---|---|
| 5.1 | Phát WiFi từ điện thoại, không dùng mạng hội trường | ⬜ |
| 5.2 | Có đường lùi: cloud Advantech hỏng → đổi 1 dòng về Plan B | 🔶 cơ chế có sẵn (`#define STAGE`), chưa diễn tập |
| 5.3 | Diễn tập trọn kịch bản 5 phút ít nhất 2 lần | ⬜ |
| 5.4 | Dashboard Grafana đã dựng sẵn, không dựng tại chỗ | ⬜ |

## AC-06 — Lớp 1: AI phát hiện cell bất thường

| # | Tiêu chí | Cách kiểm | Trạng thái |
|---|---|---|---|
| 6.1 | Mô hình < 50 KB | 1,4 KB float32 (356 tham số) | ✅ 11/09 |
| 6.2 | Bắt 100% ca ramp ở mọi tốc độ đã thử | `tune_threshold.py` | ✅ 11/09 |
| 6.3 | **Bản C khớp bản Python từng số** | `test_c_vs_python.py`, lệch <1e-6 | ✅ 11/09 |
| 6.4 | Báo động giả < 1 lần/ngày | 0,033/giờ = 1 lần mỗi 1,3 ngày | ✅ 11/09 |
| 6.5 | Chạy được trên ESP32 thật, 1 Hz | +5,5 KB flash, +2,9 KB RAM | ✅ 11/09 |
| 6.6 | Phát hiện đúng cell trên board thật | cell 5: 5,189 vs 7 cell còn lại ~0,4 | ✅ 11/09 |
| 6.7 | Kết quả AI lên tới dashboard, không đổi data contract | query InfluxDB thấy `AI_*` | ✅ 11/09 |
| 6.8 | Kiểm chứng trên mất cân bằng THẬT (Stanford/Warwick) | — | ⬜ **chưa làm** |
| 6.9 | Ngưỡng cứng 60°C vẫn chạy song song | `runAI()` kiểm độc lập | ✅ 11/09 |
| 6.10 | 8 cảm biến DS18B20 lên bus, ROM ổn định | 8/8 con, ROM giống nhau qua 4 lần chạy | ✅ 14/09 |
| 6.11 | Nhiễu nền nhỏ hơn tín hiệu cần bắt | std 0,019–0,033 °C | ✅ 14/09 |
| 6.12 | **Cảm biến cấp nguồn riêng, không ký sinh** | `isParasitePowerMode()` = true | ⛔ **HỎNG** — QĐ-023 |
| 6.13 | **Bus sạch: 3 lần × ≥5 phút, lỗi 0,00 %** | 0,00 % → 31,86 % → 35,42 % | ⛔ **HỎNG** — chặn bởi 6.12 |
| 6.14 | Bảng offset đo lại được (2 lần khớp ~0,05 °C) | lệch tới 0,15 °C giữa các lần | ⬜ chặn bởi 6.12 + 6.13 |
| 6.15 | Đọc cảm biến không chặn vòng lặp 1 Hz | mẫu code chạy, chưa hợp lệ ở chế độ ký sinh | ⬜ chặn bởi 6.12 |
| 6.16 | Lớp 1 chạy trên nhiệt độ THẬT (không mô phỏng) | — | ⬜ chặn bởi 6.12–6.15 |

## AC-07 — Lớp 2: Dự báo tuổi thọ (RUL/SOH) trên cloud

| # | Tiêu chí | Cách kiểm | Trạng thái |
|---|---|---|---|
| 7.1 | Chỉ dùng đặc trưng đo được ngoài đời (pha sạc) | `nasa_prepare.py`, không dùng dung lượng phóng | ✅ 11/09 |
| 7.2 | Đánh giá trên pin CHƯA TỪNG THẤY | leave-one-battery-out, 4 pin | ✅ 11/09 |
| 7.3 | Thắng baseline "đoán trung bình" | 12,2 vs 25,8 chu kỳ MAE | ✅ 11/09 |
| 7.4 | So với LSTM như tài liệu đề xuất | LSTM 20,3 — **thua** tuyến tính 12,2 | ✅ 11/09 (kết quả ngược kỳ vọng) |
| 7.5 | Sai số dự đoán sớm (25–50% vòng đời) | 9,8 chu kỳ | ✅ 11/09 |
| 7.6 | SOH có ích thương mại | RMSE 3,8 điểm phần trăm | ✅ 11/09 |
| 7.7 | Kiểm chéo trên bộ dữ liệu thứ hai (UPC/CALCE) | — | ⬜ **chưa làm** |
| 7.8 | Nối luồng thật: ESP32 → cloud tính → dashboard | chạy trên board, RUL 85,2→79,6 | ✅ 11/09 |
| 7.9 | Bản C của đặc trưng sạc khớp bản Python | `test_charge_cycle.py` | ✅ 11/09 |
| 7.10 | Cảnh báo khi mô hình ngoại suy | cờ `Extrapolating` + log | ✅ 11/09 |
| 7.11 | Hiệu chỉnh lại hệ số trên pack THẬT của đội | — | ⬜ **bắt buộc trước khi tin con số RUL** |

---

## Cách chạy bộ test

**Test store-and-forward (không cần board, ~10 giây):**
```bash
cd VEDCaPhenika/esp32s3_wiseiot_test && ./test/run_test.sh
```

**Test biên dịch firmware:**
```bash
arduino-cli compile --fqbn esp32:esp32:esp32s3:PartitionScheme=default_8MB,FlashSize=16M \
  VEDCaPhenika/esp32s3_wiseiot_test
```

**Test đường truyền cloud (cần Docker):** làm theo `VEDCaPhenika/PLAN_B_CLOUD.md`, rồi bắn thử một gói bằng `mosquitto_pub` và query lại InfluxDB. Các bước cụ thể và kết quả mong đợi nằm trong `docs/BANG_CHUNG_KIEM_THU_2026-09-06.md`.
