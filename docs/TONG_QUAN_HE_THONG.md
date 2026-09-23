# Hệ giám sát an toàn pack pin xe điện — Tổng quan toàn hệ thống

**Đội Hủ Tiếu · ICTU · AIoT InnoWorks 2026**
Cập nhật 22/09/2026. Đây là tài liệu **tổng hợp** — mỗi phần đều trỏ tới tài
liệu gốc sâu hơn.

> Tài liệu này viết để **một người ngoài đọc và hiểu được hệ thống**, và để cả
> đội trả lời được câu hỏi của giám khảo. Chỗ nào chưa làm được thì ghi rõ là
> chưa làm được — một hệ thống tự khai giới hạn của mình đáng tin hơn một hệ
> thống chỗ nào cũng "đã hoàn thành".

---

# PHẦN I — TỔNG QUAN

## 1. Bài toán

Pin xe điện cháy hầu như không bao giờ cháy cả pack cùng lúc. Nó bắt đầu từ
**một cell** — một mối hàn kém, một cell chai hơn phần còn lại, một chỗ tiếp
xúc tăng điện trở. Cell đó nóng hơn hàng xóm một chút, rồi nóng nhanh hơn, rồi
mất kiểm soát nhiệt (*thermal runaway*).

Khoảng thời gian từ "lệch một chút" tới "cháy" là **hàng giờ đến hàng ngày**.
Đủ dài để cứu, nếu có ai đó đang nhìn.

**Vấn đề của các hệ hiện có:** BMS phổ thông chỉ đo **một** nhiệt độ cho cả
pack, và chỉ ngắt khi vượt ngưỡng cứng (thường 60 °C). Lúc chạm 60 °C thì đã
muộn — và một cell lệch 5 °C giữa một pack mát vẫn cho ra nhiệt độ trung bình
hoàn toàn bình thường.

## 2. Giải pháp một câu

**Đo nhiệt độ TỪNG cell, và dùng AI để phát hiện cell nào đang cư xử khác phần
còn lại — trước khi bất kỳ ngưỡng tuyệt đối nào bị vượt.**

## 3. Hệ thống gồm những gì

```
  ┌─────────────────── TRÊN THIẾT BỊ (không cần mạng) ───────────────────┐
  │                                                                      │
  │   6 × DS18B20 ──► ESP32-S3 ──┬──► LỚP 1: Autoencoder  (1 Hz)         │
  │   (nhiệt độ cell)            │    phát hiện cell bất thường          │
  │   INA226 ────────────────────┤                                       │
  │   (dòng / áp pack)           ├──► LỚP 2: RUL/SOH (mỗi chu kỳ sạc)    │
  │                              │    dự báo tuổi thọ                    │
  │                              │                                       │
  │                              ├──► Còi + LED   (an toàn tại chỗ)      │
  │                              └──► BLE ──► App Android                │
  └──────────────────────────────┬───────────────────────────────────────┘
                                 │ Wi-Fi, MQTT
                                 ▼
  ┌──────────────────────── TẦNG CLOUD ──────────────────────────────────┐
  │   Mosquitto ──► Node-RED ──► InfluxDB ──► Grafana                    │
  │   (môi giới)    (xử lý)      (lưu trữ)    (biểu đồ)                  │
  └──────────────────────────────────────────────────────────────────────┘
```

**Nguyên tắc xuyên suốt: an toàn KHÔNG phụ thuộc mạng.** Mất Wi-Fi, mất cloud,
mất điện thoại — còi vẫn kêu, đèn vẫn đỏ, AI vẫn chạy. Mạng chỉ để *báo cho
người ở xa*, không phải để *quyết định*.

## 4. Ba người dùng, ba giao diện khác nhau

