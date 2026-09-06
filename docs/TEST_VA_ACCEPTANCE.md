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
