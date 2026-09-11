# Bằng chứng — Lớp 1 AI chạy trên ESP32 thật, 11/09/2026

Autoencoder phát hiện cell bất thường đã chạy **on-device** trên ESP32-S3, và
kết quả đi hết đường lên dashboard mà không phải sửa data contract.

---

## §1 — Mô hình

| | |
|---|---|
| Kiến trúc | 16 → 8 → 4 → 8 → 16, ReLU, nút thắt 4 chiều |
| Tham số | **356** = 1,4 KB float32 (giới hạn là 50 KB) |
| Dữ liệu train | UPC WLTP, 412 chu kỳ, chia theo chu kỳ 287/61/62 |
| Điểm vận hành | ngưỡng p99.9 = 1,8474, phải vượt liên tục **60 giây** |
| Báo động giả | 0,033/giờ = 1 lần mỗi 1,3 ngày |
| Chi phí trên board | +5,5 KB flash, +2,9 KB RAM, chạy 1 Hz |

## §2 — Bản C khớp bản Python từng số

Đây là test quan trọng nhất của cả phần AI. Mô hình được train trên đặc trưng
do `features.py` sinh; firmware tính lệch đi dù chỉ chút ít là autoencoder nhận
đầu vào ngoài phân phối lúc học, và mọi ngưỡng thành vô nghĩa — mà kiểu lỗi đó
không crash, không log, chỉ lặng lẽ làm mô hình sai.

`ai/test_c_vs_python.py` biên dịch `cell_ai.cpp` trên PC, cho cả hai bên ăn
cùng một chuỗi dữ liệu (có cả cell nóng dần và các giá trị bằng nhau để test
nhánh xử lý hoà hạng), rồi so từng phần tử:

```
đặc trưng           lệch tối đa
dev_mean              0.000e+00  OK      dT_diff               0.000e+00  OK
dev_median            0.000e+00  OK      dev_ema               2.985e-07  OK
zscore                4.995e-09  OK      dev_shock             3.615e-07  OK
rank                  4.867e-10  OK      t_std                 4.996e-10  OK
t_minus_amb           0.000e+00  OK      current               4.991e-10  OK
pack_minus_amb        0.000e+00  OK      soc                   4.982e-10  OK
spread                0.000e+00  OK      t_abs                 4.901e-10  OK
dT_cell               0.000e+00  OK
dT_pack               0.000e+00  OK      sai số tái tạo        1.040e-06  OK
                                         cùng cell tệ nhất        100.00%  OK
=== C VA PYTHON KHOP HOAN TOAN ===
```

**Test này đã bắt được một lỗi thật.** Lần đầu, `t_std` lệch **1,3e-3** trong
khi mọi đặc trưng khác lệch cỡ 1e-9. Nguyên nhân là lỗi số học kinh điển: tớ
tính phương sai bằng `E[x²] − E[x]²` với tổng chạy float32. Nhiệt độ ~25 bình
phương rồi cộng 60 mẫu ra ~37.500, trong khi phương sai thật chỉ ~0,001 →
float32 mất sạch chữ số có nghĩa. Xem QĐ-015.

Không có test này thì firmware vẫn chạy, vẫn in ra số, và không ai biết mô
hình đang bị nuôi bằng đầu vào sai.

## §3 — Chạy trên board thật

```
[FS  ] LittleFS OK, spool dang co 0 byte
[WiFi] OK  IP=192.168.2.10  RSSI=-75dBm
[NTP] dong bo gio.... OK -> 2026-09-11T13:07:23.551535Z
[MQTT] KET NOI OK
[AI  ] *** BAT THUONG *** cell 5, diem 5.189 (nguong 1.847), giu 60s
```

Cell #5 (cell được cố ý cho nóng dần) đạt điểm **5,189** trong khi 7 cell còn
lại đều quanh **0,4** — tách biệt gấp hơn 13 lần, không phải sát ngưỡng.

## §4 — Kết quả AI lên tới dashboard

Query InfluxDB sau khi board báo động:

```
_value,tag          _value,tag
1,AI_Alarm          0.339,AI_Score04
0.397,AI_Score01    5.189,AI_Score05   <- cell bất thường
0.402,AI_Score02    0.564,AI_Score06
0.392,AI_Score03    5,AI_WorstCell
                    5.189,AI_WorstScore
```

Đáng chú ý: **không phải sửa một dòng nào** ở Node-RED, InfluxDB hay data
contract. Kết quả AI đi qua đúng payload WISE-PaaS `{"d":{...},"ts":...}` như
dữ liệu nhiệt độ — vì `docs/DATA_CONTRACT.md` chỉ yêu cầu giá trị là số, nên
trạng thái báo động mã hoá bằng 0/1.

Cảnh báo cũng đi qua store-and-forward: mất mạng thì nó được đệm xuống flash
như mọi gói khác, không bị mất.

## §5 — Giới hạn, nói thẳng

| Giới hạn | Hệ quả |
|---|---|
| Chưa từng thấy thermal runaway thật | Mọi số liệu đều trên lỗi **tự tiêm**. Chưa kiểm trên Stanford/Warwick (mất cân bằng thật, có nhãn) — AC-06.8. |
| Bỏ sót trôi nhiệt chậm (38–67%) | Ngưỡng cứng 60 °C **phải giữ**, đang chạy song song trong `runAI()`. |
| Mọi đặc trưng đều tương đối | Không thấy được kiểu lỗi mà **cả pack** cùng bất thường. Lại thêm một lý do giữ ngưỡng cứng. |
| Nhiệt độ trên board vẫn là giả lập | Chưa gắn 8 con DS18B20 thật. Đường đi dữ liệu đã đúng, chỉ còn thay chỗ đọc cảm biến. |

**Cách nói đúng trước giám khảo:** "phát hiện sớm cell bất thường", không phải
"dự báo cháy nổ". Thermal runaway giai đoạn cuối diễn ra trong vài chục giây,
quá nhanh cho mọi mô hình; giá trị của lớp này là bắt được giai đoạn ủ bệnh
hàng chục phút đến hàng giờ trước đó.
