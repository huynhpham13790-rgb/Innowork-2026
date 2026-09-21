#!/usr/bin/env python3
"""Dung bo slide ban ket InnoWorks 2026 - Doi Hu Tieu.

Sinh HAI file, cung bo cuc, khac ngon ngu:
    SLIDE_BAN_KET_HuTieu_EN.pptx   (ban trinh bay)
    SLIDE_BAN_KET_HuTieu_VI.pptx   (ban tieng Viet de doc/tap noi)

Nen TRANG, chu to (noi dung 17-26pt), tuong phan cao.
Moi chuoi chu deu nam trong TXT[lang] o duoi - sua chu thi sua o do.
"""
import os
from pptx import Presentation
from pptx.util import Inches as In, Pt, Emu
from pptx.dml.color import RGBColor as C
from pptx.enum.text import PP_ALIGN, MSO_ANCHOR
from pptx.enum.shapes import MSO_SHAPE, MSO_CONNECTOR
from PIL import Image

ROOT = "/home/huynh/code/Innowork 2026/docs"
ASSETS = os.path.join(ROOT, "slide_assets")

# ---------- bang mau: nen trang, chu dam, tuong phan cao ----------
BG      = C(0xFF, 0xFF, 0xFF)
PANEL   = C(0xF1, 0xF5, 0xF9)   # xam rat nhat, de tach khoi cho khoi nen
PANEL2  = C(0xE2, 0xE8, 0xF0)
EDGE    = C(0xCB, 0xD5, 0xE1)
INK     = C(0x0F, 0x17, 0x2A)   # chu chinh
MUTED   = C(0x47, 0x55, 0x69)   # chu phu - van du tuong phan tren nen xam nhat
AMBER   = C(0xB4, 0x53, 0x09)   # nhan: cam dam, doc duoc tren nen trang
GREEN   = C(0x15, 0x80, 0x3D)
RED     = C(0xB9, 0x1C, 0x1C)
BLUE    = C(0x03, 0x69, 0xA1)
CAPTION = C(0x64, 0x74, 0x8B)

FONT = "Segoe UI"
W, H = In(13.333), In(7.5)


