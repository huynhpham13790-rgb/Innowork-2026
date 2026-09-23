# Bằng chứng — Lớp 1 trường hợp TH-2 trên chip thật (23/09/2026)

Pack 6S trên bàn, ESP32-S3 `/dev/ttyACM0`, điện trở sứ dán cell 3, **có người
đứng cạnh bàn suốt hai lần chạy**. Log lấy từ Serial (`[DATA]` mỗi 2 s). Dạng:
0 = chưa rõ, 1 = TH-1 nóng vọt, 2 = TH-2 ấm ổn định. Mức: 0 bình thường … 2 báo động.

## Lần 1 — firmware cũ, sưởi bật/tắt quanh +6 °C (xin qua Serial `h`)

Nền 150 s → sưởi 420 s → nguội 150 s. Nóng nhất 38,96 °C, không bị chặn.

| Pha | Mẫu | Dạng | Mức |
|---|---|---|---|
| Nền | 75 | 0: 75 | 0/1/2 (cell 3 còn dư điểm lần sưởi trước) |
| Sưởi | 210 | 1: **151**, 2: 44, 0: 15 | chủ yếu 2 |
| Nguội | 74 | 2: 70, 0: 4 | 2 |

Trong đoạn lệch ≥ +4 °C: 132/160 mẫu là TH-1 — 50 do sưởi bật lại (vọt thật,
nhãn đúng), **82 chỉ do `shock`** (cell đã đứng yên, nhãn sai). → QĐ-049.

```
 130.5s NEN   C3=30.09 diem=0.914 dang=0 lech=+0.00 dtdiff=+0.19 shock=-0.91 muc=0
 250.0s SUOI  C3=38.65 diem=40.245 dang=1 lech=+7.14 dtdiff=+2.94 shock=+5.60 muc=2
 309.1s SUOI  C3=37.84 diem=17.295 dang=1 lech=+6.42 dtdiff=-1.94 shock=+3.89 muc=2
 369.3s SUOI  C3=35.46 diem=3.205 dang=2 lech=+4.46 dtdiff=-1.81 shock=+1.41 muc=2
 429.5s SUOI  C3=35.34 diem=1.189 dang=1 lech=+4.38 dtdiff=+3.50 shock=+1.18 muc=1
 549.7s SUOI  C3=36.09 diem=1.911 dang=2 lech=+4.95 dtdiff=-1.94 shock=+0.86 muc=2
 610.0s NGUOI C3=34.09 diem=1.860 dang=2 lech=+3.31 dtdiff=-1.50 shock=-0.78 muc=2
 670.2s NGUOI C3=32.59 diem=3.911 dang=2 lech=+2.09 dtdiff=-0.94 shock=-1.74 muc=2
```

## Lần 2 — firmware QĐ-049, chế độ ỔN ĐỊNH (xin qua `POST /hutieu/cmd` y như nút trang 1880)

Nền 120 s → giữ ấm 600 s → nguội 120 s. Nóng nhất 35,28 °C, không bị chặn.
Lệnh tới chip: `[MQTT<-] …/cmd : {"cmd":"heat","mode":"steady"}`, ack `"mode":1`.

| Pha | Mẫu | Dạng | Mức |
|---|---|---|---|
| Nền | 51 | 0: 51 | 0: 51 |
| Giữ ấm (cả pha) | 299 | 2: **185**, 1: 99, 0: 15 | 2: 249 |
| Giữ ấm, từ giây 360 | 179 | 2: **162**, 1: 17 | 2: 178 |
| Nguội | 60 | 2: 60 | 0: 54, 1: 6 |

- Lệch khi đã ổn định: **+3,66 … +3,96 °C**.
- TH-1 xuất hiện từ 149 s tới 443 s — đó là lúc cell đang được đẩy lên, nhãn đúng.
- Báo động từ 220 s tới 717 s; điểm giảm dần (8,9 → 1,27) vì nền EMA quen với mức mới.

```
 100.8s NEN   C3=30.03 diem=0.169 dang=0 lech=+0.13 dtdiff=+0.06 shock=+0.00 muc=0
 189.3s GIU   C3=31.59 diem=1.010 dang=1 lech=+1.17 dtdiff=+1.94 shock=+1.22 muc=0
 279.5s GIU   C3=34.71 diem=8.931 dang=1 lech=+3.77 dtdiff=+1.56 shock=+3.12 muc=2
 368.7s GIU   C3=34.65 diem=4.993 dang=2 lech=+3.74 dtdiff=-0.25 shock=+2.25 muc=2
 460.1s GIU   C3=34.84 diem=3.398 dang=2 lech=+3.89 dtdiff=-0.31 shock=+1.78 muc=2
 550.5s GIU   C3=34.78 diem=2.001 dang=2 lech=+3.86 dtdiff=-0.13 shock=+1.30 muc=2
 641.8s GIU   C3=34.78 diem=1.270 dang=2 lech=+3.83 dtdiff=-0.06 shock=+0.93 muc=2
 729.2s NGUOI C3=34.84 diem=1.024 dang=2 lech=+3.87 dtdiff=+0.25 shock=+0.73 muc=0
 819.6s NGUOI C3=33.34 diem=0.866 dang=2 lech=+2.63 dtdiff=-0.94 shock=-0.55 muc=0
```

## Test trên PC (cùng `cell_ai.cpp`, trọng số thật, nhiễu giả định σ = 0,06 °C)

Cell nhảy bậc +4 °C rồi đứng yên: số giây bị gọi TH-1 **126 → 34** sau khi sửa
bảng tra. `test/run_test.sh`: `=== TAT CA TEST PASS ===`.

## Chỗ AI làm sai / thiếu (Definition of Done mục 3)

1. **Báo lỗi sai lúc đầu:** test PC cho rằng nhãn TH-1 chớp ~2 % do nhiễu. Trên
   chip, pha nền 75/75 và 51/51 mẫu sạch — nhiễu giả định cao hơn thực tế. Lỗi
   THẬT là `shock` bám dai, chỉ thấy khi chạy trên chip.
2. **Ước sai tốc độ lên:** tính trần 30 % cho ~1 °C/phút, đo được 1,5–1,9 °C/phút
   → đoạn lên vẫn là TH-1 ~4 phút.
3. **Không tới đích +5 °C:** chỉ khâu tỉ lệ nên đứng ở ~+3,9. Chấp nhận có chủ ý
   (xem QĐ-049), nhưng chú thích `DH_STEADY_DELTA_C` nói "+5" — đọc cùng QĐ-049.

## Việc người phụ trách còn phải làm

- [ ] Tự giải thích được vì sao TH-2 phải chờ ~4 phút và vì sao báo động tự tắt sau ~8 phút giữ.
- [ ] Bấm thử hai nút trên `http://127.0.0.1:1880/hutieu` bằng tay.
