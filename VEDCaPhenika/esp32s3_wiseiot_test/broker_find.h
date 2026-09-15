/* =============================================================================
 *  Tìm broker MQTT mà không cần ghim IP.
 *
 *  VẤN ĐỀ THẬT: máy chủ chạy lúc ở trường, lúc ở nhà, lúc qua hotspot điện
 *  thoại. IP đổi liên tục. Ghim IP vào firmware nghĩa là mỗi lần đổi mạng phải
 *  nạp lại firmware — và hôm 15/09 đã dính đúng chuyện đó: IP nhảy từ
 *  172.172.3.174 sang 172.172.5.2, ESP32 báo rc=-2 và không ai biết tại sao
 *  cho tới khi đọc log.
 *
 *  BA ĐƯỜNG, THỬ THEO THỨ TỰ — vì không đường nào tin được một mình:
 *
 *  1. mDNS: hỏi dịch vụ `_mqtt._tcp` trên mạng. Máy chủ quảng bá qua avahi
 *     (xem planb_cloud/avahi/hutieu-mqtt.service), và avahi TỰ cập nhật địa
 *     chỉ khi IP đổi. Đây là đường đúng nhất.
 *     Nhược điểm: **rất nhiều WiFi công cộng chặn multicast**, và hội trường
 *     thi rất có thể là một trong số đó. Không được tin một mình.
 *
 *  2. IP LẦN TRƯỚC DÙNG ĐƯỢC, lưu trong LittleFS. Đổi mạng thì IP thường vẫn
 *     nằm trong cùng dải nếu quay lại mạng cũ — và quan trọng hơn, nó sống sót
 *     qua lần khởi động lại khi mDNS bị chặn.
 *
 *  3. IP biên dịch sẵn (TEST_HOST). Đường lùi cuối cùng, và là đường duy nhất
 *     chắc chắn có khi mang máy tới một mạng hoàn toàn mới lần đầu tiên.
 *
 *  Nối được bằng đường nào thì GHI LẠI đường đó, để lần sau đi thẳng.
 *
 *  ⚠️ KHÔNG dùng module này để đổi format payload hay topic. Nó chỉ trả lời
 *  câu "broker ở đâu", không đụng gì tới data contract.
 * ========================================================================== */
#pragma once
#include <Arduino.h>

struct BrokerAddr {
  char     host[40];
  uint16_t port;
  const char* how;      // "mDNS" / "da luu" / "bien dich san" — để in ra log
};

/* Tìm broker. `fallback_host`/`fallback_port` là giá trị biên dịch sẵn.
   Luôn trả về một địa chỉ nào đó (tệ nhất là fallback), không bao giờ thất
   bại — vì thất bại ở đây thì bên gọi cũng chẳng làm gì khác được. */
BrokerAddr brokerFind(const char* fallback_host, uint16_t fallback_port);

/* Gọi khi đã nối MQTT thành công, để lần khởi động sau đi thẳng. Chỉ ghi khi
   địa chỉ KHÁC cái đang lưu — LittleFS có số lần ghi hữu hạn, và ghi lại cùng
   một giá trị mỗi lần nối là bào mòn flash không vì gì cả. */
void brokerRemember(const char* host, uint16_t port);
