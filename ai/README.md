# Phần AI — hai lớp

| Lớp | Chạy ở đâu | Việc | Trạng thái |
|---|---|---|---|
| **1 — Bất thường** | ESP32 (1,4 KB) | Cell nào đang cư xử lạ | ✅ chạy trên board thật |
| **2 — RUL/SOH** | Cloud | Pin còn dùng được bao lâu | ✅ mô hình xong, chưa nối luồng |

---

# Lớp 1 — Phát hiện cell bất thường (on-device)

Autoencoder chạy thẳng trên ESP32-S3, phát hiện cell nào trong pack đang cư xử
bất thường về nhiệt. **1,4 KB float32**, chạy 8 lần mỗi giây.

Bối cảnh và lý do chọn hướng này: `VEDCaPhenika/KHUNG_NGHIEN_CUU_v2_HuTieu.md` §4, §8.

---

## Ý tưởng cốt lõi

Bộ dữ liệu có pack **36 cell**, phần cứng của đội có **8 cell**. Nếu mô hình
nhận thẳng "nhiệt độ 36 cell" làm đầu vào thì train xong không deploy được.

Nên mô hình **không nhìn cả pack** — nó nhìn **từng cell một**, qua một vector
16 đặc trưng mô tả *cell này lệch khỏi phần còn lại của pack như thế nào*.

Hệ quả:
- Train trên pack 36 cell, chạy trên pack 8 cell, không sửa gì
- Mô hình chỉ ra **đúng cell nào** bất thường, không chỉ nói "pack có vấn đề"
- Chỉ 356 tham số → 1,4 KB float32, firmware tự nhân, không cần TFLite Micro

Và dữ liệu được ép về đúng phần cứng thật trước khi train: gộp Top+Bottom
thành 1 cảm biến/cell (đội dùng 8× DS18B20), hạ 2,5 Hz xuống 1 Hz, lượng tử
hoá về bước 0,0625 °C, cắt pack 36 thành các pack ảo 8 cell liền kề.

## Dữ liệu

