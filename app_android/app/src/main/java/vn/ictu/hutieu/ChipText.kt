package vn.ictu.hutieu

import android.graphics.Color
import android.graphics.Typeface
import android.text.SpannableStringBuilder
import android.text.Spanned
import android.text.style.ForegroundColorSpan
import android.text.style.RelativeSizeSpan
import android.text.style.StyleSpan

/* =============================================================================
 *  Dựng lại chữ của chip thành câu dễ đọc, có dấu, đúng ngôn ngữ đang chọn.
 *
 *  VÌ SAO KHÔNG DỊCH TỪNG CỤM NHƯ translateFromChip() NỮA:
 *  Chip gửi tiếng Việt KHÔNG DẤU ("nong hon nhung on dinh (TH-2)") và bảng
 *  thay cụm chỉ dịch được cụm nó biết — tên dạng TH-1/2/3 và lời khuyên của
 *  Lớp 1 lọt qua nguyên văn, nên bản tiếng Anh vẫn lẫn tiếng Việt (thấy trên
 *  máy thật 23/09). Ở đây app BÓC CON SỐ ra khỏi chuỗi bằng mẫu cố định rồi tự
 *  viết câu, nên cả hai ngôn ngữ đều có dấu và xuống dòng rõ ràng.
 *
 *  Mẫu chuỗi lấy nguyên từ ble_view.cpp. Firmware đổi định dạng thì mẫu không
 *  khớp và app RƠI VỀ cách cũ (translateFromChip) — chữ xấu đi nhưng KHÔNG mất
 *  số liệu. Không bao giờ được để hàm ở đây trả về rỗng khi không khớp.
 * ========================================================================== */
object ChipText {

    private fun vi() = L.cur == Lang.VI
    private fun t(v: String, e: String) = if (vi()) v else e

    // Dạng bất thường: khoá theo tên chip gửi (cell_ai.cpp aiPatternName).
    private enum class Pat(val chip: String, val color: String) {
        FAST("NONG LEN NHANH (TH-1)", "#ff8a8a"),
        WARM("nong hon nhung on dinh (TH-2)", "#ffd08a"),
        COLD("LANH bat thuong (TH-3)", "#8ac8ff"),
        NONE("chua ro dang", "#b8c0cc");
    }

    private fun pat(s: String) = Pat.values().firstOrNull { s.trim() == it.chip }

    private fun patName(p: Pat) = when (p) {
        Pat.FAST -> t("TH-1 · Nóng lên NHANH hơn pack", "Case 1 · Heating FASTER than the pack")
        Pat.WARM -> t("TH-2 · Nóng hơn nhưng ổn định", "Case 2 · Warmer but stable")
        Pat.COLD -> t("TH-3 · LẠNH bất thường", "Case 3 · Abnormally COLD")
        Pat.NONE -> t("Chưa rõ dạng", "Pattern unclear")
    }

    // Lời khuyên lấy nguyên thứ tự từ aiPatternAction(): ngắt sạc TRƯỚC.
    private fun patAction(p: Pat) = when (p) {
        Pat.FAST -> t("Ngắt sạc → ngắt tải → đưa pack ra chỗ thoáng",
                      "Stop charging → cut the load → move the pack to open air")
        Pat.WARM -> t("Chưa khẩn cấp. Ghi lại, kiểm tra ở lần bảo dưỡng",
                      "Not urgent. Log it and check at the next service")
        Pat.COLD -> t("Kiểm tra mối nối cell và xem cảm biến còn dán chặt không",
                      "Check the cell's connection and whether the sensor is still attached")
        Pat.NONE -> t("Tiếp tục theo dõi", "Keep watching")
    }

    private val R_ALARM = Regex("""CELL (\d+) - (.+?) \| .+? \| diem ([\d.]+)/([\d.]+), giu (\d+)s""")
    private val R_RISE  = Regex("""cell (\d+) dang vuot \(([\d.]+)\) - (.+?) - moi giu (\d+)s/(\d+)s""")
    private val R_OK    = Regex("""khong co cell bat thuong \(cao nhat ([\d.]+) / ([\d.]+)\)""")

    /** Ô Lớp 1. */
    fun ai(raw: String): CharSequence {
        if (raw.startsWith("AI dang khoi dong"))
            return dim(t("AI đang khởi động — chưa đủ dữ liệu để kết luận",
                         "AI is warming up — not enough data to conclude yet"))

        R_ALARM.find(raw)?.let { m ->
            val (cell, p, sc, th, held) = m.destructured
            val pt = pat(p) ?: return L.translateFromChip(raw)
            return SpannableStringBuilder()
                .head(t("Cell $cell", "Cell $cell") + " — " + patName(pt), pt.color)
                .append("\n").bold("➜ " + patAction(pt))
                .append("\n").dimLine(t("Điểm $sc (ngưỡng $th) · đã giữ ${held}s",
                                        "Score $sc (threshold $th) · held ${held}s"))
        }
        R_RISE.find(raw)?.let { m ->
            val (cell, sc, p, held, need) = m.destructured
            val pt = pat(p) ?: Pat.NONE
            return SpannableStringBuilder()
                .head(t("Cell $cell đang lệch", "Cell $cell is deviating") + " — " + patName(pt), "#ffd08a")
                .append("\n").dimLine(t("Điểm $sc · đã giữ ${held}/${need}s — đủ ${need}s mới báo động",
                                        "Score $sc · held ${held}/${need}s — alarms after ${need}s"))
        }
        R_OK.find(raw)?.let { m ->
            val (sc, th) = m.destructured
            return SpannableStringBuilder()
                .head(t("Không có cell bất thường", "No abnormal cell"), "#8fe3b0")
                .append("\n").dimLine(t("Điểm cao nhất $sc / ngưỡng $th", "Highest score $sc / threshold $th"))
        }
        return L.translateFromChip(raw)
    }

