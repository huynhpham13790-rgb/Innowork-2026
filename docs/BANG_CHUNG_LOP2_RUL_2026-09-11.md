# Bằng chứng — Lớp 2: Dự báo tuổi thọ pin (RUL/SOH), 11/09/2026

Dự báo **số chu kỳ còn lại tới khi pin còn 80% dung lượng**, chạy trên cloud,
1 lần mỗi chu kỳ sạc.

**Kết luận ngắn: mô hình đơn giản nhất thắng, và thắng cách biệt.** Chi tiết và
lý do ở §3 — đây là phần đáng kể nhất của báo cáo này.

---

## §1 — Dữ liệu và cách đặt bài toán

**NASA PCoE Battery Data Set** (chuẩn ngành), 4 pin 18650 ở 24 °C:

| Pin | Chu kỳ dùng được | Dung lượng ban đầu | Tuổi thọ tới mốc 80% |
|---|---|---|---|
| B0005 | 105 | 1,835 Ah | 105 chu kỳ |
| B0006 | 60 | 2,013 Ah | 60 chu kỳ |
| B0007 | 123 | 1,881 Ah | 123 chu kỳ |
| B0018 | 77 | 1,840 Ah | 77 chu kỳ |

Tổng **364 chu kỳ**.

### Chỉ dùng đặc trưng của PHA SẠC — và vì sao

Không dùng dung lượng phóng làm đầu vào, vì hai lý do:

1. **Rò rỉ đáp án:** nhãn RUL được tính TỪ dung lượng. Đưa dung lượng vào đầu
   vào là cho mô hình nhìn trộm.
2. **Không dùng được ngoài đời:** đo dung lượng thật đòi hỏi phóng kiệt pin
   theo dòng cố định — việc không bao giờ xảy ra với xe điện đang chạy.

Pha sạc thì ngược lại: ngày nào người dùng cũng cắm sạc, và ESP32 đo được đủ
điện áp, dòng, nhiệt độ suốt quá trình. Câu chuyện sản phẩm vì thế là **"đoán
tuổi thọ pin từ cách nó sạc"** — dùng được thật.

**Một bẫy đã gặp:** thời gian sạc CC thô KHÔNG dùng được. Đo ra 11 phút ở chu
kỳ này và 38 phút ở chu kỳ khác, nhưng không phải do lão hoá — mà do pin còn
bao nhiêu lúc cắm sạc. Nên mọi đặc trưng thời gian đều đo trên **khoảng điện áp
cố định** (3,9 → 4,15 V), tính từ lúc pin đạt mốc điện áp chứ không từ lúc cắm.

### 9 đặc trưng, tương quan với RUL

```
t_cv             r=-0.860  #########################   <- pha CV dài ra khi pin già
v_start          r=-0.607  ##################
t_v_interval     r=+0.481  ##############
dvdt_cc          r=-0.429  ############
i_cv_mean        r=+0.397  ###########
T_rise_ch        r=-0.235  #######
t_charge_total   r=-0.213  ######
T_mean_ch        r=+0.143  ####
T_max_ch         r=-0.096  ##
```

`t_cv` mạnh nhất và đúng vật lý: pin càng chai thì càng lâu "no" ở pha giữ áp.

## §2 — Giao thức đánh giá: leave-one-battery-out

Train trên 3 pin, thử trên pin thứ 4 **chưa từng thấy**. Đây là cách duy nhất
trung thực khi chỉ có 4 pin.

Chia ngẫu nhiên theo chu kỳ sẽ cho điểm rất đẹp và hoàn toàn vô nghĩa: chu kỳ
40 và 41 của cùng một pin gần như giống hệt nhau, mô hình chỉ việc nội suy.
Ngoài đời luôn là "một viên pin mới chưa từng thấy", nên phải thử đúng như vậy.

## §3 — Kết quả: mô hình đơn giản thắng

Sai số tính bằng **số chu kỳ** (MAE, càng nhỏ càng tốt):

| Phương pháp | B0005 | B0006 | B0007 | B0018 | **TB** |
|---|---|---|---|---|---|
| B1 trung bình (baseline ngu) | 24,0 | 27,2 | 30,3 | 21,6 | 25,8 |
| **B2 tuyến tính, chỉ `t_cv`** | 15,7 | **6,4** | 16,1 | 10,4 | **12,2** |
| B3 Ridge 9 đặc trưng | 16,9 | 32,4 | 18,1 | 40,9 | 27,0 |
| B4 Gradient Boosting | 8,9 | 35,7 | 12,9 | 11,4 | 17,2 |
| M LSTM cửa sổ 10 chu kỳ | 6,3 | 42,8 | 16,1 | 15,8 | 20,3 |

