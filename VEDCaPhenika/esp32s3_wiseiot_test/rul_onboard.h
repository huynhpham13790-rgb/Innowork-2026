/* =============================================================================
 *  Lớp 2 chạy TRÊN THIẾT BỊ — RUL/SOH không cần mạng (QĐ-043)
 *
 *  VÌ SAO THÊM, KHI CLOUD ĐÃ TÍNH RỒI
 *  Cloud vẫn là nơi giữ mô hình chuẩn, và lý do cũ vẫn đúng nguyên: đổi hệ số
 *  không phải nạp lại firmware, quan trọng vì hệ số còn phải hiệu chỉnh cho
 *  pack thật. Bản trên chip KHÔNG thay cloud — nó là bản dự phòng để thiết bị
 *  vẫn trả lời được khi mất mạng, và để app BLE có số mà hiện.
 *
 *  Giá phải trả rất nhỏ: 10 phép nhân cộng, chạy MỘT LẦN mỗi chu kỳ sạc (vài
 *  tiếng). So với Lớp 1 — autoencoder 271 tham số chạy 1 Hz — đây là hạt cát.
 *  Đừng lấy nó làm ví dụ cho bài toán ngân sách điện; thứ tốn điện là Wi-Fi.
 *
 *  MÔ HÌNH NÀY LÀ HỌC MÁY, KHÔNG PHẢI CÔNG THỨC NGƯỜI VIẾT
 *  Không ai chọn con số -26,96. Nó được học từ 364 chu kỳ sạc của 4 pin NASA
 *  bằng LinearRegression().fit() trong ai/train_rul.py. Đội đã train 5 họ mô
 *  hình và so bằng leave-one-battery-out; tuyến tính 2 tham số THẮNG cả LSTM
 *  3.500 tham số (MAE 12,2 so với 20,3 chu kỳ). Xem QĐ-017 — đó là câu trả lời
 *  cho "sao không dùng deep learning".
 *
 *  ===========================================================================
 *  HAI THỨ BẮT BUỘC PHẢI ĐI KÈM CON SỐ
 *  ===========================================================================
 *  1. CỜ NGOẠI SUY. Mô hình tuyến tính KHÔNG BAO GIỜ từ chối trả lời — đầu vào
 *     nằm ngoài dải đã học thì nó vẫn ra một con số trông rất hợp lý. Không có
 *     cờ này thì lỗi hiệu chỉnh chỉ hiện ra dưới dạng "RUL = 0", rất dễ tưởng
 *     là pin hỏng thật. Đây là QĐ-019, và nó quan trọng hơn chính con số RUL.
 *
 *  2. GIÁ TRỊ THÔ. `rul_raw` giữ nguyên số trước khi chặn về 0. Nhìn dashboard
 *     thấy một đường 0 phẳng lì thì không biết là pin hết đời hay mô hình đang
 *     đoán âm; nhìn rul_raw thì biết ngay.
 *
 *  PHẢI KHỚP VỚI CLOUD: thuật toán ở đây là bản dịch 1-1 của
 *  planb_cloud/nodered/rul_predict.js. Sửa một bên mà quên bên kia thì hai nơi
 *  ra hai số khác nhau cho cùng một chu kỳ sạc — và cả hai đều trông hợp lý.
 *  Hệ số thì đã an toàn (cùng sinh từ rul_model.json), nhưng LOGIC thì chưa.
 * ========================================================================== */
#pragma once

#include <stdint.h>
#include "charge_cycle.h"

struct RulResult {
  bool  valid;          // false khi chu kỳ sạc chưa hợp lệ
  float rul_cycles;     // số chu kỳ còn lại, đã chặn >= 0
  float rul_raw;        // trước khi chặn — xem chú thích (2) ở trên
  float soh;            // 0..1,2 (đã chặn). Nhân 100 ra phần trăm
  bool  extrapolating;  // có đặc trưng nào vượt ±3 độ lệch chuẩn không
  uint8_t n_outliers;   // bao nhiêu cái vượt, để biết mức độ
};

/* Tính RUL/SOH từ một gói tóm tắt chu kỳ sạc. Thuần hàm, không đụng phần cứng,
   nên test được trên PC — xem test/test_rul_onboard.cpp. */
RulResult rulPredict(const ChargeSummary& s);

/* ---------------------------------------------------------------------------
 *  Lịch sử chu kỳ sạc trên flash
 *
 *  RUL KHÔNG CẦN LỊCH SỬ để tính: mô hình chỉ đọc đặc trưng của CHU KỲ HIỆN
 *  TẠI. Lịch sử chỉ để vẽ XU HƯỚNG — và xu hướng mới là thứ phân biệt TH-6
 *  ("SOH tụt dần đều, lão hoá tự nhiên") với TH-7 ("SOH tụt đột ngột, có cell
 *  hỏng"). Hai trường hợp đó dẫn tới hai hành động khác hẳn nhau.
 *
 *  Chi phí: một bản ghi ~32 byte. Cả đời pin (~1000 chu kỳ) hết ~32 KB, trên
 *  phân vùng 1,5 MB. Tức chip chứa được TOÀN BỘ lịch sử cả đời quả pin mà
 *  không phải đánh đổi gì.
 * ------------------------------------------------------------------------ */
#define RUL_HIST_MAX   1000          // đủ cho cả đời pin; vượt thì bỏ bản cũ nhất
#define RUL_HIST_PATH  "/rulhist.bin"

struct RulHistEntry {
  uint32_t ts;          // epoch giây, 0 nếu chưa có NTP lúc ghi
  float    rul;
  float    soh;
  float    t_cv;        // giữ đặc trưng chính để còn dựng lại được nếu đổi mô hình
  uint32_t flags;       // bit0 = đang ngoại suy
};

bool  rulHistBegin();
bool  rulHistAppend(const RulResult& r, const ChargeSummary& s, uint32_t ts);
int   rulHistCount();
/* Đọc `n` bản ghi GẦN NHẤT vào out[]. Trả về số bản thực đọc được. */
int   rulHistRecent(RulHistEntry* out, int n);
/* Xu hướng SOH: điểm phần trăm thay đổi mỗi chu kỳ, tính bằng hồi quy tuyến
   tính trên `n` bản ghi gần nhất. NAN nếu chưa đủ dữ liệu. */
float rulHistSohSlope(int n);
