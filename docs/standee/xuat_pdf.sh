#!/usr/bin/env bash
# Xuất standee ra PDF khổ thật (806 × 1806 mm, đã gồm bleed 3 mm) + ảnh xem trước.
# Cần mạng lần đầu để tải font Be Vietnam Pro. Gửi nhà in file .pdf.
set -euo pipefail
cd "$(dirname "$0")"
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
google-chrome --headless=new --no-sandbox --disable-gpu --user-data-dir="$TMP" \
  --no-pdf-header-footer --virtual-time-budget=8000 \
  --print-to-pdf=standee_80x180.pdf "file://$PWD/standee_80x180.html"
# Ảnh xem trước: 1 mm = 1 px CSS gốc ≈ 3,78 px; thu nhỏ 1/4 cho nhẹ.
google-chrome --headless=new --no-sandbox --disable-gpu --user-data-dir="$TMP" \
  --hide-scrollbars --force-device-scale-factor=0.25 --window-size=3047,6826 \
  --virtual-time-budget=8000 --screenshot=xem_truoc.png "file://$PWD/standee_80x180.html"
ls -la standee_80x180.pdf xem_truoc.png
