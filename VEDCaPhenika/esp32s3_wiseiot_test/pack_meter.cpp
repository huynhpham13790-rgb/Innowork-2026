#include <Arduino.h>
#include <Wire.h>
#include "pack_meter.h"

/* Thanh ghi INA228 — datasheet TI SBOS939, bảng 7-9.
   Đối chiếu thêm với thư viện RobTillaart/INA228 (đã chạy thực tế nhiều năm)
   để chắc mấy hằng số LSB, vì sai một con số ở đây thì mọi phép đo đều lệch
   theo một hệ số cố định mà nhìn log không ra. */
static const uint8_t REG_CONFIG     = 0x00;
static const uint8_t REG_ADC_CONFIG = 0x01;
static const uint8_t REG_SHUNT_CAL  = 0x02;
static const uint8_t REG_VBUS       = 0x05;
static const uint8_t REG_DIETEMP    = 0x06;
static const uint8_t REG_CURRENT    = 0x07;
static const uint8_t REG_CHARGE     = 0x0A;
static const uint8_t REG_DEVICE_ID  = 0x3F;

// LSB do phần cứng quy định, không đổi được.
static const float LSB_VBUS    = 195.3125e-6f;   // V mỗi bước
static const float LSB_DIETEMP = 7.8125e-3f;     // °C mỗi bước

/* Thanh ghi INA226 — datasheet TI SBOS547. Đánh số khác hẳn INA228, nên để
   riêng chứ không dùng chung hằng số, tránh cái bẫy "trùng tên khác nghĩa". */
static const uint8_t R226_CONFIG  = 0x00;
static const uint8_t R226_SHUNT_V = 0x01;
static const uint8_t R226_BUS_V   = 0x02;
static const uint8_t R226_CAL     = 0x05;
static const uint8_t R226_MFR_ID  = 0xFE;   // phải là 0x5449 = "TI"
static const uint8_t R226_DIE_ID  = 0xFF;   // phải là 0x2260

static const float LSB226_SHUNT = 2.5e-6f;    // V mỗi bước, cố định
static const float LSB226_BUS   = 1.25e-3f;   // V mỗi bước, cố định
// Toàn thang chân shunt của INA226: ±81,92 mV (32768 bước × 2,5 µV).
static const float V226_SHUNT_FS = 0.08192f;

bool PackMeter::begin(int sda, int scl) {
  Wire.begin(sda, scl);
  Wire.setClock(400000);

  /* Quyết định (1) mở rộng: ACK ở 0x40 KHÔNG chứng minh đó là chip nào — rất
     nhiều chip khác dùng chung địa chỉ này. Thử lần lượt hai bộ mã định danh.
     Thử INA228 TRƯỚC: thanh ghi 0x3F của INA226 không tồn tại nên đọc ra giá
     trị không xác định, còn 0xFE/0xFF của INA228 lại là thanh ghi hợp lệ khác
     — thứ tự ngược lại dễ nhận nhầm hơn. */
  if (beginIna228()) { chip_ = PM_CHIP_INA228; r_shunt_ = PM_R_SHUNT_228; }
  else if (beginIna226()) { chip_ = PM_CHIP_INA226; r_shunt_ = PM_R_SHUNT_226; }
  else {
    Serial.println("[INA ] khong nhan duoc chip nao o 0x40 - kiem tra SDA/SCL, VCC, GND chung");
    return false;
  }

  present_ = true;
  Serial.printf("[INA ] OK, chip %s, shunt %.4f ohm, CURRENT_LSB %.3f uA, "
                "do toi +-%.3f A\n",
                chipName(), r_shunt_, current_lsb_ * 1e6f, currentFullScale());
  if (chip_ == PM_CHIP_INA226) {
    Serial.println("[INA ] ⚠️ INA226: toan thang chi +-0,819 A - du cho dien tro suoi,"
                   " KHONG du do dong sac/xa pack that");
    Serial.println("[INA ] ⚠️ INA226 khong co thanh ghi CHARGE - dem coulomb bang phan mem");
    Serial.println("[INA ] ⚠️ INA226 khong co cam bien nhiet trong chip - die_temp = NAN");
  }
  Serial.println("[INA ] ⚠️ dien tro shunt chua duoc do lai - sai so tuyet doi ~5%");
  return true;
}

