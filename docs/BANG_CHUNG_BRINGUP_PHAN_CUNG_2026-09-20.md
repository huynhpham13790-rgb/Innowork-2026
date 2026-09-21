# Bằng chứng bring-up phần cứng đợt 3 — 20/09/2026

Phần cứng mới lắp: INA226 (shunt onboard R100 = 100 mΩ), 2 module D4184,
điện trở sưởi 20 Ω / 12 V, còi SFM-27.

Sketch: `test/hw_bringup_6s/` — menu serial, gõ 1..9. Xem đầu file để biết
sơ đồ chân và hai quy tắc an toàn.

## Cách nạp và chạy

Máy này **không có `arduino-cli` trên PATH**; bản đi kèm nằm trong AppImage
của Arduino IDE (`--appimage-extract`, đường dẫn
`squashfs-root/resources/app/lib/backend/resources/arduino-cli`).

```
FQBN="esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=default,PSRAM=opi,FlashSize=16M"
arduino-cli compile -b "$FQBN" test/hw_bringup_6s
arduino-cli upload  -b "$FQBN" -p /dev/ttyACM0 test/hw_bringup_6s
```

⚠️ **Tổ hợp tuỳ chọn USB là chỗ mất thì giờ nhất, ghi lại để khỏi mò lại.**
Board đang cắm bằng cổng USB-Serial-JTAG (`/dev/ttyACM0`, không có
`/dev/ttyUSB0`). Thử ba tổ hợp:

| USBMode | CDCOnBoot | Serial ra ttyACM0? |
|---|---|---|
| hwcdc | cdc | **không** — nạp được, chạy được, nhưng câm tịt |
| default (TinyUSB) | cdc | **không** |
| hwcdc | default | **có** |

Hai tổ hợp đầu vẫn nạp thành công và chương trình vẫn chạy, chỉ là không có
một chữ nào ra cổng serial — đúng kiểu hỏng âm thầm dễ bị quy oan cho sketch.
Khi đọc bằng pyserial phải đặt `dtr=True, rts=False` lúc mở cổng; nhảy
DTR/RTS sai thứ tự sẽ đẩy chip vào chế độ download (`waiting for download`).

---

## T1 — Quét I2C ✅ ĐẠT

```
===== T1 — Quét bus I2C (SDA=8, SCL=9) =====
  thấy thiết bị ở 0x40
  ✓ INA226 MFR=0x5449 DIE=0x2260 CAL=1678 (CURRENT_LSB=0.0305 mA)
  KẾT LUẬN: 1 thiết bị, INA226 ở 0x40 = CÓ
```

Không chỉ có ACK ở 0x40 — đã đọc đúng hai mã định danh (`MFR=0x5449` = "TI",
`DIE=0x2260`). Rất nhiều chip khác cũng nằm ở 0x40, nên chỉ thấy địa chỉ thì
chưa chứng minh được gì.

CAL = 1678 tính từ CURRENT_LSB = 1,0 A / 32768 = 30,5175 µA và R = 0,1 Ω.
Toàn thang phần cứng là ±81,92 mV / 0,1 Ω = **±0,819 A** — trên mức đó thanh
ghi bão hoà, số đọc sẽ sai mà trông vẫn hợp lý.

## T6 — Địa chỉ ROM DS18B20 ✅ ĐẠT, kèm một phát hiện

```
  Tìm thấy 8 cảm biến trên GPIO4
  [0] { 0x28, 0x30, 0xF1, 0x01, 0x00, 0x00, 0x00, 0x17 },   // 26.06 °C
  [1] { 0x28, 0xB8, 0xC8, 0x01, 0x00, 0x00, 0x00, 0x2B },   // 25.81 °C
  [2] { 0x28, 0xFA, 0x81, 0x02, 0x00, 0x00, 0x00, 0xA2 },   // 26.56 °C
  [3] { 0x28, 0x46, 0xAF, 0x01, 0x00, 0x00, 0x00, 0x0A },   // 26.06 °C
  [4] { 0x28, 0xEE, 0x07, 0x03, 0x00, 0x00, 0x00, 0xFD },   // 26.25 °C
  [5] { 0x28, 0xD1, 0xF8, 0x03, 0x00, 0x00, 0x00, 0xFD },   // 26.12 °C
  [6] { 0x28, 0x4D, 0x49, 0x04, 0x00, 0x00, 0x00, 0x35 },   // 26.38 °C
  [7] { 0x28, 0x7F, 0xCD, 0x01, 0x00, 0x00, 0x00, 0xE3 },   // 26.38 °C
```

8 mã ROM **trùng khít và đúng thứ tự** với bảng `DS_ROM[]` trong
`ds18b20_offsets.h` (P01..P08). Không phải chép lại gì. Độ tản 0,75 °C giữa
con nóng nhất và lạnh nhất là chênh nhiệt thật của 8 đầu dò đang nằm rời
trong không khí, không phải sai số — sai số chế tạo đã đo được là 0,326 °C và
chỉ đo được khi nhúng nước (QĐ-025).

**Phát hiện:** cảm biến môi trường `28 73 4C 04 00 00 00 19`
(`DS_ROM_AMBIENT` trong `ds18b20_offsets.h`) **KHÔNG có trên bus**. Firmware
chính coi nó là tuỳ chọn (`ambientOk()` trả về false, `ambient()` trả NAN),
nên không chết — nhưng đặc trưng "so cell với môi trường" của Lớp 1 sẽ mất
đầu vào thật. Cần cắm lại con thứ 9 trước khi lấy dữ liệu huấn luyện.

---

## TD — Quét 3 trạng thái gate ⭐ test này không có trong phiếu gốc, và nó là test tìm ra lỗi

Thêm vào sau khi T2 trượt. Phiếu gốc đo gate LOW ở T2 và gate HIGH ở T3, hai
lần chạy khác nhau — nếu nguồn chập chờn thì hai số không so được với nhau.
TD đo LOW / HIGH / thả nổi liên tiếp, 20 mẫu mỗi trạng thái, cùng một nguồn.

### Lần 1 — TRƯỢT

