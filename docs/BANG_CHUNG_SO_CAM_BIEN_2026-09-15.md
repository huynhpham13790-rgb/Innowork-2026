# Bằng chứng: cần bao nhiêu cảm biến nhiệt? — 15/09/2026

Liên quan: QĐ-030, AC-06.30. Mã: `ai/sensor_count_study.py`.

## Câu hỏi

Trong đội có tranh luận: 8 cảm biến cho 8 cell thì vướng và không kinh tế; chia
mỗi cảm biến cho 2 hoặc 4 cell có được không?

Cả hai phía đều chưa có lập luận. "Gắn 8 con cho chắc" không phải lý lẽ, mà
"bớt đi cho gọn" cũng vậy. Tài liệu này biến nó thành số đo.

## Cách đo

Lỗi được tiêm vào **một cell thật**, rồi mới lấy trung bình nhóm để ra số đọc
của cảm biến dùng chung — đúng thứ tự vật lý. (Tiêm thẳng vào giá trị nhóm là
tự cho mình điểm cao.) 24 đoạn pack-ảo 8 cell, chu kỳ 349–356 của tập test.

Ba kiểu lỗi: `offset` (tiếp xúc kém), `ramp` (tiền đề thermal runaway),
`drift` (lão hoá chậm). Cùng ngưỡng AE, cùng quy tắc vượt liên tục 10 giây.

## KQ-01 — `ramp`: kiểu lỗi quan trọng nhất về an toàn

| Tốc độ tăng | 8 cảm biến | 4 cảm biến | 2 cảm biến |
|---|---|---|---|
| 0,5 °C/10 phút | **100 %** (6 420 s) | 96 % (13 828 s) | 12 % (25 641 s) |
| 1,0 °C/10 phút | **100 %** (2 822 s) | 100 % (6 516 s) | 88 % (16 885 s) |
| 2,0 °C/10 phút | **100 %** (1 142 s) | 100 % (2 993 s) | 100 % (8 063 s) |
| 3,0 °C/10 phút | **100 %** (626 s) | 100 % (2 133 s) | 100 % (4 765 s) |
| 5,0 °C/10 phút | **100 %** (346 s) | 100 % (1 199 s) | 100 % (2 958 s) |

**Đọc bảng này ở cột TRỄ, không phải cột tỉ lệ.** Với `ramp` mạnh thì cấu hình
nào cũng phát hiện được — câu hỏi là *sớm bao nhiêu*. Ở 2 °C/10 phút:

- 8 cảm biến: 1 142 giây (19 phút)
- 4 cảm biến: 2 993 giây (50 phút) — **chậm hơn 2,6 lần**
- 2 cảm biến: 8 063 giây (134 phút) — **chậm hơn 7 lần**

Với một sự cố đang leo thang, hơn 100 phút chênh lệch là khoảng cách giữa "ngắt
sạc kịp" và "ngắt sạc khi cell đã đi quá xa". Đây là lập luận mạnh nhất cho
việc giữ nhiều cảm biến, và nó là một con số chứ không phải một cảm giác.

## KQ-02 — `offset`: bớt cảm biến là mất hẳn khả năng phát hiện

| Độ lớn | 8 cảm biến | 4 cảm biến | 2 cảm biến |
|---|---|---|---|
| 2 °C | 79 % | 0 % | 0 % |
| 3 °C | **100 %** | 4 % | 0 % |
| 5 °C | **100 %** | 96 % | **0 %** |

Đây là hiệu ứng pha loãng, thấy rõ như sách giáo khoa: lỗi 3 °C trên một cell,
chia trung bình cho 2 cell còn 1,5 °C, cho 4 cell còn 0,75 °C — tụt xuống dưới
mức nhiễu nền.

**2 cảm biến không phát hiện được lỗi `offset` ở BẤT KỲ độ lớn nào đã thử**, kể
cả 5 °C. Cấu hình đó không bảo vệ được gì trước kiểu lỗi tiếp xúc kém.

## KQ-03 — `drift`: cả ba cấu hình đều yếu, và đó là điểm yếu đã biết

| Độ lớn | 8 cảm biến | 4 cảm biến | 2 cảm biến |
|---|---|---|---|
| 1 °C | 29 % | 4 % | 0 % |
| 3 °C | 38 % | 0 % | 4 % |
| 5 °C | 46 % | 0 % | 0 % |

