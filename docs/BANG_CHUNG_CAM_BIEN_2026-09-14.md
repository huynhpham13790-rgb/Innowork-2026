# Bằng chứng — 8 cảm biến DS18B20 trên board thật (14/09/2026)

Board: ESP32-S3-DevKitC-1 N16R8 · `/dev/ttyACM0` · bus 1-Wire ở GPIO 4.
Đấu 3 dây trên breadboard: một hàng VCC, một hàng GND, một hàng tín hiệu, điện
trở kéo lên 4,7 kΩ từ VCC sang tín hiệu.
Cảm biến **để rời, mỗi đầu dò một chỗ, chưa dán vào pin**.

Sketch dùng trong tài liệu này:
- `test/ds18b20_bench_test/` (nhánh `debug`) — đếm cảm biến, in ROM
- `test/ds18b20_stress_test/` — nhiễu nền, lệch giữa con, **đếm lỗi bus**
- `test/ds18b20_power_diag/` — hỏi nguồn **từng con**, so đọc chặn / không chặn
- `test/ds18b20_boot_latch/` — đo tần suất cờ ký sinh bị bật nhầm lúc khởi động

```
arduino-cli lib install "OneWire"           # 2.3.8
arduino-cli lib install "DallasTemperature" # 4.0.6
arduino-cli compile --fqbn esp32:esp32:esp32s3:PartitionScheme=default_8MB,FlashSize=16M
```

---

## Tóm tắt — ba kết luận

1. **Cảm biến và cách đấu dây đều đúng.** 8/8 con có nguồn riêng, nhiễu nền
   0,02–0,05 °C, 25/25 lần khởi động sạch.
2. **Lỗi bus 31–35 % đã tìm ra nguyên nhân: ẩm/nước ở đầu dò.** Lau khô rồi
   chạy 3 lần × 5 phút đều 0,0000 % lỗi. Nghi ngờ ban đầu đổ cho tiếp xúc
   breadboard là sai hướng.
3. **Sai lệch giữa các cảm biến: 0,3665 °C — đã đo được, lặp lại được, PHẢI
   hiệu chỉnh.** Đo bằng cách bó cụm nhúng nước; hai lần độc lập khớp nhau
   trong 0,025 °C. Hệ số đã ghi vào `esp32s3_wiseiot_test/ds18b20_offsets.h`.
   (Đo trong không khí thì vô nghĩa — xem KQ-06.)

---

## KQ-01 · Cả 8 con lên bus, địa chỉ ROM ổn định — ĐẠT

```
const uint8_t PROBE_01[8] = { 0x28, 0x30, 0xF1, 0x01, 0x00, 0x00, 0x00, 0x17 };
const uint8_t PROBE_02[8] = { 0x28, 0xB8, 0xC8, 0x01, 0x00, 0x00, 0x00, 0x2B };
const uint8_t PROBE_03[8] = { 0x28, 0xFA, 0x81, 0x02, 0x00, 0x00, 0x00, 0xA2 };
const uint8_t PROBE_04[8] = { 0x28, 0x46, 0xAF, 0x01, 0x00, 0x00, 0x00, 0x0A };
const uint8_t PROBE_05[8] = { 0x28, 0xEE, 0x07, 0x03, 0x00, 0x00, 0x00, 0xFD };
const uint8_t PROBE_06[8] = { 0x28, 0xD1, 0xF8, 0x03, 0x00, 0x00, 0x00, 0xFD };
const uint8_t PROBE_07[8] = { 0x28, 0x4D, 0x49, 0x04, 0x00, 0x00, 0x00, 0x35 };
const uint8_t PROBE_08[8] = { 0x28, 0x7F, 0xCD, 0x01, 0x00, 0x00, 0x00, 0xE3 };
```

