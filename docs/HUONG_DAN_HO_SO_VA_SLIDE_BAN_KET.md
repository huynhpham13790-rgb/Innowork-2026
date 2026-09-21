# Hồ sơ và slide Bán kết — dùng thế nào, còn thiếu gì

Viết 19/09/2026 · Bán kết 26/09/2026

---

## Có những file gì

| File | Là gì |
|---|---|
| `HO_SO_BAN_KET_HuTieu.docx` | **Bản NỘP** — tiếng Anh hoàn toàn, điền trên đúng file mẫu của BTC |
| `HO_SO_BAN_KET_HuTieu_VI.docx` | Bản tiếng Việt, nội dung y hệt bản nộp (nhãn cột vẫn tiếng Anh vì đó là chữ in sẵn trong mẫu) |
| `SLIDE_BAN_KET_HuTieu_EN.pptx` / `.pdf` | 19 slide tiếng Anh để trình bày |
| `SLIDE_BAN_KET_HuTieu_VI.pptx` / `.pdf` | Bản tiếng Việt y hệt, để đọc và tập nói |
| `slide_assets/` | Ảnh dùng trong slide + `CREDITS.json` ghi giấy phép và ghi chú từng ảnh. Gồm ảnh Wikimedia Commons, **ảnh chụp dashboard của đội** (`dashboard_real.png`) và 2 sơ đồ tự vẽ (`fig_deviation.png`, `fig_charge_curve.png`) |
| `tools/build_deck.py` · `tools/build_dossier.py` | Script sinh lại. Mỗi script sinh cả bản Anh lẫn bản Việt |
| `tools/` (kèm) | Chỉnh chữ ở đây rồi chạy lại, đừng sửa thẳng vào file xuất ra |

Sinh lại:

```
python3 -m venv /tmp/venv && /tmp/venv/bin/pip install python-pptx python-docx pillow
/tmp/venv/bin/python docs/tools/build_deck.py       # ra 2 file pptx
/tmp/venv/bin/python docs/tools/build_dossier.py    # ra 2 file docx; cần 3 ảnh sơ đồ, xem chú thích đầu file
```

⚠️ **Sửa chữ thì sửa trong script rồi chạy lại**, đừng sửa thẳng vào .docx/.pptx — lần
chạy sau sẽ ghi đè. Toàn bộ chữ của slide nằm trong `TXT["EN"]` và `TXT["VI"]` ở đầu
`build_deck.py`; chữ của hồ sơ nằm trong `EN = dict(...)` và `VI = dict(...)` ở đầu
`build_dossier.py`.

---

## Thông tin trong hồ sơ

Đã điền: trường (ICTU), **Khoa Công nghệ thông tin**, giảng viên hướng dẫn **Đào Thị
Hằng**, liên hệ đội, 5 thành viên (Phạm Văn Huynh, Phạm Minh Tú, Đàm Đức Đôn, Hoàng Văn
Huy, Nguyễn Hoàng Anh Tuấn), mốc thời gian, số slot WISE-IoT.

Chưa có: **email của cô Hằng** — ô Email hiện chỉ có liên hệ của đội. Nếu BTC cần email
giảng viên thì thêm vào `email_todo` trong `build_dossier.py` rồi chạy lại.

---

## Slide — nền trắng, 19 slide

Nền trắng, chữ xanh đen `#0F172A`, nhấn cam đậm `#B45309`. Tiêu đề 26–34pt (tự co theo
độ dài), chữ nội dung **15–25pt**, số liệu 22–32pt.