```
  (digitalRead lúc LOW = 1)
  gate LOW (phải tắt) I=  619.0 mA   bus=11.371 V   (20 mẫu)
  gate HIGH (phải bật) I=  618.9 mA   bus=11.367 V   (20 mẫu)
  gate THẢ NỔI       I=  619.1 mA   bus=11.359 V   (20 mẫu)
  mức trên GPIO10: thả nổi=1  kéo-xuống-nội=1  kéo-lên-nội=1
```

Ba trạng thái lệch nhau 0,2 mA — gate không có tác dụng gì.

**Nguyên nhân:** có dây 3,3 V cắm vào cọc trigger của D4184 #1. Bằng chứng
quyết định là dòng cuối: bật điện trở kéo xuống **bên trong** ESP32 mà GPIO10
**vẫn đọc mức cao** ⇒ có nguồn ngoài ghim cứng chân đó. Gate luôn cao ⇒ MOSFET
dẫn vĩnh viễn, bất kể firmware ghi gì.

Nghiêm trọng hơn cả việc sưởi không tắt: mỗi lần firmware ghi `LOW`, ESP32
đang kéo xuống đất một chân đang bị 3,3 V ghim lên — ngắn mạch tại chân ra.

⚠️ **Phải đọc mức chân ở chế độ INPUT.** Khi `pinMode(OUTPUT)`, arduino-esp32
tắt bộ đệm vào, `digitalRead` lúc đó trả về giá trị không tin được. Dòng
`(digitalRead lúc LOW = 1)` ở đầu log là đọc trong chế độ OUTPUT nên **không
dùng làm bằng chứng được** — nó chỉ tình cờ trùng kết luận.

**Cách sửa:** cọc trigger D4184 **không cần nguồn riêng**, chân tín hiệu chính
là thứ nuôi gate. Bỏ hẳn dây 3,3 V; chỉ còn `PWM/TRIG → GPIO10` và
`GND → GND ESP32`, với GND ESP32 nối chung cực âm 12 V ở `VIN−`.

### Lần 2 — sau khi rút dây 3,3 V: ĐẠT ✅

```
  gate LOW (phải tắt) I=    0.0 mA   bus=12.470 V   (20 mẫu)
  gate HIGH (phải bật) I=  621.4 mA   bus=11.430 V   (20 mẫu)
  gate THẢ NỔI       I=    0.0 mA   bus=12.467 V   (20 mẫu)
  ✓ Module đóng cắt ĐÚNG, logic thuận.
```

Trạng thái **thả nổi = 0,0 mA** là kết quả đáng giá riêng: module có điện trở
kéo xuống thật, nên vài trăm ms chân ESP32 thả nổi lúc reset sẽ **không** làm
sưởi tự bật.

## T2 — INA226 lúc nghỉ ✅ ĐẠT

```
  bus=12.467 V  shunt=0.002 mV  I=0.0 mA (reg 0.0 mA)  P=0.000 W
  ✓ bus ≈ 12 V, đúng kỳ vọng.
  ✓ dòng lúc nghỉ ≈ 0 mA.
```

## T3 — Burst sưởi ✅ ĐẠT

3 chu kỳ ON 3 s / OFF 7 s:

| chu kỳ | I đỉnh | P đỉnh | E |
|---|---|---|---|
| 1 | 621,9 mA | 7,11 W | 20,78 J |
| 2 | 621,8 mA | 7,11 W | 20,79 J |
| 3 | 622,0 mA | 7,11 W | 20,79 J |

Không có chế độ hỏng nào trong bảng chẩn đoán xảy ra: dòng dương (IN+/IN−
không đảo), ~620 mA nên chỉ **một** điện trở nối tiếp chứ không phải hai con
song song (~1,2 A), và OFF về đúng 0,0 mA.

Dòng tính từ điện áp shunt khớp thanh ghi `CURRENT` tới 0,1 mA — hai đường
tính độc lập cho cùng kết quả, nên thanh ghi `CAL` đã nạp đúng.

**Trị số điện trở thật:** 11,43 V / 0,6215 A = **18,4 Ω**.

⚠️ Đây là **HAI điện trở sứ 10 Ω / 10 W nối tiếp**, không phải một con 20 Ω
(xác nhận 21/09). Bảng chẩn đoán của T3 ở trên chỉ phân biệt "một con 20 Ω" với
"hai con 20 Ω **song song**" (10 Ω, ~1,2 A) — **thiếu mất khả năng hai con 10 Ω
nối tiếp**, cũng cho ra 20 Ω và ~600 mA. Số đo luôn đúng; chỗ thiếu nằm ở bảng
giải thích.

Cách đấu này **tốt hơn** một con 20 Ω: 7,5 W chia đôi, mỗi con **3,75 W trên
định mức 10 W** = 37 %, dư biên nhiệt thoải mái. Nối tiếp nên cùng dòng, hai
con nóng bằng nhau — đo được P03 +13,56 °C và P07 +12,81 °C sau 97 s.

## T8 — Tích phân năng lượng ✅ ĐẠT (sau khi sửa một lỗi của chính phép đo)

**Bản đầu cho kết quả sai 12 %, luôn lệch về một phía.** 3 cửa sổ 5 s ra
32,32 / 32,37 / 32,39 J, tức P trung bình 6,22 W — trong khi T3 đo tức thời
7,10 W. Lặp lại rất tốt nhưng *sai*, đúng kiểu số liệu dễ tin nhầm nhất.

Nguyên nhân: vòng tích phân gọi `dsReadAll()` (chặn ~750 ms) nên bước tích
phân thành 750 ms, lại dùng giá trị **đầu** khoảng (tổng bậc thang trái) —
khoảng đầu tiên rơi đúng lúc sưởi vừa bật, INA226 còn đang trung bình hoá
trạng thái cũ, nên gần như cả 750 ms đầu bị tính thành 0 W.

Sai số một chiều là loại tệ nhất cho một nhãn dataset: lấy trung bình nhiều
mẻ cũng không triệt tiêu, Lớp 2 sẽ học đúng cái lệch đó.

Đã sửa: lấy mẫu INA 20 ms/lần độc lập với nhịp đọc DS18B20, và dùng **quy tắc
hình thang** thay cho bậc thang. Kết quả sau khi sửa:

