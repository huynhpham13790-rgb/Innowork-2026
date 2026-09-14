# Người dùng là ai, dùng như thế nào — và chọn đường truyền nào

Viết 14/09/2026, sau góp ý của cô về phương án BLE bắn thẳng lên điện thoại.

`KHUNG_NGHIEN_CUU_v2` §5 đã trả lời **"bán cho ai"**. Cái nó chưa trả lời là
**"ai ngồi trước màn hình, mở ra lúc nào, để làm gì"** — mà chính câu đó mới
quyết định được chọn BLE hay Wi-Fi. Tài liệu này lấp chỗ trống đó.

---

## Phần 1 — Ba người dùng, ba cách dùng khác nhau

Người **mua** và người **dùng** không phải một người. Đây là chỗ hay lẫn.

### NGƯỜI DÙNG A — Quản lý vận hành đội xe / trạm đổi pin

Khách hàng chính theo §5. Là người **trả tiền** và cũng là người **dùng nhiều nhất**.

| | |
|---|---|
| Ngồi ở đâu | Văn phòng / quầy trạm, máy tính để bàn |
| Mở lúc nào | Đầu ca và cuối ca, mỗi ngày 2 lần |
| Nhìn cái gì | Bảng **tất cả** pack: pack nào SOH thấp nhất, pack nào sắp tới hạn thay |
| Cần lớp AI nào | **Lớp 2 (RUL/SOH)** là chính |
| Hành động rút ra | "Rút pack #37 ra khỏi vòng quay, đặt hàng thay" |
| Kênh báo động | Dashboard + tin nhắn nếu có pack nguy hiểm trong kho |

→ Người này **không cầm điện thoại đứng cạnh pin**. Dữ liệu bắt buộc phải lên
cloud thì họ mới làm được việc. Đây là lý do tồn tại của tầng WISE-IoT.

### NGƯỜI DÙNG B — Thợ kỹ thuật / nhân viên trạm

| | |
|---|---|
| Ngồi ở đâu | **Đứng ngay cạnh pack**, tay cầm điện thoại |
| Mở lúc nào | Khi A giao việc kiểm tra một pack cụ thể |
| Nhìn cái gì | 8 nhiệt độ cell của **đúng một pack**, ngay lúc này |
| Cần lớp AI nào | **Lớp 1** — cell nào lệch |
| Hành động rút ra | "Cell #5 nóng hơn 3 °C, mở ra kiểm tra mối hàn" |

→ Đây **chính là người mà BLE phục vụ**, và phục vụ tốt hơn Wi-Fi thật: không
cần mạng, không cần cấu hình, mở app là thấy. Ghi nhận góp ý của cô đúng ở đây.

### NGƯỜI DÙNG C — Người lái xe

| | |
|---|---|
| Ngồi ở đâu | Trên xe, hoặc ở nhà lúc cắm sạc |
| Mở lúc nào | **Gần như không bao giờ mở** |
| Cần gì | Không cần màn hình. Cần **còi kêu** khi pin sắp cháy |
| Cần lớp AI nào | Lớp 1, chạy trên thiết bị, không cần mạng |

→ Chỗ này phải nói thẳng: **người lái xe không mở app.** Thiết kế nào đòi hỏi
họ mở app để hệ thống an toàn hoạt động là thiết kế sai. Còi + LED trên chính
thiết bị mới là giao diện của C.

---

## Phần 2 — Góp ý của cô: BLE thẳng lên điện thoại

### Gỡ một hiểu nhầm trước

Cô mô tả đường hiện tại là *"bắn lên gateway xong gateway lại bắn lên điện thoại"*.
Kiến trúc hiện tại **không có gateway** — tầng Raspberry Pi đã bị bỏ từ §1 vì
đúng những lý do cô nêu (tốn điện 3–6 W, thêm điểm hỏng, đội giá BOM 5 lần).
Đường hiện tại là:

```
ESP32-S3 ──Wi-Fi/MQTT──► cloud ──► dashboard
```

Một chặng. Không có hộp trung gian nào.

### Còn chuyện tốn pin thì sao?

Đây là lý do chính cô nêu, nên phải kiểm bằng số. Ước lượng trên pack
8S × 3 Ah ≈ **86 Wh**, thiết bị chạy liên tục:

| Chế độ | Dòng trung bình | Công suất | Pack cạn sau |
|---|---|---|---|
| Wi-Fi giữ kết nối | ~100 mA | ~0,33 W | ~11 ngày |
| BLE quảng bá | ~10 mA | ~0,04 W | ~90 ngày |

