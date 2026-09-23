# Bằng chứng — Seed Lớp 2 và ba lỗi Lớp 2 (23/09/2026)

Xem QĐ-050. Chạy: `python3 VEDCaPhenika/planb_cloud/seed_layer2.py --clear`.

## Trước khi sửa (InfluxDB, 2 giờ gần nhất)

```
rul  Extrapolating  1
rul  RUL_Cycles     0
rul  SOH_Percent    55.55
```
Panel Grafana Lớp 2 truy vấn `_measurement == "cell"` → không có điểm nào (kết quả nằm ở `rul`).

## Seed

```
da xoa ket qua Lop 2 tu 2026-08-09T05:22:08Z
da gui 60 chu ky B0005 #1..#60, 30 ngay, ket thuc 2026-09-23 04:22 UTC
```
Kết quả mô hình trên 60 chu kỳ seed: `Extrapolating` 0 ở cả 60, `RUL_Cycles`
101,2 → 43,7, `SOH_Percent` 101,8 → 88,9.

Truy vấn qua `/api/ds/query` của Grafana, đúng Flux của từng panel:
```
Lớp 2 — RUL: số chu kỳ còn lại | khung 30d | so diem 60
Mô hình có đang NGOẠI SUY?     | khung 30m | 0
```

## Chip và cloud trên cùng chu kỳ

Trước khi bỏ làm tròn đặc trưng:
```
05:20:14  ONB_SOH_Percent 91.36   SOH_Percent 87.91   (lệch 3,45 điểm)
```
Sau:
```
05:22:53  ONB_RUL 44.54 = RUL 44.54   ONB_SOH 91.39 / SOH 91.46   ngoại suy 0/0
05:23:38  ONB_RUL 44.39 = RUL 44.39   ONB_SOH 91.38 / SOH 91.45   ngoại suy 0/0
```
Serial sau khi nạp: `[RUL ] tren chip: RUL 44.5 chu ky (tho 44.5), SOH 91.4%`, không còn cảnh báo ngoại suy.

## Chỗ AI làm sai / thiếu

- Chưa chụp được màn hình Grafana: Chrome headless bị trang đăng nhập chặn. Đã kiểm
  bằng API thay thế — **người phụ trách phải tự mở dashboard xem**.
- Điểm nối seed → chip: SOH nhảy từ ~88,9 (seed cuối) lên ~91,4 (chip). RUL thì liền
  mạch (43,7 → 44,5). Do đặc trưng mô phỏng trên chip không giống hệt pin B0005.
- Ô "xu hướng" trên app sai ~15 phút đầu (lịch sử cũ trên flash, QĐ-050).
