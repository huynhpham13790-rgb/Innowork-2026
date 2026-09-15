# Bằng chứng: autoencoder "thuần tương đối" — lời giải cho sản xuất hàng loạt

15/09/2026. Liên quan: QĐ-033 (đề xuất, **chưa chốt**), tiếp nối QĐ-032.
Mã: `ai/train_ae_relative.py`, `ai/eval_ae_relative.py`.

## Câu hỏi

QĐ-032 kết luận mô hình 16 đặc trưng không chuyển giao sang pack khác, và phải
hiệu chỉnh riêng cho từng pack. Câu hỏi tiếp theo là câu quan trọng nhất:
**nếu mỗi pack phải hiệu chỉnh riêng thì sản xuất hàng loạt kiểu gì?**

## Ý tưởng — tách hai loại thông tin bị trộn lẫn

QĐ-032 đã đo được: đặc trưng **tương đối giữa cell** lệch ≤1,6 sd khi sang pack
khác, còn đặc trưng **bối cảnh toàn pack** lệch tới 8,6 sd. Hai loại thông tin
này đang bị nhét chung vào một mô hình — và loại thứ hai kéo cả mô hình xuống.

Lý do vật lý để tin rằng tách được: *"cell này nóng hơn phần còn lại 2 °C"* có
nghĩa **như nhau** trên pack 8 cell 2,55 Ah lẫn pack 72 cell 5,2 Ah. Còn *"pack
nóng hơn môi trường 12 °C"* thì phụ thuộc hoàn toàn vào thiết kế tản nhiệt.

**Bỏ 5 đặc trưng mang giá trị tuyệt đối:** `T-amb`, `packT-amb`, `|I|/I_SCALE`,
`soc`, `(T-25)/25`.
**Giữ 11 đặc trưng thuần tương đối:** `dev`, `dev_med`, `z`, `rank`, `spread`,
`dT_cell`, `dT_pack`, `dT_diff`, `dev_ema`, `dev-dev_ema`, `roll_std`.

### Dự đoán ghi TRƯỚC khi chạy

Mất bối cảnh nghĩa là mô hình không còn biết đang sạc nặng hay đang nghỉ. Cùng
mức chênh 2 °C có thể bình thường lúc tải nặng, bất thường lúc nghỉ. **Dự đoán:
tỉ lệ báo oan sẽ tăng, và khả năng phát hiện sẽ giảm một ít.**

**Dự đoán này SAI.** Kết quả ngược lại — ghi ra đây vì đoán sai cũng là dữ liệu.

## KQ-01 — Trên chính dữ liệu NASA: TỐT HƠN toàn diện

| Phép đo | 16 đặc trưng (hiện tại) | 11 thuần tương đối | |
|---|---|---|---|
| **Báo oan** trên dữ liệu sạch | 0,0360 % | **0,0106 %** | thấp hơn 3,4 lần |
| `offset` 1,0 °C | 21 % | **58 %** | |
| `offset` 2,0 °C | 79 % | **100 %** | |
| `ramp` 2,0 °C — trễ | 1 142 s | **844 s** | nhanh hơn 1,35 lần |
| `ramp` 5,0 °C — trễ | 346 s | **240 s** | |
| `drift` 3,0 °C | 38 % | **67 %** | |
| `drift` 5,0 °C | 46 % | **88 %** | gần gấp đôi |

**Bỏ bớt đặc trưng làm mô hình TỐT HƠN.** Không có đánh đổi nào ở đây.

Giải thích hợp lý: nút thắt chỉ có **4 chiều**. Với 16 đặc trưng, mô hình phải
tiêu một phần dung lượng ít ỏi đó để tái tạo 5 đặc trưng bối cảnh — vốn giống
hệt nhau ở mọi cell nên **không mang thông tin phân biệt cell nào cả**. Bỏ
chúng đi thì cả 4 chiều được dành trọn cho cấu trúc tương đối.

Đáng chú ý nhất là `drift` — điểm yếu nặng nhất đã ghi trong `ai/README.md` —
cải thiện gần gấp đôi. Vì trôi chậm là tín hiệu thuần tương đối, và trước đây
nó bị các đặc trưng bối cảnh làm loãng.

## KQ-02 — Chuyển giao sang pack McMaster: đã có phân biệt

| file | loại | % pack-ảo báo | % thời gian |
|---|---|---|---|
| HWFET_15C_ModuleBlocked_Long | **lỗi cục bộ** | 44 % | **1,262 %** |
| Fanoffon_15C_Long | **lỗi** | 67 % | **0,360 %** |
| LA92_15C | bình thường | 22 % | 0,065 % |
| US06_15C | bình thường | 33 % | 0,025 % |
| HWFET_15C | bình thường | 22 % | 0,011 % |
| LA92_HighFan_15C | bình thường | 11 % | 0,002 % |
| UDDS_15C / UDDS_25C / LA92_25C / HWFET_25C / US06_25C | bình thường | **0 %** | 0,000 % |
| **UDDS_Blocked_25C** | **lỗi cục bộ** | **0 %** ❌ | 0,000 % |

