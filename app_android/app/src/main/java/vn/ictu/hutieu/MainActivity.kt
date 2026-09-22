package vn.ictu.hutieu

import android.Manifest
import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.content.pm.PackageManager
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.Gravity
import android.view.ViewGroup.LayoutParams.MATCH_PARENT
import android.view.ViewGroup.LayoutParams.WRAP_CONTENT
import android.widget.*
import java.util.ArrayDeque
import java.util.UUID

/* =============================================================================
 *  App xem pack pin qua BLE — bản Android thật (QĐ-044, QĐ-046)
 *
 *  VÌ SAO KHÔNG DÙNG BẢN WEB NỮA
 *  Bản Web Bluetooth (app/index.html) chạy đúng, nhưng phải TẢI VỀ từ một máy
 *  chủ trong mạng LAN. Trên Wi-Fi trường ICTU điện thoại không chạm được tới
 *  máy chủ (đo được: log máy chủ không hề có request nào từ điện thoại). App
 *  này cài một lần rồi không cần mạng nữa, kể cả lúc mở.
 * ========================================================================== */

private const val DEV_NAME = "HuTieu-BMS"
private fun u(x: String) = UUID.fromString("48555449-4555-4d53-$x-000000000000")

private val UUID_SVC   = u("0001")
private val UUID_STATE = u("0002")
private val UUID_TEMPS = u("0003")
private val UUID_AI    = u("0004")
private val UUID_PACK  = u("0005")
private val UUID_SYS   = u("0006")
private val UUID_DIAG  = u("0007")
private val UUID_CMD   = u("0008")
private val UUID_L2    = u("0009")
private val UUID_CTL   = u("000a")

private val CCCD = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")

/* Thứ tự hiện trên màn = thứ tự ƯU TIÊN ĐỌC của người đứng cạnh pack: cell nào
   hỏng trước, rồi số đo, rồi mới tới thông tin nền. Trạng thái và nhiệt độ
   không nằm đây vì chúng có ô riêng ở trên cùng. */
private val ROWS = listOf(
    UUID_AI   to "rowAi",
    UUID_DIAG to "rowDiag",
    UUID_PACK to "rowPack",
    UUID_L2   to "rowL2",
    UUID_SYS  to "rowSys"
)

// Bảng màu — một chỗ duy nhất, để đổi tông không phải lùng khắp file.
private const val C_BG     = "#0b0d12"
private const val C_CARD   = "#161a22"
private const val C_LABEL  = "#7d8798"
private const val C_TEXT   = "#eef1f6"
private const val C_OK     = "#1b6b45"
private const val C_WATCH  = "#8a6d1f"
private const val C_ALARM  = "#b3541e"
private const val C_CRIT   = "#9b1c1c"
private const val C_IDLE   = "#2b313d"

class MainActivity : android.app.Activity() {

    private var gatt: BluetoothGatt? = null
    private val ui = Handler(Looper.getMainLooper())
    private val views = HashMap<UUID, TextView>()
    private val labels = HashMap<UUID, TextView>()

    private lateinit var banner: TextView
    private lateinit var subBanner: TextView
    private lateinit var connectBtn: Button
    private lateinit var langBtn: Button
    private lateinit var muteBtn: Button
    private lateinit var quietBtn: Button
    private lateinit var titleView: TextView
    private lateinit var noteView: TextView
    private lateinit var tempRow: LinearLayout
    private lateinit var tempLabel: TextView
    private val cellTiles = ArrayList<Pair<TextView, TextView>>()   // (số, nhãn)

    private var lastPacketAt = 0L
    private var connected = false

    // Trạng thái điều khiển do CHIP báo về — nguồn sự thật duy nhất cho hai nút.
    private var stMuted = false
    private var stQuiet = false
    private var stLevel = 0
    private var stTtl = 0            // giây còn lại, app tự đếm lùi giữa hai notify

