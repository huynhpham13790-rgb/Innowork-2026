#!/usr/bin/env python3
"""Dien ho so vong ban ket theo dung template cua BTC (giu nguyen khung bang).

Sinh ra HAI file:
  - HO_SO_BAN_KET_HuTieu.docx     -> ban NOP, tieng Anh hoan toan
  - HO_SO_BAN_KET_HuTieu_VI.docx  -> ban doc noi bo, tieng Viet, noi dung y het

Can 3 anh so do (xuat tu SLIDE_BAN_KET_HuTieu_EN.pdf trang 5, 12, 13):
    pdftoppm -r 190 -f 5  -l 5  -png slide.pdf /tmp/diag_arch
    pdftoppm -r 190 -f 12 -l 12 -png slide.pdf /tmp/diag_now
    pdftoppm -r 190 -f 13 -l 13 -png slide.pdf /tmp/diag_wise
"""
import shutil, os, glob
from docx import Document
from docx.shared import Pt, Inches, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH

DOCS = "/home/huynh/code/Innowork 2026/docs"
SRC = os.path.join(DOCS, "MẪU HỒ SƠ VÒNG BÁN KẾT.docx")
FIGDIR = "/tmp/claude-1000"
TODO = RGBColor(0xC0, 0x00, 0x00)

B = True   # in dam
T = "todo"  # cho phai tu dien -> in do nghieng