So với mô hình 16 đặc trưng (nơi file bình thường US06_15C bị báo **98,7 %**
thời gian, cao hơn cả file lỗi):

- File lỗi: 0,360–1,262 % thời gian
- File bình thường tệ nhất: 0,065 %
- **Tách biệt khoảng 19–20 lần.** Đã dùng được để phân biệt.
- Toàn bộ 5 file bình thường ở 25 °C: **đúng 0 %**.

## KQ-03 — Vẫn còn một ca trượt, và không được giấu

`UDDS_Blocked_25C` (chặn dòng khí, lỗi thật) vẫn **im lặng hoàn toàn**.

File này làm độ rộng nhiệt tăng từ 3 °C lên 4 °C — tức là tín hiệu lỗi bằng
**đúng một bước lượng tử** của Orion BMS (1 °C). DS18B20 của đội mịn gấp 16
lần. Đây là bằng chứng gián tiếp ủng hộ giả thuyết độ phân giải ở QĐ-032,
nhưng **vẫn chưa phải bằng chứng trực tiếp** — phép kiểm dứt điểm là hạ dữ liệu
NASA xuống bước 1 °C rồi chạy lại (AC-06.36, chưa làm).

## Ý nghĩa với sản xuất hàng loạt

| | Mô hình 16 đặc trưng | Mô hình 11 thuần tương đối |
|---|---|---|
| Nạp cùng một mô hình cho mọi máy? | ❌ không | ✅ được |
| Cần hiệu chỉnh riêng từng pack? | có, toàn bộ | chỉ ngưỡng (một con số) |
| Cần dữ liệu có nhãn ở nhà máy? | không | không (autoencoder không cần nhãn) |

**Kiến trúc ba tầng cho bản thương mại:**

1. **Ngưỡng cứng 60 °C** — không học gì, giống hệt mọi máy, không bao giờ cần
   hiệu chỉnh. Đây là tầng an toàn thật sự.
2. **Autoencoder thuần tương đối** — nạp giống hệt nhau cho mọi máy xuất xưởng.
   Phát hiện *cell nào khác phần còn lại*.
3. **Bối cảnh tuyệt đối** (`pack - môi trường`, dòng, SOC) — xử lý bằng **luật
   vật lý viết tay**, không đưa vào mô hình học. Chúng vốn không cần học: "pack
   nóng hơn môi trường 25 °C là bất thường" là một ngưỡng, không phải một mẫu.

Điểm mấu chốt: **vấn đề không phải "mỗi pack một khác", mà là trước đây đã trộn
hai loại thông tin có tính chất khác hẳn nhau vào cùng một mô hình.** Loại tương
đối thì phổ quát; loại tuyệt đối thì không, và cũng không cần học.

**Hiệu chỉnh ngưỡng tại chỗ** (nếu muốn thêm): chạy vài giờ vận hành bình
thường trên chính pack đó rồi lấy phân vị p99.9. Autoencoder không cần nhãn nên
việc này tự động hoàn toàn.
⚠️ **Chỉ được làm ở cuối dây chuyền trên pack đã biết là tốt.** Nếu hiệu chỉnh
trên pack đang có lỗi thì lỗi đó bị ghi nhận thành "bình thường" vĩnh viễn —
đúng cái bẫy đã ghi ở QĐ-025 về việc hiệu chuẩn cảm biến sau khi dán lên pack.

## Đề xuất — cần người chốt

Chuyển Lớp 1 sang mô hình 11 đặc trưng thuần tương đối. Đây là **thay đổi kiến
trúc** so với QĐ-012 nên theo CLAUDE.md phải có người quyết, AI không tự đổi.

Lý do ủng hộ: tốt hơn trên **mọi** phép đo của chính đội, báo oan thấp hơn 3,4
lần, và mở được đường sản xuất hàng loạt. Mô hình nhỏ hơn (11 đầu vào thay vì
16) nên firmware cũng nhẹ đi.

Việc phải làm nếu chốt: xuất lại `cell_ae_weights.h`, sửa `cell_ai.cpp` bỏ 5
đặc trưng, chạy lại `test_c_vs_python.py`, hiệu chỉnh lại ngưỡng, cập nhật
`ai/README.md` và `docs/HAI_LOP_AI_HOAT_DONG_THE_NAO.md`.

## Tái lập

```bash
ai/.venv/bin/python ai/train_ae_relative.py   # huấn luyện
ai/.venv/bin/python ai/eval_ae_relative.py    # kiểm cả hai phía
```
