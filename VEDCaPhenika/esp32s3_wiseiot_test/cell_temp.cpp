#include "cell_temp.h"
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

static OneWire           s_wire(4);       // pin đặt lại trong begin()
static DallasTemperature s_dallas(&s_wire);

bool CellTemp::begin(uint8_t pin) {
  pin_ = pin;
  s_wire.begin(pin);
  s_dallas.setOneWire(&s_wire);           // cũng reset cờ parasite về false
  s_dallas.begin();

  st_ = CellTempStatus{};
  st_.n_found = s_dallas.getDeviceCount();

  // Cờ ký sinh của cả bus KHÔNG đủ để kết luận (QĐ-023): trong
  // DallasTemperature 4.0.6 nó chỉ có chiều bật, và quyết định bằng đúng một
  // lần hỏi mỗi con lúc quét bus. Một lần nhiễu thoáng qua là chốt cả phiên.
  // Nên hỏi lại từng con ở đây.
  uint8_t n_para = 0;
  for (uint8_t i = 0; i < CT_N; i++)
    if (s_dallas.readPowerSupply(DS_ROM[i])) n_para++;
  if (n_para) {
    Serial.printf("[TEMP] CANH BAO: %u con bao nguon ky sinh - kiem tra day VDD\n",
                  n_para);
  }

  // Đối chiếu từng ROM trong bảng với bus. Thiếu một con mà vẫn chạy tiếp là
  // đúng cái bẫy đã dính lúc đo hiệu chuẩn: chương trình chạy bình thường,
  // in ra bảng đẹp, chỉ là 6 cột thay vì 8, không cảnh báo gì.
  bool all = true;
  for (uint8_t i = 0; i < CT_N; i++) {
    if (s_dallas.isConnected(DS_ROM[i])) {
      s_dallas.setResolution(DS_ROM[i], 12);   // 0,0625 °C — khớp dữ liệu train
      st_.healthy[i] = true;
      st_.n_healthy++;
    } else {
      Serial.printf("[TEMP] KHONG THAY cam bien P%02u trong bang DS_ROM\n", i + 1);
      all = false;
    }
    t_[i] = NAN;
  }

  // Cảm biến môi trường (con thứ 9). Không có nó thì vẫn chạy được — Lớp 1
  // chủ yếu dùng đặc trưng tương đối giữa các cell — nhưng phải báo, vì thiếu
  // nó nghĩa là quay về dùng hằng số giả định.
  amb_present_ = s_dallas.isConnected(DS_ROM_AMBIENT);
  if (amb_present_) {
    s_dallas.setResolution(DS_ROM_AMBIENT, 12);
    Serial.println("[TEMP] co cam bien moi truong (con thu 9)");
#if !DS_AMBIENT_CALIBRATED
    Serial.println("[TEMP] CANH BAO: offset cam bien moi truong CHUA HIEU CHUAN");
#endif
  } else {
    Serial.println("[TEMP] KHONG co cam bien moi truong - se dung gia tri du phong");
  }

  s_dallas.setWaitForConversion(false);   // bắt buộc: xem chú thích ở cell_temp.h

  // Số cảm biến MONG ĐỢI = 8 cell + 1 môi trường (nếu có mặt). So với CT_N
  // là sai: cắm thêm con thứ 9 thành ra báo "thiếu cảm biến". Điều thật sự
  // cần kiểm là từng ROM trong bảng có mặt hay không (biến `all`), còn tổng
  // số chỉ dùng để phát hiện có con LẠ trên bus.
  const uint8_t expect = CT_N + (amb_present_ ? 1 : 0);
  Serial.printf("[TEMP] tim thay %u cam bien (mong doi %u), %u/%u kenh cell khoe\n",
                st_.n_found, expect, st_.n_healthy, CT_N);
  if (st_.n_found > expect)
    Serial.printf("[TEMP] CANH BAO: co %u con LA tren bus, khong nam trong bang ROM\n",
                  st_.n_found - expect);
  return all && st_.n_found >= expect;
}

