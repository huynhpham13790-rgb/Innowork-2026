# Data Contract — ranh giới không được tự ý phá vỡ

Đây là **hợp đồng dữ liệu** giữa ESP32 và tầng cloud. Ai sửa một trong các mục dưới đây thì phải sửa đồng thời cả hai đầu và chạy lại toàn bộ `docs/TEST_VA_ACCEPTANCE.md`.

Nguồn sự thật của format này là SDK EdgeAgent của Advantech. Firmware đã đối chiếu byte-for-byte. **Đây là lý do đội giữ được Plan B mà không phải viết lại firmware** — xem `docs/DECISION_LOG.md` QĐ-002.

---

## 1. Topic

| Topic | Chiều | Dùng làm gì |
|---|---|---|
| `/wisepaas/scada/{nodeId}/data` | ESP → cloud | Dữ liệu đo. Đây là topic duy nhất mà Node-RED ghi vào DB. |
| `/wisepaas/scada/{nodeId}/cfg` | ESP → cloud | Khai báo Node/Device/Tag. Gửi 1 lần sau khi nối. Thiếu là dashboard trống dù data vẫn về. |
| `/wisepaas/scada/{nodeId}/conn` | ESP → cloud | Trạng thái sống/chết + heartbeat. Có retained + Last Will. |
| `/wisepaas/scada/{nodeId}/cmd` | cloud → ESP | Lệnh xuống. Hiện chỉ subscribe và in ra, chưa xử lý. |
| `/wisepaas/scada/{nodeId}/ack` | cloud → ESP | Phản hồi. Hiện chỉ subscribe. |

`{nodeId}` là UUID lấy ở portal WISE-IoT (EdgeHub → Node). Với Plan B nó chỉ là một đoạn chuỗi trong topic — Node-RED subscribe bằng wildcard `+` nên giá trị nào cũng chạy.

## 2. Payload `data` — quan trọng nhất

```json
{
  "d": { "BatteryPack01": { "Cell01_Temp": 30.42, "Cell02_Temp": 29.88 } },
  "ts": "2026-09-06T09:15:22.410000Z"
}
```

**Bất biến phải giữ:**

| Quy tắc | Vì sao |
|---|---|
| `d` là object, khoá ngoài là `deviceId` | EdgeAgent gom tag theo device. Node-RED lấy khoá này làm tag `device=`. |
| Khoá trong là tên tag, giá trị là **số** | Giá trị không phải số bị `convert.js` bỏ qua (không phải lỗi — gói `conn` dùng cấu trúc khác). |
| `ts` là ISO-8601 UTC, **đúng 6 chữ số phần lẻ giây**, kết thúc bằng `Z` | Đây là format EdgeHub yêu cầu: `%Y-%m-%dT%H:%M:%S.%fZ`. |
| `ts` do **thiết bị** sinh, không phải server | Store-and-forward phụ thuộc hoàn toàn vào điều này. Xem QĐ-003. |
| Tên tag: `Cell{NN}_Temp`, NN 2 chữ số | Phải khớp giữa `publishConfig()` và `publishData()`, nếu lệch là dashboard trống. |

**Giới hạn đã biết:**
- PubSubClient chỉ publish được **QoS 0**. Không có xác nhận từ broker → một gói mất là mất im lặng. Store-and-forward chỉ cứu được trường hợp *mất kết nối*, không cứu được *broker nuốt gói*. Rủi ro đã chấp nhận, xem `docs/DECISION_LOG.md` QĐ-005.
- `mqtt.setBufferSize(4096)` là **bắt buộc**. Mặc định 256 byte sẽ cắt cụt payload `cfg`.
- `ts` bị làm tròn xuống mili-giây khi qua Node-RED (`Date.parse` của JS chỉ tới ms). Micro-giây trong payload bị mất. Không ảnh hưởng bài toán nhiệt độ ở nhịp 2s.

## 3. Payload `cfg` và `conn`

```json
// cfg — khai báo, Action 1 = Create
{"d":{"Action":1,"Scada":{"<nodeId>":{"Type":0,"Hbt":60,"Device":{"BatteryPack01":{...}}}}},"ts":"..."}

// conn — sống / chết / heartbeat / last will
{"d":{"Con":1},"ts":"..."}    {"d":{"DsC":1},"ts":"..."}
{"d":{"Hbt":1},"ts":"..."}    {"d":{"UeD":1},"ts":"..."}   // UeD = Last Will
```

Các gói này **cố tình bị `convert.js` bỏ qua** vì `d` không chứa object tag. Đó là hành vi đúng, đã có test.

## 4. Hợp đồng phía trong Plan B: Node-RED → InfluxDB

Line protocol sinh ra từ mỗi tag:

```
cell,device=BatteryPack01,tag=Cell01_Temp value=30.42 1757150122410000000
```

| Thành phần | Giá trị | Ràng buộc |
|---|---|---|
| measurement | `cell` | Cố định. Query Grafana lọc `_measurement == "cell"`. |
| tag `device` | từ khoá ngoài của `d` | |
| tag `tag` | tên tag | |
| field | luôn là `value` | Query lọc `_field == "value"`. Thêm field kiểu chuỗi thì `mean()` sẽ lỗi. |
| timestamp | ns, từ `ts` của ESP | `precision=ns` trong URL ghi. **Không được bỏ.** |
| bucket / org | `battery` / `hutieu` | Khớp với `docker-compose.yml` và data source Grafana. |

**Chặn đầu vào:** gói có `ts` trước 2023-11 (mốc `1700000000000`) bị `convert.js` loại kèm cảnh báo. Đây là bẫy NTP-fail: ts ra năm 1970 → InfluxDB trả HTTP 422 và vứt dữ liệu **hoàn toàn im lặng**. Xem bằng chứng trong `docs/BANG_CHUNG_KIEM_THU_2026-09-06.md`.

## 5. Chuyển từ Plan B sang WISE-IoT thật

Vì hợp đồng này không đổi, việc chuyển chỉ là:
1. `#define STAGE 1` → `2` trong `.ino`
2. Điền `DCCS_API_URL`, `DCCS_CRED_KEY`, `NODE_ID`
3. Hết. Không sửa một dòng logic nào.
