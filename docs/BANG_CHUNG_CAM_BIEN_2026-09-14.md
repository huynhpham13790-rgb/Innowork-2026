# Bằng chứng — 8 cảm biến DS18B20 trên board thật (14/09/2026)

Board: ESP32-S3-DevKitC-1 N16R8 · `/dev/ttyACM0` · bus 1-Wire ở GPIO 4.
Cảm biến **để rời ngoài không khí, chưa dán vào pin**.

Hai sketch:
- `test/ds18b20_bench_test/` (nhánh `debug`) — đếm cảm biến, in ROM.
- `test/ds18b20_stress_test/` — đo nhiễu nền, lệch giữa con, **đếm lỗi bus**.

```
arduino-cli lib install "OneWire"           # 2.3.8
arduino-cli lib install "DallasTemperature" # 4.0.6
arduino-cli compile --fqbn esp32:esp32:esp32s3:PartitionScheme=default_8MB,FlashSize=16M
```

---

## ⛔ KẾT LUẬN TRƯỚC: bus đang chạy ở CHẾ ĐỘ KÝ SINH — phải sửa phần cứng

```
[INFO] Che do nguon: KY SINH (2 day) - canh giac
```

Đây là nguyên nhân gốc của mọi thứ bất thường bên dưới, và nó làm **toàn bộ
số đo về sai lệch giữa các cảm biến trở nên không tin được**.

Chế độ ký sinh (parasite power) là khi chân VDD của DS18B20 không được cấp
nguồn riêng — cảm biến phải **hút điện từ chính dây dữ liệu** trong lúc chuyển
đổi. Với 1–2 cảm biến thì thường sống. Với **8 cảm biến cùng chuyển đổi một
lúc**, điện trở kéo lên không cấp đủ dòng → cảm biến sụt áp giữa chừng → đọc ra
rác hoặc mất hẳn.

**Việc phải làm: nối chân VDD của cả 8 con lên 3,3 V (đấu 3 dây).** Chạy lại
sketch, dòng trên **phải** in ra `CAP NGUON RIENG (3 day)` thì mới đi tiếp
được. Nếu đã nối VDD rồi mà vẫn báo ký sinh thì là mất tiếp xúc ở đường VDD
trên breadboard.

---

## KQ-01 · Cả 8 con lên bus, địa chỉ ROM ổn định — ĐẠT

```
[INFO] GPIO 4 — tim thay 8 thiet bi
const uint8_t PROBE_01[8] = { 0x28, 0x30, 0xF1, 0x01, 0x00, 0x00, 0x00, 0x17 };
const uint8_t PROBE_02[8] = { 0x28, 0xB8, 0xC8, 0x01, 0x00, 0x00, 0x00, 0x2B };
const uint8_t PROBE_03[8] = { 0x28, 0xFA, 0x81, 0x02, 0x00, 0x00, 0x00, 0xA2 };
const uint8_t PROBE_04[8] = { 0x28, 0x46, 0xAF, 0x01, 0x00, 0x00, 0x00, 0x0A };
const uint8_t PROBE_05[8] = { 0x28, 0xEE, 0x07, 0x03, 0x00, 0x00, 0x00, 0xFD };
const uint8_t PROBE_06[8] = { 0x28, 0xD1, 0xF8, 0x03, 0x00, 0x00, 0x00, 0xFD };
const uint8_t PROBE_07[8] = { 0x28, 0x4D, 0x49, 0x04, 0x00, 0x00, 0x00, 0x35 };
const uint8_t PROBE_08[8] = { 0x28, 0x7F, 0xCD, 0x01, 0x00, 0x00, 0x00, 0xE3 };
```

Giống hệt nhau qua cả 4 lần chạy. Đây là những con số cần **dán nhãn lên từng
sợi dây** ngay bây giờ, trước khi dán đầu dò lên pin.

Bước 12 bit = 0,0625 °C, **khớp đúng** mức lượng tử hoá mà `ai/prepare_data.py`
áp lên dữ liệu huấn luyện.

## KQ-02 · Nhiễu nền rất nhỏ — ĐẠT (ở những lần bus còn tốt)

