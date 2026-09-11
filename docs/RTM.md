# RTM — Requirement → Code → Test → Bằng chứng

Bảng này để trả lời hai câu hỏi nhanh: *"sửa chỗ này thì ảnh hưởng tới đâu?"* và *"cái này đã có ai kiểm chưa?"*

`skills.txt` gọi đây là traceability coverage — chỉ số quan trọng hơn số dòng tài liệu.

---

## Phạm vi hiện tại

RTM này mới phủ **chặng ESP32 → cloud**. Các phần Edge AI (autoencoder on-device), phần cứng đo nhiệt, và ứng dụng người dùng chưa có code nên chưa vào bảng — xem `VEDCaPhenika/KHUNG_NGHIEN_CUU_v2_HuTieu.md`.

| REQ | Yêu cầu | Code | Test | Bằng chứng | Trạng thái |
|---|---|---|---|---|---|
| REQ-01 | ESP32 gửi dữ liệu đúng giao thức WISE-PaaS, không phải sửa khi đổi cloud | `.ino` → `buildTopics()`, `publishData()`, `publishConfig()` | AC-01.3, AC-01.5 | BC §1, §3 | ✅ |
| REQ-02 | Cloud nhận, lưu và vẽ được dữ liệu | `planb_cloud/` toàn bộ | AC-01.1→1.4 | BC §1, BC2 §1 | ✅ |
| REQ-03 | Timestamp do thiết bị quyết định, cloud tôn trọng | `isoTimestampUtc()`, `convert.js`, `precision=ns` | AC-02.1 | BC §2 | ✅ |
| REQ-04 | NTP hỏng không được làm hỏng dữ liệu một cách im lặng | `timeIsValid()`, chặn mốc `1700000000000` trong `convert.js` | AC-02.2, AC-02.3 | BC §2, §4 | ✅ phía cloud · 🔶 phía board |
| REQ-05 | Mất mạng không mất dữ liệu; nối lại đẩy bù đúng thứ tự, đúng thời điểm gốc | `spoolAppend/Flush/Compact()`, `loop()` không chặn | AC-03.1→3.6 | BC §3, BC2 §2 | ✅ **đã chạy trên board thật** |
| REQ-12 | Không đẩy bù khi subscriber chưa sẵn sàng | `LINK_GRACE_MS`, `linkTrustedAt` | AC-03.7, TEST 6 | BC2 §2 | ✅ |
| REQ-06 | Đệm không được làm nghẽn vòng lấy mẫu | `FLUSH_BATCH`, `ensureWifi()`, `mqttTryConnect()` không chặn | AC-03.4 | BC §3 | ✅ |
| REQ-07 | Flash đầy không làm treo thiết bị | `spoolCompact()`, `SPOOL_MAX_BYTES` | AC-03.5 | BC §3 | ✅ |
| REQ-08 | Máy chủ công cộng không bị chiếm quyền | `docker-compose.yml` port binding, `mosquitto.conf` | AC-04.1→4.3 | BC §4 | ✅ ở local · ⬜ AC-04.6 trên VPS thật |
| REQ-09 | Lỗi ghi dữ liệu phải phát hiện được ngay | node "Kiểm tra kết quả ghi", `check_influx_response.js` | AC-04.5 | BC §4 | ✅ |
| REQ-10 | Secret không lọt lên GitHub | `.gitignore` | AC-04.4 | BC §5 | ✅ |
| REQ-13 | Lớp 1 phát hiện cell bất thường, chạy on-device | `ai/`, `cell_ai.cpp`, `cell_ae_weights.h` | AC-06 | BC3 | ✅ **chạy trên board thật** |
| REQ-14 | Bản C phải khớp bản Python từng số | `ai/test_c_vs_python.py` | AC-06.3 | BC3 §2 | ✅ lệch <1e-6 |
| REQ-11 | Chuyển sang WISE-IoT thật chỉ tốn cấu hình, không sửa logic | `#define STAGE`, `fetchCredentialFromDccs()` | — | — | ⬜ chờ tài khoản |

**Chú thích:** BC = `docs/BANG_CHUNG_KIEM_THU_2026-09-06.md` · BC2 = `docs/BANG_CHUNG_PHAN_CUNG_2026-09-06.md` · BC3 = `docs/BANG_CHUNG_AI_2026-09-11.md` · AC = `docs/TEST_VA_ACCEPTANCE.md`

---

## Orphan check — chỗ hở đã biết

`skills.txt` §c4 gọi đây là orphan rate. Ghi thẳng ra thay vì để nó ẩn:

| Loại hở | Cụ thể |
|---|---|
| **Requirement chưa có test** | REQ-11 (chuyển WISE-IoT) — không test được cho tới khi có tài khoản. |
| **Test chưa chạy trên phần cứng thật** | Đã gỡ 06/09: firmware đã chạy trên ESP32-S3 thật, store-and-forward đã diễn tập thành công. Còn lại: chưa thử với **mạng 4G/điện thoại** và chưa thử trên **VPS công cộng**. |
| **Code chưa có requirement** | `fetchCredentialFromDccs()`, các topic `cmd`/`ack` mới subscribe chứ chưa xử lý gì. |
| **Requirement chưa có code** | Toàn bộ lớp Edge AI, mạch đo nhiệt thật, cảnh báo tới người dùng. |

## Việc tiếp theo theo thứ tự ưu tiên

1. Đưa stack lên VPS công cộng và chạy lại AC-04 (AC-04.6) — chỗ hở lớn nhất còn lại.
2. Thử với WiFi phát từ điện thoại thay vì router nhà (AC-05.1).
3. Diễn tập trọn kịch bản 5 phút ít nhất 2 lần (AC-05.3).
4. Gắn cảm biến nhiệt thật thay cho 8 giá trị giả lập.
