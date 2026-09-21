/* =============================================================================
 *  Đo dòng và điện áp pack — chạy được cả INA226 lẫn INA228, tự nhận chip.
 *
 *  ⚠️ CẬP NHẬT 21/09/2026 (QĐ-037): INA228 đặt mua chưa về, đội lắp INA226 vào
 *  chạy trước. Module tự nhận chip lúc begin() nên cắm con nào cũng chạy, và
 *  khi INA228 về thì đổi vào KHÔNG phải sửa dòng nào. Phần dưới của khối chú
 *  thích này viết cho INA228 và vẫn đúng cho con đó; khác biệt của INA226 ghi
 *  ngay tại chỗ khai báo PM_R_SHUNT_226 và trong readIna226().
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
 *  NĂM QUYẾT ĐỊNH THIẾT KẾ:
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
 *     rồi sửa PM_R_SHUNT_226 / PM_R_SHUNT_228 — chưa làm thì mọi con số Lớp 2 chỉ đúng tới ~5 %.
 *
 *  5. LOẠI BỎ SỐ BÃO HOÀ (thêm 21/09, khi chạy INA226). Chân shunt của INA226
 *     chỉ chịu ±81,92 mV; với 0,1 Ω là ±0,819 A. Vượt qua thì thanh ghi dừng ở
 *     giá trị lớn nhất và trả về một con số TRÔNG HOÀN TOÀN HỢP LÝ. read() so
 *     với 99 % toàn thang và loại, đếm riêng bằng saturationCount() — vì "dòng
 *     vượt thang đo" và "hỏng dây" đòi hỏi hai hành động khác nhau.
 *
 * ========================================================================== */
#pragma once
#include <stdint.h>
#include <math.h>
#include "pack_config.h"

#define PM_I2C_ADDR   0x40      // A0=A1=GND. Module Adafruit mặc định 0x40.
#define PM_I_MAX      10.0f     // A — dòng lớn nhất trông đợi, đặt CURRENT_LSB
#define PM_N_CELLS    PACK_N_CELLS   // xem pack_config.h

/* --- HAI CHIP, MỘT GIAO DIỆN (QĐ-037) --------------------------------------
 * INA228 đặt mua chưa về, đội lắp INA226 vào chạy trước. Thay vì sửa qua sửa
 * lại mỗi lần đổi chip, module tự nhận chip lúc begin() và chạy đúng theo con
 * đang cắm. Cắm INA228 vào sau này thì không phải sửa dòng nào.
 *
 * Điện trở shunt là hằng số của MODULE, không phải của chip, nên mỗi loại một
 * giá trị. Đây là nguồn sai số tuyệt đối lớn nhất — mọi giá trị dòng tỉ lệ
 * thẳng với nó (xem quyết định (4) ở đầu file).
 *
 * ⚠️ KHÁC BIỆT QUAN TRỌNG NHẤT — TOÀN THANG DÒNG:
 *     INA228 + 0,015 Ω : ±163,84 mV / 0,015 = ±10,9 A
 *     INA226 + 0,1 Ω   : ± 81,92 mV / 0,1   = **±0,819 A**
 * INA226 đo thừa sức điện trở sưởi (0,62 A) nhưng KHÔNG đủ đo dòng sạc/xả của
 * pack thật. Vượt thang là thanh ghi bão hoà, và số bão hoà là một con số sai
 * trông hoàn toàn hợp lý — nên read() kiểm và loại nó, xem quyết định (5). */
#define PM_R_SHUNT_228  0.015f  // ohm — module INA228
#define PM_R_SHUNT_226  0.1f    // ohm — shunt R100 hàn sẵn trên module INA226

enum PmChip : uint8_t {
  PM_CHIP_NONE   = 0,
  PM_CHIP_INA226 = 1,
  PM_CHIP_INA228 = 2,
};

/* Khoảng hợp lý, suy ra từ số cell trong pack_config.h thay vì viết cứng —
   đổi cấu hình pack mà quên đổi ngưỡng là một chế độ hỏng âm thầm nữa. */
