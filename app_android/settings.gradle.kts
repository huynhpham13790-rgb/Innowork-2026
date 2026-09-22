/* Không dùng thư viện ngoài nào — xem app/build.gradle.kts để biết vì sao. */
pluginManagement {
    repositories { google(); mavenCentral(); gradlePluginPortal() }
}
dependencyResolutionManagement {
    repositories { google(); mavenCentral() }
}
rootProject.name = "HuTieuBMS"
include(":app")