    // ------------------------------------------------------------- hàng đợi GATT
    /* Android chỉ cho MỘT thao tác GATT chạy tại một thời điểm. Gọi cái thứ hai
       khi cái thứ nhất chưa xong thì nó bị BỎ IM LẶNG: không lỗi, không ngoại
       lệ, callback không bao giờ tới. Bật notify cho 5 đặc tính bằng 5 lời gọi
       liên tiếp sẽ chạy đúng 1 cái, 4 ô còn lại đứng im — trông hệt như firmware
       không gửi dữ liệu. Đây là lỗi BLE Android phổ biến và khó đoán nhất. */
    private val queue = ArrayDeque<() -> Unit>()
    private var busy = false
    private fun enqueue(op: () -> Unit) { queue.add(op); if (!busy) next() }
    private fun next() { val op = queue.poll(); if (op == null) { busy = false; return }; busy = true; op() }
    private fun opDone() { busy = false; next() }

    // ---------------------------------------------------------------------- UI
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(14), dp(10), dp(14), dp(24))
            setBackgroundColor(Color.parseColor(C_BG))
        }

        // --- thanh đầu: tên + nút đổi ngôn ngữ
        val head = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
        }
        titleView = TextView(this).apply {
            textSize = 19f
            setTypeface(null, Typeface.BOLD)
            setTextColor(Color.parseColor(C_TEXT))
        }
        head.addView(titleView, LinearLayout.LayoutParams(0, WRAP_CONTENT, 1f))
        langBtn = Button(this).apply {
            textSize = 12f
            setOnClickListener {
                L.cur = if (L.cur == Lang.VI) Lang.EN else Lang.VI
                applyLang()
            }
        }
        head.addView(langBtn, LinearLayout.LayoutParams(WRAP_CONTENT, WRAP_CONTENT))
        root.addView(head, lp())

        // --- băng trạng thái
        banner = TextView(this).apply {
            textSize = 23f
            gravity = Gravity.CENTER
            setTypeface(null, Typeface.BOLD)
            setPadding(dp(14), dp(20), dp(14), dp(6))
            setTextColor(Color.WHITE)
        }
        subBanner = TextView(this).apply {
            textSize = 13f
            gravity = Gravity.CENTER
            setPadding(dp(14), 0, dp(14), dp(18))
            setTextColor(Color.parseColor("#e6e9ef"))
        }
        val bannerBox = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            background = round(C_IDLE)
            addView(banner); addView(subBanner)
        }
        root.addView(bannerBox, lp(dp(10)))

        connectBtn = Button(this).apply {
            isAllCaps = false            // chữ dài kèm số, viết hoa hết thì khó đọc
            setOnClickListener { if (connected) disconnect() else startScan() }
        }
        root.addView(connectBtn, lp(dp(10)))

        // --- lưới 6 cell: đọc được trong một cái liếc, không phải một dòng chữ dài
        tempLabel = label()
        tempRow = LinearLayout(this).apply { orientation = LinearLayout.HORIZONTAL }
        for (i in 1..6) {
            val box = LinearLayout(this).apply {
                orientation = LinearLayout.VERTICAL
                gravity = Gravity.CENTER
                background = round(C_CARD)
                setPadding(0, dp(10), 0, dp(10))
            }
            val v = TextView(this).apply {
                text = "--"; textSize = 16f
                setTypeface(null, Typeface.BOLD)
                setTextColor(Color.parseColor(C_TEXT)); gravity = Gravity.CENTER
            }
            val n = TextView(this).apply {
                text = "$i"; textSize = 10f
                setTextColor(Color.parseColor(C_LABEL)); gravity = Gravity.CENTER
            }
            box.addView(v); box.addView(n)
            cellTiles.add(v to n)
            val p = LinearLayout.LayoutParams(0, WRAP_CONTENT, 1f)
            p.setMargins(if (i == 1) 0 else dp(4), 0, 0, 0)
            tempRow.addView(box, p)
        }
        val tempCard = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            background = round(C_CARD)
            setPadding(dp(12), dp(10), dp(12), dp(10))
            addView(tempLabel); addView(tempRow, lp(dp(8)))
        }
        root.addView(tempCard, lp(dp(10)))

        // --- các ô còn lại
        for ((uuid, key) in ROWS) {
            val lab = label()
            val value = TextView(this).apply {
                text = "—"; textSize = 15f
                setTextColor(Color.parseColor(C_TEXT))
                setPadding(0, dp(3), 0, 0)
            }
            val card = LinearLayout(this).apply {
                orientation = LinearLayout.VERTICAL
                background = round(C_CARD)
                setPadding(dp(12), dp(10), dp(12), dp(10))
                addView(lab); addView(value)
            }
            labels[uuid] = lab
            lab.tag = key
            views[uuid] = value
            root.addView(card, lp(dp(8)))
        }

        muteBtn = Button(this).apply {
            isEnabled = false
            isAllCaps = false
            /* Nút TRẠNG THÁI, không phải nút hành động: nó hiện còi đang thế nào
               và chạm vào thì đảo. Bản trước chỉ báo "đã gửi lệnh" rồi thôi —
               người dùng không biết lệnh có ăn không, mà đây là cái nút quyết
               định còi báo cháy có kêu hay không. */
            setOnClickListener { sendCmd(if (stMuted) "mute off" else "mute on") }
        }
        root.addView(muteBtn, lp(dp(16)))

        quietBtn = Button(this).apply {
            isEnabled = false
            isAllCaps = false
            setOnClickListener { sendCmd(if (stQuiet) "quiet off" else "quiet on") }
        }
        root.addView(quietBtn, lp(dp(8)))

        noteView = TextView(this).apply {
            textSize = 12f
            setTextColor(Color.parseColor(C_LABEL))
            setPadding(dp(2), dp(16), dp(2), 0)
        }
        root.addView(noteView, lp())

        setContentView(ScrollView(this).apply {
            setBackgroundColor(Color.parseColor(C_BG)); addView(root)
        })

        applyLang()
        ensurePermissions()
        startStaleWatch()
        startTtlTick()
    }

    private fun label() = TextView(this).apply {
        textSize = 11f
        setTextColor(Color.parseColor(C_LABEL))
    }

    private fun round(color: String) = GradientDrawable().apply {
        setColor(Color.parseColor(color))
        cornerRadius = dp(14).toFloat()
    }

    private fun lp(topMargin: Int = 0) =
        LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT).apply { setMargins(0, topMargin, 0, 0) }

    private fun dp(v: Int) = (v * resources.displayMetrics.density).toInt()

    /* Đổi ngôn ngữ KHÔNG dựng lại màn hình: chỉ ghi đè chữ. Dựng lại sẽ mất kết
       nối BLE, và người dùng đổi ngôn ngữ giữa lúc đang xem pack báo động thì
       mất kết nối là hỏng đúng lúc không được phép hỏng. */
    private fun applyLang() {
        titleView.text = L["title"]
        langBtn.text = if (L.cur == Lang.VI) "EN" else "VI"
        connectBtn.text = if (connected) L["disconnect"] else L["connect"]
        tempLabel.text = L["rowTemps"] + " (°C)"
        for ((uuid, v) in labels) v.text = L[v.tag as String]
        noteView.text = L["note"] + "\n\n" + L["noteCrit"]
        renderBanner()
        renderButtons()
        // Đọc lại để các ô chữ do chip gửi được dịch theo ngôn ngữ mới.
        val g = gatt ?: return
        val svc = g.getService(UUID_SVC) ?: return
        for ((uuid, _) in ROWS) svc.getCharacteristic(uuid)?.let { c ->
            enqueue { try { g.readCharacteristic(c) } catch (_: SecurityException) { opDone() } }
        }
    }

    // --------------------------------------------------------- quyền truy cập
    private fun ensurePermissions() {
        val need = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
            arrayOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)
        else arrayOf(Manifest.permission.ACCESS_FINE_LOCATION)
        val missing = need.filter { checkSelfPermission(it) != PackageManager.PERMISSION_GRANTED }
        if (missing.isNotEmpty()) requestPermissions(missing.toTypedArray(), 1)
    }

    private fun hasScanPerm(): Boolean {
        val p = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
            Manifest.permission.BLUETOOTH_SCAN else Manifest.permission.ACCESS_FINE_LOCATION
        return checkSelfPermission(p) == PackageManager.PERMISSION_GRANTED
    }

    // ------------------------------------------------------------------- quét
    private fun startScan() {
        if (!hasScanPerm()) { ensurePermissions(); return }

        val mgr = getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        val adapter = mgr.adapter
        if (adapter == null || !adapter.isEnabled) { setBanner(L["btOff"], C_ALARM); return }

        /* ĐÃ GHÉP ĐÔI THÌ ĐỪNG QUÉT — nối thẳng theo địa chỉ.
           Đo 22/09: app bị tắt nhưng Android vẫn giữ liên kết với pack. ESP32
           chỉ nhận MỘT kết nối nên lúc đó nó NGỪNG QUẢNG BÁ — máy tính quét thấy
           91 thiết bị khác mà không thấy HuTieu-BMS. Quét kiểu gì cũng không ra,
           và app báo "không thấy pack" trong khi pack vẫn chạy ngay cạnh.
           Nối theo địa chỉ không cần quảng bá nên thoát hẳn cái bẫy đó, và tránh
           luôn giới hạn của Android: quá 5 lần quét trong 30 giây là hệ thống
           lặng lẽ bỏ qua, không báo lỗi gì. */
        try {
            val known = adapter.bondedDevices?.firstOrNull { it.name == DEV_NAME }
            if (known != null) { setBanner(L["bonded"], C_IDLE); connect(known); return }
        } catch (_: SecurityException) { }

        val scanner = adapter.bluetoothLeScanner ?: return
        setBanner(L["searching"], C_IDLE)
        connectBtn.isEnabled = false

        var done = false
        val cb = object : ScanCallback() {
            override fun onScanResult(type: Int, result: ScanResult) {
                if (done || result.device?.name != DEV_NAME) return
                done = true
                try { scanner.stopScan(this) } catch (_: SecurityException) {}
                connect(result.device)
            }
            override fun onScanFailed(errorCode: Int) {
                if (done) return
                done = true
                runOnUiThread { setBanner(L["notFound"], C_ALARM); connectBtn.isEnabled = true }
            }
        }
        try {
            scanner.startScan(null,
                ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build(), cb)
        } catch (e: SecurityException) {
            setBanner(L["noPerm"], C_ALARM); connectBtn.isEnabled = true; return
        }
        /* Bỏ cuộc sau 15 s: không có mốc này thì lúc pack tắt điện app quay mãi
           và người dùng không biết nên đi kiểm dây hay đứng chờ tiếp. */
        ui.postDelayed({
            if (done) return@postDelayed
            done = true
            try { scanner.stopScan(cb) } catch (_: SecurityException) {}
            setBanner(L["notFound"], C_ALARM); connectBtn.isEnabled = true
        }, 15000)
    }

    // ---------------------------------------------------------------- kết nối
    private fun connect(device: BluetoothDevice) {
        setBanner(L["connecting"], C_IDLE)
        try { gatt = device.connectGatt(this, false, gattCb, BluetoothDevice.TRANSPORT_LE) }
        catch (e: SecurityException) { setBanner(L["noPerm"], C_ALARM); connectBtn.isEnabled = true }
    }

    private fun disconnect() {
        try { gatt?.disconnect(); gatt?.close() } catch (_: SecurityException) {}
        gatt = null; queue.clear(); busy = false
    }

    /* Xoá cache GATT — không có hàm công khai, phải gọi qua reflection.
       Android NHỚ bảng dịch vụ của thiết bị đã ghép đôi và KHÔNG dò lại ở lần
       nối sau. Nạp lại firmware làm handle đổi chỗ, nhưng điện thoại vẫn dùng
       bảng cũ, nên nó ghi vào handle giờ trỏ sang một đặc tính CHỈ ĐỌC và nhận
       0x03 WRITE_NOT_PERMITTED. Firmware của pack còn sửa tới ngày thi nên đây
       là chuyện sẽ còn xảy ra. Thất bại thì coi như bình thường: cùng lắm quay
       về đúng tình trạng cũ. */
    private fun refreshGattCache(g: BluetoothGatt) {
        try { android.util.Log.i("HuTieu", "xoa cache GATT: ${g.javaClass.getMethod("refresh").invoke(g)}") }
        catch (e: Exception) { android.util.Log.w("HuTieu", "khong xoa duoc cache: ${e.javaClass.simpleName}") }
    }

    private val gattCb = object : BluetoothGattCallback() {

        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                runOnUiThread { setBanner(L["reading"], C_IDLE) }
                try { g.requestMtu(247) } catch (_: SecurityException) {}
            } else {
                connected = false
                runOnUiThread {
                    setBanner(L["lost"], C_ALARM)
                    connectBtn.text = L["connect"]; connectBtn.isEnabled = true
                    muteBtn.isEnabled = false; quietBtn.isEnabled = false
                }
                try { g.close() } catch (_: SecurityException) {}
            }
        }

        override fun onMtuChanged(g: BluetoothGatt, mtu: Int, status: Int) {
            /* Xin MTU lớn TRƯỚC khi đọc: MTU mặc định 23 byte cắt cụt chuỗi mà
               không báo lỗi, nên dòng nhiệt độ 6 cell sẽ mất đuôi. */
            refreshGattCache(g)
            try { g.discoverServices() } catch (_: SecurityException) {}
        }

        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            val svc = g.getService(UUID_SVC)
            if (svc == null) { runOnUiThread { setBanner(L["noSvc"], C_ALARM) }; return }
            connected = true
            runOnUiThread {
                connectBtn.text = L["disconnect"]; connectBtn.isEnabled = true
                muteBtn.isEnabled = true; quietBtn.isEnabled = true
            }

            /* In handle và cờ quyền của từng đặc tính. Khi ghi thất bại, đây là
               thứ phân biệt "firmware chặn đúng" với "điện thoại đang dùng bảng
               GATT cũ" — hai nguyên nhân cho cùng một triệu chứng, và đoán mò
               giữa chúng đã tốn của đội một buổi. 0x08 = ghi được. */
            for (c in svc.characteristics)
                android.util.Log.i("HuTieu", "dac tinh %s handle=%d quyen=0x%02x"
                    .format(c.uuid.toString().substring(19, 23), c.instanceId, c.properties))

            for (uuid in listOf(UUID_CTL, UUID_STATE, UUID_TEMPS) + ROWS.map { it.first })
                svc.getCharacteristic(uuid)?.let { c ->
                    enqueue { try { g.readCharacteristic(c) } catch (_: SecurityException) { opDone() } }
                }

            for (uuid in listOf(UUID_STATE, UUID_TEMPS, UUID_AI, UUID_DIAG, UUID_CTL)) {
                val c = svc.getCharacteristic(uuid) ?: continue
                enqueue {
                    try {
                        g.setCharacteristicNotification(c, true)
                        val d = c.getDescriptor(CCCD)
                        if (d == null) opDone() else {
                            @Suppress("DEPRECATION") d.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                            @Suppress("DEPRECATION") if (!g.writeDescriptor(d)) opDone()
                        }
                    } catch (_: SecurityException) { opDone() }
                }
            }
            startLayer2Poll()
        }

        @Suppress("DEPRECATION")
        override fun onCharacteristicRead(g: BluetoothGatt, c: BluetoothGattCharacteristic, status: Int) {
            show(c.uuid, c.value); opDone()
        }

        @Suppress("DEPRECATION")
        override fun onCharacteristicChanged(g: BluetoothGatt, c: BluetoothGattCharacteristic) {
            show(c.uuid, c.value)    // notify KHÔNG qua hàng đợi, đừng gọi opDone()
        }

        override fun onDescriptorWrite(g: BluetoothGatt, d: BluetoothGattDescriptor, status: Int) = opDone()

        @Suppress("DEPRECATION")
        override fun onCharacteristicWrite(g: BluetoothGatt, c: BluetoothGattCharacteristic, status: Int) {
            runOnUiThread {
                when (status) {
                    /* KHÔNG báo "đã tắt còi" ở đây — mới chỉ biết gói tin đi tới
                       nơi. Trạng thái thật đến từ đặc tính 000A do chip gửi về,
                       và hai nút chỉ đổi theo nó. */
                    BluetoothGatt.GATT_SUCCESS -> toast(L["sent"])
                    BluetoothGatt.GATT_INSUFFICIENT_AUTHENTICATION,
                    BluetoothGatt.GATT_INSUFFICIENT_ENCRYPTION -> toast(L["needPin"])
                    3 -> toast(L["staleGatt"])   // WRITE_NOT_PERMITTED, xem refreshGattCache
                    else -> toast(L["sendFail"].format(status))
                }
            }
            opDone()
        }
    }

    // ------------------------------------------------------------------ lệnh
    private fun sendCmd(cmd: String) {
        val g = gatt ?: return
        val c = g.getService(UUID_SVC)?.getCharacteristic(UUID_CMD) ?: return
        enqueue {
            try {
                @Suppress("DEPRECATION") c.value = cmd.toByteArray(Charsets.UTF_8)
                c.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
                @Suppress("DEPRECATION") if (!g.writeCharacteristic(c)) opDone()
            } catch (_: SecurityException) { opDone() }
        }
    }

    // --------------------------------------------------------------- hiển thị
    private fun show(uuid: UUID, raw: ByteArray?) {
        val text = raw?.toString(Charsets.UTF_8) ?: return
        lastPacketAt = System.currentTimeMillis()
        runOnUiThread {
            when (uuid) {
                UUID_CTL   -> { parseCtl(text); renderBanner(); renderButtons() }
                UUID_TEMPS -> showTemps(text)
                UUID_STATE -> { }   // băng trên cùng dựng từ 000A, không dò chữ
                else       -> views[uuid]?.text = L.translateFromChip(text)
            }
        }
    }

    /* "mute=1 quiet=0 lvl=2 ttl=95" — dạng khoá=giá trị, cố ý không phải chữ
       tiếng Việt: firmware đổi một từ trong câu hiển thị cũng không làm hai nút
       này hiểu sai trạng thái còi. */
    private fun parseCtl(s: String) {
        for (kv in s.trim().split(" ")) {
            val p = kv.split("=")
            if (p.size != 2) continue
            val v = p[1].toIntOrNull() ?: continue
            when (p[0]) {
                "mute" -> stMuted = v == 1
                "quiet" -> stQuiet = v == 1
                "lvl" -> stLevel = v
                "ttl" -> stTtl = v
            }
        }
    }

    private fun showTemps(s: String) {
        /* Chip gửi "1:27.2 2:27.0 ...", cảm biến hỏng thì "3:--". Giữ nguyên
           quy ước đó: ô trống rõ ràng hơn một số cũ trông như số mới. */
        val map = HashMap<Int, String>()
        for (tok in s.trim().split(Regex("\\s+"))) {
            val p = tok.split(":")
            if (p.size == 2) p[0].toIntOrNull()?.let { map[it] = p[1] }
        }
        for (i in 1..6) {
            val v = map[i] ?: "--"
            cellTiles[i - 1].first.text = v
            val t = v.toFloatOrNull()
            cellTiles[i - 1].first.setTextColor(Color.parseColor(
                if (t == null) C_LABEL else if (t >= 60f) "#ff8a8a"
                else if (t >= 45f) "#ffd08a" else C_TEXT))
        }
    }

    private fun renderBanner() {
        if (!connected) { setBanner(L["notConnected"], C_IDLE); subBanner.text = ""; return }
        banner.text = L.level(stLevel)
        val color = when (stLevel) { 0 -> C_OK; 1 -> C_WATCH; 2 -> C_ALARM; else -> C_CRIT }
        (banner.parent as LinearLayout).background = round(color)
        /* Còi tắt rồi thì màn hình là lời nhắc DUY NHẤT còn lại — nói rõ là
           chưa xử lý, đừng chỉ hiện biểu tượng loa gạch chéo. Ở mức NGUY KỊCH
           thì nói thêm rằng còi sẽ tự kêu lại, để người dùng biết im lặng này
           có hạn. */
        subBanner.text = when {
            !stMuted -> ""
            stLevel >= 3 -> "🔇 " + L["unresolved"] + " · " + L["critRemind"]
            stLevel >= 1 -> "🔇 " + L["unresolved"]
            else -> "🔇 " + L["buzzOff"]
        }
    }

    private fun renderButtons() {
        muteBtn.text = when {
            !stMuted -> L["buzzOn"]
            stTtl > 0 -> "🔇 %s — %s %d:%02d".format(L["buzzOff"], L["buzzBack"], stTtl / 60, stTtl % 60)
            else -> "🔇 %s — %s".format(L["buzzOff"], L["tapUnmute"])
        }
        quietBtn.text = if (stQuiet) L["quietOn"] else L["quietOff"]
    }

    private fun setBanner(text: String, color: String) {
        runOnUiThread {
            banner.text = text
            (banner.parent as LinearLayout).background = round(color)
        }
    }

    /* Đếm lùi tại chỗ giữa hai lần notify. Chip chỉ gửi `ttl` lúc trạng thái
       đổi — notify mỗi giây chỉ để một con số nhích xuống là phí pin cả hai đầu. */
    private fun startTtlTick() {
        ui.postDelayed(object : Runnable {
            override fun run() {
                if (stTtl > 0) { stTtl--; renderButtons() }
                ui.postDelayed(this, 1000)
            }
        }, 1000)
    }

    /* Lớp 2 KHÔNG có notify — chip chỉ cập nhật mỗi chu kỳ sạc (vài tiếng). Nhưng
       đọc đúng một lần lúc kết nối thì nếu lúc đó chip chưa tính xong chu kỳ đầu,
       ô này sẽ trống VĨNH VIỄN — đúng cái đã thấy trên máy thật ngày 22/09. */
    private fun startLayer2Poll() {
        ui.postDelayed(object : Runnable {
            override fun run() {
                if (!connected) return
                val g = gatt
                val c = g?.getService(UUID_SVC)?.getCharacteristic(UUID_L2)
                if (g != null && c != null)
                    enqueue { try { g.readCharacteristic(c) } catch (_: SecurityException) { opDone() } }
                ui.postDelayed(this, 30000)
            }
        }, 30000)
    }

    /* 15 s không có gói nào mà vẫn tưởng đang kết nối thì phải NÓI RA. Màn hình
       đứng im với số cũ nguy hiểm hơn màn hình báo mất kết nối: người thợ sẽ tin
       là pack đang bình thường. */
    private fun startStaleWatch() {
        ui.postDelayed(object : Runnable {
            override fun run() {
                if (connected && lastPacketAt > 0 &&
                    System.currentTimeMillis() - lastPacketAt > 15000)
                    setBanner(L["stale"], C_ALARM)
                ui.postDelayed(this, 5000)
            }
        }, 5000)
    }

    private fun toast(s: String) = Toast.makeText(this, s, Toast.LENGTH_LONG).show()

    override fun onDestroy() { super.onDestroy(); disconnect() }
}
