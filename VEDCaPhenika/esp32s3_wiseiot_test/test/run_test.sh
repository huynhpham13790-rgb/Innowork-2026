#!/usr/bin/env bash
# Test logic store-and-forward trên PC, không cần board ESP32.
#
# Cách làm: cắt đúng đoạn code spool từ file .ino thật ra một file .inc rồi
# biên dịch cùng test. Không chép tay -> .ino sửa hỏng là test đỏ ngay.
#
#   ./test/run_test.sh
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
INO="$HERE/../esp32s3_wiseiot_test.ino"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

# --- cắt phần hằng số spool + các hàm spool* từ .ino -------------------------
{
  sed -n '/^const char\* SPOOL_PATH/,/^const long   *RECONNECT_MS/p' "$INO"
  echo
  echo "bool fsReady = false;"
  echo "unsigned long spoolDropped = 0;"
  echo
  sed -n '/^void spoolBegin()/,/^\/\/ ============================================================ WiFi/p' "$INO" \
    | sed '$d'
} > "$WORK/extracted_spool.inc"

LINES=$(wc -l < "$WORK/extracted_spool.inc")
if [ "$LINES" -lt 40 ]; then
  echo "LOI: chi cat duoc $LINES dong tu .ino - cau truc file da doi, sua lai sed trong script nay." >&2
  exit 1
fi
echo "Da cat $LINES dong code spool that tu .ino"

cp "$HERE/test_spool.cpp" "$WORK/"
g++ -std=c++17 -O0 -w -I"$WORK" -o "$WORK/test_spool" "$WORK/test_spool.cpp"
"$WORK/test_spool" "$WORK"
