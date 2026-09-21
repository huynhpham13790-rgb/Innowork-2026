/* =============================================================================
 *  Bản C của ai/features.py + autoencoder. Mọi công thức ở đây có một dòng
 *  tương ứng trong features.py — sửa một bên mà quên bên kia là hỏng mô hình,
 *  nên ai/test_c_vs_python.sh so từng phần tử để bắt việc đó.
 * ========================================================================== */
#include "cell_ai.h"
#include "cell_ae_weights.h"
#include <math.h>
#include <string.h>

float    CellAI::threshold() { return AE_THRESHOLD; }
uint16_t CellAI::persist_s() { return AE_PERSIST_S; }

void CellAI::begin() {
  n_ = 0; hi_ = 0; si_ = 0; ema_init_ = false;
  memset(hist_t_, 0, sizeof(hist_t_));
  memset(hist_p_, 0, sizeof(hist_p_));
  memset(std_buf_, 0, sizeof(std_buf_));
  memset(dev_ema_, 0, sizeof(dev_ema_));
  memset(run_, 0, sizeof(run_));
  memset(feat_, 0, sizeof(feat_));
}

// Autoencoder: 4 lớp dense, 3 lớp đầu ReLU, lớp cuối tuyến tính.
// Trả về sai số tái tạo trung bình bình phương (giống np.mean((rec-z)**2)).
float CellAI::infer(const float* z) {
  float a[AE_L0_OUT], b[AE_L1_OUT], c[AE_L2_OUT], out[AE_L3_OUT];

  for (int o = 0; o < AE_L0_OUT; o++) {
    float s = AE_B0[o];
    for (int i = 0; i < AE_L0_IN; i++) s += AE_W0[o * AE_L0_IN + i] * z[i];
    a[o] = s > 0.0f ? s : 0.0f;
  }
  for (int o = 0; o < AE_L1_OUT; o++) {
    float s = AE_B1[o];
    for (int i = 0; i < AE_L1_IN; i++) s += AE_W1[o * AE_L1_IN + i] * a[i];
    b[o] = s > 0.0f ? s : 0.0f;
  }
  for (int o = 0; o < AE_L2_OUT; o++) {
    float s = AE_B2[o];
    for (int i = 0; i < AE_L2_IN; i++) s += AE_W2[o * AE_L2_IN + i] * b[i];
    c[o] = s > 0.0f ? s : 0.0f;
  }
  float err = 0.0f;
  for (int o = 0; o < AE_L3_OUT; o++) {
    float s = AE_B3[o];
    for (int i = 0; i < AE_L3_IN; i++) s += AE_W3[o * AE_L3_IN + i] * c[i];
    out[o] = s;                       // lớp cuối tuyến tính
    float d = s - z[o];
    err += d * d;
  }
  (void)out;
  return err / (float)AE_L3_OUT;
}

void CellAI::features_of(int cell, float* out16) const {
  memcpy(out16, feat_[cell], sizeof(float) * AI_N_FEAT);
}