| | **A — Quản lý đội xe** | **B — Thợ kỹ thuật** | **C — Người lái xe** |
|---|---|---|---|
| Ở đâu | Văn phòng, máy tính | **Đứng cạnh pack**, cầm điện thoại | Trên xe |
| Cần gì | Pack nào sắp phải thay | Cell nào đang lệch | **Không mở app bao giờ** |
| Dùng lớp AI nào | **Lớp 2** (tuổi thọ) | **Lớp 1** (bất thường) | Lớp 1 |
| Giao diện | **Grafana** trên web | **App Android qua BLE** | **Còi + đèn** trên thiết bị |
| Cần mạng? | Có | **Không** | **Không** |

Điểm quan trọng: **người lái xe không mở app.** Thiết kế nào đòi họ mở app để
hệ an toàn hoạt động là thiết kế sai. Vì vậy còi và đèn nằm ngay trên thiết bị.

Chi tiết: `docs/NGUOI_DUNG_VA_KICH_BAN.md`

---

# PHẦN II — AI: CÓ GÌ, VÌ SAO CHỌN, ĐIỂM YẾU Ở ĐÂU

## 5. Vì sao gọi là AI mà không phải "công thức"

Phân biệt không nằm ở độ phức tạp, mà ở chỗ **hệ số đến từ đâu**:

```
"Báo động khi > 60 °C"       → con người chọn số 60  → KHÔNG phải AI
"RUL = −26,963 × z(t_cv) + 48,30" → không ai chọn −26,963 → là AI
```

Con số −26,963 được **học từ 364 chu kỳ sạc của 4 quả pin NASA**. Không ai gõ
nó vào.

## 6. Lớp 1 — Autoencoder phát hiện cell bất thường

### Autoencoder là gì

Một mạng nơ-ron được huấn luyện để làm đúng một việc: **chép lại chính đầu vào
của nó**, nhưng bắt buộc phải đi qua một "nút thắt cổ chai" hẹp ở giữa.

```
11 số vào  →  8  →  4  →  8  →  11 số ra
                   ↑
              nút thắt: chỉ 4 số
```

Vì cổ chai quá hẹp, mạng **không thể học thuộc lòng**. Nó buộc phải học "hình
dạng chung" của dữ liệu bình thường. Sau khi học xong:

- Đưa vào một cell **bình thường** → tái tạo lại rất giống → **sai số nhỏ**
- Đưa vào một cell **bất thường** → tái tạo trượt → **sai số lớn**

Sai số tái tạo đó chính là **điểm bất thường**.

### Vì sao chọn autoencoder mà không phải phân loại có nhãn

Vì **không ai có nhãn**. Muốn huấn luyện một bộ phân loại "cell hỏng / cell
tốt" thì phải có hàng nghìn cell hỏng đã được gán nhãn — không tồn tại, và
không thể tạo ra một cách an toàn.

Autoencoder chỉ cần dữ liệu **bình thường**, thứ luôn có sẵn. Nó học "thế nào
là bình thường" rồi báo động khi thấy cái khác. Đây là **học không giám sát**.

### Thông số thật

| | |
|---|---|
| Kiến trúc | 11 → 8 → 4 → 8 → 11 |
| Số tham số | **271** |
| Kích thước | ~1,4 KB (float32) |
| Ngưỡng báo động | sai số tái tạo > **1,0707** |
| Chống báo động giả | phải vượt ngưỡng **liên tục 30 giây** |
| Nhịp chạy | 1 Hz, cho **từng cell một** |
| Tốn trên ESP32-S3 | +5,5 KB flash, +2,9 KB RAM |

### 11 đặc trưng đều là TƯƠNG ĐỐI — và đây là quyết định quan trọng nhất

Không đặc trưng nào là "nhiệt độ tuyệt đối". Tất cả đều so sánh cell với phần
còn lại của pack: lệch bao nhiêu so với trung bình, nóng nhanh hơn pack bao
nhiêu, lệch đột ngột bao nhiêu so với nền của chính nó.

**Lý do:** pack ngoài trời mùa hè ở 45 °C và pack trong kho lạnh ở 10 °C đều
"bình thường". Mô hình dùng nhiệt độ tuyệt đối sẽ báo động loạn mỗi khi trời
nóng. Dùng đặc trưng tương đối thì nhiệt độ môi trường **tự triệt tiêu**.

