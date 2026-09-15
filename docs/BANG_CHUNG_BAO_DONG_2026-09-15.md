# Bằng chứng: báo động tại chỗ (còi + đèn) — 15/09/2026

Liên quan: QĐ-028, AC-06.22, AC-06.24 … AC-06.29.
Mã: `VEDCaPhenika/esp32s3_wiseiot_test/alarm.{h,cpp}`, bench `test/alarm_bench/`.

## Chạy thế nào

```bash
arduino-cli compile --fqbn "esp32:esp32:esp32s3:PartitionScheme=default_8MB,FlashSize=16M" \
  -u -p /dev/ttyACM0 test/alarm_bench
```

Không cần pin, không cần cảm biến, không cần còi. `alarm.cpp`/`alarm.h` trong
thư mục bench là **symlink** tới file firmware thật — không phải bản sao, nên
bench không thể kiểm nhầm một đoạn mã đã lỗi thời.

## KQ-01 — Máy trạng thái: 11/11 đạt

```
===== BENCH BAO DONG — kiem may trang thai =====
TH-A: binh thuong
  [DAT] 30 degC, AI im, cam bien du                    mong OK         duoc OK
TH-B: AI thay lech nhung chua du lau
  [DAT] watch=true                                     mong THEO DOI   duoc THEO DOI
TH-C: AI xac nhan bat thuong
  [DAT] ai_alarm=true                                  mong BAO DONG   duoc BAO DONG
TH-D: AI het bao dong thi phai tu ha muc
  [DAT] ai_alarm=false                                 mong OK         duoc OK
TH-E: nguong cung 60 degC — phai kich KE CA khi AI im
  [DAT] 60,2 degC, AI im hoan toan                     mong NGUY KICH  duoc NGUY KICH
TH-F: TRE 5 degC — day la phep kiem quan trong nhat
  [DAT] tut ve 57 degC: VAN phai nguy kich             mong NGUY KICH  duoc NGUY KICH
  [DAT] len lai 59,9 degC: van nguy kich               mong NGUY KICH  duoc NGUY KICH
  [DAT] tut duoi 55 degC: gio moi duoc nha             mong OK         duoc OK
TH-G: mat cam bien — khong duoc bao mau do
  [DAT] t_max=NAN, sensor_bad=true                     mong OK         duoc OK
  (muc van OK nhung ten hien thi: "OK (thieu cam bien)" — den xanh duong nhay)
TH-H: mat cam bien KHONG duoc xoa bao dong dang co
  [DAT] dang BAO DONG roi mat cam bien                 mong BAO DONG   duoc BAO DONG
TH-I: dem su kien
  So lan leo len muc bao dong: 3 (mong doi 3)

===== KET QUA: 11 dat, 0 hong =====
May trang thai DUNG.
```

**TH-E và TH-F là hai phép kiểm đáng giá nhất**, và cũng chính là hai thứ
**không thể kiểm bằng cách hơ nóng cảm biến rồi nhìn đèn**:

- TH-E chứng minh đường ngưỡng cứng không đi qua AI. `ai_alarm=false`,
  `ai_watch=false`, AI im hoàn toàn — vẫn lên NGUY KỊCH. Nghĩa là nếu
  autoencoder có lỗi và không bao giờ báo động, lớp bảo vệ 60 °C vẫn còn.
- TH-F chứng minh trễ 5 °C hoạt động: phải đưa nhiệt độ lên 60,1 rồi hạ xuống
  **đúng** 57,0 và xác nhận còi vẫn kêu. Hơ bằng ngón tay không điều khiển được
  tới mức đó.

## KQ-02 — Tích hợp vào firmware thật

Biên dịch sạch, không cảnh báo:
```
Sketch uses 1058797 bytes (31%) of program storage space.
Global variables use 50992 bytes (15%) of dynamic memory.
```

Chạy trên board, nối được MQTT sau khi sửa IP (xem KQ-04):
```
[MQTT] noi 172.172.5.2:1883 ...
[MQTT] KET NOI OK
```

## KQ-03 — Trạng thái MẤT CẢM BIẾN xảy ra thật, ngoài kế hoạch

Lúc nạp firmware, các đầu dò DS18B20 đang **rút khỏi breadboard**. Hệ báo đúng
và báo to:
```
[TEMP] tim thay 0 cam bien (mong doi 8), 0/8 kenh cell khoe
[TEMP] *** THIEU CAM BIEN - so lieu KHONG day du ***
[TEMP] 0/8 kenh khoe, ti le loi 100.000% | nan(X) nan(X) ... nan(X)
```
Đèn RGB chuyển **xanh dương nháy**, không phải đỏ, không phải xanh lá.

Đây là bằng chứng có giá trị hơn TH-G trong bench, vì nó là sự cố thật chứ
không phải đầu vào do bench bơm vào. Nó cũng xác nhận nhánh sửa lỗi ở QĐ-028:
mất cả bus mà đèn vẫn đổi trạng thái, không đứng hình.

**Việc cần làm:** cắm lại 9 đầu dò để chạy được toàn tuyến.

## KQ-04 — IP máy chủ đã đổi (phát hiện ngoài dự kiến)

```
[WiFi] OK, IP 172.172.10.194
[MQTT] noi 172.172.3.174:1883 ...
[MQTT] that bai, rc=-2 (connect failed - TLS/DNS hong)
```
DHCP của trường cấp lại IP cho máy chủ: `172.172.3.174` → `172.172.5.2`. Đã sửa
`LAN_IP` trong `.env`, `TEST_HOST` trong firmware, dựng lại container mosquitto.
Xác nhận Node-RED và InfluxDB vẫn chỉ nghe trên `127.0.0.1` — đúng ràng buộc 4
của CLAUDE.md. Rủi ro và ba đường xử lý: QĐ-029, **cần người chốt**.

## Còn thiếu gì — nói thẳng

| Việc | Trạng thái |
|---|---|
| Logic 4 mức, trễ, tắt tiếng, mất cảm biến | ✅ đã kiểm bằng bench |
| Đèn báo | ✅ chạy thật, bằng LED RGB trên DevKitC-1 |
| **Còi kêu thành tiếng** | ⬜ còi rời chưa về; `AL_PIN_BUZZER -1` |
| **Nút tắt tiếng bấm thật** | ⬜ đã nối vào nút BOOT nhưng chưa có còi để nghe khác biệt |
| **Ngắt sạc bằng rơ-le/MOSFET** | ⬜ AC-06.23 — hệ vẫn chỉ *cảnh báo*, chưa *hành động* |

Hai dòng cuối là khoảng cách thật giữa "cảnh báo" và "bảo vệ". Đừng nói với
giám khảo là hệ tự ngắt sạc khi nó chưa làm được.
