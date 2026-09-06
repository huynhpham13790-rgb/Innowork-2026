# InnoWorks 2026 — Đội Hủ Tiếu

Hệ giám sát an toàn pack pin xe điện: ESP32-S3 đo nhiệt độ cell → phát hiện bất thường on-device → đẩy lên cloud qua giao thức WISE-PaaS của Advantech → dashboard.

**Bán kết: 15/09/2026.**

---

## Bắt đầu từ đâu

| Bạn là | Đọc file này trước |
|---|---|
| Thành viên mới trong đội | `VEDCaPhenika/KHUNG_NGHIEN_CUU_v2_HuTieu.md` |
| Người sắp sửa code | `CLAUDE.md` → `docs/MODULE_MAP.md` |
| AI / trợ lý lập trình | `CLAUDE.md` |
| Người muốn biết dự án đang tới đâu | `docs/RTM.md` |

## Cấu trúc

```
CLAUDE.md                  Giao diện điều khiển AI: ràng buộc cứng, gate, Definition of Done
docs/
  DATA_CONTRACT.md         Hợp đồng dữ liệu ESP↔cloud — ranh giới không được tự ý phá
  MODULE_MAP.md            Chức năng nằm ở file nào
  TEST_VA_ACCEPTANCE.md    Thế nào là "chạy đúng"
  DECISION_LOG.md          Đã cân nhắc gì, chọn gì, vì sao
  RTM.md                   Requirement → code → test → bằng chứng
  BANG_CHUNG_KIEM_THU_*.md Bằng chứng đã chạy thật
VEDCaPhenika/
  esp32s3_wiseiot_test/    Firmware ESP32-S3 + test store-and-forward chạy trên PC
  planb_cloud/             Stack cloud dự phòng (Docker): Mosquitto + Node-RED + InfluxDB + Grafana
  *.md                     Nghiên cứu, phần cứng, lộ trình, Plan B
  *.docx *.xlsx            Đề xuất dự thi, danh sách linh kiện
```

## Chạy thử nhanh

**Test logic store-and-forward** (không cần board, ~10 giây):
```bash
cd VEDCaPhenika/esp32s3_wiseiot_test && ./test/run_test.sh
```

**Dựng tầng cloud:** làm theo `VEDCaPhenika/PLAN_B_CLOUD.md`. Nhớ tạo `mosquitto/passwd` **trước** khi `docker compose up`, thiếu là container chết ngay mà phía ESP32 chỉ báo `rc=-2` chung chung.

## Trạng thái

Chặng ESP32 → cloud đã thông và đã kiểm chứng ở mức logic + cloud: dữ liệu đúng contract vào được InfluxDB, timestamp do thiết bị quyết định, mất mạng không mất dữ liệu, lỗi ghi không còn im lặng.

**Chưa có lần nào chạy trên phần cứng thật.** Việc quan trọng nhất còn lại: nạp board và diễn tập màn rút mạng 60 giây (`docs/RTM.md`).

## Quy ước

Tài liệu và comment viết bằng tiếng Việt. Comment giải thích **tại sao**, không phải **cái gì**. Sửa code là cập nhật tài liệu trong cùng commit — chi tiết trong `CLAUDE.md`.