Xem QĐ-033.

### Điểm mạnh

- **Không cần nhãn lỗi** — học từ dữ liệu bình thường.
- **Chạy trên chip 240 MHz**, không cần cloud, không cần GPU.
- **Bắt sớm hơn ngưỡng cứng** — phát hiện lệch tương đối trước khi chạm 60 °C.
- **Miễn nhiễm nhiệt độ môi trường** nhờ đặc trưng thuần tương đối.

### Điểm yếu — phải nói thẳng

- **Chỉ biết "khác thường", không biết "vì sao".** Xem mục 7.
- **Chuyển giao sang pack lạ: qua được, nhưng chưa trọn vẹn.** Bản đầu (16 đặc
  trưng, QĐ-032) mang từ NASA sang pack McMaster thì **trượt** — báo oan cả file
  bình thường. Bản hiện tại (11 đặc trưng thuần tương đối, QĐ-033) **không cần
  train lại**: file lỗi 0,36–1,26 % thời gian, file bình thường tệ nhất 0,065 %,
  tách biệt ~19 lần. Vẫn **bỏ sót 1 ca** (`UDDS_Blocked_25C` — độ rộng nhiệt chỉ
  tăng 1 °C). Muốn chắc hơn thì hiệu chỉnh NGƯỠNG tại chỗ bằng vài giờ chạy trên
  pack đã biết là tốt — không cần nhãn, không phải train lại mạng.
  Xem `BANG_CHUNG_AE_TUONG_DOI_2026-09-15.md`.
- **Cần ít nhất 3–4 cell** để có "phần còn lại" mà so sánh. Pack 1–2 cell thì
  phương pháp này vô nghĩa.
- **Một cell hỏng từ đầu** sẽ bị coi là "bình thường" nếu nó hỏng ngay từ lúc
  mô hình học nền.

## 7. Phát hiện bất thường rồi thì có biết CỤ THỂ bị sao không?

**Không — và hệ thống nói thẳng điều đó.**

Mô hình **chưa bao giờ được dạy tên của bất kỳ lỗi nào**. Nó chỉ biết "cái này
khác với bình thường". Nhưng hệ không dừng ở một con số vô nghĩa: nó **phân
loại DẠNG** bất thường từ ba số đo, rồi đưa ra **việc phải làm**.

| Dạng | Dấu hiệu | Nghĩa là gì | Việc phải làm |
|---|---|---|---|
| **TH-1** 🔴 | Nóng lên **nhanh hơn cả pack** | Đang sinh nhiệt, nguy cơ mất kiểm soát | **Ngắt sạc, ngắt tải, cách ly pack** |
| **TH-2** 🟡 | Nóng hơn nhưng **ổn định** | Nội trở cao hơn, tiếp xúc kém | Ghi sổ, kiểm lúc bảo dưỡng |
| **TH-3** 🟡 | **Lạnh** bất thường | Cảm biến bong, hoặc cell mất kết nối | Kiểm mối nối và xem cảm biến còn dán chặt |

Phân biệt **TH-1 với TH-2** chính là khác biệt giữa *"cách ly pack ngay"* và
*"mai xem"*. Đó là giá trị thật của việc phân loại dạng.

Và ba số đo dùng để tra bảng (lệch so với pack, tốc độ nóng so với pack, lệch
đột ngột) **đều được hiện ra** bên cạnh lời khuyên — để người dùng **kiểm được**
thay vì phải tin. Một lời khuyên không kiểm được thì lúc nó sai sẽ không ai
phát hiện.

Chi tiết: `docs/HAI_LOP_AI_HOAT_DONG_THE_NAO.md`

## 8. Lớp 2 — Dự báo tuổi thọ (RUL / SOH)

### Hai con số trả lời hai câu khác nhau

| | Trả lời | Ví von |
|---|---|---|
| **SOH** — State of Health | *Pin còn khoẻ bao nhiêu % so với mới?* | Cân nặng **hiện tại** |
| **RUL** — Remaining Useful Life | *Còn sạc được bao nhiêu chu kỳ nữa?* | **Dự báo** còn bao lâu |

