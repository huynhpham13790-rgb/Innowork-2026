#!/usr/bin/env bash
# Dựng stack Plan B, TỰ dò IP LAN hiện tại.
#
# VÌ SAO CÓ SCRIPT NÀY: LAN_IP trong .env phải khớp IP thật của máy, nếu không
# container mosquitto chết ngay với "cannot assign requested address". Máy này
# đổi mạng liên tục (trường / nhà / hotspot điện thoại) nên sửa tay là việc sẽ
# quên, và quên đúng hôm thi thì mất màn demo.
#
# Khi nào KHÔNG cần script này nữa: sau khi tắt mosquitto của hệ thống
#   sudo systemctl disable --now mosquitto
# thì xoá docker-compose.override.yml đi, compose gốc bind 0.0.0.0:1883 là
# không phụ thuộc IP nữa. Đó mới là lời giải sạch; script này là đường vòng.
set -euo pipefail
cd "$(dirname "$0")"

IP=$(ip -4 route get 1.1.1.1 2>/dev/null | grep -oP 'src \K[\d.]+' || true)
if [ -z "$IP" ]; then
  echo "khong do duoc IP LAN - may co dang noi mang khong?" >&2
  exit 1
fi

if grep -q '^LAN_IP=' .env 2>/dev/null; then
  OLD=$(grep '^LAN_IP=' .env | cut -d= -f2)
  [ "$OLD" != "$IP" ] && echo "LAN_IP: $OLD -> $IP"
  sed -i "s/^LAN_IP=.*/LAN_IP=$IP/" .env
else
  echo "LAN_IP=$IP" >> .env
fi

echo "dung stack voi LAN_IP=$IP"
docker compose up -d
echo
echo "broker MQTT: $IP:1883"
ss -ltn 2>/dev/null | grep -q "$IP:1883" && echo "  -> dang nghe, OK" \
  || echo "  -> CHUA nghe, xem: docker compose logs mosquitto"
