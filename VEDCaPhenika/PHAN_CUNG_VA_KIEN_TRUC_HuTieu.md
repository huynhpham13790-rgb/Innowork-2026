# PHẦN CỨNG & KIẾN TRÚC HỆ THỐNG — Đội Hủ Tiếu

**Ngày:** 03/09/2026 · Bổ sung cho `KHUNG_NGHIEN_CUU_v2_HuTieu.md`

---

## PHẦN 1 — CẤP NGUỒN: CÂU TRẢ LỜI NGẮN VÀ 4 CÁI BẪY

> ### ⚠️ ĐỌC TRƯỚC: có HAI pack khác nhau trong tài liệu này
>
> | | **Pack thí nghiệm** (bán kết 26/09) | **Pack xe thật** (chung kết / sản phẩm) |
> |---|---|---|
> | Cấu hình | **8S 18650** (2 đế 4 cell nối tiếp) | 16S–20S |
> | Điện áp danh định | 29,6 V | 48 / 60 / 72 V |
> | Điện áp sạc đầy | **33,6 V** | 54,6 / 67,2 / **84,0 V** |
> | Nguồn cho ESP32-S3 | USB hoặc buck nhỏ — đơn giản | buck ≥100V + cầu chì + TP4056 + pin dự phòng |
> | Đo dòng | **INA228 (85V)** — INA226 (36V) cũng đủ. **INA219 (26V) KHÔNG dùng được** | INA228 hoặc Hall ACS758 |
>
> **Toàn bộ Phần 1 dưới đây viết cho PACK XE THẬT.** Với pack 8S 33,6V trên bàn, Bẫy #1, #2 và phần buck 100V **không áp dụng** — cậu không cần mua buck cao áp cho bán kết. Bẫy #3 (BMS ngắt lúc sự cố) và #4 (cầu chì, lấy điện sau BMS) thì vẫn giữ, vì #3 chính là điểm demo ăn tiền.
>
> Lý do vẫn viết cả phần 60–84V: giám khảo chắc chắn hỏi "lên xe thật thì sao?". Có sẵn phân tích này cho thấy đội biết mạch bàn không tự động scale lên 84V.

### Trả lời ngắn
**Có**, ESP32 lấy nguồn từ chính pack pin xe điện. Nhưng **không cắm thẳng được** — và có 4 chỗ dễ chết mà nếu không biết trước sẽ cháy linh kiện hoặc hỏng cả thiết kế.

### Bẫy #1 — Điện áp pack CAO HƠN con số ghi trên pin

Pack "60V" không phải 60V. Đó là điện áp danh định. Khi sạc đầy:

| Pack danh định | Số cell nối tiếp | **Điện áp khi sạc đầy** |
|---|---|---|
| 48V | 13S | **54,6 V** |
| 60V | 16S | **67,2 V** |
| 72V | 20S | **84,0 V** |

🔥 **Hệ quả:** module giảm áp **LM2596HV** rất phổ biến ở VN chỉ chịu được **tối đa 60V đầu vào**. Cắm vào pack 60V (đỉnh 67,2V) là **quá ngưỡng** → nổ. Pack 72V thì khỏi bàn.

→ **Quy tắc mua:** chọn buck có điện áp đầu vào chịu được **≥ 100V**. Đừng tin con số danh định của pack.

### Bẫy #2 — Dòng nghỉ (quiescent current) giết chết cả bài toán tiết kiệm điện

Ở §2 tài liệu khung, tớ có nói ESP32 deep sleep chỉ ăn ~10 µA. Đúng — **nhưng vô nghĩa nếu mạch giảm áp phía trước ăn 5–10 mA.**

Tính thử: buck rẻ tiền ăn 8 mA × 24h = **192 mAh/ngày**. Xe đỗ 1 tháng = **~5,8 Ah** bị rút không. Đủ để làm kiệt một pack nhỏ.

**Đây là cái bẫy nguy hiểm nhất trong toàn bộ thiết kế** — vì nó im lặng, không báo lỗi, chỉ âm thầm rút pin. Và nó là điều mỉa mai chết người: *thiết bị bảo vệ pin lại chính là thứ làm hỏng pin.* Giám khảo có kinh nghiệm sẽ hỏi đúng câu này.