### Nguyên lý vật lý

Sạc pin lithium theo kiểu **CC-CV**: giai đoạn đầu giữ **dòng** cố định cho áp
tăng dần; chạm áp giới hạn thì chuyển sang giữ **áp** cố định cho dòng tụt dần.

```
Pin chai → nội trở TĂNG → chạm áp giới hạn SỚM hơn
        → pha CC ngắn lại, pha CV DÀI ra
        → t_cv (thời gian trong pha CV) là thước đo độ chai
```

**Vậy mô hình này thực chất là mô hình nội trở** — chỉ là đo gián tiếp, không
cần thiết bị đo nội trở chuyên dụng.

Và nó **nối thẳng sang Lớp 1**: nội trở cao → cùng một dòng thì toả nhiệt `I²R`
nhiều hơn → chính là dạng **TH-2 "nóng hơn nhưng ổn định"**. Hai lớp AI đang
nhìn cùng một hiện tượng vật lý từ hai phía khác nhau.

### Mô hình

```
RUL = −26,963 × z(t_cv) + 48,30        ← ĐÚNG MỘT đặc trưng, 2 tham số
SOH = 0,91263 + Σ(9 hệ số × 9 đặc trưng)  ← hồi quy Ridge
```

9 đặc trưng: `t_v_interval, t_cv, i_cv_mean, t_charge_total, T_max_ch,
T_mean_ch, T_rise_ch, v_start, dvdt_cc`

### Vì sao KHÔNG dùng deep learning — và đây là bằng chứng, không phải ý kiến

Đội train **5 họ mô hình**, so bằng *leave-one-battery-out* (giấu hẳn một quả
pin, bắt đoán quả chưa từng thấy):

| Mô hình | Tham số | Sai số (MAE, chu kỳ) |
|---|---|---|
| **Tuyến tính 1 đặc trưng** | **2** | **12,2** ✅ |
| GBM | ~nghìn | 17,2 |
| LSTM | ~3.500 | 20,3 |
| Ridge 9 đặc trưng | 10 | 27,0 |
| Đoán trung bình (baseline) | 0 | 25,8 |

**LSTM 3.500 tham số THUA mô hình 2 tham số.** Khi giám khảo hỏi *"sao không
dùng deep learning?"* — câu trả lời là *"đã làm, đã đo, nó thua, đây là bảng
số"*.

⚠️ **Giới hạn của kết luận này:** chỉ đúng ở quy mô **4 quả pin**. Với hàng
nghìn pin thật thì mô hình lớn có thể thắng. Đội không được nói quá (QĐ-017).

### Điểm mạnh

- **2 tham số** — chạy trên chip, giải thích được từng hệ số.
- **Đánh giá đúng cách**: leave-one-battery-out, không phải chia ngẫu nhiên.
- **Chỉ dùng đặc trưng đo được ngoài đời** (pha sạc), không dùng dung lượng
  phóng — thứ chỉ đo được trong phòng thí nghiệm.
- **Có cờ ngoại suy** — xem dưới.

### Điểm yếu

- **Hệ số vẫn là của NASA**, chưa hiệu chỉnh trên pack của đội.
- **Chu kỳ sạc hiện đang mô phỏng** vì thiếu thiết bị đo dòng đúng dải.
- **SOH kém tin hơn RUL** (mục 12).

## 9. Cờ ngoại suy — tính năng quan trọng hơn cả con số RUL

Mô hình tuyến tính **không bao giờ từ chối trả lời**. Đưa vào dữ liệu hoàn toàn
lạ, nó vẫn ra một con số trông rất hợp lý.

Nên hệ kiểm: nếu bất kỳ đặc trưng nào lệch quá **±3 độ lệch chuẩn** so với dữ
liệu huấn luyện → bật cờ, và **cảnh báo hiện TRƯỚC con số**:

```
⚠️ NGOAI DAI HUAN LUYEN (2/9 dac trung) - SOH 78.6% | con ~0 chu ky
```

