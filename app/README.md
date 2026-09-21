# App xem pack pin qua BLE — chạy trong Chrome trên Android

Một trang web nối **thẳng** tới ESP32 bằng Bluetooth. Không cài đặt, không Play
Store, không máy chủ trung gian. Sửa gì thì tải lại là xong.

## Mở thế nào

1. Điện thoại và máy chủ **cùng một mạng Wi-Fi** (chỉ cần để TẢI trang).
2. Mở Chrome, vào `http://<IP máy chủ>:1880/hutieu/app` — hiện tại là
   **`http://192.168.2.16:1880/hutieu/app`**. IP đổi theo mạng, xem bằng
   `hostname -I`.
3. **Khai báo origin tin cậy** (làm một lần trên mỗi điện thoại):
   - Vào `chrome://flags/#unsafely-treat-insecure-origin-as-secure`
   - Dán `http://192.168.2.16:1880` vào ô, chọn **Enabled**
   - Bấm **Relaunch**
4. Quay lại trang → **Kết nối tới pack** → chọn `HuTieu-BMS`.

Từ lúc kết nối xong, **dữ liệu không đi qua mạng nữa** — tắt Wi-Fi và 4G vẫn
chạy. Mạng chỉ cần để tải trang.

## Vì sao phải khai báo origin

Web Bluetooth đòi "ngữ cảnh an toàn": **HTTPS, hoặc localhost, hoặc origin được
khai báo tin cậy**. Trang đang phục vụ qua HTTP trong mạng LAN nên không đạt,
phải khai báo thủ công. Mở bằng `file://` cũng **không** chạy.

Muốn bỏ bước này thì đưa trang lên một nơi có HTTPS (ví dụ GitHub Pages) — lúc
đó mở là dùng được ngay, không phải đụng `chrome://flags`. Chưa làm vì đó là
**đăng công khai**, cần người quyết.

## Giới hạn phải biết trước

- **Chỉ Android.** iOS Safari **không có** Web Bluetooth và Apple chưa có ý
  định thêm. Trên iPhone bắt buộc phải viết app Swift thật. Trang này sẽ báo
  thẳng điều đó thay vì im lặng không chạy.
- Bán kính khoảng 10 m, không xuyên tường tốt.
- Tắt tiếng còi cần **ghép đôi + mã PIN** ở lần bấm đầu tiên (QĐ-042).

## Ba màn hình, đừng nhầm vai

| Màn | Phục vụ ai | Cần mạng? |
|---|---|---|
| Grafana `:3000` | Quản lý đội xe | Có — và **đây là phần được chấm điểm** |
| Node-RED `/hutieu` | Người trình diễn, điều khiển sưởi | Có |
| **App này** | Thợ / chủ xe đứng cạnh pack | **Không** |

## Lớp 2 (RUL/SOH) không có ở đây — cố ý

RUL/SOH được tính **trên cloud**, và hồ sơ đã nộp ghi đúng như vậy
(*"remaining-useful-life prediction in the cloud"*). Đưa nó xuống thiết bị sẽ
mâu thuẫn với văn bản đã gửi, và làm mỏng đi phần WISE-IoT — mà
`KHUNG_NGHIEN_CUU_v2` §1 nói rõ điểm thi phụ thuộc lớn vào mức độ dùng WISE-IoT.

Về mặt sử dụng thì cũng không thiệt: RUL là thông tin **bảo dưỡng**, không phải
thông tin khẩn cấp. Thứ cần ngay khi đứng cạnh pack là *cell nào đang có vấn
đề* — và Lớp 1 chạy hoàn toàn trên thiết bị, có sẵn ở đây.
