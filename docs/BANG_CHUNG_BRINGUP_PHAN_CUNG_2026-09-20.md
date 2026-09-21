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

**Trị số điện trở thật:** 11,43 V / 0,6215 A = **18,4 Ω**, không phải 20 Ω.
Trong dung sai thì hơi rộng, nên nhiều khả năng là điện trở 18 Ω. Không nguy
hiểm, nhưng T8 dùng con số này để gán nhãn dataset nên phải ghi lại.

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
