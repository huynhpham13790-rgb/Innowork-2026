/* =============================================================================
 *  Test store-and-forward — chạy trên PC, KHÔNG cần board.
 *
 *  Điểm quan trọng: test này KHÔNG chép lại logic. Script `run_test.sh` cắt
 *  đúng đoạn code spool từ file .ino thật rồi nhét vào đây qua #include, nên
 *  nếu ai sửa .ino mà làm hỏng logic thì test đỏ ngay. Chép tay là test sẽ
 *  xanh trong khi firmware đã hỏng — đúng cái bẫy cần tránh.
 *
 *  Chạy:  ./test/run_test.sh
 * ========================================================================== */

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

// ------------------------------------------------------- giả lập Arduino API
struct String : public std::string {
    String() {}
    String(const char* s) : std::string(s ? s : "") {}
    String(const std::string& s) : std::string(s) {}
    String(unsigned long v, int) { *this = std::to_string(v); }
    unsigned length() const { return (unsigned)size(); }
    void trim() {
        size_t a = find_first_not_of(" \t\r\n");
        size_t b = find_last_not_of(" \t\r\n");
        if (a == npos) { clear(); return; }
        *this = substr(a, b - a + 1);
    }
    const char* c_str() const { return std::string::c_str(); }
};

#define FILE_READ   0
#define FILE_WRITE  1
#define FILE_APPEND 2

// File giả: đọc/ghi thẳng xuống thư mục tạm của máy thật.
struct File {
    std::fstream fs;
    std::string path;
    bool ok = false;
    File() {}
    File(const std::string& p, int mode) : path(p) {
        auto m = std::ios::in;
        if (mode == FILE_WRITE)  m = std::ios::out | std::ios::trunc;
        if (mode == FILE_APPEND) m = std::ios::out | std::ios::app;
        fs.open(p, m);
        ok = fs.is_open();
    }
    operator bool() const { return ok; }
    size_t size() {
        std::ifstream f(path, std::ios::ate | std::ios::binary);
        return f.is_open() ? (size_t)f.tellg() : 0;
    }
    void seek(size_t p) { fs.clear(); fs.seekg(p); }
    bool available() { return fs.peek() != EOF; }
    String readStringUntil(char) {
        std::string l;
        if (!std::getline(fs, l)) return String("");
        return String(l);
    }
    void println(const String& s) { fs << s << "\n"; }
    void close() { if (ok) fs.close(); ok = false; }
};

struct FsMock {
    std::string root;
    File open(const char* p, int mode) { return File(root + p, mode); }
    bool remove(const char* p) { return ::remove((root + p).c_str()) == 0; }
    bool rename(const char* a, const char* b) {
        return ::rename((root + a).c_str(), (root + b).c_str()) == 0;
    }
    bool begin(bool) { return true; }
} LittleFS;

// ------------------------------------------------- giả lập MQTT client
struct MqttMock {
    bool up = true;              // đang có mạng hay không
    int  failAfter = -1;         // gửi được bao nhiêu gói thì "rớt mạng"
    std::vector<std::string> sent;
    bool connected() { return up; }
    bool publish(const char*, const char* payload) {
        if (!up) return false;
        if (failAfter >= 0 && (int)sent.size() >= failAfter) { up = false; return false; }
        sent.push_back(payload);
        return true;
    }
} mqtt;

const char* topicData = "/wisepaas/scada/TEST/data";

struct SerialMock {
    void printf(const char*, ...) {}
    void println(const char*) {}
    void println(const String&) {}
    void print(const char*) {}
} Serial;

// ------------------------------------------------- code thật, cắt từ .ino
#include "extracted_spool.inc"

// ------------------------------------------------------------------ tiện ích
static int failures = 0;
void check(bool cond, const std::string& name) {
    std::cout << (cond ? "  PASS  " : "  FAIL  ") << name << "\n";
    if (!cond) failures++;
}
void resetAll(const std::string& dir) {
    LittleFS.root = dir;
    ::remove((dir + SPOOL_PATH).c_str());
    ::remove((dir + SPOOL_TMP_PATH).c_str());
    mqtt.sent.clear(); mqtt.up = true; mqtt.failAfter = -1;
    fsReady = true; spoolDropped = 0;
}
String pkt(int i) {
    return String("{\"d\":{\"BatteryPack01\":{\"Cell01_Temp\":30.0}},\"ts\":\"P" +
                  std::to_string(i) + "\"}");
}