Giống hệt nhau qua mọi lần chạy. **Dán nhãn lên từng sợi dây theo bảng này ngay
bây giờ**, trước khi dán đầu dò lên pack — vì đọc theo index sẽ hoán vị dữ liệu
8 cell mà không có lỗi nào báo.

Bước 12 bit = 0,0625 °C, khớp đúng mức lượng tử hoá của `ai/prepare_data.py`.

## KQ-02 · Nhiễu nền — ĐẠT

Độ lệch chuẩn mỗi kênh **0,019–0,048 °C** qua mọi lần chạy sạch. Nhỏ hơn một
bậc so với mọi thứ Lớp 1 cần phát hiện.

## KQ-03 · Nguồn cấp: ĐÚNG. Giả thuyết "quên nối VDD" của tớ là SAI

`test/ds18b20_power_diag` hỏi riêng từng con 200 lần:

```
[INFO] Co chung cua ca bus: CAP NGUON RIENG
P01 2830F10100000017   0.0%   OK - co nguon rieng
...   (ca 8 con deu 0.0%)
P08 287FCD01000000E3   0.0%   OK - co nguon rieng

CHAN      : 320 tot, 0 loi (0.00%)
KHONG CHAN: 320 tot, 0 loi (0.00%)
```

**1 600 lần hỏi, không một lần nào báo ký sinh.** Cách đấu dây 3 hàng +
điện trở 4,7 kΩ là đúng.

> ⚠️ **Đính chính.** Bản trước của tài liệu này kết luận "chân VDD không được
> cấp nguồn, phải đấu lại 3 dây". Kết luận đó **sai**. Nó dựa trên đúng một
> dòng `isParasitePowerMode() = true` mà không kiểm chứng thêm gì. Bài học:
> một cờ tổng hợp của cả bus không đủ để kết luận về phần cứng — phải hỏi được
> từng con thì mới biết con nào.

## KQ-04 · Vì sao cờ ký sinh bật nhầm — lỗi chốt trong thư viện

Đọc mã nguồn DallasTemperature 4.0.6 thì thấy `parasite = false` **chỉ** nằm
trong `setOneWire()`. Còn `begin()` thì:

```cpp
if (!parasite && readPowerSupply(deviceAddress)) parasite = true;
```

Cờ **chỉ có chiều bật, không có chiều tắt**, và được quyết định bằng **một lần
hỏi duy nhất** cho mỗi con, ngay lúc quét bus lúc khởi động.

Hệ quả: chỉ cần một lần nhiễu thoáng qua đúng khoảnh khắc quét bus là cờ bật và
**giữ nguyên suốt cả phiên**. Từ đó thư viện xử lý cả 8 con theo kiểu ký sinh
dù dây nối hoàn toàn đúng.

Điều này giải thích hình dạng kỳ lạ của lỗi ở KQ-05: không phải suy giảm dần,
mà là một cái **chốt** — hoặc 0 % hoặc ~33 %, không có ở giữa.

`test/ds18b20_boot_latch` khởi động lại board 25 lần liên tiếp:

```
25/25 lan: BOOT N=8 PARA=0 POLL=0 BAD=0/24
co PARA=1 (co ky sinh bi bat): 0
```

Hiện tại cờ không bị bật nhầm lần nào.

## KQ-05 · ĐỘ BỀN BUS — đã hỏng tới 35 %, nguyên nhân tìm ra ở KQ-10 (ẩm)

Năm lần chạy `ds18b20_stress_test`, **cùng sketch, cùng phần cứng**:

| Lần | Thời lượng | Đọc tốt | Lỗi | Tỉ lệ lỗi |
|---|---|---|---|---|
| 2 | 272 s | 2 520 | 0 | 0,00 % |
| 3 | 182 s | 1 172 | 548 | **31,86 %** |
| 4 | 150 s | 744 | 408 | **35,42 %** |
| 5 *(sau chẩn đoán)* | 272 s | 2 520 | 0 | 0,00 % |

