/* =============================================================================
 *  Kiểm phép giải mã thanh ghi INA228 — chạy khô, KHÔNG CẦN CHIP.
 *
 *  VÌ SAO LÀM TRƯỚC KHI LINH KIỆN VỀ: hai lỗi kinh điển của INA228 (quên dịch
 *  phải 4 bit, quên mở rộng dấu) đều cho ra con số TRÔNG NHƯ SỐ ĐO. Nhìn log
 *  không thể thấy. Nếu đợi tới lúc có chip mới phát hiện, ta sẽ mất hàng giờ
 *  nghi ngờ dây nối, nguồn, và điện trở shunt trước khi nghĩ tới phần mềm.
 *
 *  Giá trị thô ở đây tự tính tay từ datasheet, không lấy từ chip — nên đây là
 *  phép kiểm độc lập thật sự, không phải ghi lại hành vi hiện có rồi gọi là
 *  test.
 * ========================================================================== */
#include "pack_meter.h"

int pass = 0, fail = 0;

void chk(const char* what, double got, double want, double tol) {
  bool ok = fabs(got - want) <= tol;
  ok ? pass++ : fail++;
  Serial.printf("  [%s] %-44s mong %12.5f  duoc %12.5f\n",
                ok ? "DAT" : "HONG", what, want, got);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  delay(500);
  Serial.println("\n===== BENCH INA228 — kiem giai ma thanh ghi (khong can chip) =====\n");

  const float LSB_VBUS = 195.3125e-6f;
  const float CURRENT_LSB = 10.0f / 524288.0f;    // PM_I_MAX / 2^19 = 19,073 uA

  Serial.println("VBUS — 24 bit khong dau, 4 bit thap la bit thua");
  // Pack 8S sac day 33,6 V => 33,6 / 195,3125e-6 = 172032 buoc
  // 172032 = 0x02A000; dich trai 4 => 0x02A0000 -> ba byte 0x2A 0x00 0x00
  chk("33,6 V (0x2A0000)", pm_decode24(0x2A,0x00,0x00,false)*LSB_VBUS, 33.6, 1e-4);
  chk("0 V", pm_decode24(0,0,0,false)*LSB_VBUS, 0.0, 1e-9);
  // BAY: quen dich phai 4 thi so nay gap 16 lan -> 537,6 V, van "trong nhu so do"
  chk("bit thua bi bo (0x2A000F ~ 0x2A0000)",
      pm_decode24(0x2A,0x00,0x0F,false)*LSB_VBUS, 33.6, 1e-4);

  Serial.println("\nCURRENT — 24 bit CO DAU, mo rong dau tu 20 bit");
  // +1,9 A (dong sac cua doi) => 1,9 / 19,073e-6 = 99614 buoc = 0x1851E
  chk("+1,9 A sac", pm_decode24(0x18,0x51,0xE0,true)*CURRENT_LSB, 1.9, 1e-3);
  // -1,9 A xa => bu hai 20 bit: 0x100000 - 0x1851E = 0xE7AE2
  chk("-1,9 A xa (BAY mo rong dau)",
      pm_decode24(0xE7,0xAE,0x20,true)*CURRENT_LSB, -1.9, 1e-3);
  chk("-1 buoc (0xFFFFFx)", (double)pm_decode24(0xFF,0xFF,0xF0,true), -1.0, 0);
  chk("gia tri am lon nhat (0x800000)",
      (double)pm_decode24(0x80,0x00,0x00,true), -524288.0, 0);
  chk("gia tri duong lon nhat (0x7FFFFx)",
      (double)pm_decode24(0x7F,0xFF,0xF0,true), 524287.0, 0);

  Serial.println("\nCHARGE — 40 bit co dau, KHONG co bit thua");
  uint8_t q1[5] = {0x00,0x00,0x00,0x00,0x01};
  chk("1 buoc", (double)pm_decode40(q1), 1.0, 0);
  uint8_t qn[5] = {0xFF,0xFF,0xFF,0xFF,0xFF};
  chk("-1 buoc (mo rong dau 40 bit)", (double)pm_decode40(qn), -1.0, 0);
  // 2,55 Ah (dung luong pack cua doi) = 9180 A.s / 19,073e-6 = 481.296.384 buoc
  uint8_t q2[5] = {0x00,0x1C,0xB4,0x00,0x00};
  double ah = (double)pm_decode40(q2) * CURRENT_LSB / 3600.0;
  chk("~2,55 Ah dem coulomb", ah, 2.5480, 0.01);

  Serial.println("\nNHIET DO CHIP — 16 bit co dau, 7,8125 m degC moi buoc");
  chk("+25 degC", (int16_t)3200 * 7.8125e-3, 25.0, 1e-6);
  chk("-10 degC", (int16_t)(-1280) * 7.8125e-3, -10.0, 1e-6);

  Serial.println("\nSHUNT_CAL — cau hinh phai nam trong 16 bit");
  double cal = 13107.2e6 * CURRENT_LSB * 0.015;
  chk("13107,2e6 x LSB x 0,015 ohm", cal, 3750.0, 1.0);
  Serial.printf("  (vua trong 65535 => cau hinh hop le)\n");

  Serial.printf("\n===== KET QUA: %d dat, %d hong =====\n", pass, fail);
  Serial.println(fail == 0 ? "Phep giai ma DUNG. Cam chip vao la chay."
                           : "CO LOI trong phep giai ma - sua truoc khi cam chip.");
}

void loop() {}
