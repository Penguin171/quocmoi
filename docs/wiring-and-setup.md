# Thiết Kế Mạch & Hướng Dẫn Cài Đặt Hệ Thống Giám Sát Khí Gas MQ5 Không Dây (STM32F103C8T6 + nRF24L01+ PA+LNA + LCD 20x4)

Tài liệu này thiết kế trọn bộ phần cứng (tận dụng đúng các linh kiện thực tế bạn đang có) và hướng dẫn nạp code trên **Keil C v5 (STM32 HAL)**.

---

## 1. Danh sách linh kiện sử dụng

| Linh kiện | Mạch Phát (TX) | Mạch Thu (RX) | Ghi chú |
| :--- | :---: | :---: | :--- |
| **STM32F103C8T6** (Blue Pill) | 1 | 1 | Dùng nguồn 5V qua chân `5V` hoặc cổng USB |
| **nRF24L01+ PA+LNA** | 1 | 1 | **Bắt buộc cấp 3.3V**, tuyệt đối không cấp 5V |
| **Tụ hóa 100µF** | 1 | 1 | Gắn sát chân `VCC`–`GND` của từng module nRF24L01+ |
| **Cảm biến khí gas MQ5** | 1 | 0 | Cấp nguồn 5V, ngõ ra `AO` qua cầu phân áp |
| **Điện trở 10kΩ** | 1 | 0 | Cầu phân áp `AO` → `PA0` (nối phía trên) |
| **Điện trở 15kΩ** | 1 | 0 | Cầu phân áp `PA0` → `GND` (nối phía dưới) |
| **Màn hình LCD 20x4** (`LM044L` trong Proteus) | 0 | 1 | Giao tiếp song song 4-bit |

> **Lưu ý quan trọng về nguồn cho nRF24L01+ PA+LNA:**
> - Bản PA+LNA tiêu thụ dòng đỉnh lớn (~115mA). Hãy gắn **tụ hóa 100µF** (chân dài `+` vào `3.3V`, chân ngắn có vạch trắng `-` vào `GND`) **ngay sát chân module nRF24L01+**.
> - Code mặc định đặt mức phát `LOW (-12dBm)` ở tốc độ `250kbps` để chạy ổn định ngay cả khi lấy 3.3V từ board Blue Pill. Khi chạy xa và có nguồn 3.3V riêng (như mạch AMS1117-3.3V), bạn có thể đổi lên `MAX` trong `nrf24l01.c`.

---

## 2. Sơ đồ nối dây MẠCH PHÁT (TX)

### 2.1. Cảm biến MQ5 → STM32F103C8T6 (TX)
Vì MQ5 chạy **5V**, điện áp ngõ ra `AO` có thể lên tới **5V**, trong khi chân `PA0` của STM32 chỉ chịu tối đa **3.3V**. Ta dùng cầu phân áp **10kΩ + 15kΩ** để hạ áp:

$$V_{\text{PA0}} = V_{\text{AO}} \times \frac{15\text{k}\Omega}{10\text{k}\Omega + 15\text{k}\Omega} = 0.6 \times V_{\text{AO}} \le 3.0\text{V}$$

```text
MQ5 (AO) ────[ R1 = 10kΩ ]────┬────► STM32 (PA0 / ADC1_IN0)
                              │
                         [ R2 = 15kΩ ]
                              │
                             GND
```

| Chân MQ5 | Nối tới | Ghi chú |
| :--- | :--- | :--- |
| `VCC` | `5V` | Nguồn 5V (chung GND với STM32) |
| `GND` | `GND` | Mass chung |
| `AO` | Đầu vào `R1 (10kΩ)` | Điểm giữa `R1 (10kΩ)` và `R2 (15kΩ)` nối vào **`PA0`** |
| `DO` | Để trống | Không cần dùng |

### 2.2. Module nRF24L01+ PA+LNA → STM32F103C8T6 (TX)

