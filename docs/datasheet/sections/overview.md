# Device Overview

<!-- Datasheet-only source: this file provides Chapter 1 of the unified
datasheet (build_datasheet.py). Everything else is pulled from the existing
spec/guide markdown. -->

## Introduction

The Cmod A7-35T RISC-V MCU is a soft microcontroller implemented on the
Digilent Cmod A7-35T module. It combines a MicroBlaze-V (RISC-V) processor
with tightly-coupled BRAM, cached external SRAM, and QSPI flash, and
provides memory-mapped peripherals: GPIO, timers and PWM, UART, I2C, SPI,
and an ADC.

## Features

| Feature | Description |
|---------|-------------|
| Core | 32-bit RISC-V (RV32IMB), 100 MHz |
| Cache | 16 KB instruction, 16 KB data |
| On-chip RAM | 128 KB: 32 KB bootloader, 32 KB ITCM, 64 KB DTCM |
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

## System Block Diagram

![System Architecture](../../images/system_architecture.svg)
