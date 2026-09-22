package vn.ictu.hutieu

import android.Manifest
import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.content.pm.PackageManager
import android.graphics.Color
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.Gravity
import android.view.View
import android.view.ViewGroup.LayoutParams.MATCH_PARENT
import android.view.ViewGroup.LayoutParams.WRAP_CONTENT
import android.widget.*
import java.util.ArrayDeque
import java.util.UUID

/* =============================================================================
 *  App xem pack pin qua BLE — bản Android thật (QĐ-044)
 *
 *  VÌ SAO KHÔNG DÙNG BẢN WEB NỮA
 *  Bản Web Bluetooth (app/index.html) chạy đúng, nhưng nó phải TẢI VỀ từ một
 *  máy chủ trong mạng LAN. Trên Wi-Fi trường ICTU điện thoại không chạm được
 *  tới máy chủ (đo được: log máy chủ không hề có request nào từ điện thoại) —
 *  mạng chặn máy-nói-với-máy. Không sửa được từ phía đội.
 *
 *  App này cài một lần rồi KHÔNG CẦN MẠNG nữa, kể cả lúc mở. Đó là khác biệt
 *  duy nhất nhưng quyết định: hôm thi không ai biết Wi-Fi hội trường thế nào.
 *
 *  Bản web vẫn giữ, không xoá: nó là phương án cho máy không cài được app, và
 *  để giám khảo thấy cùng một GATT phục vụ được hai loại client.
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

/* Descriptor chuẩn để bật notify. Con số 2902 này do Bluetooth SIG quy định,
   không phải do firmware đội đặt. */
private val CCCD = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")

/* Thứ tự hiện trên màn, và nhãn. Đặt ở một chỗ để đổi thứ tự không phải sửa
   nhiều nơi — thứ tự này là thứ tự ƯU TIÊN ĐỌC của người đứng cạnh pack:
   trạng thái trước, rồi cell nào hỏng, rồi mới tới số liệu nền. */
private val ROWS = listOf(
    UUID_STATE to "Trạng thái",
    UUID_AI    to "Lớp 1 — cell nghi ngờ",
    UUID_TEMPS to "Nhiệt độ từng cell (°C)",
    UUID_DIAG  to "Số đo đã dùng để chẩn đoán",
    UUID_PACK  to "Điện áp / dòng / SoC",
    UUID_L2    to "Lớp 2 — sức khoẻ và tuổi thọ",
    UUID_SYS   to "Tình trạng hệ thống"
)

class MainActivity : Activity_() {

    private var gatt: BluetoothGatt? = null
    private val ui = Handler(Looper.getMainLooper())
    private val views = HashMap<UUID, TextView>()

    private lateinit var banner: TextView
    private lateinit var connectBtn: Button
    private lateinit var muteBtn: Button
    private lateinit var quietBtn: Button

    private var lastPacketAt = 0L
    private var connected = false

    /* ---------------------------------------------------------------------
     *  HÀNG ĐỢI GATT — thứ hay bị bỏ nhất khi viết BLE trên Android.
     *
     *  Android chỉ cho MỘT thao tác GATT chạy tại một thời điểm. Gọi cái thứ
     *  hai khi cái thứ nhất chưa xong thì nó bị BỎ IM LẶNG: không lỗi, không
     *  ngoại lệ, chỉ là callback không bao giờ tới. Bật notify cho 4 đặc tính
     *  bằng 4 lời gọi liên tiếp sẽ chạy đúng 1 cái, và 3 ô còn lại đứng im —
     *  trông hệt như firmware không gửi dữ liệu.
     * ------------------------------------------------------------------ */
    private val queue = ArrayDeque<() -> Unit>()
    private var busy = false

    private fun enqueue(op: () -> Unit) {
        queue.add(op)
        if (!busy) next()
    }

    private fun next() {
        val op = queue.poll()
        if (op == null) { busy = false; return }
        busy = true
        op()
    }

    private fun opDone() { busy = false; next() }

