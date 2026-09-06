# RTM — Requirement → Code → Test → Bằng chứng

Bảng này để trả lời hai câu hỏi nhanh: *"sửa chỗ này thì ảnh hưởng tới đâu?"* và *"cái này đã có ai kiểm chưa?"*

`skills.txt` gọi đây là traceability coverage — chỉ số quan trọng hơn số dòng tài liệu.

---

## Phạm vi hiện tại

RTM này mới phủ **chặng ESP32 → cloud**. Các phần Edge AI (autoencoder on-device), phần cứng đo nhiệt, và ứng dụng người dùng chưa có code nên chưa vào bảng — xem `VEDCaPhenika/KHUNG_NGHIEN_CUU_v2_HuTieu.md`.

| REQ | Yêu cầu | Code | Test | Bằng chứng | Trạng thái |
|---|---|---|---|---|---|
| REQ-01 | ESP32 gửi dữ liệu đúng giao thức WISE-PaaS, không phải sửa khi đổi cloud | `.ino` → `buildTopics()`, `publishData()`, `publishConfig()` | AC-01.3, AC-01.5 | BC §1, §3 | ✅ |
| REQ-02 | Cloud nhận, lưu và vẽ được dữ liệu | `planb_cloud/` toàn bộ | AC-01.1→1.4 | BC §1 | ✅ trừ AC-01.4 (Grafana panel) |
| REQ-03 | Timestamp do thiết bị quyết định, cloud tôn trọng | `isoTimestampUtc()`, `convert.js`, `precision=ns` | AC-02.1 | BC §2 | ✅ |
| REQ-04 | NTP hỏng không được làm hỏng dữ liệu một cách im lặng | `timeIsValid()`, chặn mốc `1700000000000` trong `convert.js` | AC-02.2, AC-02.3 | BC §2, §4 | ✅ phía cloud · 🔶 phía board |
| REQ-05 | Mất mạng không mất dữ liệu; nối lại đẩy bù đúng thứ tự, đúng thời điểm gốc | `spoolAppend/Flush/Compact()`, `loop()` không chặn | AC-03.1→3.5 (`test/run_test.sh`) | BC §3 | ✅ ở mức logic · ⬜ AC-03.6 cần board |
| REQ-06 | Đệm không được làm nghẽn vòng lấy mẫu | `FLUSH_BATCH`, `ensureWifi()`, `mqttTryConnect()` không chặn | AC-03.4 | BC §3 | ✅ |
| REQ-07 | Flash đầy không làm treo thiết bị | `spoolCompact()`, `SPOOL_MAX_BYTES` | AC-03.5 | BC §3 | ✅ |
| REQ-08 | Máy chủ công cộng không bị chiếm quyền | `docker-compose.yml` port binding, `mosquitto.conf` | AC-04.1→4.3 | BC §4 | ✅ ở local · ⬜ AC-04.6 trên VPS thật |
| REQ-09 | Lỗi ghi dữ liệu phải phát hiện được ngay | node "Kiểm tra kết quả ghi", `check_influx_response.js` | AC-04.5 | BC §4 | ✅ |
| REQ-10 | Secret không lọt lên GitHub | `.gitignore` | AC-04.4 | BC §5 | ✅ |
| REQ-11 | Chuyển sang WISE-IoT thật chỉ tốn cấu hình, không sửa logic | `#define STAGE`, `fetchCredentialFromDccs()` | — | — | ⬜ chờ tài khoản |

**Chú thích:** BC = `docs/BANG_CHUNG_KIEM_THU_2026-09-06.md` · AC = `docs/TEST_VA_ACCEPTANCE.md`

---

## Orphan check — chỗ hở đã biết

`skills.txt` §c4 gọi đây là orphan rate. Ghi thẳng ra thay vì để nó ẩn:

| Loại hở | Cụ thể |
|---|---|
| **Requirement chưa có test** | REQ-11 (chuyển WISE-IoT) — không test được cho tới khi có tài khoản. |
| **Test chưa chạy trên phần cứng thật** | REQ-04 phía board, REQ-05 AC-03.6. Toàn bộ RTM hiện dừng ở mức "logic đúng + cloud đúng", **chưa có một lần nào chạy trên ESP32 thật**. Đây là chỗ hở lớn nhất. |
| **Code chưa có requirement** | `fetchCredentialFromDccs()`, các topic `cmd`/`ack` mới subscribe chứ chưa xử lý gì. |
| **Requirement chưa có code** | Toàn bộ lớp Edge AI, mạch đo nhiệt thật, cảnh báo tới người dùng. |

## Việc tiếp theo theo thứ tự ưu tiên

1. **Nạp firmware lên board thật** và chạy AC-01.6 + AC-03.6. Gỡ được chỗ hở lớn nhất ở trên.
2. Dựng dashboard Grafana (AC-01.4) — cần cho mọi lần demo về sau.
3. Đưa stack lên VPS công cộng và chạy lại AC-04 (AC-04.6).
4. Diễn tập kịch bản 5 phút (AC-05.3).
