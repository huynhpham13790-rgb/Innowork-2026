#!/usr/bin/env python3
"""
Nút tắt tiếng có bị cổng USB tự bấm hộ không?

VÌ SAO CÓ FILE NÀY
AL_PIN_MUTE = GPIO0, mà GPIO0 cũng là chân DTR của mạch nạp trên board. Mở
cổng serial với DTR bật sẽ GHÌ GPIO0 XUỐNG LIÊN TỤC suốt phiên (đo 21/09 —
không phải một xung ngắn), và mở cổng còn làm board RESET. Hậu quả nếu firmware
bắt nhầm: CÒI TỰ TẮT TIẾNG mỗi khi ai đó mở rồi đóng Serial Monitor — đèn vẫn
đỏ, mức vẫn leo, chỉ tiếng là không kêu, không dấu hiệu nào ra ngoài.

VÌ SAO KHÔNG ĐO BẰNG CHÍNH SERIAL
Bản đầu của phép thử này đọc dòng [ALRM] trên serial và báo "đạt" — SAI. Cú
đảo xảy ra đúng lúc ĐÓNG cổng, tức sau khi đã thôi đọc. Phải quan sát bằng một
kênh không đụng vào GPIO0, nên ở đây đọc trạng thái qua BLE.

Mốc đúng: sau một phiên serial, thiết bị phải KHÔNG ở trạng thái tắt tiếng
(vì mở cổng làm board reset, mà mặc định sau reset là còi bật).

Chạy:  /tmp/claude-1000/blevenv/bin/python test/dtr_mute_check.py [so lan]
"""
import asyncio, serial, sys, time
from bleak import BleakScanner, BleakClient

ST = "48555449-4555-4d53-0002-000000000000"     # đặc tính "Trang thai"
N  = int(sys.argv[1]) if len(sys.argv) > 1 else 3


async def state():
    d = await BleakScanner.find_device_by_name("HuTieu-BMS", timeout=20.0)
    if not d:
        return None
    async with BleakClient(d) as c:
        return (await c.read_gatt_char(ST)).decode(errors="replace")


async def main():
    bad = 0
    for i in range(1, N + 1):
        p = serial.Serial('/dev/ttyACM0', 115200, timeout=0.5)
        p.dtr, p.rts = True, False        # CỐ Ý: đây là kịch bản xấu nhất
        time.sleep(6)
        p.close()
        time.sleep(9)                     # chờ boot lại + BLE quảng bá lại

        s = await state()
        if s is None:
            print(f"  lan {i}: ? khong quet thay thiet bi"); bad += 1
        elif "TAT TIENG" in s:
            print(f"  lan {i}: ✗ bi tat tieng — {s}"); bad += 1
        else:
            print(f"  lan {i}: ✓ con bat tieng — {s}")

    print(f"\n{N - bad}/{N} dat.", "DAT" if bad == 0 else f"TRUOT — {bad} lan")
    sys.exit(1 if bad else 0)


asyncio.run(main())