int main(int argc, char** argv) {
    std::string dir = argc > 1 ? argv[1] : "/tmp";

    std::cout << "\n=== TEST 1: mat mang -> dem, noi lai -> day bu DUNG THU TU ===\n";
    resetAll(dir);
    mqtt.up = false;
    for (int i = 1; i <= 10; i++) spoolAppend(pkt(i));      // 10 goi luc mat mang
    check(spoolSize() > 0, "mat mang thi du lieu duoc ghi xuong flash");
    mqtt.up = true;
    bool clean = spoolFlush();
    check(clean, "noi lai -> spool sach sau 1 luot (10 < FLUSH_BATCH)");
    check(mqtt.sent.size() == 10, "day bu du 10 goi, khong mat goi nao");
    bool order = true;
    for (int i = 0; i < 10; i++)
        if (mqtt.sent[i].find("\"P" + std::to_string(i + 1) + "\"") == std::string::npos) order = false;
    check(order, "day bu DUNG THU TU goc (P1..P10)");
    check(spoolSize() == 0, "spool rong sau khi day bu xong");

    std::cout << "\n=== TEST 2: rot mang GIUA CHUNG luc dang day bu ===\n";
    resetAll(dir);
    mqtt.up = false;
    for (int i = 1; i <= 10; i++) spoolAppend(pkt(i));
    mqtt.up = true; mqtt.failAfter = 4;                     // gui duoc 4 goi roi rot
    spoolFlush();
    check(mqtt.sent.size() == 4, "gui duoc 4 goi truoc khi rot");
    mqtt.up = true; mqtt.failAfter = -1;
    spoolFlush();
    check(mqtt.sent.size() == 10, "noi lai lan 2 -> gui not 6 goi con lai, TONG = 10");
    bool order2 = true;
    for (int i = 0; i < 10; i++)
        if (mqtt.sent[i].find("\"P" + std::to_string(i + 1) + "\"") == std::string::npos) order2 = false;
    check(order2, "van dung thu tu du bi ngat giua chung");

    std::cout << "\n=== TEST 3: nhieu hon FLUSH_BATCH -> chia nhieu luot, khong nghen loop ===\n";
    resetAll(dir);
    mqtt.up = false;
    int N = FLUSH_BATCH * 3 + 7;
    for (int i = 1; i <= N; i++) spoolAppend(pkt(i));
    mqtt.up = true;
    int rounds = 0;
    while (!spoolFlush() && rounds < 100) rounds++;
    check((int)mqtt.sent.size() == N, "day bu du " + std::to_string(N) + " goi qua nhieu luot");
    check(rounds >= 3, "chia thanh nhieu luot (moi luot toi da FLUSH_BATCH goi)");

    std::cout << "\n=== TEST 4: flash day -> compact, giu du lieu MOI ===\n";
    resetAll(dir);
    mqtt.up = false;
    // Phai ghi du nhieu de VUOT that su SPOOL_MAX_BYTES, neu khong compact
    // khong bao gio chay va test nay xanh mot cach vo nghia.
    const int BIG = (int)(SPOOL_MAX_BYTES / pkt(1).length()) * 2;
    for (int i = 1; i <= BIG; i++) spoolAppend(pkt(i));
    size_t szAfter = spoolSize();
    check(szAfter < SPOOL_MAX_BYTES * 2, "compact da chay that su (kiem tra tien de cua test)");
    check(szAfter <= SPOOL_MAX_BYTES * 1.1, "spool bi chan duoi nguong, khong phinh vo han");
    mqtt.up = true;
    while (!spoolFlush()) {}
    check(mqtt.sent.size() > 0, "van day bu duoc sau khi compact");
    bool keptNewest = mqtt.sent.back().find("\"P" + std::to_string(BIG) + "\"") != std::string::npos;
    check(keptNewest, "goi MOI NHAT duoc giu lai");
    bool droppedOldest = mqtt.sent.front().find("\"P1\"") == std::string::npos;
    check(droppedOldest, "goi cu nhat (P1) da bi bo - dung chinh sach giu du lieu moi");

    std::cout << "\n=== TEST 5: spool rong thi flush khong lam gi ===\n";
    resetAll(dir);
    check(spoolFlush(), "spool rong -> tra ve 'da sach'");
    check(mqtt.sent.empty(), "khong gui gi ca");

    std::cout << "\n" << (failures ? "=== CO " + std::to_string(failures) + " TEST HONG ==="
                                   : "=== TAT CA TEST PASS ===") << "\n\n";
    return failures ? 1 : 0;
}
