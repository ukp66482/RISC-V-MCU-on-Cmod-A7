# Cmod A7-35T Internal IP Peripheral Reference

**Platform:** Xilinx Artix-7 (xc7a35tcpg236-1) — Cmod A7-35T  
**Processor:** MicroBlaze RISC-V  
**System Clock:** 100 MHz (PLL from 12 MHz on-board oscillator via Clocking Wizard)  
**Toolchain:** Vivado & Vitis 2025.2  

---

## 1. Processor Core & System Infrastructure

### 1.1 MicroBlaze RISC-V (`microblaze_riscv_0`)

| Parameter | Value |
|-----------|-------|
| IP Version | `xilinx.com:ip:microblaze_riscv:1.0` |
| Instruction / Data Bus | 32-bit LMB (Local Memory Bus) + AXI4 Data Port |
| Debug Interface | Enabled (`C_DEBUG_ENABLED = 1`) |
| Multiply / Divide | Enabled (`C_USE_MULDIV = 2`) |
| Bit Manipulation | Bitmanip A/B/C/S all enabled |
| Cache | Disabled (both I-Cache and D-Cache) |

**Description:** The main processor core. Executes user firmware and accesses all peripherals through the AXI SmartConnect interconnect.

### 1.2 Local Memory

| Parameter | Value |
|-----------|-------|
| IP | `lmb_v10` (LMB Bus) + `lmb_bram_if_cntlr` + `blk_mem_gen` |
| Data Bus (DLMB) | 0x0000_0000 – 0x0001_FFFF (128 KB) |
| Instruction Bus (ILMB) | 0x0000_0000 – 0x0001_FFFF (128 KB) |
| Memory Type | True Dual-Port Block RAM |
| ECC | Disabled |

**Description:** Instruction and data local memory implemented with FPGA Block RAM. Provides zero-wait-state access for the processor.

### 1.3 AXI SmartConnect (`microblaze_riscv_0_axi_periph`)

| Parameter | Value |
|-----------|-------|
| IP Version | `xilinx.com:ip:smartconnect:1.0` |
| Master Ports | 20 (M00 – M19) |
| Slave Ports | 1 (S00, connected to MicroBlaze M_AXI_DP) |

**Description:** AXI interconnect crossbar that routes processor data transactions to 20 peripheral endpoints.

### 1.4 AXI Interrupt Controller (`microblaze_riscv_0_axi_intc`)

| Parameter | Value |
|-----------|-------|
| IP Version | `xilinx.com:ip:axi_intc:4.1` |
| AXI Base Address | `0x4120_0000` |
| Fast Interrupt | Enabled (`C_HAS_FAST = 1`) |
| Interrupt Sources | 6 (merged via `ilconcat`) |

**Interrupt Mapping:**

| Channel | Source | Description |
|---------|--------|-------------|
| In0 | `timer_0` | System timer 0 interrupt |
| In1 | `timer_1` | System timer 1 interrupt |
| In2 | `timer_2` | System timer 2 interrupt |
| In3 | `uart_1` | External UART interrupt |
| In4 | `uart_USB` | USB UART interrupt |
| In5 | `INT_0_3` | External GPIO interrupt (4-bit) |

### 1.5 Debug Module (`mdm_1`)

| Parameter | Value |
|-----------|-------|
| IP Version | `xilinx.com:ip:mdm_riscv:1.0` |

**Description:** MicroBlaze Debug Module providing JTAG debug access for breakpoints, register inspection, and memory read/write. Its `Debug_SYS_Rst` signal is connected to the system reset module.

### 1.6 Clocking Wizard (`clk_wiz_1`)

| Parameter | Value |
|-----------|-------|
| IP Version | `xilinx.com:ip:clk_wiz:6.0` |
| Input Clock | 12 MHz (on-board oscillator) |
| Output Clock | 100 MHz |
| PLL Multiply Factor | MMCM_CLKFBOUT_MULT_F = 62.5 |
| PLL Divide Factor | MMCM_CLKOUT0_DIVIDE_F = 7.5 |

**Description:** Uses an MMCM/PLL to multiply the 12 MHz board clock to 100 MHz for the entire system. The `locked` signal indicates clock stability.

### 1.7 Processor System Reset (`rst_clk_wiz_1_100M`)

| Parameter | Value |
|-----------|-------|
| IP Version | `xilinx.com:ip:proc_sys_reset:5.0` |
| External Reset | On-board push button (Active Low) |

**Description:** Generates synchronized reset signals (`mb_reset`, `bus_struct_reset`, `peripheral_aresetn`) ensuring all modules are released from reset only after the clock is stable.

---

## 2. UART Communication

### 2.1 USB UART (`uart_USB`)