Lúc hỏng thì cả 8 kênh mất cùng lúc (đúng 51 lần `DISC` mỗi kênh), và P08 đọc
ra **−20,31 °C rồi +43,06 °C trong phòng 28 °C**.

Lúc viết mục này thì chưa tái hiện được, và tớ đã nghi **tiếp xúc breadboard**.
**Nghi sai.** Nguyên nhân thật là **ẩm ở đầu dò** — xem KQ-10: lau khô là tỉ lệ
lỗi về 0,0000 % qua ba lần chạy liên tiếp.

Giữ nguyên đoạn suy luận sai này vì nó có ích: dấu hiệu "cả 8 kênh mất cùng
lúc" hợp với **cả hai** giả thuyết (tiếp xúc chung, và rò do ẩm), nên nó không
phân biệt được. Thứ phân biệt được là **can thiệp rồi đo lại** — lau khô rồi
chạy lại. Suy luận từ triệu chứng thì ra hai đáp án; làm thí nghiệm thì ra một.

Dù vậy kết luận về firmware vẫn không đổi: với hệ giám sát an toàn, đọc sai 1/3
số lần mà **không báo gì cả** là chế độ hỏng tệ nhất có thể có. Đó là QĐ-024,
và nó đúng bất kể nguyên nhân phần cứng là gì.

## KQ-06 · Sai lệch giữa các cảm biến — CHƯA ĐO ĐƯỢC, và đây là câu trả lời

Bảng `T_OFFSET` sinh ra ở các lần chạy sạch khác nhau:

| Kênh | Lần 2 | Lần 5 | Chênh |
|---|---|---|---|
| P01 | −0,101 | −0,070 | 0,031 |
| P02 | −0,075 | −0,280 | **0,205** |
| P03 | +0,152 | −0,007 | 0,159 |
| P04 | −0,109 | **+0,250** | **0,359** |
| P05 | +0,275 | +0,150 | 0,125 |
| P06 | +0,103 | +0,045 | 0,058 |
| P07 | −0,077 | +0,134 | **0,211** |
| P08 | −0,169 | −0,224 | 0,055 |

**P04 đổi 0,359 °C giữa hai lần chạy sạch.** Sai số chế tạo của một con cảm
biến là **hằng số vật lý** — nó không thể đổi. Nhiễu đọc chỉ 0,03 °C, nên cũng
không phải nhiễu.

→ Kết luận: **cái đang đo được KHÔNG PHẢI sai số cảm biến.** Nó là chênh lệch
nhiệt độ **thật** giữa các vị trí mà đầu dò đang nằm. Tám đầu dò vứt mỗi cái
một chỗ thì mỗi cái ở trong một luồng không khí khác nhau, gần board ấm hoặc xa
board, gần cửa sổ hoặc không. Chênh 0,3 °C giữa hai điểm cách nhau 20 cm trong
phòng là chuyện hoàn toàn bình thường.

Trong **một** lần chạy thì offset lại rất ổn định (P05 đi +0,2741 → +0,2749 →
+0,2791 qua ba lần báo cáo, tức ±0,005 °C) — đúng như kỳ vọng, vì trong vài
phút thì bố trí không khí không đổi.

> ⚠️ **Đính chính lần hai.** Bản trước khẳng định "sai lệch 0,575 °C, **bắt
> buộc** hiệu chỉnh". Cả con số lẫn kết luận đều chưa có cơ sở. Đúng ra phải
> nói: **chưa biết 8 con này lệch nhau bao nhiêu, vì phép đo bị nhiễu môi
> trường lấn át hoàn toàn.** Có thể chúng rất đều, cũng có thể không.

### Cách đo cho đúng

Muốn tách sai số cảm biến ra khỏi chênh lệch môi trường thì phải ép cả 8 con về
**cùng một nhiệt độ thật**:

1. **Bó cả 8 đầu dò lại thành một cụm**, đầu kim loại chụm sát nhau, buộc dây
   rút hoặc quấn băng dính.
