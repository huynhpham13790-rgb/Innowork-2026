# Bằng chứng chạy trên phần cứng thật — 06/09/2026

Lần đầu tiên firmware chạy trên board thật, không phải giả lập. Gỡ được chỗ hở lớn nhất mà `RTM.md` đã ghi nhận.

**Thiết bị:** ESP32-S3 (QFN56) rev v0.2, MAC `7c:e8:b1:b2:7c:d4`
**Nạp qua:** cổng COM, chip CH343 (`1a86:55d3`), cáp USB-A → USB-C
**Board setting:** `PartitionScheme=default_8MB, FlashSize=16M, CDCOnBoot=default` → firmware chiếm 30% của 3MB app
**Mạng:** WiFi `HOANG2` băng 2.4GHz · ESP `192.168.2.17` · broker trên laptop `192.168.2.16`

---

## §1 — Đường truyền thật, từ board tới dashboard

```
===== Hu Tieu | Test duong truyen ESP32-S3 -> WISE-IoT =====
STAGE = 1
[FS  ] LittleFS OK, spool dang co 0 byte
[WiFi] OK  IP=192.168.2.17  RSSI=-36dBm
[NTP] dong bo gio......... OK -> 2026-09-06T15:12:43.847695Z
[MQTT] noi 192.168.2.16:1883 ...
[MQTT] KET NOI OK
[CFG ] 1066 bytes -> OK
[DATA] OK   {"d":{"BatteryPack01":{"Cell01_Temp":30.73,...}},"ts":"2026-09-06T15:12:44.892570Z"}
```

Sau 10 phút chạy liên tục, đếm trong InfluxDB:

```
tổng: 800 điểm
Cell01_Temp .. Cell08_Temp : mỗi cell đúng 100 điểm
```

Đúng 100 điểm mỗi cell ở nhịp 2 giây — **không mất một gói nào**.

→ **AC-01.4, AC-01.6, AC-02.3 đạt.** LittleFS mount đúng ngay từ boot, xác nhận lựa chọn partition scheme ở QĐ-006 là đúng.

## §2 — Store-and-forward: lần đầu THẤT BẠI, và vì sao

Kịch bản: ngắt broker 60 giây rồi bật lại, giữ nguyên board không đụng vào.

### Lần 1 — Serial báo thành công, nhưng dữ liệu mất sạch

```
22:15:08 [DATA] DEM  (spool 221 byte) ...      <- bat dau dem
   ... 33 goi ...
22:16:13 [MQTT] KET NOI OK
22:16:14 [BUFF] co 7307 byte cho day bu
22:16:14 [BUFF] day bu 25 goi, con lai 8
22:16:14 [BUFF] day bu 8 goi, con lai 0        <- "thanh cong"
```

Nhìn Serial thì hoàn hảo: đệm 33 gói, đẩy bù 25 + 8 = 33, còn lại 0. Nhưng query InfluxDB:

```
LỖ HỔNG 15:15:06 -> 15:16:24  (78 giây)
```

**Dữ liệu bay vào hư vô.** Log Node-RED chỉ ra thủ phạm:

```
22:15:08  Disconnected from broker
22:16:23  Connected to broker      <- CHAM HON ESP32 10 GIAY
```

Broker sống lại lúc 22:16:08. ESP32 nối lại sau 5 giây và **lập tức** bắn hết buffer. Nhưng Node-RED mãi 22:16:23 mới nối lại. 33 gói được publish vào một broker **không có ai subscribe** — MQTT QoS 0 không lưu cho subscriber offline, nên broker nhận rồi vứt đi ngay.

Đây là mặt trái nguy hiểm của chính cơ chế store-and-forward: bình thường mất mạng chỉ mất vài gói, nhưng có buffer thì nó **gom cả phút dữ liệu lại rồi bắn hết đúng vào khoảnh khắc mong manh nhất**. Và nó thất bại *im lặng* — phía ESP32 không có cách nào biết, vì QoS 0 không có xác nhận.

