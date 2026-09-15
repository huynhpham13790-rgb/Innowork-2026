/* MẪU — copy thành arduino_secrets.h rồi điền giá trị thật.
   File thật KHÔNG được commit, .gitignore đã chặn sẵn. */
#pragma once

#define SECRET_MQTT_USER "hutieu"
#define SECRET_MQTT_PASS "dien-mat-khau-mqtt-vao-day"

/* Danh sách WiFi, thử theo thứ tự ưu tiên từ trên xuống.
   Mạng mở thì để mật khẩu là chuỗi rỗng "". */
#define SECRET_WIFI_LIST                 \
  { "ten-wifi-uu-tien-1", "mat-khau-1" }, \
  { "ten-wifi-uu-tien-2", "" },