| # | Slide | Ăn mục nào trong barem |
|---|---|---|
| 1 | Title | — |
| 2 | Vấn đề | Tính ứng dụng và tác động (10đ) |
| 3 | Vì sao ngưỡng cứng không đủ | Tính đổi mới (10đ) |
| 4 | Giải pháp trong một câu | Nội dung thuyết trình (10đ) |
| 5 | **Sơ đồ kiến trúc** | Mức độ ứng dụng AIoT (10đ) |
| 6 | Phần cứng đã làm | Sản phẩm/prototype (10đ) |
| 7 | Lớp 1 — Edge AI | Mức độ ứng dụng AIoT (10đ) |
| 8 | Đội đã thử nghiệm những gì | Sản phẩm và minh chứng (10đ) |
| 9 | **Phép kiểm đã TRƯỢT và cách sửa** | Khả năng phản biện (10đ) |
| 10 | Lớp 2 — sức khoẻ / tuổi thọ | Tính khả thi (15đ) |
| 11 | Bốn mức báo động | Tính khả thi và mức độ hoàn thiện (15đ) |
| 12 | **Hệ thống hiện tại đang chạy ra sao** | Tính khả thi (15đ) |
| 13 | **Dashboard — người vận hành nhìn thấy gì** | Sản phẩm và minh chứng (10đ) |
| 14 | **Nếu được cấp tài khoản Advantech** | Mức độ ứng dụng AIoT (10đ) |
| 15 | Kịch bản demo 90 giây | Sản phẩm/prototype (10đ) |
| 16 | Bán cho ai | Tiềm năng thương mại hóa (10đ) |
| 17 | Những gì **chưa** chứng minh được | Khả năng phản biện (10đ) |
| 18 | Lộ trình tới Chung kết | Tính khả thi (15đ) |
| 19 | Kết + Q&A | — |

Slide 12 = hiện trạng (firmware đã nói đúng giao thức WISE-PaaS, đội tự host Mosquitto →
Node-RED → InfluxDB → Grafana). Slide 14 = khi có tài khoản (cùng firmware, chỉ đổi host
broker + credential → IoT Hub + Dashboard + AFS). Hai sơ đồ này cũng là Figure 2 và
Figure 3 trong hồ sơ.

### Ảnh dashboard (slide 13 và Figure 4 trong hồ sơ)

Ảnh là **dashboard Grafana thật của đội** (`planb_cloud`), dựng lại bằng Docker rồi chụp
màn hình; **số liệu trong ảnh là số mô phỏng** seed vào InfluxDB theo kịch bản cell #3
nóng dần rồi vượt ngưỡng. Chú thích trên slide và trong hồ sơ đều ghi rõ điều đó — nếu
giám khảo hỏi "số này ở đâu ra" thì trả lời đúng như vậy.

Chụp lại khi có dữ liệu thật: bật stack (`planb_cloud/start.sh`), cho ESP32 chạy, rồi
mở dashboard và chụp; thay `slide_assets/dashboard_real.png` và chạy lại hai script.
Nhớ cắt lại `dashboard_strip.png` (phần đầu của ảnh) cho slide 12.

Trong lúc dựng thử, hai lỗi hiển thị đã lộ ra và **đã sửa** trong
`planb_cloud/grafana/make_dashboard.py` — panel "Cell nóng nhất" hiện 6 số chồng nhau,
và `Sensor_Healthy` vẫn để thang 8 cell. Chi tiết trong `DECISION_LOG.md` phần bổ sung
của QĐ-036.

---

## Cách nói về kết quả — quan trọng, đừng nói quá

Slide và hồ sơ **cố ý không có con số phần trăm**. Lý do: đội đang nghiên cứu trên
**pack thí nghiệm dựng từ cell cùng loại** và trên **dữ liệu công khai**, chứ chưa đo
trên pack xe điện đang chạy. Nói "phát hiện 100 %" mà không gắn với điều kiện đo là chỗ
giám khảo bẻ được ngay.

Cách nói đã dùng: *"chúng em đã thử nghiệm trên ... và thấy ..."*, kèm một câu nói thẳng
rằng đo trên pack xe điện thật là bước tiếp theo.

Nhưng **số liệu gốc phải thuộc nằm lòng** để trả lời khi bị hỏi:

| Nếu bị hỏi | Con số thật | Nguồn |
|---|---|---|
| Mô hình to bao nhiêu? | 271 tham số, 1,1 KB, 11 đặc trưng | QĐ-033 |
| Phát hiện được bao nhiêu? | offset 2 °C: **100 %** · drift 5 °C: **~67 %** | QĐ-039 (đo ở quy tắc giữ 30 s, đúng như firmware) |
| Báo động giả? | 0 sự kiện / 6.123.080 mẫu-cell → chặn trên 95 %: hiếm hơn 1 lần/3 ngày | QĐ-033 (cũng ở 30 s) |
| Xuống 6 cell có yếu đi không? | Báo động giả vẫn 0 %, tỉ lệ phát hiện không đổi, chỉ **chậm hơn 8–12 %** với lỗi ramp | QĐ-039 |
| Chuyển sang pack khác thì sao? | tách biệt ~19× trên pack McMaster | QĐ-033 |
| Trước đây từng hỏng thế nào? | báo oan 98,7 % trên file lành | QĐ-032 |
| Ngưỡng báo động? | 1,071, phải giữ 30 giây | QĐ-033 |
| Bốn mức báo động? | QĐ-028, bench 11/11 đạt | `BANG_CHUNG_BAO_DONG_2026-09-15.md` |
| Thống kê cháy? | ~31 % khi đỗ, ~25 % trong hầm, thay pack 1,5–12 triệu | `KHUNG_NGHIEN_CUU_v2_HuTieu.md` §2, §5 |