    private val R_DIAG = Regex("""lech ([-+\d.]+) degC \| nhanh hon pack ([-+\d.]+) degC/phut \| dot ngot ([-+\d.]+) degC""")

    /** Ba số đã dùng để tra dạng — mỗi số một dòng, kèm nghĩa. */
    fun diag(raw: String): CharSequence {
        if (raw.startsWith("chua co so")) return dim(t("Chưa có số (AI đang khởi động)", "No data yet (AI warming up)"))
        val m = R_DIAG.find(raw) ?: return L.translateFromChip(raw)
        val (dev, dtd, sh) = m.destructured
        return SpannableStringBuilder()
            .row(t("Lệch so với pack", "Offset from pack"), "$dev °C")
            .append("\n").row(t("Nóng nhanh hơn pack", "Heating faster than pack"), "$dtd °C/${t("phút", "min")}")
            .append("\n").row(t("Lệch đột ngột", "Sudden jump"), "$sh °C")
    }

    private val R_PACK = Regex("""([-\d.]+) V\s+([-\d.]+) A\s+SoC ~(\d+)%""")

    fun pack(raw: String): CharSequence {
        if (raw.startsWith("chua do duoc"))
            return dim(t("Chưa đo được (không thấy mạch INA)", "Not measurable (no INA sensor found)"))
        val m = R_PACK.find(raw) ?: return L.translateFromChip(raw)
        val (v, a, soc) = m.destructured
        return SpannableStringBuilder()
            .row(t("Điện áp", "Voltage"), "$v V")
            .append("\n").row(t("Dòng", "Current"), "$a A")
            .append("\n").row(t("Dung lượng (SoC)", "Charge (SoC)"), "~$soc %")
    }

    private val R_SYS = Regex("""cam bien (\d+)/(\d+)\s+wifi (\S+)\s+cloud (\S+)\s+chay (\d+) phut""")

    fun sys(raw: String): CharSequence {
        val m = R_SYS.find(raw) ?: return L.translateFromChip(raw)
        val (ok, n, wifi, cloud, up) = m.destructured
        fun st(s: String) = if (s == "OK") "✓ OK" else "✗ " + t("mất", "lost")
        return SpannableStringBuilder()
            .row(t("Cảm biến", "Sensors"), "$ok/$n" + if (ok == n) " ✓" else " ⚠")
            .append("\n").row("Wi-Fi", st(wifi))
            .append("\n").row("Cloud", st(cloud))
            .append("\n").row(t("Đã chạy", "Uptime"), "$up " + t("phút", "min"))
    }

    private val R_L2    = Regex("""SOH ([\d.]+)% \| con ~(\d+) chu ky \| da ghi (\d+) chu ky""")
    private val R_TREND = Regex("""xu huong ([-+\d.]+) %/chu ky""")
    private val R_OOD   = Regex("""NGOAI DAI HUAN LUYEN \((\d+)/(\d+) dac trung\)""")

    fun l2(raw: String): CharSequence {
        if (raw.startsWith("chua co chu ky"))
            return dim(t("Chưa ghi nhận chu kỳ sạc nào", "No charge cycle recorded yet"))
        val m = R_L2.find(raw) ?: return L.translateFromChip(raw)
        val (soh, rul, cyc) = m.destructured
        val b = SpannableStringBuilder()
        /* Cảnh báo ngoại suy đặt LÊN ĐẦU, không phải chú thích cuối: con số RUL
           bên dưới chỉ có nghĩa nếu người đọc thấy câu này trước. */
        R_OOD.find(raw)?.let { o ->
            val (k, n) = o.destructured
            b.head("⚠ " + t("Ngoài dải huấn luyện ($k/$n đặc trưng) — số dưới chưa đáng tin",
                            "Outside training range ($k/$n features) — numbers below are not reliable"),
                   "#ffd08a").append("\n")
        }
        b.row(t("Sức khoẻ (SOH)", "Health (SOH)"), "$soh %")
            .append("\n").row(t("Còn lại", "Remaining"), "~$rul " + t("chu kỳ", "cycles"))
            .append("\n").row(t("Đã ghi", "Logged"), "$cyc " + t("chu kỳ", "cycles"))
        R_TREND.find(raw)?.let { b.append("\n").row(t("Xu hướng", "Trend"), it.groupValues[1] + " %/" + t("chu kỳ", "cycle")) }
        return b
    }

    // ------------------------------------------------------------ định dạng
    private fun SpannableStringBuilder.span(s: String, vararg what: Any): SpannableStringBuilder {
        val a = length; append(s)
        for (w in what) setSpan(w, a, length, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE)
        return this
    }
    private fun SpannableStringBuilder.head(s: String, color: String) =
        span(s, StyleSpan(Typeface.BOLD), ForegroundColorSpan(Color.parseColor(color)), RelativeSizeSpan(1.08f))
    private fun SpannableStringBuilder.bold(s: String) = span(s, StyleSpan(Typeface.BOLD))
    private fun SpannableStringBuilder.dimLine(s: String) =
        span(s, ForegroundColorSpan(Color.parseColor("#9aa4b4")), RelativeSizeSpan(0.9f))
    /** "Nhãn   giá trị" — nhãn mờ, giá trị đậm: mắt tìm số, không tìm chữ. */
    private fun SpannableStringBuilder.row(k: String, v: String) =
        span("$k  ", ForegroundColorSpan(Color.parseColor("#9aa4b4"))).bold(v)
    private fun dim(s: String): CharSequence = SpannableStringBuilder().dimLine(s)
}
