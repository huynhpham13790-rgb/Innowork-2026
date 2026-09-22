#include "rul_onboard.h"
#include "rul_model.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <math.h>

/* Bản dịch 1-1 của planb_cloud/nodered/rul_predict.js. Giữ NGUYÊN thứ tự phép
   tính và các mốc chặn — lệch một chỗ là hai nơi ra hai số khác nhau cho cùng
   một chu kỳ sạc, mà cả hai đều trông hợp lý nên không ai phát hiện. */
RulResult rulPredict(const ChargeSummary& s) {
  RulResult r = {};
  if (!s.valid) return r;

  float z[RUL_N_FEAT];
  uint8_t bad = 0;
  for (int i = 0; i < RUL_N_FEAT; i++) {
    // std bằng 0 nghĩa là đặc trưng đó hằng số trong tập train — chia cho nó
    // là NaN lan ra toàn bộ kết quả. Không xảy ra với bộ hệ số hiện tại,
    // nhưng bộ sau thì chưa chắc.
    const float sd = (RUL_STD[i] > 1e-12f) ? RUL_STD[i] : 1.0f;
    z[i] = (s.f[i] - RUL_MEAN[i]) / sd;
    if (!isfinite(z[i])) return r;            // đầu vào rác thì KHÔNG đoán
    if (fabsf(z[i]) > RUL_Z_OUTLIER) bad++;
  }

  r.rul_raw = RUL_COEF * z[RUL_FEAT_IDX] + RUL_INTERCEPT;
  r.rul_cycles = r.rul_raw < 0.0f ? 0.0f : r.rul_raw;

  float soh = SOH_INTERCEPT;
  for (int i = 0; i < RUL_N_FEAT; i++) soh += SOH_COEF[i] * z[i];
  r.soh = soh < 0.0f ? 0.0f : (soh > 1.2f ? 1.2f : soh);

  r.n_outliers   = bad;
  r.extrapolating = bad > 0;
  r.valid = true;

  Serial.printf("[RUL ] tren chip: RUL %.1f chu ky (tho %.1f), SOH %.1f%%%s\n",
                r.rul_cycles, r.rul_raw, r.soh * 100.0f,
                r.extrapolating ? "  ⚠️ NGOAI DAI HUAN LUYEN" : "");
  if (r.extrapolating) {
    Serial.printf("[RUL ] %u/%d dac trung vuot +-%.0f do lech chuan - con so nay "
                  "KHONG dang tin\n", r.n_outliers, RUL_N_FEAT, RUL_Z_OUTLIER);
  }
  return r;
}

/* ---------------------------------------------------------------------------
 *  Lịch sử trên flash — vòng tròn đơn giản, ghi nối đuôi rồi cắt bớt đầu.
 *
 *  KHÔNG dùng cấu trúc vòng ghi đè tại chỗ: nó tiết kiệm hơn nhưng khi file
 *  hỏng giữa chừng thì không còn cách nào biết bản ghi nào mới bản nào cũ.
 *  Ghi nối đuôi thì file luôn đọc được từ đầu đến cuối, hỏng thì mất đuôi chứ
 *  không mất nghĩa. Với 32 byte/chu kỳ và vài tiếng một lần, hao mòn flash
 *  không phải vấn đề cần tối ưu.
 * ------------------------------------------------------------------------ */
bool rulHistBegin() {
  if (!LittleFS.begin(true)) {
    Serial.println("[RUL ] LittleFS hong - khong luu duoc lich su chu ky sac");
    return false;
  }
  Serial.printf("[RUL ] lich su: %d chu ky da luu\n", rulHistCount());
  return true;
}

int rulHistCount() {
  File f = LittleFS.open(RUL_HIST_PATH, "r");
  if (!f) return 0;
  const int n = f.size() / sizeof(RulHistEntry);
  f.close();
  return n;
}

bool rulHistAppend(const RulResult& r, const ChargeSummary& s, uint32_t ts) {
  if (!r.valid) return false;

  RulHistEntry e = {};
  e.ts    = ts;
  e.rul   = r.rul_cycles;
  e.soh   = r.soh;
  e.t_cv  = s.f[RUL_FEAT_IDX];
  e.flags = r.extrapolating ? 1u : 0u;

  File f = LittleFS.open(RUL_HIST_PATH, "a");
  if (!f) return false;
  const bool ok = f.write((uint8_t*)&e, sizeof(e)) == sizeof(e);
  f.close();
  if (!ok) return false;

  /* Quá hạn mức thì cắt bớt phần ĐẦU (cũ nhất). Viết ra file tạm rồi đổi tên,
     không sửa tại chỗ — mất điện giữa chừng thì còn nguyên file cũ, thay vì
     còn một file cụt đầu không ai đọc được. */
  if (rulHistCount() > RUL_HIST_MAX) {
    File src = LittleFS.open(RUL_HIST_PATH, "r");
    File dst = LittleFS.open("/rulhist.tmp", "w");
    if (src && dst) {
      src.seek((rulHistCount() - RUL_HIST_MAX) * sizeof(RulHistEntry));
      uint8_t buf[256];
      while (int n = src.read(buf, sizeof(buf))) dst.write(buf, n);
      src.close(); dst.close();
      LittleFS.remove(RUL_HIST_PATH);
      LittleFS.rename("/rulhist.tmp", RUL_HIST_PATH);
    } else {
      if (src) src.close();
      if (dst) dst.close();
    }
  }
  return true;
}

int rulHistRecent(RulHistEntry* out, int n) {
  File f = LittleFS.open(RUL_HIST_PATH, "r");
  if (!f) return 0;
  const int have = f.size() / sizeof(RulHistEntry);
  const int take = n < have ? n : have;
  f.seek((have - take) * sizeof(RulHistEntry));
  int got = 0;
  while (got < take && f.read((uint8_t*)&out[got], sizeof(RulHistEntry))
                        == sizeof(RulHistEntry)) got++;
  f.close();
  return got;
}

/* Độ dốc SOH theo chu kỳ, bằng bình phương tối thiểu trên chỉ số bản ghi.
   Dùng chỉ số chứ không dùng thời gian: xe sạc thưa hay mau không đổi bản chất
   lão hoá, cái đếm là SỐ CHU KỲ. Đây cũng đúng đơn vị mà mô hình dùng. */
float rulHistSohSlope(int n) {
  static RulHistEntry buf[64];
  if (n > 64) n = 64;
  const int got = rulHistRecent(buf, n);
  if (got < 3) return NAN;              // 2 điểm luôn thẳng hàng, vô nghĩa

  double sx = 0, sy = 0, sxx = 0, sxy = 0;
  for (int i = 0; i < got; i++) {
    const double x = i, y = buf[i].soh * 100.0;   // điểm phần trăm
    sx += x; sy += y; sxx += x * x; sxy += x * y;
  }
  const double den = got * sxx - sx * sx;
  if (fabs(den) < 1e-9) return NAN;
  return (float)((got * sxy - sx * sy) / den);
}
