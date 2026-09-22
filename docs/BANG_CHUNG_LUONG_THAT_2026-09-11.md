# Bằng chứng — Nối luồng thật Lớp 2, 11/09/2026

Toàn tuyến chạy thông trên phần cứng thật:

```
ESP32-S3  ──9 đặc trưng pha sạc──►  Mosquitto  ──►  Node-RED (mô hình RUL/SOH)
                                                          │
                                                          ▼
                                                   InfluxDB ──► Grafana
```

Dashboard: `http://localhost:3000/d/hutieu-pin` — có cả 6 panel của Lớp 1 và Lớp 2.

---

## §1 — Chia việc giữa thiết bị và cloud

ESP32 **không** chạy mô hình RUL. Nó chỉ quan sát suốt quá trình sạc rồi gửi
lên 9 con số; Node-RED nhân với hệ số đã train.

Vì sao chia như vậy:
- RUL không cần real-time — 1 lần mỗi chu kỳ sạc là đủ
- Đổi mô hình không phải nạp lại firmware. Quan trọng, vì mô hình **còn phải
  hiệu chỉnh lại** cho pack thật (xem §4)
- ~~Cloud thành nơi tính toán thật ... đúng thứ WISE-IoT được chấm điểm~~
  **SAI, đính chính 22/09/2026:** barem không chấm nền tảng cloud
  (`docs/BAREM_CHAM_BAN_KET.md`). Hai lý do kỹ thuật ở trên vẫn đúng.

Mô hình chỉ là **tích vô hướng** (RUL: 2 tham số; SOH: 10), nên nhúng thẳng hệ
số vào một node function của Node-RED. Không thêm container, không thêm phụ
thuộc — cùng lý do đã không dùng TFLite Micro ở Lớp 1.

## §2 — Bản C khớp bản Python

`ai/test_charge_cycle.py` lấy chu kỳ sạc **thật** từ bộ NASA, bơm từng mẫu vào
`charge_cycle.cpp` biên dịch trên PC, rồi so 9 đặc trưng với bản Python:

```
chu kỳ sạc #168 (10218 giây):
  t_v_interval     python=   1261.0000  C=   1261.0000  OK
  t_cv             python=   8681.0000  C=   8681.0000  OK
  i_cv_mean        python=      0.2816  C=      0.2816  OK
  t_charge_total   python=  10205.0000  C=  10205.0000  OK
  T_max_ch / T_mean_ch / T_rise_ch / v_start / dvdt_cc      OK
=== C VA PYTHON KHOP ===
```

**Test này bắt được hai lỗi thật:**

1. **Định nghĩa đặc trưng không dùng được ngoài đời.** Bản Python đầu tiên lấy
   `t_charge_total = t[-1]`, tức toàn bộ mảng ghi được. Ngoài đời con số đó
   phụ thuộc lúc nào người dùng rút sạc — nhiễu thuần tuý. Đã sửa cả hai bên
   về "cửa sổ từ lúc dòng bắt đầu chạy tới lúc pin no".

2. **Off-by-one một giây.** Bản C bỏ qua chính mẫu kích hoạt chu kỳ sạc vì
   `return` sớm sau khi bật cờ. Lệch đúng 1 giây, chỉ lộ ra ở chu kỳ đầu.

Cũng trong lúc sửa, đã chốt ngưỡng kết thúc sạc ở **0,2 A (~C/10)** thay vì
0,02 A của giao thức phòng thí nghiệm NASA — lý do là khả dụng ngoài đời: mọi
bộ sạc thương mại ngắt quanh C/20–C/10, còn cái đuôi dòng rất thấp phía sau dài
bao lâu là do cài đặt bộ sạc chứ không phải do sức khoẻ pin.

## §3 — Chạy thật, và một cái bẫy tự bắt được

Board mô phỏng một chu kỳ sạc mỗi 45 giây (thời gian ảo), pin "già dần" theo
từng chu kỳ:

```
[CYC ] chu ky sac #1 xong sau 5256s ao -> gui 9 dac trung
[CYC ] chu ky sac #2 xong sau 5292s ao -> gui 9 dac trung
```

Cloud tính ra:

```
20:59:11  Lop 2: RUL=85.2 chu ky, SOH=90.6%
20:59:55  Lop 2: RUL=79.6 chu ky, SOH=90.2%
```

RUL giảm dần đúng quy luật khi pha CV dài ra.

### Cái bẫy: mô hình tuyến tính ngoại suy im lặng

Lần chạy đầu, RUL luôn bằng **0** còn SOH ra **105%** — vô lý. Nguyên nhân:
profile sạc mô phỏng của tớ cho điện áp dâng **tuyến tính**, trong khi pin
lithium thật có vùng điện áp phẳng ở giữa. Ba đặc trưng rơi ngoài dải huấn
luyện, mô hình ngoại suy và trả về số vô nghĩa.

Đáng sợ ở chỗ: **mô hình tuyến tính không bao giờ từ chối trả lời.** Nó nhận
đầu vào cách xa dữ liệu huấn luyện 5 độ lệch chuẩn và vẫn đưa ra một con số
trông rất hợp lý. Nếu chỉ có mình con số RUL trên dashboard thì không ai biết.

Đã thêm **kiểm tra ngoại suy** vào node tính: nếu đặc trưng nào lệch quá 3 độ
lệch chuẩn so với trung bình huấn luyện thì cảnh báo và ghi cờ `Extrapolating`
lên dashboard. Chính nó đã chỉ đúng ba đặc trưng có vấn đề:

```
Lop 2: dac trung NGOAI DAI huan luyen (t_v_interval, T_mean_ch, dvdt_cc)
       -> ket qua khong dang tin
```

Sau khi sửa profile mô phỏng theo đúng số liệu NASA (điện áp có vùng phẳng,
nhiệt độ hiệu chỉnh), cảnh báo tắt và kết quả hợp lý.

Panel **"Mô hình có đang NGOẠI SUY không?"** trên dashboard giữ lại vĩnh viễn —
đây là thứ cần nhất khi gắn pack thật, vì lúc đó đầu vào chắc chắn sẽ khác NASA.

## §4 — Giới hạn, phải nói trước khi ai hỏi

| Giới hạn | Mức nghiêm trọng |
|---|---|
| **Chu kỳ sạc đang MÔ PHỎNG** | Chưa có mạch sạc thật. Đường ống đúng, chỉ còn thay nguồn số liệu bằng INA228 + ADC + DS18B20. |
| **Hệ số train trên pin 18650 ĐƠN 2 Ah, sạc 0,75C** | Pack của đội là 8S cell LG HG2 3 Ah. `t_cv` phụ thuộc mạnh vào tỉ lệ dòng sạc/dung lượng, nên **con số RUL chưa dùng được cho pack thật** cho tới khi hiệu chỉnh lại trên chính pack đó. |
| Điện áp pack quy về trung bình mỗi cell | Xấp xỉ hợp lý sau khi BMS cân bằng, nhưng vẫn là xấp xỉ. |
| Sai số ±12 chu kỳ | Trên tuổi thọ 60–123 chu kỳ là 10–20%. Đủ để lên lịch thay pin, không đủ để hứa ngày hỏng. |

**Cách nói đúng trước giám khảo:** "đường ống đã chạy đầy đủ từ thiết bị tới
dashboard; mô hình đang dùng hệ số hiệu chỉnh trên bộ dữ liệu chuẩn NASA, và
chúng em biết phải hiệu chỉnh lại khi có dữ liệu pack thật". Đừng trình bày
con số RUL như thể đã đo trên pin của đội.