| Chân nRF24L01+ | Nối tới STM32 (TX) | Chức năng |
| :--- | :--- | :--- |
| `VCC` | `3.3V` | **Gắn tụ hóa 100µF (`+` vào `3.3V`, `-` vào `GND`) sát module** |
| `GND` | `GND` | Mass chung |
| `CE` | `PB0` | GPIO Output |
| `CSN` | `PA4` | GPIO Output (Chip Select) |
| `SCK` | `PA5` | `SPI1_SCK` |
| `MOSI` | `PA7` | `SPI1_MOSI` |
| `MISO` | `PA6` | `SPI1_MISO` |
| `IRQ` | Để trống | Không cần dùng |

*(LED tích hợp `PC13` trên board Blue Pill sẽ nháy mỗi lần gửi gói tin thành công.)*

---

## 3. Sơ đồ nối dây MẠCH THU (RX)

### 3.1. Module nRF24L01+ PA+LNA → STM32F103C8T6 (RX)
Nối **giống hệt mạch TX** để dễ kiểm tra và dùng chung thư viện:

| Chân nRF24L01+ | Nối tới STM32 (RX) | Chức năng |
| :--- | :--- | :--- |
| `VCC` | `3.3V` | **Gắn tụ hóa 100µF (`+` vào `3.3V`, `-` vào `GND`) sát module** |
| `GND` | `GND` | Mass chung |
| `CE` | `PB0` | GPIO Output |
| `CSN` | `PA4` | GPIO Output |
| `SCK` | `PA5` | `SPI1_SCK` |
| `MOSI` | `PA7` | `SPI1_MOSI` |
| `MISO` | `PA6` | `SPI1_MISO` |
| `IRQ` | Để trống | Không cần dùng |

### 3.2. Màn hình LCD 20x4 (4-bit) → STM32F103C8T6 (RX)
Toàn bộ các chân chọn đều tránh `PA13/PA14/PA15/PB3/PB4` (chân JTAG/SWD) nên không bao giờ bị lỗi treo chân.

| Chân LCD 20x4 | Tên chân | Nối tới | Ghi chú |
| :---: | :--- | :--- | :--- |
| 1 | `VSS` | `GND` | Mass chung |
| 2 | `VDD` | `5V` | Nguồn 5V |
| 3 | `VEE` (`V0`) | `GND` | Vì bạn chưa có biến trở 10kΩ, nối thẳng `GND` để thấy chữ rõ |
| 4 | `RS` | `PB12` | GPIO Output |
| 5 | `RW` | `GND` | Chỉ ghi dữ liệu |
| 6 | `E` | `PB13` | GPIO Output |
| 7–10 | `D0`–`D3` | Để trống | Chế độ 4-bit không dùng |
| 11 | `D4` | `PB14` | GPIO Output |
| 12 | `D5` | `PB15` | GPIO Output |
| 13 | `D6` | `PA8` | GPIO Output |
| 14 | `D7` | `PA9` | GPIO Output |
| 15 | `A` (LED+) | `5V` | Đèn nền LCD (mạch thật) |
| 16 | `K` (LED-) | `GND` | Đèn nền LCD (mạch thật) |

*(LED tích hợp `PC13` trên board RX sẽ nháy mỗi lần nhận được gói tin hợp lệ từ TX.)*

---

## 4. Hướng dẫn tạo Project Keil C v5 bằng STM32CubeMX

### Bước 1: Tạo project TX (`STM32F103C8Tx`)
1. **SYS**: Debug = `Serial Wire`.
2. **RCC**: Có thể dùng mặc định **HSI** (không cần thạch anh ngoài) hoặc **HSE = Crystal/Ceramic Resonator** (8MHz → Clock 72MHz). Code chạy tốt với cả hai.
3. **ADC1**: Tích chọn **`IN0`** (`PA0`), giữ nguyên mặc định (12-bit, Single Conversion).
4. **SPI1**: Mode = **`Full-Duplex Master`**, Hardware NSS = `Disable`.
   - Data Size = `8 Bits`, First Bit = `MSB First`
   - Prescaler = **`32`** (hoặc `16`, miễn sao tốc độ SPI ≤ 4 MHz cho ổn định trên breadboard)
   - CPOL = `Low`, CPHA = `1 Edge`