#define PM_V_MIN      PACK_V_MIN
#define PM_V_MAX      PACK_V_MAX
#define PM_I_ABS_MAX  40.0f

struct PackMeasurement {
  bool  valid;
  float voltage;      // V, điện áp pack
  float cell_v;       // V, trung bình mỗi cell = voltage / PM_N_CELLS
  float current;      // A, dương = đang sạc
  float charge_ah;    // Ah tích luỹ từ lần reset gần nhất (đếm coulomb)
  /* °C nhiệt độ CHIP — KHÔNG phải nhiệt độ cell.
     NAN khi đang chạy INA226: con đó không có cảm biến nhiệt trong chip. Bên
     gọi phải kiểm isnan() chứ module KHÔNG bịa ra một số trông hợp lý. */
  float die_temp;
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
  /* Tự nhận chip rồi cấu hình theo đúng con đang cắm. Trả về false nếu không
     thấy chip nào, hoặc thấy một chip lạ ở 0x40. Đừng bỏ qua giá trị trả về:
     chạy tiếp với chip không tồn tại nghĩa là Lớp 2 ăn số 0. */
  bool begin(int sda, int scl);

  /* Đọc một lượt. Cả hai chip đều chuyển đổi liên tục nên đây chỉ là giao dịch
     I2C, mất chưa tới 1 ms — không cần máy trạng thái như DS18B20. */
  PackMeasurement read();

  bool present() const { return present_; }

  PmChip      chip() const { return chip_; }
  const char* chipName() const;
  /* Toàn thang dòng đo được, theo chip và shunt đang dùng. Đẩy lên dashboard
     để người trực biết con số 0,8 A là giới hạn dụng cụ chứ không phải pin. */
  float       currentFullScale() const;

  /* Xoá bộ tích luỹ điện lượng. Gọi khi bắt đầu một chu kỳ sạc/xả mới, nếu
     không thì con số Ah là tổng từ lúc bật máy và vô nghĩa. */
  void resetCharge();

  uint32_t errorCount() const { return n_err_; }
  /* Số lần đọc bị loại vì shunt bão hoà — tách riêng khỏi errorCount() vì đây
     KHÔNG phải lỗi dây hay lỗi chip, mà là "dòng vượt quá thang đo". Hai thứ
     đòi hỏi hành động khác nhau: một cái đi kiểm dây, một cái đi đổi shunt. */
  uint32_t saturationCount() const { return n_sat_; }

 private:
  bool     w16(uint8_t reg, uint16_t val);
  bool     r24(uint8_t reg, int32_t& out, bool is_signed);
  bool     r16(uint8_t reg, uint16_t& out);
  bool     r40(uint8_t reg, int64_t& out);

  bool     beginIna228();
  bool     beginIna226();
  PackMeasurement readIna228();
  PackMeasurement readIna226();

  bool     present_ = false;
  PmChip   chip_ = PM_CHIP_NONE;
  float    r_shunt_ = 0;        // ohm, theo module của chip đang cắm
  float    current_lsb_ = 0;
  uint32_t n_err_ = 0;
  uint32_t n_sat_ = 0;

  /* Đếm coulomb bằng phần mềm — chỉ dùng cho INA226.
     INA228 có thanh ghi CHARGE tích phân ngay trong chip (QĐ-026); INA226 thì
     không, nên phải tự cộng dồn I·Δt. Khác biệt cần biết: bản phần mềm chỉ
     tích phân những lúc read() được gọi, nên dòng biến thiên giữa hai lần gọi
     là mất. Ở nhịp 1 Hz với dòng sạc CC-CV (đổi chậm) thì chấp nhận được; nếu
     chuyển sang nhịp thưa hơn thì con số Ah bắt đầu sai. */
  double   q_as_ = 0;           // A·s tích luỹ
  uint32_t t_q_ms_ = 0;         // mốc thời gian lần tích phân trước
  bool     q_started_ = false;
};