# ===================== CHU =====================
TXT = {}
TXT["EN"] = dict(
    out="SLIDE_BAN_KET_HuTieu_EN.pptx",
    # 1
    s1_kicker="AIoT InnoWorks 2026  ·  Semi-final",
    s1_title="Edge-AI Thermal Safety Monitor\nfor EV Battery Packs",
    s1_team="Team Hu Tieu  ·  ICTU – Thai Nguyen University",
    s1_tag="We look for the failing cell before it becomes a fire.",
    s1_credit="Photo: lithium-ion pack destroyed by thermal runaway (JAL 787 APU battery) — NTSB, public domain",
    # 2
    s2_title="The problem",
    s2_lead1="A pack does not fail all at once.",
    s2_lead2="One weak cell heats up first.",
    s2_sub="Once thermal runaway starts, it feeds itself.",
    s2_facts=[("~31%", "of EV fires start while the vehicle is parked"),
              ("~25%", "happen in underground car parks"),
              ("1.5–12M", "VND to replace one e-motorbike pack")],
    s2_credit="Photo: vehicle fire in a car park (not an EV) — Jim Heaphy, CC BY-SA 3.0  ·  Fire statistics: EV FireSafe",
    # 3
    s3_title="Every BMS already has a 60 °C cut-off. So why AI?",
    s3_sub="Because the dangerous signal appears long before any absolute limit.",
    s3_l=["FIXED THRESHOLD", "Verdict:  all normal", "Nothing is near 60 °C."],
    s3_r=["OUR EDGE AI", "Verdict:  abnormal cell", "Cell #3 is +4 °C above the rest, under the same load."],
    s3_foot="The model reads relations between cells — not absolute degrees.",
    # 4
    s4_title="Our solution",
    s4_lead="A low-cost module that watches every cell and thinks on the device.",
    s4_cards=[("1", "DETECT", "An autoencoder on the ESP32-S3 flags the odd cell every second. No cloud needed."),
              ("2", "PREDICT", "A cloud model reads each charge curve and reports battery health and remaining life."),
              ("3", "ACT", "A 4-level local alarm: buzzer and LED fire even with the network down.")],
    s4_credit="Photo: 18650 / 21700 cells — Wikimedia Commons, CC0",
    # 5
    s5_title="System architecture",
    s5_sub="Two AI layers, split by how fast each answer is needed.",
    s5_n1=["6-cell test pack", "6 × DS18B20  ·  INA228", "1 sample / second"],
    s5_n2=["ESP32-S3", "LAYER 1 — autoencoder", "runs on-device, every second"],
    s5_n3=["MQTT / WISE-PaaS", "/wisepaas/scada/{nodeId}/data"],
    s5_n4=["Cloud", "LAYER 2", "health / remaining life", "1× per charge"],
    s5_alarm=["LOCAL ALARM", "buzzer + LED · works offline"],
    s5_store=["Store & forward", "buffered in flash while offline"],
    s5_note="Safety never depends on the network. The device detects and alarms on its own; "
            "the cloud adds dashboards and long-term prediction.",
    # 6
    s6_title="The hardware we built",
    s6_sub="A 6-cell 18650 test pack — every cell has its own probe.",
    s6_caps=["ESP32-S3-DevKitC-1\nvector instructions for ML",
             "6 × DS18B20\none probe per cell",
             "6S BMS with balancing\nthe part that cuts out",
             "6 matched 18650 cells\nsame batch, same age"],
    s6_note="Faults are induced safely: a ceramic power resistor warms one cell. Firmware hard-cuts "
            "the heater at 60 °C. No cell is ever abused.",
    s6_credit="Component photos: Wikimedia Commons (CC BY-SA 4.0 / CC BY 2.0 / CC0)",
    # 7
    s7_title="Layer 1 — the odd-cell detector",
    s7_sub="Runs on the microcontroller itself, every second, with no internet.",
    s7_head="It reads relations, not degrees",
    s7_bullets=["·  how far it sits from the pack average",
                "·  whether it heats up faster than the others",
                "·  slow drift (ageing) vs. sudden jump (fault)"],
    s7_foot="It alarms only if the signal persists for half a minute — noise alone never triggers it.",
    s7_stats=[("tiny", "about 1 KB\nof flash"),
              ("1 Hz", "inference rate\non the device"),
              ("offline", "detection needs\nno network"),
              ("60 °C", "hard threshold,\noutside the AI")],
    s7_note="If the model is ever wrong, the hard threshold still fires. That line is never removed.",
    # 8
    s8_title="What we have tested so far",
    s8_sub="A lab pack of similar cells, plus public datasets from real packs — not an EV pack in service.",
    s8_rows=[("Sensors and data path, on the bench", "live on the dashboard, buffered while offline"),
             ("Alarm logic, full bench test", "all four levels correct, even with the AI disabled"),
             ("Public dataset, injected cell fault", "detected reliably"),
             ("Independent real pack, unseen in training", "faulty runs separate clearly from healthy")],
    s8_foot="These are results on the bench and on public datasets. Warming a cell on our own pack, "
            "and measuring an EV pack in service, are the next two steps — we have not done them yet.",
    # 9
    s9_title="The test we failed — and what it taught us",
    s9_sub="We report this because it is the strongest engineering result we have.",
    s9_c1=["1 · FAILED", "On a real pack we had never trained on, the model alarmed on a healthy run.",
           "Worse than on the faulty one."],
    s9_c2=["2 · DIAGNOSED", "The absolute inputs — ambient, current, charge level — had drifted far "
                            "outside the training range.",
           "They are the same for every cell, so they lifted all cells at once."],
    s9_c3=["3 · FIXED", "Dropped them. Kept only cell-to-cell relations.",
           "Fewer false alarms, better detection, and the model now transfers to unseen packs."],
    s9_foot="This is also what makes mass production possible: one identical model ships in every unit.",
    # 10
    s10_title="Layer 2 — how long will this pack last?",
    s10_sub="You cannot deep-discharge a pack every week. But everyone charges every day.",
    s10_head="Read the shape of the charge curve.",
    s10_body="An aged cell reaches the voltage limit early, then stays in constant-voltage far longer.",
    s10_body2="Measured during an ordinary charge — nobody has to do anything.",
    s10_stats=[],
    s10_note="Layer 2 runs in the cloud on purpose: improving the model must not require reflashing "
             "every vehicle.",
    # 11
    s11_title="Four alarm levels, not one buzzer",
    s11_sub='"Keep an eye on it" and "cut the charger now" are different orders.',
    s11_levels=[("OK", "normal"), ("WATCH", "one cell drifting"),
                ("ALARM", "AI, sustained"), ("CRITICAL", "any cell over 60 °C")],
    s11_b1="SENSOR LOST is blue, not red.\n\"I cannot see the pack\" ≠ \"the pack is dangerous\".",
    s11_b2="Mute exists — and mute never kills the light.\nA buzzer you cannot silence gets its wires cut.",
    # 12
    s12_title="Where the system runs today",
    s12_sub="We already speak the WISE-PaaS protocol — we are simply hosting the layers ourselves.",
    s12_nodes=[["ESP32-S3", "WISE-PaaS payload"], ["Mosquitto", "MQTT broker"],
               ["Node-RED", "parse + Layer 2"], ["InfluxDB", "time series"], ["Grafana", "dashboard"]],
    s12_topic="topic  /wisepaas/scada/{nodeId}/data  — the same format as Advantech's own Edge SDK",
    s12_head="Why this stack?",
    s12_body="These are the same components WISE-PaaS is built on.",
    s12_body2="Nothing here is a workaround we would have to throw away.",
    s12_credit="Our own Grafana dashboard — data shown is simulated for this screenshot",
    # 13
    s12b_title="What the operator actually sees",
    s12b_sub="Three seconds to answer: is the pack safe, which cell, and can I trust the number?",
    s12b_cards=[("Top row first", "Pack state, suspect cell, hottest cell, sensor health — no chart reading required."),
                ("Cell #3 stands out", "The bar gauge is a spatial comparison, so the odd cell is obvious at a glance."),
                ("Honest by design", "Numbers that are still assumptions label themselves on screen.")],
    s12b_credit="Our own Grafana dashboard. The data in this screenshot is simulated to show the scenario.",
    s13_title="With a WISE-PaaS account",
    s13_sub="A config change, not a rewrite — the firmware does not change by a single line.",
    s13_n1=["ESP32-S3", "unchanged firmware", "same payload, same topic"],
    s13_n2=["WISE-PaaS IoT Hub", "device management + TLS"],
    s13_n3=["WISE-PaaS Dashboard"],
    s13_n4=["AFS — Layer 2 model serving"],
    s13_lab="only the broker host + credentials change",
    s13_cards=[("Device management", "Provisioning, TLS and OTA handled by the platform."),
               ("Dashboards & rules", "WISE-PaaS Dashboard replaces our Grafana."),
               ("AFS for Layer 2", "Retrain and redeploy without touching a vehicle.")],
    # 14
    s14_title="Live demo — 90 seconds",
    s14_steps=["The dashboard is streaming live cell temperatures.",
               "We unplug the network in front of you.",
               "We warm one cell with the resistor.",
               "The module detects it and sounds the alarm — still offline.",
               "Network back: the offline data lands on the dashboard, at the right timestamps."],
    s14_foot="Edge AI, local alarm and store-and-forward — all proven in one run.",
    s14_credit="Photo: e-scooter charging station — Wikimedia Commons, CC BY-SA 3.0",
    # 15
    s15_title="Who pays for this",
    s15_lead="Fleet operators and battery-swap networks — where one company owns the packs.",
    s15_cards=[("Owner = payer", "The operator owns the packs, so whoever loses money on a dead pack "
                                 "is whoever can buy the device."),
               ("Planned, not urgent", "Pack replacement becomes scheduled maintenance instead of an "
                                       "emergency."),
               ("No SIM needed", "Packs come back to a station daily — station Wi-Fi is the uplink.")],
    s15_foot="Individual riders are the emotional story. Fleets are the business model — and we know "
             "the difference.",
    s15_credit="Photo: e-scooter battery-swap cabinets — Wikimedia Commons, CC BY-SA 4.0",
    # 16
    s16_title="What we have not proven yet",
    s16_sub="We would rather say it than be asked it.",
    s16_rows=[("A whole pack overheating uniformly",
               "No deviation between cells, so Layer 1 stays silent. The hard 60 °C threshold covers "
               "it — not yet tested on hardware."),
              ("A real EV pack in service",
               "So far: a lab pack of similar cells, plus public datasets. Measuring a vehicle in "
               "daily use is next."),
              ("Ageing prediction on a worn pack",
               "Our cells are new — one point at the top of the scale. We plan to borrow a degraded pack.")],
    s16_foot="Nothing in this deck is claimed to be more than what we actually measured.",
    # 17
    s17_title="From semi-final to final",
    s17_sub="27 November 2026",
    s17_ms=[("NOW", "6-cell rig, Layer 1 on the device, 4-level alarm, cloud stack running"),
            ("OCT", "Current sensing → real charge cycles → real Layer 2 results"),
            ("NOV", "Worn-pack validation · enclosure · cost model · fleet interviews")],
    # 18
    s18_l1="One cell starts it.",
    s18_l2="We watch every one of them.",
    s18_team="Team Hu Tieu  ·  ICTU – Thai Nguyen University",
    s18_thanks="Thank you. Questions welcome — especially the hard ones.",
    s18_credit="Photo: 18650 cell after thermal runaway — Wikimedia Commons, CC BY 4.0",
)

