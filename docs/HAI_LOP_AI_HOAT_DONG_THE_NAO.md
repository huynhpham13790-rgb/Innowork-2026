# Hai lớp AI hoạt động thế nào — các trường hợp, nguyên nhân, việc phải làm

Viết 14/09/2026. Tài liệu này để **giải thích được cho người khác**, không phải
để mô tả code. Ai trong đội cũng phải trả lời được những câu ở đây.

---

# LỚP 1 — Phát hiện cell bất thường (chạy trên ESP32, 1 Hz)

## Nó nhìn cái gì

Không nhìn nhiệt độ tuyệt đối. Nó nhìn **quan hệ giữa một cell với phần còn lại
của pack**, qua 16 con số cho mỗi cell:

| Nhóm | Đặc trưng | Ý nghĩa |
|---|---|---|
| So với pack | `dev_mean`, `dev_median`, `zscore`, `rank` | cell này nóng hơn/lạnh hơn phần còn lại bao nhiêu |
| So với môi trường | `t_minus_amb`, `pack_minus_amb` | pack tự sinh nhiệt hay chỉ đang nóng theo trời |
| Tốc độ | `dT_cell`, `dT_pack`, **`dT_diff`** | cell này nóng lên **nhanh hơn** phần còn lại không |
| Kiểu lệch | `dev_ema`, `dev_shock` | lệch từ từ (lão hoá) hay lệch đột ngột (sự cố) |
| Bối cảnh | `spread`, `t_std`, `current`, `soc`, `t_abs` | pack đang sạc/xả, đang đầy/cạn |

Autoencoder học "quan hệ bình thường trông thế nào" từ dữ liệu pack khoẻ. Khi
một cell có bộ 16 số **không khớp** với những gì nó từng thấy, sai số tái tạo
tăng vọt. Vượt **1,071** và giữ liên tục **30 giây** thì báo động.

*Vì sao phải giữ 30 giây:* nhiễu đọc luôn có. Không có quy tắc này thì báo động
suốt ngày, và báo động mà ai cũng bỏ qua thì bằng không có.

## Năm trường hợp — nguyên nhân và việc phải làm

### TH-1 · Một cell nóng lên NHANH HƠN hẳn phần còn lại 🔴 NGUY HIỂM

Dấu hiệu: `dT_diff` lớn, `dev_shock` lớn, điểm tăng dốc.

**Nguyên nhân:** mối hàn/tiếp xúc kém (điện trở tiếp xúc sinh nhiệt ngay tại
chỗ), cell nội trở cao bất thường, hoặc chớm chập nội bộ.

**Vì sao đáng sợ:** đây là **tiền đề của thermal runaway**. Cell nóng → phản
ứng phụ tăng → sinh nhiệt nhiều hơn → nóng hơn nữa. Vòng lặp tự nuôi, và một
khi đã chạy thì không dừng được.

**Việc phải làm — ngay lập tức, theo thứ tự:**
1. **Ngắt sạc** (nguy hiểm nhất là lúc đang nạp)
2. Ngắt tải nếu đang chạy
3. **Cách ly pack** — đưa ra chỗ thoáng, xa người và vật dễ cháy
4. Không đụng vào, không sạc lại, để nguội hẳn rồi mới kiểm tra

### TH-2 · Một cell nóng hơn nhưng ỔN ĐỊNH, không tăng 🟡 THEO DÕI

Dấu hiệu: `dev_mean` cao đều, `dT_diff` ≈ 0.

**Nguyên nhân:** cell đó nội trở cao hơn các cell khác (chai hơn), **hoặc** đơn
giản là nó nằm ở vị trí tản nhiệt kém — cell giữa pack luôn nóng hơn cell rìa.

**Việc phải làm:** không khẩn cấp. Ghi nhận, kiểm tra ở lần bảo dưỡng. Phân
biệt hai nguyên nhân bằng cách **đổi vị trí cell**: nếu điểm nóng đi theo cell
thì là cell; nếu ở lại theo vị trí thì là tản nhiệt. Nếu là cell thì đánh dấu
để thay khi tân trang (thay bằng cell **khớp độ chai**, xem QĐ-027).