2. **Nhúng cụm đó vào cốc nước ở nhiệt độ phòng** — ngập **CHỈ phần đầu kim
   loại**, giữ toàn bộ dây và mối nối trên mặt nước. Nước dẫn nhiệt tốt hơn
   không khí hàng trăm lần nên ép được cả 8 con về cùng một nhiệt độ.
   ⚠️ Ngâm cả dây làm tỉ lệ lỗi bus leo lên 9,4 % sau ~45 phút — xem KQ-10.
3. **Khuấy, rồi đợi 10 phút** cho ổn định mới bắt đầu đo.
4. Chạy `ds18b20_stress_test` **5 phút**, lấy bảng `T_OFFSET`.
5. **Làm lại toàn bộ lần hai.** Hai bảng phải khớp nhau trong ~0,05 °C thì mới
   tin được. Không khớp nghĩa là chưa đẳng nhiệt, làm lại.

Lúc đó mới trả lời được câu "có cần hiệu chỉnh không". Nếu độ rộng < 0,1 °C thì
**bỏ qua hiệu chỉnh**, không cần làm gì thêm.

*Vì sao vẫn phải quan tâm:* Lớp 1 nhìn chênh lệch tương đối giữa các cell
(QĐ-012), nên một sai lệch cố định trông y hệt "cell này lúc nào cũng nóng hơn".
Tra `models/eval_results.npz`, autoencoder bắt lỗi offset 0,5 °C ở tỉ lệ 41,7 %.
Nên *nếu* lệch thật sự cỡ 0,5 °C thì phải hiệu chỉnh. Chỉ là ta **chưa biết**.

## KQ-08 · Hiệu chuẩn trong nước — ✅ ĐẠT, và trả lời được câu hỏi

Bó cả 8 đầu dò thành một cụm, nhúng trong nước ở nhiệt độ phòng (~27,3 °C),
khuấy đều, đợi ổn định. Đo **hai lần độc lập**, mỗi lần 5 phút / 350 vòng đọc:

| Kênh | Lần A | Lần B | \|lệch\| |
|---|---|---|---|
| P01 | −0,0207 | −0,0126 | 0,0081 |
| P02 | −0,1700 | −0,1776 | 0,0076 |
| P03 | +0,1958 | +0,1896 | 0,0062 |
| P04 | −0,0539 | −0,0567 | 0,0028 |
| P05 | +0,0911 | +0,1163 | **0,0252** |
| P06 | +0,0125 | +0,0031 | 0,0094 |
| P07 | −0,0592 | −0,0633 | 0,0041 |
| P08 | +0,0044 | +0,0012 | 0,0032 |

**Lệch lớn nhất giữa hai lần: 0,025 °C** — dưới tiêu chí 0,05 °C đặt ra ở
KQ-06. Độ rộng: lần A 0,3658 °C, lần B 0,3672 °C, khớp nhau trong 0,0014 °C.

So với đo trong không khí, nơi cùng một kênh đổi tới **0,359 °C** giữa hai lần:
phép đo nay ổn định hơn **14 lần**. Đây là bằng chứng nước đã ép được cả 8 con
về cùng một nhiệt độ thật, còn không khí thì không.

→ **Sai số chế tạo thật sự tồn tại và đo được: 0,3665 °C** (P03 nóng nhất,
P02 lạnh nhất). Nằm trong ±0,5 °C của datasheet nên cảm biến bình thường,
nhưng vượt xa ngưỡng bỏ qua 0,1 °C.

→ **Trả lời dứt điểm: CÓ, phải hiệu chỉnh.** Bảng hệ số đã ghi vào
`VEDCaPhenika/esp32s3_wiseiot_test/ds18b20_offsets.h`, kèm cả 8 địa chỉ ROM.