Nếu chỉ nhìn Serial mà không đối chiếu Grafana thì đã kết luận "chạy tốt" và mang lên sân khấu.

### Lần 2 — sau khi sửa (QĐ-010)

Thêm `LINK_GRACE_MS = 20000`: nối lại rồi vẫn tiếp tục ghi vào spool thêm 20 giây, hết khoảng đó mới đẩy bù.

```
22:21:50 [MQTT] KET NOI OK
22:21:51 [BUFF] cho 20s cho subscriber noi lai roi moi day bu
22:21:51 [BUFF] co 7068 byte cho day bu
22:22:12 [BUFF] day bu 25 goi, con lai 17
22:22:12 [BUFF] day bu 17 goi, con lai 0
```

Query lại InfluxDB trên cửa sổ bao trùm cả đoạn mất mạng:

```
số điểm: 83   từ 15:20:23 đến 15:23:08 UTC
khoảng trống > 3s: 0
=> LIEN TUC HOAN TOAN
```

**Không còn lỗ hổng nào.** Đoạn mất mạng 60 giây được bù đầy đủ, đúng vị trí thời gian gốc.

→ **AC-03.6, AC-03.7 đạt.** Màn demo 90 giây đã chạy được thật.

## §3 — Test hồi quy

Cơ chế cắt code thật từ `.ino` đã phát huy tác dụng: sau khi sửa firmware, `run_test.sh` **báo lỗi biên dịch ngay** vì thiếu mock cho `millis()` và `linkTrustedAt` — đúng ý đồ thiết kế, test không thể xanh giả khi code đã đổi.

Đã bổ sung TEST 6 làm hồi quy cho chính lỗi ở §2:

```
=== TEST 6: khoang cho subscriber - KHONG day bu qua som ===
  PASS  trong khoang cho -> BAO CHUA SACH, khong day bu
  PASS  KHONG goi mot goi nao ra duong truyen chua tin
  PASS  du lieu van nam nguyen trong spool
  PASS  het khoang cho -> day bu sach
  PASS  day bu du 10 goi, khong mat goi nao

=== TAT CA TEST PASS ===   (22/22)
```

## §4 — Ghi chú phần cứng (xem QĐ-011)

| Tổ hợp | Nguồn | Dữ liệu | Vào mode nạp |
|---|---|---|---|
| Cáp C-to-C + cổng USB | ✅ | ✅ | ❌ phải bấm BOOT+RESET |
| Cáp C-to-C + cổng COM | ❌ | — | — |
| **Cáp A-to-C + cổng COM** | ✅ | ✅ | ✅ **tự động** |

Cổng COM thiếu điện trở CC 5.1k nên cáp C-to-C không cấp nguồn. **Mang đúng sợi A-to-C đi thi.**

Một lỗi tự gây ra đáng ghi lại: lần nạp đầu biên dịch với `CDCOnBoot=cdc` nhưng nạp với `CDCOnBoot=default` — hệ quả là `Serial` của sketch đổ ra cổng USB native trong khi đang nghe ở cổng COM, nên chỉ thấy log ROM bootloader rồi im bặt. Trông y hệt "sketch bị treo". Luôn dùng **cùng một FQBN** cho cả compile và upload.

---

## Còn lại

| Việc | Trạng thái |
|---|---|
| Stack trên VPS công cộng (AC-04.6) | ⬜ chỗ hở lớn nhất còn lại |
| Thử qua WiFi phát từ điện thoại (AC-05.1) | ⬜ |
| Diễn tập trọn kịch bản 5 phút (AC-05.3) | ⬜ |
| Cảm biến nhiệt thật thay cho 8 giá trị giả lập | ⬜ |
| WISE-IoT thật | ⬜ chờ tài khoản |

**Kết luận:** chặng ESP32 → cloud giờ đã chạy **thật, trên phần cứng thật**, gồm cả màn store-and-forward. Và quan trọng hơn con số: lần chạy này tìm ra một lỗi mà không có bài test giả lập nào phát hiện nổi, vì nó nằm ở tương tác thời gian giữa hai tiến trình độc lập.