const char* PackMeter::chipName() const {
  switch (chip_) {
    case PM_CHIP_INA226: return "INA226";
    case PM_CHIP_INA228: return "INA228";
    default:             return "none";
  }
}

float PackMeter::currentFullScale() const {
  if (r_shunt_ <= 0) return 0;
  // Toàn thang là giới hạn của CHÂN SHUNT chia điện trở, không phải PM_I_MAX.
  // PM_I_MAX chỉ đặt độ phân giải của INA228, không mở rộng được thang đo.
  return (chip_ == PM_CHIP_INA226 ? V226_SHUNT_FS : 0.16384f) / r_shunt_;
}

bool PackMeter::beginIna228() {
  uint16_t id = 0;
  if (!r16(REG_DEVICE_ID, id)) return false;
  if ((id >> 4) != 0x228) return false;

  w16(REG_CONFIG, 0x8000);        // reset toàn bộ, bỏ mọi cấu hình cũ
  delay(2);

  /* Quyết định (2): ADCRANGE = 0 => dải shunt rộng ±163,84 mV.
     Với shunt 0,015 Ω thì đo được tới ±10,9 A. Dải hẹp cho độ phân giải gấp 4
     nhưng chỉ tới 2,73 A — xả sẽ tràn, và số tràn là số sai trông hợp lý. */
  w16(REG_CONFIG, 0x0000);

  /* ADC_CONFIG: chế độ liên tục đo cả shunt/bus/nhiệt (MODE=0xF), thời gian
     chuyển đổi 1052 µs mỗi kênh, trung bình 64 mẫu.
     Trung bình 64 mẫu là có chủ đích: mạch sạc băm xung, dòng tức thời nhiễu
     nặng. Lớp 2 cần dòng TRUNG BÌNH để thấy đoạn CV thoải dần, không cần dòng
     tức thời. 64 mẫu × 1052 µs ≈ 67 ms mỗi kênh — vẫn nhanh hơn nhịp 1 Hz rất
     nhiều. */
  w16(REG_ADC_CONFIG, 0xFB6A);

  /* SHUNT_CAL = 13107,2e6 × CURRENT_LSB × R_SHUNT   (datasheet công thức 2)
     CURRENT_LSB = I_MAX / 2^19 */
  current_lsb_ = PM_I_MAX / 524288.0f;
  const float cal = 13107.2e6f * current_lsb_ * PM_R_SHUNT_228;
  if (cal > 65535.0f) {          // cấu hình sai thì nói ra, đừng ghi tràn im lặng
    Serial.printf("[INA ] SHUNT_CAL = %.0f > 65535 - giam PM_I_MAX hoac PM_R_SHUNT_228\n", cal);
    return false;
  }
  w16(REG_SHUNT_CAL, (uint16_t)(cal + 0.5f));
  return true;
}

