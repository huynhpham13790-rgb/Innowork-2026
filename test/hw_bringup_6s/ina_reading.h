/* Kiểu trả về của inaRead(), tách riêng ra file này KHÔNG phải vì gọn:
 * Arduino tự sinh nguyên mẫu hàm rồi chèn lên đầu .ino, trước chỗ khai báo
 * struct trong thân file — nên một hàm trả về struct khai báo trong .ino sẽ
 * không biên dịch được. Nằm trong header thì nguyên mẫu nhìn thấy kiểu. */
#pragma once

struct InaReading {
  bool  valid;
  float shunt_mv;
  float bus_v;
  float current_ma;     // tính từ điện áp shunt — không phụ thuộc thanh ghi CAL
  float current_reg_ma; // đọc từ thanh ghi CURRENT — để đối chiếu, xem CAL đúng chưa
  float power_w;
};
