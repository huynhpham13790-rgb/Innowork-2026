# Barem chấm Bán kết — NGUỒN SỰ THẬT DUY NHẤT

Chép từ **`docs/PHIẾU CHẤM BÁN KẾT AIOT.docx`** của BTC, ngày 22/09/2026.

> ⚠️ **Đọc file này trước khi lập luận bất cứ điều gì về "điểm thi".**
> Trước đây đội tự suy ra tiêu chí từ thông báo cũ và viết vào
> `InnoWorks2026_Lo_trinh_Doi_HuTieu.md` §9 rằng *"WISE-IoT chiếm phần lớn
> điểm"*. **Điều đó SAI.** Barem thật không nhắc tới WISE-IoT một lần nào.
> Những chỗ sai đã được đính chính tại chỗ và trỏ về đây.

---

## Bảng điểm — tổng 100

| Nhóm | Mục | Điểm |
|---|---|---|
| **1. Hồ sơ và sản phẩm dự thi — 15** | Hồ sơ đầy đủ, rõ ràng, đúng yêu cầu; thể hiện nội dung và quá trình phát triển | 5 |
| | **Sản phẩm mẫu/prototype hoặc kết quả thử nghiệm; video/demo thể hiện nguyên lý, chức năng, kết quả** | **10** |
| **2. Nội dung dự án — 60** | Mức độ ứng dụng AIoT: vai trò của AI, cảm biến, kết nối, xử lý dữ liệu, tính liên kết hệ thống | 10 |
| | Tính đổi mới, sáng tạo: điểm mới/khác biệt so với giải pháp hiện có | 10 |
| | Tính ứng dụng và tác động: giải quyết nhu cầu thực tiễn, rõ đối tượng dùng, tạo giá trị | 10 |
| | **Tính khả thi và mức độ hoàn thiện: khả năng triển khai, lộ trình rõ, sản phẩm tiếp tục hoàn thiện được** | **15** |
| | Tiềm năng phát triển và thương mại hoá: mở rộng, nhân rộng, khách hàng/thị trường | 10 |
| **3. Thuyết trình — 25** | Nội dung và kỹ năng: cấu trúc logic, đúng trọng tâm, đúng thời gian, thuyết phục | 10 |
| | **Khả năng phản biện: hiểu sâu dự án, trả lời có căn cứ về công nghệ, khả thi, hướng phát triển** | **10** |
| | Làm việc nhóm: phân công rõ, **mức độ tham gia và hiểu biết của TỪNG thành viên** | 5 |

---

## Bốn điều rút ra, và chúng đổi cách tiêu thời gian

### 1. WISE-IoT đáng 0 điểm trực tiếp

Không có mục nào chấm nền tảng cloud cụ thể. WISE-IoT chỉ tính gián tiếp qua
*"mức độ ứng dụng AIoT"* (10đ) — mà **Plan B thoả mãn đầy đủ**: có AI, có IoT,
có cảm biến, có kết nối, có xử lý dữ liệu.

**Hệ quả:** mọi quyết định kiểu *"đặt cái này ở cloud để ăn điểm WISE-IoT"* đều
mất cơ sở. Chọn chỗ đặt tính toán theo **kỹ thuật**, không theo điểm.

Vẫn phải giữ tài khoản WISE-IoT còn sống vì lý do **điều kiện dự thi**
(chứng chỉ + tối thiểu 1 Dashboard, xem `Lo_trinh` §2) — đó là điều kiện **cần
để không bị loại**, không phải thang điểm.

### 2. "Tính khả thi và mức độ hoàn thiện" là mục nặng nhất — 15 điểm

Thưởng cho hệ **chạy được, hoàn chỉnh, có lộ trình**. Không thưởng cho hệ dùng
công nghệ oai hơn.

**Hệ quả:** đổi nền tảng vào phút chót để lấy thứ "tốt hơn" là đánh cược đúng
mục điểm nặng nhất. Một hệ đang chạy thắng một hệ đẹp hơn nhưng dở dang.

### 3. Một phần tư tổng điểm KHÔNG nằm ở code

25 điểm cho thuyết trình, phản biện, làm việc nhóm. Đây là phần dễ bị bỏ quên
nhất vì nó không có gì để "làm xong".

- **Phản biện (10đ)** đòi *trả lời có căn cứ*. `docs/DECISION_LOG.md` chính là
  kho căn cứ đó — nó ghi cả những lần chọn sai và vì sao. Phải **đọc lại trước
  khi thi**, không phải chỉ viết ra rồi để đấy.
- **Làm việc nhóm (5đ)** chấm *hiểu biết của TỪNG thành viên*. Nếu chỉ một
  người giải thích được hệ thống thì giám khảo nhìn ra ngay. Điều này trùng
  đúng với Definition of Done trong `CLAUDE.md`: *"người phụ trách giải thích
  được nó làm gì"*.

### 4. Prototype + minh chứng đáng 10 điểm — bằng chỗ với cả mục AIoT

Video/demo *"thể hiện được nguyên lý, chức năng và kết quả hoạt động"*. Các file
`docs/BANG_CHUNG_*.md` phục vụ đúng mục này. **Clip dự phòng** cũng vậy — và nó
còn là phương án chống rủi ro nếu demo trực tiếp trục trặc.