**Cell sát hai cực chính của pack (thêm 23/09 — CHƯA KIỂM trên pack đội):**
cell đầu và cell cuối chuỗi nối với dây ra tải/sạc. Toàn bộ dòng đi qua mối nối
đó, nên điện trở tiếp xúc ở cực chính (P = I²R) có thể làm hai cell này **ấm hơn**
lúc tải nặng. Ngược lại, lúc nghỉ chúng thường **mát hơn** vì nằm ở rìa, tản nhiệt
tốt hơn cell giữa. Với Lớp 1 thì đây là một **độ lệch theo vị trí**, không phải lỗi:
- Nếu nhỏ và đều (cỡ 1–2 °C) thì nó là một phần của "bình thường" — nhãn có thể
  là TH-2 nhưng điểm không vượt ngưỡng, không báo động.
- Nếu lớn và **tăng theo dòng** thì dễ bị gọi TH-2, thậm chí TH-1 khi dòng tăng
  đột ngột. Cách chữa đúng: hiệu chỉnh ngưỡng tại chỗ trên pack đã biết là tốt
  (QĐ-033), dán đầu dò lên thân cell cách xa tab cực, và siết/kiểm mối nối cực chính.
- Một cực chính nóng DẦN theo thời gian thì là mối nối đang lỏng — đó là lỗi thật,
  chính là thứ TH-1/TH-2 muốn bắt.
Cách kiểm: chạy pack ở dòng cao nhất đo được (INA226 tới 0,819 A) 20–30 phút, xem
cell 1 và cell 6 có lệch theo dòng không.

**Diễn TH-2 (23/09, QĐ-049):** nút "TH-2 · Ấm ổn định" trên trang 1880. Lúc đầu
cell đang được đẩy lên nên hiện TH-1 — đúng, vì nó đang nóng lên thật. Sau ~4
phút cell đứng ở ~+3,9 °C và nhãn chuyển TH-2. Giữ quá ~8 phút thì báo động tự
tắt (nền trượt quen với mức mới) dù dạng vẫn là TH-2 — đó cũng chính là lý do
TH-2 là "theo dõi", không phải "khẩn cấp".

### TH-3 · Một cell LẠNH bất thường 🟡 DỄ BỎ SÓT

Dấu hiệu: `dev_mean` âm rõ rệt.

**Nguyên nhân:** cell **mất kết nối** — không có dòng qua nên không sinh nhiệt.
Hoặc **cảm biến bong khỏi cell**, đang đo không khí.

**Vì sao quan trọng:** ai cũng canh "nóng", không ai canh "lạnh". Nhưng một cell
mất kết nối trong pack nối tiếp nghĩa là pack **mất điện hoàn toàn** hoặc đang
dồn dòng qua chỗ tiếp xúc chớm hở — chính chỗ đó sẽ phát nhiệt và cháy.

**Việc phải làm:** kiểm tra mối nối của cell đó và kiểm tra cảm biến còn dán
chặt không. Đây cũng là lý do có `Sensor_Healthy` trên dashboard.

### TH-4 · Cả pack cùng nóng 🟠 LỚP 1 KHÔNG BẮT ĐƯỢC

**Đây là giới hạn đã biết và phải nói thẳng** (QĐ-012): mọi đặc trưng của Lớp 1
đều là **tương đối giữa các cell**. Cả tám cùng nóng thì không cell nào lệch,
điểm không tăng.

**Bắt bằng cái khác:**
- `t_minus_amb` và `pack_minus_amb` — pack nóng hơn môi trường bao nhiêu
- **Ngưỡng cứng 60 °C** chạy song song, độc lập hoàn toàn với AI

**Nguyên nhân:** sạc quá dòng, trời nóng, tản nhiệt kém, tải nặng kéo dài.

**Việc phải làm:** giảm dòng sạc hoặc dừng nghỉ cho nguội.

