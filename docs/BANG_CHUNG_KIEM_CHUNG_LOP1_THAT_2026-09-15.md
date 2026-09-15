# Bằng chứng: Lớp 1 trên mất cân bằng THẬT — **kết quả âm tính**

15/09/2026. Liên quan: QĐ-032, AC-06.8. Mã: `ai/validate_mcmaster.py`.

> **Kết luận ngắn: Lớp 1 KHÔNG chuyển giao được sang pack khác mà không hiệu
> chỉnh lại.** Đây là kết quả xấu, và nó được ghi ở đây nguyên vẹn vì biết
> trước khi lên sân khấu tốt hơn nhiều so với bị giám khảo tìm ra hộ.

## Vì sao làm

Cho tới hôm nay, **mọi** ca lỗi của Lớp 1 đều do chính đội tiêm vào dữ liệu
bình thường (`evaluate.py`, `sensor_count_study.py`). Cách đó kiểm được rằng mô
hình nhạy với thứ *mình nghĩ là lỗi* — nhưng không kiểm được rằng lỗi thật
trông giống thứ mình tưởng tượng.

## Bộ dữ liệu

M. Naguib, P. Kollmeyer, J. Chen, A. Emadi, *"Battery Pack with Introduced
Faults Dataset – Air Cooled SBLimotive 5Ah"*, Borealis Data, 2025, CC-BY 4.0,
[doi:10.5683/SP3/THZTJC](https://borealisdata.ca/dataset.xhtml?persistentId=doi:10.5683/SP3/THZTJC).

Pack **thật 72 cell** nối tiếp lấy từ xe hybrid, đo qua Orion BMS ở **1 Hz**
(đúng nhịp hệ mình), buồng nhiệt 15/25 °C. Lỗi **gây ra cố ý**: chặn dòng khí,
chặn một module, tắt quạt. Nhãn nằm ngay trong tên file.

Bộ này được chọn vì kiểm được **cả hai chiều**: `ModuleBlocked` tạo chênh lệch
không gian (Lớp 1 *phải thấy*), còn `Fanoffon` tắt quạt cả pack (theo TH-4 thì
Lớp 1 *phải mù*). Bộ chỉ kiểm chiều thuận thì dễ tự lừa mình.

Đã **loại** bộ Zenodo 46Ah Kokam vì là đơn cell — không có chênh lệch giữa cell
thì không có gì cho Lớp 1 nhìn.

## Cách kiểm: zero-shot, không nới tay

Mô hình **không huấn luyện lại**, ngưỡng **không chỉnh**. Đúng `cell_ae.keras`
và đúng p99.9 = 2,01459 đang nạp trong firmware.

## KQ-01 — Chạy thẳng: hỏng hoàn toàn

| file | loại | % pack-ảo báo | % thời gian |
|---|---|---|---|
| Fanoffon_15C_Long | lỗi toàn cục | 100 % | 99,9 % |
| HWFET_15C_ModuleBlocked_Long | lỗi cục bộ | 100 % | 97,7 % |
| US06_15C | **bình thường** | 100 % | **98,7 %** |
| HWFET_15C | **bình thường** | 100 % | **90,7 %** |
| LA92_15C | **bình thường** | 100 % | **87,0 %** |
| UDDS_Blocked_25C | lỗi cục bộ | 100 % | 12,4 % |
| UDDS_25C | bình thường | 100 % | 0,4 % |

**Mô hình báo động trên mọi thứ.** File bình thường US06_15C bị báo 98,7 % thời
gian — cao hơn cả file lỗi UDDS_Blocked (12,4 %). Không có khả năng phân biệt
nào ở đây.

Điều đáng chú ý: tỉ lệ báo động bám theo **mức khắc nghiệt của chu trình lái**
(US06 > LA92 > UDDS) và theo **nhiệt độ buồng** (15 °C ≫ 25 °C), chứ **không**
bám theo nhãn lỗi. Đó là dấu vân tay của lệch phân bố, không phải của phát hiện.

## KQ-02 — Chẩn đoán: bốn đặc trưng bối cảnh toàn pack là thủ phạm

Đo độ lệch trung bình (đơn vị độ lệch chuẩn của tập huấn luyện):

| Đặc trưng | UDDS_25C | US06_15C | Loại |
|---|---|---|---|
| `packT - amb` (số 5) | 2,4 | **8,6** | bối cảnh toàn pack |
| `\|I\|/I_SCALE` (số 13) | 2,7 | **6,6** | bối cảnh toàn pack |
| `T - amb` (số 4) | 1,2 | **4,5** | bối cảnh toàn pack |
| `soc` (số 14) | 2,4 | 2,4 | bối cảnh toàn pack |
| `dev`, `z`, `rank`, `dev_ema`, `roll_std` | ≤1,2 | ≤1,6 | **tương đối giữa cell** |

Đây là phát hiện có giá trị nhất của cả nghiên cứu:

- **Các đặc trưng tương đối giữa cell chuyển giao TỐT** — vẫn nằm trong ~1,6 sd
  dù là pack khác hẳn, hoá học khác, thiết kế tản nhiệt khác.
- **Bốn đặc trưng bối cảnh toàn pack thì không.** Và vì chúng **giống hệt nhau
  ở mọi cell**, khi lệch phân bố chúng đẩy sai số tái tạo của **tất cả** các
  cell lên cùng lúc. Đó chính là cơ chế làm 100 % pack-ảo báo động.

## KQ-03 — Vô hiệu hoá bốn đặc trưng đó: đỡ hơn, vẫn hỏng

Ép bốn đặc trưng bối cảnh về đúng trung bình tập huấn luyện:

| file | loại | % pack-ảo báo | % thời gian |
|---|---|---|---|
| UDDS_25C | bình thường | **0 %** | 0,000 % |
| LA92_25C | bình thường | **0 %** | 0,000 % |
| HWFET_25C | bình thường | **0 %** | 0,000 % |
| US06_25C | bình thường | **0 %** | 0,000 % |
| **UDDS_Blocked_25C** | **lỗi cục bộ** | **0 %** ❌ | 0,000 % |
| UDDS_15C | bình thường | 100 % | 0,793 % |
| LA92_15C | bình thường | 100 % | 2,398 % |
| HWFET_15C_ModuleBlocked_Long | lỗi cục bộ | 89 % | 3,648 % |
| Fanoffon_15C_Long | lỗi toàn cục | 100 % | 8,433 % |

Đọc bảng này cho đúng:

- **Ở 25 °C: báo oan về đúng 0 %** trên cả bốn file bình thường. Rất tốt.
- **Nhưng cũng bỏ sót hoàn toàn lỗi thật `UDDS_Blocked_25C`.** Im lặng 0 %. Đây
  là một ca **trượt**, không phải một ca "thận trọng".
- **Ở 15 °C: mọi file đều báo, có lỗi hay không.** Không phân biệt được.

Nên phép sửa này **không cứu được kết quả**. Nó chỉ chỉ ra đúng chỗ hỏng.

## KQ-04 — Một dự đoán của chính mình đã sai

`docs/HAI_LOP_AI_HOAT_DONG_THE_NAO.md` TH-4 nói: pack nóng đều thì Lớp 1 **mù**.
Thực tế `Fanoffon` (tắt quạt cả pack) lại là file bị báo **nhiều nhất** (8,4 %).

Giải thích hợp lý: "Fanoffon" là **tắt rồi bật lại quạt nhiều lần**, chứ không
phải tắt hẳn. Mỗi lần bật, cell gần cửa gió nguội nhanh hơn cell ở xa — tạo ra
chênh lệch không gian thật. Vậy nó không phải lỗi "toàn cục" như tên gọi gợi ý.
Dự đoán TH-4 chưa bị bác bỏ, nhưng **file này không kiểm được TH-4**, và tài
liệu đã được sửa lại cho đúng.

## Vì sao trượt — hai giả thuyết, chưa cái nào được chứng minh

1. **Độ phân giải 1 °C.** Orion BMS làm tròn tới 1 °C. `UDDS_Blocked` chỉ làm
   độ rộng nhiệt tăng từ 3 °C lên 4 °C — tức là tín hiệu lỗi **bằng đúng một
   bước lượng tử**. DS18B20 của đội là 0,0625 °C, mịn gấp 16 lần. Đây là lý do
   dễ tin nhất, và nếu đúng thì nó **ủng hộ** lựa chọn DS18B20 — nhưng nó vẫn
   là giả thuyết, chưa kiểm.
2. **Ngưỡng và bộ chuẩn hoá thuộc về pack NASA**, không thuộc pack này.

Chưa tách được hai nguyên nhân này. Đừng nói với giám khảo là đã biết.

## Kết luận — điều được phép nói và không được phép nói

**KHÔNG được nói:** *"AI của chúng em phát hiện được bất thường nhiệt trên pack
pin nói chung."* Nghiên cứu này bác bỏ câu đó.

**Được nói:** *"Chúng em kiểm trên một bộ dữ liệu pack thật độc lập với lỗi gây
ra cố ý. Mô hình không chuyển giao được zero-shot: nó phải được hiệu chỉnh trên
chính pack sẽ giám sát. Chúng em đã tìm ra lý do — bốn đặc trưng bối cảnh toàn
pack lệch phân bố, trong khi các đặc trưng tương đối giữa cell thì chuyển giao
tốt. Đó là một sửa đổi thiết kế cụ thể, không phải một lời bào chữa."*

Với bài dự thi thì giới hạn này **không chặn đường**: hệ giám sát chính pack của
đội, và ngưỡng được hiệu chỉnh trên chính pack đó. Nhưng lời quảng cáo phải
khớp với thứ đã chứng minh được.

## Việc phải làm tiếp

| Việc | Vì sao |
|---|---|
| Chuẩn hoá 4 đặc trưng bối cảnh theo **dải vận hành của chính pack đó** thay vì giá trị tuyệt đối | Sửa đúng nguyên nhân đã tìm ra ở KQ-02 |
| Kiểm giả thuyết 1 °C: hạ dữ liệu NASA xuống bước 1 °C rồi chạy lại `evaluate.py` | Tách được hai nguyên nhân; làm được ngay, không cần phần cứng |
| Hiệu chỉnh ngưỡng trên chính pack của đội sau khi dán cảm biến | Điều kiện để mọi con số Lớp 1 có nghĩa |

## Tái lập

```bash
ai/.venv/bin/python ai/validate_mcmaster.py
```
In cả ba cấu hình A (chuẩn hoá I_SCALE), B (đối chứng), C (vô hiệu hoá đặc
trưng bối cảnh). Dữ liệu tải từ Borealis, CC-BY 4.0 — **không** commit vào repo
(1,3 GB); `ai/data/` đã nằm trong `.gitignore`.