**Cách xử lý (chọn 1):**
- Mua buck có Iq thấp (dòng chip TPS54160, MP9486A... — đọc datasheet mục "quiescent current", tìm số < 100 µA)
- Hoặc: dùng MOSFET cắt hẳn buck khi ngủ, giữ một LDO tí hon Iq thấp nuôi RTC để đánh thức
- **Bắt buộc dù chọn cách nào:** mạch **cắt nguồn khi điện áp pack xuống thấp** (low-voltage cutoff). Pack tụt dưới ngưỡng → module tự ngắt.

### Bẫy #3 — Nếu BMS ngắt, module chết đúng lúc cần nó nhất

Khi có sự cố nhiệt, BMS sẽ ngắt đầu ra để tự bảo vệ. Nếu module chỉ ăn điện từ đầu ra pack → **module tắt ngay khoảnh khắc xảy ra sự cố.** Vô dụng.

**Giải pháp: module có pin dự phòng riêng.** Một cell 18650 nhỏ (hoặc LiPo 500–1000 mAh) + mạch sạc TP4056, được nạp rỉ rả từ pack.

Lợi ích kép, và đây là **luận điểm thiết kế mạnh nhất để trình bày**:
- Module vẫn kêu còi và ghi log kể cả khi BMS đã ngắt hoặc pack bị tháo ra
- Tách rời hoàn toàn bài toán rút điện ký sinh ở Bẫy #2
- Với mô hình đổi pin: pack bị tháo khỏi xe, module vẫn theo dõi được pack trong lúc vận chuyển/lưu kho — mà **cháy trong kho là rủi ro lớn nhất của đơn vị vận hành đổi pin**

### Bẫy #4 — Đấu nguồn ở đâu

- ✅ Lấy từ **đầu ra sau BMS** (cổng phóng) hoặc cổng sạc → vẫn được BMS bảo vệ
- ❌ **Không** đấu thẳng vào cực cell trước BMS → bỏ qua toàn bộ bảo vệ, cực kỳ nguy hiểm
- ⚠️ Bắt buộc có **cầu chì 0,5–1A** trên dây lấy nguồn
- ⚠️ 60V+ DC đã vượt ngưỡng an toàn khi chạm (60V DC) — bọc cách điện tử tế, không để dây trần

### Sơ đồ chuỗi nguồn đề xuất

```
Pack pin (48–84 V)
   │
   ├─ [Cầu chì 0,5A]
   │
   ├─ [Buck ≥100V in → 5V, Iq thấp]
   │        │
   │        ├─ [TP4056] ─ [Cell 18650 dự phòng] ──┐
   │        │                                      │
   │        └──────────────────────────────────────┴─→ [LDO 3.3V] → ESP32 + cảm biến
   │
   └─ [Chia áp điện trở] ─→ ADC ESP32 (đo điện áp pack)
```

---

## PHẦN 2 — DANH SÁCH PHẦN CỨNG CẦN MUA

> **Cập nhật 04/09/2026 — kinh phí đã xin được, không còn là ràng buộc.** Bảng dưới đây liệt kê theo **tên linh kiện**, mua ở shop nào cũng được. Link chỉ để tham chiếu giá.
>
> **Hai quyết định thay đổi so với bản trước:**
> 1. **Chuyển từ ESP32 đời đầu sang ESP32-S3** — xem hộp so sánh bên dưới.
> 2. **Pack thí nghiệm làm 8 cell (8S), không phải 4S** — kéo theo phải dùng INA228/INA226 thay INA219.

### Bảng A — Nguyên mẫu phòng lab (bản 8S)

**A1. Vi điều khiển**

| Linh kiện | SL | Ghi chú |
|---|---|---|
| **ESP32-S3-DevKitC-1 N16R8** (16MB Flash / 8MB PSRAM) | 2 | Bản N16R8, đừng lấy N8R2 |