```
  win	bat_dau_ms	ket_thuc_ms	J	P_tb_W
  1	14141	19158	35.11	6.998
  2	24688	29704	35.10	6.997
  3	35234	40250	35.11	6.999
```

35,11 / 35,10 / 35,11 J — tản 0,03 %, và 6,998 W khớp 7,10 W tức thời (phần
chênh còn lại là sườn lên thật của 16 mẫu trung bình lúc đóng mạch).

## Hệ an toàn tự chứng minh — hai lần, ngoài ý muốn

Trong lúc chạy T8, nhánh "mất cảm biến" của `safetyCheck()` **tự kích hoạt hai
lần**: cắt sưởi, chốt trạng thái, còi kêu liên tục, đúng thiết kế.

Cả hai lần đều là báo oan, và cả hai đều do cùng một thiếu sót trong **cấu trúc
test**, không phải trong hệ an toàn:

1. Đồng hồ `SAFE_STALE_MS` vẫn chạy trong lúc board ngồi không ở menu — mà lúc
   đó không ai đọc cảm biến. Test nào khởi động sau 5 giây ngồi không cũng bị
   cắt ngay dòng đầu.
2. Khoảng nghỉ giữa hai cửa sổ T8 dùng `delay(5000)` trần — đúng bằng ngưỡng.

Đã sửa: nạp lại mốc thời gian ngay trước mọi vòng lặp có sưởi, và thay
`delay()` trần bằng vòng vừa nghỉ vừa đọc cảm biến.

Bài học đáng giữ: **hệ an toàn phải được nuôi liên tục, kể cả lúc đang không
làm gì.** Đây cũng chính là lý do firmware thật không được có `delay()` dài
trong vòng lặp chính.

## T4 — Còi ✅ ĐẠT

Chuỗi 3 tiếng ngắn 100 ms + 1 tiếng dài 2 s. **Xác nhận bằng tai người**
(Phạm Văn Huynh, 20/09) — đây là test duy nhất trong phiếu mà máy không tự
chấm được.

SFM-27 là còi **chủ động**: cấp điện là kêu, tần số cố định trong thân còi.
Không dùng `tone()` — vừa không đổi được cao độ, vừa chiếm một bộ định thời.

## T5 — Nhiễu còi lên bus 1-Wire ✅ ĐẠT

```
  KHÔNG còi: 166 lượt × 7 kênh = 1162 phép đọc, 0 lỗi (0.00 %)
  CÓ còi   : 166 lượt × 7 kênh = 1162 phép đọc, 0 lỗi (0.00 %)
  ✓ Còi không làm hỏng bus 1-Wire. Không cần chống nhiễu thêm.
```

Chạy lại hai lần, cả hai lần đều 0/1162. **Không cần** thêm tụ 100 µF + 100 nF
ở rail 3V3, cũng không cần tách thời gian còi/chuyển-đổi.

Ghi chú về tính hợp lệ: hai lượt này chạy trên "đường nhanh" của
DallasTemperature (xem mục dưới). Điều đó **không làm hỏng kết luận** — thứ
đếm ở đây là lỗi CRC trên bus, không phải giá trị nhiệt độ, và đường nhanh cho
nhiều giao dịch 1-Wire hơn trong cùng 60 s nên còn nhạy hơn.

## T7 — Ngưỡng cắt an toàn ✅ ĐẠT

```
*** CẮT AN TOÀN *** vượt ngưỡng nhiệt (t_max=32.50 °C, ngưỡng 31.0)
  ✓ ĐẠT: đã cắt, đã chốt, còi kêu.
  t_max=32.50 °C  sưởi=off
```

Dòng ngay sau lệnh cắt đã là `sưởi=off` — cắt trong đúng lượt đọc phát hiện
ra, không trễ lượt nào.

⚠️ **Ngưỡng test phải là 31 °C, không phải 35 °C như phiếu gốc ghi.** Lần chạy
đầu với 35 °C trượt sau 120 s: nắm tay vào đầu dò chỉ đưa được lên **32,31 °C**
rồi bắt đầu nguội — vỏ kim loại đầu dò không bao giờ đạt thân nhiệt lõi. 35 °C
là ngưỡng không thể với tới, nên phép test đó không nói lên điều gì về hệ an
toàn. 31 °C nằm giữa: cao hơn con nóng nhất lúc nằm không (28,4 °C) đủ để
không tự kích, thấp hơn 32,31 °C đủ để tay người với tới.

**Cả hai nhánh của hệ an toàn giờ đã chứng minh trên phần cứng thật:** nhánh
vượt nhiệt ở đây, nhánh mất cảm biến ở mục trên.

Ngưỡng đã tự trả về 60,0 °C ở cuối hàm — không để người phải nhớ.

## T9 — Chạy gộp 5 phút ✅ ĐẠT

DS18B20 + INA226 + sưởi (10 s bật / 20 s nghỉ) + còi bíp, chạy đồng thời 300 s.

```
  KẾT THÚC: 356 vòng, vòng chậm nhất 901 ms, heap 340808 -> 340808,
            lỗi DS18B20 2, lỗi I2C 0
```

| Tiêu chí | Kết quả |
|---|---|
| Watchdog reset | không có |
| Rò bộ nhớ | heap **340808 → 340808**, Δ = 0 byte sau 356 vòng |
| Thời gian vòng ổn định | max 901 ms (tiêu chí < 1500 ms) |
| 1-Wire chặn I2C | **0 lỗi I2C** trên 356 lượt đọc |
| Lỗi DS18B20 | 2 lỗi, cả hai trong 34 giây đầu, sau đó 0 suốt 266 s còn lại |

Heap phẳng tuyệt đối là câu trả lời cho câu hỏi quan trọng nhất của T9: 1-Wire
chặn 750–900 ms mỗi vòng **không** làm hỏng giao dịch I2C, và không có rò bộ
nhớ tích luỹ.

---

## Hai phát hiện phụ về DS18B20

### 1. Con P05 rụng khỏi bus

Đầu buổi T6 thấy đủ **8** cảm biến. Các lần khởi động sau chỉ còn **7**. Con
thiếu là `28 EE 07 03 00 00 00 FD` = **P05**.