# ===================== NOI DUNG =====================
EN = dict(
    university="Thai Nguyen University of Information and Communication Technology (ICTU)",
    department="Faculty of Information Technology",
    prof="Dao Thi Hang",
    email_todo="",
    email_extra="Team contact: huynhpham13790@gmail.com · (+84) 359 260 290 — Pham Van Huynh, team lead",
    duration=[
        "l   Skills development: 2026/08/15 to 2026/09/10  (WISE-IoT Academy certificate, "
        "MQTT / WISE-PaaS protocol, TinyML)",
        "l   Application development: 2026/09/01 to 2026/11/20  (hardware, Edge AI, cloud "
        "integration, field validation)",
        "l   Semi-final: 2026/09/26   ·   Final Demo Day: 2026/11/27",
    ],
    students=["1. Number of teams: 01 (Team Hu Tieu)", "2. Number of team members: 05 students"],
    slots="05 slots (one per team member)",
    space="2026/09/01 ~ 2026/12/31",
    team="Hu Tieu",
    nmem="05",
    topic="Edge-AI Thermal Safety Monitor for Electric-Vehicle Battery Packs — per-cell anomaly "
          "detection on-device + remaining-useful-life prediction in the cloud",
    adviser="Dao Thi Hang",
    members=["All five members: Faculty of Information Technology, ICTU.",
             "Pham Van Huynh — team lead: planning, business model, presentation",
             "Pham Minh Tu — AI/ML: Layer 1 anomaly model, training and evaluation",
             "Dam Duc Don — AI/ML: Layer 2 battery-health model, datasets",
             "Hoang Van Huy — embedded: ESP32-S3 firmware, sensors, local alarm",
             "Nguyen Hoang Anh Tuan — cloud: MQTT/WISE-PaaS integration, dashboard, demo video"],
    intro=[
        ("Target audience", B),
        "Primary (B2B): electric two-wheeler fleet operators and battery-swap networks — delivery "
        "fleets, ride-hailing fleets, rental operators. In a swap model the operator owns the packs, "
        "so the party that loses money on a dead or burnt pack is the same party that can buy the "
        "device. Secondary: EV service garages and used-vehicle appraisers (handheld battery health "
        "report). Individual riders are the emotional story, not the business model.",
        "",
        ("The problem", B),
        "A lithium battery pack does not fail all at once: one weak cell heats up first. Every BMS "
        "already has a hard 60 °C cut-off, but by the time any absolute limit is crossed, thermal "
        "runaway is self-sustaining and cannot be stopped. About 31 % of EV fires start while the "
        "vehicle is parked and about 25 % happen in underground car parks — exactly when a system "
        "that only wakes up while riding or charging would be asleep. Replacing an e-motorbike pack "
        "in Vietnam costs 1.5–12 million VND, and a fire in a fleet warehouse can destroy the whole "
        "warehouse.",
        "",
        ("What the project does", B),
        "A low-cost module measures every cell individually (6 × DS18B20 probes + an INA228 "
        "current/voltage sensor on a 6-cell 18650 test pack) and runs an autoencoder directly on an "
        "ESP32-S3, once per second. The model does not look at absolute temperature; it looks at the relation "
        "between one cell and the rest of the pack — how far it deviates, its rank inside the pack, "
        "and whether it is heating faster than its neighbours. A fixed threshold can never see "
        "\"cell #3 is 4 °C hotter than the others at only 35 °C\"; this model can. A separate hard "
        "60 °C rule runs in parallel, outside the AI, so that a wrong model can never silence the "
        "last line of defence.",
        "",
        ("Expected benefits", B),
        "· Safety: the cell that is behaving differently is identified early, and the local alarm "
        "still works when the network is completely down.",
        "· Maintenance: reporting battery health after each charge turns pack replacement from an "
        "emergency into a scheduled maintenance item, which also extends the useful life of the fleet.",
        "· Low running cost: the device is built from inexpensive, widely available components, and "
        "in a swap model the packs return to a station every day, so the station Wi-Fi can serve as "
        "the uplink instead of a mobile data plan.",
        "",
        ("What we have tested so far", B),
        "We have not tested on an electric-vehicle pack in service. Our work is research on a "
        "laboratory pack built from cells of the same type, together with public datasets recorded "
        "on real packs. So far we have verified that:",
        "· the sensing chain and the data path work end to end on the bench — readings reach the "
        "dashboard live and are buffered while the device is offline;",
        "· the four-level alarm logic behaves correctly in a full bench test, including when the AI "
        "output is disabled entirely;",
        "· on public datasets, an injected single-cell fault is detected reliably, while normal "
        "operation produces essentially no false alarms over long runs;",
        "· on an independent real pack that the model had never seen, the faulty runs separate "
        "clearly from the healthy ones.",
        "Warming a cell on our own pack, and then measuring a pack on a vehicle in daily use, are the "
        "next two steps. We also state a known limit openly: because Layer 1 compares cells with each "
        "other, a pack that heats up uniformly produces no deviation — that case is handled by the "
        "hard 60 °C threshold, which we have not yet been able to test on hardware.",
    ],
    diagram=[
        ("System / functional diagram — see the three figures below.", B),
        "",
        ("How Edge AI / Edge Computing is used", B),
        "Layer 1 — Edge AI, on the device. A small autoencoder runs on the ESP32-S3 itself, once per "
        "second, and is small enough to sit in a fraction of the microcontroller's flash. Its inputs "
        "are purely relative: how far each cell sits from the rest of the pack, its rank among the "
        "cells, whether it is heating faster than its neighbours, and whether the deviation is a slow "
        "drift or a sudden jump. It raises an alarm only when the signal persists, so sensor noise "
        "alone cannot trigger it. A fixed 60 °C threshold runs in parallel, outside the AI, as an "
        "independent last line of defence.",
        "Why the edge and not the cloud: sensing, inference, the local buzzer/LED alarm and logging "
        "all work with no network at all. The network is needed to tell someone far away — not to "
        "stay safe. While offline the module buffers its data in flash and back-fills the cloud, with "
        "timestamps generated by the device, as soon as a link returns.",
        "Layer 2 — cloud. Battery health and remaining life are computed once per charge cycle from "
        "the shape of the charge curve, which changes measurably as cells age. The device sends only "
        "a short summary per charge and the cloud does the prediction, so the model can be improved "
        "without reflashing any vehicle.",
        "",
        ("An Edge AI design result worth highlighting", B),
        "Our first model mixed absolute context inputs (ambient temperature, current, charge level) "
        "with the relative ones. On an independent real pack it alarmed on a healthy run almost all "
        "of the time — worse than on the faulty run. The cause turned out to be that those absolute "
        "inputs had shifted far outside the range the model was trained on and, because they are "
        "identical for every cell, they raised the error of all cells at once. Removing them and "
        "keeping only cell-to-cell relations reduced false alarms, improved detection, and made the "
        "model transfer to packs it had never seen. That is also what would make manufacturing "
        "practical: one identical model in every unit, instead of one calibration per pack.",
        "",
        ("WISE-PaaS integration status", B),
        "The firmware already publishes the native WISE-PaaS SCADA format — topic "
        "/wisepaas/scada/{nodeId}/data with payload {\"d\":{…},\"ts\":…} — verified byte-for-byte "
        "against Advantech's own Edge SDK. While waiting for a WISE-IoT account we host the same "
        "underlying components ourselves (Mosquitto + Node-RED + InfluxDB + Grafana), so moving onto "
        "WISE-PaaS IoT Hub, Dashboard and AFS is a change of broker host and credentials, not a "
        "rewrite. Figures 2 and 3 show both states.",
    ],
    figs=["Figure 1 — Functional / system diagram: sensing → Edge AI on ESP32-S3 → MQTT "
          "(WISE-PaaS format) → cloud health prediction.",
          "Figure 2 — Current deployment: the same WISE-PaaS payload into a self-hosted broker / "
          "time-series store / dashboard stack.",
          "Figure 3 — Target deployment with a WISE-PaaS account: unchanged firmware, IoT Hub + "
          "Dashboard + AFS.",
          "Figure 4 — Our monitoring dashboard: pack state, the suspect cell, per-cell temperatures "
          "and the Layer 1 anomaly score. The data in this screenshot is simulated to show the "
          "scenario of one cell heating up."],
)