> ### Vì sao S3 chứ không phải ESP32 đời đầu?
>
> "DevKitC **V4**" là *tên phiên bản bo mạch*, con chip trên nó là ESP32 đời đầu (Xtensa LX6). **S3 là con chip thế hệ mới** (LX7) — không cùng loại để so.
>
> | | ESP32 (WROOM-32D/E) | **ESP32-S3** |
> |---|---|---|
> | Lệnh vector (SIMD) cho ML | ❌ không | ✅ **có** — ESP-NN dùng để tăng tốc kernel INT8 trong TFLite Micro. *Lớp 1 hiện KHÔNG cần tới: mô hình chỉ 356 tham số, nhân tay đã thừa nhanh. Để dành cho mô hình lớn hơn sau này.* |
> | PSRAM | thường không / 4MB | tới 8MB |
> | Flash | 4MB | 16MB → buffer offline dài hơn nhiều (Bẫy mất mạng, §9 khung nghiên cứu) |
> | USB | qua chip CP2102/CH340 | native USB-OTG |
>
> Autoencoder hiện tại nhỏ, chip nào cũng chạy. Nhưng nếu lên chung kết muốn đẩy cả mô hình RUL dạng 1D-CNN/TCN xuống edge thì **chỉ S3 mới đủ cửa**.
>
> 🎤 **Câu nói trên slide:** "Đội chọn ESP32-S3 vì nó có tập lệnh vector phục vụ suy luận ML — đúng trọng tâm AIoT." Đây là câu trả lời ăn điểm.
>
> ⚠️ **Sơ đồ chân S3 KHÁC ESP32 đời đầu.** Mọi gán chân trong lộ trình cũ phải gán lại. Không khó, chỉ là đừng copy nguyên.

**A2. Cảm biến nhiệt**

| Linh kiện | SL | Ghi chú |
|---|---|---|
| **DS18B20 vỏ kim loại chống nước, dây 1m** | 12 | 8 đo cell + 2 đo vỏ pack/môi trường + 2 dự phòng |
| Điện trở 4,7 kΩ 1/4W (pull-up 1-Wire) | vài | Hạ xuống 2,2 kΩ nếu gặp lỗi CRC |

**A3. Đo dòng / điện áp**

| Linh kiện | SL | Ghi chú |
|---|---|---|
| **Module INA228** (85V, I2C) | 1 | Ưu tiên — đằng nào lên xe thật cũng phải dùng, mua luôn khỏi làm lại |
| *(thay thế)* Module INA226 (36V) | 1 | Vẫn đủ cho 8S (33,6V đỉnh). Chọn 1 trong 2 |
| Điện trở chia áp sai số 1% | vài | Đo áp pack qua ADC của ESP32 |

⚠️ **GIỚI HẠN ĐIỆN ÁP — đọc kỹ trước khi đấu:**

| Chip | Điện áp bus tối đa | Dùng được với |
|---|---|---|
| INA219 | 26 V | Pack tối đa 6 cell. **8S = 33,6V → hỏng chip.** Đã loại khỏi phương án 8S |
| INA226 | 36 V | Pack tối đa 8 cell — vừa đủ, không dư |
| **INA228** | **85 V** | 8S thoải mái, và pack xe thật 60V (67,2V đỉnh) |

**A4. Pack thí nghiệm 8S**

| Linh kiện | SL | Ghi chú |
|---|---|---|
| **Cell pin 18650** *(kế hoạch: LG HG2 3 Ah — thực tế đã mua **2,55 Ah**, xem QĐ-026)* | 9 | 8 cho pack + 1 làm nguồn dự phòng module. **Mua cùng một lô** |
| **Đế pin (hộp nhựa rỗng) 18650 loại 4 cell nối tiếp** | 2 | Ghép nối tiếp 2 đế → 8S. *Đây là khung nhựa, KHÔNG có pin bên trong* |
| **BMS 8S 30A có cân bằng (balance)** | 1 | Bắt buộc. Chính nó tạo ra Bẫy #3 để demo |
| Bộ sạc 33,6V 2A | 1 | Đúng điện áp cho 8S |
| Dây silicone 18AWG, terminal block, ống co nhiệt | — | Đấu nối pack |

