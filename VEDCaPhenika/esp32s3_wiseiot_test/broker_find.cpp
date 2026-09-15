#include <ESPmDNS.h>
#include <LittleFS.h>
#include "broker_find.h"

static const char* SAVE_PATH = "/broker.txt";

BrokerAddr brokerFind(const char* fallback_host, uint16_t fallback_port) {
  BrokerAddr b = {};
  b.port = fallback_port;

  // --- Đường 1: mDNS -------------------------------------------------------
  // Tên máy đăng ký ở đây không quan trọng (không ai gọi tới ESP32), nhưng
  // begin() bắt buộc phải chạy trước khi queryService() dùng được.
  if (MDNS.begin("hutieu-node")) {
    const int n = MDNS.queryService("mqtt", "tcp");
    if (n > 0) {
      // Lấy dịch vụ đầu tiên. Mạng nhà thường chỉ có một broker; nếu có nhiều
      // thì đây là chỗ cần lọc theo txt-record, chưa cần tới.
      strncpy(b.host, MDNS.address(0).toString().c_str(), sizeof(b.host) - 1);
      b.port = MDNS.port(0);
      b.how = "mDNS";
      Serial.printf("[BRK ] mDNS tim thay %s:%u\n", b.host, b.port);
      return b;
    }
    Serial.println("[BRK ] mDNS khong thay dich vu _mqtt._tcp "
                   "(mang chan multicast, hoac may chu chua cai avahi service)");
  }

  // --- Đường 2: địa chỉ lần trước dùng được --------------------------------
  File f = LittleFS.open(SAVE_PATH, "r");
  if (f) {
    String line = f.readStringUntil('\n');
    f.close();
    const int sep = line.indexOf(':');
    if (sep > 0 && sep < (int)sizeof(b.host)) {
      line.substring(0, sep).toCharArray(b.host, sizeof(b.host));
      b.port = (uint16_t)line.substring(sep + 1).toInt();
      if (b.host[0] && b.port) {
        b.how = "da luu";
        Serial.printf("[BRK ] dung dia chi da luu %s:%u\n", b.host, b.port);
        return b;
      }
    }
  }

  // --- Đường 3: biên dịch sẵn ----------------------------------------------
  strncpy(b.host, fallback_host, sizeof(b.host) - 1);
  b.port = fallback_port;
  b.how = "bien dich san";
  Serial.printf("[BRK ] dung dia chi bien dich san %s:%u\n", b.host, b.port);
  return b;
}

void brokerRemember(const char* host, uint16_t port) {
  char want[48];
  snprintf(want, sizeof(want), "%s:%u", host, port);

  // Chỉ ghi khi KHÁC cái đang lưu. Flash có số lần ghi hữu hạn; ghi lại cùng
  // một giá trị mỗi lần nối lại MQTT là bào mòn vô ích — mà nối lại thì xảy ra
  // hàng chục lần mỗi giờ khi sóng yếu.
  File r = LittleFS.open(SAVE_PATH, "r");
  if (r) {
    String cur = r.readStringUntil('\n');
    r.close();
    cur.trim();
    if (cur == want) return;
  }

  File w = LittleFS.open(SAVE_PATH, "w");
  if (!w) return;
  w.println(want);
  w.close();
  Serial.printf("[BRK ] da nho %s cho lan sau\n", want);
}