> **Cập nhật 15/09 — trường hợp này VẪN CHƯA ĐƯỢC KIỂM.** Đã thử kiểm bằng file
> `Fanoffon` của bộ McMaster (tắt quạt cả pack, tưởng là "nóng đều"), nhưng đó
> là tắt-rồi-bật-lại nhiều lần: mỗi lần bật, cell gần cửa gió nguội nhanh hơn
> cell ở xa, tạo ra chênh lệch không gian thật. Nên file đó **không** kiểm được
> TH-4. Dự đoán ở trên chưa bị bác bỏ, nhưng cũng chưa có bằng chứng ủng hộ.
> Xem `BANG_CHUNG_KIEM_CHUNG_LOP1_THAT_2026-09-15.md` KQ-04.

### TH-5 · Bất kỳ cell nào vượt 60 °C 🔴 NGẮT NGAY

Không qua AI. Một vòng `for` so sánh trực tiếp, chạy song song trong `runAI()`.

**Vì sao phải có:** AI là mô hình học từ dữ liệu, và mô hình nào cũng có lúc
sai. Ngưỡng cứng không học gì cả nên không sai kiểu đó. **Không bao giờ được bỏ
dòng này**, kể cả khi AI tốt lên.

## ⚠️ Chỗ chưa làm — phải nói rõ

**Hiện tại hệ thống chỉ CẢNH BÁO, chưa TỰ HÀNH ĐỘNG.** Chưa có còi, chưa có
LED, chưa có rơ-le ngắt sạc. Nó in ra Serial và đẩy lên dashboard; việc ngắt
sạc đang do con người làm.

Phần còn thiếu, xếp theo độ quan trọng:
1. **Còi + LED** — rẻ, dễ, và là thứ phục vụ NGƯỜI DÙNG C (người lái không mở
   app). Đây cũng chính là khoảnh khắc ăn tiền trong kịch bản demo.
2. **Rơ-le / MOSFET ngắt đường sạc** — cần cân nhắc kỹ: tự ngắt sai lúc cũng là
   một kiểu hỏng. Nên ngắt sạc trước, ngắt tải sau (ngắt tải giữa lúc đang chạy
   xe có thể nguy hiểm hơn là không ngắt).

---

# LỚP 2 — Dự báo sức khoẻ và tuổi thọ (tính trên cloud, 1 lần mỗi chu kỳ sạc)

## Bài toán và mẹo giải

Muốn biết pin còn bao nhiêu % dung lượng thì cách chuẩn là **xả kiệt theo dòng
cố định rồi đếm điện lượng**. Trên xe đang chạy ngoài đường thì không ai làm
được — không ai chịu xả kiệt pin mỗi tuần để đo.

**Mẹo: đọc dáng đường SẠC.** Ngày nào người dùng cũng cắm sạc, và trong lúc sạc
thì đo được đủ điện áp, dòng, nhiệt độ.

## Vật lý phía sau — vì sao đường sạc biết pin chai

Sạc lithium theo kiểu **CC-CV**: giai đoạn đầu giữ **dòng không đổi** (CC), điện
áp dâng dần; chạm 4,2 V thì chuyển sang giữ **áp không đổi** (CV), dòng tụt dần
cho tới khi pin no.

Cell chai thì **nội trở tăng**. Điện áp ở hai đầu cực = điện áp thật của cell +
(dòng × nội trở). Nội trở lớn hơn → phần cộng thêm lớn hơn → **chạm 4,2 V sớm
hơn** dù pin chưa thật sự đầy → vào pha CV sớm, và phải **nằm ở CV lâu hơn** để
nhồi nốt phần điện còn lại.

→ **`t_cv` (thời gian nằm ở pha CV) dài ra theo tuổi pin.** Đây là đặc trưng
mạnh nhất, và mô hình RUL chỉ dùng đúng một mình nó.

Đồng thời quãng 3,90 → 4,15 V đi **nhanh hơn** vì còn ít chỗ để nạp → `t_v_interval`
ngắn lại.