> **Vì sao 8 cell chứ không phải 4?**
> Luận điểm cốt lõi của dự án là "cell này nóng bất thường **so với các cell còn lại**". 4 cell hỏng 1 thì chỉ còn 3 mẫu tham chiếu — giám khảo vặn được ngay "3 mẫu thì thống kê gì". 8 cell còn 7 tham chiếu, lập luận vững hẳn.
> Thêm nữa, 8 cell xếp hàng bắt đầu xuất hiện **gradient theo vị trí** (cell giữa nóng hơn cell rìa) — hiện tượng có thật trong pack xe, cho Autoencoder một mẫu hình không tầm thường để học thay vì "8 cell y hệt nhau".
>
> **Vì sao LG HG2 chứ không phải cell rẻ:** không phải vì cần dòng xả cao, mà vì cần các viên **giống hệt nhau**. Cell rẻ vốn đã lệch nội trở → tự nóng khác nhau ngay từ đầu → Autoencoder học phải nhiễu đó và mất hết ý nghĩa. Riêng viên nguồn dự phòng nằm ngoài phép đo nên loại nào cũng được.
>
> ⏱️ **Chiến thuật thi công:** đấu 4 cell chạy thông pipeline **trước** trong tuần này, xong mới mở rộng lên 8. Đừng chờ đủ đồ mới bắt đầu — còn 11 ngày tới bán kết.

**A5. Gây lỗi có kiểm soát (phần demo)**

| Linh kiện | SL | Ghi chú |
|---|---|---|
| **Điện trở công suất sứ 10W — giá trị `10R`** ⭐ | 4 | Giá trị chính. Xem bảng chọn bên dưới |
| Điện trở công suất sứ 10W — `5R` | 2 | Dự phòng |
| Điện trở công suất sứ 10W — `20R` | 2 | Dự phòng, hợp nếu chỉ có adapter 12V |
| **Module MOSFET công suất logic-level** (IRF520 hoặc tương đương) | 1 | Để ESP32 **tự** bật/tắt điện trở sưởi theo kịch bản demo |
| Nguồn DC điều chỉnh được 0–30V / 5A (bench power supply) | 1 | Cấp cho điện trở sưởi. Mượn phòng lab trường được thì mượn |
| Băng keo Kapton + keo tản nhiệt | — | Gá cảm biến và điện trở lên cell. **Tiếp xúc kém = số đo sai** |

> ### Chọn giá trị điện trở sưởi
>
> Trên shop ghi `1R 2R 5R 8R 10R 15R 20R 25R 60R` — **`R` nghĩa là Ω (ohm)**, còn `5%` là sai số, không quan trọng ở đây.
>
> Công suất sưởi = V²/R. Với **10Ω** cậu có sẵn cả dải từ nhẹ đến mạnh mà không bao giờ vượt định mức 10W:
>
> | Điện áp đặt | Công suất | Dùng để |
> |---|---|---|
> | 5 V | 2,5 W | Sưởi chậm — giả lập cell suy giảm từ từ, tạo dị thường tinh vi |
> | **7 V** | **4,9 W** | **Tốc độ demo tốt nhất** — cell lên ~50°C trong vài phút |
> | 10 V | 10,0 W | Kịch trần, không vượt |
>
> - **5R**: chỉ tới 7,07V là chạm trần 10W → dải chỉnh hẹp hơn
> - **20R**: phải lên 14,1V mới đủ nóng, nhưng hợp nếu chỉ có adapter 12V (→ 7,2W)
> - 🚫 **KHÔNG mua 1R hoặc 2R**: chỉ 5V là 1R đã ăn 25W, 2R ăn 12,5W — vượt định mức, cháy đen trong vài chục giây
>
> ### ⚠️ AN TOÀN — điện trở nóng KHÁC cell nóng
>
> Điện trở sứ 10W chạy đúng 10W thì bề mặt có thể vượt **200°C**. Mình cần **cell ở 50°C**, không phải **điện trở ở 200°C** — ép quá tay là làm hỏng cell thật chứ không còn là giả lập nữa. Bắt buộc:
>
> 1. Chạy ở **2–5 W**, đừng kịch trần
> 2. Gá 1 con DS18B20 áp thẳng vào cell bị sưởi, viết **cắt cứng trong firmware ở 60°C** — MOSFET tự ngắt, không phụ thuộc người bấm nút
> 3. Dùng camera nhiệt soi lần chạy đầu tiên để xác nhận nhiệt lan **vào cell** chứ không tỏa hết ra không khí

**A6. Cảnh báo tại chỗ**

| Linh kiện | SL | Ghi chú |
|---|---|---|
| Còi buzzer chủ động 3–24V (SFM-27) | 1 | Kêu được khi mất mạng — khoảnh khắc ăn tiền của demo |
| LED RGB 5mm 4 chân | 3 | |

**A7. Lưu trữ offline**

