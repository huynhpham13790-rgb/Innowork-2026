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
8S × 18650 2,55 Ah ≈ **73 Wh**, thiết bị chạy liên tục:

| Chế độ | Dòng trung bình | Công suất | Pack cạn sau |
|---|---|---|---|
| Wi-Fi giữ kết nối | ~100 mA | ~0,33 W | ~9 ngày |
| BLE quảng bá | ~10 mA | ~0,04 W | ~76 ngày |

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

**3.** ~~Lộ trình nói rõ điểm Bán kết phụ thuộc lớn vào mức độ dùng WISE-IoT.~~
**SAI — đính chính 22/09/2026**, xem `docs/BAREM_CHAM_BAN_KET.md`. Lý do thật
vẫn còn: NGƯỜI DÙNG A ngồi ở văn phòng nên **bắt buộc** phải có cloud, và
phương án nào làm dữ liệu lên cloud đứt quãng là bỏ rơi đúng khách hàng chính.
Đó là lý do sản phẩm, không phải lý do điểm thi.

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

## Phần 3 — Chốt phương án

Phần này **không xét còn bao nhiêu ngày tới ngày thi**. Lý do: một kiến trúc
chọn vì "kịp deadline" là kiến trúc phải làm lại ngay sau đó. Câu hỏi đúng là
*phương án nào giải quyết được vấn đề thật*, và câu trả lời đó không đổi theo
lịch thi.

**Chốt: giữ cả bốn kênh, Wi-Fi là kênh chính lên cloud, BLE là kênh phụ cho
NGƯỜI DÙNG B.**

Đây không phải thỏa hiệp giữa hai phương án — nó là kết luận thẳng từ Phần 1:
**ba nhóm người dùng đứng ở ba chỗ khác nhau, nên cần ba kênh khác nhau.** Hỏi
"BLE hay Wi-Fi" là hỏi sai câu, vì nó giả định một kênh phải phục vụ cả ba.

| | NGƯỜI DÙNG A (quản lý) | NGƯỜI DÙNG B (thợ) | NGƯỜI DÙNG C (người lái) |
|---|---|---|---|
| Ở đâu khi cần dữ liệu | Văn phòng, cách pack hàng km | Đứng ngay cạnh pack | Trên xe / đang ngủ |
| BLE phục vụ được? | ❌ ngoài tầm | ✅ đúng bài | ❌ không mở app |
| Wi-Fi→cloud phục vụ được? | ✅ đúng bài | ⚠️ được, nhưng vòng vèo | ❌ không nhìn màn hình |
| Còi tại chỗ phục vụ được? | ❌ | ✅ | ✅ đúng bài |

Không ô nào thừa, và không kênh nào phủ được cả ba cột. Đó là toàn bộ lý do
giữ cả ba.

### Vì sao Wi-Fi là kênh *chính*, không phải BLE

Ba lý do, không cái nào liên quan tới thời gian:

1. **BLE không phủ được khoảnh khắc nguy hiểm nhất.** Pin lithium dễ cháy nhất
   lúc **sạc qua đêm không ai trông** — đúng lúc điện thoại ở phòng khác và app
   đã bị hệ điều hành đóng. Một hệ an toàn không được có lỗ hổng đúng chỗ đó.
2. **BLE không thay được cloud, chỉ đổi người đưa thư.** Lớp 2 (RUL/SOH) cần
   nhìn **cả đội xe qua nhiều tháng** mới có ích. Dữ liệu vẫn phải về một chỗ
   tập trung; BLE chỉ thêm một thiết bị nữa phải có mặt đúng lúc.
3. **BLE làm trung tâm là âm thầm đổi khách hàng mục tiêu.** Kiến trúc
   "ESP32 ↔ điện thoại cá nhân" chỉ hợp với người dùng cá nhân — mà §5 xếp hạng
   3 và khuyến nghị không làm khách chính. Đổi kiến trúc truyền dữ liệu là đổi
   luôn mô hình kinh doanh, dù không ai nói ra.

### Vì sao vẫn nên làm BLE

Không phải để chiều góp ý. NGƯỜI DÙNG B là người thật, và với họ BLE **tốt hơn
Wi-Fi thật**: không cần cấu hình SSID, không phụ thuộc mạng kho, mở máy là thấy
8 nhiệt độ của đúng cái pack đang cầm. Bắt thợ mở dashboard cloud để xem một
pack cách mình 30 cm là thiết kế tồi.

Bản tối giản đủ dùng: ESP32 phát 8 nhiệt độ + trạng thái Lớp 1 qua GATT
characteristic, đọc bằng nRF Connect có sẵn trên store — **không phải viết app**.
ESP32-S3 có sẵn BLE, không tốn thêm linh kiện. Chi phí thấp, giá trị thật.

### Thứ tự làm — xếp theo "cái nào còn đang giả"

Không xếp theo deadline, xếp theo mức độ thứ đó còn là mô phỏng:

1. **Nối 8 cảm biến thật vào firmware chính** — kèm hiệu chỉnh offset (QĐ-022)
   và đọc không chặn. Đây là mắt xích mô phỏng cuối cùng của Lớp 1, và là việc
   có giá trị cao nhất trong toàn bộ dự án lúc này: nó biến demo "hơ nóng cell
   → còi kêu" từ diễn thành thật.
2. **Dán đầu dò lên pack rồi đo lại offset** — bảng offset đo lúc đầu dò để rời
   ngoài không khí không dùng được sau khi dán.
3. **Hiệu chỉnh lại hệ số Lớp 2 trên pack thật** — hiện đang dùng hệ số của pin
   NASA 18650 2,0 Ah; pack của đội là 8S × 18650 2,55 Ah — cùng loại cell nên
   sai lệch nhỏ hơn tưởng, nhưng phải sạc ở 1,9 A cho khớp tốc độ 0,75C.
4. **BLE tối giản cho NGƯỜI DÙNG B.**
5. Thử Wi-Fi phát từ điện thoại (AC-05.1), diễn tập demo (AC-05.3).

Việc 1–3 đều là "biến thứ đang giả thành thật". Việc 4 là thêm năng lực mới.
Khi phải chọn, ưu tiên loại 1–3 trước: một hệ thống ít chức năng mà thật thì
thuyết phục hơn một hệ nhiều chức năng mà nửa mô phỏng.
