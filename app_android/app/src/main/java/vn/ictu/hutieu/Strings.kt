package vn.ictu.hutieu

/* =============================================================================
 *  Song ngữ Việt / Anh (QĐ-046)
 *
 *  HAI LOẠI CHỮ, XỬ LÝ KHÁC NHAU — đây là chỗ dễ nhầm nhất của file này.
 *
 *  1. Chữ của APP (nhãn, nút, thông báo lỗi): nằm ở `L` dưới đây, dịch trọn vẹn.
 *
 *  2. Chữ do CHIP gửi qua BLE: firmware nói tiếng Việt không dấu. App KHÔNG
 *     dịch lại được một cách chắc chắn — nó chỉ thay được những cụm đã biết,
 *     bằng `translateFromChip()`.
 *
 *  Vì sao chấp nhận cách vá này thay vì làm cho đúng: làm đúng nghĩa là firmware
 *  gửi dữ liệu SỐ và app tự dựng câu. Đó là việc đáng làm, nhưng nó đụng vào
 *  ble_view.cpp, app, bản web VÀ dashboard cùng lúc — không phải thứ nên làm
 *  bốn ngày trước Bán kết.
 *
 *  ⚠️ HẠN CHẾ PHẢI BIẾT: firmware đổi một chữ trong câu là bản dịch ngừng khớp
 *  và người dùng thấy tiếng Việt lẫn trong bản tiếng Anh. Nó KHÔNG gây lỗi, chỉ
 *  xấu. Riêng băng trạng thái trên cùng thì miễn nhiễm — nó dựng từ con số
 *  `lvl` trong đặc tính máy đọc 000A, không dò chữ (xem MainActivity).
 * ========================================================================== */

enum class Lang { VI, EN }

object L {
    var cur = Lang.VI

    private val T = mapOf(
        "title"        to ("Pack pin Hủ Tiếu"            to "Hu Tieu Battery Pack"),
        "connect"      to ("Kết nối tới pack"            to "Connect to pack"),
        "disconnect"   to ("Ngắt kết nối"                to "Disconnect"),
        "notConnected" to ("Chưa kết nối"                to "Not connected"),
        "searching"    to ("Đang tìm pack…"              to "Searching for pack…"),
        "connecting"   to ("Đang kết nối…"               to "Connecting…"),
        "reading"      to ("Đã kết nối, đang đọc…"       to "Connected, reading…"),
        "bonded"       to ("Đã ghép đôi — đang nối thẳng…" to "Already paired — connecting directly…"),
        "lost"         to ("Mất kết nối — số đang hiện KHÔNG còn mới"
                        to "Disconnected — the values shown are NO LONGER current"),
        "stale"        to ("Không nhận được dữ liệu — số đang hiện đã cũ"
                        to "No data received — the values shown are stale"),
        "notFound"     to ("Không thấy pack. Pack có đang bật không?"
                        to "Pack not found. Is it powered on?"),
        "btOff"        to ("Hãy bật Bluetooth rồi thử lại"  to "Turn on Bluetooth and try again"),
        "noPerm"       to ("Thiếu quyền Bluetooth"          to "Bluetooth permission missing"),
        "noSvc"        to ("Thiết bị không có dịch vụ của pack"
                        to "Device does not expose the pack service"),

        "rowState"     to ("Trạng thái"                   to "Status"),
        "rowAi"        to ("Lớp 1 — cell nghi ngờ"        to "Layer 1 — suspect cell"),
        "rowTemps"     to ("Nhiệt độ từng cell"           to "Per-cell temperature"),
        "rowDiag"      to ("Số đo đã dùng để chẩn đoán"   to "Measurements behind the diagnosis"),
        "rowPack"      to ("Điện áp / dòng / SoC"         to "Voltage / current / SoC"),
        "rowL2"        to ("Lớp 2 — sức khoẻ và tuổi thọ" to "Layer 2 — health and remaining life"),
        "rowSys"       to ("Tình trạng hệ thống"          to "System status"),

        "lvl0"         to ("BÌNH THƯỜNG"                  to "NORMAL"),
        "lvl1"         to ("THEO DÕI — AI thấy lệch"      to "WATCH — AI sees a deviation"),
        "lvl2"         to ("BÁO ĐỘNG — cell bất thường"   to "ALARM — abnormal cell"),
        "lvl3"         to ("NGUY KỊCH — vượt ngưỡng cứng" to "CRITICAL — hard limit exceeded"),

        "buzzOn"       to ("Còi: ĐANG BẬT — chạm để tắt"  to "Buzzer: ON — tap to mute"),
        "buzzOff"      to ("Còi: ĐÃ TẮT"                  to "Buzzer: MUTED"),
        "buzzBack"     to ("còi tự kêu lại sau"           to "unmutes automatically in"),
        "tapUnmute"    to ("chạm để bật lại"              to "tap to unmute"),
        "quietOn"      to ("Bíp thưa: ĐANG BẬT — chạm để tắt" to "Sparse beep: ON — tap to disable"),
        "quietOff"     to ("Bíp thưa: TẮT — chạm để bật"  to "Sparse beep: OFF — tap to enable"),

        "sent"         to ("Đã gửi lệnh, đang chờ pack xác nhận…"
                        to "Command sent, waiting for the pack to confirm…"),
        "needPin"      to ("Cần ghép đôi: nhập PIN rồi bấm lại"
                        to "Pairing required: enter the PIN, then tap again"),
        "staleGatt"    to ("Điện thoại đang dùng bảng cũ của thiết bị. Quên ghép đôi HuTieu-BMS trong Cài đặt rồi nối lại."
                        to "Your phone is using a stale device profile. Forget HuTieu-BMS in Settings, then reconnect."),
        "sendFail"     to ("Gửi lệnh thất bại (mã %d)"    to "Command failed (code %d)"),

        "note"         to ("Lần bấm đầu tiên máy sẽ hỏi ghép đôi và mã PIN — đó là cố ý: người đi ngang qua không tắt được còi."
                        to "The first tap asks for pairing and a PIN — deliberately: a passer-by must not be able to silence the alarm."),
        "noteCrit"     to ("Ở mức NGUY KỊCH, tắt tiếng tự hết hạn sau 2 phút. Đèn đỏ và dữ liệu lên cloud không bao giờ bị tắt."
                        to "At CRITICAL level the mute expires after 2 minutes. The red light and the cloud feed are never silenced.")
    )