| Parameter | Value |
|-----------|-------|
| IP Version | `xilinx.com:ip:axi_uart16550:2.0` |
| AXI Base Address | `0x44A0_0000` |
| Address Range | 64 KB (0x44A0_0000 – 0x44A0_FFFF) |
| TX Pin | J18 (via Micro-USB connector) |
| RX Pin | J17 (via Micro-USB connector) |
| Interrupt | Connected to `xlconcat In4` |

**Description:** 16550-compatible UART for host PC communication through the on-board Micro-USB connector. Baud rate is software-configured via the Divisor Latch Register. At 100 MHz system clock, the divisor for 115200 baud is 54 (0x36). Typically mapped as STDIN/STDOUT.

### 2.2 External UART (`uart_1`)

| Parameter | Value |
|-----------|-------|
| IP Version | `xilinx.com:ip:axi_uart16550:2.0` |
| AXI Base Address | `0x44A1_0000` |
| Address Range | 64 KB (0x44A1_0000 – 0x44A1_FFFF) |
| TX Pin | J1 (DIP Pin 11) |
| RX Pin | K2 (DIP Pin 12) |
| Interrupt | Connected to `xlconcat In3` |

**Description:** Second 16550 UART exposed on the DIP connector for communication with external devices (e.g., sensor modules, Bluetooth modules).

---

## 3. GPIO (General Purpose I/O)

All GPIO instances use `xilinx.com:ip:axi_gpio:2.0`.

### 3.1 On-Board GPIO

| Instance | AXI Base Address | Width | Direction | Connection | Description |
|----------|-----------------|-------|-----------|------------|-------------|
| `board_led_2bits` | `0x4000_0000` | 2 | Output | A17, C16 | On-board LEDs × 2 |
| `board_button` | `0x4001_0000` | 1 | Input | A18 | On-board push button × 1 |
| `board_rgb` | `0x4002_0000` | 3 | Output | B17, B16, C17 | On-board RGB LED (R/G/B) |

### 3.2 DIP Connector GPIO (4 Groups × 7-bit)

| Instance | AXI Base Address | Width | DIP Pins | Description |
|----------|-----------------|-------|----------|-------------|
| `gpio_A_0_6` | `0x4003_0000` | 7 | Pin 1–7 | GPIO Group A, bidirectional I/O |
| `gpio_B_0_6` | `0x4004_0000` | 7 | Pin 17–23 | GPIO Group B, bidirectional I/O |
| `gpio_C_0_6` | `0x4005_0000` | 7 | Pin 42–48 | GPIO Group C, bidirectional I/O |
| `gpio_D_0_6` | `0x4006_0000` | 7 | Pin 26–32 | GPIO Group D, bidirectional I/O |

**Description:** Each group is a 7-bit bidirectional port (`C_ALL_OUTPUTS = 0`). Pin direction is set through the TRI register; data is read/written via the DATA register.

### 3.3 External Interrupt Inputs (`INT_0_3`)

| Parameter | Value |
|-----------|-------|
| AXI Base Address | `0x4007_0000` |
| Width | 4-bit (all inputs) |
| Interrupt Capability | Enabled (`C_INTERRUPT_PRESENT = 1`) |
| DIP Pins | Pin 8 (INTR_0), Pin 9 (INTR_1), Pin 41 (INTR_2), Pin 33 (INTR_3) |

**Description:** Four external interrupt inputs grouped into a single AXI GPIO instance. The interrupt signal is routed to `xlconcat In5` and then to the AXI Interrupt Controller.

---

## 4. Timers & PWM

All timer instances use `xilinx.com:ip:axi_timer:2.0`.

### 4.1 System Timers

| Instance | AXI Base Address | Mode | Interrupt | Description |
|----------|-----------------|------|-----------|-------------|
| `timer_0` | `0x41C0_0000` | 32-bit (`Default`) | `xlconcat In0` | General-purpose system timer |
| `timer_1` | `0x41C4_0000` | Default | `xlconcat In1` | General-purpose timer |
| `timer_2` | `0x41C5_0000` | Default | `xlconcat In2` | General-purpose timer |

### 4.2 PWM Outputs

| Instance | AXI Base Address | Output Pin | DIP Pin | Description |
|----------|-----------------|-----------|---------|-------------|
| `PWM_0` | `0x41C1_0000` | J3 | Pin 10 | PWM Channel 0 |
| `PWM_1` | `0x41C2_0000` | W3 | Pin 34 | PWM Channel 1 |
| `PWM_2` | `0x41C3_0000` | W4 | Pin 40 | PWM Channel 2 |

**Description:** AXI Timer instances configured in PWM mode to generate square-wave outputs. Useful for LED dimming, motor speed control, buzzer tone generation, etc. Frequency and duty cycle are configured through the Timer Load Registers and the PWM enable bit.

