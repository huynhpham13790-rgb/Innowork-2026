#!/usr/bin/env python3
"""
Seed lịch sử Lớp 2 cho demo — phát lại chu kỳ sạc THẬT của pin NASA B0005.

VÌ SAO CẦN: chưa có INA228 nên chip chưa đo được chu kỳ sạc thật. Lớp 2 chỉ có
nghĩa khi nhìn thấy XU HƯỚNG qua nhiều chu kỳ (SOH giảm dần, RUL giảm dần), mà
chip mô phỏng mỗi 45 s thì giám khảo chỉ thấy vài điểm trong buổi demo.

VÌ SAO PHÁT LẠI QUA MQTT CHỨ KHÔNG GHI THẲNG SOH/RUL VÀO DB:
Script này KHÔNG bịa SOH hay RUL. Nó chỉ gửi 9 đặc trưng pha sạc đo thật trong
bộ NASA, đúng định dạng chip gửi (docs/DATA_CONTRACT.md), rồi để CHÍNH Node-RED
chạy CHÍNH mô hình (rul_predict.js) tính ra kết quả. Tức là trên dashboard là
đầu ra thật của mô hình trên dữ liệu thật — chỉ có THỜI ĐIỂM là dàn dựng.

PHẢI NÓI VỚI GIÁM KHẢO: "lịch sử 30 ngày là 60 chu kỳ sạc của pin NASA B0005
phát lại qua đúng đường truyền; từ chu kỳ 61 trở đi là chip đang chạy". Gói
seed mang thêm tag SEED_NASA_Cycle để luôn lọc/xoá được, không lẫn vào dữ liệu thật.

Chạy:  python3 VEDCaPhenika/planb_cloud/seed_layer2.py [--clear]
  --clear  xoá kết quả Lớp 2 cũ (measurement rul + tag CYC_/ONB_/SEED_) trong
           45 ngày qua trước khi seed. Chỉ là dữ liệu MÔ PHỎNG — chip chưa từng
           đo chu kỳ thật — nhưng xoá rồi là không lấy lại được.
"""
import csv, json, subprocess, sys, urllib.request
from datetime import datetime, timedelta, timezone
from pathlib import Path

HERE = Path(__file__).parent
ROOT = HERE.parent.parent
ENV = dict(l.split("=", 1) for l in (HERE / ".env").read_text().splitlines()
           if "=" in l and not l.strip().startswith("#"))

NODE_ID = "DAN_NODE_ID_VAO_DAY"         # khớp NODE_ID trong firmware
DEVICE = "BatteryPack01"
FEATS = ["t_v_interval", "t_cv", "i_cv_mean", "t_charge_total", "T_max_ch",
         "T_mean_ch", "T_rise_ch", "v_start", "dvdt_cc"]
# Chu kỳ 0 nằm NGOÀI dải huấn luyện (4 đặc trưng |z| > 3) — bỏ, không thì điểm
# đầu tiên của đồ thị đã báo "ngoại suy". Dừng ở 60: SOH mô hình ~91 %, RUL
# ~55, vẫn trong dải; firmware mô phỏng (tau_cv = 1370) tiếp nối từ đó.
FIRST, LAST = 1, 60
SPAN_DAYS = 30


def clear():
    now = datetime.now(timezone.utc)
    start = (now - timedelta(days=45)).strftime("%Y-%m-%dT%H:%M:%SZ")
    stop = now.strftime("%Y-%m-%dT%H:%M:%SZ")
    for pred in ['_measurement="rul"'] + [
            f'_measurement="cell" AND "tag"="{t}"' for t in
            [f"CYC_{f}" for f in FEATS] + ["CYC_duration_s", "ONB_RUL_Cycles",
             "ONB_SOH_Percent", "ONB_Extrapolating", "ONB_SOH_Slope",
             "ONB_Cycles_Logged", "SEED_NASA_Cycle"]]:
        req = urllib.request.Request(
            "http://127.0.0.1:8086/api/v2/delete?org=hutieu&bucket=battery",
            data=json.dumps({"start": start, "stop": stop, "predicate": pred}).encode(),
            method="POST")
        req.add_header("Authorization", "Token " + ENV["INFLUX_TOKEN"])
        req.add_header("Content-Type", "application/json")
        # "tag" phải đặt trong ngoặc kép: không có thì InfluxDB báo
        # "bad logical expression" (đo 23/09).
        urllib.request.urlopen(req).read()
    print(f"da xoa ket qua Lop 2 tu {start}")


def main():
    if "--clear" in sys.argv:
        clear()
    rows = [r for r in csv.DictReader(open(ROOT / "ai/data/nasa_cycles.csv"))
            if r["battery"] == "B0005" and FIRST <= int(r["cycle_idx"]) <= LAST]
    # Kết thúc 1 giờ trước để điểm đầu tiên của chip nối tiếp ngay sau.
    end = datetime.now(timezone.utc) - timedelta(hours=1)
    step = timedelta(days=SPAN_DAYS) / (len(rows) - 1)
    lines = []
    for k, r in enumerate(rows):
        ts = end - step * (len(rows) - 1 - k)
        dev = {f"CYC_{f}": float(r[f]) for f in FEATS}   # không làm tròn: dvdt_cc ~1e-4
        dev["CYC_duration_s"] = round(float(r["t_charge_total"]))
        dev["SEED_NASA_Cycle"] = int(r["cycle_idx"])
        lines.append(json.dumps({"d": {DEVICE: dev},
                                 "ts": ts.strftime("%Y-%m-%dT%H:%M:%S.000Z")}))
    subprocess.run(["docker", "exec", "-i", "hutieu-mosquitto", "mosquitto_pub",
                    "-u", ENV["MQTT_USER"], "-P", ENV["MQTT_PASSWORD"],
                    "-t", f"/wisepaas/scada/{NODE_ID}/data", "-q", "1", "-l"],
                   input="\n".join(lines) + "\n", text=True, check=True)
    print(f"da gui {len(lines)} chu ky B0005 #{FIRST}..#{LAST}, "
          f"{SPAN_DAYS} ngay, ket thuc {end:%Y-%m-%d %H:%M} UTC")


if __name__ == "__main__":
    main()
