# RISC-V MCU on Cmod A7

A soft-core RISC-V MCU system built on the **Digilent Cmod A7-35T** (Xilinx Artix-7, xc7a35tcpg236-1), designed for the **NCKU Microprocessor Principles and Applications** course.

## Overview

This project implements a MicroBlaze RISC-V soft processor with a complete peripheral subsystem on an FPGA, giving students hands-on experience with embedded system design, peripheral programming, and hardware–software integration.

### Key Specifications

| Item | Detail |
|------|--------|
| FPGA | Xilinx Artix-7 xc7a35tcpg236-1 |
| Processor | MicroBlaze RISC-V (32-bit, RV32IM + Bitmanip) |
| System Clock | 100 MHz (PLL from 12 MHz on-board oscillator) |
| Local Memory | 128 KB (Block RAM, 128 KB Instruction + Data, shared True Dual-Port) |
| Interconnect | AXI SmartConnect (20 peripheral ports) |
| Toolchain | Vivado & Vitis 2025.2 |

### Peripherals

- GPIO (LEDs, buttons, DIP switches, accent LEDs)
- PWM (Timer/Counter-based)
- UART (115200 baud)
- SPI (on-board SRAM)
- AXI Timers
- Interrupt Controller (6-channel)

## Repository Structure

```
├── Cmod-A7-spec/          # Board documentation
│   ├── IP-Specification/  # IP peripheral reference
│   ├── Pin-Specification/ # Pin mapping & constraints
│   ├── Kicad_symbol/      # KiCad schematic symbol
│   ├── Power-Specification/
│   ├── Cmod-A7-Master.xdc # Constraints file
│   ├── top.tcl            # Vivado project rebuild script
│   ├── top.bit            # Pre-built bitstream
│   └── top_wrapper.xsa    # Hardware export for Vitis
├── Nexys_version/         # Nexys board variant documentation
├── workspace-example/     # Vitis firmware examples
│   ├── GPIO_test/src/     # GPIO peripheral test
│   ├── PWM_test/src/      # PWM servo motor control
│   └── UART_test/src/     # UART communication test
└── .gitignore
```

## Getting Started

### 1. Rebuild the Vivado Project

Open Vivado 2025.2 and run:

```tcl
source Cmod-A7-spec/top.tcl
```

Or use the pre-built bitstream (`Cmod-A7-spec/top.bit`) directly.

### 2. Create a Vitis Application

1. Open Vitis 2025.2 and create a new platform using `Cmod-A7-spec/top_wrapper.xsa`.
2. Create a new application project targeting the MicroBlaze RISC-V processor.
3. Copy source files from one of the examples in `workspace-example/` into your project.
4. Build and program the FPGA.

### 3. Run an Example

The `workspace-example/` directory contains three ready-to-use test programs:

- **GPIO_test** — Toggle LEDs and read button/switch inputs
- **PWM_test** — Drive a servo motor via PWM output
- **UART_test** — Send and receive data over UART

## Documentation

- [IP Peripheral Reference](Cmod-A7-spec/IP-Specification/Cmod_A7_IP_Peripheral_Reference_EN.md)
- [Pin Specification](Cmod-A7-spec/Pin-Specification/Cmod_A7_Pin_Specification.md)

## License

This project is developed for educational use at National Cheng Kung University (NCKU).
