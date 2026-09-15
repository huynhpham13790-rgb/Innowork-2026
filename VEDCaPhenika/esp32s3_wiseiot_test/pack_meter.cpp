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

bool PackMeter::begin(int sda, int scl) {
  Wire.begin(sda, scl);
  Wire.setClock(400000);

  // Quyết định (1): ACK ở địa chỉ 0x40 KHÔNG chứng minh đó là INA228 — rất
  // nhiều chip khác dùng chung địa chỉ này. Phải đọc DEVICE_ID.
  uint16_t id = 0;
  if (!r16(REG_DEVICE_ID, id)) {
    Serial.println("[INA ] khong doc duoc DEVICE_ID - kiem tra day SDA/SCL va nguon");
    return false;
  }
  if ((id >> 4) != 0x228) {
    Serial.printf("[INA ] DEVICE_ID = 0x%04X, KHONG phai INA228 (mong 0x228x)\n", id);
    return false;
  }

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
  const float cal = 13107.2e6f * current_lsb_ * PM_R_SHUNT;
  if (cal > 65535.0f) {          // cấu hình sai thì nói ra, đừng ghi tràn im lặng
    Serial.printf("[INA ] SHUNT_CAL = %.0f > 65535 - giam PM_I_MAX hoac PM_R_SHUNT\n", cal);
    return false;
  }
  w16(REG_SHUNT_CAL, (uint16_t)(cal + 0.5f));

  present_ = true;
  Serial.printf("[INA ] OK, ID=0x%04X, shunt %.4f ohm, CURRENT_LSB %.3f uA, "
                "do toi +-%.1f A / %.1f V\n",
                id, PM_R_SHUNT, current_lsb_ * 1e6f,
                0.16384f / PM_R_SHUNT, 85.0f);
  Serial.println("[INA ] ⚠️ PM_R_SHUNT chua duoc do lai - sai so tuyet doi ~5%");
  return true;
}

void PackMeter::resetCharge() {
  if (!present_) return;
  uint16_t cfg;
  if (!r16(REG_CONFIG, cfg)) return;
  w16(REG_CONFIG, cfg | 0x4000);        // bit RSTACC
}

PackMeasurement PackMeter::read() {
  PackMeasurement m = {};
  m.valid = false;
  if (!present_) return m;

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
