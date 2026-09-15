#!/usr/bin/env python3
"""
Sinh và nạp dashboard Grafana cho pack pin Hủ Tiếu.

VÌ SAO SINH BẰNG SCRIPT CHỨ KHÔNG KÉO THẢ TRÊN GIAO DIỆN:
Dashboard kéo thả nằm trong database của Grafana, không nằm trong git. Dựng lại
máy là mất, và không ai biết ai đã đổi gì. File này đưa giao diện vào diện quản
lý phiên bản như mọi thứ khác.

NGUYÊN TẮC THIẾT KẾ — đối tượng là GIÁM KHẢO trong 5 PHÚT, không phải kỹ sư:

 1. HÀNG ĐẦU TRẢ LỜI ĐƯỢC MÀ KHÔNG CẦN ĐỌC BIỂU ĐỒ NÀO.
    "Pack có an toàn không? Cell nào? Tin được số này không?" — ba câu đó phải
    trả lời xong trong 3 giây đầu. Bản cũ đặt biểu đồ 8 đường nhiệt độ lên đầu:
    muốn biết có an toàn không thì phải đọc và so sánh 8 đường, tức là bắt người
    xem làm việc của máy.

 2. CÓ HÀNG RIÊNG CHO "SỐ NÀY CÓ THẬT KHÔNG".
    `Meter_IsReal`, `Ambient_IsReal`, `Sensor_Healthy` — đây là thứ phân biệt hệ
    này với một bản demo tô vẽ. Giấu chúng đi là vứt bỏ đúng điểm mạnh nhất.
    Số giả định phải TỰ KHAI là giả định, ngay trên màn hình.

 3. NHIỆT ĐỘ 8 CELL DÙNG BAR GAUGE, KHÔNG DÙNG 8 ĐƯỜNG CHỒNG NHAU.
    Câu hỏi thật là "cell nào khác phần còn lại", mà đó là so sánh KHÔNG GIAN
    giữa 8 giá trị tại một thời điểm — bar gauge trả lời tức thì. Lịch sử theo
    thời gian vẫn giữ, nhưng đẩy xuống dưới vì nó trả lời câu hỏi khác.

 4. NGƯỠNG VẼ THÀNH ĐƯỜNG, KHÔNG ĐỂ NGƯỜI XEM TỰ NHỚ.
    Điểm bất thường 1,847 và ngưỡng cứng 60 °C đều vẽ lên biểu đồ.

Chạy: VEDCaPhenika/planb_cloud/grafana/make_dashboard.py
"""
import json, os, sys, urllib.request, base64
from pathlib import Path

HERE = Path(__file__).parent
ENV = HERE.parent / ".env"
DS = {"type": "influxdb", "uid": "cfxg34mlaudxce"}
BUCKET = "battery"

AE_TH = 1.84735274      # khớp AE_THRESHOLD trong cell_ai.h
T_CRIT = 60.0           # khớp AL_T_CRIT trong alarm.h


def q(flux):
    return [{"datasource": DS, "query": flux, "refId": "A"}]


def last(tag, rng="-6h"):
    return q(f'''from(bucket: "{BUCKET}")
  |> range(start: {rng})
  |> filter(fn: (r) => r._measurement == "cell" and r._field == "value" and r.tag == "{tag}")
  |> last()''')


def series(regex, rng="v.timeRangeStart"):
    return q(f'''from(bucket: "{BUCKET}")
  |> range(start: v.timeRangeStart, stop: v.timeRangeStop)
  |> filter(fn: (r) => r._measurement == "cell" and r._field == "value" and r.tag =~ {regex})
  |> aggregateWindow(every: v.windowPeriod, fn: mean, createEmpty: false)
  |> keep(columns: ["_time","_value","tag"])''')


def panel(pid, typ, title, x, y, w, h, targets, opts=None, fc=None, desc=None):
    p = {"id": pid, "type": typ, "title": title, "datasource": DS,
         "gridPos": {"x": x, "y": y, "w": w, "h": h}, "targets": targets,
         "options": opts or {}, "fieldConfig": fc or {"defaults": {}, "overrides": []}}
    if desc:
        p["description"] = desc
    return p