Không có cờ này, lỗi hiệu chỉnh chỉ hiện ra dưới dạng "RUL = 0" — rất dễ tưởng
pin hỏng thật. **Đã chạy đúng trên máy thật ngày 22/09.**

## 10. Ngưỡng cứng 60 °C — đường AI KHÔNG được phép chạm vào

Song song với cả hai lớp AI là một đường kiểm **hoàn toàn độc lập**: nhiệt độ
bất kỳ cell nào ≥ 60 °C → mức NGUY KỊCH, bất kể AI nói gì.

**Nếu toàn bộ code AI hỏng và không bao giờ báo động, nhánh này vẫn chạy.** Đó
là lý do nó là một đường riêng chứ không phải một điều kiện ghép vào AI.

Bốn mức báo động phân biệt bằng **cả màu lẫn nhịp nháy** — người mù màu đỏ-lục
(~8% nam giới) vẫn phân biệt được bằng nhịp.

---

# PHẦN III — TẦNG CLOUD

## 11. Grafana, InfluxDB, Node-RED, MQTT là gì

| Thành phần | Là gì | Vai trò trong hệ |
|---|---|---|
| **MQTT / Mosquitto** | Giao thức nhắn tin nhẹ cho thiết bị IoT | ESP32 "đăng" dữ liệu lên một chủ đề; ai quan tâm thì "theo dõi". Thiết bị không cần biết ai đang nghe |
| **Node-RED** | Công cụ nối luồng dữ liệu bằng kéo-thả | Nhận gói từ MQTT, chuyển đổi, ghi vào database. Cũng phục vụ trang điều khiển cho người trình diễn |
| **InfluxDB** | Cơ sở dữ liệu chuyên cho **chuỗi thời gian** | Lưu hàng triệu điểm "lúc 10:32:05, cell 3 = 27,4 °C". Nhanh hơn database thường cho loại truy vấn "6 giờ qua" |
| **Grafana** | Công cụ vẽ biểu đồ từ database | Màn hình của **Người dùng A**. 16 panel: trạng thái pack, nhiệt độ từng cell, điểm AI, SOH/RUL |

**Grafana có gì hay:** mã nguồn mở, miễn phí, tự host được; nối thẳng InfluxDB;
có sẵn ngưỡng, cảnh báo, và bảng điều khiển thời gian thực. Đội **sinh dashboard
bằng script** (`make_dashboard.py`) chứ không kéo thả — để giao diện nằm trong
git, dựng lại máy không mất, và ai đổi gì cũng thấy trong lịch sử.

Có **hai bản**: tiếng Việt (`hutieu-pin`) và tiếng Anh (`hutieu-pin-en`), sinh
từ cùng một mã nguồn nên không bao giờ lệch nhau về bố cục.

## 12. Store-and-forward — 90 giây quan trọng nhất của bài trình diễn

Mất mạng thì dữ liệu **không mất**. ESP32 ghi xuống flash, nối lại thì đẩy bù
**đúng thứ tự gốc**, với **timestamp do chính thiết bị sinh** — nên trên biểu đồ
các điểm nằm đúng vị trí thời gian thật, không dồn cục vào lúc nối lại.

Đã kiểm: ngắt mạng 60 s, 83 điểm liên tục, **0 lỗ hổng**. Lúc gặp lỗi mạng thật
ngày 22/09, spool giữ **215 KB** không mất gói nào.

---

# PHẦN IV — USER FLOW

## 13. Luồng của Người dùng B (thợ kỹ thuật) — thường dùng nhất

