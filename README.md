# RISC-V MCU on Cmod A7

This repository provides a soft-core RISC-V MCU system on the Digilent Cmod
A7-35T, used in the NCKU "Microprocessor Principles and Applications" course.
The FPGA design is
provided prebuilt, so students can treat the board as a normal microcontroller
and work entirely at the firmware level.

![Digilent Cmod A7-35T](docs/images/cmod-a7-0.png)

## System overview

- CPU
  - MicroBlaze-V, RV32IM + Bitmanip, 100 MHz
  - 16 KB I-cache + 16 KB D-cache
- Memory
  - 128 KB block RAM: 32 KB bootloader + 32 KB ITCM + 64 KB DTCM
  - 512 KB SRAM: application code and data
  - 4 MB QSPI flash: bitstream and application storage
- Peripherals
  - GPIO: 28 pins in 4 groups, and the on-board LEDs, RGB LED, and buttons
  - PWM: 3 channels
  - UART: 2 (USB and external)
  - I2C and SPI masters
  - XADC: 12-bit, 2 external channels
  - Timers: 3, with interrupts
  - Interrupt controller: 8 inputs

![System Architecture](docs/images/system_architecture.svg)

## Boot modes

### JTAG mode

JTAG mode is intended for development. Run the application over JTAG from
Vitis (**Run/Debug**): code is loaded into RAM and executes under debugger
control, and a power cycle restores the application stored in flash. See the
JTAG Debug Mode chapter of the
[datasheet](docs/datasheet/Cmod_A7_MCU_Datasheet.pdf).

### Standalone boot

To retain a program on the board, deploy it into the QSPI flash. The program
then starts automatically at every power-on. The bootloader is AMD/Xilinx's
standard SREC bootloader, embedded in the hardware image. The complete
walkthrough is the Standalone Boot chapter of the datasheet
([docs/datasheet/sections/standalone-boot.md](docs/datasheet/sections/standalone-boot.md)),
including the one-time step that prepares a new board.

The same ELF file runs in both modes.

## Repository layout

```
release/                  prebuilt outputs: boot_srec.bit, top.bit, top_wrapper.xsa, top_wrapper.mmi
RISC-V-MCU/               Vivado project (recreate_project.tcl)
board/                    Cmod A7 hardware: constraints, board files, KiCad symbol
docs/                     specs (pin / power / IP), guides (JTAG / standalone boot), diagrams
workspace-example/
  showcase/               full-feature demo: SPI+I2C to an ESP32, servo, ADC, interrupts (preloaded on the board)
  app_template/           template for new applications (C)
  asm_template/           template for RISC-V assembly applications (.S)
  mem_bench/              measures the memory-hierarchy latencies in the datasheet
tools/                    maintainer helper scripts (app scaffolding, JTAG run, flash deploy, doc builds)
```

## Rebuilding the hardware

Rebuilding the hardware requires Vivado 2025.2:

```tcl
source RISC-V-MCU/recreate_project.tcl
```

After implementation, export the XSA and create a Vitis platform (standalone,
`microblaze_riscv_0`), or use the prebuilt `release/top_wrapper.xsa`.

## Other documents

- [Unified MCU datasheet](docs/datasheet/Cmod_A7_MCU_Datasheet.pdf): the primary reference, covering overview, architecture, memory, pinout, peripherals, power, and the JTAG debug and standalone boot walkthroughs (chapter sources under [docs/datasheet/sections/](docs/datasheet/sections/))
- [Bus topology diagram](docs/images/dp_ip_topology.svg): AXI masters, caches, and interconnect wiring

## License

This project is developed for educational use at National Cheng Kung University (NCKU).