TXT["VI"] = dict(
    out="SLIDE_BAN_KET_HuTieu_VI.pptx",
    s1_kicker="AIoT InnoWorks 2026  ·  Vòng Bán kết",
    s1_title="Giám sát an toàn nhiệt\npack pin xe điện bằng Edge AI",
    s1_team="Đội Hủ Tiếu  ·  ICTU – Đại học Thái Nguyên",
    s1_tag="Tìm ra cell hỏng trước khi nó thành đám cháy.",
    s1_credit="Ảnh: pack pin lithium-ion hỏng vì thermal runaway (pin APU Boeing 787 của JAL) — NTSB, phạm vi công cộng",
    s2_title="Vấn đề",
    s2_lead1="Pack pin không hỏng đồng loạt.",
    s2_lead2="Một cell yếu nóng lên trước.",
    s2_sub="Thermal runaway một khi đã chạy thì tự nuôi, không dừng được.",
    s2_facts=[("~31%", "vụ cháy xe điện xảy ra khi xe đang đỗ"),
              ("~25%", "xảy ra trong hầm để xe"),
              ("1,5–12tr", "đồng để thay một pack xe máy điện")],
    s2_credit="Ảnh: xe cháy trong bãi đỗ (không phải xe điện) — Jim Heaphy, CC BY-SA 3.0  ·  Số liệu cháy: EV FireSafe",
    s3_title="BMS nào cũng có ngưỡng cứng 60 °C. Vậy cần AI làm gì?",
    s3_sub="Vì dấu hiệu nguy hiểm xuất hiện rất lâu trước khi chạm bất kỳ ngưỡng tuyệt đối nào.",
    s3_l=["NGƯỠNG CỨNG", "Kết luận:  bình thường", "Không cell nào gần 60 °C."],
    s3_r=["EDGE AI CỦA ĐỘI", "Kết luận:  cell bất thường", "Cell #3 nóng hơn phần còn lại 4 °C, cùng một tải."],
    s3_foot="Mô hình đọc quan hệ giữa các cell — không đọc con số tuyệt đối.",
    s4_title="Giải pháp của đội",
    s4_lead="Một module giá thấp, theo dõi từng cell và tự suy luận ngay trên thiết bị.",
    s4_cards=[("1", "PHÁT HIỆN", "Autoencoder chạy trên ESP32-S3, mỗi giây chỉ ra cell bất thường. Không cần cloud."),
              ("2", "DỰ BÁO", "Mô hình trên cloud đọc đường sạc, báo sức khoẻ pin và tuổi thọ còn lại."),
              ("3", "HÀNH ĐỘNG", "Báo động tại chỗ 4 mức: còi và đèn vẫn kêu khi mất mạng hoàn toàn.")],
    s4_credit="Ảnh: cell 18650 / 21700 — Wikimedia Commons, CC0",
    s5_title="Kiến trúc hệ thống",
    s5_sub="Hai lớp AI, chia theo việc câu trả lời cần nhanh tới mức nào.",
    s5_n1=["Pack thí nghiệm 6 cell", "6 × DS18B20  ·  INA228", "1 mẫu / giây"],
    s5_n2=["ESP32-S3", "LỚP 1 — autoencoder", "chạy trên thiết bị, mỗi giây"],
    s5_n3=["MQTT / WISE-PaaS", "/wisepaas/scada/{nodeId}/data"],
    s5_n4=["Cloud", "LỚP 2", "sức khoẻ / tuổi thọ còn lại", "1 lần mỗi chu kỳ sạc"],
    s5_alarm=["BÁO ĐỘNG TẠI CHỖ", "còi + LED · chạy khi mất mạng"],
    s5_store=["Đệm và gửi bù", "lưu vào flash lúc offline"],
    s5_note="An toàn không bao giờ phụ thuộc vào mạng. Thiết bị tự phát hiện và tự kêu; cloud chỉ "
            "thêm dashboard và phần dự báo dài hạn.",
    s6_title="Phần cứng đội đã làm",
    s6_sub="Pack thí nghiệm 6 cell 18650 — mỗi cell một đầu dò riêng.",
    s6_caps=["ESP32-S3-DevKitC-1\ncó tập lệnh vector cho ML",
             "6 × DS18B20\nmỗi cell một đầu dò",
             "BMS 6S có cân bằng\nchính nó tạo tình huống ngắt",
             "6 cell 18650 đồng bộ\ncùng lô, cùng tuổi"],
    s6_note="Gây lỗi một cách an toàn: điện trở công suất sứ sưởi một cell. Firmware cắt cứng ở "
            "60 °C. Không cell nào bị lạm dụng.",
    s6_credit="Ảnh linh kiện: Wikimedia Commons (CC BY-SA 4.0 / CC BY 2.0 / CC0)",
    s7_title="Lớp 1 — bộ phát hiện cell bất thường",
    s7_sub="Chạy ngay trên vi điều khiển, mỗi giây một lần, không cần internet.",
    s7_head="Nó đọc quan hệ, không đọc độ C",
    s7_bullets=["·  lệch bao nhiêu so với trung bình của pack",
                "·  có nóng lên nhanh hơn các cell khác không",
                "·  lệch từ từ (lão hoá) hay đột ngột (sự cố)"],
    s7_foot="Chỉ báo động khi tín hiệu giữ liên tục nửa phút — nhiễu một mình không kích hoạt được.",
    s7_stats=[("rất nhỏ", "mô hình gọn\ntrong ~1 KB flash"),
              ("1 Hz", "tần số suy luận\ntrên thiết bị"),
              ("offline", "phát hiện\nkhông cần mạng"),
              ("60 °C", "ngưỡng cứng,\nnằm ngoài AI")],
    s7_note="Mô hình có sai thì ngưỡng cứng vẫn kêu. Dòng đó không bao giờ được bỏ.",
    s8_title="Đội đã thử nghiệm những gì",
    s8_sub="Pack thí nghiệm dùng cell tương tự, cộng với dữ liệu công khai đo trên pack thật — "
           "chưa phải pack xe điện đang chạy.",
    s8_rows=[("Cảm biến và đường truyền, trên bàn", "lên dashboard theo thời gian thực, đệm khi mất mạng"),
             ("Logic báo động, kiểm đủ trên bench", "cả bốn mức đúng, kể cả khi tắt AI"),
             ("Dữ liệu công khai, tiêm lỗi vào một cell", "phát hiện ổn định"),
             ("Pack thật độc lập, chưa từng thấy", "file lỗi tách rõ khỏi file bình thường")],
    s8_foot="Đây là kết quả trên bàn thí nghiệm và trên dữ liệu công khai. Sưởi một cell trên chính "
            "pack của đội, và đo trên pack xe điện đang chạy, là hai bước tiếp theo — đội chưa làm.",
    s9_title="Phép kiểm đội đã TRƯỢT — và nó dạy được gì",
    s9_sub="Đội nêu chuyện này vì đây là kết quả kỹ thuật giá trị nhất đội có.",
    s9_c1=["1 · TRƯỢT", "Trên pack thật chưa từng huấn luyện, mô hình báo động cả ở file bình thường.",
           "Cao hơn cả file có lỗi."],
    s9_c2=["2 · CHẨN ĐOÁN", "Các đầu vào tuyệt đối — nhiệt môi trường, dòng, mức sạc — lệch xa khỏi "
                            "dải đã học.",
           "Chúng giống nhau ở mọi cell nên đẩy tất cả lên cùng lúc."],
    s9_c3=["3 · ĐÃ SỬA", "Bỏ chúng, chỉ giữ quan hệ giữa các cell.",
           "Báo oan giảm, phát hiện tốt hơn, và chuyển giao được sang pack lạ."],
    s9_foot="Đây cũng chính là thứ làm cho sản xuất hàng loạt khả thi: mọi máy nạp cùng một mô hình.",
    s10_title="Lớp 2 — pack này còn dùng được bao lâu?",
    s10_sub="Không ai xả kiệt pin mỗi tuần để đo. Nhưng ngày nào người dùng cũng cắm sạc.",
    s10_head="Đọc dáng của đường sạc.",
    s10_body="Cell chai chạm ngưỡng điện áp sớm hơn, rồi nằm ở pha điện áp không đổi lâu hơn hẳn.",
    s10_body2="Đo được ngay trong một lần sạc bình thường — người dùng không phải làm gì.",
    s10_stats=[],
    s10_note="Lớp 2 đặt trên cloud là cố ý: cải tiến mô hình không được phép bắt nạp lại firmware "
             "cho từng xe.",
    s11_title="Bốn mức báo động, không phải một cái còi",
    s11_sub='"Theo dõi tiếp" và "ngắt sạc ngay" là hai mệnh lệnh khác nhau.',
    s11_levels=[("OK", "bình thường"), ("THEO DÕI", "một cell đang lệch"),
                ("BÁO ĐỘNG", "AI, giữ liên tục"), ("NGUY KỊCH", "có cell vượt 60 °C")],
    s11_b1="MẤT CẢM BIẾN báo xanh dương, không phải đỏ.\n\"Không nhìn thấy pack\" ≠ \"pack đang nguy hiểm\".",
    s11_b2="Có nút tắt tiếng — và tắt tiếng không tắt đèn.\nCòi không tắt được thì sẽ bị cắt dây.",
    s12_title="Hiện tại hệ thống đang chạy ở đâu",
    s12_sub="Firmware đã nói đúng giao thức WISE-PaaS — đội chỉ đang tự host các lớp bên dưới.",
    s12_nodes=[["ESP32-S3", "payload WISE-PaaS"], ["Mosquitto", "MQTT broker"],
               ["Node-RED", "bóc tách + Lớp 2"], ["InfluxDB", "chuỗi thời gian"], ["Grafana", "dashboard"]],
    s12_topic="topic  /wisepaas/scada/{nodeId}/data  — đúng định dạng của Edge SDK chính chủ Advantech",
    s12_head="Vì sao đúng bộ này?",
    s12_body="Đây chính là các thành phần mà WISE-PaaS dựng trên đó.",
    s12_body2="Không phần nào là giải pháp chống chế phải vứt đi.",
    s12_credit="Dashboard Grafana của chính đội — số liệu trong ảnh là số mô phỏng",
    s12b_title="Người vận hành nhìn thấy gì",
    s12b_sub="Ba giây để trả lời: pack có an toàn không, cell nào, và số này có tin được không?",
    s12b_cards=[("Đọc hàng đầu là đủ", "Trạng thái pack, cell đang nghi, cell nóng nhất, cảm biến khoẻ — không phải đọc biểu đồ."),
                ("Cell #3 lộ ngay", "Bar gauge là so sánh không gian, nên cell khác thường thấy ngay."),
                ("Trung thực trên màn hình", "Số nào còn là giả định thì tự khai là giả định.")],
    s12b_credit="Dashboard Grafana của chính đội. Số liệu trong ảnh là số mô phỏng để minh hoạ kịch bản.",
    s13_title="Nếu được cấp tài khoản WISE-PaaS",
    s13_sub="Là đổi cấu hình, không phải viết lại — firmware không đổi một dòng nào.",
    s13_n1=["ESP32-S3", "firmware không đổi", "cùng payload, cùng topic"],
    s13_n2=["WISE-PaaS IoT Hub", "quản lý thiết bị + TLS"],
    s13_n3=["WISE-PaaS Dashboard"],
    s13_n4=["AFS — nơi chạy mô hình Lớp 2"],
    s13_lab="chỉ đổi host broker + credential",
    s13_cards=[("Quản lý thiết bị", "Cấp phát, TLS và cập nhật OTA do nền tảng lo."),
               ("Dashboard & cảnh báo", "WISE-PaaS Dashboard thay cho Grafana của đội."),
               ("AFS cho Lớp 2", "Huấn luyện lại, triển khai mà không đụng vào xe nào.")],
    s14_title="Demo trực tiếp — 90 giây",
    s14_steps=["Dashboard đang chảy nhiệt độ từng cell theo thời gian thực.",
               "Rút mạng ngay trước mặt giám khảo.",
               "Sưởi một cell bằng điện trở.",
               "Module phát hiện và kêu còi — vẫn đang mất mạng.",
               "Cắm mạng lại: dữ liệu quãng offline hiện lên dashboard, đúng vị trí trên trục thời gian."],
    s14_foot="Edge AI, báo động tại chỗ và đệm-gửi-bù — cả ba trong một lần chạy.",
    s14_credit="Ảnh: trạm sạc xe điện — Wikimedia Commons, CC BY-SA 3.0",
    s15_title="Ai là người trả tiền",
    s15_lead="Đơn vị vận hành đội xe và mạng lưới trạm đổi pin — nơi pin thuộc về một pháp nhân.",
    s15_cards=[("Chủ sở hữu = người trả tiền", "Pin thuộc đơn vị vận hành, nên người mất tiền khi pin "
                                               "hỏng cũng chính là người mua được thiết bị."),
               ("Có kế hoạch, không khẩn cấp", "Thay pack thành bảo dưỡng có lịch, thay vì sự cố."),
               ("Không cần SIM", "Pin về trạm mỗi ngày — WiFi trạm làm đường truyền.")],
    s15_foot="Người dùng cá nhân là câu chuyện mở đầu. Đội xe mới là mô hình kinh doanh.",
    s15_credit="Ảnh: tủ đổi pin xe máy điện — Wikimedia Commons, CC BY-SA 4.0",
    s16_title="Những gì đội CHƯA chứng minh được",
    s16_sub="Thà nói trước còn hơn để bị hỏi.",
    s16_rows=[("Cả pack cùng nóng đều",
               "Không cell nào lệch nên Lớp 1 im. Ngưỡng cứng 60 °C lo việc này — chưa kiểm được "
               "trên phần cứng."),
              ("Pack xe điện đang vận hành thật",
               "Hiện mới có pack thí nghiệm cùng loại cell và dữ liệu công khai. Đo trên xe chạy "
               "hằng ngày là bước tiếp theo."),
              ("Dự báo lão hoá trên pack đã chai",
               "Cell còn mới — chỉ có một điểm ở đầu thang đo. Đội tính mượn pack đã chai.")],
    s16_foot="Không có điều gì trong bộ slide này được nói quá hơn thứ đội thật sự đo được.",
    s17_title="Từ Bán kết tới Chung kết",
    s17_sub="27/11/2026",
    s17_ms=[("HIỆN TẠI", "Giàn 6 cell, Lớp 1 chạy trên thiết bị, báo động 4 mức, cloud đã chạy"),
            ("THÁNG 10", "Đo dòng thật → chu kỳ sạc thật → kết quả Lớp 2 thật"),
            ("THÁNG 11", "Kiểm trên pack đã chai · vỏ hộp · mô hình chi phí · phỏng vấn đội xe")],
    s18_l1="Một cell châm ngòi.",
    s18_l2="Đội theo dõi từng cell.",
    s18_team="Đội Hủ Tiếu  ·  ICTU – Đại học Thái Nguyên",
    s18_thanks="Cảm ơn thầy cô. Rất mong nhận câu hỏi — nhất là câu khó.",
    s18_credit="Ảnh: cell 18650 sau thermal runaway — Wikimedia Commons, CC BY 4.0",
)


