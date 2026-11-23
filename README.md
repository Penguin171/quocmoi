# Wireless MQ5 Gas Monitoring System (STM32F103C8T6 + nRF24L01+ + LCD 20x4)

Two-node wireless gas monitoring system built on **STM32F103C8T6 (Blue Pill)** using bare-metal **CMSIS** in **Keil µVision 5** (no STM32CubeMX required):

- **Transmitter (`TX`):** Reads the **MQ5** gas sensor via `ADC1` (`PA0` through a `10kΩ / 15kΩ` voltage divider) and transmits 8-byte packets over **nRF24L01+ PA+LNA** (`SPI1`).
- **Receiver (`RX`):** Receives packets over **nRF24L01+ PA+LNA** (`SPI1`) and displays ADC value, MQ5 voltage, gas percentage bar, and safety status (`SAFE` / `WARNING` / `DANGER! GAS!` / `NO SIGNAL`) on a **20×4 LCD** in 4-bit parallel mode.

---

## Repository Layout

```text
.
├── README.md                              # Project overview, pinout summary, & quick navigation
├── .gitignore                             # Ignores Keil intermediate builds, Proteus backups, & OS files
├── docs/
│   ├── context.md                         # Full project state, hardware constraints, RF config, & debug log
│   └── wiring-and-setup.md                # Hardware wiring guide, voltage divider math, & CubeMX HAL steps
├── firmware/
│   ├── cmsis/                             # Active standalone CMSIS firmware (Keil C v5, single-file per node)
│   │   ├── tx/
│   │   │   ├── main_tx_keil_only.c        # TX entry point (see warning below)
│   │   │   └── test_tx_carrier_wave.c     # RF CONT_WAVE test (CH 76, 2s ON / 2s OFF)
│   │   ├── rx/
│   │   │   └── main_rx_keil_only.c        # RX entry point (see warning below)
│   │   └── test_cau_a/
│   │       └── main_keil_only.c           # Requirement (a) single-board test: STM32 + MQ5 + LCD 20x4
│   ├── hal_reference/                     # Reference STM32CubeMX HAL implementation (backup, not active)
│   │   ├── common/                        # Shared nRF24L01+ HAL driver (nrf24l01.h / .c)
│   │   ├── tx/                            # HAL TX app (main_tx.c)
│   │   ├── rx/                            # HAL RX app + LCD driver (lcd20x4.h / .c, main_rx.c)
│   │   └── test_cau_a/                    # HAL Requirement (a) app (main_cau_a.c)
│   └── releases/                          # Pre-built .hex binaries for ST-Link flashing
│       ├── mach_phat.hex
│       ├── mach_thu.hex
│       └── test_cau_a.hex
├── projects/
│   └── keil/                              # Portable Keil µVision 5 projects (relative paths + shared RTE)
│       ├── mach_phat.uvprojx              # Builds firmware/cmsis/tx/main_tx_keil_only.c
│       ├── mach_thu.uvprojx               # Builds firmware/cmsis/rx/main_rx_keil_only.c
│       ├── test_cau_a.uvprojx             # Builds firmware/cmsis/test_cau_a/main_keil_only.c
│       └── RTE/                           # STM32F103C8 CMSIS startup & system clock files
└── hardware/
    ├── proteus/
    │   └── quocmoi.pdsprj                 # Proteus 8.15 schematic / simulation file
    └── schematics/
        └── design.pdf                     # Circuit schematic PDF
```

---

## Important Firmware Status Note

> [!WARNING]
> During refactoring, inspection of the imported scratch source files revealed two accidental overwrites in `firmware/cmsis/` (and the corresponding `.hex` builds in `firmware/releases/`):
> 1. `firmware/cmsis/tx/main_tx_keil_only.c` currently contains the same **CONT_WAVE carrier-wave test** code as `firmware/cmsis/tx/test_tx_carrier_wave.c` (rather than the MQ5 ADC + 8-byte TX packet sender).
> 2. `firmware/cmsis/rx/main_rx_keil_only.c` currently contains the same **single-board Requirement (a)** code as `firmware/cmsis/test_cau_a/main_keil_only.c` (rather than the nRF24L01+ RX receiver + diagnostics display).
>
> Restore or re-implement the CMSIS TX/RX source files before flashing production TX/RX firmware. See [`docs/context.md`](docs/context.md) for the exact packet protocol and RF register settings.

---

## Quick Hardware Reference

### MQ5 Sensor (TX only)
- `VCC` → `5V`, `GND` → `GND`, `DO` → unconnected
- Voltage divider (`5V → 3.0V max`): `MQ5 AO` → `R1 (10kΩ)` → **`PA0`** → `R2 (15kΩ)` → `GND`

### nRF24L01+ PA+LNA (identical on TX and RX)
| nRF24L01+ Pin | STM32 Pin | Notes |
| :--- | :--- | :--- |
| `VCC` | `3.3V` | Place a `100µF` electrolytic capacitor directly across `VCC`–`GND` |
| `GND` | `GND` | Common ground |
| `CE` | `PB0` | GPIO Output |
| `CSN` | `PA4` | GPIO Output |
| `SCK` | `PA5` | `SPI1_SCK` |
| `MISO` | `PA6` | `SPI1_MISO` |
| `MOSI` | `PA7` | `SPI1_MOSI` |
| `IRQ` | — | Unconnected |

### LCD 20×4 — 4-bit Parallel Mode (RX / Test Câu A)
| LCD Pin | Signal | STM32 / Rail |
| :--- | :--- | :--- |
| 1, 3, 5, 16 | `VSS`, `VEE`, `RW`, `K` | `GND` |
| 2, 15 | `VDD`, `A` | `5V` |
| 4, 6 | `RS`, `E` | `PB12`, `PB13` |
| 11, 12, 13, 14 | `D4`, `D5`, `D6`, `D7` | `PB14`, `PB15`, `PA8`, `PA9` |

### Synchronized RF Configuration (CMSIS TX & RX)
- **Channel:** `76` (`2476 MHz`) · **Air Data Rate:** `1 Mbps` · **TX Power:** `0 dBm (MAX)` (`RF_SETUP = 0x06`)
- **5-byte Address:** `{ 0xE7, 0xE7, 0xE7, 0xE7, 0xE7 }` · **CRC:** 2-byte (`CONFIG`: TX `0x0E`, RX `0x0F`) · **Auto-ACK:** Disabled (`EN_AA = 0x00`)
- **Fixed 8-byte Payload:** `[0xAA, seq, adc_hi, adc_lo, mv_hi, mv_lo, gas_pct, xor_checksum]`

---

## Building & Flashing (Keil µVision 5)

1. Open any project in [`projects/keil/`](projects/keil/):
   - `mach_phat.uvprojx` (TX)
   - `mach_thu.uvprojx` (RX)
   - `test_cau_a.uvprojx` (Single-board MQ5 + LCD 20×4 test)
2. Press **F7** to build — output `.hex` files are generated in `projects/keil/Objects/`.
3. Flash via **ST-Link V2** (press **F8** in Keil, or flash `.hex` with **STM32 ST-LINK Utility** / **STM32CubeProgrammer** for compatible `CKS32/CS32` boards).

For full project history, troubleshooting logs, and next steps, read [`docs/context.md`](docs/context.md) and [`docs/wiring-and-setup.md`](docs/wiring-and-setup.md).
