# Bằng chứng: Lớp 2 (RUL/SOH) chạy trên chip thật — 22/09/2026

Kiểm trên ESP32-S3 (`/dev/ttyACM0`, CH343), firmware build từ commit Lớp 2.
FQBN: `esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=default,PSRAM=opi,FlashSize=16M,PartitionScheme=default_8MB`

> ⚠️ Thiếu `PartitionScheme=default_8MB` thì build **trượt** với
> `text section exceeds available space in board`. FQBN ghi trong
> `docs/MODULE_MAP.md` §nạp `hw_bringup_6s` không có tham số này — đó là FQBN
> của sketch khác, đừng chép sang đây.

Kích thước: **1.377.643 byte (41%)** flash, **58.080 byte (17%)** RAM.

---

## 1. Mô hình chạy được trên chip, không cần mạng

```
[RUL ] lich su: 0 chu ky da luu
[RUL ] tren chip: RUL 85.2 chu ky (tho 85.2), SOH 94.0%
[CYC ] chu ky sac #1 xong sau 5256s ao -> gui 9 dac trung
```

`RUL tho` bằng `RUL` → chưa bị chặn về 0. Không có cờ `NGOAI DAI HUAN LUYEN`
→ cả 9 đặc trưng nằm trong ±3 độ lệch chuẩn của tập train.

## 2. Kết quả trên chip lên tới cloud, cạnh kết quả cloud tự tính

Bắt trực tiếp trên MQTT (`mosquitto_sub`, topic `/wisepaas/scada/+/data`):

```json
{"d":{"BatteryPack01":{"CYC_t_cv":2614, ... ,"ONB_RUL_Cycles":79.57,
"ONB_SOH_Percent":93.67,"ONB_Extrapolating":0,"ONB_Cycles_Logged":2}},
"ts":"2026-09-22T02:14:52.361504Z"}
```

Đúng data contract cũ — vẫn `{"d":{...},"ts":...}`, chỉ thêm tag. Node-RED
không phải sửa gì về đường truyền.

**`ONB_SOH_Slope` vắng mặt ở gói `Cycles_Logged=2`, xuất hiện ở gói `=3`** với
giá trị `-0.3763`. Đây đúng là chốt chặn `got < 3 → NAN` trong
`rulHistSohSlope()`: hai điểm thì luôn thẳng hàng nên độ dốc vô nghĩa. Quan
sát được từ ngoài, không phải tin vào code.

## 3. Lịch sử sống qua mất điện

Reset cứng bằng chân RTS rồi đọc lại log khởi động:

```
[FS  ] LittleFS OK, spool dang co 0 byte
[RUL ] lich su: 7 chu ky da luu
```

Trước reset là 0. Đây là thứ phân biệt "chip tính được RUL" với "chip theo dõi
được pin qua cả đời nó" — không có bước này thì mọi con số xu hướng đều mất khi
rút điện.

## 4. Chip và cloud ra cùng một số

`python3 ai/test_rul_c_vs_js.py` — biên dịch `rulPredict()` trên PC, so với bản
dịch của `rul_predict.js`, trên 500 chu kỳ ngẫu nhiên **cố ý gồm cả ca ngoài
dải huấn luyện** (chỉ thử điểm ở giữa thì không bao giờ chạm nhánh ngoại suy):

```
  lech RUL lon nhat : 0.000032 chu ky
  lech SOH lon nhat : 0.000066 diem phan tram
  co ngoai suy lech : 0 truong hop
DAT — chip va cloud ra cung mot so
```

---

## Chỗ AI làm sai, người soi ra

Log khởi động BLE vẫn in **`CHI DOC, khong co lenh ghi nao`** — câu đó đã thành
sai từ lúc thêm đặc tính lệnh 0008 (tắt còi), nhưng không ai sửa vì nó vẫn
biên dịch và chạy bình thường. Log sai kiểu này nguy hiểm hơn không log: người
soi bảo mật đọc câu đó sẽ kết luận thiết bị không có bề mặt ghi nào rồi dừng
kiểm. Đã sửa thành mô tả đúng (7 đặc tính chỉ đọc + 1 đặc tính lệnh, ghi đòi
ghép đôi có PIN) và nạp lại.

## Chưa chứng minh được, đừng nói quá khi thi

- **Chu kỳ sạc vẫn là mô phỏng** (`chu ky sac #1 ... 5256s ao`) vì chưa có bộ
  sạc và shunt. Chuyển tính toán xuống chip làm nó chạy **offline**, KHÔNG làm
  nó **thật hơn**. Trên sân khấu phải nói đúng như vậy.
- **Hệ số vẫn là của NASA** (18650 đơn, 2,0 Ah). Pack đội 2,55 Ah — con số
  tuyệt đối chưa có ý nghĩa cho pack này; cái đã kiểm là *đường đi của dữ liệu*
  và *sự khớp nhau giữa hai nơi tính*.