## Đo bằng cái gì

| Cần đo | Linh kiện | Trạng thái |
|---|---|---|
| Dòng sạc | **INA228** (đo qua điện trở shunt) | ⬜ **chưa có** |
| Điện áp pack | cầu chia áp → ADC của ESP32 | ⬜ chưa có |
| Nhiệt độ | DS18B20 (đã có) | ✅ |

Chưa có INA228 nên hiện `simulateChargeCycle()` đang **bịa ra** đường cong V/I
đã hiệu chỉnh theo số liệu NASA. Đường ống thì thật, nguồn số liệu thì chưa.

## Chạy ra sao

```
ESP32, mỗi giây trong suốt lúc sạc:
   đọc V pack, I sạc, T  ->  charge_cycle.cpp tích luỹ
        |
   sạc xong (dòng tụt dưới 0,2 A)
        |
   chốt sổ -> 9 con số  ->  gửi MQTT MỘT lần
        |
   Node-RED nhân với hệ số đã train  ->  RUL, SOH  ->  InfluxDB -> Grafana
```

**9 đặc trưng:** `t_v_interval`, `t_cv`, `i_cv_mean`, `t_charge_total`,
`T_max_ch`, `T_mean_ch`, `T_rise_ch`, `v_start`, `dvdt_cc`.

**Vì sao ESP32 không chạy mô hình:** RUL không cần real-time (1 lần/chu kỳ sạc
là đủ), và để cloud tính thì đổi mô hình không phải nạp lại firmware — quan
trọng vì mô hình chắc chắn còn phải chỉnh.

**Đầu ra là MỨC PACK, không phải từng cell** — pack nối tiếp có dung lượng bằng
dung lượng cell yếu nhất, nên SOH pack đã phản ánh cell yếu nhất (QĐ-027).

## Hai trường hợp và việc phải làm

### TH-6 · SOH xuống dần đều, RUL giảm ổn định 🟢 BÌNH THƯỜNG

**Nguyên nhân:** lão hoá tự nhiên.

**Việc phải làm:** không làm gì cả, cho tới khi RUL xuống dưới ngưỡng đặt trước
(ví dụ 30 chu kỳ) → **lên kế hoạch thay pack**, đặt hàng trước, sắp lịch. Giá
trị ở đây là **chủ động thay vì bị động**: không phải chờ pack chết giữa ca.

### TH-7 · SOH tụt đột ngột 🟡 KIỂM TRA

**Nguyên nhân:** một cell hỏng kéo cả pack xuống (pack nối tiếp = cell yếu
nhất), sạc sai cách, hoặc để pin ở nhiệt độ cao lâu ngày.

**Việc phải làm:** đối chiếu với Lớp 1. Nếu Lớp 1 cũng chỉ ra một cell lệch →
**lỗi một cell**, có thể sửa bằng cách thay cell khớp độ chai. Nếu Lớp 1 im →
**cả pack xuống đều**, tức hết đời tự nhiên.

*Đây chính là chỗ hai lớp bổ trợ nhau*, và là lý do cần cả hai.

## ⚠️ Giới hạn của Lớp 2 — phải nói trước khi bị hỏi

1. **Chu kỳ sạc đang mô phỏng**, chưa có INA228.
2. **Hệ số train trên NASA 18650 2,0 Ah.** Pack đội là 18650 2,55 Ah — cùng
   loại cell nên gần, nhưng **phải sạc ở 1,9 A** mới khớp tốc độ 0,75C (QĐ-026).
3. **Chưa kiểm chứng được trên pin thật**, vì pin của đội mới tinh nên chỉ có
   một điểm ở đầu thang. Cách gỡ: mượn pin đã chai sẵn (QĐ-026).
4. Mô hình tuyến tính **không bao giờ từ chối trả lời** — nó luôn ra một con số
   trông hợp lý kể cả khi đầu vào ngoài dải đã học. Đó là lý do có cờ
   `Extrapolating` trên dashboard (QĐ-019).