| Linh kiện | SL | Ghi chú |
|---|---|---|
| Module đọc/ghi thẻ MicroSD giao tiếp SPI | 1 | |
| Thẻ MicroSD 16GB Class 10 | 1 | |

**A8. Đo đối chứng — nên mua**

| Linh kiện | SL | Ghi chú |
|---|---|---|
| **Camera nhiệt gắn điện thoại (InfiRay P2 Pro / FLIR ONE)** ⭐⭐ | 1 | **Đáng tiền nhất danh sách.** Đặt ảnh nhiệt cạnh số liệu DS18B20 trên dashboard = bằng chứng trực quan giám khảo không cãi được. Và giúp chính đội debug xem cảm biến gá đúng chỗ chưa |
| Nhiệt kế hồng ngoại cầm tay | 1 | Đối chứng rẻ hơn. Giám khảo sẽ hỏi "sao biết cảm biến chuẩn?" |
| Đồng hồ vạn năng | 1 | |

**A9. Cơ khí / đấu nối**

| Linh kiện | SL | Ghi chú |
|---|---|---|
| Breadboard + dây jumper | 1 bộ | Chỉ dùng giai đoạn thử |
| **Perfboard (board đục lỗ) + mỏ hàn + thiếc** | 1 bộ | Bản mang đi thi. **Đừng mang breadboard lên sân khấu — dây lỏng là hỏng demo** |

### Bảng B — Bổ sung khi lên xe thật (giai đoạn Chung kết)

| # | Linh kiện | Vai trò | Giá ước tính | Ghi chú |
|---|---|---|---|---|
| 12 | **Buck DC-DC ≥100V in → 5V, Iq thấp** | Lấy nguồn từ pack | 80–200k | **Đọc kỹ Bẫy #1 + #2.** Không mua LM2596HV. |
| 13 | Cầu chì 0,5–1A + đế | An toàn | ~20k | Bắt buộc. |
| 14 | **TP4056 + cell 18650 nhỏ** | Nguồn dự phòng module | ~60k | Xem Bẫy #3. |
| 15 | ~~INA228~~ | — | — | ✅ **Đã chuyển lên Bảng A** — mua ngay từ giai đoạn lab để khỏi làm lại mạch. Thay thế: cảm biến dòng Hall ACS758 nếu muốn cách ly. |
| 16 | Hộp nhựa chống nước IP65 | Vỏ module | 100–200k | Xe máy đi mưa. |
| 17 | Keo tản nhiệt / băng dính nhôm | Gắn cảm biến áp cell | ~50k | Tiếp xúc kém = số đo sai. |
| 18 | *(Gói Pro)* Module **4G LTE Cat-1 A7670C** | Kết nối khi ở xa | 300–500k | 🚨 **PHẢI là 4G.** VN tắt 2G từ 15/9/2026 — SIM800L thành sắt vụn. |
| | | **Tổng bảng B** | **~500k–1,1tr** | |

### Bảng C — Không cần mua

| Thứ | Vì sao |
|---|---|
| ~~Raspberry Pi 4~~ | Đã quyết bỏ khỏi sản phẩm (§1 tài liệu khung). Nếu cần cầu MQTT tạm thì mượn, đừng mua. |
| ~~Pin xe máy điện thật~~ | §3 tài liệu khung — không cần, không an toàn, không kịp. |
| ~~Module SIM800L / SIM900 (2G)~~ | Chết từ 15/9/2026. |

### Thứ tự mua — theo đường tới hạn, không theo tiền

Kinh phí đã xin được nên tiêu chí bây giờ là **thứ gì chặn tiến độ thì mua trước**.

1. **Đặt ngay hôm nay (chặn mọi thứ phía sau):** ESP32-S3-DevKitC-1 N16R8 ×2, DS18B20 ×12, cell LG HG2 ×9, đế pin 4 cell ×2, BMS 8S, INA228, điện trở sưởi + module MOSFET, buzzer, breadboard
2. **Đặt cùng lúc nhưng về muộn cũng không sao:** module SD + thẻ nhớ, camera nhiệt, nhiệt kế hồng ngoại, perfboard, bộ sạc 33,6V
3. **Sau Bán kết:** toàn bộ Bảng B