bool PackMeter::beginIna226() {
  uint16_t mfr = 0, die = 0;
  if (!r16(R226_MFR_ID, mfr) || !r16(R226_DIE_ID, die)) return false;
  if (mfr != 0x5449 || die != 0x2260) return false;

  w16(R226_CONFIG, 0x8000);       // reset
  delay(2);

  /* AVG = 16 mẫu, VBUSCT = VSHCT = 1,1 ms, chế độ liên tục shunt+bus.
     Một lượt ≈ 2 × 1,1 ms × 16 ≈ 35 ms — vẫn nhanh hơn nhịp 1 Hz rất nhiều.
     Cùng lý do với INA228: mạch sạc băm xung, Lớp 2 cần dòng TRUNG BÌNH để
     thấy đoạn CV thoải dần, không cần dòng tức thời.
     Hệ quả kèm theo: KHÔNG được băm PWM nhanh bất cứ tải nào nằm trong đường
     đo này — 35 ms trung bình hoá trên ~35 chu kỳ băm 1 kHz cho ra một con số
     không ứng với trạng thái nào cả. */
  if (!w16(R226_CONFIG, 0x4527)) return false;

  /* CAL = 0,00512 / (CURRENT_LSB × R_SHUNT)   (datasheet công thức 1)
     Chọn CURRENT_LSB = 1 A / 2^15 = 30,5175 µA. Con số 1 A chỉ là hệ số thang,
     KHÔNG mở rộng được dải đo — dải đo do chân shunt ±81,92 mV quyết định. */
  current_lsb_ = 1.0f / 32768.0f;
  const float cal = 0.00512f / (current_lsb_ * PM_R_SHUNT_226);
  if (cal > 65535.0f) {
    Serial.printf("[INA ] CAL = %.0f > 65535 - cau hinh sai\n", cal);
    return false;
  }
  if (!w16(R226_CAL, (uint16_t)(cal + 0.5f))) return false;
  q_as_ = 0; q_started_ = false;
  return true;
}

void PackMeter::resetCharge() {
  if (!present_) return;
  if (chip_ == PM_CHIP_INA226) {
    // Không có thanh ghi tích luỹ để xoá — xoá bộ đếm phần mềm.
    q_as_ = 0;
    q_started_ = false;
    return;
  }
  uint16_t cfg;
  if (!r16(REG_CONFIG, cfg)) return;
  w16(REG_CONFIG, cfg | 0x4000);        // bit RSTACC
}

PackMeasurement PackMeter::read() {
  PackMeasurement m = {};
  m.valid = false;
  if (!present_) return m;
  return chip_ == PM_CHIP_INA226 ? readIna226() : readIna228();
}

/* --------------------------------------------------------------- INA226 ---- */
PackMeasurement PackMeter::readIna226() {
  PackMeasurement m = {};
  m.valid = false;

  uint16_t sh_raw, bus_raw;
  if (!r16(R226_SHUNT_V, sh_raw) || !r16(R226_BUS_V, bus_raw)) {
    n_err_++;
    return m;
  }

  const float v_shunt = (int16_t)sh_raw * LSB226_SHUNT;   // V, có dấu
  m.voltage  = bus_raw * LSB226_BUS;
  m.cell_v   = m.voltage / (float)PM_N_CELLS;
  /* Tính dòng TỪ ĐIỆN ÁP SHUNT, không đọc thanh ghi CURRENT. Hai đường cho
     cùng kết quả (đã đo: lệch 0,1 mA trên 605 mA), nhưng đường này không phụ
     thuộc thanh ghi CAL — nếu CAL ghi hỏng thì dòng vẫn đúng. */
  m.current  = v_shunt / r_shunt_;
  m.die_temp = NAN;                       // INA226 không có cảm biến nhiệt

  /* Quyết định (5) — CHẶN SỐ BÃO HOÀ.
     Với shunt 0,1 Ω, chân shunt chỉ chịu tới ±0,819 A. Vượt qua là thanh ghi
     dừng ở giá trị lớn nhất, và con số đó trông hoàn toàn hợp lý — đúng kiểu
     hỏng âm thầm mà cả dự án này chống. Dùng 99 % toàn thang làm ngưỡng vì
     đúng đáy/đỉnh thang thì nhiễu đã kẹp giá trị từ trước đó rồi. */
  if (fabsf(v_shunt) >= V226_SHUNT_FS * 0.99f) {
    n_sat_++;
    return m;
  }

  /* Đếm coulomb bằng phần mềm. Bỏ qua lượt ĐẦU TIÊN: chưa có mốc thời gian
     trước đó, lấy Δt tính từ millis()=0 sẽ cộng một cục Ah bịa vào. */
  const uint32_t now = millis();
  if (q_started_) {
    const uint32_t dt = now - t_q_ms_;    // trừ theo uint32 nên tràn vẫn đúng
    if (dt < 60000u) q_as_ += (double)m.current * dt / 1000.0;
    // dt ≥ 60 s nghĩa là đã lỡ nhịp quá lâu; cộng vào thì con số Ah thành bịa.
  }
  t_q_ms_ = now;
  q_started_ = true;
  m.charge_ah = (float)(q_as_ / 3600.0);

  if (m.voltage < PM_V_MIN || m.voltage > PM_V_MAX ||
      fabsf(m.current) > PM_I_ABS_MAX || isnan(m.voltage)) {
    n_err_++;
    return m;
  }
  m.valid = true;
  return m;
}