Lần chạy sạch (272 s, 315 vòng đọc): độ lệch chuẩn mỗi kênh **0,019–0,033 °C**.
Không kênh nào nhảy quá 1 °C giữa hai lần đọc liền.

Nhiễu này nhỏ hơn một bậc so với mọi thứ Lớp 1 cần phát hiện. Cảm biến tốt.

## KQ-03 · ĐỘ BỀN BUS — ⛔ KHÔNG ĐẠT, và đây là phát hiện quan trọng nhất

Bốn lần chạy liên tiếp, **cùng một sketch, không ai động vào phần cứng**:

| Lần | Thời lượng | Đọc tốt | Lỗi | Tỉ lệ lỗi |
|---|---|---|---|---|
| 2 | 272 s | 2 520 | 0 | **0,00 %** |
| 3 | 182 s | 1 172 | 548 | **31,86 %** |
| 4 | 150 s | 744 | 408 | **35,42 %** |

Lần 2 sạch tuyệt đối. Hai lần sau hỏng 1/3. Đây **không phải** cảm biến hỏng —
đây đúng là chữ ký của nguồn ký sinh: nó nằm ngay ranh giới hoạt động được,
nên lúc chạy lúc không tuỳ nhiệt độ, tiếp xúc và điện áp.

Hai dấu hiệu xác nhận:

**(a) Cả 8 kênh mất cùng lúc.** Lần 4: mọi kênh đều đúng 51 lần `DISC`. Nếu là
một con cảm biến hỏng thì chỉ một kênh chết. Cả bus cùng chết nghĩa là vấn đề
ở **nguồn hoặc đường dây chung**.

**(b) P08 đọc ra số vô lý nhưng qua được CRC:**
```
P08  trung binh 28.56  nhieu(std) 7.3057   min -20.31   max 32.12   | 72 lan nhay >1C
```
−20 °C và +43 °C trong phòng 28 °C. Cảm biến bị sụt áp giữa lúc chuyển đổi thì
thanh ghi ra giá trị rác.

> ⚠️ **Nếu không đếm lỗi thì không bao giờ thấy chuyện này.** Sketch bench đầu
> tiên in số ra màn hình trông rất đẹp, và lần chạy thứ hai cho 0 lỗi. Chỉ khi
> chạy nhiều lần và **đếm** mới lộ ra. Một hệ an toàn mà cứ 3 lần đọc sai 1 lần
> là hệ vô dụng — mà nó sẽ không báo lỗi gì cả, chỉ âm thầm cho số sai.

## KQ-04 · Lệch giữa các cảm biến — CHƯA KẾT LUẬN ĐƯỢC

Đây là chỗ tớ phải **rút lại kết luận trước đó**. Bản đầu của tài liệu này viết
"sai lệch 0,575 °C, bắt buộc hiệu chỉnh". Con số đó không đứng vững:

| Lần chạy | Độ rộng lệch | P03 | P05 | P06 |
|---|---|---|---|---|
| 1 (bench, 60 s) | 0,575 °C | +0,275 | +0,412 | −0,049 |
| 2 (sạch, 272 s) | 0,443 °C | +0,152 | +0,275 | +0,103 |
| 4 (bus hỏng) | 0,645 °C | −0,027 | +0,469 | −0,035 |

Trong **một** lần chạy, offset rất ổn định (P05 đi +0,2741 → +0,2749 → +0,2791
qua ba lần báo cáo, tức ±0,005 °C). Nhưng **giữa các lần chạy thì lệch tới
0,15 °C** — gấp 5 lần nhiễu đọc.

Sai số chế tạo của cảm biến là hằng số, nó không thể tự đổi. Nên phần chênh
giữa các lần chạy phải đến từ chỗ khác. Hai khả năng, và nhiều khả năng là cả hai:

1. **Nguồn ký sinh làm sai cả giá trị đọc**, không chỉ làm mất gói.
2. **8 đầu dò để rời ngoài không khí thì KHÔNG cùng một nhiệt độ.** Chỉ cần
   luồng gió nhẹ hay một con nằm gần board ấm hơn là chênh 0,1–0,2 °C. Cái ta
   đo được là *(sai số cảm biến) + (chênh nhiệt độ thật giữa các vị trí)*, và
   không tách được hai phần đó.