Mua ở đâu cũng được — không bắt buộc một shop. Các nơi có bán: Điện Tử Đức Huy, Hshop, Nshop, IcDayRoi, Điện Tử Tương Lai, Linh Kiện 3M (chotroihn.vn), Robocon.vn. Camera nhiệt InfiRay/FLIR thì tìm trên Shopee/Lazada hoặc cửa hàng thiết bị đo.

> ⚠️ **Rủi ro tiến độ:** ESP32-S3-DevKitC-1 bản N16R8 và BMS 8S không phải shop nào cũng sẵn hàng. **Gọi điện hỏi tồn kho trước khi đặt**, đừng đợi shop báo hết sau 2 ngày — chỉ còn 11 ngày tới bán kết. Có thể đặt song song ở 2 shop cho chắc.

---

## PHẦN 3 — KIẾN TRÚC HỆ THỐNG

### 3.1 Toàn cảnh

```
╔═══════════════════ TRÊN XE (hoạt động không cần mạng) ═══════════════════╗
║                                                                          ║
║   PACK PIN XE ĐIỆN                                                       ║
║   ┌──────────────────────────┐                                           ║
║   │ [C1][C2][C3][C4]...[C8]  │                                           ║
║   │  T1  T2  T3  T4    T8    │  ← DS18B20 áp sát từng cell               ║
║   └───┬──────────────────┬───┘                                           ║
║       │ 1-Wire (1 chân)  │ nguồn 48–84V                                  ║
║       │                  │                                               ║
║       │            [Cầu chì] → [Buck ≥100V→5V] → [TP4056+18650 dự phòng] ║
║       │                  │                            │                  ║
║       ▼                  ▼                            ▼                  ║
║   ┌─────────────────────────────────────────────────────────┐            ║
║   │                    ESP32-S3                             │            ║
║   │                                                         │            ║
║   │   [Đọc cảm biến]  ──►  [Chuẩn hoá + cửa sổ trượt]        │            ║
║   │                              │                          │            ║
║   │                              ▼                          │            ║
║   │                   ┌──────────────────────┐              │            ║
║   │                   │  AUTOENCODER float32 │  ◄── LỚP 1   │            ║
║   │                   │  16→8→4→8→16, 1,4 KB │              │            ║
║   │                   └──────────┬───────────┘              │            ║
║   │                              │ lỗi tái tạo              │            ║
║   │                              ▼                          │            ║
║   │                   [So ngưỡng percentile 95–99]           │            ║
║   │                     │                    │              │            ║
║   │            vượt ────┘                    └──── bình thường           ║
║   │              ▼                                  ▼       │            ║
║   │       ┌─────────────┐                  ┌──────────────┐ │            ║
║   │       │ CÒI + LED   │                  │ Ghi vào      │ │            ║
║   │       │ (tức thì,   │                  │ flash/SD     │ │            ║
║   │       │  offline)   │                  │ (đệm 30 ngày)│ │            ║
║   │       └─────────────┘                  └──────┬───────┘ │            ║
║   │                                                │        │            ║
║   │       [Quản lý chế độ: sạc 1Hz/chạy 0,2Hz/đỗ 1'/mẫu]    │            ║
║   └────────────────────────────┬───────────────────┬────────┘            ║
║                                │                   │                     ║
╚════════════════════════════════│═══════════════════│═════════════════════╝
                                 │                   │
                    Wi-Fi khi về nhà/trạm      BLE tới điện thoại
                    hoặc 4G (gói Pro)          (gói Basic, khi đang chạy)
                                 │                   │
                                 └─────────┬─────────┘
                                           │ MQTT / TLS
                                           ▼
╔═══════════════════════ WISE-IoT (CLOUD) ═══════════════════════╗
║                                                                ║
║   [IoT Hub]  ──►  [InfluxDB: chuỗi thời gian]                  ║
║       │                    │                                   ║
║       │                    ▼                                   ║
║       │        ┌────────────────────────┐                      ║
║       │        │  LSTM / CNN-LSTM  RUL  │  ◄── LỚP 2           ║
║       │        │  chạy 1 lần/chu kỳ sạc │                      ║
║       │        └───────────┬────────────┘                      ║
║       │                    │                                   ║
║       ▼                    ▼                                   ║
║   ┌────────────────────────────────────────┐                   ║
║   │  DASHBOARD (Grafana)                   │                   ║
║   │  • Nhiệt độ 8 cell real-time           │                   ║
║   │  • Bản đồ nhiệt chênh lệch giữa cell   │                   ║
║   │  • RUL: còn ~N chu kỳ                  │                   ║
║   │  • Lịch sử cảnh báo                    │                   ║
║   └────────────────────────────────────────┘                   ║
╚════════════════════════════════════════════════════════════════╝
```