Đây đúng là chế độ hỏng mà `cell_temp.cpp` được viết ra để bắt (QĐ-024): hệ
vẫn chạy bình thường, vẫn ra số, chỉ là thiếu một cell và không có lỗi nào
báo. Cần kiểm lại mối nối trước khi lấy dữ liệu.

### 2. Đường "hỏi cảm biến xong chưa" của DallasTemperature trả về sau 90 ms

```
  Một lượt đọc 7 kênh mất 90 ms (12 bit cần ≥750 ms)
  Đường chờ đủ 750 ms mất 841 ms; lệch lớn nhất so với đường nhanh: 0.5000 °C
```

90 ms là bất khả thi về vật lý cho chuyển đổi 12 bit, nên số đọc của đường
nhanh có thể là của lượt trước.

**Nhưng con số 0,5 °C KHÔNG phải bằng chứng của lỗi** — kênh lệch nhiều nhất
chính là kênh đang nguội sau đợt sưởi (28,44 → 27,38 °C), còn 6 kênh kia chỉ
lệch đúng một bước lượng tử 0,0625 °C. Phép đo này **không tách được** "đọc số
cũ" khỏi "nhiệt độ thay đổi thật". Ghi lại đúng như vậy, không kết luận quá.

Bench đã chuyển sang `setCheckForConversion(false)` (ép chờ đủ 750 ms) cho hết
nghi ngờ. **Firmware thật không dính chuyện này**: `cell_temp.cpp` không dùng
cơ chế chờ của thư viện mà tự đếm `CT_CONV_MS = 760 ms` bằng máy trạng thái
không chặn, vì nó còn phải gọi `mqtt.loop()` đều đặn.

Độ phân giải đã kiểm bằng cách đọc ngược từ từng con: **12 bit trên cả 7
kênh**, và mọi giá trị đều là bội của 0,0625 °C. Giả thuyết "đang chạy 10 bit"
bị số liệu bác bỏ.

---

## Tổng kết

| Test | Kết quả |
|---|---|
| T1 quét I2C | ✅ |
| T2 INA226 lúc nghỉ | ✅ (sau khi sửa dây trigger) |
| T3 burst sưởi | ✅ |
| T4 còi | ✅ (tai người xác nhận) |
| T5 nhiễu còi lên 1-Wire | ✅ 0/1162, hai lần |
| T6 ROM DS18B20 | ✅ |
| T7 ngưỡng cắt an toàn | ✅ (ngưỡng thử 31 °C) |
| T8 tích phân năng lượng | ✅ (sau khi sửa phép tích phân) |
| T9 chạy gộp 5 phút | ✅ |
| TD quét gate (thêm mới) | ✅ |

---

## Bổ sung 21/09 — `PackMeter` chạy được cả INA226 lẫn INA228

Đội quyết định dùng INA226 trước, INA228 về thì đổi vào. `PackMeter::begin()`
giờ tự nhận chip; giao diện công khai không đổi nên firmware chính không phải
sửa dòng nào. Chi tiết quyết định: **QĐ-037**.

Kiểm trên chip thật (`test/pack_meter_live/`):

```
valid=0  V=  0.270  cell_v= 0.034  I=  0.0 mA  Q=0.00000 Ah  die=NAN  loi=2  bao_hoa=0
```

`die=NAN` chỉ xuất hiện trên đường INA226 (INA228 trả về số đọc từ thanh ghi
`DIETEMP`) ⇒ nhận chip đúng. Bộ đếm lỗi tăng đúng 1 mỗi lượt vì 0,27 V nằm
ngoài `PM_V_MIN` = 15,0 V của pack 8S ⇒ phép loại số vô lý cũng đúng.

⚠️ **Phép này chạy lúc 12 V KHÔNG cắm** (bus 0,27 V thả nổi), nên nó chứng minh
phần nhận chip và phần loại số vô lý, **không** chứng minh phép đo dòng. Phép
đo dòng đã chứng minh riêng ở T2/T3 bằng đường truy cập thanh ghi thô độc lập,
cùng cấu hình `0x4527` và cùng `CAL = 1678`.

Biên dịch lại sạch: firmware chính (85 % flash), `pack_meter_bench`,
`pack_meter_live`, `hw_bringup_6s`. `test/run_test.sh` (spool, chạy trên PC):
toàn bộ PASS.

---

## Bổ sung 21/09 — Gán nhãn 8 đầu dò bằng nước lạnh

Công cụ: `test/ds18b20_identify/`. Nhúng **một** đầu dò vào cốc nước lạnh,
sketch in ra tên con đang tụt nhiệt, dán giấy ghi số lên sợi đó.

**Không dùng ngưỡng tuyệt đối kiểu "dưới 20 °C là đang nhúng"** — ngưỡng đó phụ
thuộc nước lạnh tới đâu và phòng nóng tới đâu, hôm nay đúng mai sai. Dùng
chênh lệch so với **trung vị cả dàn**: mốc tự trôi theo nhiệt độ phòng. Trung
vị chứ không phải trung bình, vì chính kênh đang nhúng sẽ kéo lệch trung bình
làm nó trông đỡ lạnh đi.

### Kết quả — cả 8 sợi, khớp 100 % bảng đã hiệu chuẩn

| Giấy dán | ROM | Trong pack 6S? |
|---|---|---|
| **1** | `28 30 F1 01 00 00 00 17` | ✓ |
| **2** | `28 B8 C8 01 00 00 00 2B` | ✓ |
| **3** | `28 FA 81 02 00 00 00 A2` | ✓ |
| **4** | `28 46 AF 01 00 00 00 0A` | ✓ |
| **5** | `28 EE 07 03 00 00 00 FD` | ✓ |
| **6** | `28 D1 F8 03 00 00 00 FD` | ✓ |
| **7** | `28 4D 49 04 00 00 00 35` | ✗ dự phòng (bản 8S) |
| **8** | `28 7F CD 01 00 00 00 E3` | ✗ dự phòng (bản 8S) |