# ===================== HA TANG VE =====================
def build(lang):
    S = TXT[lang]
    prs = Presentation()
    prs.slide_width, prs.slide_height = W, H
    BLANK = prs.slide_layouts[6]

    def slide():
        s = prs.slides.add_slide(BLANK)
        r = s.shapes.add_shape(MSO_SHAPE.RECTANGLE, 0, 0, W, H)
        r.fill.solid(); r.fill.fore_color.rgb = BG; r.line.fill.background()
        r.shadow.inherit = False
        return s

    def txt(s, x, y, w, h, text, size=22, color=INK, bold=False, align=PP_ALIGN.LEFT,
            anchor=MSO_ANCHOR.TOP, space=8, italic=False, line=1.15):
        tb = s.shapes.add_textbox(x, y, w, h)
        tf = tb.text_frame
        tf.word_wrap = True
        tf.vertical_anchor = anchor
        tf.margin_left = tf.margin_right = 0
        tf.margin_top = tf.margin_bottom = 0
        for i, it in enumerate(text if isinstance(text, list) else [text]):
            body, ov = (it if isinstance(it, tuple) else (it, {}))
            p = tf.paragraphs[0] if i == 0 else tf.add_paragraph()
            p.alignment = ov.get("align", align)
            p.space_after = Pt(ov.get("space", space))
            p.line_spacing = ov.get("line", line)
            r = p.add_run(); r.text = body
            f = r.font
            f.name = FONT
            f.size = Pt(ov.get("size", size))
            f.bold = ov.get("bold", bold)
            f.italic = ov.get("italic", italic)
            f.color.rgb = ov.get("color", color)
        return tb

    def title(s, t, sub=None):
        # tieu de dai thi thu nho lai cho van gon MOT dong, khong de de len phu de
        sz = 34 if len(t) <= 44 else (29 if len(t) <= 56 else 26)
        txt(s, In(0.75), In(0.45), In(11.9), In(1.0), t, size=sz, bold=True, color=INK)
        bar = s.shapes.add_shape(MSO_SHAPE.RECTANGLE, In(0.75), In(1.22), In(1.1), Pt(5))
        bar.fill.solid(); bar.fill.fore_color.rgb = AMBER
        bar.line.fill.background(); bar.shadow.inherit = False
        if sub:
            txt(s, In(0.75), In(1.45), In(11.9), In(0.5), sub, size=20, color=MUTED)

    def photo(s, name, x, y, w, h, crop_bias=0.5):
        path = os.path.join(ASSETS, name)
        iw, ih = Image.open(path).size
        box_ar, img_ar = w / h, iw / ih
        if img_ar > box_ar:
            new_w = Emu(int(h * img_ar)); new_h = h
            px = x - Emu(int((new_w - w) * crop_bias)); py = y
        else:
            new_w = w; new_h = Emu(int(w / img_ar))
            px = x; py = y - Emu(int((new_h - h) * crop_bias))
        pic = s.shapes.add_picture(path, px, py, new_w, new_h)
        pic.crop_left = max(0.0, (x - px) / new_w)
        pic.crop_right = max(0.0, ((px + new_w) - (x + w)) / new_w)
        pic.crop_top = max(0.0, (y - py) / new_h)
        pic.crop_bottom = max(0.0, ((py + new_h) - (y + h)) / new_h)
        pic.left, pic.top, pic.width, pic.height = x, y, w, h
        return pic

    def photo_fit(s, name, x, y, w, h, border=True):
        """Dat anh vua trong khung, KHONG cat - dung khi moi chi tiet deu quan trong."""
        path = os.path.join(ASSETS, name)
        iw, ih = Image.open(path).size
        ar = iw / ih
        if ar > w / h:
            nw, nh = w, Emu(int(w / ar))
        else:
            nh, nw = h, Emu(int(h * ar))
        px = x + Emu(int((w - nw) / 2)); py = y + Emu(int((h - nh) / 2))
        pic = s.shapes.add_picture(path, px, py, nw, nh)
        if border:
            ln = s.shapes.add_shape(MSO_SHAPE.RECTANGLE, px, py, nw, nh)
            ln.fill.background(); ln.line.color.rgb = EDGE; ln.line.width = Pt(1)
            ln.shadow.inherit = False
        return pic

    def card(s, x, y, w, h, fill=PANEL, line=EDGE, radius=0.06):
        b = s.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, x, y, w, h)
        b.fill.solid(); b.fill.fore_color.rgb = fill
        if line:
            b.line.color.rgb = line; b.line.width = Pt(1.5)
        else:
            b.line.fill.background()
        b.shadow.inherit = False
        try:
            b.adjustments[0] = radius
        except Exception:
            pass
        return b

    def stat(s, x, y, w, big, label, color=AMBER, big_size=32, lab_size=17, h=1.55):
        card(s, x, y, w, In(h))
        txt(s, x + In(0.15), y + In(0.16), w - In(0.3), In(0.8), big, size=big_size,
            bold=True, color=color, align=PP_ALIGN.CENTER, line=1.0)
        txt(s, x + In(0.15), y + In(0.92), w - In(0.3), In(0.55), label, size=lab_size,
            color=MUTED, align=PP_ALIGN.CENTER, line=1.05)

    def node(s, x, y, w, h, lines, fill=PANEL, border=EDGE, size=15, title_size=18):
        card(s, x, y, w, h, fill=fill, line=border)
        items = [(l, {"size": title_size if i == 0 else size, "bold": i == 0,
                      "color": INK if i == 0 else MUTED,
                      "align": PP_ALIGN.CENTER, "space": 2})
                 for i, l in enumerate(lines)]
        txt(s, x + In(0.1), y, w - In(0.2), h, items, anchor=MSO_ANCHOR.MIDDLE)

    def arrow(s, x1, y1, x2, y2, color=AMBER, width=2.25):
        cn = s.shapes.add_connector(MSO_CONNECTOR.STRAIGHT, x1, y1, x2, y2)
        cn.line.color.rgb = color; cn.line.width = Pt(width)
        from pptx.oxml.ns import qn
        ln = cn.line._get_or_add_ln()
        ln.append(ln.makeelement(qn('a:tailEnd'), {'type': 'triangle', 'w': 'med', 'len': 'med'}))
        return cn

    def credit(s, text, w=6.4):
        txt(s, In(0.75), In(6.95), In(w), In(0.45), text, size=10, color=CAPTION, line=1.1)

    # ---------------- 1 TITLE ----------------
    s = slide()
    photo(s, "jal787.jpg", In(7.1), 0, In(6.233), H, crop_bias=0.5)
    txt(s, In(0.8), In(1.6), In(5.9), In(0.5), S["s1_kicker"], size=19, color=AMBER, bold=True)
    t1sz = 36 if max(len(x) for x in S["s1_title"].split("\n")) <= 30 else 30
    txt(s, In(0.8), In(2.05), In(5.9), In(2.3), S["s1_title"], size=t1sz, bold=True, color=INK, line=1.12)
    bar = s.shapes.add_shape(MSO_SHAPE.RECTANGLE, In(0.8), In(4.35), In(1.8), Pt(5))
    bar.fill.solid(); bar.fill.fore_color.rgb = AMBER
    bar.line.fill.background(); bar.shadow.inherit = False
    txt(s, In(0.8), In(4.7), In(5.9), In(1.6), [
        (S["s1_team"], {"size": 21, "bold": True, "color": INK}),
        (S["s1_tag"], {"size": 19, "color": AMBER, "italic": True}),
    ], space=10)
    credit(s, S["s1_credit"])

    # ---------------- 2 PROBLEM ----------------
    s = slide()
    photo(s, "auto_vallejo.jpg", In(7.3), 0, In(6.033), H, crop_bias=0.45)
    title(s, S["s2_title"])
    txt(s, In(0.75), In(1.75), In(6.0), In(1.6), [
        (S["s2_lead1"], {"size": 25, "bold": True, "color": INK}),
        (S["s2_lead2"], {"size": 25, "bold": True, "color": AMBER}),
        ("", {"size": 8}),
        (S["s2_sub"], {"size": 19, "color": MUTED}),
    ], space=8)
    for i, (big, lab) in enumerate(S["s2_facts"]):
        fy = In(3.95) + i * In(1.0)
        card(s, In(0.75), fy, In(6.0), In(0.88), fill=PANEL)
        txt(s, In(0.95), fy, In(1.95), In(0.88), big, size=24, bold=True, color=AMBER,
            anchor=MSO_ANCHOR.MIDDLE)
        txt(s, In(3.0), fy, In(3.6), In(0.88), lab, size=17, color=INK,
            anchor=MSO_ANCHOR.MIDDLE, line=1.1)
    credit(s, S["s2_credit"])

    # ---------------- 3 WHY AI ----------------
    s = slide()
    title(s, S["s3_title"], S["s3_sub"])
    photo(s, "fig_deviation.png", In(0.75), In(2.15), In(6.5), In(3.75))
    for side, (data, fill, line, accent, verdict) in enumerate(
            [(S["s3_l"], PANEL, EDGE, MUTED, GREEN), (S["s3_r"], PANEL2, AMBER, AMBER, RED)]):
        y = In(2.15) + side * In(2.0)
        card(s, In(7.6), y, In(5.0), In(1.8), fill=fill, line=line)
        txt(s, In(7.95), y, In(4.4), In(1.8), [
            (data[0], {"size": 18, "bold": True, "color": accent}),
            (data[1], {"size": 23, "bold": True, "color": verdict}),
            (data[2], {"size": 16, "color": MUTED}),
        ], space=4, anchor=MSO_ANCHOR.MIDDLE)
    txt(s, In(7.6), In(6.3), In(5.0), In(0.7), S["s3_foot"], size=18, color=INK, bold=True)

    # ---------------- 4 SOLUTION ----------------
    s = slide()
    photo(s, "cells.jpg", In(9.5), In(1.9), In(3.1), In(4.3), crop_bias=0.5)
    title(s, S["s4_title"])
    txt(s, In(0.75), In(1.9), In(8.4), In(0.9), S["s4_lead"], size=25, bold=True, color=AMBER)
    for i, (n, head, body) in enumerate(S["s4_cards"]):
        y = In(3.0) + i * In(1.1)
        card(s, In(0.75), y, In(8.4), In(1.0))
        txt(s, In(1.0), y, In(0.5), In(1.0), n, size=24, bold=True, color=AMBER,
            anchor=MSO_ANCHOR.MIDDLE)
        txt(s, In(1.6), y, In(1.9), In(1.0), head, size=19, bold=True, color=INK,
            anchor=MSO_ANCHOR.MIDDLE)
        txt(s, In(3.6), y, In(5.3), In(1.0), body, size=16, color=MUTED,
            anchor=MSO_ANCHOR.MIDDLE, line=1.15)
    credit(s, S["s4_credit"])

    # ---------------- 5 ARCHITECTURE ----------------
    s = slide()
    title(s, S["s5_title"], S["s5_sub"])
    y0 = In(2.3)
    node(s, In(0.7), y0, In(2.6), In(1.5), S["s5_n1"])
    node(s, In(3.95), y0, In(3.1), In(1.5), S["s5_n2"], fill=PANEL2, border=AMBER)
    node(s, In(7.7), y0, In(2.7), In(1.5), S["s5_n3"])
    node(s, In(10.95), y0, In(1.7), In(1.5), S["s5_n4"], fill=PANEL2, border=BLUE, size=13)
    arrow(s, In(3.3), y0 + In(0.75), In(3.95), y0 + In(0.75))
    arrow(s, In(7.05), y0 + In(0.75), In(7.7), y0 + In(0.75))
    arrow(s, In(10.4), y0 + In(0.75), In(10.95), y0 + In(0.75))
    node(s, In(3.95), In(4.5), In(3.1), In(1.15), S["s5_alarm"], border=RED)
    arrow(s, In(5.5), y0 + In(1.5), In(5.5), In(4.5), color=RED)
    node(s, In(7.7), In(4.5), In(2.7), In(1.15), S["s5_store"])
    arrow(s, In(9.05), y0 + In(1.5), In(9.05), In(4.5), color=MUTED, width=1.75)
    card(s, In(0.7), In(6.05), In(11.95), In(0.9), fill=PANEL2)
    txt(s, In(1.0), In(6.05), In(11.4), In(0.9), S["s5_note"], size=19, color=INK,
        anchor=MSO_ANCHOR.MIDDLE)

    # ---------------- 6 HARDWARE ----------------
    s = slide()
    title(s, S["s6_title"], S["s6_sub"])
    for i, (f, cap) in enumerate(zip(["esp32.jpg", "ds18b20.jpg", "bms6s.jpg", "cells_charger.jpg"],
                                     S["s6_caps"])):
        x = In(0.75) + i * In(3.05)
        photo(s, f, x, In(2.2), In(2.75), In(2.0))
        txt(s, x, In(4.35), In(2.8), In(1.0), cap, size=17, color=INK, line=1.2)
    card(s, In(0.75), In(5.65), In(11.85), In(1.05), fill=PANEL2, line=AMBER)
    txt(s, In(1.1), In(5.65), In(11.2), In(1.05), S["s6_note"], size=18, color=INK,
        anchor=MSO_ANCHOR.MIDDLE)
    credit(s, S["s6_credit"], w=11.9)

    # ---------------- 7 LAYER 1 ----------------
    s = slide()
    title(s, S["s7_title"], S["s7_sub"])
    txt(s, In(0.75), In(2.15), In(6.0), In(2.4),
        [(S["s7_head"], {"size": 22, "bold": True, "color": AMBER}), ("", {"size": 6})]
        + [(b, {"size": 18, "color": INK, "line": 1.1}) for b in S["s7_bullets"]], space=6)
    txt(s, In(0.75), In(4.45), In(6.0), In(0.7), S["s7_foot"], size=16, color=MUTED, line=1.15)
    for i, (big, lab) in enumerate(S["s7_stats"]):
        r, c = divmod(i, 2)
        stat(s, In(7.1) + c * In(2.85), In(2.15) + r * In(1.9), In(2.6), big, lab,
             big_size=24 if len(big) > 5 else 29, lab_size=14, h=1.72)
    card(s, In(0.75), In(6.1), In(11.85), In(0.85), fill=PANEL2, line=RED)
    txt(s, In(1.1), In(6.1), In(11.2), In(0.85), S["s7_note"], size=19, color=INK,
        anchor=MSO_ANCHOR.MIDDLE)

    # ---------------- 8 WHAT WE TESTED ----------------
    s = slide()
    title(s, S["s8_title"], S["s8_sub"])
    y = In(2.35)
    for i, (k, v) in enumerate(S["s8_rows"]):
        card(s, In(0.75), y, In(11.85), In(0.75), fill=PANEL if i % 2 == 0 else PANEL2)
        txt(s, In(1.1), y, In(5.4), In(0.75), k, size=18, color=INK, anchor=MSO_ANCHOR.MIDDLE)
        txt(s, In(6.8), y, In(5.5), In(0.75), v, size=18, bold=True, color=GREEN,
            anchor=MSO_ANCHOR.MIDDLE, line=1.05)
        y += In(0.83)
    txt(s, In(0.75), In(6.75), In(11.85), In(0.6), S["s8_foot"], size=16, color=MUTED, italic=True)

    # ---------------- 9 FAILED TEST ----------------
    s = slide()
    title(s, S["s9_title"], S["s9_sub"])
    for i, (data, line) in enumerate([(S["s9_c1"], RED), (S["s9_c2"], AMBER), (S["s9_c3"], GREEN)]):
        x = In(0.75) + i * In(4.1)
        card(s, x, In(2.25), In(3.8), In(3.85), fill=PANEL if i < 2 else PANEL2, line=line)
        txt(s, x + In(0.3), In(2.55), In(3.2), In(3.3), [
            (data[0], {"size": 21, "bold": True, "color": line}),
            ("", {"size": 6}),
            (data[1], {"size": 17, "color": INK}),
            ("", {"size": 6}),
            (data[2], {"size": 17, "color": MUTED}),
        ], space=7)
    txt(s, In(0.75), In(6.35), In(11.85), In(0.6), S["s9_foot"], size=20, color=AMBER, bold=True)

    # ---------------- 10 LAYER 2 ----------------
    s = slide()
    title(s, S["s10_title"], S["s10_sub"])
    txt(s, In(0.75), In(2.2), In(5.5), In(3.7), [
        (S["s10_head"], {"size": 24, "bold": True, "color": AMBER}),
        ("", {"size": 8}),
        (S["s10_body"], {"size": 19, "color": INK}),
        ("", {"size": 8}),
        (S["s10_body2"], {"size": 19, "color": MUTED}),
    ], space=9)
    photo(s, "fig_charge_curve.png", In(6.5), In(2.1), In(6.1), In(3.75))
    card(s, In(0.75), In(6.1), In(11.85), In(0.85), fill=PANEL2, line=BLUE)
    txt(s, In(1.1), In(6.1), In(11.2), In(0.85), S["s10_note"], size=18, color=INK,
        anchor=MSO_ANCHOR.MIDDLE)

    # ---------------- 11 ALARM ----------------
    s = slide()
    title(s, S["s11_title"], S["s11_sub"])
    cols = [GREEN, AMBER, C(0xC2, 0x41, 0x0C), RED]
    for i, (name, desc) in enumerate(S["s11_levels"]):
        x = In(0.75) + i * In(3.05)
        card(s, x, In(2.3), In(2.75), In(2.25), fill=PANEL, line=cols[i])
        txt(s, x, In(2.6), In(2.75), In(0.7), name, size=24, bold=True, color=cols[i],
            align=PP_ALIGN.CENTER)
        txt(s, x + In(0.2), In(3.4), In(2.35), In(1.0), desc, size=17, color=MUTED,
            align=PP_ALIGN.CENTER)
        if i < 3:
            arrow(s, x + In(2.75), In(3.4), x + In(3.05), In(3.4), color=MUTED, width=1.75)
    card(s, In(0.75), In(4.9), In(5.8), In(1.85), fill=PANEL, line=BLUE)
    txt(s, In(1.1), In(4.9), In(5.2), In(1.85), S["s11_b1"], size=18, color=INK,
        anchor=MSO_ANCHOR.MIDDLE)
    card(s, In(6.85), In(4.9), In(5.75), In(1.85), fill=PANEL, line=AMBER)
    txt(s, In(7.2), In(4.9), In(5.15), In(1.85), S["s11_b2"], size=18, color=INK,
        anchor=MSO_ANCHOR.MIDDLE)

    # ---------------- 12 TODAY ----------------
    s = slide()
    title(s, S["s12_title"], S["s12_sub"])
    y0 = In(2.4)
    xs = [In(0.7), In(3.5), In(6.2), In(8.9), In(11.0)]
    ws = [In(2.5), In(2.2), In(2.2), In(1.75), In(1.65)]
    for i, lines in enumerate(S["s12_nodes"]):
        node(s, xs[i], y0, ws[i], In(1.3), lines,
             fill=PANEL2 if i == 0 else PANEL, border=AMBER if i == 0 else EDGE,
             size=14, title_size=17)
        if i:
            arrow(s, xs[i - 1] + ws[i - 1], y0 + In(0.65), xs[i], y0 + In(0.65))
    txt(s, In(0.7), y0 + In(1.42), In(12.0), In(0.4), S["s12_topic"], size=16, color=AMBER,
        align=PP_ALIGN.CENTER)
    photo_fit(s, "dashboard_strip.png", In(0.7), In(4.3), In(8.4), In(2.6))
    txt(s, In(9.3), In(4.35), In(3.35), In(2.5), [
        (S["s12_head"], {"size": 20, "bold": True, "color": INK}),
        ("", {"size": 6}),
        (S["s12_body"], {"size": 17, "color": MUTED}),
        ("", {"size": 6}),
        (S["s12_body2"], {"size": 17, "color": INK}),
    ], space=6)
    credit(s, S["s12_credit"], w=11.9)

    # ---------------- 12b WHAT THE OPERATOR SEES ----------------
    s = slide()
    title(s, S["s12b_title"], S["s12b_sub"])
    photo_fit(s, "dashboard_real.png", In(0.7), In(2.1), In(8.4), In(4.8))
    for i, (h, b) in enumerate(S["s12b_cards"]):
        y = In(2.15) + i * In(1.65)
        card(s, In(9.35), y, In(3.28), In(1.5))
        txt(s, In(9.6), y + In(0.12), In(2.85), In(0.62), h, size=17, bold=True, color=AMBER, line=1.05)
        txt(s, In(9.6), y + In(0.72), In(2.85), In(0.72), b, size=14, color=MUTED, line=1.12)
    credit(s, S["s12b_credit"], w=11.9)

    # ---------------- 13 WITH WISE-PAAS ----------------
    s = slide()
    title(s, S["s13_title"], S["s13_sub"])
    y0 = In(2.4)
    node(s, In(0.7), y0, In(2.9), In(1.5), S["s13_n1"], fill=PANEL2, border=AMBER, size=15)
    node(s, In(4.35), y0, In(3.3), In(1.5), S["s13_n2"], fill=PANEL2, border=BLUE, size=15)
    node(s, In(8.6), y0, In(4.05), In(0.68), S["s13_n3"], border=BLUE, title_size=17)
    node(s, In(8.6), y0 + In(0.82), In(4.05), In(0.68), S["s13_n4"], border=BLUE, title_size=17)
    arrow(s, In(3.6), y0 + In(0.75), In(4.35), y0 + In(0.75))
    arrow(s, In(7.65), y0 + In(0.75), In(8.25), y0 + In(0.34), color=BLUE, width=1.75)
    arrow(s, In(7.65), y0 + In(0.75), In(8.25), y0 + In(1.16), color=BLUE, width=1.75)
    txt(s, In(3.4), y0 + In(1.58), In(5.2), In(0.4), S["s13_lab"], size=15, color=AMBER,
        align=PP_ALIGN.CENTER)
    for i, (h, b) in enumerate(S["s13_cards"]):
        x = In(0.7) + i * In(4.1)
        card(s, x, In(4.55), In(3.8), In(2.2))
        txt(s, x + In(0.3), In(4.72), In(3.2), In(0.75), h, size=18, bold=True, color=BLUE, line=1.05)
        txt(s, x + In(0.3), In(5.5), In(3.25), In(1.2), b, size=15, color=MUTED, line=1.18)

    # ---------------- 14 DEMO ----------------
    s = slide()
    photo(s, "charging.jpg", In(8.9), 0, In(4.433), H, crop_bias=0.5)
    title(s, S["s14_title"])
    y = In(2.05)
    for i, st in enumerate(S["s14_steps"]):
        c = s.shapes.add_shape(MSO_SHAPE.OVAL, In(0.75), y, In(0.52), In(0.52))
        c.fill.solid(); c.fill.fore_color.rgb = AMBER if i != 3 else RED
        c.line.fill.background(); c.shadow.inherit = False
        txt(s, In(0.75), y, In(0.52), In(0.52), str(i + 1), size=17, bold=True, color=BG,
            align=PP_ALIGN.CENTER, anchor=MSO_ANCHOR.MIDDLE)
        txt(s, In(1.5), y - In(0.05), In(6.9), In(0.85), st, size=19,
            color=INK if i != 3 else RED, bold=(i == 3), line=1.18)
        y += In(0.9)
    txt(s, In(0.75), In(6.4), In(7.7), In(0.5), S["s14_foot"], size=16, color=MUTED, italic=True)
    credit(s, S["s14_credit"])

    # ---------------- 15 BUSINESS ----------------
    s = slide()
    photo(s, "swap_cabinet.jpg", In(9.6), In(1.9), In(3.05), In(4.35), crop_bias=0.5)
    title(s, S["s15_title"])
    txt(s, In(0.75), In(1.9), In(8.5), In(0.85), S["s15_lead"], size=22, bold=True, color=AMBER)
    for i, (h, b) in enumerate(S["s15_cards"]):
        y = In(3.05) + i * In(1.1)
        card(s, In(0.75), y, In(8.5), In(1.0))
        txt(s, In(1.05), y, In(2.6), In(1.0), h, size=18, bold=True, color=INK,
            anchor=MSO_ANCHOR.MIDDLE, line=1.1)
        txt(s, In(3.85), y, In(5.2), In(1.0), b, size=16, color=MUTED,
            anchor=MSO_ANCHOR.MIDDLE, line=1.15)
    txt(s, In(0.75), In(6.4), In(8.5), In(0.5), S["s15_foot"], size=16, color=MUTED, italic=True)
    credit(s, S["s15_credit"])

    # ---------------- 16 LIMITS ----------------
    s = slide()
    title(s, S["s16_title"], S["s16_sub"])
    y = In(2.25)
    for i, (h, b) in enumerate(S["s16_rows"]):
        card(s, In(0.75), y, In(11.85), In(1.4), fill=PANEL if i % 2 == 0 else PANEL2)
        txt(s, In(1.15), y + In(0.15), In(4.6), In(1.1), h, size=20, bold=True, color=AMBER,
            anchor=MSO_ANCHOR.MIDDLE)
        txt(s, In(6.1), y + In(0.12), In(6.2), In(1.15), b, size=16, color=MUTED,
            anchor=MSO_ANCHOR.MIDDLE, line=1.2)
        y += In(1.55)
    txt(s, In(0.75), In(7.0), In(11.85), In(0.5), S["s16_foot"], size=19, color=INK, bold=True)

    # ---------------- 17 ROADMAP ----------------
    s = slide()
    title(s, S["s17_title"], S["s17_sub"])
    line = s.shapes.add_shape(MSO_SHAPE.RECTANGLE, In(1.2), In(3.5), In(10.9), Pt(4))
    line.fill.solid(); line.fill.fore_color.rgb = EDGE
    line.line.fill.background(); line.shadow.inherit = False
    for i, ((tag, body), col) in enumerate(zip(S["s17_ms"], [AMBER, BLUE, GREEN])):
        x = In(0.9) + i * In(4.0)
        c = s.shapes.add_shape(MSO_SHAPE.OVAL, x + In(1.6), In(3.33), In(0.42), In(0.42))
        c.fill.solid(); c.fill.fore_color.rgb = col
        c.line.fill.background(); c.shadow.inherit = False
        txt(s, x, In(2.6), In(3.6), In(0.5), tag, size=22, bold=True, color=col,
            align=PP_ALIGN.CENTER)
        card(s, x, In(4.1), In(3.6), In(1.9))
        txt(s, x + In(0.3), In(4.1), In(3.0), In(1.9), body, size=18, color=INK,
            align=PP_ALIGN.CENTER, anchor=MSO_ANCHOR.MIDDLE, line=1.2)

    # ---------------- 18 CLOSING ----------------
    s = slide()
    photo(s, "cell_explosion.jpg", In(7.3), 0, In(6.033), H, crop_bias=0.5)
    e_sz = 34 if max(len(S["s18_l1"]), len(S["s18_l2"])) <= 28 else 29
    txt(s, In(0.8), In(2.2), In(6.0), In(2.4), [
        (S["s18_l1"], {"size": e_sz, "bold": True, "color": INK}),
        (S["s18_l2"], {"size": e_sz, "bold": True, "color": AMBER}),
    ], space=6)
    bar = s.shapes.add_shape(MSO_SHAPE.RECTANGLE, In(0.8), In(4.35), In(1.8), Pt(5))
    bar.fill.solid(); bar.fill.fore_color.rgb = AMBER
    bar.line.fill.background(); bar.shadow.inherit = False
    txt(s, In(0.8), In(4.7), In(6.0), In(1.6), [
        (S["s18_team"], {"size": 21, "bold": True, "color": INK}),
        (S["s18_thanks"], {"size": 19, "color": MUTED}),
    ], space=10)
    credit(s, S["s18_credit"])

    out = os.path.join(ROOT, S["out"])
    prs.save(out)
    print("saved", out, os.path.getsize(out) // 1024, "KB")


for lg in ("EN", "VI"):
    build(lg)