```
1. Nhận việc "kiểm pack #37"
2. Đứng cạnh pack, mở app "Pack pin Hủ Tiếu"
3. Chạm "Kết nối tới pack"        → app tự nối, không cần Wi-Fi
4. Nhìn băng trạng thái            → BÌNH THƯỜNG / THEO DÕI / BÁO ĐỘNG / NGUY KỊCH
5. Nếu có bất thường:
   ├─ xem ô "Lớp 1 — cell nghi ngờ"  → cell nào, dạng gì (TH-1/2/3)
   ├─ xem ô "Số đo đã dùng để chẩn đoán" → KIỂM lại lời khuyên
   └─ làm theo "việc phải làm"
6. Còi quá to khi đang làm việc?
   └─ Chạm "Tắt tiếng"  → hỏi mã PIN (lần đầu) → còi im
      • Bất thường Lớp 1 → tắt là tắt hẳn
      • Mức NGUY KỊCH   → tự kêu lại sau 2 phút để nhắc
      • Màn hình LUÔN hiện "BẤT THƯỜNG CHƯA ĐƯỢC XỬ LÝ"
7. Xong việc → "Ngắt kết nối"
```

**Vì sao tắt còi phải nhập PIN:** người đi ngang qua không được phép tắt còi báo
cháy. Chỉ máy đã ghép đôi mới ghi lệnh được — chặn ngay ở tầng GATT, chưa tới
được phần mềm.

## 14. Luồng của Người dùng A (quản lý)

```
1. Đầu ca, mở Grafana trên máy tính
2. Hàng đầu trả lời 3 câu trong 3 giây:
   "Pack có an toàn không?" · "Cell nào?" · "Tin được số này không?"
3. Xem hàng SOH/RUL → pack nào sắp phải thay
4. Rút pack sắp hết đời khỏi vòng quay, đặt hàng thay
```

Hàng *"số này có thật không"* (`Meter_IsReal`, `Sensor_Healthy`) là thứ phân
biệt hệ này với một bản demo tô vẽ: **số giả định phải TỰ KHAI là giả định,
ngay trên màn hình**.

## 15. Luồng của Người dùng C (người lái xe)

```
Không mở gì cả.
Pin có vấn đề → CÒI KÊU + ĐÈN ĐỎ ngay trên thiết bị.
```

---

# PHẦN V — SO SÁNH VỚI THỊ TRƯỜNG

## 16. So với BMS phổ thông

| | BMS phổ thông | Hệ của đội |
|---|---|---|
| Cảm biến nhiệt | **1–2** cho cả pack | **6**, mỗi cell một |
| Phát hiện | Ngưỡng cứng 60 °C | **AI so sánh tương đối** + ngưỡng cứng |
| Phát hiện sớm | Không | **Có** — bắt lệch trước khi chạm ngưỡng |
| Biết cell nào | Không | **Có** |
| Dự báo tuổi thọ | Không | **Có** (RUL/SOH) |
| Gửi lên cloud | Không | Có, kèm đệm khi mất mạng |
| Giá | Rẻ | Rẻ + ~1 module đo dòng |

## 17. So với BMS cao cấp trên xe điện thương mại

Nói thẳng: **BMS của Tesla/BYD làm được tất cả những gì hệ này làm, và hơn
nhiều** — họ đo nội trở trực tiếp, cân bằng chủ động, giảm dòng sạc theo nhiệt
độ (chuẩn JEITA), ước lượng thời gian sạc đầy.

Khác biệt nằm ở **chỗ khác**:

| | BMS cao cấp | Hệ của đội |
|---|---|---|
| Có sẵn cho ai | Chỉ hãng xe, gắn liền xe | **Lắp thêm** vào pack có sẵn |
| Giá | Trong giá xe | Vài trăm nghìn |
| Mở/đóng | Đóng kín, không truy cập được | **Mã nguồn mở** |
| Thị trường Việt Nam | Xe máy điện giá rẻ **không có** | Đây là chỗ trống |

**Thị trường thật:** hàng triệu xe máy điện, xe đạp điện và pin đổi ở Việt Nam
dùng pack lắp ráp với BMS phổ thông 1 cảm biến. Đó mới là nơi cháy nổ xảy ra,
và cũng là nơi chưa có giải pháp nào.

## 18. So với các nghiên cứu học thuật về RUL

Phần lớn bài báo dùng **dung lượng phóng** làm đặc trưng chính — đo được trong
phòng thí nghiệm, **không đo được trên xe đang chạy** (không ai xả cạn pin theo
lịch để đo).