**Không có ROM lạ** ⇒ không con nào bị thay ⇒ bảng `DS_OFFSET` vẫn đúng,
**không phải hiệu chuẩn lại bằng nước** (QĐ-025).

⚠️ Việc này xác định **"sợi dây ↔ ROM"**, chưa phải **"cell ↔ ROM"**. Lúc dán
lên pack vẫn phải đặt đúng sợi 1 lên cell 1, sợi 2 lên cell 2... Không có phép
đo nào kiểm được điều đó sau khi đã dán.

### Hai cái bẫy gặp thật trong lúc làm

**1. −127 °C cướp mất kết luận.** Đó là mã báo **mất kết nối** của DS18B20,
không phải nhiệt độ. Trung vị ~25 °C nên nó hiện thành lệch −152 °C — luôn
thắng cuộc thi "lạnh nhất". Sketch trên board có chặn, nhưng script đọc log
viết vội thì không, và nó đã **báo sai tên một lần** (nói P05 trong khi thực tế
là P07). Nước đá lạnh nhất cũng chỉ ~−25 °C so với phòng ⇒ ngưỡng −50 tách
được hai thứ.

**2. Sợi vừa nhấc ra vẫn còn lạnh hàng chục giây**, nên có lúc hai ba sợi cùng
lạnh và **sợi lạnh nhất lại là sợi đã làm xong**. Gặp thật: P08 ở −6,31 (đang
lạnh dần) mới là sợi trong nước, còn P07 ở −7,75 → −3,56 (đang ấm lên) là sợi
cũ. Phân biệt bằng **chiều biến thiên**: trong nước thì lạnh dần, vừa nhấc ra
thì ấm dần. Quy tắc này đã đưa vào sketch nên lần sau không cần ai ngồi nhìn.

### ⚠️ Tiếp xúc chập chờn — chẩn đoán cũ SAI, đã sửa lại

Chẩn đoán ban đầu "con P05 hỏng" là **sai**. Dữ liệu bác bỏ nó:

| Thời điểm | P05 | P07 |
|---|---|---|
| 20/09, lần chạy T6 đầu | có | có |
| 20/09, các lần khởi động sau | **mất** | có |
| 21/09, bắt đầu gán nhãn | có (tự về, không ai sửa) | có |
| 21/09, sau khi nhúng P07 | **mất** rồi **tự về** | **mất**, chưa về |

### ✅ NGUYÊN NHÂN THẬT — và cả hai chẩn đoán của AI đều chưa đúng

Người phụ trách tìm ra: **có đầu dò cắm nhầm ray trên breadboard**. Ray nguồn
của nhiều breadboard **bị cắt đôi ở giữa**, hai nửa không thông nhau trừ khi
có dây bắc qua — con nào nằm nửa có điện thì chạy, nửa kia thì câm, và chạm
tay vào là đổi. Khớp với toàn bộ số liệu đã đo.

Ghi lại để không quên: chẩn đoán đầu của AI là **"con P05 hỏng"** — sai, dữ
liệu bác bỏ khi P05 tự sống lại. Chẩn đoán thứ hai là **"xê dịch cơ học"** —
đúng hướng nhưng chưa tới nơi, và nó không giải thích nổi việc P05 tự hồi mà
không ai đụng vào. Cả hai đều là suy đoán từ triệu chứng; thứ giải được là
người nhìn thẳng vào cách cắm dây.

### Đã sửa: hàn chụm ba bó, ra 3 dây jump

Hàn 8 chân DATA vào nhau, 8 chân VDD vào nhau, 8 chân GND vào nhau, mỗi bó ra
một dây jump cắm breadboard. Đây đúng là cách bus 1-Wire được thiết kế để đấu:
open-drain, mọi thiết bị chung một dây, phân biệt bằng mã ROM 64 bit — không
có chuyện tranh chấp điện.

**Kết quả đo sau khi hàn (21/09):**

| | Trước hàn | Sau hàn |
|---|---|---|
| Số cảm biến thấy | 7/8, lúc có lúc không | **8/8, ổn định qua nhiều lần khởi động** |
| Lỗi đọc (T5, 2 lượt) | — | **1 / 2.272 (0,04 %)**, lượt hai 0/1136 |
| Tản nhiệt giữa các kênh | 0,75 °C | **0,19 °C** |
| Nhiễu còi lên bus | 0 lỗi | 0 lỗi |

Lỗi duy nhất rơi vào lượt đầu ngay sau khi hàn, lượt hai sạch hoàn toàn — nhiều
khả năng là chấn động còn sót lại. Tản nhiệt giảm còn 0,19 °C là hệ quả của
việc bó chung một chỗ, không phải cải thiện cảm biến.

8 ROM vẫn khớp bảng, không con nào lạ ⇒ `DS_OFFSET` giữ nguyên.

⚠️ Đổi lại: hàn chụm biến 8 kênh thành **một điểm hỏng chung** — cục hàn lỗi
là mất sạch 8 cell cùng lúc thay vì mất từng con. Với dây ngắn thế này thì
đáng đánh đổi, nhưng phải bọc cách điện riêng từng bó và níu dây ở chỗ dây
jump cứng gặp dây mềm của đầu dò.

---

## Bổ sung 21/09 — Đặc tính nhiệt điện trở sưởi (`test/heater_thermal/`)

Đầu dò P07 quấn lên điện trở sứ. Chi tiết và kết luận: **QĐ-040**.

| | |
|---|---|
| Tốc độ lên | 0,115 °C/s (28,81 → 40,00 °C trong 97,7 s, 720 J) |
| Cắt tại | 40,00 °C, t = 97 s |
| **Đỉnh** | **45,62 °C tại t = 157 s — 60 s SAU khi cắt** |
| **Vọt lố** | **+5,62 °C** |
| Hằng số nguội | τ ≈ 359 s (về trong 1 °C của nền cần ~18 phút) |

Hai điều rút ra, cả hai đều đổi cách nghĩ về hệ an toàn:

1. **Ngưỡng cắt không phải trần nhiệt.** Cắt ở 40 vẫn lên tới 45,62. Chiếu sang
   `AL_T_CRIT = 60 °C` thì đỉnh thật sẽ là ~65,6 °C nếu động học tương tự.
