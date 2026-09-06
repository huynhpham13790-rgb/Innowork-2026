// Kiem tra ket qua ghi InfluxDB. Khong co node nay thi loi ghi HOAN TOAN im lang:
// Influx tra 401 (sai token) hoac 422 (ts ngoai retention, vd NTP fail -> nam 1970),
// Node-RED khong log gi, container van "running", Grafana chi don gian la trong.
const code = msg.statusCode;

if (code === 204) {
    node.status({ fill: "green", shape: "dot", text: "OK " + new Date().toLocaleTimeString() });
    return null;                     // ghi thanh cong -> khong can bao gi them
}

let hint = "";
if (code === 401) { hint = " -> SAI INFLUX_TOKEN (kiem tra .env va restart nodered)"; }
if (code === 422) { hint = " -> ts ngoai khoang retention 90d. NTP tren ESP32 fail? (ts nam 1970)"; }
if (code === 400) { hint = " -> line protocol sai cu phap"; }

const detail = typeof msg.payload === "string" ? msg.payload.slice(0, 300) : "";
node.status({ fill: "red", shape: "ring", text: "LOI " + code });
node.error("Influx ghi that bai HTTP " + code + hint + "  " + detail, msg);
return msg;