**Đừng nói "0 báo động giả" thành "không bao giờ báo oan".**

### ⚠️ Con số "drift 5 °C: 88 %" đã bị rút lại — đọc kỹ chỗ này

Số cũ đo ở quy tắc giữ liên tục **10 giây**, nhưng **firmware thật đòi 30
giây**. Ở 30 giây, drift 5 °C chỉ còn **~67 %**. Nguy hiểm hơn: ở 10 giây thì
báo động giả **không còn là 0** mà là 0,0189 %.

Nghĩa là **hai con số đinh cũ không thể cùng đúng một lúc** — "0 báo động giả"
là số của 30 giây, "88 %" là số của 10 giây. Ai hỏi "hai số này đo cùng lúc
chứ?" thì bản cũ không trả lời được.

Toàn bộ số liệu đang công bố giờ lấy **cùng một cấu hình 30 giây**. Chi tiết và
bảng đối chiếu: QĐ-039.

---

## Ảnh — giấy phép và tính trung thực

Tất cả ảnh đều từ Wikimedia Commons, giấy phép tự do. Ghi công in ở chân slide,
chi tiết trong `slide_assets/CREDITS.json`. Giấy phép CC BY-SA bắt buộc ghi công —
đừng xoá dòng chân slide.

Hai ảnh liên quan tới cháy, và đây là lý do chọn đúng hai ảnh đó:

- **`jal787.jpg`** (slide 1) — pack pin lithium-ion hỏng vì thermal runaway thật, vật
  chứng điều tra của NTSB (vụ Boeing 787 của JAL). Đây là **ảnh pin lithium cháy thật**,
  rõ nét, và nguồn thì không ai bắt bẻ được.
- **`auto_vallejo.jpg`** (slide 2) — xe cháy trong bãi đỗ, rõ nét, ban ngày. **Đây không
  phải xe điện**, và chú thích trên slide ghi đúng như vậy. Wikimedia Commons hiện
  **không có ảnh cháy xe điện nào xác minh được nguồn gốc** — nếu chú thích ảnh xe xăng
  thành "xe điện cháy" thì đó là nói sai sự thật ngay trên slide, mất nhiều hơn được.

Muốn ảnh cháy xe điện thật ở Việt Nam thì phải lấy từ báo (VnExpress, Tuổi Trẻ…) — ảnh
đó **có bản quyền**, muốn dùng phải xin phép hoặc chấp nhận rủi ro; nếu dùng thì tối
thiểu phải ghi rõ nguồn và ngày.

Slide 16 (bán cho ai) dùng ảnh **tủ đổi pin xe điện**, không phải bãi xe máy xăng như
bản trước.

Ảnh linh kiện ở slide 6 hiện là ảnh minh hoạ trên mạng. **Nên thay bằng ảnh chụp giàn
thật của đội** ngay khi dán xong đầu dò — vừa thuyết phục hơn, vừa đúng mục "có sản
phẩm mẫu/prototype" (10đ).

---

## Việc còn lại trước 26/09

- [ ] Thay ảnh linh kiện ở slide 6 bằng ảnh giàn 6 cell thật
- [ ] Bổ sung email giảng viên hướng dẫn vào hồ sơ nếu BTC yêu cầu
- [ ] Soát lại phân công nhiệm vụ 5 thành viên trong hồ sơ — tớ chia theo lộ trình, đổi nếu chưa đúng
- [ ] Quay video demo dự phòng theo đúng 5 bước ở slide 15
- [ ] Đọc thử slide bấm giờ — 19 slide, mục tiêu dưới 8 phút
- [ ] Chụp lại dashboard bằng **dữ liệu thật** khi đầu dò đã dán lên pack
- [ ] Đảo `Huynh` lên đầu danh sách WiFi trong `arduino_secrets.h` (QĐ-035)
- [ ] Sửa firmware từ 8 cell xuống 6 cell — xem QĐ-036
