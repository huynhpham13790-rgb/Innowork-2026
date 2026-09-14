# Bằng chứng — 8 cảm biến DS18B20 trên board thật (14/09/2026)

Sketch thử: `test/ds18b20_bench_test/` (nhánh `debug`, commit `5d18ccb`).
Board: ESP32-S3-DevKitC-1 N16R8 · `/dev/ttyACM0` · bus 1-Wire ở GPIO 4.
Cảm biến đang để **ngoài không khí, chưa dán vào pin** — đây là điều kiện đúng
để đo sai số giữa các cảm biến, vì lúc này cả 8 con phải cùng một nhiệt độ.

```
arduino-cli lib install "OneWire"           # 2.3.8
arduino-cli lib install "DallasTemperature" # 4.0.6
arduino-cli compile --fqbn esp32:esp32:esp32s3:PartitionScheme=default_8MB,FlashSize=16M
arduino-cli upload  -p /dev/ttyACM0 --fqbn <như trên>
```

## KQ-01 · Cả 8 con lên bus, đọc được — ĐẠT

```
[INFO] Devices found on GPIO 4: 8
const uint8_t PROBE_01[8] = { 0x28, 0x30, 0xF1, 0x01, 0x00, 0x00, 0x00, 0x17 };
const uint8_t PROBE_02[8] = { 0x28, 0xB8, 0xC8, 0x01, 0x00, 0x00, 0x00, 0x2B };
const uint8_t PROBE_03[8] = { 0x28, 0xFA, 0x81, 0x02, 0x00, 0x00, 0x00, 0xA2 };
const uint8_t PROBE_04[8] = { 0x28, 0x46, 0xAF, 0x01, 0x00, 0x00, 0x00, 0x0A };
const uint8_t PROBE_05[8] = { 0x28, 0xEE, 0x07, 0x03, 0x00, 0x00, 0x00, 0xFD };
const uint8_t PROBE_06[8] = { 0x28, 0xD1, 0xF8, 0x03, 0x00, 0x00, 0x00, 0xFD };
const uint8_t PROBE_07[8] = { 0x28, 0x4D, 0x49, 0x04, 0x00, 0x00, 0x00, 0x35 };
const uint8_t PROBE_08[8] = { 0x28, 0x7F, 0xCD, 0x01, 0x00, 0x00, 0x00, 0xE3 };
```

Không con nào `[DISC]`, không con nào kẹt ở 85,0 °C (giá trị reset). Một bus,
một điện trở kéo lên, 8 cảm biến — đúng như thiết kế.

Bước 0,0625 °C (12 bit) **khớp đúng** mức lượng tử hoá mà `ai/prepare_data.py`
áp lên dữ liệu huấn luyện. Không phải may: đã chọn từ trước để dữ liệu thật và
dữ liệu train có cùng độ phân giải.

## KQ-02 · Nhiễu rất nhỏ — ĐẠT

60 giây, 28 mẫu, cả 8 kênh: độ lệch chuẩn **0,000–0,032 °C**. Nhiễu đọc gần
như bằng 0 so với mọi thứ ta cần phát hiện.

## KQ-03 · Sai lệch GIỮA các cảm biến 0,575 °C — ⚠️ PHẢI XỬ LÝ TRƯỚC KHI GẮN PACK

Cùng một khối không khí, đáng lẽ 8 số phải bằng nhau. Thực tế:

| Kênh | Trung bình (°C) | Lệch so với TB pack | Nhiễu (std) |
|---|---|---|---|
| P01 | 28,006 | −0,101 | 0,019 |
| P02 | 27,944 | −0,163 | 0,015 |
| P03 | 28,382 | **+0,275** | 0,011 |
| P04 | 27,998 | −0,109 | 0,011 |
| P05 | 28,519 | **+0,412** | 0,032 |
| P06 | 28,058 | −0,049 | 0,011 |
| P07 | 27,951 | −0,157 | 0,023 |
| P08 | 28,000 | −0,107 | 0,000 |

**Độ rộng: 0,575 °C.** Con số này nằm trong dải ±0,5 °C mà datasheet DS18B20
công bố, nên **cảm biến không hỏng** — đây là sai số chế tạo bình thường.

### Vì sao nó nguy hiểm với Lớp 1

Lớp 1 không nhìn nhiệt độ tuyệt đối, nó nhìn **chênh lệch tương đối giữa các
cell** (QĐ-012). Một sai lệch cố định +0,41 °C ở P05 trông giống hệt "cell số 5
lúc nào cũng nóng hơn phần còn lại" — tức là đúng cái dấu hiệu ta đang đi tìm.

Sai lệch này **lớn gấp ~18 lần nhiễu đọc** (0,575 so với 0,032). Nó không phải
thứ tự triệt tiêu theo thời gian.

Mức độ nghiêm trọng đo được từ chính kết quả đánh giá Lớp 1
(`models/eval_results.npz`, cột lỗi kiểu `offset`):

| Độ lớn lỗi offset | Tỉ lệ AE phát hiện |
|---|---|
| **0,5 °C** | **41,7 %** |
| 1,0 °C | 58,3 % |
| 2,0 °C | 62,5 % |

Nghĩa là: nếu cắm thẳng 8 cảm biến này vào mà không hiệu chỉnh, có khoảng
**4 trên 10 khả năng P05 bị báo động vĩnh viễn** — báo động giả, ngay trước
mặt giám khảo, và không bao giờ tự tắt.

### Cách xử lý

Hiệu chỉnh offset một lần: để cả 8 đầu dò trong **cùng một môi trường ổn định**
(chai nước ở nhiệt độ phòng, đợi 10 phút cho cân bằng), lấy trung bình mỗi kênh
trong ~60 s, rồi lưu `offset[i] = mean[i] − mean(tất cả)` vào firmware và trừ đi
khi đọc.

⚠️ **Bảng offset ở KQ-03 đo lúc đầu dò để rời ngoài không khí, KHÔNG dùng được
sau khi dán lên pin.** Dán xong phải đo lại: lúc đó chênh lệch còn gồm cả tiếp
xúc nhiệt của từng mối dán, mà đó mới là phần lớn sai số. Đo trước khi pack có
dòng chạy qua (pack nghỉ, cả 8 cell cùng nhiệt độ).

## KQ-04 · Chuyển đổi 750 ms sẽ chặn vòng lặp 1 Hz — ⚠️ CHƯA XỬ LÝ

Đo được chu kỳ vòng lặp 2 146 ms với `delay(1500)` → phần đọc cảm biến tốn
~646 ms. Thư viện mặc định `setWaitForConversion(true)`, tức là
`requestTemperatures()` **chặn** cho tới khi xong.

Firmware chính chạy `runAI()` ở 1 Hz và phải gọi `mqtt.loop()` đều đặn. Chặn
650–750 ms mỗi giây là không chấp nhận được: MQTT sẽ rớt keep-alive.

Cách sửa khi tích hợp: `sensors.setWaitForConversion(false)` → gọi
`requestTemperatures()` rồi **quay lại lấy kết quả ở vòng lặp sau**, không đợi.

## Kết luận

Phần cứng cảm biến **chạy đúng**. Hai việc phải làm trước khi thay nguồn nhiệt
độ mô phỏng trong firmware chính bằng số đo thật:

1. Hiệu chỉnh offset (**sau khi đã dán đầu dò lên pin**) — nếu bỏ qua, Lớp 1
   gần như chắc chắn báo động giả.
2. Đọc cảm biến theo kiểu không chặn.

Xem QĐ-021, QĐ-022 trong `DECISION_LOG.md`.
