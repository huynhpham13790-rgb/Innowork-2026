/* =============================================================================
 *  Lớp 1 — Phát hiện cell bất thường, chạy thẳng trên ESP32-S3.
 *
 *  Bản C của ai/features.py + autoencoder 16->8->4->8->16.
 *  Hai bên PHẢI cho ra cùng một con số: ai/test_c_vs_python.sh kiểm tra điều đó
 *  bằng cách chạy cả hai trên cùng dữ liệu rồi so từng phần tử. Lệch là mô hình
 *  đang nhận đầu vào khác với lúc train, và mọi ngưỡng đều vô nghĩa.
 *
 *  Cách dùng trong sketch:
 *      CellAI ai;
 *      ai.begin();
 *      // mỗi giây, sau khi đọc xong 8 con DS18B20:
 *      CellAIResult r = ai.update(temps, ambient, current_A, soc_pct);
 *      if (r.alarm) { ... cell r.worst_cell đang bất thường ... }
 *
 *  Chi phí: ~3 KB RAM cho cửa sổ trượt, ~1,4 KB flash cho trọng số,
 *  356 phép nhân-cộng × 8 cell mỗi giây — không đáng kể ở 240 MHz.
 * ========================================================================== */
#pragma once
#include <stdint.h>

#define AI_N_CELLS   8
// 11 đặc trưng THUẦN TƯƠNG ĐỐI (QĐ-033). Trước đây là 16; đã bỏ 5 đặc trưng
// mang giá trị tuyệt đối (T-amb, packT-amb, |I|/I_SCALE, soc, (T-25)/25).
// Lý do: chúng giống hệt nhau ở mọi cell nên không mang thông tin phân biệt
// cell nào, mà lại gắn chặt với một pack cụ thể — đo được là chúng lệch tới
// 8,6 sd khi sang pack khác, trong khi đặc trưng tương đối chỉ lệch ≤1,6 sd.
// Bỏ đi thì nút thắt 4 chiều được dành trọn cho cấu trúc tương đối, và kết
// quả TỐT HƠN trên mọi phép đo: giữ 30s thay vì 60s, trễ 104 phút thay vì
// 138 phút, 0 báo động giả trên 6,1 triệu mẫu thay vì 1 lần/1,3 ngày.
#define AI_N_FEAT    11
#define AI_DT_WIN    10     // cửa sổ tính tốc độ đổi nhiệt (giây)
#define AI_STD_WIN   60     // cửa sổ tính độ dao động (giây)
#define AI_EMA_TAU   300    // hằng số thời gian EMA chậm (giây)
#define AI_I_SCALE   20.0f  // dòng chuẩn hoá (A)

struct CellAIResult {
  bool  valid;                  // false khi chưa đủ dữ liệu khởi động
  bool  alarm;                  // đã vượt ngưỡng liên tục đủ lâu
  int   worst_cell;             // cell có sai số tái tạo lớn nhất (0-based)
  float worst_score;            // sai số tái tạo của cell đó
  float score[AI_N_CELLS];      // sai số tái tạo từng cell
  uint16_t run_s[AI_N_CELLS];   // số giây liên tục đang vượt ngưỡng
};

class CellAI {
 public:
  void begin();

  /* temps: 8 nhiệt độ cell (°C) · ambient (°C) · current (A) · soc (0..100)
     Gọi đúng 1 Hz. Gọi sai nhịp thì các đặc trưng theo thời gian sẽ sai. */
  CellAIResult update(const float* temps, float ambient,
                      float current, float soc);

  /* Lộ ra để test so với Python. Điền 16 đặc trưng của cell `cell`. */
  void features_of(int cell, float* out16) const;

  uint32_t samples() const { return n_; }

  /* Ngưỡng và quy tắc giữ liên tục, lấy từ cell_ae_weights.h. Lộ qua đây để
     sketch khỏi phải include file trọng số — nó chỉ cần biết hai con số này. */
  static float    threshold();
  static uint16_t persist_s();

 private:
  uint32_t n_ = 0;                              // số mẫu đã nhận

  float hist_t_[AI_N_CELLS][AI_DT_WIN + 1];     // vòng: nhiệt cell để tính dT
  float hist_p_[AI_DT_WIN + 1];                 // vòng: trung bình pack
  int   hi_ = 0;

  float std_buf_[AI_N_CELLS][AI_STD_WIN];       // vòng: cho độ lệch chuẩn trượt
  int   si_ = 0;

  float dev_ema_[AI_N_CELLS];
  bool  ema_init_ = false;

  uint16_t run_[AI_N_CELLS];                    // đếm giây vượt ngưỡng liên tục

  float feat_[AI_N_CELLS][AI_N_FEAT];           // đặc trưng của bước hiện tại

  static float infer(const float* z16);         // 1 cell -> sai số tái tạo
};
