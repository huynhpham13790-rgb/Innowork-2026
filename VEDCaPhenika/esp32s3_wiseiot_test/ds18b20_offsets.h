/* =============================================================================
 *  Bảng hiệu chỉnh sai số chế tạo của 8 cảm biến DS18B20.
 *
 *  Đo ngày 14/09/2026 (bản thứ hai, có cả cảm biến môi trường): bó CẢ 9 đầu dò
 *  thành một cụm, nhúng **chỉ phần đầu kim loại** vào nước ở nhiệt độ phòng
 *  (~26,3 °C), giữ dây và mối nối trên mặt nước. Đo 2 lần độc lập, mỗi lần
 *  ~6,5 phút / 420 vòng đọc, cả hai lần bus 0,0000 % lỗi.
 *
 *  Hai bảng khớp nhau trong **0,012 °C** — tốt gấp 4 lần tiêu chí 0,05 °C.
 *  Và khớp với bảng đo lần trước (8 con, mẻ khác) trong **0,029 °C** ở MỌI
 *  kênh. Điều này giải quyết chỗ lấn cấn cũ: bản đo lúc để khô trong không khí
 *  từng lệch 0,049 và 0,068 °C ở hai kênh — nay xác định bản KHÔ mới là bản
 *  sai lệch, do bó cụm trong không khí vẫn còn chênh nhiệt giữa lõi và rìa.
 *
 *  VÌ SAO PHẢI NHÚNG NƯỚC, KHÔNG ĐO TRONG KHÔNG KHÍ:
 *  Đo trong không khí cho ra bảng offset **xáo trộn hoàn toàn giữa hai lần
 *  chạy** — có kênh đổi tới 0,359 °C. Sai số chế tạo là hằng số vật lý, không
 *  thể đổi. Thứ đo được lúc đó là chênh lệch nhiệt độ THẬT giữa các vị trí đầu
 *  dò đang nằm, không phải sai số cảm biến. Nước dẫn nhiệt hơn không khí hàng
 *  trăm lần nên mới ép được cả 8 con về cùng một nhiệt độ thật.
 *  Chi tiết: docs/BANG_CHUNG_CAM_BIEN_2026-09-14.md
 *
 *  ⚠️ KHÔNG ĐO LẠI BẢNG NÀY SAU KHI DÁN LÊN PACK — xem QĐ-025.
 *  Bảng này chỉ sửa **sai số của dụng cụ đo**. Nếu hiệu chuẩn lúc đã dán lên
 *  pack, ta sẽ trừ đi luôn cả chênh lệch nhiệt độ THẬT giữa các cell — mà đó
 *  chính là tín hiệu Lớp 1 cần. Tệ hơn: nếu lúc hiệu chuẩn đã có một cell lỗi
 *  đang nóng sẵn, phép hiệu chuẩn sẽ ghi nhận cái lỗi đó thành "bình thường"
 *  và vĩnh viễn không bao giờ phát hiện được nữa.
 *
 *  Thứ tự kênh theo địa chỉ ROM ở dưới, KHÔNG theo thứ tự dò bus. Phải đọc
 *  bằng ROM (`getTempC(rom)`), không dùng `getTempCByIndex()`.
 *
 *  Sinh lại bảng này khi: thay bất kỳ con cảm biến nào. Chạy
 *  test/ds18b20_stress_test hai lần theo quy trình nhúng nước ở trên.
 * ========================================================================== */
#pragma once
#include <stdint.h>
#include "pack_config.h"

#define DS_N_PROBES PACK_N_CELLS