VI = dict(
    university="Trường Đại học Công nghệ Thông tin và Truyền thông — Đại học Thái Nguyên (ICTU)",
    department="Khoa Công nghệ thông tin",
    prof="Đào Thị Hằng",
    email_todo="",
    email_extra="Liên hệ đội: huynhpham13790@gmail.com · (+84) 359 260 290 — Phạm Văn Huynh, đội trưởng",
    duration=[
        "l   Học kỹ năng: 15/08/2026 → 10/09/2026 (chứng chỉ WISE-IoT Academy, giao thức "
        "MQTT / WISE-PaaS, TinyML)",
        "l   Phát triển ứng dụng: 01/09/2026 → 20/11/2026 (phần cứng, Edge AI, tích hợp cloud, "
        "kiểm chứng thực tế)",
        "l   Bán kết: 26/09/2026   ·   Chung kết: 27/11/2026",
    ],
    students=["1. Số đội: 01 (Đội Hủ Tiếu)", "2. Số thành viên mỗi đội: 05 sinh viên"],
    slots="05 slot (mỗi thành viên một slot)",
    space="01/09/2026 ~ 31/12/2026",
    team="Hủ Tiếu",
    nmem="05",
    topic="Hệ giám sát an toàn nhiệt pack pin xe điện dùng Edge AI — phát hiện bất thường từng cell "
          "ngay trên thiết bị + dự báo tuổi thọ còn lại trên cloud",
    adviser="Đào Thị Hằng",
    members=["Cả 5 thành viên: Khoa Công nghệ thông tin, ICTU.",
             "Phạm Văn Huynh — đội trưởng: điều phối, mô hình kinh doanh, thuyết trình",
             "Phạm Minh Tú — AI/ML: mô hình phát hiện bất thường Lớp 1, huấn luyện và đánh giá",
             "Đàm Đức Đôn — AI/ML: mô hình sức khoẻ pin Lớp 2, xử lý dữ liệu",
             "Hoàng Văn Huy — nhúng: firmware ESP32-S3, cảm biến, báo động tại chỗ",
             "Nguyễn Hoàng Anh Tuấn — cloud: tích hợp MQTT/WISE-PaaS, dashboard, video demo"],
    intro=[
        ("Đối tượng sử dụng", B),
        "Chính (B2B): đơn vị vận hành đội xe máy điện và mạng lưới trạm đổi pin — đội giao hàng, xe "
        "ôm công nghệ, doanh nghiệp cho thuê xe. Trong mô hình đổi pin, pin thuộc sở hữu của đơn vị "
        "vận hành, nên người mất tiền khi pin hỏng hoặc cháy chính là người có quyền mua thiết bị. "
        "Phụ: gara xe điện và đơn vị thẩm định xe cũ (thiết bị cầm tay xuất chứng nhận sức khoẻ "
        "pin). Người dùng cá nhân là câu chuyện để mở đầu, không phải mô hình kinh doanh.",
        "",
        ("Vấn đề cần giải quyết", B),
        "Pack pin lithium không hỏng đồng loạt: một cell yếu nóng lên trước. Mọi BMS đều đã có ngưỡng "
        "cứng 60 °C, nhưng tới lúc chạm ngưỡng tuyệt đối thì thermal runaway đã tự nuôi và không dừng "
        "được nữa. Khoảng 31 % vụ cháy xe điện xảy ra khi xe đang đỗ và khoảng 25 % xảy ra trong hầm "
        "để xe — đúng lúc một hệ thống chỉ bật khi chạy hoặc khi sạc đang ngủ. Thay pack xe máy điện "
        "ở Việt Nam tốn 1,5–12 triệu đồng, và một vụ cháy trong kho đội xe có thể mất cả kho.",
        "",
        ("Dự án làm gì", B),
        "Một module giá thấp đo nhiệt độ từng cell riêng biệt (6 đầu dò DS18B20 + cảm biến "
        "dòng/áp INA228 trên pack thí nghiệm 6 cell 18650) và chạy autoencoder ngay trên ESP32-S3, "
        "mỗi giây một lần. Mô hình không nhìn nhiệt độ tuyệt đối, nó nhìn quan hệ giữa một cell với phần "
        "còn lại của pack — lệch bao nhiêu, xếp hạng thứ mấy trong pack, và có đang nóng lên nhanh "
        "hơn các cell khác không. Ngưỡng cứng không bao giờ thấy được \"cell #3 nóng hơn các cell còn "
        "lại 4 °C dù mới 35 °C\"; mô hình này thấy. Một luật ngưỡng cứng 60 °C chạy song song, nằm "
        "ngoài AI, để mô hình có sai thì tuyến phòng thủ cuối cùng vẫn kêu.",
        "",
        ("Lợi ích mang lại", B),
        "· An toàn: chỉ ra sớm cell đang hoạt động khác thường, và báo động tại chỗ vẫn hoạt động khi "
        "mất mạng hoàn toàn.",
        "· Bảo dưỡng: báo sức khoẻ pin sau mỗi lần sạc biến việc thay pack từ sự cố khẩn cấp thành "
        "hạng mục bảo dưỡng có lịch, đồng thời kéo dài tuổi thọ hữu ích của cả đội pin.",
        "· Chi phí vận hành thấp: thiết bị dựng từ linh kiện phổ thông, giá rẻ; và trong mô hình đổi "
        "pin thì pin về trạm mỗi ngày nên WiFi trạm có thể làm đường truyền, không cần gói cước di "
        "động cho từng module.",
        "",
        ("Đội đã thử nghiệm những gì", B),
        "Đội chưa thử nghiệm trên pack xe điện đang vận hành. Công việc hiện tại là nghiên cứu trên "
        "pack thí nghiệm dựng từ cell cùng loại, cộng với các bộ dữ liệu công khai đo trên pack thật. "
        "Tới thời điểm này đội đã kiểm chứng được:",
        "· chuỗi cảm biến và đường truyền chạy thông suốt trên bàn thí nghiệm — số liệu lên dashboard "
        "theo thời gian thực và được đệm lại khi thiết bị mất mạng;",
        "· logic báo động bốn mức hoạt động đúng qua bộ kiểm đầy đủ trên bench, kể cả khi tắt hẳn đầu "
        "ra của AI;",
        "· trên dữ liệu công khai, lỗi tiêm vào một cell được phát hiện ổn định, còn vận hành bình "
        "thường thì gần như không sinh báo động giả qua các đợt chạy dài;",
        "· trên một pack thật độc lập mà mô hình chưa từng thấy, các file có lỗi tách rõ khỏi các "
        "file bình thường.",
        "Sưởi một cell trên chính pack của đội, rồi đo trên pack gắn trên xe chạy hằng ngày, là hai "
        "bước tiếp theo. Đội cũng nói rõ một giới hạn đã biết: vì Lớp 1 so các cell với nhau nên pack "
        "nóng đều thì không cell nào lệch — trường hợp đó do ngưỡng cứng 60 °C lo, và ngưỡng này đội "
        "chưa kiểm được trên phần cứng.",
    ],
    diagram=[
        ("Sơ đồ chức năng / hệ thống — xem ba hình bên dưới.", B),
        "",
        ("Edge AI / Edge Computing được dùng ra sao", B),
        "Lớp 1 — Edge AI, chạy ngay trên thiết bị. Một autoencoder nhỏ chạy trên chính ESP32-S3, mỗi "
        "giây một lần, gọn tới mức chỉ chiếm một phần nhỏ bộ nhớ flash của vi điều khiển. Đầu vào của "
        "nó thuần tương đối: cell này lệch bao nhiêu so với phần còn lại của pack, xếp thứ mấy trong "
        "các cell, có đang nóng lên nhanh hơn các cell bên cạnh không, và độ lệch là trôi từ từ hay "
        "nhảy đột ngột. Chỉ báo động khi tín hiệu giữ liên tục, nên nhiễu cảm biến một mình không "
        "kích hoạt được. Một ngưỡng cứng 60 °C chạy song song, nằm ngoài AI, làm tuyến phòng thủ cuối "
        "cùng độc lập.",
        "Vì sao chạy ở biên chứ không phải cloud: đọc cảm biến, suy luận, còi/LED tại chỗ và ghi log "
        "đều chạy khi không có mạng. Mạng chỉ cần để báo cho người ở xa, không cần để giữ an toàn. "
        "Lúc mất mạng, module đệm dữ liệu vào flash rồi tự đẩy bù lên cloud kèm timestamp do chính "
        "thiết bị sinh, ngay khi có mạng trở lại.",
        "Lớp 2 — cloud. Sức khoẻ pin và tuổi thọ còn lại được tính một lần mỗi chu kỳ sạc, dựa trên "
        "dáng của đường sạc — thứ thay đổi rõ rệt theo tuổi của cell. Thiết bị chỉ gửi một bản tóm "
        "tắt ngắn mỗi lần sạc, phần dự báo do cloud làm, nhờ vậy cải tiến mô hình không phải nạp lại "
        "firmware cho chiếc xe nào.",
        "",
        ("Một kết quả thiết kế Edge AI đáng nêu", B),
        "Mô hình đầu tiên trộn các đầu vào bối cảnh tuyệt đối (nhiệt độ môi trường, dòng, mức sạc) "
        "với các đầu vào tương đối. Trên một pack thật độc lập, nó báo động gần như suốt thời gian "
        "của một đợt chạy bình thường — cao hơn cả đợt có lỗi. Nguyên nhân tìm ra được là các đầu vào "
        "tuyệt đối đó đã lệch rất xa khỏi dải mà mô hình từng học, và vì chúng giống hệt nhau ở mọi "
        "cell nên đẩy sai số của tất cả các cell lên cùng lúc. Bỏ chúng đi, chỉ giữ quan hệ giữa các "
        "cell, thì báo động giả giảm, phát hiện tốt lên, và mô hình chuyển giao được sang pack chưa "
        "từng thấy. Đây cũng chính là thứ làm cho sản xuất hàng loạt khả thi: mọi máy nạp cùng một mô "
        "hình, thay vì mỗi pack phải hiệu chỉnh riêng.",
        "",
        ("Tình trạng tích hợp WISE-PaaS", B),
        "Firmware đã phát đúng định dạng SCADA gốc của WISE-PaaS — topic "
        "/wisepaas/scada/{nodeId}/data với payload {\"d\":{…},\"ts\":…} — đã đối chiếu byte-for-byte "
        "với Edge SDK chính chủ của Advantech. Trong lúc chờ cấp tài khoản WISE-IoT, đội tự host "
        "chính các thành phần nền bên dưới (Mosquitto + Node-RED + InfluxDB + Grafana), nên chuyển "
        "sang WISE-PaaS IoT Hub, Dashboard và AFS là đổi host broker và credential, không phải viết "
        "lại. Hình 2 và Hình 3 mô tả cả hai trạng thái.",
    ],
    figs=["Hình 1 — Sơ đồ chức năng/hệ thống: cảm biến → Edge AI trên ESP32-S3 → MQTT (định dạng "
          "WISE-PaaS) → dự báo sức khoẻ trên cloud.",
          "Hình 2 — Hiện trạng: cùng payload WISE-PaaS đi vào bộ broker / kho chuỗi thời gian / "
          "dashboard do đội tự dựng.",
          "Hình 3 — Đích nhắm khi có tài khoản WISE-PaaS: firmware không đổi, IoT Hub + Dashboard + "
          "AFS.",
          "Hình 4 — Dashboard giám sát của đội: trạng thái pack, cell đang nghi, nhiệt độ từng cell "
          "và điểm bất thường của Lớp 1. Số liệu trong ảnh là số mô phỏng để minh hoạ kịch bản một "
          "cell nóng lên."],
)


