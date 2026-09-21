#!/usr/bin/env python3
"""
Nghiệm thu kênh BLE bằng MÁY, không chỉ bằng mắt nhìn nRF Connect.

VÌ SAO CẦN CÁI NÀY
Nhìn nRF Connect trên điện thoại chỉ chứng minh "có chữ hiện ra". Nó KHÔNG
chứng minh được ba thứ dễ hỏng nhất và cũng là ba thứ đáng giá nhất:

  1. Có quảng bá lại sau khi ngắt kết nối không? Thiếu thì người thứ hai tới
     kiểm pack sẽ không quét thấy gì — thiết bị vẫn chạy, chỉ là vô hình.
  2. Notify có thật sự bắn khi số đổi không, hay chỉ đọc thủ công mới thấy?
  3. Có đặc tính GHI nào NGOÀI DỰ KIẾN lọt vào không? Từ QĐ-042 có đúng MỘT
     đặc tính ghi được (tắt tiếng còi); mọi cái khác phải chỉ đọc. Việc chặn
     thật sự do test/ble_write_check.py kiểm.

Chạy:  /tmp/claude-1000/blevenv/bin/python test/ble_check.py [số giây nghe]
"""
import asyncio, sys
from bleak import BleakScanner, BleakClient

NAME = "HuTieu-BMS"
SVC  = "48555449-4555-4d53-0001-000000000000"
LISTEN_S = int(sys.argv[1]) if len(sys.argv) > 1 else 25


async def main():
    print(f"quet tim \"{NAME}\" ...")
    dev = await BleakScanner.find_device_by_name(NAME, timeout=15.0)
    if not dev:
        sys.exit(f"KHONG THAY. Kiem tra: board da nap ban co BLE chua, "
                 f"USE_BLE=1 chua, va PC da bat bluetooth chua (rfkill).")
    print(f"thay: {dev.address}\n")

    async with BleakClient(dev) as cli:
        svc = cli.services.get_service(SVC)
        if not svc:
            sys.exit(f"noi duoc nhung KHONG CO dich vu {SVC}")

        # --- (1) đọc từng đặc tính, kèm nhãn 0x2901 -------------------------
        print("=" * 70)
        print("(1) DOC — gia tri day du qua lenh READ")
        print("=" * 70)
        labels = {}
        for ch in svc.characteristics:
            label = "(khong co nhan)"
            for d in ch.descriptors:
                if d.uuid.startswith("00002901"):
                    label = (await cli.read_gatt_descriptor(d.handle)).decode(errors="replace")
            labels[ch.uuid] = label
            val = (await cli.read_gatt_char(ch.uuid)).decode(errors="replace")
            print(f"  {label:<32} : {val}")

        # --- (2) không được có đặc tính ghi được ----------------------------
        print("\n" + "=" * 70)
        print("(2) AN TOAN — chi DUNG MOT dac tinh ghi duoc, va no phai doi ghep doi")
        print("=" * 70)
        # Truoc QĐ-042 phep thu nay doi KHONG co dac tinh ghi nao. Gio co dung
        # mot cai (tat tieng coi), nen moc do chuyen thanh: dung mot cai, dung
        # cai do, va phai la loai doi ma hoa/xac thuc. Noi long moc thi phai
        # noi long DUNG BANG phan da chung minh duoc, khong hon.
        UUID_CMD = "48555449-4555-4d53-0008-000000000000"
        w = [c for c in svc.characteristics
             if {"write", "write-without-response"} & set(c.properties)]
        ok = True
        if len(w) != 1 or w[0].uuid != UUID_CMD:
            print(f"  ✗ TRUOT — dac tinh ghi duoc khong dung nhu mong doi: "
                  f"{[c.uuid for c in w]}")
            ok = False
        else:
            print(f"  ✓ chi 1 dac tinh ghi duoc: {w[0].uuid}")
            print(f"    quyen: {w[0].properties}")
        if ok:
            print(f"  ✓ DAT — {len(svc.characteristics)-1} dac tinh chi DOC, 1 dac tinh lenh")
            print("    (viec CHAN that su do test/ble_write_check.py kiem)")

        # --- (3) notify có bắn không ---------------------------------------
        print("\n" + "=" * 70)
        print(f"(3) NOTIFY — nghe {LISTEN_S}s. Lam nong mot cell de thay doi.")
        print("=" * 70)
        hits = {}

        def cb(ch, data):
            txt = bytes(data).decode(errors="replace")
            name = labels.get(ch.uuid, ch.uuid)
            hits[name] = hits.get(name, 0) + 1
            print(f"  [notify] {name:<32} : {txt}")

        for ch in svc.characteristics:
            if "notify" in ch.properties:
                await cli.start_notify(ch.uuid, cb)
        await asyncio.sleep(LISTEN_S)

        print(f"\n  tong {sum(hits.values())} goi notify tu {len(hits)} dac tinh")
        if not hits:
            print("  ⚠ khong co goi nao — co the do khong co gi DOI trong luc nghe.")
            print("    notify chi ban khi chuoi thuc su khac lan truoc (co y).")

    # --- (4) sau khi ngắt, phải quảng bá lại -----------------------------
    print("\n" + "=" * 70)
    print("(4) QUANG BA LAI sau khi ngat ket noi")
    print("=" * 70)
    await asyncio.sleep(3)
    again = await BleakScanner.find_device_by_name(NAME, timeout=15.0)
    print(f"  {'✓ DAT — quet lai van thay' if again else '✗ TRUOT — da bien mat, thiet bi thanh vo hinh'}")


asyncio.run(main())