    operator fun get(k: String): String {
        val p = T[k] ?: return k
        return if (cur == Lang.VI) p.first else p.second
    }

    fun level(lvl: Int) = get("lvl" + lvl.coerceIn(0, 3))

    /* Thay những cụm đã biết trong chuỗi do chip gửi. Xếp cụm DÀI trước cụm
       NGẮN: "chu ky sac" phải được thay trước "chu ky", không thì còn lại một
       mảnh cụt. */
    private val CHIP = listOf(
        "NGOAI DAI HUAN LUYEN" to "OUTSIDE TRAINING RANGE",
        "dac trung"      to "features",
        "khong co cell bat thuong" to "no abnormal cell",
        "cao nhat"       to "max",
        "AI dang khoi dong, chua ket luan" to "AI is warming up, no conclusion yet",
        "chua co chu ky sac nao duoc ghi nhan" to "no charge cycle recorded yet",
        "chua co so"     to "no data",
        "dang vuot"      to "is exceeding",
        "moi giu"        to "held for only",
        "cam bien"       to "sensors",
        "chua do duoc (khong thay INA)" to "not measurable (no INA found)",
        "nhanh hon pack" to "faster than pack",
        "dot ngot"       to "sudden",
        "xu huong"       to "trend",
        "%/chu ky"       to "%/cycle",   // số ít: phải đứng TRƯỚC luật "chu ky"
        "da ghi"         to "logged",
        "chu ky"         to "cycles",
        "con ~"          to "~",
        "lech"           to "offset",
        "diem"           to "score",
        "giu"            to "held",
        "chay"           to "up",
        "phut"           to "min",
        "mat"            to "lost",
        "CELL"           to "CELL"
    )

    fun translateFromChip(s: String): String {
        if (cur == Lang.VI) return s
        var r = s
        for ((vi, en) in CHIP) r = r.replace(vi, en)
        return r
    }
}