2. **Lần chạy đầu KHÔNG cắt** vì ngưỡng đặt 50 °C mà đầu dò chỉ lên 46,94 °C sau
   113 s — trong khi thân điện trở đã bỏng tay. Thứ dừng thí nghiệm là **bàn tay
   người, không phải chương trình**. Đã thêm hạn mức **không đọc cảm biến** (cắt
   sau 165 s hoặc 1200 J).

⚠️ **Giữa hai lần diễn demo phải chờ tới khi cell vừa sưởi về trong ~1 °C so
với các cell kia** — sờ tay không kiểm được (lúc thấy "đã nguội" thì đầu dò vẫn
còn cao hơn nền 8,9 °C; da người ~33 °C nên 35 °C sờ vào thấy trung tính). Lớp 1
nhìn chênh lệch tương đối, nên cell còn ấm sẽ gây báo giả hoặc làm EMA coi mức
ấm đó là nền. Xem QĐ-040.

⚠️ Cả hai con số trên đo trên **điện trở + đầu dò quấn ngoài**, không phải trên
cell. Phải đo lại sau khi dán đầu dò lên pack rồi mới quyết có hạ `AL_T_CRIT`
hay không.

---

## Bổ sung 21/09 — Kịch bản đầu-cuối ĐÃ CHẠY THÔNG

P03 và P07 cùng quấn lên cụm điện trở sứ (**hai con 10 Ω nối tiếp**). P03 đóng
vai "cell lỗi", P07 canh quá nhiệt. Firmware chính chạy thật: 6 cảm biến →
Lớp 1 → báo động → MQTT → Node-RED → InfluxDB → Grafana.

### Kết quả

```
[ALRM] OK -> THEO DOI -> BAO DONG
[AI  ] *** BAT THUONG *** cell 3, diem 24.697 (nguong 1.071), giu 30s

AI_Score01 0.733   AI_Score02 0.559   AI_Score03 24.697
AI_Score04 0.717   AI_Score05 0.626   AI_Score06 0.771
```

Cell 3 đạt **24,697** so với ngưỡng **1,071** — gấp 23 lần, trong khi năm cell
còn lại nằm trong 0,56–0,77 (chưa tới ngưỡng). Mô hình chỉ đúng cell, không mập
mờ, và leo đúng bậc thang OK → THEO DÕI → BÁO ĐỘNG với quy tắc giữ 30 giây.

Đã xác nhận dữ liệu tới InfluxDB (truy vấn trực tiếp, timestamp khớp). mDNS tự
tìm broker `172.172.3.126:1883` — không cần IP ghim sẵn.

⚠️ **Đây là kiểm thông chuỗi, KHÔNG phải minh chứng "phát hiện cell pin hỏng".**
Đầu dò đang quấn trên điện trở sứ, chưa dán lên cell pin nào. Nói đúng thì đây
là bằng chứng mạnh cho "thuật toán + đường dữ liệu chạy đúng đầu-cuối"; nói quá
thành "phát hiện được pin lỗi" là bị bẻ ngay.

Cell 3 lúc bắt đầu đã ấm sẵn +3,4 °C từ đợt đo trước, nên lần chạy này không
dùng làm ảnh/clip demo được — bản đẹp phải đợi cell 3 nguội về sát các cell kia.

### Vọt lố xuất hiện lại, đúng như đã đo

Giới hạn sưởi đặt +6 °C nhưng cell 3 lên tới **+9,8 °C** — chính là vọt lố
+5,62 °C đo ở QĐ-040. **Cắt không phải là trần.** Thiết kế nhiều lớp làm đúng
việc: giới hạn mềm vọt qua, trần cứng 50 °C không bị đụng tới (đỉnh 36,65 °C).

### ⚠️ Ba lỗi chỉ lộ ra khi chạy trọn chuỗi, không bench nào thấy

1. **`AL_PIN_BUZZER` vẫn là `-1`.** Comment ghi "còi đang đặt mua, chưa về" và
   không ai sửa lại sau khi còi về. Firmware chính báo động mà **loa im**. Còi
   đã nghiệm thu từ T4 (20/09) nhưng chỉ trong sketch bench. Đã nối vào GPIO5.
2. **Còi tự tắt tiếng ngay giây đầu.** `[ALRM] TAT TIENG coi` xuất hiện ở dòng
   log đầu tiên. Nút tắt tiếng là nút BOOT (GPIO0) — chân strapping, sau khi nạp
   hoặc reset có thể còn ở mức thấp trước khi điện trở kéo lên ổn định, và code
   khởi tạo `mute_prev_ = true` nên lần đọc đầu trông y hệt một cú nhấn. Kết
   quả: báo động vẫn leo mức, đèn vẫn đỏ, chỉ tiếng là không bao giờ kêu. Đã sửa
   bằng cách nạp mức thật của chân lúc `begin()` và bỏ qua 300 ms đầu.
3. **`DemoHeater` tự chốt vĩnh viễn lúc khởi động.** Bản đầu kiểm mọi hạn mức ở
   mọi vòng lặp, nên ngay lúc `gCellTemp[]` còn toàn NAN (cảm biến chưa có số
   đọc đầu tiên) nó chốt luôn, dù 6 cảm biến đều khoẻ và chẳng ai xin sưởi. Đã
   sửa: không ai xin và đang tắt thì nằm im, không chốt.

Lỗi 1 và 2 cộng lại đúng bằng "màn demo mất phần gây ấn tượng nhất mà không ai
biết vì sao". Cả ba đều là loại **không có lỗi nào báo**.

---

## Bổ sung 21/09 — Bảng điều khiển demo (`http://127.0.0.1:1880/hutieu`)

Nút bấm để xin bật sưởi và tắt tiếng còi khi diễn. Dùng **node lõi của Node-RED**
(`http in` / `template` / `mqtt out`), **không cài palette nào** — giữ đúng kỷ
luật ghim phiên bản của dự án, và không thêm một thứ có thể hỏng vào ngày thi.

### Nguyên tắc: cloud chỉ được XIN, thiết bị mới QUYẾT

