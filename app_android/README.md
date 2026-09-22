# App Android xem pack pin qua BLE

APK dựng sẵn: **`dist/HuTieuBMS.apk`** (790 KB).

## Vì sao có app này khi đã có bản web

Bản web (`app/index.html`) chạy đúng, nhưng nó phải **tải về từ một máy chủ
trong mạng LAN**. Trên Wi-Fi trường ICTU điện thoại không chạm được tới máy chủ
— đo được, không phải đoán: log máy chủ **không hề có request nào từ IP điện
thoại**, trong khi máy tính tự gọi thì vẫn 200. Mạng chặn máy-nói-với-máy
(client isolation), đội không sửa được.

App này **cài một lần rồi không cần mạng nữa, kể cả lúc mở**. Hôm thi không ai
biết Wi-Fi hội trường thế nào, nên đây mới là đường an toàn.

Bản web **vẫn giữ, không xoá**: nó là phương án cho máy không cài được app, và
cho thấy cùng một GATT phục vụ được hai loại client khác hẳn nhau.

## Cài lên điện thoại

⚠️ **Không tải APK qua Wi-Fi trường** — vướng đúng cái đã chặn bản web.

**Cách 1 — cáp USB (chắc nhất):** bật *Tuỳ chọn nhà phát triển* → *Gỡ lỗi USB*,
cắm cáp, rồi:

```bash
~/Android/Sdk/platform-tools/adb install -r dist/HuTieuBMS.apk
```

**Cách 2 — chép file:** cắm cáp ở chế độ truyền file, kéo `HuTieuBMS.apk` vào
điện thoại, mở bằng trình quản lý file, cho phép *cài từ nguồn không xác định*.

## Build lại

```bash
cd app_android
ANDROID_HOME=$HOME/Android/Sdk JAVA_HOME=$HOME/.local/share/jdk-21 \
  ~/.gradle/wrapper/dists/gradle-8.9-bin/*/gradle-8.9/bin/gradle assembleDebug
```

Không có thư viện ngoài nào nên **build được cả khi mất mạng**. Lý do đầy đủ ở
cuối `app/build.gradle.kts`.

## Ba thứ trong code đáng đọc trước khi sửa

1. **Hàng đợi GATT** (`enqueue`/`opDone`). Android chỉ cho **một** thao tác GATT
   chạy một lúc; gọi cái thứ hai khi cái thứ nhất chưa xong thì nó bị **bỏ im
   lặng** — không lỗi, không ngoại lệ, callback không bao giờ tới. Bật notify
   cho 4 đặc tính bằng 4 lời gọi liên tiếp sẽ chạy đúng 1 cái và 3 ô đứng im,
   trông hệt như firmware không gửi dữ liệu. Đây là lỗi BLE Android phổ biến
   nhất và khó đoán nhất.
2. **`requestMtu(247)` trước khi đọc.** MTU mặc định 23 byte cắt cụt chuỗi mà
   không báo lỗi — dòng nhiệt độ 6 cell sẽ mất đuôi.
3. **Đồng hồ canh dữ liệu cũ.** 15 s không có gói mà vẫn tưởng đang kết nối thì
   băng trên cùng đổi sang đỏ. Màn hình đứng im với số cũ nguy hiểm hơn màn hình
   báo mất kết nối: người thợ sẽ tin là pack đang bình thường.

## Ký bằng khoá debug — cố ý

App chỉ để cài tay cho đội và giám khảo xem, không lên Play Store. Khoá phát
hành đòi quản lý bí mật: thêm một thứ để làm hỏng trước ngày thi mà không đổi
lại được gì.

## Đã kiểm tới đâu

| Việc | Trạng thái |
|---|---|
| Biên dịch, đóng gói APK | ✅ 22/09 |
| Cài và mở được, không crash | ✅ 22/09 (Waydroid) |
| Giao diện vẽ đủ 7 ô, tiếng Việt đúng | ✅ 22/09 (ảnh chụp) |
| Xin quyền đúng lúc | ✅ 22/09 |
| **Nối BLE thật tới ESP32** | ⬜ **CHƯA** — Waydroid không có Bluetooth thật |
| **Nút tắt còi + ghép đôi PIN** | ⬜ **CHƯA** |

⚠️ Hai dòng cuối phải làm trên **điện thoại thật** trước ngày thi. Đường dữ
liệu GATT thì đã kiểm bằng `test/ble_check.py` từ máy tính và đạt, nên phần
chưa chắc là **phía Android**, không phải phía firmware.