/* Địa chỉ ROM — dán nhãn đúng thứ tự này lên từng sợi dây.
   Đọc theo index thay vì theo ROM sẽ hoán vị dữ liệu giữa các cell mà KHÔNG có
   lỗi nào báo: Lớp 1 vẫn chạy, vẫn ra số, chỉ là sai cell.

   ⚠️ BẢNG NÀY LÀ ÁNH XẠ VẬT LÝ — không suy ra được từ số cell. Đổi
   PACK_N_CELLS mà không kiểm bảng này là đang giả định điều chưa kiểm.

   ✅ NHÃN VẬT LÝ ĐÃ XÁC ĐỊNH 21/09/2026 bằng `test/ds18b20_identify/`: nhúng
   từng đầu dò vào nước lạnh, đối chiếu ROM nào tụt nhiệt. Cả 8 sợi đều khớp
   bảng này, KHÔNG có ROM lạ ⇒ không thay cảm biến nào ⇒ bảng DS_OFFSET dưới
   đây vẫn đúng, không phải hiệu chuẩn lại (QĐ-025). Trên mỗi sợi đã dán giấy
   ghi số 1..8 theo đúng thứ tự bảng này.

   ⚠️ NHƯNG ĐÓ MỚI LÀ "SỢI DÂY ↔ ROM", CHƯA PHẢI "CELL ↔ ROM". Lúc dán đầu dò
   lên pack phải đặt sợi số 1 lên cell 1, số 2 lên cell 2... Không có phép đo
   nào kiểm được việc đó sau khi đã dán, nên làm sai lúc dán là sai vĩnh viễn
   và im lặng.

   ⚠️ GIẢ ĐỊNH KHI XUỐNG 6 CELL (21/09/2026): lấy **P01..P06**, bỏ P07 và P08.
   Đây là lựa chọn theo thứ tự đánh số, KHÔNG phải kết quả đo. Nếu sáu đầu dò
   đang dán lên pack không phải P01..P06 thì phải sửa bảng dưới đây cho khớp
   nhãn thật — sai chỗ này thì mọi thứ phía sau đều sai một cách im lặng.
   ✅ TIẾP XÚC ĐÃ SỬA 21/09. Trước đó bus chập chờn (7/8, sợi nào vừa động tay
   vào thì mất kết nối rồi có khi tự về). Nguyên nhân: có đầu dò cắm nhầm ray
   trên breadboard — ray nguồn bị cắt đôi ở giữa, hai nửa không thông nhau.
   Đã hàn chụm 8 DATA / 8 VDD / 8 GND thành ba bó, mỗi bó ra một dây jump.
   Sau khi sửa: 8/8 ổn định, lỗi đọc 0,04 % (1/2.272), ROM không đổi con nào.
   Chi tiết: docs/BANG_CHUNG_BRINGUP_PHAN_CUNG_2026-09-20.md */
const uint8_t DS_ROM[DS_N_PROBES][8] = {
  { 0x28, 0x30, 0xF1, 0x01, 0x00, 0x00, 0x00, 0x17 },   // P01
  { 0x28, 0xB8, 0xC8, 0x01, 0x00, 0x00, 0x00, 0x2B },   // P02
  { 0x28, 0xFA, 0x81, 0x02, 0x00, 0x00, 0x00, 0xA2 },   // P03
  { 0x28, 0x46, 0xAF, 0x01, 0x00, 0x00, 0x00, 0x0A },   // P04
  { 0x28, 0xEE, 0x07, 0x03, 0x00, 0x00, 0x00, 0xFD },   // P05
  { 0x28, 0xD1, 0xF8, 0x03, 0x00, 0x00, 0x00, 0xFD },   // P06
#if PACK_N_CELLS == 8
  { 0x28, 0x4D, 0x49, 0x04, 0x00, 0x00, 0x00, 0x35 },   // P07 — chỉ bản 8S
  { 0x28, 0x7F, 0xCD, 0x01, 0x00, 0x00, 0x00, 0xE3 },   // P08 — chỉ bản 8S
#endif
};

