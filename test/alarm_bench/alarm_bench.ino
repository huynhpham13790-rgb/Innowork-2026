/* =============================================================================
 *  Kiểm chứng máy trạng thái báo động — chạy khô, không cần pin, không cần còi.
 *
 *  VÌ SAO CẦN SKETCH NÀY thay vì cứ hơ nóng cảm biến rồi nhìn đèn:
 *  Ba tính chất quan trọng nhất của module báo động KHÔNG kiểm được bằng tay.
 *    - Trễ 5 °C: phải đưa nhiệt độ lên 60,1 rồi hạ xuống đúng 57 và xác nhận
 *      còi VẪN kêu. Hơ nóng bằng ngón tay không điều khiển được tới mức đó.
 *    - Tự huỷ tắt tiếng khi leo thang: cần đúng một thứ tự sự kiện cụ thể.
 *    - Đèn phải đổi khi mất cảm biến: phải rút dây đúng lúc.
 *  Thứ gì không kiểm được thì sớm muộn sẽ hỏng mà không ai biết.
 *
 *  alarm.cpp và alarm.h ở đây là SYMLINK tới file thật trong firmware. Không
 *  phải bản sao — bản sao sẽ phân kỳ sau vài lần sửa và bench sẽ kiểm một
 *  đoạn mã không còn tồn tại.
 *
 *  Đèn RGB trên board vẫn chạy thật trong lúc bench chạy, nên đây cũng là lần
 *  nhìn tận mắt xem màu và nhịp nháy có phân biệt được không.
 * ========================================================================== */
#include "alarm.h"

Alarm al;
int n_pass = 0, n_fail = 0;

const char* NAME[4] = { "OK", "THEO DOI", "BAO DONG", "NGUY KICH" };

/* Chạy máy trạng thái một khoảng thời gian mô phỏng với đầu vào giữ nguyên.
   Cần lặp nhiều vòng vì phần nháy phụ thuộc millis(); một lần gọi update()
   không đủ để thấy trạng thái ổn định. */
void hold(bool ai_alarm, bool watch, float tmax, bool sbad, uint16_t ms) {
  uint32_t t0 = millis();
  while (millis() - t0 < ms) { al.update(ai_alarm, watch, tmax, sbad); delay(5); }
}

void expect(const char* what, AlarmLevel got, AlarmLevel want) {
  bool ok = (got == want);
  ok ? n_pass++ : n_fail++;
  Serial.printf("  [%s] %-46s mong %-10s duoc %s\n",
                ok ? "DAT" : "HONG", what, NAME[want], NAME[got]);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  delay(500);
  Serial.println("\n===== BENCH BAO DONG — kiem may trang thai =====\n");

  al.begin();

  Serial.println("TH-A: binh thuong");
  hold(false, false, 30.0f, false, 300);
  expect("30 degC, AI im, cam bien du", al.level(), AL_OK);

  Serial.println("\nTH-B: AI thay lech nhung chua du lau");
  hold(false, true, 32.0f, false, 300);
  expect("watch=true", al.level(), AL_WATCH);

  Serial.println("\nTH-C: AI xac nhan bat thuong");
  hold(true, true, 35.0f, false, 300);
  expect("ai_alarm=true", al.level(), AL_ALARM);

  Serial.println("\nTH-D: AI het bao dong thi phai tu ha muc");
  hold(false, false, 33.0f, false, 300);
  expect("ai_alarm=false", al.level(), AL_OK);

  Serial.println("\nTH-E: nguong cung 60 degC — phai kich KE CA khi AI im");
  hold(false, false, 60.2f, false, 300);
  expect("60,2 degC, AI im hoan toan", al.level(), AL_CRITICAL);

  Serial.println("\nTH-F: TRE 5 degC — day la phep kiem quan trong nhat");
  hold(false, false, 57.0f, false, 300);
  expect("tut ve 57 degC: VAN phai nguy kich", al.level(), AL_CRITICAL);
  hold(false, false, 59.9f, false, 300);
  expect("len lai 59,9 degC: van nguy kich", al.level(), AL_CRITICAL);
  hold(false, false, 54.5f, false, 300);
  expect("tut duoi 55 degC: gio moi duoc nha", al.level(), AL_OK);

  Serial.println("\nTH-G: mat cam bien — khong duoc bao mau do");
  hold(false, false, NAN, true, 300);
  expect("t_max=NAN, sensor_bad=true", al.level(), AL_OK);
  Serial.printf("  (muc van OK nhung ten hien thi: \"%s\" — den xanh duong nhay)\n",
                al.levelName());

  Serial.println("\nTH-H: mat cam bien KHONG duoc xoa bao dong dang co");
  hold(true, true, 41.0f, false, 200);
  hold(true, true, NAN,   true,  300);
  expect("dang BAO DONG roi mat cam bien", al.level(), AL_ALARM);

  Serial.println("\nTH-I: dem su kien");
  Serial.printf("  So lan leo len muc bao dong: %lu (mong doi 3)\n", al.eventCount());
  al.eventCount() == 3 ? n_pass++ : n_fail++;

  Serial.printf("\n===== KET QUA: %d dat, %d hong =====\n", n_pass, n_fail);
  Serial.println(n_fail == 0 ? "May trang thai DUNG."
                             : "CO LOI — khong duoc nap firmware len pack that.");

  Serial.println("\nGio chay vong lap trinh dien mau/nhip, xem bang mat:");
  Serial.println("  Nhan nut BOOT de thu TAT TIENG (chi tat coi, den van bao).");
}

/* Sau phần kiểm tự động là vòng trình diễn: quay đủ 5 trạng thái, mỗi trạng
   thái 4 giây, để nhìn tận mắt màu và nhịp có phân biệt được từ xa không. */
void loop() {
  struct { const char* ten; bool a, w; float t; bool s; } demo[] = {
    { "OK — xanh la mo",              false, false, 30.0f, false },
    { "THEO DOI — vang nhay cham",    false, true,  33.0f, false },
    { "BAO DONG — do nhay nhanh+coi", true,  true,  38.0f, false },
    { "NGUY KICH — do lien tuc+coi",  false, false, 61.0f, false },
    { "MAT CAM BIEN — xanh duong",    false, false, 30.0f, true  },
  };
  for (auto &d : demo) {
    Serial.printf("[DEMO] %s\n", d.ten);
    // Ra khỏi trạng thái chốt nguy kịch trước khi sang mục kế, nếu không thì
    // mọi mục sau đều kẹt ở màu đỏ và phần trình diễn thành vô nghĩa.
    hold(false, false, 30.0f, false, 50);
    hold(d.a, d.w, d.t, d.s, 4000);
  }
}