Hệ này **cố ý chỉ dùng đặc trưng của pha SẠC**, vì sạc là việc người dùng làm
đều đặn ngoài đời. Đổi lại độ chính xác thấp hơn, nhưng **dùng được thật**.

---

# PHẦN VI — GIỚI HẠN VÀ LỘ TRÌNH

## 19. Hệ thống hiện KHÔNG làm được gì

Phần này quan trọng với mục *"khả năng phản biện"*. Nói trước thì là hiểu biết;
để giám khảo tìm ra thì là lỗ hổng.

| Chưa làm được | Vì sao |
|---|---|
| **Kiểm chứng Lớp 2 trên pack của đội** | Cần một pack **đã già đi** để đối chiếu — mất nhiều tháng. Tiền không rút ngắn được |
| **Đo dòng sạc thật** | Shunt INA226 chỉ tới 0,819 A; sạc 2 A vượt thang |
| **Lớp 1 chuyển giao sang pack lạ** | Bản QĐ-033 qua được trên McMaster (tách biệt ~19×) nhưng bỏ sót 1/3 file lỗi (`UDDS_Blocked_25C`). Chưa kiểm trên pack thứ ba |
| Đo nội trở trực tiếp | Cần bước nhảy dòng có điều khiển |
| Giảm dòng sạc theo nhiệt độ (JEITA) | Cần điều khiển bộ sạc |
| Ước lượng thời gian sạc đầy | Cần dòng đo được |
| Ngắt sạc tự động khi báo động | Cần rơ-le; rủi ro ngắt sai phải cân nhắc |

## 20. Lộ trình triển khai

### Giai đoạn 1 — Hoàn thiện đo lường *(1–2 tuần, ~500k)*

- [ ] Thay INA226 bằng **INA228** — code đã tự nhận chip, **không sửa dòng nào**
- [ ] Hoặc sửa firmware để dùng chính việc bão hoà làm tín hiệu bắt đầu sạc *(0 đồng)*
- [ ] Hiệu chỉnh điện trở shunt bằng nguồn dòng đã biết *(mục 6.33)*
- [ ] Dán đầu dò lên pack thật, bỏ breadboard, hàn cố định

**Kết quả:** đo được chu kỳ sạc thật thay vì mô phỏng.

### Giai đoạn 2 — Hiệu chỉnh trên pack thật *(2–6 tháng)*

- [ ] Chạy sạc–xả liên tục, ghi lại toàn bộ chu kỳ *(chip chứa được cả đời pin
      trong 32 KB)*
- [ ] **Kiểm chéo trên bộ dữ liệu công khai thứ hai** (CALCE/UPC) — *làm được
      ngay, không cần phần cứng*
- [ ] Huấn luyện lại hệ số trên dữ liệu của chính đội
- [ ] Đo lại sai số bằng leave-one-battery-out trên pack thật

**Kết quả:** RUL/SOH có căn cứ trên pack của đội, không còn mượn NASA.

### Giai đoạn 3 — Mở rộng chức năng *(3–6 tháng)*

- [ ] Đo nội trở trực tiếp (DCIR) bằng bước nhảy dòng
- [ ] Ước lượng thời gian sạc đầy
- [ ] Giảm dòng sạc theo nhiệt độ (JEITA)
- [ ] Ngắt sạc tự động khi TH-1

### Giai đoạn 4 — Đưa ra thực địa *(6–12 tháng)*

- [ ] Vỏ chống nước chống rung
- [ ] 4G thay Wi-Fi *(module A7670C đã khảo sát)*
- [ ] Nguồn dự phòng riêng cho module *(TP4056 + cell 18650)*
- [ ] Thử trên đội xe thật, nhiều pack cùng lúc
- [ ] Mở rộng 6S → 8S → 16S/20S *(đổi một hằng số `PACK_N_CELLS`)*

## 21. Tiềm năng thương mại hoá

**Khách hàng:** trạm đổi pin, đội xe giao hàng, xưởng lắp ráp pack, hãng xe máy
điện nhỏ.