/* Nhiệt độ đã hiệu chỉnh = số đọc được - DS_OFFSET[i]
   Tổng các hệ số bằng 0, nên phép hiệu chỉnh không làm dịch nhiệt độ TRUNG
   BÌNH của pack — chỉ nắn lại chênh lệch giữa các kênh.

   ⚠️ BẢN 6 CELL PHẢI CĂN LẠI GỐC, KHÔNG ĐƯỢC CẮT BỚT BẢNG 8 CELL.
   Bảng gốc căn theo trung bình của 8 kênh. Giữ nguyên 6 hệ số đầu thì tổng
   thành +0,0406 °C, nghĩa là phép hiệu chỉnh sẽ dịch nhiệt độ trung bình của
   cả pack đi +0,0068 °C. Bản thân độ dịch đó vô hại với Lớp 1 (nó nhìn chênh
   lệch tương đối), nhưng nó phá mất tính chất "hiệu chỉnh không đụng tới giá
   trị tuyệt đối" — mà ngưỡng cứng 60 °C thì lại đọc giá trị tuyệt đối.
   Nên: mỗi hệ số trừ đi trung bình của sáu hệ số (0,006767).
   Chênh lệch GIỮA các kênh không đổi, nên không cần hiệu chuẩn lại bằng nước. */
const float DS_OFFSET[DS_N_PROBES] = {
#if PACK_N_CELLS == 6
  -0.0430f,   // P01
  -0.1653f,   // P02  <- lạnh nhất
  +0.1607f,   // P03  <- nóng nhất
  -0.0498f,   // P04
  +0.0745f,   // P05
  +0.0227f,   // P06
#else
  -0.0362f,   // P01
  -0.1585f,   // P02  <- lạnh nhất
  +0.1675f,   // P03  <- nóng nhất
  -0.0430f,   // P04
  +0.0813f,   // P05
  +0.0295f,   // P06
  -0.0719f,   // P07
  +0.0313f,   // P08
#endif
};

// Độ rộng thật của sai số chế tạo: 0,3260 °C (P03 - P02).
// Datasheet DS18B20 công bố ±0,5 °C nên đây là hàng bình thường, không hỏng.
// Nhưng Lớp 1 nhìn CHÊNH LỆCH TƯƠNG ĐỐI giữa các cell (QĐ-012), nên lệch cố
// định cỡ này trông y hệt "cell P03 lúc nào cũng nóng hơn". Đối chiếu
// ai/models/eval_results.npz: autoencoder bắt lỗi offset 0,5 °C ở tỉ lệ 41,7%
// => không hiệu chỉnh là rước báo động giả vĩnh viễn.
#define DS_OFFSET_SPREAD_C  0.3260f

/* ---------------------------------------------------------------------------
 *  Cảm biến thứ 9 — ĐO NHIỆT ĐỘ MÔI TRƯỜNG, không dán lên cell.
 *
 *  Vì sao cần: đặc trưng của Lớp 1 có phần so cell với môi trường. Trước đây
 *  chỗ này là hằng số 28 °C, tức là một giả định được nhét vào giữa đường dữ
 *  liệu thật. Pack nóng lên 10 °C vì trời nắng sẽ bị đọc nhầm thành pack tự
 *  sinh nhiệt.
 *
 *  ✅ ĐÃ HIỆU CHUẨN 14/09/2026, cùng mẻ nước với 8 con cell — bắt buộc phải
 *  cùng mẻ, vì thứ cần đúng là chênh lệch GIỮA nó và các cell. Hiệu chuẩn
 *  riêng lẻ là vô nghĩa.
 *
 *  Mốc quy chiếu của cả bảng là TRUNG BÌNH 8 CELL (không phải trung bình 9),
 *  để việc thêm/bớt cảm biến môi trường không làm dịch offset của các cell.
 * ------------------------------------------------------------------------- */
const uint8_t DS_ROM_AMBIENT[8] =
  { 0x28, 0x73, 0x4C, 0x04, 0x00, 0x00, 0x00, 0x19 };

#define DS_AMBIENT_OFFSET   +0.0452f
#define DS_AMBIENT_CALIBRATED 1
