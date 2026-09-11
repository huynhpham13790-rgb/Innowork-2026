// Lớp 2 — tính RUL và SOH từ gói tóm tắt chu kỳ sạc.
//
// Chỉ là tích vô hướng, nên không cần dịch vụ Python riêng: hệ số nhúng thẳng
// vào đây. Sinh bởi ai/export_rul_model.py — ĐỪNG sửa số bằng tay, chạy lại
// script đó rồi dán lại cả khối MODEL.
//
// Đầu vào : gói MQTT có các tag CYC_* (9 đặc trưng pha sạc)
// Đầu ra  : line protocol ghi RUL_Cycles, SOH_Percent, EOL_Cycle vào InfluxDB
//
// SAI SỐ KỲ VỌNG (leave-one-battery-out trên 4 pin NASA):
//   RUL ±12.07 chu kỳ  ·  SOH ±4,3 điểm phần trăm
// Xem docs/BANG_CHUNG_LOP2_RUL_2026-09-11.md trước khi trích số lên slide.

const MODEL = { "_comment": "Sinh b\u1edfi ai/export_rul_model.py. Xem docs/BANG_CHUNG_LOP2_RUL_2026-09-11.md", "features": [ "t_v_interval", "t_cv", "i_cv_mean", "t_charge_total", "T_max_ch", "T_mean_ch", "T_rise_ch", "v_start", "dvdt_cc" ], "mean": [ 2074.8475247252745, 2813.042972527473, 0.6288362623103789, 5566.0271043956045, 30.108611603634806, 26.71054260591287, 2.2711164229142002, 3.615197397546694, 0.00011190709037515129 ], "std": [ 289.1027484967059, 171.6792380127685, 0.02205949216824044, 358.4939484415254, 1.9361973580871237, 0.7524074021411363, 1.5860721496998074, 0.15993105141592168, 2.4287009487753285e-05 ], "rul": { "feature": "t_cv", "coef": -26.962900606205096, "intercept": 48.304945054944994 }, "soh": { "coef": [ -0.01955931644364552, -0.051809110979603584, 0.010091031831626737, 0.07254137300890526, 0.0036991655452140956, -0.008248802943374512, 0.004887006816375613, 0.003572543594894502, 0.040717757214034624 ], "intercept": 0.9126275109872223 }, "eol_frac": 0.8, "trained_on": { "dataset": "NASA PCoE", "batteries": [ "B0005", "B0006", "B0007", "B0018" ], "cycles": 364 }, "expected_error": { "protocol": "leave-one-battery-out, 4 pin NASA", "rul_mae_cycles": 12.07, "rul_early_mae_cycles": 9.46, "rul_rmse_cycles": 14.95 } };

const p = msg.payload;
if (p === null || typeof p !== "object" || typeof p.d !== "object") { return null; }

let ms = Date.parse(p.ts);
if (isNaN(ms) || ms < 1700000000000) { return null; }
const tsNs = String(ms) + "000000";

const lines = [];
for (const dev of Object.keys(p.d)) {
    const tags = p.d[dev];
    if (tags === null || typeof tags !== "object") { continue; }

    // Chỉ xử lý gói tóm tắt chu kỳ sạc; gói nhiệt độ thường thì bỏ qua.
    const feats = [];
    let ok = true;
    for (const name of MODEL.features) {
        const v = tags["CYC_" + name];
        if (typeof v !== "number" || !isFinite(v)) { ok = false; break; }
        feats.push(v);
    }
    if (!ok) { continue; }

    // chuẩn hoá đúng như lúc train
    const z = feats.map((v, i) => (v - MODEL.mean[i]) / MODEL.std[i]);

    const icv = MODEL.features.indexOf(MODEL.rul.feature);

    // Cảnh báo NGOẠI SUY: mô hình tuyến tính vẫn trả về số khi đầu vào nằm
    // ngoài dải đã học, và con số đó trông rất hợp lý nhưng không có căn cứ.
    // |z| > 3 nghĩa là cách trung bình huấn luyện hơn 3 độ lệch chuẩn.
    // Không có kiểm tra này thì lỗi hiệu chỉnh chỉ hiện ra dưới dạng "RUL = 0",
    // rất dễ tưởng là pin hỏng thật.
    const outliers = MODEL.features.filter((n, i) => Math.abs(z[i]) > 3);
    if (outliers.length) {
        node.warn("Lop 2: dac trung NGOAI DAI huan luyen (" + outliers.join(", ")
                  + ") -> ket qua khong dang tin");
    }

    let rul = MODEL.rul.coef * z[icv] + MODEL.rul.intercept;
    const rul_raw = rul;
    if (rul < 0) { rul = 0; }   // đã quá hạn thì báo 0, không báo số âm

    let soh = MODEL.soh.intercept;
    for (let i = 0; i < z.length; i++) { soh += MODEL.soh.coef[i] * z[i]; }
    soh = Math.min(Math.max(soh, 0), 1.2);   // chặn giá trị vô lý

    const esc = (s) => String(s).replace(/([ ,=])/g, "\\$1");
    const d = esc(dev);
    lines.push("rul,device=" + d + ",tag=RUL_Cycles value=" + rul.toFixed(2) + " " + tsNs);
    lines.push("rul,device=" + d + ",tag=SOH_Percent value=" + (soh * 100).toFixed(2) + " " + tsNs);
    // Ghi cả giá trị THÔ chưa chặn, để trên dashboard nhìn ra được lúc nào mô
    // hình đang ngoại suy thay vì chỉ thấy một đường 0 phẳng lì.
    lines.push("rul,device=" + d + ",tag=RUL_Raw value=" + rul_raw.toFixed(2) + " " + tsNs);
    lines.push("rul,device=" + d + ",tag=Extrapolating value=" + (outliers.length ? 1 : 0) + " " + tsNs);

    node.status({ fill: "blue", shape: "dot",
                  text: "RUL " + rul.toFixed(0) + " chu ky, SOH " + (soh*100).toFixed(1) + "%" });
    node.warn("Lop 2: RUL=" + rul.toFixed(1) + " chu ky, SOH=" + (soh*100).toFixed(1) + "%");
}

if (lines.length === 0) { return null; }

msg.payload = lines.join("\n");
msg.headers = {
    "Authorization": "Token " + (env.get("INFLUX_TOKEN") || ""),
    "Content-Type": "text/plain; charset=utf-8"
};
return msg;