Kể cả với 8 cảm biến, autoencoder chỉ bắt được dưới một nửa số ca trôi chậm.
Đây **không phải phát hiện mới** — `ai/README.md` đã ghi từ đầu, và chính vì
vậy mà ngưỡng cứng 60 °C phải giữ. Bảng này xác nhận lại, và cho thấy bớt cảm
biến làm nó từ yếu thành gần như bằng không.

## KQ-04 — Cái bẫy: ít cảm biến thì "báo oan" ít hơn

| Số cảm biến | Tỉ lệ báo oan |
|---|---|
| 8 | 0,0360 % |
| 4 | 0,0005 % |
| 2 | 0,0001 % |

Nhìn qua thì tưởng 2 cảm biến là cấu hình tốt nhất. **Sai.** Tỉ lệ báo oan thấp
ở đây không phải dấu hiệu chính xác — nó là *triệu chứng của sự mù*. Cùng một
phép pha loãng làm cấu hình 2 cảm biến không thấy lỗi thật (KQ-02) cũng làm nó
không thấy nhiễu. Một hệ không bao giờ báo động có tỉ lệ báo oan 0 % hoàn hảo.

Bài học chung: **không bao giờ đọc tỉ lệ báo oan tách khỏi tỉ lệ phát hiện.**

## KQ-05 — Thứ bảng số không đo được: chỉ đích danh

| Số cảm biến | Thu hẹp còn |
|---|---|
| 8 | đúng 1 cell |
| 4 | 2 cell — phải tháo ra dò tay |
| 2 | 4 cell — phải tháo ra dò tay |

Điều này quan trọng với chiến lược **thay cell cùng tuổi** ở QĐ-027: muốn thay
đúng cell hỏng thì phải biết cell nào. 2 cảm biến chỉ nói được "một trong bốn
cell ở nửa này".

## Kết luận

| Cấu hình | Có dùng được không |
|---|---|
| **8 cảm biến** | ✅ Bắt 100 % `ramp` ở mọi tốc độ, 100 % `offset` từ 3 °C, chỉ đúng cell. |
| **4 cảm biến** | ⚠️ Chấp nhận được về an toàn (`ramp` ≥ 96 %) nhưng **trễ gấp 2,6 lần**, mất `offset` dưới 5 °C, chỉ thu hẹp còn 2 cell. Là một đánh đổi có thể bảo vệ được — nếu nói rõ mất gì. |
| **2 cảm biến** | ❌ Không bảo vệ được. Mù hoàn toàn với `offset`, trễ gấp 7 lần với `ramp`. |

**Câu nói trước giám khảo:** *"Chúng em đã đo mức đánh đổi. Bản lab dùng 8 cảm
biến. Bản thương mại có thể rút về 4 — chấp nhận trễ gấp 2,6 lần và mất khả
năng bắt lỗi tiếp xúc dưới 5 °C — nhưng 2 cảm biến thì không còn bảo vệ được
gì."*

Khác hẳn *"chúng em gắn 8 con cho chắc"*.

## Ba chỗ nghiên cứu này còn lạc quan — nói ra trước khi bị hỏi

1. **Trung bình nhóm là trường hợp TỐT.** Cảm biến thật dán lệch về một cell sẽ
   nhạy với cell đó và gần mù với cell kia. Thực tế tệ hơn bảng này.
2. **Autoencoder được huấn luyện trên pack 8 kênh.** Chạy nó với 2 kênh là dùng
   ngoài điều kiện huấn luyện; "trung bình pack" của 2 kênh nhiễu hơn nhiều. Kết
   quả N=2 nên đọc như **giới hạn trên** — huấn luyện riêng cho 2 kênh có thể
   khá hơn, nhưng không thể vượt qua giới hạn vật lý của phép pha loãng.
3. **Bỏ qua dẫn nhiệt giữa các cell**, vốn còn làm nhoè lỗi thêm.

Cả ba đều nghiêng về hướng làm cấu hình ít cảm biến trông **đẹp hơn thực tế**.
Nghĩa là kết luận "2 cảm biến không dùng được" càng vững.