    // ------------------------------------------------------------------ UI
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val root = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            setPadding(dp(16), dp(16), dp(16), dp(16))
            setBackgroundColor(Color.parseColor("#0f1115"))
        }

        banner = TextView(this).apply {
            text = "Chưa kết nối"
            textSize = 20f
            gravity = Gravity.CENTER
            setPadding(dp(12), dp(16), dp(12), dp(16))
            setTextColor(Color.WHITE)
            setBackgroundColor(Color.parseColor("#3a3f4b"))
        }
        root.addView(banner, lp())

        connectBtn = Button(this).apply {
            text = "Kết nối tới pack"
            setOnClickListener { if (connected) disconnect() else startScan() }
        }
        root.addView(connectBtn, lp(dp(8)))

        for ((uuid, label) in ROWS) {
            val card = LinearLayout(this).apply {
                orientation = LinearLayout.VERTICAL
                setPadding(dp(12), dp(10), dp(12), dp(10))
                setBackgroundColor(Color.parseColor("#1a1d24"))
            }
            card.addView(TextView(this).apply {
                text = label
                textSize = 12f
                setTextColor(Color.parseColor("#8b93a7"))
            })
            val value = TextView(this).apply {
                text = "—"
                textSize = 16f
                setTextColor(Color.WHITE)
            }
            card.addView(value)
            views[uuid] = value
            root.addView(card, lp(dp(8)))
        }

        muteBtn = Button(this).apply {
            text = "Tắt tiếng còi (5 phút)"
            isEnabled = false
            setOnClickListener { sendCmd("mute on") }
        }
        root.addView(muteBtn, lp(dp(12)))

        quietBtn = Button(this).apply {
            text = "Chế độ yên lặng"
            isEnabled = false
            setOnClickListener { sendCmd("quiet on") }
        }
        root.addView(quietBtn, lp(dp(8)))

        root.addView(TextView(this).apply {
            text = "Lần bấm đầu tiên máy sẽ hỏi ghép đôi và mã PIN — đó là cố ý " +
                   "(QĐ-042): người đi ngang qua không tắt được còi báo cháy.\n\n" +
                   "Còi mức NGUY HIỂM không tắt được bằng nút này."
            textSize = 12f
            setTextColor(Color.parseColor("#8b93a7"))
            setPadding(0, dp(16), 0, 0)
        }, lp())

        setContentView(ScrollView(this).apply { addView(root) })

        ensurePermissions()
        startStaleWatch()
    }

    private fun lp(topMargin: Int = 0) =
        LinearLayout.LayoutParams(MATCH_PARENT, WRAP_CONTENT).apply { setMargins(0, topMargin, 0, 0) }

    private fun dp(v: Int) = (v * resources.displayMetrics.density).toInt()

    // --------------------------------------------------------- quyền truy cập
    private fun ensurePermissions() {
        val need = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
            arrayOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)
        else
            arrayOf(Manifest.permission.ACCESS_FINE_LOCATION)

        val missing = need.filter {
            checkSelfPermission(it) != PackageManager.PERMISSION_GRANTED
        }
        if (missing.isNotEmpty()) requestPermissions(missing.toTypedArray(), 1)
    }

    private fun hasScanPerm(): Boolean {
        val p = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
            Manifest.permission.BLUETOOTH_SCAN else Manifest.permission.ACCESS_FINE_LOCATION
        return checkSelfPermission(p) == PackageManager.PERMISSION_GRANTED
    }

    // ----------------------------------------------------------------- quét
    private fun startScan() {
        if (!hasScanPerm()) { ensurePermissions(); return }

        val mgr = getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        val adapter = mgr.adapter
        if (adapter == null || !adapter.isEnabled) {
            setBanner("Hãy bật Bluetooth rồi thử lại", "#b3541e"); return
        }
        val scanner = adapter.bluetoothLeScanner
        if (scanner == null) { setBanner("Máy không quét được BLE", "#b3541e"); return }

        setBanner("Đang tìm $DEV_NAME…", "#3a3f4b")
        connectBtn.isEnabled = false

        var done = false
        val cb = object : ScanCallback() {
            override fun onScanResult(type: Int, result: ScanResult) {
                if (done) return
                if (result.device?.name != DEV_NAME) return
                done = true
                try { scanner.stopScan(this) } catch (_: SecurityException) {}
                connect(result.device)
            }
            override fun onScanFailed(errorCode: Int) {
                if (done) return
                done = true
                runOnUiThread {
                    setBanner("Quét thất bại (mã $errorCode)", "#b3541e")
                    connectBtn.isEnabled = true
                }
            }
        }

        try {
            scanner.startScan(null,
                ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build(), cb)
        } catch (e: SecurityException) {
            setBanner("Thiếu quyền Bluetooth", "#b3541e"); connectBtn.isEnabled = true; return
        }

        /* Bỏ cuộc sau 15 s. Không có mốc này thì lúc pack tắt điện app quay mãi
           và người dùng không biết là nên đi kiểm dây hay đứng chờ tiếp. */
        ui.postDelayed({
            if (done) return@postDelayed
            done = true
            try { scanner.stopScan(cb) } catch (_: SecurityException) {}
            setBanner("Không thấy $DEV_NAME. Pack có đang bật không?", "#b3541e")
            connectBtn.isEnabled = true
        }, 15000)
    }

    // -------------------------------------------------------------- kết nối
    private fun connect(device: BluetoothDevice) {
        setBanner("Đang kết nối…", "#3a3f4b")
        try {
            gatt = device.connectGatt(this, false, gattCb, BluetoothDevice.TRANSPORT_LE)
        } catch (e: SecurityException) {
            setBanner("Thiếu quyền kết nối", "#b3541e"); connectBtn.isEnabled = true
        }
    }

    private fun disconnect() {
        try { gatt?.disconnect(); gatt?.close() } catch (_: SecurityException) {}
        gatt = null
        queue.clear(); busy = false
    }

    private val gattCb = object : BluetoothGattCallback() {

        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                runOnUiThread { setBanner("Đã kết nối, đang đọc…", "#3a3f4b") }
                try { g.requestMtu(247) } catch (_: SecurityException) {}
            } else {
                connected = false
                runOnUiThread {
                    setBanner("Mất kết nối — số đang hiện KHÔNG còn mới", "#b3541e")
                    connectBtn.text = "Kết nối tới pack"
                    connectBtn.isEnabled = true
                    muteBtn.isEnabled = false
                    quietBtn.isEnabled = false
                }
                try { g.close() } catch (_: SecurityException) {}
            }
        }

        override fun onMtuChanged(g: BluetoothGatt, mtu: Int, status: Int) {
            /* Xin MTU lớn TRƯỚC khi đọc: MTU mặc định 23 byte cắt cụt chuỗi mà
               không báo lỗi, nên dòng nhiệt độ 6 cell sẽ mất đuôi. */
            try { g.discoverServices() } catch (_: SecurityException) {}
        }

        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            val svc = g.getService(UUID_SVC)
            if (svc == null) {
                runOnUiThread { setBanner("Thiết bị không có dịch vụ của pack", "#b3541e") }
                return
            }
            connected = true
            runOnUiThread {
                connectBtn.text = "Ngắt kết nối"
                connectBtn.isEnabled = true
                muteBtn.isEnabled = true
                quietBtn.isEnabled = true
            }

            // Đọc một lượt để có số ngay, đừng bắt người dùng chờ gói notify đầu.
            for ((uuid, _) in ROWS) {
                svc.getCharacteristic(uuid)?.let { c ->
                    enqueue { try { g.readCharacteristic(c) } catch (_: SecurityException) { opDone() } }
                }
            }
            // Rồi mới bật notify — từng cái một, qua hàng đợi.
            for (uuid in listOf(UUID_STATE, UUID_TEMPS, UUID_AI, UUID_DIAG)) {
                val c = svc.getCharacteristic(uuid) ?: continue
                enqueue {
                    try {
                        g.setCharacteristicNotification(c, true)
                        val d = c.getDescriptor(CCCD)
                        if (d == null) { opDone() } else {
                            @Suppress("DEPRECATION")
                            d.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                            @Suppress("DEPRECATION")
                            if (!g.writeDescriptor(d)) opDone()
                        }
                    } catch (_: SecurityException) { opDone() }
                }
            }
        }

        @Suppress("DEPRECATION")
        override fun onCharacteristicRead(g: BluetoothGatt, c: BluetoothGattCharacteristic, status: Int) {
            show(c.uuid, c.value)
            opDone()
        }

        @Suppress("DEPRECATION")
        override fun onCharacteristicChanged(g: BluetoothGatt, c: BluetoothGattCharacteristic) {
            show(c.uuid, c.value)   // notify KHÔNG đi qua hàng đợi, đừng gọi opDone()
        }

        override fun onDescriptorWrite(g: BluetoothGatt, d: BluetoothGattDescriptor, status: Int) {
            opDone()
        }

        @Suppress("DEPRECATION")
        override fun onCharacteristicWrite(g: BluetoothGatt, c: BluetoothGattCharacteristic, status: Int) {
            runOnUiThread {
                when (status) {
                    BluetoothGatt.GATT_SUCCESS ->
                        toast("Đã gửi lệnh")
                    /* 5 và 15 = chưa xác thực / chưa mã hoá. KHÔNG phải lỗi:
                       đó đúng là firmware đang đòi ghép đôi (QĐ-042). Android
                       sẽ hiện hộp nhập PIN; bấm lại lần nữa là xong. */
                    BluetoothGatt.GATT_INSUFFICIENT_AUTHENTICATION,
                    BluetoothGatt.GATT_INSUFFICIENT_ENCRYPTION ->
                        toast("Cần ghép đôi: nhập PIN rồi bấm lại")
                    else -> toast("Gửi lệnh thất bại (mã $status)")
                }
            }
            opDone()
        }
    }

    // --------------------------------------------------------------- lệnh
    private fun sendCmd(cmd: String) {
        val g = gatt ?: return
        val c = g.getService(UUID_SVC)?.getCharacteristic(UUID_CMD) ?: return
        enqueue {
            try {
                @Suppress("DEPRECATION")
                c.value = cmd.toByteArray(Charsets.UTF_8)
                c.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
                @Suppress("DEPRECATION")
                if (!g.writeCharacteristic(c)) opDone()
            } catch (_: SecurityException) { opDone() }
        }
    }

    // ------------------------------------------------------------ hiển thị
    private fun show(uuid: UUID, raw: ByteArray?) {
        val text = raw?.toString(Charsets.UTF_8) ?: return
        lastPacketAt = System.currentTimeMillis()
        runOnUiThread {
            views[uuid]?.text = text
            if (uuid == UUID_STATE) {
                val up = text.uppercase()
                val color = when {
                    up.contains("NGUY HIỂM") || up.contains("NGUY HIEM") -> "#8b1a1a"
                    up.contains("BÁO ĐỘNG")  || up.contains("BAO DONG")  -> "#b3541e"
                    up.contains("THEO DÕI")  || up.contains("THEO DOI")  -> "#8a6d1f"
                    else -> "#1e5c3a"
                }
                setBanner(text, color)
            }
        }
    }

    private fun setBanner(text: String, color: String) {
        runOnUiThread {
            banner.text = text
            banner.setBackgroundColor(Color.parseColor(color))
        }
    }

    /* Nếu 15 s không có gói nào mà app vẫn tưởng đang kết nối thì phải NÓI RA.
       Màn hình đứng im với số cũ nguy hiểm hơn màn hình báo mất kết nối: người
       thợ sẽ tin là pack đang bình thường. */
    private fun startStaleWatch() {
        ui.postDelayed(object : Runnable {
            override fun run() {
                if (connected && lastPacketAt > 0 &&
                    System.currentTimeMillis() - lastPacketAt > 15000) {
                    setBanner("Không nhận được dữ liệu — số đang hiện đã cũ", "#b3541e")
                }
                ui.postDelayed(this, 5000)
            }
        }, 5000)
    }

    private fun toast(s: String) = Toast.makeText(this, s, Toast.LENGTH_SHORT).show()

    override fun onDestroy() {
        super.onDestroy()
        disconnect()
    }
}

/* Dùng android.app.Activity trần, không AndroidX — xem chú thích cuối
   app/build.gradle.kts để biết vì sao không kéo thư viện ngoài vào. */
typealias Activity_ = android.app.Activity