/* --------------------------------------------------------------- INA228 ---- */
PackMeasurement PackMeter::readIna228() {
  PackMeasurement m = {};
  m.valid = false;

  int32_t vbus_raw, cur_raw;
  uint16_t t_raw;
  int64_t q_raw;
  if (!r24(REG_VBUS, vbus_raw, false) || !r24(REG_CURRENT, cur_raw, true) ||
      !r16(REG_DIETEMP, t_raw) || !r40(REG_CHARGE, q_raw)) {
    n_err_++;
    return m;
  }

  m.voltage  = vbus_raw * LSB_VBUS;
  m.cell_v   = m.voltage / (float)PM_N_CELLS;
  m.current  = cur_raw * current_lsb_;
  m.die_temp = (int16_t)t_raw * LSB_DIETEMP;
  // Thanh ghi CHARGE tính bằng coulomb (A·s); chia 3600 ra Ah.
  m.charge_ah = (float)((double)q_raw * current_lsb_ / 3600.0);

  /* Quyết định (3): kiểm khoảng hợp lý. Số ngoài khoảng này là hỏng dây hoặc
     hỏng chip, KHÔNG phải pin bất thường — nên trả về không hợp lệ chứ không
     đẩy vào AI rồi để AI báo động nhầm. */
  if (m.voltage < PM_V_MIN || m.voltage > PM_V_MAX ||
      fabsf(m.current) > PM_I_ABS_MAX || isnan(m.voltage)) {
    n_err_++;
    return m;
  }

  m.valid = true;
  return m;
}

/* ------------------------------------------------------------------ I2C thô */

bool PackMeter::w16(uint8_t reg, uint16_t val) {
  Wire.beginTransmission(PM_I2C_ADDR);
  Wire.write(reg);
  Wire.write(val >> 8);
  Wire.write(val & 0xFF);
  return Wire.endTransmission() == 0;
}

bool PackMeter::r16(uint8_t reg, uint16_t& out) {
  Wire.beginTransmission(PM_I2C_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)PM_I2C_ADDR, (uint8_t)2) != 2) return false;
  out = ((uint16_t)Wire.read() << 8) | Wire.read();
  return true;
}

bool PackMeter::r24(uint8_t reg, int32_t& out, bool is_signed) {
  Wire.beginTransmission(PM_I2C_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)PM_I2C_ADDR, (uint8_t)3) != 3) return false;
  const uint8_t b0 = Wire.read(), b1 = Wire.read(), b2 = Wire.read();
  out = pm_decode24(b0, b1, b2, is_signed);
  return true;
}

bool PackMeter::r40(uint8_t reg, int64_t& out) {
  Wire.beginTransmission(PM_I2C_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)PM_I2C_ADDR, (uint8_t)5) != 5) return false;
  uint8_t b[5];
  for (int i = 0; i < 5; i++) b[i] = Wire.read();
  out = pm_decode40(b);
  return true;
}
