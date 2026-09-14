# CLAUDE.md — giao diện điều khiển AI cho dự án Hủ Tiếu

File này là thứ AI đọc đầu tiên. Nó không mô tả sản phẩm (đã có `VEDCaPhenika/KHUNG_NGHIEN_CUU_v2_HuTieu.md`), nó nói **AI được phép làm gì, phải dừng ở đâu, và cái gì là đúng**.

---

## Dự án là gì

Hệ giám sát an toàn pack pin xe điện cho cuộc thi **AIoT InnoWorks 2026**. ESP32-S3 đọc nhiệt độ cell → phát hiện bất thường on-device → đẩy lên cloud qua giao thức WISE-PaaS của Advantech → dashboard.

Bán kết: **26/09/2026**. Mọi quyết định kỹ thuật đều bị chi phối bởi mốc này.

## Bản đồ tài liệu — đọc cái nào khi nào

| Cần biết | Đọc |
|---|---|
| Sản phẩm giải bài toán gì, kiến trúc AI 3 lớp | `VEDCaPhenika/KHUNG_NGHIEN_CUU_v2_HuTieu.md` |
| Mua gì, nguồn điện, đấu nối | `VEDCaPhenika/PHAN_CUNG_VA_KIEN_TRUC_HuTieu.md` |
| Ai làm gì, deadline nào | `VEDCaPhenika/InnoWorks2026_Lo_trinh_Doi_HuTieu.md` |
| **Ranh giới không được tự ý phá** | `docs/DATA_CONTRACT.md` |
| Code nằm ở đâu | `docs/MODULE_MAP.md` |
| Thế nào là "chạy đúng" | `docs/TEST_VA_ACCEPTANCE.md` |
| Vì sao chọn hướng hiện tại | `docs/DECISION_LOG.md` |
| Ai dùng, dùng lúc nào, chọn kênh truyền nào | `docs/NGUOI_DUNG_VA_KICH_BAN.md` |
| Requirement nối tới code/test ở đâu | `docs/RTM.md` |
| Phương án cloud dự phòng | `VEDCaPhenika/PLAN_B_CLOUD.md` |

---

## Ràng buộc cứng — vi phạm là hỏng cả bài dự thi

1. **KHÔNG đổi format payload MQTT.** Topic `/wisepaas/scada/{nodeId}/data` và payload `{"d":{...},"ts":...}` là tài sản lớn nhất của đội trước giám khảo Advantech. Mọi phương án bắt sửa format đều bị loại. Chi tiết: `docs/DATA_CONTRACT.md`.
2. **KHÔNG bỏ ghim version image** trong `planb_cloud/docker-compose.yml`. Ba tag đã ghim đều có lý do ghi ngay tại chỗ (`eclipse-mosquitto:2.0.22`, `influxdb:2.7`, `nodered/node-red:4.1`). Nâng lên là hỏng.
3. **KHÔNG commit secret.** `.env`, `mosquitto/passwd`, credential key WISE-IoT. `.gitignore` đã chặn — đừng dùng `git add -f` để lách.
4. **KHÔNG mở Node-RED (1880) hay InfluxDB (8086) ra 0.0.0.0.** Node-RED không có mật khẩu mặc định; mở ra là mất máy chủ. Chỉ 1883 (MQTT, có auth) và 3000 (Grafana, có auth) được ra ngoài.
5. **KHÔNG dùng `ts` do server tự đóng dấu.** Timestamp phải do ESP32 sinh, vì màn demo store-and-forward phụ thuộc vào nó.

## Gate — phải dừng lại hỏi người

AI tự làm được: sửa code, viết test, viết/cập nhật tài liệu, dựng và chạy stack cloud thử nghiệm ở local.

**Phải hỏi trước khi làm:**
- Mua sắm, tiêu tiền, đăng ký dịch vụ trả phí.
- Đẩy code lên GitHub (`git push`) — người phải xem diff trước.
- Thay đổi kiến trúc đã chốt trong `docs/DECISION_LOG.md`.
- Bất cứ thứ gì chạm vào pack pin thật hoặc mạch sạc — sai là cháy nổ, không phải bug.
- Liên hệ ra ngoài đội (Advantech, BTC, nhà tài trợ).

## Definition of Done — theo `skills.txt`

Một việc **chưa xong** nếu chỉ có "AI đã sinh ra code/tài liệu". Xong nghĩa là:

1. Chạy được, và **có bằng chứng chạy được** (log, kết quả test, ảnh chụp) ghi vào `docs/BANG_CHUNG_KIEM_THU_*.md`.
2. Người phụ trách giải thích được nó làm gì.
3. Người phụ trách chỉ ra được **ít nhất một chỗ AI làm sai hoặc thiếu**, và đã sửa.
4. Tài liệu liên quan đã được cập nhật ngược trong **cùng một commit** — đây là bước hay bị bỏ nhất, và bỏ nó là tài liệu chết.

## Quy ước làm việc

- Viết tài liệu và comment code bằng **tiếng Việt**. Giám khảo và cả đội đều đọc tiếng Việt; comment tiếng Anh nửa vời chỉ làm chậm mọi người.
- Comment giải thích **tại sao**, không phải **cái gì**. `// tăng i lên 1` là rác; `// ghim 2.0.22 vì 2.1 bỏ password_file` là tài sản.
- Sửa code có logic đệm/truyền dữ liệu → chạy lại `VEDCaPhenika/esp32s3_wiseiot_test/test/run_test.sh`.
- Mỗi quyết định kỹ thuật đáng kể → thêm một dòng vào `docs/DECISION_LOG.md`.