bool CellTemp::update() {
  if (!converting_) {
    s_dallas.requestTemperatures();      // trả về ngay
    convStart_ = millis();
    converting_ = true;
    return false;
  }
  if (millis() - convStart_ < CT_CONV_MS) return false;
  converting_ = false;

  // Vòng 1: đọc thô, cập nhật sức khoẻ từng kênh.
  float  raw[CT_N];
  bool   good[CT_N];
  double sum = 0;
  uint8_t n = 0;

  for (uint8_t i = 0; i < CT_N; i++) {
    float v = s_dallas.getTempC(DS_ROM[i]);
    // 85,0 chẵn là giá trị thanh ghi lúc bật nguồn: đọc trước khi chuyển đổi
    // xong, hoặc cảm biến vừa bị reset vì sụt áp. Không phải nhiệt độ thật.
    good[i] = !(v == DEVICE_DISCONNECTED_C || v == 85.0f ||
                v < CT_T_MIN || v > CT_T_MAX);
    if (good[i]) {
      raw[i] = v - DS_OFFSET[i];         // trừ sai số chế tạo (QĐ-022)
      st_.n_ok[i]++;
      st_.consec_err[i] = 0;
      st_.consec_ok[i]++;
      // Phục hồi có TRỄ: phải tốt liên tiếp CT_MIN_CONSEC_OK lần mới cho sống
      // lại. Xem lý do ở cell_temp.h.
      if (!st_.healthy[i] && st_.consec_ok[i] >= CT_MIN_CONSEC_OK) {
        st_.healthy[i] = true;
        st_.n_flap++;
        Serial.printf("[TEMP] kenh %u da doc tot %u lan lien tiep - cho dung lai\n",
                      i + 1, CT_MIN_CONSEC_OK);
      }
      // Kênh đang bị loại thì giá trị của nó chưa được tin, không tính vào
      // trung bình pack.
      if (st_.healthy[i]) { sum += raw[i]; n++; } else { good[i] = false; }
    } else {
      raw[i] = NAN;
      st_.n_err[i]++;
      st_.consec_err[i]++;
      st_.consec_ok[i] = 0;
      if (st_.consec_err[i] >= CT_MAX_CONSEC && st_.healthy[i]) {
        st_.healthy[i] = false;
        st_.n_flap++;
        Serial.printf("[TEMP] *** KENH %u HONG *** %lu lan loi lien tiep - "
                      "loai khoi dau vao Lop 1\n", i + 1, st_.consec_err[i]);
      }
    }
  }

  st_.n_healthy = 0;
  for (uint8_t i = 0; i < CT_N; i++) if (st_.healthy[i]) st_.n_healthy++;

  if (n == 0) return false;              // mất cả bus, giữ nguyên giá trị cũ
  float mean = (float)(sum / n);

  // Vòng 2: kênh hỏng thay bằng trung bình các kênh khoẻ. Làm vậy để đặc trưng
  // tương đối của nó ra ~0 ("trông bình thường") thay vì thành số rác kéo lệch
  // trung bình pack và làm sai điểm của CẢ 8 cell. Việc báo hỏng đã do
  // healthy[] và publishSensorHealth() lo.
  for (uint8_t i = 0; i < CT_N; i++) t_[i] = good[i] ? raw[i] : mean;

  if (amb_present_) {
    float a = s_dallas.getTempC(DS_ROM_AMBIENT);
    bool ok = !(a == DEVICE_DISCONNECTED_C || a == 85.0f ||
                a < CT_T_MIN || a > CT_T_MAX);
    amb_ = ok ? (a - DS_AMBIENT_OFFSET) : NAN;
  }

  st_.ready = true;
  return true;
}

float CellTemp::errorRate() const {
  uint32_t ok = 0, err = 0;
  for (uint8_t i = 0; i < CT_N; i++) { ok += st_.n_ok[i]; err += st_.n_err[i]; }
  return (ok + err) ? (100.0f * err / (ok + err)) : 0.0f;
}

void CellTemp::printStatus() const {
  Serial.printf("[TEMP] %u/%u kenh khoe, ti le loi %.3f%%, %lu lan doi trang thai |",
                st_.n_healthy, CT_N, errorRate(), st_.n_flap);
  for (uint8_t i = 0; i < CT_N; i++)
    Serial.printf(" %.2f%s", t_[i], st_.healthy[i] ? "" : "(X)");
  if (amb_present_) Serial.printf("  moi truong %.2f", amb_);
  Serial.println();
}