def stat_opts(size=48, mode="value"):
    return {"reduceOptions": {"calcs": ["lastNotNull"], "fields": "", "values": False},
            "textMode": mode, "colorMode": "background", "graphMode": "none",
            "justifyMode": "center", "text": {"valueSize": size}}


def thresholds(steps):
    return {"mode": "absolute", "steps": steps}


def main():
    P = []
    pid = 0

    def nxt():
        nonlocal pid
        pid += 1
        return pid

    # ===================== HÀNG 1 — trả lời trong 3 giây =====================
    # Mức báo động: ánh xạ số sang chữ. Để nguyên 0/1/2/3 thì người xem phải
    # tra bảng, tức là màn hình chưa làm xong việc của nó.
    P.append(panel(nxt(), "stat", "TRẠNG THÁI PACK", 0, 0, 6, 5,
        last("Alarm_Level"),
        stat_opts(40),
        {"defaults": {
            "mappings": [{"type": "value", "options": {
                "0": {"text": "AN TOÀN",   "color": "green",  "index": 0},
                "1": {"text": "THEO DÕI",  "color": "yellow", "index": 1},
                "2": {"text": "BÁO ĐỘNG",  "color": "orange", "index": 2},
                "3": {"text": "NGUY KỊCH", "color": "red",    "index": 3}}}],
            "noValue": "MẤT KẾT NỐI",
            "thresholds": thresholds([{"color": "text", "value": None}])},
         "overrides": []},
        "Mức leo thang của alarm.cpp. Không có dữ liệu = ESP32 mất kết nối, "
        "KHÔNG phải an toàn."))

    P.append(panel(nxt(), "stat", "Cell đang nghi", 6, 0, 4, 5,
        last("AI_WorstCell"), stat_opts(48),
        {"defaults": {"unit": "none", "noValue": "—",
                      "thresholds": thresholds([{"color": "text", "value": None}])},
         "overrides": []},
        "Cell có sai số tái tạo lớn nhất. Chỉ có nghĩa khi TRẠNG THÁI khác AN TOÀN."))

    P.append(panel(nxt(), "stat", "Cell nóng nhất", 10, 0, 5, 5,
        q(f'''from(bucket: "{BUCKET}")
  |> range(start: -6h)
  |> filter(fn: (r) => r._measurement == "cell" and r._field == "value" and r.tag =~ /^Cell0[1-8]_Temp$/)
  |> last()
  |> max()'''),
        stat_opts(48),
        {"defaults": {"unit": "celsius", "decimals": 1, "noValue": "—",
                      "thresholds": thresholds([
                          {"color": "green", "value": None},
                          {"color": "yellow", "value": 45},
                          {"color": "orange", "value": 55},
                          {"color": "red", "value": T_CRIT}])},
         "overrides": []},
        f"Đỏ từ {T_CRIT} °C — ngưỡng cứng, độc lập hoàn toàn với AI."))

    P.append(panel(nxt(), "stat", "Cảm biến khoẻ", 15, 0, 4, 5,
        last("Sensor_Healthy"), stat_opts(48),
        {"defaults": {"unit": "none", "max": 8, "noValue": "0",
                      "thresholds": thresholds([
                          {"color": "red", "value": None},
                          {"color": "orange", "value": 6},
                          {"color": "green", "value": 8}])},
         "overrides": []},
        "Trên tổng 8. Dưới 8 nghĩa là số liệu KHÔNG đầy đủ — xem docs/HAI_LOP_AI."))

    # Panel này là đặc sản của dự án: số nào đang là GIẢ ĐỊNH thì tự khai.
    P.append(panel(nxt(), "stat", "Dòng/áp: số THẬT?", 19, 0, 5, 5,
        last("Meter_IsReal"), stat_opts(28),
        {"defaults": {
            "mappings": [{"type": "value", "options": {
                "1": {"text": "ĐO THẬT",       "color": "green", "index": 0},
                "0": {"text": "CÒN GIẢ ĐỊNH",  "color": "red",   "index": 1}}}],
            "noValue": "CÒN GIẢ ĐỊNH",
            "thresholds": thresholds([{"color": "text", "value": None}])},
         "overrides": []},
        "0 = chưa có INA228, dòng và SOC là hằng số giả định. Đừng tin đồ thị Lớp 2."))

    # ===================== HÀNG 2 — Lớp 1 ====================================
    P.append(panel(nxt(), "bargauge", "Lớp 1 — Nhiệt độ từng cell ngay lúc này", 0, 5, 10, 9,
        q(f'''from(bucket: "{BUCKET}")
  |> range(start: -6h)
  |> filter(fn: (r) => r._measurement == "cell" and r._field == "value" and r.tag =~ /^Cell0[1-8]_Temp$/)
  |> last()
  |> keep(columns: ["_value","tag"])'''),
        {"displayMode": "gradient", "orientation": "horizontal",
         "reduceOptions": {"calcs": ["lastNotNull"], "fields": "", "values": True},
         "showUnfilled": True, "minVizHeight": 16},
        {"defaults": {"unit": "celsius", "decimals": 2, "min": 15, "max": 65,
                      "thresholds": thresholds([
                          {"color": "green", "value": None},
                          {"color": "yellow", "value": 45},
                          {"color": "orange", "value": 55},
                          {"color": "red", "value": T_CRIT}])},
         "overrides": []},
        "So sánh KHÔNG GIAN giữa 8 cell tại một thời điểm — cell nào khác phần "
        "còn lại thì thấy ngay, không phải đọc 8 đường chồng nhau."))

    P.append(panel(nxt(), "timeseries", "Lớp 1 — Điểm bất thường từng cell", 10, 5, 14, 9,
        series("/^AI_Score0[1-8]$/"),
        {"legend": {"displayMode": "list", "placement": "bottom", "showLegend": True},
         "tooltip": {"mode": "multi", "sort": "desc"}},
        {"defaults": {"unit": "none", "decimals": 3,
                      "custom": {"lineWidth": 2, "fillOpacity": 0,
                                 "thresholdsStyle": {"mode": "line"}},
                      "thresholds": thresholds([
                          {"color": "text", "value": None},
                          {"color": "red", "value": AE_TH}])},
         "overrides": []},
        f"Đường đỏ = ngưỡng {AE_TH}. Vượt ngưỡng phải GIỮ LIÊN TỤC 60 giây mới "
        "thành báo động — chống báo động giả do nhiễu."))

    # ===================== HÀNG 3 — Lớp 2 ====================================
    P.append(panel(nxt(), "gauge", "Lớp 2 — SOH (sức khoẻ pack)", 0, 14, 6, 7,
        last("SOH_Percent"),
        {"reduceOptions": {"calcs": ["lastNotNull"], "fields": "", "values": False},
         "showThresholdLabels": False, "showThresholdMarkers": True},
        {"defaults": {"unit": "percent", "min": 60, "max": 100, "decimals": 1,
                      "thresholds": thresholds([
                          {"color": "red", "value": None},
                          {"color": "orange", "value": 80},
                          {"color": "green", "value": 90}])},
         "overrides": []},
        "SOH ở mức PACK, không phải từng cell — pack nối tiếp sống chết theo "
        "cell yếu nhất (QĐ-027). 80 % là mốc thường dùng để coi là hết đời xe."))

    P.append(panel(nxt(), "timeseries", "Lớp 2 — RUL: số chu kỳ còn lại", 6, 14, 12, 7,
        series("/^RUL_Cycles$/"),
        {"legend": {"showLegend": False}, "tooltip": {"mode": "single"}},
        {"defaults": {"unit": "none", "decimals": 0,
                      "custom": {"lineWidth": 2, "fillOpacity": 10}},
         "overrides": []},
        "Hồi quy tuyến tính trên t_cv (thời gian ở giai đoạn điện áp không đổi). "
        "Nội trở tăng => vào CV sớm hơn => t_cv dài ra."))

    P.append(panel(nxt(), "stat", "Mô hình có đang NGOẠI SUY?", 18, 14, 6, 7,
        last("Extrapolating"), stat_opts(24),
        {"defaults": {
            "mappings": [{"type": "value", "options": {
                "0": {"text": "TRONG VÙNG\nĐÃ HUẤN LUYỆN", "color": "green", "index": 0},
                "1": {"text": "ĐANG NGOẠI SUY\nKHÔNG ĐÁNG TIN", "color": "red", "index": 1}}}],
            "noValue": "—",
            "thresholds": thresholds([{"color": "text", "value": None}])},
         "overrides": []},
        "Mô hình tuyến tính ngoài vùng dữ liệu huấn luyện thì con số RUL vô "
        "nghĩa. Thà nói ra còn hơn im lặng đưa số đẹp."))

    # ===================== HÀNG 4 — độ tin cậy ===============================
    P.append(panel(nxt(), "timeseries", "Nhiệt độ 8 cell theo thời gian", 0, 21, 12, 8,
        series("/_Temp$/"),
        {"legend": {"displayMode": "list", "placement": "bottom", "showLegend": True},
         "tooltip": {"mode": "multi", "sort": "desc"}},
        {"defaults": {"unit": "celsius", "decimals": 2,
                      "custom": {"lineWidth": 1, "fillOpacity": 0,
                                 "thresholdsStyle": {"mode": "line"}},
                      "thresholds": thresholds([
                          {"color": "text", "value": None},
                          {"color": "red", "value": T_CRIT}])},
         "overrides": []},
        "Gồm cả Ambient_Temp (cảm biến thứ 9, đo môi trường — KHÔNG dán lên cell)."))

    P.append(panel(nxt(), "timeseries", "Tỉ lệ lỗi bus 1-Wire (%)", 12, 21, 7, 8,
        series("/^Sensor_ErrPct$/"),
        {"legend": {"showLegend": False}, "tooltip": {"mode": "single"}},
        {"defaults": {"unit": "percent", "decimals": 3,
                      "custom": {"lineWidth": 2, "fillOpacity": 20},
                      "thresholds": thresholds([
                          {"color": "green", "value": None},
                          {"color": "orange", "value": 1},
                          {"color": "red", "value": 5}])},
         "overrides": []},
        "Đã từng đo được 35 % lỗi do ẩm trên đầu dò mà không có dấu hiệu gì "
        "khác (QĐ-024). Đây là lý do phải đếm và hiển thị."))

    P.append(panel(nxt(), "stat", "Số lần báo động\ntừ lúc bật máy", 19, 21, 5, 8,
        last("Alarm_Events"), stat_opts(48),
        {"defaults": {"unit": "none", "noValue": "0",
                      "thresholds": thresholds([
                          {"color": "green", "value": None},
                          {"color": "orange", "value": 1}])},
         "overrides": []},
        "Cách duy nhất biết đêm qua có chuyện gì xảy ra mà không ai ở đó nghe còi."))

    dash = {
        "uid": "hutieu-pin",
        "title": "Hủ Tiếu — Giám sát pack pin 8S",
        "tags": ["hutieu", "pin"],
        "timezone": "browser",
        "schemaVersion": 39,
        "refresh": "5s",
        "time": {"from": "now-30m", "to": "now"},
        "panels": P,
    }

    out = HERE / "dashboards" / "hutieu-pin.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(dash, ensure_ascii=False, indent=2))
    print(f"da ghi {out}  ({len(P)} panel)")

    # Nạp thẳng lên Grafana để thấy ngay, không phải khởi động lại container.
    env = dict(l.split("=", 1) for l in ENV.read_text().splitlines()
               if "=" in l and not l.strip().startswith("#"))
    user, pw = env["GRAFANA_USER"], env["GRAFANA_PASSWORD"]
    body = json.dumps({"dashboard": dash, "overwrite": True,
                       "message": "sinh tu make_dashboard.py"}).encode()
    req = urllib.request.Request("http://127.0.0.1:3000/api/dashboards/db", data=body,
                                 method="POST")
    req.add_header("Content-Type", "application/json")
    req.add_header("Authorization", "Basic " +
                   base64.b64encode(f"{user}:{pw}".encode()).decode())
    with urllib.request.urlopen(req) as r:
        print("Grafana:", json.load(r).get("status"))


if __name__ == "__main__":
    main()
