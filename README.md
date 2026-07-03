# RISC-V MCU on Cmod A7

A soft-core RISC-V MCU system built on the **Digilent Cmod A7-35T** (Xilinx Artix-7, xc7a35tcpg236-1), designed for the **NCKU Microprocessor Principles and Applications** course.

## Overview

A ready-to-use RISC-V MCU environment on the Cmod A7-35T for the **NCKU Microprocessor Principles and Applications** course. Students focus on firmware — register programming, interrupts, peripheral control — without dealing with FPGA or circuit-level details.

![Digilent Cmod A7-35T](docs/images/cmod-a7-0.png)

### System Architecture

![System Architecture](docs/images/system_architecture.svg)

### Bus & Cache Topology

![Bus & Cache Topology](docs/images/dp_ip_topology.svg)

MicroBlaze exposes four AXI masters — **DP/IP** (uncached data/instruction) and **DC/IC** (the cache masters). DP reaches all peripheral registers through the 20-port `axi_periph` crossbar and is never cached; the external **SRAM** (the application execution region) sits behind `smartconnect_0`, where all four masters converge, and is served by the 16 KB I/D caches. The QSPI flash is register-controlled (`0x40500000`), so the CPU can erase/program it — this is what powers the UART bootloader and `upload.py`. BRAM sits on a separate 1-cycle LMB path outside the cache, holding the bootloader and the application stack.

### Key Specifications

| Item | Detail |
|------|--------|
| FPGA | Xilinx Artix-7 xc7a35tcpg236-1 |
| Processor | MicroBlaze RISC-V (RV32IM + Bitmanip, 5-stage pipeline) |
| System Clock | 100 MHz (PLL from 12 MHz oscillator) |
| L1 Cache | 16 KB I-cache + 16 KB D-cache (write-through, 32 B lines, over SRAM) |
| Local Memory | 128 KB Block RAM (LMB, 1-cycle) — lower 64 KB bootloader, upper 64 KB app stack |
| Program Memory | 512 KB external SRAM (cached) — applications execute here |
| Storage | 4 MB QSPI flash — bitstream + persistent application slot |
| Interconnect | AXI SmartConnect (20 peripheral ports) |
| Toolchain | Vivado & Vitis 2025.2 (day-to-day student use: Python + pyserial only) |

![Cmod A7-35T DIP Pinout](Cmod-A7-spec/Pin-Specification/images/pinout_diagram.png)

### Peripherals

- **GPIO** — 4 × 7-bit DIP groups (A–D), on-board LEDs × 2, RGB LED, push button
- **PWM** — 3 channels (DIP Pin 10 / 34 / 40)
- **UART** — 2 × 16550 (USB + DIP external)
- **Timers** — 3 × 32-bit with interrupt
- **Interrupt Controller** — 6-channel AXI INTC
- **XADC** — 12-bit, 500 KSPS (2 external analog inputs)
- **QSPI Flash** — 4 MB NOR, CPU-programmable (bitstream + persistent app storage)
- **SRAM** — 512 KB external cellular RAM (I/D-cached; application program memory)

### Memory Map

Peripherals follow a class-based scheme — **`0x40[C]x_xxxx`, where `C` is the class** — so an address tells you the device type at a glance.

| Region | Address | Notes |
|--------|---------|-------|
| Block RAM (LMB) | `0x0000_0000` – `0x0001_FFFF` | 128 KB, 1-cycle, not cached; lower 64 KB bootloader, upper 64 KB stack |
| GPIO (class 0) | `0x4000_0000` – `0x4007_0000` | LED/RGB/Button, DIP A–D, external interrupts |
| Timer (class 1) | `0x4010_0000` – `0x4012_0000` | timer_0 / 1 / 2 |
| PWM (class 2) | `0x4020_0000` – `0x4022_0000` | PWM_0 / 1 / 2 |
| UART (class 3) | `0x4030_0000` – `0x4031_0000` | uart_USB / uart_1 (16550) |
| INTC (class 4) | `0x4040_0000` | AXI interrupt controller |
| QSPI ctrl (class 5) | `0x4050_0000` | flash register interface (erase/program) |
| XADC (class 6) | `0x4060_0000` | 12-bit ADC |
| SRAM | `0x6000_0000` – `0x6007_FFFF` | 512 KB, I/D-cached, application execution region |
| QSPI Flash | *(not memory-mapped)* | 4 MB storage — bitstream + app image, accessed via the QSPI controller |

## Repository Structure

```
├── release/                    # Pre-built outputs (boot.mcs, top.bit, top_wrapper.xsa)
├── RISC-V-MCU/                 # Vivado project (recreate_project.tcl, top.tcl, IP reference)
├── Cmod-A7-spec/               # Board files (XDC constraints, pin/power specs)
├── Vitis-Software-Dev-Guide/   # Vitis guides (JTAG debug, standalone boot)
├── workspace-example/          # Firmware (examples, bootloader, SRAM app template)
├── tools/                      # upload.py (UART app upload), make_boot_mcs.sh
└── Intro_PPT/                  # Course introduction slides
```