**Hồi quy tuyến tính trên đúng MỘT đặc trưng đánh bại LSTM, Gradient Boosting
và Ridge.** Ridge 9 đặc trưng thậm chí còn tệ hơn baseline "đoán bừa trung
bình".

### Vì sao — đã kiểm chứng, không phải phỏng đoán

Bốn pin có tuổi thọ rất khác nhau: 60, 77, 105, 123 chu kỳ. Kiểm tra từng pin
xem tuổi thọ của nó có nằm trong dải của 3 pin train không:

```
B0005: 105 | 3 pin train [60, 77, 123]  -> TRONG dải
B0006:  60 | 3 pin train [77, 105, 123] -> NGOÀI dải   <-- ngắn hơn mọi pin train
B0007: 123 | 3 pin train [60, 77, 105]  -> NGOÀI dải
B0018:  77 | 3 pin train [60, 105, 123] -> TRONG dải
```

**B0006 là pin ngắn nhất và nằm ngoài dải — và đúng là chỗ mọi mô hình phức tạp
sụp đổ** (LSTM 42,8 · GBM 35,7 · Ridge 32,4 · tuyến tính 6,4).

Nguyên nhân rõ ràng khi nhìn số tham số so với lượng dữ liệu:

| Mô hình | Tham số | Dữ liệu để học |
|---|---|---|
| Tuyến tính 1 đặc trưng | **2** | 3 pin |
| Ridge 9 đặc trưng | 10 | 3 pin |
| LSTM | **~3.500** | 3 pin |

LSTM có 3.500 tham số để học từ 3 viên pin. Nó không học "pin già thì pha CV
dài ra" — nó học thuộc tuổi thọ của đúng 3 viên đó. Gặp viên thứ tư ngắn hơn
tất cả, nó ngoại suy sai nặng.

### Sai số dự đoán SỚM (ở 25–50% vòng đời) — con số quan trọng nhất thương mại

| Phương pháp | TB |
|---|---|
| **B2 tuyến tính `t_cv`** | **9,8 chu kỳ** |
| B4 Gradient Boosting | 16,4 |
| B1 trung bình | 17,7 |
| M LSTM | 17,8 |
| B3 Ridge | 21,6 |

Ở nửa đầu vòng đời, mô hình tuyến tính sai trung bình **~10 chu kỳ**. Đây mới là
lúc dự báo còn kịp có ích — biết trước để lên lịch thay pin.

## §4 — SOH (tỉ lệ dung lượng còn lại)

Ridge dự đoán SOH, cùng giao thức leave-one-battery-out:

| Pin | RMSE |
|---|---|
| B0005 | 3,8 điểm phần trăm |
| B0006 | 3,2 |
| B0007 | 4,3 |
| B0018 | 3,9 |
| **Trung bình** | **3,8 điểm phần trăm** |

Đáng chú ý: SOH dự đoán **tốt hơn hẳn RUL một cách tương đối**, vì SOH không
phụ thuộc tuổi thọ tổng của viên pin — nó chỉ là "còn bao nhiêu phần trăm ngay
lúc này". Với khách hàng định giá xe cũ hay thẩm định pin, SOH mới là con số họ
cần, và nó đáng tin hơn.

## §5 — Kết luận và giới hạn

**Chọn dùng:** hồi quy tuyến tính trên `t_cv` cho RUL, Ridge cho SOH. Cả hai
chạy trên cloud, vài mili-giây, không cần GPU.

**Nói thẳng những gì chưa biết:**

| Giới hạn | Hệ quả |
|---|---|
| Chỉ có **4 viên pin** | Mọi kết luận về "mô hình nào tốt hơn" chỉ đúng ở quy mô này. Có 50 viên thì LSTM rất có thể thắng — kết luận ở đây là *"với dữ liệu chúng em có"*, không phải *"LSTM vô dụng"*. |
| Pin 18650 đơn, thí nghiệm trong phòng | Chưa phải pack 8 cell trên xe thật, chưa có nhiệt độ môi trường thay đổi. |
| Sai số ~12 chu kỳ trên tuổi thọ 60–123 | Tương đối là 10–20%. Đủ để lên lịch thay pin, **không đủ** để hứa chính xác ngày hỏng. |
| Chưa kiểm chéo trên CALCE / UPC | Bộ UPC đã tải sẵn (410 chu kỳ, có suy giảm dung lượng) — là việc tiếp theo nên làm. |

**Bài học chung với Lớp 1:** cả hai lớp đều phải so với baseline đơn giản, và
cả hai lần baseline đều dạy được điều gì đó. Lớp 1: autoencoder thắng ở ca nguy
hiểm nhưng thua ở trôi chậm. Lớp 2: mô hình phức tạp **thua hẳn**. Không chạy
baseline thì đã mang LSTM lên sân khấu và sai 43 chu kỳ trên viên pin yếu nhất.
