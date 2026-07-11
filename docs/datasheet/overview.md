# Device Overview

<!-- Datasheet-only source: this file provides Chapter 1 of the unified
datasheet (build_datasheet.py). Everything else is pulled from the existing
spec/guide markdown. -->

## Introduction

The Cmod A7-35T RISC-V MCU is a soft microcontroller implemented on the
Artix-7 FPGA of the Digilent Cmod A7-35T module. To the programmer it behaves
like a commercial flash-based MCU: firmware is stored in the on-board QSPI
flash and boots automatically in under a second at power-on, and the full AMD
Vitis/JTAG development flow is available on the same board over a single USB
cable, with no jumpers or mode switching.

The system is built around a MicroBlaze-V (RISC-V) processor with a
three-level memory hierarchy — tightly-coupled BRAM (ITCM/DTCM), cached
external SRAM, and QSPI flash storage — and a class-based AXI peripheral
address map.

## Features

- **CPU** — 32-bit RISC-V (RV32IM + bit-manipulation) at 100 MHz; 16 KB
  I-cache + 16 KB D-cache (write-through, 32-byte lines)
- **Memory** — 128 KB block RAM (32 KB bootloader + 32 KB ITCM + 64 KB DTCM,
  single-cycle); 512 KB SRAM for application code and data (cached);
  4 MB QSPI flash (bitstream + application storage)
- **GPIO** — 28 bidirectional pins in four 7-bit groups, plus 4 dedicated
  external interrupt inputs
- **Timers** — 3 × 32-bit general-purpose timers with interrupts; 3 dedicated
  PWM output channels
- **Communication** — 2 × 16550 UART (USB + DIP header); I2C master
  (100 kHz, 50 ns glitch filter); SPI master (software-settable clock
  ≈1.6 – 25 MHz, 2 slave selects)
- **Analog** — 2 external ADC inputs (12-bit XADC, 0–3.3 V range) plus on-die
  temperature and supply monitors
- **I/O** — 48-pin DIP form factor, 3.3 V LVCMOS (not 5 V tolerant)
- **Boot** — standalone boot from flash (< 1 s) via the on-chip bootloader,
  or JTAG load/debug from Vitis; both coexist without reconfiguration
- **Debug** — JTAG breakpoints, single-step, register and memory inspection

## System Block Diagram

![System Architecture](../images/system_architecture.svg)

## Reference Documents

- Digilent *Cmod A7 Reference Manual* — board schematics, connectors, and the
  power tree
- AMD/Xilinx *DS181, Artix-7 FPGAs Data Sheet* — absolute maximum ratings and
  DC/AC characteristics behind Chapter 5
- AMD/Xilinx LogiCORE IP product guides (PG series) — full register maps for
  the AXI peripherals (e.g. PG153 for the Quad SPI controller)
- The per-topic markdown under `docs/` in the course repository — the single
  source this datasheet is built from
