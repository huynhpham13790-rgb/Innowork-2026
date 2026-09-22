# Bằng chứng: tắt tiếng ở bất thường Lớp 1 giữ được — 22/09/2026 (QĐ-047)

Yêu cầu: bất thường Lớp 1 (chưa tới NGUY KỊCH) thì **tắt còi là tắt hẳn**, không
tự kêu lại; nhưng màn hình **vẫn phải báo** tới khi xử lý xong. Mức NGUY KỊCH thì
ngược lại — còi tự kêu lại để nhắc.

---

## Lần thử thứ nhất: TRƯỢT, và nó lộ ra một lỗi không ai đọc code mà thấy

Sau khi bỏ hẹn giờ 5 phút của đường BLE, tắt tiếng ở mức BÁO ĐỘNG vẫn **kêu lại
sau 98 giây**:

```
  4s  TAT TIENG coi
 27s  BAO DONG -> OK
 72s  OK -> THEO DOI
101s  THEO DOI -> BAO DONG
102s  coi: KEU              ← kêu lại, dù sự cố không hề nặng thêm
```

Thủ phạm không phải hẹn giờ mà là luật `if (lvl_ > prev) muted_ = false`. Điểm
AI dao động quanh ngưỡng nên mức **tụt xuống rồi lên lại**, và mỗi lần lên lại
đều bị tính là "leo thang".

Luật đó đọc thì hợp lý và đã qua **11/11 ca bench**. Cái bench thiếu là một tín
hiệu **dao động** — nó chỉ có các mức đi lên đều. Chỉ phần cứng thật với điểm AI
thật mới lộ ra.

**Sửa (QĐ-047b):** nhớ `mute_lvl_` — mức tại lúc người dùng bấm tắt — và chỉ xoá
tắt tiếng khi `lvl_ > mute_lvl_`.

## Lần thử thứ hai: ĐẠT, qua trọn một chu trình tụt–lên

```
160s [ALRM] TAT TIENG coi (lenh tu xa)          ← tắt lúc đang BÁO ĐỘNG
160s [BLE ] lenh "mute on"
295s [ALRM] BAO DONG -> OK      (dang tat tieng)
343s [ALRM] OK -> THEO DOI      (dang tat tieng)   ← luật cũ xoá tắt tiếng ở ĐÂY
372s [ALRM] THEO DOI -> BAO DONG (dang tat tieng)  ← quay lại BÁO ĐỘNG, VẪN IM
```

**Số lần còi kêu sau khi tắt: 0.** Giữ im 212 giây qua trọn một chu trình
tụt–lên. Trước khi sửa, cùng kịch bản này còi kêu lại sau 98 giây.

Cách tạo dao động: sưởi → BÁO ĐỘNG → ngắt sưởi cho nguội → sưởi lại. Đây đúng
là cách một sự cố thật diễn ra: nóng lên, dịu đi, rồi nóng lại.

## Màn hình thay cho tiếng còi

![băng trạng thái] App hiện:

```
BÁO ĐỘNG — cell bất thường
🔇 BẤT THƯỜNG CHƯA ĐƯỢC XỬ LÝ — còi đã tắt
```

Khi còi đã im, **màn hình là lời nhắc duy nhất còn lại**, nên nó phải nói thành
câu chứ không chỉ là một biểu tượng loa gạch chéo. Ở mức NGUY KỊCH thì nói thêm
*"còi sẽ kêu lại để nhắc"*, để người dùng biết im lặng này có hạn.

## Đối chiếu hai mức

| Diễn biến | Trước | Sau |
|---|---|---|
| BÁO ĐỘNG → OK → BÁO ĐỘNG | còi kêu lại sau 98 s ❌ | im suốt ✅ |
| BÁO ĐỘNG → NGUY KỊCH | còi kêu lại ✅ | còi kêu lại ✅ |
| NGUY KỊCH, tắt tiếng | — | tự kêu lại sau 2 phút ✅ (QĐ-045) |

---

## Lỗi thứ hai, tìm được vì bài thử bị nó chặn

Giữa bài thử, chip **mất cloud và không tự cứu được**: `rc=-2` lặp lại, Wi-Fi
báo `OK` với RSSI −52 dBm, nhưng máy khác **không ping tới được**. Lệnh sưởi đi
qua MQTT nên cả bài thử đứng hình.

Đây là **lần thứ hai trong ngày** — sáng nay đã gặp một lần và chưa tìm ra
nguyên nhân. Vòng lặp cũ chỉ thử lại MQTT mãi mãi:

```cpp
if (!mqtt.connected() && WiFi.status() == WL_CONNECTED && ...)
    mqttTryConnect();      // không bao giờ đụng tới Wi-Fi
```

**Sửa (QĐ-048):** đếm số lần hỏng liên tiếp, rồi leo thang — 6 lần (~30 s) ép
nối lại Wi-Fi, 24 lần (~2 phút) khởi động lại. Cả hai đều an toàn với dữ liệu:
spool nằm trên flash, lúc đo đang giữ **215 KB** và không mất gói nào.

⚠️ **Chưa chứng kiến nấc tự cứu chạy thật** — lỗi chỉ xuất hiện ngẫu nhiên, và
nạp lại firmware đã vô tình chữa nó. Phải theo dõi tiếp; nếu tái hiện thì log sẽ
có dòng `[NET ]`.

## Còn lại

- [ ] Xem `[NET ]` có xuất hiện khi lỗi mạng tái hiện không.
- [ ] Thử tắt tiếng ở mức NGUY KỊCH rồi để mức **rơi xuống** BÁO ĐỘNG — nhánh
      `lvl_ > mute_lvl_` khi `mute_lvl_` là NGUY KỊCH chưa được chạy qua.
