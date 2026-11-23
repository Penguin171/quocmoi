# Project Context: Hệ Thống Giám Sát Khí Gas MQ5 Không Dây (STM32F103C8T6 + nRF24L01+ PA+LNA + LCD 20x4)

Tài liệu này tổng hợp toàn bộ yêu cầu, cấu hình phần cứng/phần mềm, danh sách file, kết quả kiểm thử và các bước tiếp theo từ phiên làm việc hiện tại.

---

## 1. Mục tiêu đề bài
- **Hệ thống 2 mạch STM32F103C8T6:**
  - **Mạch Phát (TX):** STM32F103C8T6 đọc cảm biến khí gas **MQ5** qua ADC → đóng gói và gửi không dây qua **nRF24L01+ PA+LNA** (SPI1).
  - **Mạch Thu (RX):** STM32F103C8T6 nhận dữ liệu qua **nRF24L01+ PA+LNA** (SPI1) → hiển thị thông số khí gas lên **LCD 20x4** (chế độ 4-bit).
- **Yêu cầu (a):** Đọc giá trị cảm biến khí gas MQ5 và hiển thị lên LCD 20x4.
- **Yêu cầu (b):** Tìm hiểu chuẩn nén **Codec2** và xây dựng chương trình gửi/nhận RF qua SPI của STM32F103 *(hiện tạm gác phần Codec2 để ưu tiên hoàn thiện mạch TX–RX chạy thực tế trước)*.

---

## 2. Môi trường phát triển & Ràng buộc thực tế
- **IDE:** **Keil C v5 (µVision 5)** — máy người dùng **không cài STM32CubeMX**.
- **Kiến trúc code đang dùng:** **CMSIS thuần (`#include "stm32f10x.h"`)**, mỗi mạch chỉ cần **1 file `.c` duy nhất** (tích `CMSIS -> CORE` và `Device -> Startup` trong *Manage Run-Time Environment*, bật `Create HEX File` trong *Options for Target -> Output*).
- **Mạch nạp:** **ST-Link V2**.
  - Lưu ý: Board Mạch Thu dùng chip tương thích (`CKS32/CS32`, IDCODE khác ST chính hãng) nên Keil báo `Not a genuine ST Device` khi nạp trực tiếp bằng `ST-Link Debugger` → nạp file `.hex` bằng `STM32 ST-LINK Utility` / `STM32CubeProgrammer` (hoặc cấu hình *External Tool* trong Keil).
- **Proteus 8.15:**
  - Thư viện bên thứ 3 `TEPNRF24L01.DLL` và `TEPMQGAS.DLL` bị Proteus 8.15 chặn (`failed to authorize`).
  - Linh kiện LCD 20x4 chuẩn trong Proteus là **`LM044L`** (không phải `LM041L` là 16x4).
- **Linh kiện thực tế hiện có:**
  - 2 × `STM32F103C8T6` (Blue Pill)
  - 2 × `nRF24L01+ PA+LNA` (bản trần không vỏ chống nhiễu kim loại, header 2 cột dọc × 4 hàng; **hiện bị mất 1 ăng-ten SMA**)
  - 1 × Cảm biến khí gas `MQ5`
  - 1 × `LCD 20x4` (không dùng module I2C)
  - Tụ hóa `100µF`, điện trở `10kΩ`, điện trở `15kΩ` (không có biến trở 10kΩ cho chân `VEE` LCD)

---

## 3. Sơ đồ đấu nối chuẩn đã kiểm chứng

### 3.1. Cảm biến MQ5 (trên Mạch Phát TX)
- `VCC` → `5V`, `GND` → `GND`, `DO` → *để trống*
- Cầu phân áp hạ `5V → 3.0V` an toàn cho `PA0`:
  `MQ5 (AO)` → **R1 = 10kΩ** → **`PA0`** → **R2 = 15kΩ** → `GND`

### 3.2. Module `nRF24L01+ PA+LNA` (giống nhau trên cả TX và RX)
Nhìn từ **mặt lưng (8 chân kim hướng vào mặt mình, đầu ăng-ten quay sang TRÁI)** — ô pad **VUÔNG** là chân 1 (`GND`) ở góc **TRÊN–PHẢI**:

| Cột TRÁI (gần chip) | Nối Blue Pill | Cột PHẢI (sát mép) | Nối Blue Pill |
| :--- | :--- | :--- | :--- |
| 2. `VCC` | `3.3V` (+ tụ hóa `100µF` sát chân) | 1. `GND` *(pad vuông)* | `G` (`GND`) |
| 4. `CSN` | `A4` (`PA4`) | 3. `CE` | `B0` (`PB0`) |
| 6. `MOSI` | `A7` (`PA7`) | 5. `SCK` | `A5` (`PA5`) |
| 8. `IRQ` | *Để trống* | 7. `MISO` | `A6` (`PA6`) |

### 3.3. Màn hình LCD 20x4 chế độ 4-bit (trên Mạch Thu RX)
- `VSS (1)`, `VEE (3)`, `RW (5)`, `K (16)` → `GND`
- `VDD (2)`, `A (15)` → `5V`
- `RS (4)` → `PB12`, `E (6)` → `PB13`
- `D4 (11)` → `PB14`, `D5 (12)` → `PB15`, `D6 (13)` → `PA8`, `D7 (14)` → `PA9`
- `D0–D3` → *để trống*

---

## 4. Cấu trúc các file trong dự án

### Bộ file CHÍNH đang dùng (Keil C v5 độc lập — không cần STM32CubeMX)
- `firmware/cmsis/tx/main_tx_keil_only.c`: File nguồn Mạch Phát *(⚠️ Lưu ý: bản hiện có trên đĩa đang bị ghi đè giống hệt `test_tx_carrier_wave.c`, cần khôi phục lại code đọc ADC `PA0` + đóng gói 8 byte `0xAA`/XOR + phát SPI1 `nRF24L01+`)*.
- `firmware/cmsis/tx/test_tx_carrier_wave.c`: Chương trình test phát sóng mang liên tục (`CONT_WAVE`, kênh 76, luân phiên 2s phát / 2s tắt) để kiểm chứng tầng RF/PA/LNA.
- `firmware/cmsis/rx/main_rx_keil_only.c`: File nguồn Mạch Thu *(⚠️ Lưu ý: bản hiện có trên đĩa đang bị ghi đè giống hệt `firmware/cmsis/test_cau_a/main_keil_only.c`, cần khôi phục lại code thu SPI1 `nRF24L01+` + màn hình chẩn đoán `CH/CF/RPD/ST/FI/BAD` + timeout mất sóng 3 giây)*.
- `firmware/cmsis/test_cau_a/main_keil_only.c`: Chương trình test câu (a) gộp 1 mạch (STM32 + MQ5 + LCD 20x4, dùng cho Proteus hoặc test cảm biến trực tiếp).

### Project Keil, bản build `.hex`, phần cứng & Bộ file dự phòng (HAL / STM32CubeMX — chưa dùng)
- `projects/keil/`: Project Keil µVision 5 (`mach_phat.uvprojx`, `mach_thu.uvprojx`, `test_cau_a.uvprojx`) kèm `RTE/` dùng đường dẫn tương đối.
- `firmware/releases/`: Các file `.hex` đã biên dịch sẵn (`mach_phat.hex`, `mach_thu.hex`, `test_cau_a.hex`).
- `hardware/proteus/quocmoi.pdsprj` & `hardware/schematics/design.pdf`: Mạch mô phỏng Proteus 8.15 và sơ đồ nguyên lý PDF.
- `docs/wiring-and-setup.md`: Hướng dẫn nối dây & thiết lập chi tiết.
- `firmware/hal_reference/common/nrf24l01.h`, `firmware/hal_reference/common/nrf24l01.c`: Thư viện RF bản HAL.
- `firmware/hal_reference/rx/lcd20x4.h`, `firmware/hal_reference/rx/lcd20x4.c`, `firmware/hal_reference/rx/main_rx.c`: Code RX bản HAL.
- `firmware/hal_reference/tx/main_tx.c`, `firmware/hal_reference/test_cau_a/main_cau_a.c`: Code TX & câu (a) bản HAL.

---

