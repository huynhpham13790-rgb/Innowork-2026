#!/usr/bin/env python3
"""
Bản dịch tiếng Anh cho dashboard Grafana (QĐ-046).

VÌ SAO LÀ MỘT DASHBOARD RIÊNG, KHÔNG PHẢI NÚT ĐỔI NGÔN NGỮ:
Grafana không cho một dashboard đổi ngôn ngữ theo người xem — tiêu đề panel là
chuỗi tĩnh nằm trong JSON. Nên bản tiếng Anh là một dashboard thứ hai, sinh từ
CÙNG mã nguồn: `make_dashboard.py --lang en`. Hai bản không bao giờ lệch nhau
về bố cục hay truy vấn, vì chỉ có chữ là khác.

CÁCH DÙNG TỪ ĐIỂN NÀY: khớp CHÍNH XÁC cả chuỗi. Không khớp thì giữ nguyên
tiếng Việt và script IN RA CẢNH BÁO — thà thấy một dòng tiếng Việt lọt vào và
biết mà sửa, còn hơn dịch nửa vời rồi không ai phát hiện.
"""

TITLE = {
 "TRẠNG THÁI PACK":                       "PACK STATUS",
 "Cell đang nghi":                        "Suspect cell",
 "Cell nóng nhất":                        "Hottest cell",
 "Cảm biến khoẻ":                         "Healthy sensors",
 "Dòng/áp: số THẬT?":                     "Current/voltage: REAL?",
 "Lớp 1 — Nhiệt độ từng cell ngay lúc này":
     "Layer 1 — Per-cell temperature right now",
 "Lớp 1 — Điểm bất thường từng cell":     "Layer 1 — Per-cell anomaly score",
 "Lớp 1 — DẠNG bất thường":               "Layer 1 — Anomaly PATTERN",
 "Việc phải làm":                         "What to do",
 "Ba số đã dùng để tra":                  "The three numbers behind it",
 "Lớp 2 — SOH (sức khoẻ pack)":           "Layer 2 — SOH (pack health)",
 "Lớp 2 — RUL: số chu kỳ còn lại":        "Layer 2 — RUL: cycles remaining",
 "Mô hình có đang NGOẠI SUY?":            "Is the model EXTRAPOLATING?",
 "Nhiệt độ 6 cell theo thời gian":        "Six-cell temperature over time",
 "Tỉ lệ lỗi bus 1-Wire (%)":              "1-Wire bus error rate (%)",
 "Số lần báo động\ntừ lúc bật máy":       "Alarms raised\nsince power-on",
}

DESC = {
 "Mức leo thang của alarm.cpp. Không có dữ liệu = ESP32 mất kết nối, KHÔNG phải an toàn.":
   "Escalation level from alarm.cpp. No data = the ESP32 is offline, NOT that all is well.",
 "Cell có sai số tái tạo lớn nhất. Chỉ có nghĩa khi TRẠNG THÁI khác AN TOÀN.":
   "Cell with the largest reconstruction error. Only meaningful when PACK STATUS is not normal.",
 "Đỏ từ 60.0 °C — ngưỡng cứng, độc lập hoàn toàn với AI.":
   "Red from 60.0 °C — a hard limit, entirely independent of the AI.",
 "Trên tổng 6 (QĐ-036). Dưới 6 nghĩa là số liệu KHÔNG đầy đủ — xem docs/HAI_LOP_AI.":
   "Out of 6 (QĐ-036). Fewer than 6 means the data is INCOMPLETE — see docs/HAI_LOP_AI.",
 "0 = chưa có INA228, dòng và SOC là hằng số giả định. Đừng tin đồ thị Lớp 2.":
   "0 = no INA228 yet; current and SoC are assumed constants. Do not trust the Layer 2 charts.",
 "So sánh KHÔNG GIAN giữa 6 cell tại một thời điểm — cell nào khác phần còn lại thì thấy ngay, không phải đọc 6 đường chồng nhau.":
   "A SPATIAL comparison across the 6 cells at one instant — the odd cell stands out immediately, instead of reading 6 overlapping lines.",
 "Đường đỏ = ngưỡng 1.0707. Vượt ngưỡng phải GIỮ LIÊN TỤC 60 giây mới thành báo động — chống báo động giả do nhiễu.":
   "Red line = the 1.0707 threshold. Crossing it must PERSIST for 60 seconds to raise an alarm — this rejects noise-driven false alarms.",
 "Tra bảng TH-1/2/3 từ ba đặc trưng đã tính sẵn (dev_mean, dT_diff, dev_shock). Đây là phân loại DẠNG, KHÔNG phải chẩn đoán nguyên nhân — mô hình chưa bao giờ được dạy tên của bất kỳ lỗi nào. Xem QĐ-042.":
   "Looks up cases TH-1/2/3 from three already-computed features (dev_mean, dT_diff, dev_shock). This classifies the PATTERN, it does NOT diagnose a cause — the model was never taught the name of any fault. See QĐ-042.",
 "Lấy nguyên từ bảng trong docs/HAI_LOP_AI_HOAT_DONG_THE_NAO.md. Phân biệt TH-1 với TH-2 là khác biệt giữa cách ly pack ngay và ghi sổ để mai xem.":
   "Taken verbatim from the table in docs/HAI_LOP_AI_HOAT_DONG_THE_NAO.md. Telling TH-1 from TH-2 is the difference between isolating the pack now and noting it for tomorrow.",
 "Đưa số thô ra cạnh lời khuyên để người còn KIỂM được.":
   "The raw numbers sit next to the advice so a human can still CHECK it.",
 "SOH ở mức PACK, không phải từng cell — pack nối tiếp sống chết theo cell yếu nhất (QĐ-027). 80 % là mốc thường dùng để coi là hết đời xe.":
   "SOH at PACK level, not per cell — a series pack lives or dies by its weakest cell (QĐ-027). 80 % is the usual end-of-automotive-life mark.",
 "Hồi quy tuyến tính trên t_cv (thời gian ở giai đoạn điện áp không đổi). Nội trở tăng => vào CV sớm hơn => t_cv dài ra.":
   "Linear regression on t_cv (time spent in the constant-voltage phase). Rising internal resistance => CV starts earlier => t_cv grows.",
 "Mô hình tuyến tính ngoài vùng dữ liệu huấn luyện thì con số RUL vô nghĩa. Thà nói ra còn hơn im lặng đưa số đẹp.":
   "Outside its training range a linear model's RUL figure is meaningless. Better to say so than to quietly serve a tidy number.",
 "Gồm cả Ambient_Temp (cảm biến thứ 9, đo môi trường — KHÔNG dán lên cell).":
   "Includes Ambient_Temp (the 9th sensor, measuring the room — NOT attached to a cell).",
 "Đã từng đo được 35 % lỗi do ẩm trên đầu dò mà không có dấu hiệu gì khác (QĐ-024). Đây là lý do phải đếm và hiển thị.":
   "A 35 % error rate from probe moisture was once measured with no other symptom (QĐ-024). That is why this is counted and shown.",
 "Cách duy nhất biết đêm qua có chuyện gì xảy ra mà không ai ở đó nghe còi.":
   "The only way to know something happened overnight with nobody there to hear the buzzer.",
}


