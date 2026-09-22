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
 *  GHI ĐƯỢC ĐÚNG MỘT THỨ: TẮT TIẾNG CÒI — và phải trả giá để có nó (QĐ-042)
 *  ===========================================================================
 *  Bản đầu (QĐ-041) KHÔNG có đặc tính ghi nào. Lý do vẫn đúng nguyên: BLE
 *  quảng bá công khai, nên một đặc tính ghi tự do là cho bất kỳ ai trong bán
 *  kính 10 m tắt còi của một pack pin lithium — không mật khẩu, không dấu vết.
 *
 *  Nhưng người dùng cần tắt được còi từ điện thoại khi đã biết lỗi, và bắt họ
 *  chạy lại chỗ pack để bấm nút BOOT là thiết kế tồi. Nên mở, kèm BA lớp chặn:
 *
 *   1. GHÉP ĐÔI + MÃ PIN. Đặc tính lệnh đòi liên kết đã mã hoá và xác thực
 *      (bonding + MITM). Người lạ đi ngang KHÔNG ghi được vì chưa ghép đôi, mà
 *      ghép đôi thì phải nhập đúng 6 số BLE_PASSKEY.
 *   2. CHỈ TẮT TIẾNG, KHÔNG BẬT SƯỞI. Danh sách lệnh qua BLE chỉ có mute và
 *      quiet. Sưởi vẫn chỉ đi qua MQTT có tài khoản + công tắc chết người.
 *      Kẻ xấu ghép đôi được cũng không làm nóng được pack.
 *   3. TỰ HẾT HẠN — CHỈ Ở MỨC NGUY KỊCH (QĐ-047). Trước đây tắt tiếng từ BLE
 *      tự hết sau 5 phút ở MỌI mức; bỏ đi vì tắt tiếng một bất thường Lớp 1
 *      rồi 5 phút sau còi lại kêu là đúng cái làm người dùng tháo còi ra cho
 *      xong. Hạn giờ giờ nằm ở alarm.cpp nên áp cho cả nút bấm. Kể cả bị lạm
 *      dụng thì im lặng cũng không vĩnh viễn — và người dùng thật cũng khỏi
 *      quên bật lại, đúng cái bẫy đã gặp ngày 21/09.
 *
 *  Ngoài ra Alarm không cho tắt tiếng ở mức NGUY KỊCH (ngưỡng cứng 60 °C) —
 *  lớp cuối cùng thì không ai được bịt miệng, kể cả chủ máy.
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
#include "rul_onboard.h"

/* Bật/tắt cả khối BLE khi biên dịch. Để 0 thì firmware quay về đúng như trước,
   không tốn RAM/flash nào của Bluedroid — hữu ích khi cần chỗ để gỡ lỗi. */
#ifndef USE_BLE
#define USE_BLE 1
#endif

/* Tên thiết bị lúc quảng bá. Giữ NGẮN: gói advertising chỉ có 31 byte, tên dài
   sẽ đẩy UUID dịch vụ ra ngoài và thợ sẽ không lọc được theo dịch vụ. */
#define BLE_DEV_NAME  "HuTieu-BMS"

/* Mã ghép đôi 6 số. ĐỔI TRƯỚC KHI GIAO MÁY THẬT — số này in trên nhãn dán ở
   thân thiết bị, không phải bí mật lớn, nhưng nó chặn được người đi ngang.
   Không đưa vào arduino_secrets.h vì nó phải khớp với cái in trên nhãn, tức
   thuộc về cấu hình sản phẩm chứ không phải bí mật của kho mã. */
#define BLE_PASSKEY       123456

/* Tắt tiếng từ BLE tự hết sau 5 phút. Xem lớp chặn thứ 3 ở trên. */
#define BLE_MUTE_TTL_MS   300000UL

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
              int n_healthy, bool wifi_ok, bool cloud_ok,
              uint32_t crit_mute_left_s = 0);

  /* Lớp 2 tính trên chip. Tách khỏi update() vì nó chỉ đổi MỘT LẦN mỗi chu kỳ
     sạc — gọi kèm nhịp 1 Hz là ghi lại cùng một chuỗi vài nghìn lần vô ích. */
  void setLayer2(const RulResult& r, int cycles_logged, float soh_slope);

  /* Gắn hai hàm mà lệnh BLE được phép gọi. KHÔNG có đường nào khác từ BLE vào
     thiết bị — muốn thêm lệnh phải thêm ở đây, nên không thể vô tình mở một
     đường điều khiển chỉ vì đổi payload ở điện thoại. */
  void onCommand(void (*mute_fn)(bool), void (*quiet_fn)(bool));

  /* Gọi mỗi vòng loop(): lo việc cho tắt tiếng từ BLE tự hết hạn. */
  void tick();

  bool ready()     const { return ready_; }
  bool connected() const;
  int  clientCount() const;

 private:
#if USE_BLE
  void put(int idx, const char* s);   // ghi + notify nếu đổi
#endif
  bool ready_ = false;
};
