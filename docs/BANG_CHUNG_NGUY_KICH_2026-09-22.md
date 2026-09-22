# Bằng chứng: tắt tiếng ở mức NGUY KỊCH — 22/09/2026 (QĐ-045)

## Thử thế nào, và vì sao KHÔNG nung cell lên 60 °C

Mức NGUY KỊCH cần `t_max >= AL_T_CRIT`. Thay vì nung một cell thật lên 60 °C,
đội **hạ tạm ngưỡng xuống 27,0 °C** (nhiệt độ phòng 27,3 °C) rồi nạp bản thử.

Đường đi trong code **giống hệt** — cùng `crit_latched_`, cùng `mute_gate`,
cùng hẹn giờ. Chỉ một hằng số khác. Đổi lại: không phải nung gì, không có rủi
ro nhiệt, và thử được ngay.

⚠️ Sau khi thử **đã trả ngưỡng về 60,0 / 55,0 °C**, nạp lại, và xác nhận qua
MQTT: `Alarm_Level = 0`, cao nhất 27,73 °C.

## Kết quả — cả hai vế đều chạy trên phần cứng thật

```
26.5s  [ALRM] OK -> NGUY KICH
26.5s  [ALRM] coi: KEU

107.9s [ALRM] TAT TIENG coi (lenh tu xa)
107.9s [BLE ] lenh "mute on"
108.0s [ALRM] coi: IM                       ← tắt được ở mức NGUY KỊCH

103.9s [ALRM] het han tat tieng o muc NGUY KICH - coi bat lai
103.9s [ALRM] coi: KEU                      ← tự bật lại sau 2 phút
```

App hiện băng **đỏ `NGUY KỊCH — vượt ngưỡng cứng`** kèm `🔇 Còi: ĐÃ TẮT`.

**Dòng `[ALRM] coi: KEU/IM` là thứ làm bài kiểm này có giá trị.** Không có nó,
"còi có thật sự im không" chỉ suy được từ logic — mà logic tắt tiếng chính là
chỗ đã sai ba lần ngày 21/09, và cả ba lần đều trông đúng khi đọc code.

---

## Hai lỗi của AI trong buổi này

### (a) Nghi oan cho phần cứng

Thấy `Alarm_Muted = 1` mà không có lệnh nào trong log, AI kết luận đây là "cú
nhấn ma từ cổng serial" và bắt đầu dựng bài đo để truy. **Sai** — người dùng vừa
bấm tắt bằng tay. Cửa sổ log AI nhìn chỉ đơn giản là bắt đầu sau lúc bấm.

Bài học: *"không thấy trong log"* không đồng nghĩa *"không xảy ra"* — phải hỏi
trước khi dựng giả thuyết về lỗi phần cứng. Phép đo AI chạy để kiểm cũng cho
kết quả **âm tính** (mở/đóng cổng serial 1 s, `muted` vẫn `0`), nên nếu đọc kỹ
kết quả đó thì đã không cần đoán tiếp.

### (b) Chip hứa sai với người dùng

Chip trả lời `"da tat tieng - TU BAT LAI sau 5 phut"` rồi **tự bật lại sau đúng
2 phút**. Câu đó đúng cho mức BÁO ĐỘNG nhưng sai ở mức NGUY KỊCH, và
`CmdCb::onWrite` không biết mức hiện tại.

Đây là lời hứa sai với người đang đứng cạnh pack đang cháy: họ tưởng còn 5 phút
yên tĩnh. Đã sửa thành câu chung chung, và đồng hồ đếm lùi thật lấy từ `ttl`
trong đặc tính `000A`.

---

## Còn lại

- [ ] Thử ở **60 °C thật** bằng sưởi, để xác nhận chính con số ngưỡng (bài này
      xác nhận *đường đi*, không xác nhận *ngưỡng*).
- [ ] Đo xem còi SFM-27 có thật sự im về **0 dB** hay chỉ nhỏ đi — log chỉ chứng
      minh chân GPIO5 đã xuống LOW.