BLE tiết kiệm hơn **thật**, khoảng 8 lần — cô nói đúng về hướng. Nhưng phải
nhìn mẫu số: xe máy điện được sạc **mỗi 1–3 ngày**. Kể cả phương án tốn điện
nhất cũng dư sức sống giữa hai lần sạc, mà §2 còn có 3 chế độ giảm tần suất khi
xe đỗ nữa.

→ **Điện không phải ràng buộc quyết định ở đây.** Nói thế này không phải để bác
góp ý của cô — mà vì nếu lên sân khấu lấy "tiết kiệm pin" làm lý do chính, giám
khảo hỏi đúng phép tính trên là đội đuối. Có lý do khác mạnh hơn nhiều.

### Ràng buộc thật sự: ai đang ở đó, và điểm thi

**1. BLE chỉ hoạt động khi có người cầm điện thoại đứng gần.**
Lúc nguy hiểm nhất của pin lithium là **đang sạc qua đêm, không ai trông**.
Đúng lúc đó thì điện thoại ở trong phòng ngủ và app đã bị hệ điều hành đóng.
BLE không phủ được khoảnh khắc quan trọng nhất.

**2. BLE không thay được cloud, nó chỉ đổi người đưa thư.**
Dữ liệu vẫn phải tới NGƯỜI DÙNG A ở văn phòng. BLE chỉ thay chặng
`ESP32 → internet` bằng `ESP32 → điện thoại → internet`. Cloud vẫn còn nguyên,
và giờ phụ thuộc thêm một thiết bị nữa có mặt hay không.

**3. Lộ trình nói rõ điểm Bán kết phụ thuộc lớn vào mức độ dùng WISE-IoT.**
Đây là ràng buộc cứng của cuộc thi. Phương án nào làm dữ liệu lên cloud trở nên
đứt quãng là tự bỏ điểm.

### Kết luận

Góp ý của cô **đúng, nhưng là đúng cho NGƯỜI DÙNG B** — và nó đã nằm sẵn trong
`KHUNG_NGHIEN_CUU_v2` §9 phương án (B), trong gói sản phẩm **Basic**. Nó là một
**kênh bổ sung**, không phải kênh thay thế.

Phân vai đúng:

| Kênh | Phục vụ ai | Vai trò |
|---|---|---|
| Còi + LED tại chỗ | C (người lái) | An toàn. Không cần mạng, không cần app. |
| Wi-Fi → cloud | A (quản lý) | RUL/SOH, quản lý cả đội xe. **Là phần được chấm điểm.** |
| BLE → điện thoại | B (thợ) | Xem tại chỗ khi đứng cạnh pack |
| Store-and-forward | tất cả | Không mất dữ liệu lúc mất mạng |

Một điểm nữa đáng lưu ý: BLE hợp với NGƯỜI DÙNG C (cá nhân) hơn cả, nhưng §5
xếp C là **hạng 3 và khuyến nghị không làm khách hàng chính**. Chọn BLE làm
kiến trúc trung tâm là âm thầm đổi khách hàng mục tiêu từ B2B sang B2C — ngược
lại kết luận kinh doanh đã chốt. Đây là lý do mạnh nhất để không đổi.

---

## Phần 3 — Quyết định cho ngày mai (Bán kết 15/09)

**Giữ nguyên kiến trúc. Không viết BLE trước Bán kết.**

Lý do: còn **1 ngày**. Làm BLE cho ra hồn cần một app Android — vài ngày làm,
vài ngày sửa. Đổi kiến trúc truyền dữ liệu vào đêm trước ngày thi là rủi ro
lớn nhất có thể tự chuốc vào, mà đổi lại không được thêm điểm nào.

**Nhưng đưa nó vào bài nói.** Khi giám khảo hỏi *"đi ra đường thì lấy mạng ở
đâu?"* — mà chắc chắn sẽ có người hỏi — trả lời bằng bảng phân vai 4 kênh ở
trên. Nó cho thấy đội đã nghĩ tới, biết vì sao chưa làm, và biết làm cho ai.
Đó là câu trả lời của người hiểu bài toán, không phải người quên mất.

Việc cần làm sau Bán kết, xếp theo thứ tự: gộp BLE vào gói Basic cho NGƯỜI
DÙNG B, viết app đọc 8 nhiệt độ + trạng thái Lớp 1 qua BLE, giữ Wi-Fi làm kênh
chính lên cloud.
