/* =============================================================================
 *  Đo dòng và điện áp pack bằng INA228.
 *
 *  VÌ SAO CẦN: hai con số `current` và `soc` trong runAI() cho tới nay VẪN LÀ
 *  HẰNG SỐ BỊA (0.0f và 80.0f). Lớp 1 có hai đặc trưng ăn dòng điện, và toàn bộ
 *  Lớp 2 dựng trên vật lý sạc CC-CV — không có dòng và áp thật thì Lớp 2 không
 *  tồn tại, chỉ là mô phỏng.
 *
 *  VÌ SAO KHÔNG CẦN MUA GÌ THÊM ĐỂ ĐO ĐIỆN ÁP PACK: INA228 đo điện áp bus tới
 *  85 V bằng ADC 20 bit. Pack 8S tối đa 33,6 V, nằm gọn trong dải. Không cần
 *  cầu chia áp (phi tuyến theo nhiệt, và làm hỏng độ chính xác), không cần
 *  ADS1115.
 *
 *  TIỆN THỂ — INA228 có thanh ghi CHARGE tích luỹ điện lượng ngay trong chip.
 *  Nó tự tích phân dòng theo thời gian, tức là phép "đếm coulomb" cần để đo
 *  dung lượng THẬT làm đáp án chấm điểm cho Lớp 2 (QĐ-026). Một con chip giải
 *  cả hai việc.
 *
 *  BỐN QUYẾT ĐỊNH THIẾT KẾ:
 *
 *  1. KIỂM DEVICE_ID LÚC KHỞI ĐỘNG. Một địa chỉ I2C có ACK không chứng minh
 *     đó là INA228 — rất nhiều chip khác cũng ở 0x40. Đọc 0x3F và so với
 *     0x228x. Cùng tinh thần với việc cell_temp.cpp kiểm từng ROM.
 *
 *  2. CHỌN DẢI SHUNT RỘNG (ADCRANGE=0, ±163,84 mV) chứ không phải dải hẹp.
 *     Dải hẹp cho độ phân giải gấp 4 nhưng với shunt 0,015 Ω chỉ đo được tới
 *     2,73 A — sạc 1,9 A thì vừa, nhưng xả sẽ TRÀN. Số đo tràn là một con số
 *     sai trông hoàn toàn hợp lý, đúng kiểu hỏng âm thầm mà dự án này chống.
 *     Đổi lại chỉ mất độ phân giải xuống ~21 µA, vẫn thừa thãi.
 *
 *  3. KIỂM KHOẢNG HỢP LÝ, không tin mù thanh ghi. Giống cell_temp.cpp: thà
 *     không có số còn hơn có số rác.
 *
 *  4. ĐIỆN TRỞ SHUNT LÀ NGUỒN SAI SỐ TUYỆT ĐỐI LỚN NHẤT. Mọi giá trị dòng đều
 *     tỉ lệ thẳng với nó. Module bán sẵn ghi 0,015 Ω nhưng dung sai thường
 *     ±1 %, và vết hàn thêm vài mΩ nữa. PHẢI đo lại bằng nguồn dòng đã biết
 *     rồi sửa PM_R_SHUNT — chưa làm thì mọi con số Lớp 2 chỉ đúng tới ~5 %.
 * ========================================================================== */
#pragma once
#include <stdint.h>
#include <math.h>

#define PM_I2C_ADDR   0x40      // A0=A1=GND. Module Adafruit mặc định 0x40.
#define PM_R_SHUNT    0.015f    // ohm — ⚠️ PHẢI đo lại, xem quyết định (4)
#define PM_I_MAX      10.0f     // A — dòng lớn nhất trông đợi, đặt CURRENT_LSB
#define PM_N_CELLS    8         // pack 8S

/* Khoảng hợp lý cho pack 8S lithium. Ngoài khoảng này là hỏng dây/hỏng chip,
   không phải pin bất thường — nên trả về không hợp lệ chứ không báo động. */
#define PM_V_MIN      15.0f     // dưới mức này 8 cell đã cạn kiệt (1,9 V/cell)
#define PM_V_MAX      35.0f     // trên mức này là sai, 8S sạc đầy chỉ 33,6 V
#define PM_I_ABS_MAX  40.0f

struct PackMeasurement {
  bool  valid;
  float voltage;      // V, điện áp pack
  float cell_v;       // V, trung bình mỗi cell = voltage / 8
  float current;      // A, dương = đang sạc
  float charge_ah;    // Ah tích luỹ từ lần reset gần nhất (đếm coulomb)
  float die_temp;     // °C nhiệt độ chip — KHÔNG phải nhiệt độ cell
};

/* Giải mã thanh ghi — tách thành hàm THUẦN, không đụng I2C, để kiểm được
 * bằng giá trị thô đã biết trước TRƯỚC KHI có chip trong tay. Hai lỗi kinh
 * điển của INA228 nằm gọn trong hai hàm này:
 *   - quên dịch phải 4 (bỏ 4 bit dành riêng) => mọi số gấp 16 lần
 *   - quên mở rộng dấu 20/40 bit            => dòng xả thành số dương khổng lồ
 * Cả hai đều cho ra kết quả "trông như số đo", nên phải kiểm bằng test chứ
 * không thể phát hiện bằng mắt khi nhìn log. */
static inline int32_t pm_decode24(uint8_t b0, uint8_t b1, uint8_t b2, bool is_signed) {
  uint32_t v = ((uint32_t)b0 << 16) | ((uint32_t)b1 << 8) | b2;
  v >>= 4;                                            // bỏ 4 bit dành riêng
  if (is_signed && (v & 0x00080000)) v |= 0xFFF00000; // mở rộng dấu 20 bit
  return (int32_t)v;
}

static inline int64_t pm_decode40(const uint8_t* b) {
  uint64_t v = 0;
  for (int i = 0; i < 5; i++) v = (v << 8) | b[i];    // CHARGE: không có bit thừa
  if (v & 0x8000000000ULL) v |= 0xFFFFFF0000000000ULL;
  return (int64_t)v;
}

class PackMeter {
 public:
  /* Trả về false nếu không thấy chip hoặc DEVICE_ID sai. Đừng bỏ qua giá trị
     trả về: chạy tiếp với INA228 không tồn tại nghĩa là Lớp 2 ăn số 0. */
  bool begin(int sda, int scl);

  /* Đọc một lượt. INA228 chuyển đổi liên tục nên đây chỉ là giao dịch I2C,
     mất chưa tới 1 ms — không cần máy trạng thái như DS18B20. */
  PackMeasurement read();

  bool present() const { return present_; }

  /* Xoá thanh ghi tích luỹ. Gọi khi bắt đầu một chu kỳ sạc/xả mới, nếu không
     thì con số Ah là tổng từ lúc bật máy và vô nghĩa. */
  void resetCharge();

  uint32_t errorCount() const { return n_err_; }

 private:
  bool     w16(uint8_t reg, uint16_t val);
  bool     r24(uint8_t reg, int32_t& out, bool is_signed);
  bool     r16(uint8_t reg, uint16_t& out);
  bool     r40(uint8_t reg, int64_t& out);

  bool     present_ = false;
  float    current_lsb_ = 0;
  uint32_t n_err_ = 0;
};