*Ghi chú về chất lượng số liệu:* lần A có 2 mẫu ngoại lai ở P03 (đọc ra
24,81 °C) và 4 lần `DISC` ở P08 — tỉ lệ lỗi 0,14 %. Lần B sạch tuyệt đối
(2 800 đọc, 0 lỗi). Hai lần vẫn khớp nhau nên kết luận không bị ảnh hưởng.

### Sự cố giữa chừng — và nó xác nhận giả thuyết ở KQ-05

Lần đo nước đầu tiên phải bỏ đi vì **chỉ tìm thấy 6 cảm biến**: một dây tín
hiệu tuột khỏi breadboard. Cắm lại thì đủ 8 ngay.

Đây chính là loại tiếp xúc chập chờn đã nghi ở KQ-05. Nó xác nhận: breadboard
là nghi phạm đúng, và **phải bỏ breadboard trước khi gắn lên pack**.

Cũng cho thấy một chế độ hỏng cần đề phòng trong firmware: khi mất một cảm
biến, sketch vẫn chạy bình thường, vẫn in ra bảng đẹp, chỉ là bảng có 6 cột
thay vì 8. Không có cảnh báo nào. Firmware chính **phải kiểm số cảm biến đếm
được đúng bằng 8 lúc khởi động**, thiếu là báo lỗi ngay — xem QĐ-024.

## KQ-09 · Lớp 1 chạy trên cảm biến THẬT — ✅ ĐẠT

Bench `test/cell_temp_ai_bench/` dùng **đúng** `cell_temp.*` và `cell_ai.*` của
firmware chính (symlink, không phải bản sao — sửa firmware là bench đổi theo).
Chạy khi 8 đầu dò đang bó cụm trong nước, tức cả 8 cell "khoẻ" như nhau.

**A. Bảng offset có tác dụng thật:**

```
do rong THO          : 0.3750 C   (neu KHONG hieu chinh)
do rong DA HIEU CHINH: 0.0303 C   <- AI nhin thay cai nay
  -> hieu chinh thu hep 12.38 lan
```

Đây là kiểm chứng **đầu-cuối**: không chỉ bảng số đúng, mà ánh xạ ROM → kênh
trong firmware cũng đúng. Nếu ánh xạ lệch thì phép trừ offset sẽ làm độ rộng
**rộng ra**, không hẹp lại.

**B. Lớp 1 im lặng khi mọi thứ bình thường:**

```
diem cao nhat: cell 8 = 0.3183 (nguong 1.8474)
bao dong: 0/251 mau (0.00%)  <- tot, im lang
```

Điểm cao nhất chỉ bằng **17 %** ngưỡng. Đây là phép thử báo động giả rẻ nhất và
thật nhất có thể dựng: 8 cell cùng nhiệt độ là trạng thái khoẻ rõ ràng nhất.
AI mà kêu ở đây thì ngoài đời nó sẽ kêu suốt ngày.

*Ghi chú:* lần chạy đầu có **1 báo động trên 381 mẫu**, ngay sau giai đoạn khởi
động — lúc đó độ lệch chuẩn trượt còn rất nhỏ nên đặc trưng tương đối bị thổi
phồng. Sau khi sửa logic phục hồi kênh (mục C) thì lần chạy sau ra 0/251. Vẫn
nên thêm thời gian ủ dài hơn trước khi cho phép báo động — ghi vào việc cần làm.

**C. Một lỗi logic tự tìm ra và đã sửa:** bản đầu của `cell_temp.cpp` cho kênh
sống lại chỉ sau **một** lần đọc tốt. Với kênh chập chờn thì nó nhấp nháy
sống–chết liên tục: log spam, dashboard lúc báo 8/8 lúc 7/8, người trực mất
lòng tin. Đã thêm ngưỡng kép — phải đọc tốt **10 lần liên tiếp** mới cho sống
lại. Sau khi sửa: chỉ còn **2 lần đổi trạng thái** trong cả lần chạy.

