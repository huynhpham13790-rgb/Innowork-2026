# Bằng chứng — toàn tuyến trên CẢM BIẾN THẬT (14/09/2026)

Lần đầu tiên không còn chỗ nào mô phỏng ở Lớp 1: nhiệt độ là số đo thật từ 8
con DS18B20, chạy qua AI on-device, lên broker của đội, vào InfluxDB, ra Grafana.

Mạng: WiFi trường **ICTU** (mạng mở). ESP `172.172.13.166`, máy chủ
`172.172.3.174`. Broker Plan B (Mosquitto có auth).

## KQ-01 · Phép thử MÙ — AI chỉ đúng cảm biến bị làm nóng

Quy trình: người làm chọn **một đầu dò bất kỳ** rồi áp vào máy tính đang nóng,
**không nói cho AI biết là con nào**. Sau đó đọc log để xem hệ thống chỉ ra kênh
nào. Làm mù để tránh chuyện biết trước đáp án rồi vô tình đọc log theo hướng
mình mong muốn.

Diễn biến nhiệt độ (°C, đã trừ offset):

```
  24.5s | 27.52 27.55 27.56 27.56 27.52 27.49 27.56 27.50   <- nền, cả 8 bằng nhau
  84.5s | 27.70 27.55 27.56 27.81 27.83 27.55 27.75 27.56
 114.5s | 27.83 27.92 27.74 28.43 28.40 27.93 28.12 33.81   <- kênh 8 tách đàn
 144.5s | 27.95 28.11 27.87 28.37 28.40 28.05 28.12 37.81
```

Kết quả AI:

```
162.6s [AI] *** BAT THUONG *** cell 8, diem 55.077 (nguong 1.847), giu 60s
```

| Kênh | 1 | 2 | 3 | 4 | 5 | 6 | 7 | **8** |
|---|---|---|---|---|---|---|---|---|
| Điểm | 1,63 | 1,35 | 1,71 | 1,30 | 1,29 | 1,40 | 1,46 | **55,08** |

Bảy cell còn lại đều **dưới** ngưỡng 1,847. Cell 8 vượt ngưỡng **30 lần**. Không
có kênh nào ở vùng lưng chừng — tách bạch hoàn toàn.

Đáp án đúng: chính là con áp vào máy tính.

*Đáng chú ý:* điểm của 7 cell "bình thường" (1,29–1,71) nằm ngay dưới ngưỡng
1,847 chứ không phải gần 0. Biên an toàn mỏng hơn con số tuyệt đối gợi ý. Lý do:
khi một cell tách hẳn ra, trung bình pack bị kéo lên, làm 7 cell còn lại trông
"lạnh bất thường" so với pack. Đây là hệ quả tất yếu của đặc trưng tương đối
(QĐ-012). Không phải lỗi, nhưng là chỗ cần theo dõi nếu về sau có nhiều hơn một
cell hỏng cùng lúc.

## KQ-02 · Dữ liệu vào tới InfluxDB, đúng data contract cũ

Truy vấn thật, 5 phút gần nhất, measurement `cell`:

```
AI_Alarm          1
AI_WorstCell      8
AI_WorstScore     14.094
Cell01_Temp       28.89
Cell02_Temp       29.05
Cell03_Temp       28.56
Cell04_Temp       29.43
Cell05_Temp       28.65
Cell06_Temp       29.12
Cell07_Temp       28.25
Cell08_Temp       43.18     <- con dang ap vao may tinh
Sensor_ErrPct     0
Sensor_FaultMask  0
Sensor_Found      8
Sensor_Healthy    8
```

Bốn tag `Sensor_*` là phần mới theo QĐ-024, vẫn nằm trong
`{"d":{"<device>":{...}},"ts":...}` nên **không đụng vào data contract**.

Lớp 2 cũng chạy song song: `rul` có 12 điểm trong 10 phút, Node-RED log
`RUL=85,2 → 79,6 → 73,8 chu kỳ`.

## KQ-03 · Bus sạch suốt cả phiên

`Sensor_ErrPct = 0`, `Sensor_Healthy = 8`, 0 lần đổi trạng thái kênh — từ lúc
khởi động tới hết phiên. Xác nhận lại kết luận ở KQ-10 của
`BANG_CHUNG_CAM_BIEN_2026-09-14.md`: lỗi bus trước đó là do **ẩm ở đầu dò**,
lau khô là hết.

## Còn giả những gì

Phải nói rõ để không ai tưởng cả hệ đã thật:

| Thành phần | Trạng thái |
|---|---|
| 8 nhiệt độ cell | ✅ **THẬT** |
| Lớp 1 (phát hiện bất thường) | ✅ **THẬT**, chạy on-device |
| Sức khoẻ cảm biến | ✅ **THẬT** |
| Nhiệt độ môi trường | ⬜ hằng số 28 °C — cần con DS18B20 thứ 9 |
| Dòng điện, SOC | ⬜ hằng số — cần INA228 + đo áp pack |
| Chu kỳ sạc (Lớp 2) | ⬜ **MÔ PHỎNG** — cần mạch sạc thật |
| Hệ số RUL/SOH | ⬜ train trên pin NASA 18650 2 Ah, **chưa hiệu chỉnh cho pack 8S 3 Ah** |
| Pack pin | ⬜ chưa có — đầu dò còn để rời |

Câu nói đúng trước giám khảo: *"Lớp 1 đã chạy hoàn toàn trên số đo thật và
chúng em chứng minh được bằng phép thử mù. Lớp 2 đường ống đã thông, nhưng hệ
số còn của bộ dữ liệu NASA nên con số RUL chưa dùng được cho pack của chúng em."*