**UPC WLTP Pack Cycling** — DOI [10.34810/data2395](https://doi.org/10.34810/data2395),
CC BY 4.0, 412 chu kỳ, 1,4 GB, pack 3P12S với **72 kênh nhiệt độ cell**.

Chia theo **chu kỳ** (287 train / 61 val / 62 test), không chia ngẫu nhiên theo
dòng — chuỗi thời gian mà chia ngẫu nhiên thì mẫu train và test cách nhau 1
giây, mô hình chỉ việc nội suy và điểm test đẹp giả tạo.

⚠️ Kênh **P3S11 hỏng** trong bộ gốc (2,74% số mẫu, giá trị nhảy về 0 hoặc bão
hoà 655,35 = 0xFFFF/100). Pipeline tự phát hiện và loại kênh hỏng theo từng
chu kỳ. Không lọc thì autoencoder học luôn lỗi cảm biến và coi đó là bình thường.

## Kết quả

Điểm vận hành: ngưỡng p99.9 = 1,8474, phải vượt liên tục **60 giây**.

**Ca an toàn quan trọng nhất — ramp (tiền đề thermal runaway), độ trễ phát hiện:**

| Tốc độ tăng nhiệt | Autoencoder | Ngưỡng cứng 60°C | Lệch TB > 5°C |
|---|---|---|---|
| 0,5 °C/phút | **6 phút** | 62 phút | 12 phút |
| 0,2 °C/phút | **20 phút** | 156 phút | 25 phút |
| 0,1 °C/phút | **46 phút** | 313 phút | 47 phút |
| 0,05 °C/phút | **91 phút** | không bao giờ | 97 phút |

Cả ba đều bắt được, nhưng AE **nhanh hơn ngưỡng cứng 10–14 lần**. Với pin thì
chênh lệch đó là khoảng cách giữa cảnh báo và đám cháy.

**Báo động giả:** 0 lần trên 4,8 triệu mẫu-cell dữ liệu bình thường. Vì không
quan sát được lần nào nên chỉ kết luận được chặn trên (quy tắc số 3): **dưới
0,018 lần/giờ**, tức hiếm hơn 1 lần mỗi 2,3 ngày.

**Chỗ Autoencoder KHÔNG thắng — nói thẳng ra:**
- *Drift chậm* (trôi 0,5–3 °C suốt cả chu kỳ): chỉ bắt được 38–67%, ngang
  hoặc thua Isolation Forest. Nguyên nhân nằm ngay trong thiết kế: đặc trưng
  `dev_ema` bám theo trôi chậm nên chính nó nuốt mất tín hiệu. Đây là **đánh
  đổi có chủ ý** để chống báo động giả, không phải lỗi.
- *Offset nhỏ* (< 1 °C): 33–58%. Dưới mức nhiễu tự nhiên giữa các cell.

→ Kết luận trung thực: AE thắng rõ ở **ca nguy hiểm (ramp)** và offset vừa/lớn,
nhưng **không thay thế được ngưỡng cứng 60 °C** — phải chạy song song cả hai.

## Chạy lại

```bash
python3 -m venv ai/.venv && ai/.venv/bin/pip install numpy pandas pyarrow scikit-learn scipy tensorflow

ai/.venv/bin/python ai/download_upc.py       # 1,4 GB, có thể chạy lại nếu đứt
ai/.venv/bin/python ai/test_features.py      # test đặc trưng, ~5 giây
ai/.venv/bin/python ai/prepare_data.py       # ~8 phút
ai/.venv/bin/python ai/train_autoencoder.py  # ~1 phút trên CPU
ai/.venv/bin/python ai/evaluate.py           # AE đấu 3 baseline, ~7 phút
ai/.venv/bin/python ai/tune_threshold.py     # chọn điểm vận hành, ~7 phút
ai/.venv/bin/python ai/export_tflite.py      # -> models/cell_ae_model.h
```

**Không cần GPU.** Mô hình 356 tham số, 1 giây/epoch trên CPU laptop. Nút thắt
là RAM và đọc parquet, không phải phép nhân ma trận.

## File

| File | Vai trò |
|---|---|
| `features.py` | 16 đặc trưng — **đây là thứ firmware phải viết lại bằng C** |
| `test_features.py` | Test đặc trưng, gồm cả kiểm tra tính nhân quả |
| `download_upc.py` | Tải dataset từ Dataverse |
| `prepare_data.py` | Parquet → tập train/val/test, giả lập phần cứng |
| `train_autoencoder.py` | Train 16→8→4→8→16 |
| `evaluate.py` | AE đấu 3 baseline, tiêm lỗi tổng hợp |
| `tune_threshold.py` | Dò ngưỡng × thời gian giữ |
| `export_c_model.py` | → **`cell_ae_weights.h`** cho firmware (float32) |
| `export_tflite.py` | → INT8 .tflite. *Không dùng nữa*, giữ làm dự phòng |
| `test_c_vs_python.py` | **So bản C với bản Python từng số** |
| `models/cell_ae_model.h` | **File firmware `#include`** |

## Việc còn lại

- [x] ~~Viết lại `features.py` bằng C~~ → xong 11/09, `cell_ai.cpp`, khớp Python <1e-6
- [x] ~~Nhúng model vào sketch~~ → xong 11/09, **không dùng TFLite Micro** (QĐ-013)
- [ ] Thay 8 giá trị nhiệt giả lập bằng 8 con DS18B20 thật
- [ ] Kiểm chứng trên **Stanford/Warwick** — mất cân bằng THẬT, có nhãn
      (Mendeley `zh58byr53c`). Đây là bằng chứng mạnh nhất để lên slide,
      mạnh hơn nhiều so với lỗi tự tiêm.
- [ ] Lớp 2 (RUL/SOH) trên NASA PCoE — chạy ở cloud, chưa bắt đầu


---

# Lớp 2 — Dự báo tuổi thọ (RUL/SOH), chạy trên cloud

Dự báo **số chu kỳ còn lại tới khi pin còn 80% dung lượng**, 1 lần mỗi chu kỳ sạc.
Dữ liệu: NASA PCoE, 4 pin, 364 chu kỳ. Chi tiết: `docs/BANG_CHUNG_LOP2_RUL_2026-09-11.md`.

**Chỉ dùng đặc trưng PHA SẠC** — không dùng dung lượng phóng, vì (a) nhãn RUL
tính từ dung lượng nên đó là rò rỉ đáp án, (b) đo dung lượng thật cần phóng
kiệt pin, việc không bao giờ xảy ra với xe đang chạy. Câu chuyện sản phẩm:
*"đoán tuổi thọ pin từ cách nó sạc"*.

## Kết quả — mô hình đơn giản thắng

MAE (số chu kỳ), leave-one-battery-out:

| Phương pháp | TB | Ghi chú |
|---|---|---|
| **Tuyến tính, chỉ `t_cv`** | **12,2** | 2 tham số |
| Gradient Boosting | 17,2 | |
| LSTM cửa sổ 10 | 20,3 | ~3.500 tham số |
| Ridge 9 đặc trưng | 27,0 | tệ hơn cả baseline |
| Baseline đoán trung bình | 25,8 | |

Lý do: 4 pin có tuổi thọ 60/77/105/123. B0006 (60, ngắn hơn mọi pin train) là
chỗ mọi mô hình phức tạp sụp — LSTM sai 42,8 chu kỳ, tuyến tính sai 6,4. LSTM
học thuộc tuổi thọ 3 viên pin được xem, không học quy luật.

**SOH: RMSE 3,8 điểm phần trăm** — đáng tin hơn RUL, và là con số khách hàng
định giá xe cũ thật sự cần. Nên dẫn slide bằng SOH.

## Chạy

```bash
ai/.venv/bin/python ai/nasa_prepare.py   # .mat -> bảng đặc trưng theo chu kỳ
ai/.venv/bin/python ai/train_rul.py      # 5 phương pháp, leave-one-battery-out
```

Tải dữ liệu: `https://phm-datasets.s3.amazonaws.com/NASA/5.+Battery+Data+Set.zip`
(210 MB, giải nén hai lớp zip).

## Việc còn lại

- [ ] Kiểm chéo trên bộ thứ hai (UPC đã tải sẵn, 410 chu kỳ có suy giảm dung lượng)
- [ ] Nối luồng thật: ESP32 gửi ~10 con số tóm tắt sau mỗi chu kỳ sạc → cloud tính RUL
- [ ] Panel RUL/SOH trên Grafana
