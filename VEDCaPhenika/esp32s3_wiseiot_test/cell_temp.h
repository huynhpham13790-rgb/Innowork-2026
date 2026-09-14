/* =============================================================================
 *  Đọc 8 cảm biến nhiệt độ DS18B20 — bản dùng được trên xe thật.
 *
 *  Bốn thứ module này làm mà `sensors.getTempCByIndex(i)` không làm:
 *
 *  1. ĐỌC THEO ĐỊA CHỈ ROM, không theo index.
 *     Index là thứ tự dò bus, do giá trị ROM quyết định. Thay một con cảm biến
 *     là toàn bộ ánh xạ "kênh i ↔ cell nào" đổi hết mà KHÔNG có lỗi nào báo —
 *     Lớp 1 vẫn chạy, vẫn ra số, chỉ là sai cell. Ở đây thứ tự do bảng DS_ROM
 *     trong ds18b20_offsets.h quy định, cố định vĩnh viễn.
 *
 *  2. KHÔNG CHẶN. Chuyển đổi 12 bit tốn ~750 ms. Firmware phải gọi mqtt.loop()
 *     đều đặn; chặn 750 ms mỗi giây là rớt keep-alive. update() trả về ngay,
 *     lấy kết quả ở lần gọi sau.
 *
 *  3. TRỪ SAI SỐ CHẾ TẠO (QĐ-022, QĐ-025). Đo được 0,3665 °C giữa con nóng
 *     nhất và lạnh nhất. Lớp 1 nhìn chênh lệch tương đối giữa các cell nên
 *     lệch cố định cỡ này trông y hệt một cell luôn nóng hơn.
 *
 *  4. ĐẾM VÀ BÁO LỖI (QĐ-024). Đã đo được bus lỗi tới 35 % mà không có dấu
 *     hiệu gì, và đã gặp cảnh tuột một dây tín hiệu mà chương trình vẫn chạy
 *     bình thường với 6 cảm biến. Với hệ giám sát an toàn thì đọc sai mà không
 *     báo là chế độ hỏng tệ nhất — tệ hơn chết hẳn, vì chết hẳn thì còn biết
 *     mà sửa. Kênh nào hỏng thì bị LOẠI khỏi đầu vào Lớp 1, không đưa số rác
 *     cho AI.
 * ========================================================================== */
#pragma once
#include <stdint.h>
#include <math.h>
#include "ds18b20_offsets.h"

#define CT_N              DS_N_PROBES
#define CT_CONV_MS        760    // 12 bit cần ≤750 ms (datasheet); chừa biên 10 ms
#define CT_MAX_CONSEC     3      // hỏng liên tiếp bấy nhiêu lần thì coi kênh là chết
// Phải đọc tốt liên tiếp bấy nhiêu lần mới cho kênh sống lại. Có ngưỡng kép
// (trễ) vì nếu chỉ cần MỘT lần đọc tốt là phục hồi thì một kênh chập chờn sẽ
// nhấp nháy sống-chết liên tục: log spam, mà dashboard thì lúc báo 8/8 lúc báo
// 7/8 nên người trực không tin nữa. Đã thấy đúng hiện tượng này với kênh 5.
#define CT_MIN_CONSEC_OK  10
#define CT_T_MIN         (-40.0f)
#define CT_T_MAX          125.0f

struct CellTempStatus {
  bool     ready;                // đã có ít nhất một lượt đọc hợp lệ
  uint8_t  n_found;              // số cảm biến thấy lúc khởi động (phải bằng 8)
  uint8_t  n_healthy;            // số kênh đang khoẻ
  bool     healthy[CT_N];
  uint32_t n_ok[CT_N];
  uint32_t n_err[CT_N];
  uint32_t consec_err[CT_N];
  uint32_t consec_ok[CT_N];
  uint32_t n_flap;               // số lần một kênh chuyển khoẻ <-> hỏng
};

class CellTemp {
 public:
  /* Trả về false nếu không đủ 8 cảm biến, hoặc có ROM trong bảng không tìm
     thấy trên bus. Gọi trong setup(); KHÔNG được bỏ qua giá trị trả về. */
  bool begin(uint8_t pin);

  /* Gọi thoải mái trong loop(), không chặn. Trả về true đúng một lần mỗi khi
     có bộ 8 giá trị mới. */
  bool update();

  /* Nhiệt độ đã trừ offset. Kênh chết được thay bằng trung bình các kênh khoẻ
     — để đặc trưng tương đối của nó thành ~0 (trông "bình thường") thay vì
     thành số rác kéo lệch cả pack. Dùng healthy[] để biết cái nào là thật. */
  const float* temps() const { return t_; }

  const CellTempStatus& status() const { return st_; }

  /* Nhiệt độ môi trường từ cảm biến thứ 9. Trả về NAN nếu con đó không có
     trên bus — lúc đó bên gọi phải tự quyết dùng giá trị dự phòng, chứ module
     này KHÔNG bịa ra một con số trông hợp lý. */
  float ambient() const { return amb_; }
  bool  ambientOk() const { return !isnan(amb_); }

  /* Tỉ lệ lỗi của toàn bus, để đẩy lên dashboard. */
  float errorRate() const;

  /* In một dòng tóm tắt ra Serial — gọi định kỳ cho người trực nhìn. */
  void printStatus() const;

 private:
  float  t_[CT_N];
  CellTempStatus st_ = {};
  float    amb_ = NAN;
  bool     amb_present_ = false;
  bool     converting_ = false;
  uint32_t convStart_ = 0;
  uint8_t  pin_ = 0;
};
