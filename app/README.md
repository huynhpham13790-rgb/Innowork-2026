# App xem pack pin qua BLE — chạy trong Chrome trên Android

Một trang web nối **thẳng** tới ESP32 bằng Bluetooth. Không cài đặt, không Play
Store, không máy chủ trung gian. Sửa gì thì tải lại là xong.

## Mở thế nào

1. Điện thoại và máy chủ **cùng một mạng Wi-Fi** (chỉ cần để TẢI trang).
2. Trên máy chủ, phục vụ thư mục này ra LAN:

   ```bash
   cd app && python3 -m http.server 8088 --bind $(ip -4 -br addr show wlo1 | awk '{print $3}' | cut -d/ -f1)
   ```

   ⚠️ **ĐỪNG mở cổng 1880 của Node-RED ra LAN để lấy trang này.** Bản cũ của
   file hướng dẫn đúng như vậy và nó **vi phạm ràng buộc cứng #4** trong
   `CLAUDE.md`: ai vào được 1880 là deploy được function node, tức chạy code
   tuỳ ý trên máy chủ, mà Node-RED không có mật khẩu mặc định. Trang này là
   **file tĩnh thuần**, không gọi mạng lần nào — phục vụ riêng là đủ và an
   toàn.

3. Mở Chrome, vào `http://<IP máy chủ>:8088/` — xem IP bằng `hostname -I`
   (đổi theo mạng, nên đừng chép cứng vào slide).
4. **Khai báo origin tin cậy** (làm một lần trên mỗi điện thoại):
   - Vào `chrome://flags/#unsafely-treat-insecure-origin-as-secure`
   - Dán `http://<IP máy chủ>:8088` vào ô, chọn **Enabled**
   - Bấm **Relaunch**
5. Quay lại trang → **Kết nối tới pack** → chọn `HuTieu-BMS`.

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

## Lớp 2 (RUL/SOH) — ĐÃ có ở đây, từ QĐ-043

Mục này từng ghi *"Lớp 2 không có ở đây — cố ý"*, và điều đó **hết đúng** từ
khi chip tự tính được RUL/SOH: app đọc đặc tính BLE `0009` nên có số **kể cả
khi mất mạng**. Cloud vẫn tính song song, kết quả trên chip đi kèm dưới tiền tố
`ONB_*` để lệch nhau là nhìn ra ngay.

⚠️ Lý do "làm mỏng phần WISE-IoT nên mất điểm" từng được viết ở đây cũng **SAI** —
barem thật không chấm nền tảng cloud (`docs/BAREM_CHAM_BAN_KET.md`).

**Đọc con số này phải kèm cảnh báo.** Hệ số đang là của NASA (18650 đơn,
2,0 Ah) và chu kỳ sạc vẫn mô phỏng, nên app hiển thị cờ ngoại suy **trước** con
số chứ không phải dưới nó. RUL là thông tin **bảo dưỡng**, không phải thông tin
khẩn cấp — thứ cần ngay khi đứng cạnh pack vẫn là *cell nào đang có vấn đề*,
và đó là Lớp 1.
