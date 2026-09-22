# Bằng chứng: app Android chạy trên điện thoại thật — 22/09/2026

Máy: **Samsung Galaxy M34 (SM-M346B), Android 16**. Đáng chú ý: Android 16 mới
hơn `targetSdk 34` mà app khai, nên đây là bài kiểm thật chứ không phải chạy
trên đúng phiên bản đã nhắm.

---

## 1. Đường dữ liệu — ĐẠT

Cài, mở, không crash. Bảy ô đều có số thật, cập nhật liên tục qua notify:

```
Trạng thái        : BINH THUONG
Lớp 1             : khong co cell bat thuong (cao nhat 0.15 / 1.07)
Nhiệt độ từng cell: 1:27.2 2:27.0 3:27.4 4:27.4 5:27.6 6:27.9
Tình trạng hệ thống: cam bien 6/6  wifi OK  cloud OK  chay 31 phut
```

Nhiều đặc tính cùng chảy về một lúc → **hàng đợi GATT làm đúng việc**. Không có
nó, 3 trong 4 ô notify sẽ đứng im mà không báo lỗi gì.

## 2. Nút tắt còi — ĐẠT, kiểm bằng ba đường độc lập

| Đường | Bằng chứng |
|---|---|
| Android | `onWriteCharacteristic() status=GATT_SUCCESS (0x00), length=7` |
| Chip (serial) | `[BLE ] lenh "mute on" -> da tat tieng - TU BAT LAI sau 5 phut` |
| Cloud (MQTT) | `Alarm_Muted = 1` |

Đường thứ ba quan trọng nhất: nó **không đi qua BLE**, nên loại trừ khả năng
app tự báo thành công mà chip chưa nhận gì.

⚠️ **KHÔNG hiện hộp nhập PIN, và điều đó ĐÚNG** — điện thoại đã ghép đôi với
`HuTieu-BMS` từ trước (qua nRF Connect), liên kết đang mã hoá `keySize=16`. PIN
chỉ hỏi ở lần ghép đôi ĐẦU TIÊN. **Hệ quả: đội CHƯA tận mắt thấy màn hỏi PIN
trên máy này.** Muốn trình diễn phần bảo mật thì phải *Quên thiết bị* trong Cài
đặt Bluetooth trước, rồi mới bấm.

## 3. Lớp 2 hiện đúng cách — và đang chứng minh giá trị của cờ ngoại suy

```
⚠️ NGOAI DAI HUAN LUYEN (2/9 dac trung) - SOH 78.6% | con ~0 chu ky
   | da ghi 61 chu ky | xu huong -0.365 %/chu ky
```

Cảnh báo nằm **trước** con số, cố ý. Không có nó, người đọc thấy *"còn ~0 chu
kỳ"* và kết luận pin sắp chết — trong khi thực tế là chu kỳ mô phỏng đã trôi ra
ngoài dải NASA đã học và mô hình đang đoán mò. Đây đúng là tình huống QĐ-019
được viết ra để chặn, và nó chặn được trên máy thật.

---

## Bốn lỗi tìm ra trong buổi này

### (a) Bản web KHÔNG mở được trên Wi-Fi trường — và chẩn đoán đầu là sai

AI đoán người dùng làm sai bước `chrome://flags`. **Sai.** Log máy chủ tĩnh có
đúng 5 dòng, *tất cả* từ IP máy tính; **không một request nào từ điện thoại**.
Trang chưa bao giờ tải được nên Web Bluetooth còn chưa tới lượt. Mạng trường
chặn máy-nói-với-máy. Đây là lý do có app gốc (QĐ-044).

### (b) Ghi lệnh trả về `GATT_WRITE_NOT_PERMITTED (0x03)`

`0x03` **không phải** lỗi thiếu quyền — thiếu xác thực là `0x05`/`0x0F`. `0x03`
nghĩa là ghi vào một handle không cho ghi. Firmware đặt cờ đúng
(`BLE_GATT_CHR_F_WRITE_AUTHEN = 0x2000`), và sau khi sửa, app đọc được
`0008 handle=44 quyen=0x0a` (READ|WRITE) — đúng như mong đợi.

> ⚠️ **Thành thật về nguyên nhân:** giả thuyết là Android giữ **cache bảng GATT**
> của thiết bị đã ghép đôi, mà firmware thì đã nạp lại 2 lần trong ngày nên
> handle đổi chỗ. Nhưng bản sửa thêm **hai** thay đổi cùng lúc (`refresh()` qua
> reflection **và** nối thẳng theo địa chỉ), và **chưa hề ghi lại cờ quyền
> TRƯỚC khi sửa**. Nên không chứng minh được thay đổi nào đã chữa. Chỉ biết
> chắc: sau khi sửa thì ghi thành công.

### (c) Ô Lớp 2 trống vĩnh viễn

Lớp 2 không có notify (firmware chỉ cập nhật mỗi chu kỳ sạc). App chỉ đọc **một
lần** lúc kết nối; nếu lúc đó chip chưa tính xong, ô đó trống **mãi mãi**. Đã
thêm đọc lại mỗi 30 s.

### (d) Chip NGỪNG QUẢNG BÁ khi điện thoại còn giữ liên kết — nguy hiểm nhất

Đo được: máy tính quét thấy **91 thiết bị nhưng không có `HuTieu-BMS`**, trong
khi chip vẫn chạy, vẫn đẩy dữ liệu lên cloud bình thường. ESP32 chỉ nhận **một**
kết nối; app bị tắt nhưng Android vẫn giữ liên kết, nên chip không quảng bá và
**không ai khác quét thấy nó**.

App báo *"Không thấy pack. Pack có đang bật không?"* — câu đó dẫn người ta đi
kiểm dây, kiểm nguồn, trong khi pack hoàn toàn khoẻ.

**Đã sửa phía app:** thiết bị đã ghép đôi thì **nối thẳng theo địa chỉ, không
quét**. Nối theo địa chỉ không cần quảng bá. Nó còn tránh luôn giới hạn của
Android: quá 5 lần quét trong 30 giây là hệ thống lặng lẽ bỏ qua.

⚠️ **Phía firmware thì CHƯA sửa.** Máy nào chưa ghép đôi lần nào vẫn phải quét,
và vẫn sẽ không thấy gì nếu có điện thoại khác đang giữ liên kết. Hôm thi, nếu
giám khảo muốn nối bằng máy của họ trong lúc máy đội đang nối thì sẽ **không
thấy pack**. Cần cân nhắc cho phép nhiều kết nối, hoặc luôn ngắt máy đội trước.

---

## Còn lại phải làm trước 26/09

- [ ] Trình diễn màn hỏi PIN: *Quên thiết bị* trước rồi mới bấm tắt còi.
- [ ] Quyết định phần nhiều kết nối BLE ở firmware (mục **d**).
- [ ] Thử lại toàn bộ khi có **báo động thật** (hiện mới thử lúc `BINH THUONG`),
      để thấy băng đỏ và xác nhận còi mức NGUY HIỂM **không** tắt được.
