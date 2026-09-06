// Nội dung của node "function" trong Node-RED.
// File này để bạn đọc/sửa cho dễ; bản chạy thật nằm trong flow_wisepaas_to_influx.json.
//
// Vào : {"d":{"BatteryPack01":{"Cell01_Temp":30.42}},"ts":"2026-09-06T09:15:22.410000Z"}
// Ra  : cell,device=BatteryPack01,tag=Cell01_Temp value=30.42 1757150122410000000

const p = msg.payload;
if (p === null || typeof p !== "object" || typeof p.d !== "object") { return null; }

let ms = Date.parse(p.ts);
if (isNaN(ms)) { ms = Date.now(); }

// Chan som goi co timestamp vo ly. Truong hop hay gap nhat: NTP tren ESP32 fail
// -> ts ra nam 1970 -> Influx tra HTTP 422 (ngoai retention 90d) va vut du lieu di.
// Bat o day de bao dung nguyen nhan, thay vi de no chet lang le o buoc ghi.
if (ms < 1700000000000) {
    node.warn("Bo goi: ts=" + p.ts + " (nam 1970?). NTP tren ESP32 chua dong bo.");
    return null;
}

const tsNs = String(ms) + "000000";   // ms -> ns

const esc = (s) => String(s).replace(/([ ,=])/g, "\\$1");
const lines = [];

for (const dev of Object.keys(p.d)) {
    const tags = p.d[dev];
    // bỏ qua gói trạng thái {"d":{"Con":1}} vì giá trị không phải object
    if (tags === null || typeof tags !== "object") { continue; }
    for (const t of Object.keys(tags)) {
        const v = tags[t];
        if (typeof v !== "number" || isFinite(v) === false) { continue; }
        lines.push("cell,device=" + esc(dev) + ",tag=" + esc(t) + " value=" + v + " " + tsNs);
    }
}

if (lines.length === 0) { return null; }

msg.payload = lines.join("\n");
msg.headers = {
    "Authorization": "Token " + (env.get("INFLUX_TOKEN") || ""),
    "Content-Type": "text/plain; charset=utf-8"
};
return msg;