CellAIResult CellAI::update(const float* temps, float ambient,
                            float current, float soc) {
  CellAIResult res;
  memset(&res, 0, sizeof(res));
  const int N = AI_N_CELLS;

  // ---- thống kê pack ------------------------------------------------------
  float sum = 0.0f, tmax = temps[0], tmin = temps[0];
  for (int i = 0; i < N; i++) {
    sum += temps[i];
    if (temps[i] > tmax) tmax = temps[i];
    if (temps[i] < tmin) tmin = temps[i];
  }
  const float pack_mean = sum / N;

  float var = 0.0f;
  for (int i = 0; i < N; i++) { float d = temps[i] - pack_mean; var += d * d; }
  const float pack_std = sqrtf(var / N);     // độ lệch chuẩn tổng thể (ddof=0)

  // trung vị: chép ra rồi sắp xếp chèn (N=8, không đáng kể)
  float s[AI_N_CELLS];
  memcpy(s, temps, sizeof(s));
  for (int i = 1; i < N; i++) {
    float v = s[i]; int j = i - 1;
    while (j >= 0 && s[j] > v) { s[j + 1] = s[j]; j--; }
    s[j + 1] = v;
  }
  const float pack_med = (s[N / 2 - 1] + s[N / 2]) * 0.5f;
  const float spread = tmax - tmin;

  // ---- vòng đệm cho tốc độ đổi nhiệt --------------------------------------
  hi_ = (hi_ + 1) % (AI_DT_WIN + 1);
  for (int i = 0; i < N; i++) hist_t_[i][hi_] = temps[i];
  hist_p_[hi_] = pack_mean;
  const int old = (hi_ + 1) % (AI_DT_WIN + 1);       // mẫu của AI_DT_WIN giây trước
  const bool have_dt = n_ >= AI_DT_WIN;
  // features.py: _rate_per_min chia cho (win/60) -> nhân 60/win
  const float rate_k = 60.0f / (float)AI_DT_WIN;
  const float dT_pack = have_dt ? (pack_mean - hist_p_[old]) * rate_k : 0.0f;

  // ---- vòng đệm cho độ lệch chuẩn trượt -----------------------------------
  // features.py dùng cửa sổ MỞ RỘNG cho tới khi đủ AI_STD_WIN mẫu, rồi mới
  // trượt. Phải bắt chước đúng, không thì giá trị lúc mới khởi động lệch nhau.
  for (int i = 0; i < N; i++) std_buf_[i][si_] = temps[i];
  si_ = (si_ + 1) % AI_STD_WIN;
  const int nstd_i = (n_ + 1 < AI_STD_WIN) ? (int)(n_ + 1) : AI_STD_WIN;

  // ---- EMA của độ lệch ----------------------------------------------------
  const float a = 1.0f / (float)AI_EMA_TAU;

  // ---- dựng 16 đặc trưng cho từng cell ------------------------------------
  for (int i = 0; i < N; i++) {
    const float dev = temps[i] - pack_mean;

    if (!ema_init_) dev_ema_[i] = dev;                  // features.py: y[0]=x[0]
    else            dev_ema_[i] += a * (dev - dev_ema_[i]);

    // hạng trung bình khi hoà — khớp định nghĩa trong features.py
    int n_less = 0, n_eq = 0;
    for (int j = 0; j < N; j++) {
      if (temps[j] < temps[i]) n_less++;
      else if (temps[j] == temps[i]) n_eq++;
    }
    const float rank = ((float)n_less + (float)(n_eq - 1) * 0.5f) / (float)(N - 1);

    const float dT_cell = have_dt ? (temps[i] - hist_t_[i][old]) * rate_k : 0.0f;

    // Tính lại độ lệch chuẩn từ vòng đệm mỗi bước, hai lượt (trung bình rồi
    // phương sai), tích luỹ bằng double.
    // KHÔNG dùng công thức E[x^2]-E[x]^2 với tổng chạy float32: nhiệt độ ~25
    // bình phương rồi cộng 60 mẫu ra ~37500, trong khi phương sai thật chỉ
    // ~0,001 -> float32 mất sạch chữ số có nghĩa (đã đo: lệch 1,3e-3 so với
    // Python). Tổng chạy còn tích luỹ sai số vô hạn theo thời gian vì cứ cộng
    // vào rồi trừ ra. 60x8 phép tính mỗi giây là không đáng kể.
    double m = 0.0;
    for (int k = 0; k < nstd_i; k++) m += std_buf_[i][k];
    m /= nstd_i;
    double v = 0.0;
    for (int k = 0; k < nstd_i; k++) { double d = std_buf_[i][k] - m; v += d * d; }
    v /= nstd_i;
    const float t_std = (float)sqrt(v > 0.0 ? v : 0.0);

    float* f = feat_[i];
    // 11 đặc trưng THUẦN TƯƠNG ĐỐI — thứ tự PHẢI khớp REL trong
    // ai/train_ae_relative.py, nếu không thì mô hình ăn sai đầu vào mà vẫn
    // chạy ra số trông hợp lý. test_c_vs_python.py kiểm đúng chỗ này.
    f[0]  = dev;                        // lệch so với trung bình pack
    f[1]  = temps[i] - pack_med;        // lệch so với trung vị pack
    f[2]  = dev / (pack_std + 1e-6f);   // lệch đã chuẩn hoá
    f[3]  = rank;                       // thứ hạng nhiệt trong pack, 0..1
    f[4]  = spread;                     // độ rộng nhiệt của cả pack
    f[5]  = dT_cell;                    // tốc độ đổi nhiệt của cell
    f[6]  = dT_pack;                    // tốc độ đổi nhiệt của pack
    f[7]  = dT_cell - dT_pack;          // cell nóng lên NHANH HƠN pack bao nhiêu
    f[8]  = dev_ema_[i];                // lệch trung bình trượt chậm
    f[9]  = dev - dev_ema_[i];          // lệch đột ngột so với nền
    f[10] = t_std;                      // độ dao động trượt

    // ĐÃ BỎ (QĐ-033): temps[i]-ambient, pack_mean-ambient, |current|/I_SCALE,
    // soc/100, (temps[i]-25)/25. Cả năm đều mang giá trị TUYỆT ĐỐI và giống
    // hệt nhau ở mọi cell, nên không giúp phân biệt cell nào — nhưng lại làm
    // mô hình không dùng được cho pack khác. `ambient`, `current`, `soc` vẫn
    // được nhận vào hàm này vì chúng đi lên dashboard và vào Lớp 2.
  }
  ema_init_ = true;
  n_++;

  // ---- suy luận + quy tắc giữ liên tục ------------------------------------
  // Bỏ giai đoạn khởi động: trước khi cửa sổ trượt đầy thì đặc trưng chưa đáng
  // tin, báo động lúc này gần như chắc chắn là giả.
  res.valid = n_ > AI_STD_WIN;
  res.worst_cell = 0; res.worst_score = 0.0f;

  for (int i = 0; i < N; i++) {
    float z[AI_N_FEAT];
    for (int k = 0; k < AI_N_FEAT; k++)
      z[k] = (feat_[i][k] - AE_FEAT_MEAN[k]) / AE_FEAT_STD[k];

    const float e = infer(z);
    res.score[i] = e;
    if (e > res.worst_score) { res.worst_score = e; res.worst_cell = i; }

    if (res.valid && e > AE_THRESHOLD) {
      if (run_[i] < 0xFFFF) run_[i]++;
    } else {
      run_[i] = 0;
    }
    res.run_s[i] = run_[i];
    if (run_[i] >= AE_PERSIST_S) res.alarm = true;
  }

  /* --- Tra bảng DẠNG bất thường cho cell tệ nhất -------------------------
     Chỉ là tra bảng của docs/HAI_LOP_AI_HOAT_DONG_THE_NAO.md, KHÔNG phải một
     bộ phân loại được huấn luyện, và KHÔNG ảnh hưởng gì tới quyết định báo
     động — báo động đã chốt xong ở vòng lặp trên. Xem chú thích dài ở
     enum AiPattern trong cell_ai.h trước khi sửa mấy dòng này. */
  {
    const int w = res.worst_cell;
    res.dev     = feat_[w][0];    // lệch so với trung bình pack
    res.dt_diff = feat_[w][7];    // nóng nhanh hơn pack bao nhiêu
    res.shock   = feat_[w][9];    // lệch đột ngột so với nền chậm

    /* Thứ tự kiểm CÓ Ý NGHĨA, không được đảo:
       LẠNH xét trước vì nó dễ bị bỏ sót nhất (ai cũng canh nóng) và vì một
       cell mất kết nối là chuyện khẩn cấp theo kiểu khác hẳn.
       NHANH xét trước ẤM vì một cell vừa nóng vừa đang vọt lên thì điều đáng
       nói là NÓ ĐANG VỌT — xếp nó vào "ấm ổn định" là hạ cấp một TH-1 thành
       TH-2, tức biến "cách ly ngay" thành "ghi sổ để mai xem". */
    if (!res.valid)                                    res.pattern = AIP_NONE;
    else if (res.dev <= AIP_COLD_DEV)                  res.pattern = AIP_COLD;
    else if (res.dt_diff >= AIP_FAST_DTDIFF ||
             res.shock   >= AIP_FAST_SHOCK)            res.pattern = AIP_FAST;
    else if (res.dev >= AIP_WARM_DEV)                  res.pattern = AIP_WARM;
    else                                               res.pattern = AIP_NONE;
  }
  return res;
}

const char* aiPatternName(AiPattern p) {
  switch (p) {
    case AIP_FAST: return "NONG LEN NHANH (TH-1)";
    case AIP_WARM: return "nong hon nhung on dinh (TH-2)";
    case AIP_COLD: return "LANH bat thuong (TH-3)";
    default:       return "chua ro dang";
  }
}

const char* aiPatternAction(AiPattern p) {
  switch (p) {
    // Thứ tự việc phải làm lấy nguyên từ TH-1 trong tài liệu: ngắt sạc TRƯỚC,
    // vì lúc đang nạp là lúc nguy hiểm nhất.
    case AIP_FAST: return "NGAT SAC, ngat tai, CACH LY PACK ra cho thoang";
    case AIP_WARM: return "chua khan. Ghi so, kiem o lan bao duong";
    case AIP_COLD: return "kiem moi noi cua cell va xem cam bien con dan chat";
    default:       return "theo doi tiep";
  }
}
