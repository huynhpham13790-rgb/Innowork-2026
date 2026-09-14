# Tự tạo tài khoản Advantech — không cần chờ BTC

Đội Hủ Tiếu · 06/09/2026 · Bán kết **26/09** *(cập nhật 14/09: bản gốc ghi "còn 9 ngày" theo mốc cũ 15/09)*

---

## Tin tốt: có gói dùng thử EdgeHub 90 ngày

Advantech có sẵn gói **free trial EdgeHub 90 ngày, quota 5 thiết bị / 100 parameter, đầy đủ tính năng**. Đội chỉ cần 1 thiết bị và 8 tag nhiệt độ, nên quota này thừa sức — kể cả khi sau này thêm dòng, áp, SOC vẫn còn xa mới chạm 100.

Mốc thời gian cũng vừa đủ đẹp: đăng ký hôm nay 06/09 thì trial hết hạn khoảng 05/12, **phủ được cả Bán kết 26/09 lẫn Chung kết 27/11**. Nhưng chỉ dư 8 ngày sau Chung kết, nên đừng đăng ký muộn hơn tuần này — trễ một tuần là trial chết trước ngày thi.

---

## Bốn tài khoản cần tạo (đều miễn phí)

Advantech dùng SSO, nên một email dùng chung được cho cả bốn. Dùng **email trường (.edu.vn)** nếu có — đăng ký kiểu sinh viên/nghiên cứu thường được duyệt nhanh và dễ xin ưu đãi hơn email cá nhân.

**1. Tài khoản Advantech (nền tảng).** Vào https://wise.advantech.com/en-int → Sign in → Register. Điền tên, email, tổ chức. Ở ô công ty ghi rõ *Thai Nguyen University of Information and Communication Technology (ICTU)*, ở ô mục đích ghi *AIoT InnoWorks 2026 competition — student team project*. Nói thẳng là đội thi InnoWorks có lợi, vì InnoWorks là chương trình của chính Advantech.

**2. Gói dùng thử EdgeHub.** Sau khi đăng nhập, vào https://wise.advantech.com/en-int/marketplace/products/advantech.edgehub → nút **Free Trial / Redeem**. Trang này render bằng JavaScript nên tớ không đọc trực tiếp được nội dung nút, bạn mở lên xem tận nơi. Nếu nó bắt điền form liên hệ thay vì kích hoạt ngay, cứ điền — và ghi rõ dòng "InnoWorks 2026 finalist team, need access before Sep 15 semifinal" vào ô ghi chú. Deadline cụ thể làm người duyệt xử lý nhanh hơn nhiều so với một yêu cầu chung chung.

**3. IoT Academy** — https://academy.advantech.com/ → Sign up. Hoàn toàn miễn phí, không cần duyệt. Khóa nên học trước: *WISE-PaaS Core Level I* và *WISE-IoTSuite/Dashboard L1*. BTC bảo chưa cần chứng chỉ, nhưng có sẵn chứng chỉ khi thuyết trình là thứ chứng minh được ngay, không tốn gì ngoài vài buổi tối.

**4. Forum** — https://forum.wise-paas.advantech.com/. Tạo account để hỏi kỹ thuật. Đây cũng là nơi hỏi nhanh nhất khi kẹt credential.

---

## Nếu trial phải chờ duyệt

Khả năng cao là có, vì đường free trial của Advantech thường đi qua bước "AIoT expert liên hệ lại". Đó là lý do sketch tớ viết hôm qua chia làm 2 giai đoạn: **STAGE 1 chạy được ngay hôm nay trên broker công cộng, không phụ thuộc gì vào Advantech**. Cứ chạy STAGE 1 song song trong lúc chờ; khi credential về thì đổi 4 dòng.

Thêm một đường nữa để rút ngắn thời gian chờ: hỏi lại BTC xem họ có kênh liên hệ trực tiếp với Advantech Việt Nam không. Một email giới thiệu từ BTC thường nhanh hơn form đăng ký công khai vài ngày.

---

## Về ý định ghi điểm với giám khảo

Ý tưởng dùng Advantech dù chưa bắt buộc là hợp lý, nhưng nên cân nhắc chỗ này: một dashboard dựng vội, dữ liệu giả, không nói lên nhiều điều. Cái thực sự tạo khác biệt ở vòng đầu là **bạn đã tự giải mã được giao thức MQTT của WISE-IoT và cho ESP32 nối thẳng, bỏ được tầng Raspberry Pi** — kéo BOM từ 2–3 triệu xuống 400–500k mỗi xe. Đó là một quyết định kỹ thuật có con số đằng sau, và nó chính là kiểu lập luận đã giúp đội vô địch 2025 thắng.

Nên nếu trial chưa kịp về trước 26/09, đừng coi là hỏng. Slide kiến trúc vẫn vẽ WISE-IoT ở tầng cloud, kèm demo ESP32 đang bắn đúng topic và đúng payload của WISE-IoT trên một broker MQTT — nói rõ là đang chờ cấp tài khoản. Giám khảo Advantech sẽ nhận ra ngay format đó là của họ, và điều đó thuyết phục hơn một dashboard đẹp mà bên dưới không có gì.

---

## Việc hôm nay

Đăng ký cả bốn tài khoản (khoảng 20 phút), bấm redeem EdgeHub trial, nhắn BTC hỏi kênh liên hệ Advantech VN. Rồi quay lại cắm ESP chạy STAGE 1 — việc đó không chờ ai cả.

---

**Nguồn:**
- [EdgeHub — WISE Marketplace](https://wise.advantech.com/en-int/marketplace/products/advantech.edgehub)
- [Try Our Products for Free — Advantech WISE](https://wise.advantech.com/en-int/free-trial)
- [Advantech IoT Academy](https://academy.advantech.com/)
- [WISE-PaaS Core Level I](https://academy.advantech.com/catalog/info/id:499) · [WISE-IoTSuite/Dashboard L1](https://academy.advantech.com/catalog/info/id:1597)
- [WISE Developer Community (Forum)](https://forum.wise-paas.advantech.com/)