→ **Không khí không phải môi trường hiệu chuẩn hợp lệ.** Mọi bảng `T_OFFSET`
sinh ra cho tới giờ đều phải vứt.

**Nhưng câu hỏi "có cần hiệu chỉnh không" thì đã trả lời được: CÓ.** Kể cả lần
chạy sạch nhất cũng cho độ rộng 0,443 °C. Lớp 1 nhìn **chênh lệch tương đối
giữa các cell** (QĐ-012), nên lệch cố định trông y hệt "cell này lúc nào cũng
nóng hơn". Tra `models/eval_results.npz`: autoencoder bắt lỗi offset 0,5 °C với
tỉ lệ **41,7 %** — tức là cắm thẳng vào thì có ~4/10 khả năng một kênh bị báo
động giả vĩnh viễn, và nó sẽ không bao giờ tự tắt.

## KQ-05 · Đọc không chặn — chạy được, nhưng chưa đo được thời gian thật

Mẫu code không chặn (`setWaitForConversion(false)` + quay lại lấy kết quả ở
vòng sau) hoạt động: không có lần nào đọc phải giá trị reset 85,0 °C.

*Nói cho đúng:* cột "thời gian chuyển đổi TB 760 ms" trong log **không phải số
đo** — nó chính là ngưỡng chờ tớ đặt cứng trong sketch. Thử nghiệm này chỉ
chứng minh **760 ms là đủ**, không đo được thực tế cần bao nhiêu. Datasheet ghi
≤750 ms cho 12 bit.

⚠️ Lưu ý cho lúc tích hợp: ở **chế độ ký sinh**, thư viện phải giữ dây dữ liệu
ở mức cao suốt quá trình chuyển đổi, nên kiểu đọc không chặn về nguyên tắc
không tương thích với ký sinh. Sau khi đấu lại 3 dây thì vấn đề này tự hết.

---

## Việc phải làm, theo đúng thứ tự

Không được đảo thứ tự — mỗi bước sau chỉ có nghĩa khi bước trước đã xong.

1. **Đấu VDD của cả 8 con lên 3,3 V.** Chạy lại `ds18b20_stress_test`, xác nhận
   in ra `CAP NGUON RIENG (3 day)`.
2. **Chạy lại ít nhất 3 lần, mỗi lần ≥5 phút, đòi tỉ lệ lỗi 0,00 % cả 3 lần.**
   Một lần sạch không chứng minh được gì — lần 2 ở trên đã sạch rồi mà vẫn hỏng.
   Nếu còn lỗi: hạ điện trở kéo lên từ 4,7 kΩ xuống 2,2 kΩ (8 cảm biến + dây
   dài thì 4,7 k hay yếu), rút ngắn dây, kiểm tra mass chung.
3. **Dán nhãn ROM lên từng sợi dây** theo bảng KQ-01, rồi dán đầu dò lên pack.
4. **Hiệu chuẩn offset trong môi trường đẳng nhiệt thật** — không phải không
   khí. Cách làm được: bó cả 8 đầu dò lại, nhúng trong cốc nước ở nhiệt độ
   phòng, khuấy đều, đợi 10 phút cho ổn định rồi mới đo 5 phút. Nước dẫn nhiệt
   tốt hơn không khí hàng trăm lần nên đảm bảo cả 8 con thật sự cùng nhiệt độ.
   Lặp lại 2 lần; hai bảng offset phải khớp nhau trong ~0,05 °C thì mới tin.
5. Sau khi dán lên pack thì **đo lại lần nữa lúc pack nghỉ** (không sạc, không
   xả, cả 8 cell cùng nhiệt độ). Lúc này sai số còn gồm tiếp xúc nhiệt của từng
   mối dán — và đó mới là bảng thật sự nạp vào firmware.

Chỉ sau khi xong bước 5 mới được cho Lớp 1 ăn nhiệt độ thật.

Xem QĐ-021, QĐ-022, QĐ-023 trong `DECISION_LOG.md`.