# Nhãn GIÁ TRỊ và tên trục. Đây mới là chữ người xem đọc nhiều nhất — tiêu đề
# panel chỉ đọc một lần, còn mấy chữ này thì nhìn suốt. Bản dịch đầu tiên bỏ sót
# hẳn nhóm này và dashboard "tiếng Anh" vẫn hiện NGẮT SẠC · NGẮT TẢI.
VALUE = {
 "AN TOÀN":                      "SAFE",
 "THEO DÕI":                     "WATCH",
 "NGUY KỊCH":                    "CRITICAL",
 "MẤT KẾT NỐI":                  "LINK LOST",
 "BÁO ĐỘNG":                     "ALARM",
 "ĐO THẬT":                      "REAL MEASUREMENT",
 "CÒN GIẢ ĐỊNH":                 "STILL ASSUMED",
 "Chưa rõ dạng":                 "Pattern unclear",
 "NÓNG LÊN NHANH (TH-1)":        "HEATING FAST (TH-1)",
 "Nóng hơn, ổn định (TH-2)":     "Hotter but steady (TH-2)",
 "LẠNH bất thường (TH-3)":       "Abnormally COLD (TH-3)",
 "Theo dõi tiếp":                "Keep watching",
 "NGẮT SẠC · NGẮT TẢI · CÁCH LY PACK":
     "STOP CHARGE · STOP LOAD · ISOLATE PACK",
 "Ghi sổ, kiểm lúc bảo dưỡng":   "Log it, check at next service",
 "Kiểm mối nối cell và cảm biến":"Check cell joints and the sensor",
 "Lệch so với pack (°C)":        "Offset vs pack (°C)",
 "Nhanh hơn pack (°C/phút)":     "Faster than pack (°C/min)",
 "Đột ngột so với nền (°C)":     "Sudden vs baseline (°C)",
 "TRONG VÙNG\nĐÃ HUẤN LUYỆN":    "WITHIN\nTRAINING RANGE",
 "ĐANG NGOẠI SUY\nKHÔNG ĐÁNG TIN":"EXTRAPOLATING\nNOT TRUSTWORTHY",
}

DASH_TITLE = "Hu Tieu — 6S battery pack monitoring"


def translate(dash):
    """Dịch tại chỗ. Trả về số chuỗi KHÔNG tìm thấy bản dịch."""
    miss = 0
    dash["title"] = DASH_TITLE
    dash["uid"] = "hutieu-pin-en"
    for p in dash.get("panels", []):
        t = p.get("title")
        if t:
            if t in TITLE: p["title"] = TITLE[t]
            else: miss += 1; print(f"  ⚠️ chua dich tieu de: {t!r}")
        d = p.get("description")
        if d:
            if d in DESC: p["description"] = DESC[d]
            else: miss += 1; print(f"  ⚠️ chua dich mo ta: {d[:60]!r}…")
        miss += _walk(p)
    return miss


# Ký tự tiếng Việt có dấu. Dùng để TỰ TÌM chuỗi còn sót thay vì trông chờ người
# nhớ hết — bản dịch đầu tiên sót 16 chuỗi và không có gì báo.
_VI = "àáảãạăằắẳẵặâầấẩẫậèéẻẽẹêềếểễệìíỉĩịòóỏõọôồốổỗộơờớởỡợùúủũụưừứửữựỳýỷỹỵđ"


def _walk(o):
    """Dịch nhãn giá trị / tên trục ở mọi độ sâu. Trả về số chuỗi còn sót."""
    miss = 0
    if isinstance(o, dict):
        for k, v in list(o.items()):
            if isinstance(v, str):
                if v in VALUE: o[k] = VALUE[v]
                elif k in ("text", "noValue") and any(c in v.lower() for c in _VI):
                    miss += 1; print(f"  ⚠️ chua dich nhan: {v!r}")
            else:
                miss += _walk(v)
    elif isinstance(o, list):
        for v in o: miss += _walk(v)
    return miss