`onMqttMessage()` **không bật sưởi**. Nó chỉ gọi `gHeater.request()`, rồi
`DemoHeater` tự kiểm trần 50 °C, hạn mức thời gian, mất cảm biến — và mới quyết.
**Không lệnh nào từ mạng nâng được các hạn mức đó.** `request()` còn tự kẹp
`hold_ms` xuống tối đa `DH_DEADMAN_MS`, nên kể cả payload xin 1 giờ cũng chỉ
được 15 giây.

Hai tầng chặn độc lập:
- **Node-RED**: danh sách TRẮNG 4 lệnh (`heat`, `heat_stop`, `mute`, `quiet`).
  Danh sách trắng chứ không phải danh sách đen — thêm lệnh mới phải sửa code,
  nên không thể vô tình mở một đường điều khiển chỉ vì đổi payload ở trình duyệt.
- **Firmware**: mọi hạn mức an toàn, không thể can thiệp từ xa.

### Công tắc chết người — đã kiểm

```
xin lần đầu        -> left=15s
gia hạn đều 5s     -> heater=1, delta 1,96 °C, "dang bom nhiet"
ngừng gia hạn 20s  -> heater=0, left=0s, "lenh xin het han"
```

Trang web tự gia hạn mỗi 5 giây; thiết bị cho 15 giây. **Đóng tab, mất mạng,
sập Node-RED → sưởi TẮT**, không kẹt ở trạng thái bật lúc không ai nhìn.

### Giao diện hiện trạng thái THẬT, không hiện thứ vừa bấm

ESP32 trả `ack` sau mỗi lệnh kèm trạng thái thật (`heater`, `delta`, `left_s`,
`reason`, `muted`, `quiet`, `alarm`); trang web hiển thị cái đó. Nút bấm và
trạng thái thiết bị là **hai thứ khác nhau**, và chỉ cái sau là thật.

Kiểm: danh sách trắng chặn `heat_forever` → HTTP 400; lệnh hợp lệ trả lời trong
~2 ms.

### Tắt tiếng và "bíp thưa"

- **Tắt tiếng**: đi qua đúng cờ mà nút BOOT tại chỗ dùng, nên giữ nguyên quy tắc
  cũ — **leo thang mức báo động thì tự huỷ tắt tiếng** (QĐ-028).
- **Bíp thưa**: rút tiếng còn 90 ms mỗi chu kỳ. ⚠️ **KHÔNG phải giảm âm lượng.**
  SFM-27 là còi **chủ động** — mạch dao động nằm trong thân còi, chỉ có hai
  trạng thái có điện/không điện; băm PWM nguồn của nó không làm nhỏ tiếng mà chỉ
  chặt tiếng thành đoạn. Thứ giảm được là mức gây khó chịu, không phải decibel.
- **Bíp thưa KHÔNG áp dụng ở mức NGUY KỊCH.** Ngưỡng cứng 60 °C là lớp bảo vệ
  cuối cùng, nó phải kêu hết cỡ.

### Một lỗi đã mắc và đã sửa

Node lọc lệnh ban đầu tạo một object mới cho nhánh trả lời thay vì dùng lại
`msg`, nên mất `msg.res` và node `http response` không biết trả về cho request
nào. Hậu quả rất dễ chẩn đoán nhầm: **lệnh VẪN tới thiết bị và vẫn thực thi**
(ack chứng minh), chỉ có trình duyệt treo vĩnh viễn. Nhìn từ phía người dùng thì
giống hệt "hệ thống không nhận lệnh".

## Việc còn lại

- [x] ~~Cố định lại chỗ đấu nối cụm DS18B20~~ — đã hàn chụm 3 bó 21/09, bus
      ổn định 8/8, lỗi 0,04 %
- [ ] Cắm lại cảm biến **môi trường** (`28 73 4C 04 00 00 00 19`)
- [x] ~~`pack_meter` viết cho INA228~~ — đã cho tự nhận cả hai chip (QĐ-037)
- [ ] Đo lại điện trở shunt của module INA226 (0,1 Ω là trị số ghi trên nhãn)
- [ ] Ghi lại trị số điện trở sưởi thật (đo được ~18,4 Ω, không phải 20 Ω)
- [x] ~~Sửa firmware 8 cell → 6 cell~~ — đã làm, gom về `PACK_N_CELLS` (QĐ-038)
- [x] ~~Xác định sợi dây nào ứng với ROM nào~~ — xong 21/09 bằng nước lạnh, đã
      dán giấy 1..8
- [ ] **Đo lại vọt lố trên CELL THẬT** sau khi dán đầu dò, rồi quyết có hạ
      `AL_T_CRIT` từ 60 °C xuống không (QĐ-040)
- [ ] Cân nhắc đưa hạn mức "thời gian + năng lượng" vào `alarm.cpp` của firmware
      thật, không chỉ ở bench (QĐ-040)
- [ ] **Khi dán lên pack: đặt đúng sợi 1 lên cell 1, ... sợi 6 lên cell 6.**
      Bảng `DS_ROM[]` giả định thế, và sau khi dán thì không phép đo nào kiểm
      lại được (QĐ-038)

---

## 21/09/2026 — Kênh BLE cho thợ kỹ thuật, và lỗi "cổng USB tự bấm nút tắt tiếng"

Phần cứng: ESP32-S3 + 6× DS18B20 (6/6 khoẻ, tỉ lệ lỗi 0,000 %).
INA226 **không thấy trên I2C** trong cả đợt đo này — adapter 12 V chưa cắm.
Không ảnh hưởng tới điều đang nghiệm thu; BLE báo đúng `chua do duoc (khong
thay INA)` chứ không bịa số.

### Chi phí thêm BLE

| | Flash | RAM tĩnh | Heap lúc chạy |
|---|---|---|---|
| Không BLE (`-DUSE_BLE=0`) | 1 133 707 B (33 %) | 54 524 B | — |
| Có BLE | 1 370 879 B (41 %) | 56 696 B | tốn ~80 KB, còn trống ~122 KB |

### `test/ble_check.py` — cả bốn phép đều đạt

