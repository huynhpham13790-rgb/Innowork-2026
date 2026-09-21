#!/usr/bin/env python3
"""
Nguoi LA co tat duoc coi qua BLE khong? (QĐ-042)

VI SAO CAN: mo mot dac tinh GHI tren BLE la mo mot duong dieu khien vao mot
pack pin lithium cho bat ky ai trong ban kinh 10 m. Quyet dinh mo no chi chap
nhan duoc neu chung minh duoc rang may CHUA GHEP DOI thi KHONG ghi duoc.

Phep thu: noi (khong ghep doi) roi ghi "mute on".
  DAT   = bi tu choi (loi Insufficient Authentication/Encryption)
  TRUOT = ghi duoc -> bat ky ai cung tat duoc coi bao chay

Chay: /tmp/claude-1000/blevenv/bin/python test/ble_write_check.py
"""
import asyncio, sys
from bleak import BleakScanner, BleakClient

CMD = "48555449-4555-4d53-0008-000000000000"
ST  = "48555449-4555-4d53-0002-000000000000"


async def main():
    d = await BleakScanner.find_device_by_name("HuTieu-BMS", timeout=20.0)
    if not d:
        sys.exit("khong thay thiet bi")
    async with BleakClient(d) as c:
        before = (await c.read_gatt_char(ST)).decode(errors="replace")
        print("truoc khi ghi :", before)
        # Han gio 20s: khi thiet bi doi GHEP DOI ma khong ai nhap ma PIN thi
        # BlueZ ngoi cho vo han. Treo O DAY cung la DAT — nghia la ghi khong di
        # qua duoc. Ban dau cua phep thu nay khong co han gio nen no treo 10
        # phut va khong ket luan duoc gi.
        try:
            await asyncio.wait_for(
                c.write_gatt_char(CMD, b"mute on", response=True), timeout=20)
        except asyncio.TimeoutError:
            print("\n✓ DAT — ghi bi CHAN (thiet bi doi ghep doi, khong ai nhap PIN)")
            return 0
        except Exception as e:
            print(f"\n✓ DAT — ghi bi TU CHOI: {type(e).__name__}: {e}")
            return 0
        await asyncio.sleep(2)
        after = (await c.read_gatt_char(ST)).decode(errors="replace")
        print("sau khi ghi   :", after)
        if "TAT TIENG" in after and "TAT TIENG" not in before:
            print("\n✗ TRUOT — may CHUA GHEP DOI van tat duoc coi")
            return 1
        print("\n? ghi khong loi nhung trang thai khong doi — xem lai bang tay")
        return 1

sys.exit(asyncio.run(main()))
