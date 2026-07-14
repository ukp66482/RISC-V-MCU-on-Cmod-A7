# Device Overview

<!-- Datasheet-only source: this file provides Chapter 1 of the unified
datasheet (build_datasheet.py). Everything else is pulled from the existing
spec/guide markdown. -->

## Introduction

The Cmod A7-35T RISC-V MCU is a soft microcontroller implemented on the
Digilent Cmod A7-35T module. It combines a MicroBlaze-V (RISC-V) processor
with tightly-coupled memory, cached external SRAM, and QSPI flash, and
provides memory-mapped peripherals: GPIO, timers and PWM, UART, I2C, SPI,
and an ADC.

## Features

| Feature | Description |
|---------|-------------|
| Core | 32-bit RISC-V (RV32IMB), 100 MHz |
| Cache | 16 KB instruction, 16 KB data |
| Tightly-coupled memory | 128 KB: 32 KB bootloader, 32 KB ITCM, 64 KB DTCM |
| External RAM | 512 KB SRAM, cached |
| Flash | 4 MB QSPI |
| GPIO | 28 pins, 4 external interrupts |
| Timers / PWM | 3 × 32-bit timers with interrupts, 3 PWM channels |
| UART | 2 × 16550 UART |
| I2C | Master, 100 kHz |
| SPI | Master, ≈1.6 – 25 MHz, 2 slave selects |
| ADC | 12-bit, 2 external inputs |
| I/O | 48-pin DIP, 3.3 V |
| Boot | Standalone from flash, or JTAG from Vitis |
| Debug | JTAG (IEEE 1149.1)|

## On-Board Components

The MCU is implemented on the module's FPGA; the remaining devices provide
storage, memory, communication, and power.

| Component | Part | Role |
|-----------|------|------|
| FPGA | Xilinx Artix-7 (`xc7a35tcpg236-1`) | Implements the MCU: processor, memories, and peripherals |
| QSPI flash | Macronix MX25L3273F (Micron N25Q032A on older boards) | 4 MB non-volatile storage for the hardware image and the application |
| SRAM | ISSI IS61WV5128BLL-10BLI | 512 KB application memory |
| USB bridge | FTDI FT2232HQ | JTAG and UART over the Micro-USB connector |
| Regulator | Linear Technology LTC3569 | Generates the on-board supply rails |

## System Block Diagram

![System Architecture](../../images/system_architecture.svg)