## Prerequisites

- **Vivado 2025.2** — for synthesizing/implementing the FPGA design and programming flash
- **Vitis 2025.2** — for creating the hardware platform and developing firmware
- **Python 3 + pyserial** — all a student needs for day-to-day work: build once, then `tools/upload.py app.elf` over USB (no Vivado/Vitis in the loop)

## Getting Started

There are two ways to get code onto the board — they coexist on the same bitstream:

| | JTAG Debug Mode | Standalone Boot (UART bootloader) |
|---|---|---|
| Best for | development & debugging | deployment ("ship it") |
| Code goes to | RAM (volatile) | QSPI flash (boots at every power-on) |
| Tooling | Vivado + Vitis | `python3 tools/upload.py app.elf` |
| Guide | [JTAG Debug Mode](Vitis-Software-Dev-Guide/JTAG-Debug-Mode/JTAG-Debug-Mode.md) | [Standalone Boot Mode](Vitis-Software-Dev-Guide/Standalone-Boot-Mode/Standalone-Boot-Mode.md) |

The fastest way to get up and running is **JTAG Debug Mode** — load and run firmware directly over USB without touching flash memory.

### 1. Rebuild the Vivado Block Design

Open Vivado 2025.2 and rebuild the hardware design from the Tcl script:

```tcl
source RISC-V-MCU/recreate_project.tcl
```

This registers the board files, creates the project, reconstructs the full block design (MicroBlaze RISC-V processor and all peripherals), adds XDC constraints, and generates the HDL wrapper. After synthesis and implementation, export the hardware as an `.xsa` file for Vitis. Alternatively, use the pre-built `release/top_wrapper.xsa` directly.

### 2. Create a Vitis Platform and Application

1. Open Vitis 2025.2 and create a new platform using `release/top_wrapper.xsa`.
2. Select **standalone** OS and **microblaze_riscv_0** as the processor. Build the platform.
3. Create a new application from the **Hello World** template.
4. Copy source files from one of the examples in `workspace-example/`, or write your own.
5. Build the application to produce an `.elf` file.

### 3. Run and Debug over JTAG

1. Connect the Cmod A7-35T to your computer via USB.
2. Click **Run** in the FLOW panel — Vitis will automatically program the FPGA and execute your application.
3. Click **Debug** to enter a GDB-like interactive session with breakpoints, stepping, and register/memory inspection.

For detailed step-by-step instructions with screenshots, see the [JTAG Debug Mode Guide](Vitis-Software-Dev-Guide/JTAG-Debug-Mode/JTAG-Debug-Mode.md).

### Example Programs

The `workspace-example/` directory contains ready-to-use programs:

- **Btn_LED_asm_test** — Button & LED control in RISC-V assembly
- **GPIO_test** — Toggle LEDs and read button/switch inputs
- **PWM_test** — Drive a servo motor via PWM output
- **UART_test** — Send and receive data over UART
- **ADC_test** — Read the XADC analog inputs
- **MemoryRead_test** — Measure BRAM / SRAM access latency with a hardware timer
- **SRAM_app_template** — Starting point for standalone-boot apps (SRAM layout, works over JTAG too)
- **bootloader** — The UART flash bootloader baked into `release/boot.mcs` (instructor-maintained; great loader/SPI-flash reading material)

## Documentation

### MCU Design Reference

| Document | Description |
|----------|-------------|
| [IP Peripheral Reference](RISC-V-MCU/IP-Specification/Cmod_A7_IP_Peripheral_Reference.md) | Full AXI IP list, base addresses, parameters, interrupt mapping |

### Board Specification

Hardware documentation in [`Cmod-A7-spec/`](Cmod-A7-spec/):

| Document | Description |
|----------|-------------|
| [Pin Specification](Cmod-A7-spec/Pin-Specification/Cmod_A7_Pin_Specification.md) | DIP connector pin map, GPIO/PWM/UART/ADC assignments, electrical characteristics |
| [Power Specification](Cmod-A7-spec/Power-Specification/Cmod_A7_Power_Specification.md) | Power rails, input options, VU pin behavior, dual-supply considerations |

### Vitis Guides

Step-by-step guides for software development in [`Vitis-Software-Dev-Guide/`](Vitis-Software-Dev-Guide/):

| Guide | Description |
|-------|-------------|
| [Vitis Core Concepts](Vitis-Software-Dev-Guide/README.md) | Platform, Application, XSDB, and workflow overview |
| [JTAG Debug Mode](Vitis-Software-Dev-Guide/JTAG-Debug-Mode/JTAG-Debug-Mode.md) | Load and debug applications over JTAG |
| [Standalone Boot Mode](Vitis-Software-Dev-Guide/Standalone-Boot-Mode/Standalone-Boot-Mode.md) | Program flash for standalone boot |

## License

This project is developed for educational use at National Cheng Kung University (NCKU).
