/* =============================================================================
 *  ble_view — cửa sổ CHỈ ĐỌC nhìn vào pack, qua BLE GATT.
 *
 *  PHỤC VỤ AI (docs/NGUOI_DUNG_VA_KICH_BAN.md §1):
 *  NGƯỜI DÙNG B — thợ kỹ thuật đứng ngay cạnh pack, được giao đi kiểm tra một
 *  pack cụ thể. Họ cần thấy 6 nhiệt độ cell và AI chỉ vào cell nào, NGAY LÚC
 *  NÀY, không cần Wi-Fi, không cần cấu hình, không cần cài app.
 *
 *  KHÔNG phục vụ người lái xe. Tài liệu trên nói thẳng: người lái gần như
 *  không bao giờ mở app, nên giao diện của họ là còi + LED trên thiết bị. Thiết
 *  kế nào bắt họ mở app để an toàn hoạt động là thiết kế sai.
 *
 *  KHÔNG thay Wi-Fi (QĐ-021). BLE chỉ phủ được lúc có người đứng gần, mà
 *  khoảnh khắc nguy hiểm nhất là sạc qua đêm không ai trông. Đây là kênh PHỤ.
 *
 *  ===========================================================================
 *  VÌ SAO KHÔNG CÓ ĐẶC TÍNH NÀO GHI ĐƯỢC — đọc kỹ trước khi thêm
 *  ===========================================================================
 *  Mọi characteristic ở đây đều READ + NOTIFY, KHÔNG CÓ WRITE. Cố ý.
 *
 *  BLE quảng bá công khai và không có xác thực. Thêm một đặc tính ghi được là
 *  cho bất kỳ ai trong bán kính 10 m bật được sưởi hoặc tắt được còi của một
 *  pack pin lithium — không cần mật khẩu, không để lại dấu vết. Đường điều
 *  khiển đã có rồi và nó đi qua MQTT có tài khoản (xem onMqttMessage), cộng
 *  thêm công tắc chết người DH_DEADMAN_MS ở phía thiết bị.
 *
 *  Muốn điều khiển qua BLE thì phải có ghép đôi + mã PIN trước, và đó là một
 *  quyết định phải ghi vào DECISION_LOG, không phải một dòng code thêm vào.
 *
 *  ===========================================================================
 *  DÙNG THẾ NÀO (không cần viết app)
 *  ===========================================================================
 *  Cài "nRF Connect for Mobile" (miễn phí, có trên cả Android và iOS)
 *    → SCAN → thấy thiết bị tên "HuTieu-BMS" → CONNECT
 *    → mở service "Hu Tieu BMS" → bấm mũi tên xuống ở từng đặc tính để đọc,
 *      hoặc bấm biểu tượng nhiều mũi tên để BẬT NOTIFY (tự cập nhật).
 *  Mỗi đặc tính có sẵn nhãn tiếng Việt không dấu (descriptor 0x2901) nên nhìn
 *  là hiểu, không phải tra UUID.
 *
 *  LƯU Ý VỀ MTU: gói notify bị chặn ở MTU-3 byte, mà MTU mặc định chỉ 23 →
 *  20 byte. Chuỗi nhiệt độ 6 cell dài hơn thế. Cho nên notify chỉ nên coi là
 *  "có cái gì đó vừa đổi"; giá trị ĐẦY ĐỦ luôn lấy được bằng lệnh READ (ATT tự
 *  chia nhiều lần, tới 512 byte). Android thường tự nâng MTU lên 247 và khi đó
 *  notify cũng đủ chỗ; iOS thì tuỳ máy. Đừng thiết kế phụ thuộc vào notify đầy.
 * ========================================================================== */
#pragma once

#include <Arduino.h>
#include "pack_config.h"
#include "alarm.h"
#include "cell_ai.h"

/* Bật/tắt cả khối BLE khi biên dịch. Để 0 thì firmware quay về đúng như trước,
   không tốn RAM/flash nào của Bluedroid — hữu ích khi cần chỗ để gỡ lỗi. */
#ifndef USE_BLE
#define USE_BLE 1
#endif

/* Tên thiết bị lúc quảng bá. Giữ NGẮN: gói advertising chỉ có 31 byte, tên dài
   sẽ đẩy UUID dịch vụ ra ngoài và thợ sẽ không lọc được theo dịch vụ. */
#define BLE_DEV_NAME  "HuTieu-BMS"

class BleView {
 public:
  bool begin(const char* device_id);

  /* Gọi theo nhịp AI (1 Hz là đủ). Tự so với lần trước và CHỈ notify khi chuỗi
     thực sự đổi — notify mỗi giây cho 5 đặc tính là tốn pin điện thoại và làm
     nhật ký nRF Connect trôi mất thứ đáng nhìn.

     temps     : mảng AI_N_CELLS nhiệt độ cell, NAN nếu chưa đọc được
     ai        : kết quả Lớp 1 của bước hiện tại (ai.valid=false lúc khởi động)
     lvl       : mức báo động đang có
     pack_v/a  : điện áp/dòng pack, meter_ok=false thì bỏ qua hai số này
     n_healthy : số cảm biến còn sống
     cloud_ok  : MQTT đang nối được hay không */
  void update(const float* temps, const CellAIResult& ai, AlarmLevel lvl,
              bool muted, bool quiet,
              bool meter_ok, float pack_v, float pack_a, float soc_pct,
              int n_healthy, bool wifi_ok, bool cloud_ok);

  bool ready()     const { return ready_; }
  bool connected() const;
  int  clientCount() const;

 private:
#if USE_BLE
  void put(int idx, const char* s);   // ghi + notify nếu đổi
#endif
  bool ready_ = false;
};
