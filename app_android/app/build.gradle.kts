plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "vn.ictu.hutieu"
    compileSdk = 34

    defaultConfig {
        applicationId = "vn.ictu.hutieu"
        /* minSdk 26: dưới đó API BLE khác hẳn và đội không có máy nào để thử.
           Nhận một dòng "máy quá cũ" lúc cài còn hơn crash giữa lúc trình diễn. */
        minSdk = 26
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"
    }

    buildTypes {
        /* Ký bằng khoá debug là CỐ Ý. App này chỉ để cài tay cho đội và giám
           khảo xem, không lên Play Store. Khoá phát hành đòi quản lý bí mật —
           thêm một thứ để làm hỏng trước ngày thi mà không đổi lại được gì. */
        getByName("debug") {
            isMinifyEnabled = false
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions { jvmTarget = "17" }
}

/* KHÔNG có khối dependencies — cố ý, và đây là quyết định chứ không phải thiếu sót.
 *
 * App chỉ cần android.bluetooth (có sẵn trong framework) và vài View dựng bằng
 * code. Thêm AndroidX/Compose là thêm hàng trăm MB phải tải về đúng tuần trước
 * ngày thi, và thêm một cách để build trượt trên máy khác. Đổi lại được gì?
 * Giao diện đẹp hơn một chút. Không đáng.
 *
 * Hệ quả trực tiếp: build được kể cả khi mất mạng, vì không phải tải gì thêm.
 */