```
thay: 7C:E8:B1:B2:7C:D5
(1) DOC
  Trang thai                : BINH THUONG
  Nhiet do tung cell (degC) : 1:26.5 2:26.3 3:26.5 4:26.5 5:26.2 6:26.5
  Lop 1 - cell nghi ngo     : khong co cell bat thuong (cao nhat 0.14 / 1.07)
  Dien ap / dong / SoC      : chua do duoc (khong thay INA)
  Tinh trang he thong       : cam bien 6/6  wifi OK  cloud OK  chay 1 phut
(2) AN TOAN  ✓ DAT — 5 dac tinh, tat ca chi READ/NOTIFY
(3) NOTIFY   5 goi tu 2 dac tinh trong 15 s
(4) QUANG BA LAI sau khi ngat  ✓ DAT
```

### Lỗi tìm ra: mở Serial Monitor làm còi tắt tiếng

Đo bằng ba cấu hình cổng, mỗi lần 6 giây:

| Cấu hình | Dòng serial | Tắt tiếng bị đảo |
|---|---|---|
| `dtr=True,  rts=False` | 4 | **có** |
| `dtr=False, rts=False` | 25 | không |
| `dtr=False, rts=True` | 0 | không |

Hai kết luận:

- Script đọc serial của đội đang dùng `dtr=True` — **sai**, và làm **mất phần
  lớn dữ liệu** (4 dòng so với 25).
- `dtr=True` ghì GPIO0 (= `AL_PIN_MUTE`) xuống **liên tục suốt phiên**, kèm
  reset board. Ai mở rồi đóng Serial Monitor là còi bị tắt tiếng, không dấu
  hiệu gì ra ngoài.

Diễn biến vá — ghi cả hai lần sai vì cách sai mới là bài học:

| Lần | Cách vá | Kết quả |
|---|---|---|
| 1 | Đòi giữ nút ≥ 600 ms ("xung DTR chỉ vài chục ms") | **TRƯỢT 6/6** — đo lại xung dài 998 ms |
| 2 | Khoá nút 3 s đầu sau khởi động | **TRƯỢT 6/6** — chân bị giữ vĩnh viễn, không phải nhấn lúc boot |
| 3 | Đảo lúc **NHẢ** + phải **thấy nút nhả ít nhất một lần** trước | **ĐẠT 3/3** |

**Phép thử cũng từng sai.** Bản đầu đọc dòng `[ALRM]` trên chính serial và báo
"6/6 đạt" — sai, vì cú đảo xảy ra đúng lúc **đóng cổng**. Bản hiện tại quan sát
qua BLE, là kênh không đụng vào GPIO0:

```
  lan 1: ✓ con bat tieng — BINH THUONG
  lan 2: ✓ con bat tieng — BINH THUONG
  lan 3: ✓ con bat tieng — BINH THUONG
  3/3 dat. DAT
```

### Kiểm nút BOOT thật — và lỗi thứ hai lộ ra ở đây

Ba lần vá trên mới chứng minh *cổng USB không bấm hộ được*. Kiểm vế còn lại —
*người bấm có ăn không* — thì **trượt**: nhấn hai lần, không lần nào vào.

Bật dòng chẩn đoán rồi bắt tại trận (serial `dtr=False`, không đụng GPIO0):

```
[  22.9s] [ALRM] TAT TIENG coi (giu nut 199 ms)
[  23.9s] [ALRM] BAT LAI TIENG coi (giu nut 232 ms)
[  25.6s] [ALRM] TAT TIENG coi (giu nut 254 ms)
[  29.4s] [ALRM] bo qua cu nham 1735 ms (chi nhan 120..1500 ms)
[  30.8s] [ALRM] BAT LAI TIENG coi (giu nut 205 ms)
[  32.1s] [ALRM] TAT TIENG coi (giu nut 241 ms)
[  33.5s] [ALRM] BAT LAI TIENG coi (giu nut 259 ms)
[  34.5s] [ALRM] TAT TIENG coi (giu nut 595 ms)
```

| Kiểu nhấn | Đo được | Ngưỡng cũ 600–5000 ms |
|---|---|---|
| bình thường ×6 | 199–259 ms | **loại sạch** |
| giữ "khoảng 1 giây" | 1735 ms | ăn |
| giữ vừa | 595 ms | **loại** |

Hai nguyên nhân, cả hai là chọn số bằng cảm tính:

1. `pollMute()` chạy ở nhịp 1 Hz nên cú nhấn 200 ms lọt giữa hai lần lấy mẫu →
   tách `Alarm::pollButton()`, gọi **mỗi vòng `loop()`**.
2. Ngưỡng tối thiểu 600 ms **đã giết nút thật ngay từ lần vá đầu** → chốt
   **120–2500 ms**, lấy từ số đo trên.

Sau khi sửa: nút thật ăn **7/7** cú nhấn bình thường; `dtr_mute_check.py` vẫn
**3/3 đạt**.

⚠️ **Tớ đọc nhật ký quá sớm một lần và kết luận nhầm** rằng cú nhấn không tới
được chương trình. File lúc đó chưa kịp ghi. Kết luận đúng chỉ có sau khi đọc
lại — ghi ra đây vì suýt nữa đã đi sửa nhầm hướng (nghi chân GPIO0 hỏng).

### INA226 sau khi cắm adapter 12 V

```
Dien ap / dong / SoC : 12.52 V  0.000 A  SoC ~0%
```

Chip sống lại ngay — module lấy nguồn từ nhánh adapter, không phải từ ESP.
Dòng 0,000 A đúng vì sưởi đang tắt.

Hai điều cần biết về con số này:

- `SoC ~0%` **không phải lỗi**: 12,52 V ÷ 6 = 2,09 V/cell, dưới 3,0 V nên hàm
  kẹp về 0. Đang đo adapter, chưa có pack.
- **`PACK_V_MIN` của 6S là 11,40 V, nên 12,52 V của adapter LỌT QUA** phép kiểm
  "điện áp pack hợp lý". Phép kiểm đó không phân biệt được *chưa lắp pack* với
  *pack gần cạn*. Chưa sửa, nhưng phải biết trước khi tin vào cờ đó.

### Còn nợ ở mục này

- Dời nút tắt tiếng sang chân không dùng chung với DTR, nếu có board rời.