# ===================== DUNG FILE =====================
def put(cell, blocks, size=11):
    cell.text = ""
    p = cell.paragraphs[0]
    first = True
    for b in (blocks if isinstance(blocks, list) else [blocks]):
        text, flag = (b if isinstance(b, tuple) else (b, None))
        if not first:
            p = cell.add_paragraph()
        first = False
        p.alignment = WD_ALIGN_PARAGRAPH.LEFT
        p.paragraph_format.space_after = Pt(2)
        r = p.add_run(text)
        r.font.size = Pt(size)
        r.font.name = "Calibri"
        # chỗ phải tự điền: nhận diện bằng dấu ngoặc vuông đầu dòng
        if flag is True:
            r.bold = True
        if text.startswith("[") or flag == T:
            r.font.color.rgb = TODO
            r.italic = True


def build(cfg, out_name):
    out = os.path.join(DOCS, out_name)
    shutil.copy(SRC, out)
    d = Document(out)
    t0, t1 = d.tables[0], d.tables[1]

    put(t0.cell(1, 1), cfg["university"])
    put(t0.cell(1, 3), cfg["department"])
    put(t0.cell(2, 1), cfg["prof"])
    put(t0.cell(3, 1), [x for x in [cfg["email_todo"], cfg["email_extra"]] if x])
    put(t0.cell(5, 1), cfg["duration"])
    put(t0.cell(6, 1), cfg["students"])
    put(t0.cell(7, 1), cfg["slots"])
    put(t0.cell(8, 1), cfg["space"])

    put(t1.cell(1, 1), cfg["team"])
    put(t1.cell(1, 3), cfg["nmem"])
    put(t1.cell(2, 1), cfg["topic"])
    put(t1.cell(2, 3), cfg["adviser"])
    put(t1.cell(3, 1), cfg["members"])
    put(t1.cell(4, 1), cfg["intro"])
    put(t1.cell(5, 1), cfg["diagram"])

    cell = t1.cell(6, 0)
    cell.text = ""
    paths = [sorted(glob.glob(os.path.join(FIGDIR, n + "-*.png")))[0]
             for n in ("diag_arch", "diag_now", "diag_wise")]
    paths.append(os.path.join(DOCS, "slide_assets", "dashboard_real.png"))
    first = True
    for path, cap in zip(paths, cfg["figs"]):
        p = cell.paragraphs[0] if first else cell.add_paragraph()
        first = False
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        p.add_run().add_picture(path, width=Inches(5.9))
        c = cell.add_paragraph()
        c.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = c.add_run(cap)
        r.font.size = Pt(9)
        r.italic = True

    d.save(out)
    print("saved", out, os.path.getsize(out) // 1024, "KB")


build(EN, "HO_SO_BAN_KET_HuTieu.docx")
build(VI, "HO_SO_BAN_KET_HuTieu_VI.docx")