### 3.2 Nguyên tắc thiết kế — 3 câu để nhớ

1. **Mọi thứ liên quan đến AN TOÀN nằm trên ESP32 và chạy offline.** Mất mạng, mất cloud, mất điện thoại — còi vẫn kêu.
2. **Mọi thứ liên quan đến KINH TẾ (RUL, dashboard, báo cáo) nằm trên WISE-IoT.** Chậm vài giờ không sao.
3. **Mạng là tuỳ chọn, không phải điều kiện.** Có mạng thì tốt hơn, không có thì vẫn hoạt động.

Ba câu này vừa là kiến trúc, vừa là bài thuyết trình.

### 3.3 Chi tiết đấu nối

> ⚠️ **Bảng dưới đã đổi sang sơ đồ chân ESP32-S3.** Chân của S3 KHÁC ESP32 đời đầu — đừng dùng lại GPIO21/22 như tài liệu ESP32 cũ.

| Kết nối | Giao thức | Chân ESP32-S3 (gợi ý) | Ghi chú |
|---|---|---|---|
| 8 × DS18B20 | 1-Wire | GPIO4 | Tất cả chung 1 bus + pull-up 4,7 kΩ. **Không dùng chế độ parasitic power** — pack pin gần motor controller nên nhiễu điện từ rất mạnh, parasitic hay lỗi CRC. Đấu đủ 3 dây (VDD/GND/DQ). |
| INA228 (hoặc INA226) | I2C | **GPIO8 (SDA), GPIO9 (SCL)** | Mặc định của Arduino-ESP32 trên S3. Địa chỉ 0x40 |
| Điện áp pack | ADC | GPIO1–GPIO10 (ADC1) | Qua chia áp điện trở, nhớ tính hệ số. **Dùng ADC1**, ADC2 xung đột với Wi-Fi |
| Buzzer + LED RGB | GPIO out | GPIO5, 6, 7 | |
| MOSFET điều khiển điện trở sưởi | GPIO out | GPIO10 | Để ESP32 tự chạy kịch bản demo |
| Thẻ SD | SPI | GPIO11(MOSI), 12(SCK), 13(MISO), 14(CS) | Chỉ giai đoạn thí nghiệm |
| Module 4G *(Pro)* | UART | GPIO17(TX), 18(RX) | Chỉ gói Pro |

🚫 **Chân KHÔNG được dùng trên S3:** GPIO0, 45, 46 (strapping pin — kéo sai mức là không boot được); GPIO19, 20 (USB D-/D+); GPIO26–32 (nối SPI flash/PSRAM nội bộ, dùng là chết board).

**Lưu ý thời gian đọc DS18B20:** ở độ phân giải 12 bit, mỗi lần chuyển đổi mất **750 ms**. Dùng lệnh `Skip ROM + Convert T` để **tất cả cảm biến chuyển đổi song song**, sau đó đọc lần lượt → tổng vẫn ~750ms cho cả 8 con. Đủ cho 1 Hz nhưng khá sát. Nếu cần nhanh hơn, hạ xuống **10 bit (187 ms)** — độ phân giải 0,25°C vẫn thừa cho bài toán này.

### 3.4 Luồng dữ liệu — số liệu cụ thể

| Chặng | Nội dung | Tần suất | Dung lượng |
|---|---|---|---|
| Cảm biến → ESP32 | 8 nhiệt độ + dòng + áp | Theo chế độ | — |
| ESP32 nội bộ | Cửa sổ trượt → vector 16 chiều → Autoencoder | 1 Hz | Mô hình 1,4 KB (+5,5 KB flash, +2,9 KB RAM) |
| ESP32 → flash | Bản ghi nén | Mỗi mẫu | ~20 byte/mẫu, **~115 KB/ngày** khi đỗ |
| ESP32 → WISE-IoT | Tóm tắt + cảnh báo | 30s–15 phút tuỳ chế độ | vài trăm byte/lần |
| ESP32 → WISE-IoT | Tóm tắt chu kỳ sạc (cho RUL) | 1 lần/chu kỳ sạc | ~10–20 số |

