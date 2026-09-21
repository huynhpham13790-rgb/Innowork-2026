/* =============================================================================
 *  Số cell của pack — NGUỒN SỰ THẬT DUY NHẤT.
 *
 *  VÌ SAO CÓ FILE NÀY (QĐ-036, QĐ-038):
 *  Trước đây số cell được viết cứng ở NĂM chỗ trong bốn file khác nhau:
 *  `AI_N_CELLS`, `DS_N_PROBES`, `CC_N_CELLS`, `PM_N_CELLS`, và ngưỡng điện áp
 *  của pack_meter. Đổi cấu hình pack mà sót một chỗ thì chương trình **vẫn
 *  biên dịch, vẫn chạy, vẫn ra số** — chỉ là chỉ số cell lệch nhau giữa các
 *  module. Không có lỗi nào báo, và Lớp 1 sẽ chỉ đúng cell sai.
 *
 *  Đó là chế độ hỏng đắt nhất trong cả dự án này, nên nó bị loại bỏ bằng cấu
 *  trúc chứ không bằng sự cẩn thận: sửa đúng một dòng dưới đây, mọi module
 *  theo sau.
 *
 *  Đổi cấu hình pack thì phải làm gì thêm:
 *    1. Đổi PACK_N_CELLS ở dưới.
 *    2. Kiểm bảng ROM trong ds18b20_offsets.h có đúng những đầu dò đang dán
 *       lên pack không — bảng đó là ánh xạ VẬT LÝ, không suy ra được từ con số.
 *    3. Chạy lại: test/run_test.sh và ai/test_c_vs_python.py.
 *
 *  KHÔNG phải xuất lại trọng số autoencoder: mô hình chạy trên TỪNG CELL MỘT
 *  với 11 đặc trưng thuần tương đối, số cell chỉ đổi cách tính các đại lượng
 *  tham chiếu (trung bình, median, rank, spread), không đổi kích thước mạng.
 *  Đây là lợi ích trực tiếp của QĐ-033.
 * ========================================================================== */
#pragma once

/* 6 = cấu hình đang chạy (đổi 21/09/2026).
   8 = bản dự phòng, vẫn dùng được: đổi số này rồi nạp lại là xong. */
#define PACK_N_CELLS  6

#if (PACK_N_CELLS != 6) && (PACK_N_CELLS != 8)
#error "PACK_N_CELLS chi ho tro 6 hoac 8 — bang ROM va bang offset trong ds18b20_offsets.h chi co hai ban nay"
#endif

/* Khoảng điện áp hợp lý, suy ra từ số cell thay vì viết cứng — để không bao
   giờ xảy ra cảnh đổi số cell mà quên đổi ngưỡng.
     1,90 V/cell : dưới mức này lithium đã cạn kiệt, không còn là pin nữa
     4,35 V/cell : trên mức này là sai, sạc đầy chỉ 4,2 V (chừa biên đo)
   Ngoài khoảng này là hỏng dây hoặc hỏng chip, KHÔNG phải pin bất thường. */
#define PACK_V_MIN   (PACK_N_CELLS * 1.90f)   // 6S: 11,40 V · 8S: 15,20 V
#define PACK_V_MAX   (PACK_N_CELLS * 4.35f)   // 6S: 26,10 V · 8S: 34,80 V