---

## 5. External Memory

### 5.1 QSPI Flash (`axi_quad_spi_0`)

| Parameter | Value |
|-----------|-------|
| IP Version | `xilinx.com:ip:axi_quad_spi:3.2` |
| AXI Base Address | `0x44A2_0000` |
| Address Range | 64 KB (control register space) |
| Interface | Quad SPI |

**Description:** Controls the on-board Quad-SPI Flash memory. Can be used to store non-volatile data such as configuration files or firmware images.

### 5.2 SRAM / Cellular RAM (`axi_emc_0`)

| Parameter | Value |
|-----------|-------|
| IP Version | `xilinx.com:ip:axi_emc:3.0` |
| AXI Base Address | `0x6000_0000` |
| Address Range | 32 MB (0x6000_0000 – 0x61FF_FFFF) |
| Physical Capacity | 512 KB Cellular RAM |

**Description:** External memory controller providing access to the on-board 512 KB SRAM. Suitable for large data buffers, but access latency is higher than Block RAM.

---

## 6. Analog-to-Digital Conversion (XADC)

### 6.1 XADC Wizard (`xadc_wiz_0`)

| Parameter | Value |
|-----------|-------|
| IP Version | `xilinx.com:ip:xadc_wiz:3.3` |
| AXI Base Address | `0x44A3_0000` |
| Address Range | 64 KB |
| Conversion Rate | 500 KSPS |
| Sequencer Mode | Continuous |

**Enabled Channels:**

| Channel | Source | Description |
|---------|--------|-------------|
| VAUX4 | DIP Pin 15 (G3/G2) | External analog input 0 (on-board divider: 0–3.3 V → 0–1 V) |
| VAUX12 | DIP Pin 16 (H2/J2) | External analog input 1 (on-board divider: 0–3.3 V → 0–1 V) |
| Temperature | Internal | FPGA die temperature monitor |
| VCCINT | Internal | Core voltage monitor (1.0 V) |
| VCCAUX | Internal | Auxiliary voltage monitor (1.8 V) |

**Description:** The Artix-7 built-in 12-bit ADC capable of measuring external analog signals and monitoring internal FPGA temperature and supply voltages. External input pins pass through an on-board resistive voltage divider (2.32 KΩ / 1 KΩ, ratio ≈ 0.301), accepting up to 3.3 V at the DIP pin.

**Effective Per-Channel Sampling Rate:** The sequencer continuously cycles through all 5 enabled channels (VAUX4, VAUX12, Temperature, VCCINT, VCCAUX). The aggregate conversion rate is 500 KSPS, giving each channel an effective rate of 500 K ÷ 5 = **100 KSPS**.

---

## 7. Complete Address Map

| AXI Base Address | Range | Peripheral | IP Type | Category |
|-----------------|-------|------------|---------|----------|
| `0x0000_0000` | 128K / 128K | Local Memory (BRAM) | blk_mem_gen | Memory |
| `0x4000_0000` | 64K | board_led_2bits | axi_gpio | GPIO |
| `0x4001_0000` | 64K | board_button | axi_gpio | GPIO |
| `0x4002_0000` | 64K | board_rgb | axi_gpio | GPIO |
| `0x4003_0000` | 64K | gpio_A_0_6 | axi_gpio | GPIO |
| `0x4004_0000` | 64K | gpio_B_0_6 | axi_gpio | GPIO |
| `0x4005_0000` | 64K | gpio_C_0_6 | axi_gpio | GPIO |
| `0x4006_0000` | 64K | gpio_D_0_6 | axi_gpio | GPIO |
| `0x4007_0000` | 64K | INT_0_3 | axi_gpio | Interrupt |
| `0x4120_0000` | 64K | axi_intc | axi_intc | System |
| `0x41C0_0000` | 64K | timer_0 | axi_timer | Timer |
| `0x41C1_0000` | 64K | PWM_0 | axi_timer | PWM |
| `0x41C2_0000` | 64K | PWM_1 | axi_timer | PWM |
| `0x41C3_0000` | 64K | PWM_2 | axi_timer | PWM |
| `0x41C4_0000` | 64K | timer_1 | axi_timer | Timer |
| `0x41C5_0000` | 64K | timer_2 | axi_timer | Timer |
| `0x44A0_0000` | 64K | uart_USB | axi_uart16550 | Communication |
| `0x44A1_0000` | 64K | uart_1 | axi_uart16550 | Communication |
| `0x44A2_0000` | 64K | axi_quad_spi_0 | axi_quad_spi | Memory |
| `0x44A3_0000` | 64K | xadc_wiz_0 | xadc_wiz | ADC |
| `0x6000_0000` | 32M | axi_emc_0 | axi_emc | Memory |

---