**Giá trị bán được:**
- Trạm đổi pin: biết pack nào sắp hỏng **trước khi** giao cho khách
- Đội xe: lên lịch thay pin theo dữ liệu thay vì theo lịch cố định
- Bảo hiểm / an toàn: có nhật ký chứng minh pack được giám sát

**Vì sao nhân rộng được:** mã nguồn mở, linh kiện phổ thông, đổi số cell chỉ
cần sửa một hằng số, và giao thức MQTT là chuẩn công nghiệp nên ghép được vào
hệ thống sẵn có của khách.

---

# PHẦN VII — BẰNG CHỨNG

## 22. Những gì đã chạy thật, không phải mô phỏng

| Việc | Bằng chứng |
|---|---|
| 6 cảm biến, bus sạch, lỗi 0,00% qua 3×2.800 lần đọc | `BANG_CHUNG_CAM_BIEN_2026-09-14.md` |
| Lớp 1 bắt đúng cell trên board thật *(cell 5: 5,189 vs các cell khác ~0,4)* | `BANG_CHUNG_AI_2026-09-11.md` |
| Bản C khớp bản Python từng số *(lệch < 1e-6)* | `BANG_CHUNG_AE_TUONG_DOI_2026-09-15.md` |
| Lớp 2 chạy trên chip, khớp cloud *(lệch 3,2e-5)*, lịch sử sống qua mất điện | `BANG_CHUNG_LOP2_TREN_CHIP_2026-09-22.md` |
| App Android nối BLE thật, tắt còi tới được chip *(3 đường kiểm độc lập)* | `BANG_CHUNG_APP_ANDROID_2026-09-22.md` |
| Mức NGUY KỊCH: tắt được và tự kêu lại sau 2 phút | `BANG_CHUNG_NGUY_KICH_2026-09-22.md` |
| Tắt tiếng Lớp 1 giữ qua dao động mức *(0 lần kêu lại trong 212 s)* | `BANG_CHUNG_TAT_COI_LOP1_2026-09-22.md` |
| Store-and-forward: ngắt 60 s, 83 điểm, 0 lỗ hổng | `BANG_CHUNG_KIEM_THU_2026-09-06.md` |
| Lớp 1 bản đầu **TRƯỢT** chuyển giao; bản thuần tương đối qua nhưng **bỏ sót** `UDDS_Blocked_25C` | `BANG_CHUNG_KIEM_CHUNG_LOP1_THAT_2026-09-15.md`, `BANG_CHUNG_AE_TUONG_DOI_2026-09-15.md` |

Dòng cuối cùng cố ý nằm trong bảng. **Một bảng bằng chứng chỉ toàn ✅ là một
bảng chưa ai kiểm nghiêm túc.**

## 23. Đọc tiếp ở đâu

| Cần biết | Đọc |
|---|---|
| Hai lớp AI hoạt động ra sao, gặp trường hợp nào thì làm gì | `docs/HAI_LOP_AI_HOAT_DONG_THE_NAO.md` |
| Vì sao chọn hướng hiện tại — **48 quyết định, kèm cả những lần chọn sai** | `docs/DECISION_LOG.md` |
| Thế nào là "chạy đúng" | `docs/TEST_VA_ACCEPTANCE.md` |
| Ai dùng, dùng lúc nào | `docs/NGUOI_DUNG_VA_KICH_BAN.md` |
| Code nằm ở đâu | `docs/MODULE_MAP.md` |
| Ranh giới không được phá | `docs/DATA_CONTRACT.md` |
| Mua gì, đấu nối ra sao | `VEDCaPhenika/PHAN_CUNG_VA_KIEN_TRUC_HuTieu.md` |

**`DECISION_LOG.md` là tài sản lớn nhất cho phần phản biện** — nó ghi cả những
lần đội chọn sai và vì sao. Đó đúng là thứ barem gọi là *"trả lời có căn cứ"*.
Phải **đọc lại trước khi thi**, không phải chỉ viết ra rồi để đấy.