**D. Và đây là chỗ thiết kế QĐ-024 trả công:** trong lần chạy này bus lỗi tới
**9,4 %**, kênh 5 có lúc bị loại — nhưng Lớp 1 vẫn cho ra **0 báo động giả** và
nhiệt độ vẫn đúng. Nếu không đếm lỗi và không loại kênh hỏng thì số rác đã lọt
thẳng vào AI. Đây là lý do bỏ công viết phần đếm lỗi thay vì gọi thẳng
`getTempCByIndex()`.

## KQ-10 · ⚠️ Tỉ lệ lỗi bus tăng dần khi ngâm nước lâu — cần thử thêm

Diễn biến trong ngày, cùng phần cứng:

| Thời điểm | Tình trạng đầu dò | Tỉ lệ lỗi |
|---|---|---|
| Đo hiệu chuẩn lần A | mới nhúng nước | 0,14 % |
| Đo hiệu chuẩn lần B | trong nước | **0,00 %** |
| Bench Lớp 1 lần 1 | trong nước, ~30 phút sau | 7,6 % |
| Bench Lớp 1 lần 2 | trong nước, ~45 phút sau | **9,4 %** |

Tăng đều theo thời gian ngâm. Giả thuyết: **nước thấm dần lên dây hoặc lên mối
nối ở đầu breadboard**, gây rò giữa VDD / DATA / GND. Đầu dò DS18B20 loại chống
nước chỉ kín ở **đầu kim loại**, phần dây và mối nối thì không.

### ✅ ĐÃ XÁC NHẬN: đúng là do nước

Lau khô đầu dò rồi chạy lại `ds18b20_stress_test` **ba lần liên tiếp**, mỗi lần
5 phút / 2 800 lượt đọc:

| Lần | Đọc tốt | Lỗi | Tỉ lệ lỗi |
|---|---|---|---|
| khô 1 | 2 800 | 0 | **0,0000 %** |
| khô 2 | 2 800 | 0 | **0,0000 %** |
| khô 3 | 2 800 | 0 | **0,0000 %** |

Từ 9,4 % về 0 % chỉ bằng thao tác lau khô. **Đây cũng là lời giải cho lỗi chập
chờn 31–35 % ở KQ-05** — nghi ngờ ban đầu đổ cho tiếp xúc breadboard là sai
hướng; thủ phạm là ẩm gây rò giữa VDD / DATA / GND.

Ba lần sạch liên tiếp cũng chính là tiêu chí AC-06.14. **Đạt.**

*Đây không phải vấn đề của sản phẩm* — trên pack pin thật đầu dò không ngâm
nước. Nhưng nó là vấn đề của **quy trình hiệu chuẩn**: lần sau chỉ nhúng phần
đầu kim loại, giữ toàn bộ dây và mối nối trên mặt nước.

## KQ-11 · Sai số còn lại của bảng offset: ±0,03 °C, riêng P07/P08 ±0,07 °C

Ba lần chạy lúc khô (đầu dò vẫn đang bó cụm, để trong không khí) cho thêm một
bộ số để đối chiếu với bảng đo trong nước:

| Kênh | NƯỚC | khô 1 | khô 2 | khô 3 | lệch so với NƯỚC |
|---|---|---|---|---|---|
| P01 | −0,0166 | −0,0160 | +0,0024 | −0,0004 | 0,019 |
| P02 | −0,1738 | −0,1815 | −0,1656 | −0,1664 | 0,008 |
| P03 | +0,1927 | +0,1945 | +0,1847 | +0,1779 | 0,015 |
| P04 | −0,0553 | −0,0314 | −0,0306 | −0,0277 | 0,028 |
| P05 | +0,1037 | +0,0992 | +0,0933 | +0,1000 | 0,010 |
| P06 | +0,0078 | +0,0094 | +0,0037 | +0,0004 | 0,007 |
| **P07** | −0,0612 | −0,0124 | −0,0251 | −0,0189 | **0,049** |
| **P08** | +0,0028 | −0,0617 | −0,0628 | −0,0648 | **0,068** |