## 5. Cấu hình RF đồng bộ hiện tại (TX & RX)
- **SPI1:** Master, Mode 0 (`CPOL=0, CPHA=0`), 8-bit, MSB first, Prescaler `/256`, có `delay_short()` ở `CSN_Low()` / `CSN_High()` để đảm bảo `t_cwh` trên dây cắm dài.
- **Kênh RF:** `76` (2476 MHz)
- **Tốc độ & Công suất:** `1 Mbps`, `PA MAX (0 dBm)` (`RF_SETUP = 0x06`)
- **Địa chỉ 5 byte:** `{ 0xE7, 0xE7, 0xE7, 0xE7, 0xE7 }`
- **CRC & Auto-ACK:** `2-byte CRC` (`CONFIG`: TX=`0x0E`, RX=`0x0F`), tắt Auto-ACK (`EN_AA = 0x00`), tắt Dynamic Payload (`DYNPD = 0x00`, `FEATURE = 0x00`).
- **Gói tin cố định 8 byte:** `[0xAA, seq, adc_hi, adc_lo, mv_hi, mv_lo, gas_pct, xor_checksum]`.

---

## 6. Trạng thái hiện tại & Kết quả debug trong session
### Đã hoạt động tốt (100%):
- Biên dịch sạch `0 Error(s), 0 Warning(s)` trên Keil C v5.
- Màn hình LCD 20x4 trên Mạch Thu hiển thị rõ nét, đúng 4 dòng.
- Giao tiếp SPI giữa STM32 và `nRF24L01+` thông 100% trên **cả TX và RX**:
  - RX hiển thị chuẩn: `CH:76 CF:0F RPD:0` và `ST:0E FI:11 BAD:0`.
  - TX vượt qua kiểm tra `RF_CH == 76 && CONFIG == 0x0E` và bật cờ `TX_DS` thật (đèn `PC13` nháy 1 nhịp chậm đều; tháo `A6` là đổi kiểu nháy).
- Cảm biến MQ5 đã cấp nguồn và hoạt động.

### Vấn đề phần cứng đang tồn đọng:
- Khi chạy phép test sóng mang liên tục `CONT_WAVE` (`test_tx_carrier_wave.c`, `RF_SETUP = 0x96`, MAX `0 dBm`, bật 2s / tắt 2s) và đã đảo chéo cả 2 module `nRF24L01+ PA+LNA` giữa TX–RX, giá trị **`RPD` bên RX vẫn đứng im ở `0`**.
- **Kết luận:** Lỗi phần cứng ở tầng khuếch đại RF/PA (`RFX2401C`) trên ít nhất 1 trong 2 module `nRF24L01+ PA+LNA` trần (kết hợp thiếu 1 ăng-ten và nguồn 3.3V yếu).

---

## 7. Các bước tiếp theo (Next Steps)
1. **Thay/bổ sung module RF:** Mua 1 cặp `nRF24L01+` bản thường (anten mạch in PCB tích hợp, ăn dòng thấp ~12mA, cắm thẳng lại 7 dây cũ là chạy ngay) hoặc bổ sung `nRF24L01+ PA+LNA` đủ 2 ăng-ten kèm đế ổn áp `5V → 3.3V`.
2. **Nạp lại code TX chính:** Trong project `mach phat`, thay `test_tx_carrier_wave.c` trở lại bằng `main_tx_keil_only.c` → Build (`F7`) → Nạp (`F8`); giữ nguyên `main_rx_keil_only.c` bên RX.
3. **Nghiệm thu hệ thống:** Xác nhận số `#001, #002...` tăng đều trên LCD, đèn `PC13` RX nháy theo nhịp, test xì gas từ bật lửa vào MQ5 (`SAFE` → `WARNING` → `DANGER! GAS!`) và test mất sóng 3 giây (`NO SIGNAL`).
4. **Hoàn thiện yêu cầu (b) – Chuẩn nén Codec2:** Sau khi mạch chạy ổn định, viết phần phân tích lý thuyết / cấu trúc khung nén Codec2 (và mô phỏng truyền frame nén qua SPI nRF24L01+ nếu đề bài yêu cầu).