---

## PHẦN 4 — VIỆC CẦN LÀM

**Trước khi đặt mua** — ✅ đã chốt 04/09/2026
- [x] ~~Pack lab bao nhiêu cell~~ → **8S**, đầy 33,6V
- [x] ~~Dùng INA nào~~ → **INA228** (INA226 dự phòng). INA219 loại vì chỉ chịu 26V
- [x] ~~Bao nhiêu DS18B20~~ → **12 con**: 8 đo cell, 2 đo vỏ pack/môi trường, 2 dự phòng
- [x] ~~Chip nào~~ → **ESP32-S3** (có lệnh vector cho ML), không phải ESP32 đời đầu
- [ ] Gọi shop xác nhận còn hàng ESP32-S3 N16R8 và BMS 8S **trước khi** đặt

**Sau khi linh kiện về**
- [ ] Gán lại toàn bộ chân theo sơ đồ S3 (§3.3) — **không copy pinout ESP32 cũ**
- [ ] Đọc được 8 DS18B20 trên 1 bus, ổn định, không lỗi CRC
- [ ] Đối chứng với camera nhiệt + nhiệt kế hồng ngoại → ghi lại sai số từng con
- [ ] **Đo baseline: 8 cell nghỉ cùng điều kiện, chênh lệch nhiệt độ giữa các cell là bao nhiêu?** Đây là sàn nhiễu — mọi dị thường phải vượt trên nó mới có nghĩa
- [ ] Kiểm tra gradient theo vị trí: cell giữa có nóng hơn cell rìa không, chênh bao nhiêu
- [ ] Đo dòng tiêu thụ thật của module ở từng chế độ (số này lên slide)
- [ ] Thử điện trở sưởi qua MOSFET: bao lâu thì cell lên 45°C, chênh với cell khác bao nhiêu
- [ ] **Viết cắt cứng firmware: cell bị sưởi chạm 60°C → MOSFET ngắt ngay.** Làm cái này TRƯỚC lần sưởi đầu tiên, không phải sau
- [x] ~~Xác nhận TFLite Micro build được cho S3~~ → **KHÔNG DÙNG TFLite Micro nữa.** Mô hình 356 tham số nên firmware tự nhân tay, 30 dòng C, không phụ thuộc thư viện. Xem `docs/DECISION_LOG.md` QĐ-013

**Chốt trước Bán kết**
- [ ] Sơ đồ kiến trúc này vẽ lại thành 1 slide sạch đẹp
- [ ] Chuẩn bị trả lời: *"thiết bị có làm hết pin xe không?"* → đưa số đo thật + nói về Bẫy #2 và #3

---

## Nguồn

- [Datasheet DS18B20 (Analog Devices)](https://analog.com/media/en/technical-documentation/data-sheets/ds18b20.pdf)
- [ESP32 với nhiều cảm biến DS18B20 — Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-multiple-ds18b20-temperature-sensors/)
- [Hướng dẫn chống nhiễu cho bus 1-Wire](https://industrialmonitordirect.com/blogs/knowledgebase/ds18b20-noise-immunity-1-wire-wiring-pull-ups-and-emi-field)
- [Module buck LM2596HV 5–60V — lưu ý giới hạn 60V](https://chotroihn.vn/module-buck-dc-dc-lm2596hvs-in-5-60v-3a)
- [Mạch buck hạ áp 12–80V vào, có chỉnh dòng (Robocon.vn)](https://robocon.vn/detail/mdl454-mach-buck-ha-ap-dc-dc-72v-60v-48v-sang-36v-24v-19v-12v-20a-600w-co-chinh-dong-vo-kim-loai.html)
- [Module 4G LTE Cat.1 A7670C bán tại VN](https://caka.vn/module-sim-4g-lte-cat-1-a7670c-simcom-ho-tro-iot-giao-tiep-uart)
- [VNPT — 2G ngừng hoạt động 15/9/2026](https://vnpt.vn/gioi-thieu/tin-tuc/15-9-2026-he-thong-2g-se-ngung-hoat-dong-tai-viet-nam.html)

*Giá tham khảo tháng 9/2026, cần kiểm tra lại tại cửa hàng — giá linh kiện VN biến động nhiều.*