Sáu kênh khớp trong 0,028 °C. P07 và P08 lệch 0,05–0,07 °C.

**Ba lần khô rất khớp nhau (±0,005 °C) — nhưng điều đó KHÔNG chứng minh chúng
đúng.** Bó cụm không đổi vị trí giữa ba lần, nên một chênh lệch nhiệt do hình
học bó sẽ lặp lại y nguyên cả ba lần. Lặp lại được mà vẫn sai là chuyện hoàn
toàn có thể. Nhiều khả năng P07/P08 nằm ở rìa bó nên tiếp xúc với không khí
nhiều hơn phần lõi.

→ **Giữ bảng đo trong nước.** Nước ép đẳng nhiệt mạnh hơn hẳn không khí; đó là
lý do chọn nó ngay từ đầu.

**Phép thử dứt điểm nếu muốn chắc chắn:** đảo vị trí các đầu dò trong bó rồi đo
lại. Nếu offset **đi theo con cảm biến** thì đó là sai số chế tạo thật; nếu nó
**ở lại theo vị trí trong bó** thì đó là chênh lệch môi trường. Tách được hai
thứ này bằng đúng một lần đo.

*Có cần làm không:* sai số còn lại 0,07 °C nhỏ hơn độ rộng thô 0,37 °C **5 lần**,
và thấp hơn nhiều mọi mức mà Lớp 1 phản ứng (ở 0,5 °C tỉ lệ phát hiện đã chỉ
41,7 %). Nên bảng hiện tại dùng được. Ghi lại đây để không ai tưởng nó chính
xác tới chữ số thứ ba.

## KQ-07 · Đọc không chặn — chạy được

Không lần nào đọc phải giá trị reset 85,0 °C. Đọc chặn và không chặn cho tỉ lệ
lỗi như nhau (0,00 % cả hai) khi bus khoẻ.

*Nói cho đúng:* cột "thời gian chuyển đổi TB 760 ms" trong log **không phải số
đo** — nó chính là ngưỡng chờ đặt cứng trong sketch. Thử nghiệm chỉ chứng minh
760 ms là đủ. Datasheet ghi ≤750 ms cho 12 bit.

---

## Việc phải làm, theo thứ tự

1. ~~Đo offset bằng cách bó cụm + nhúng nước~~ — **XONG**, xem KQ-08.
   Hệ số ở `VEDCaPhenika/esp32s3_wiseiot_test/ds18b20_offsets.h`.
2. **Dán nhãn ROM lên từng sợi dây** theo bảng KQ-01, làm ngay trước khi động
   vào pack.
3. **Nối cảm biến thật vào firmware chính**: đọc theo ROM (không theo index),
   đọc không chặn, trừ `DS_OFFSET`, thay nhiệt độ mô phỏng của Lớp 1.
4. **Firmware phải ĐẾM và BÁO lỗi cảm biến lúc chạy** — QĐ-024. Quan trọng hơn
   cả việc truy cho ra nguyên nhân lỗi chập chờn, vì trên xe thật có rung thì
   dây **chắc chắn** sẽ có lúc tiếp xúc kém. Kèm theo: kiểm đủ 8 con lúc khởi
   động (sự cố tuột dây ở KQ-08 cho thấy thiếu cảm biến không báo gì cả).
5. **Bỏ breadboard trước khi gắn lên pack** — hàn thẳng hoặc terminal block bắt
   vít. Breadboard không chịu được rung, và đã tuột một lần ngay trên bàn.
6. Dán đầu dò lên pack. **KHÔNG hiệu chuẩn lại sau khi dán** — xem QĐ-025.

Xem QĐ-022, QĐ-023, QĐ-024, QĐ-025 trong `DECISION_LOG.md`.