5. **GPIO**:
   - `PA4` (`NRF_CSN`): `GPIO_Output`, mặc định `High`.
   - `PB0` (`NRF_CE`): `GPIO_Output`, mặc định `Low`.
   - `PC13`: `GPIO_Output`, mặc định `High` (tắt LED Blue Pill).
6. **Project Manager**: Toolchain/IDE = **`MDK-ARM` (V5)** → bấm **GENERATE CODE**.
7. Chép `firmware/hal_reference/common/nrf24l01.h`, `firmware/hal_reference/common/nrf24l01.c` vào thư mục `Core/Inc` và `Core/Src` của project TX, thêm `nrf24l01.c` vào cây project trong Keil C v5, rồi chép nội dung `firmware/hal_reference/tx/main_tx.c` vào `main.c` và Build (`F7`).

### Bước 2: Tạo project RX (`STM32F103C8Tx`)
1. **SYS**: Debug = `Serial Wire`.
2. **SPI1**: Cấu hình **giống hệt TX** (`Full-Duplex Master`, Prescaler `32`, CPOL `Low`, CPHA `1 Edge`).
3. **GPIO**:
   - `PA4` (`NRF_CSN`): `GPIO_Output`, mặc định `High`.
   - `PB0` (`NRF_CE`): `GPIO_Output`, mặc định `Low`.
   - `PB12, PB13, PB14, PB15, PA8, PA9`: `GPIO_Output`, mặc định `Low` (cho LCD 20x4).
   - `PC13`: `GPIO_Output`, mặc định `High`.
4. **Project Manager**: Toolchain/IDE = **`MDK-ARM` (V5)** → bấm **GENERATE CODE**.
5. Chép `firmware/hal_reference/common/nrf24l01.h`, `firmware/hal_reference/common/nrf24l01.c`, `firmware/hal_reference/rx/lcd20x4.h`, `firmware/hal_reference/rx/lcd20x4.c` vào `Core/Inc` và `Core/Src` của project RX, thêm 2 file `.c` vào Keil C v5, rồi chép nội dung `firmware/hal_reference/rx/main_rx.c` vào `main.c` và Build (`F7`).

---

## 5. Checklist kiểm tra khi chạy thực tế
1. **Cấp nguồn cho RX trước:** LCD 20x4 sẽ hiện dòng `MQ5 GAS MONITOR` và `WAIT FOR SIGNAL...`. Nếu màn hình sáng nhưng không thấy chữ → kiểm tra lại chân `VEE (3)` đã nối `GND` chưa và kiểm tra dây `RS, E, D4..D7`.
2. **Cấp nguồn cho TX:** Đèn LED `PC13` trên TX nháy mỗi 0.5 giây. Nếu TX gửi được và RX nhận được, đèn `PC13` trên RX cũng nháy theo và LCD cập nhật giá trị ADC, điện áp MQ5, mức khí gas (%) và trạng thái (`SAFE` / `WARNING` / `DANGER`).
3. **Nếu RX báo `NO SIGNAL`:**
   - Kiểm tra dây `MISO` (`PA6`) và `MOSI` (`PA7`) xem có bị đấu chéo không (SPI nối thẳng: `MOSI→MOSI`, `MISO→MISO`).
   - Kiểm tra tụ hóa **100µF** đã cắm đúng cực sát chân `VCC–GND` của cả 2 module nRF24L01+ chưa.
   - Đặt 2 module nRF24L01+ PA+LNA cách nhau tối thiểu **0.5 – 1 mét** (đặt quá sát nhau đôi khi bị bão hòa bộ khuếch đại LNA).
