# Release — Pre-built Outputs

Pre-built outputs for the MicroBlaze RISC-V MCU design on the Cmod A7-35T (with 16 KB I/D caches and the UART-bootloader boot flow). Use these directly without rebuilding the Vivado project.

## Files

| File | Description |
|------|-------------|
| `boot.mcs` | **Deployment flash image** — bitstream with the UART bootloader baked into BRAM. Program once (see below); the board then boots stand-alone and accepts app uploads via `tools/upload.py` |
| `top.bit` | Plain bitstream (no bootloader) — what Vitis programs during JTAG Run/Debug |
| `top_wrapper.xsa` | Hardware export (XSA) for creating a Vitis platform |
| `top_wrapper/` | Extracted XSA contents (`top_wrapper.bit` + `top_wrapper.mmi`) used by `tools/make_boot_mcs.sh` / `updatemem` |

## Usage

### One-time board deployment (instructor)

Program `boot.mcs` into the QSPI flash: Vivado **Hardware Manager > Add Configuration
Memory Device** (`mx25l3273f-spi-x1_x2_x4` for Macronix, or `n25q32-3.3v-spi-x1_x2_x4` per the IC3 marking) >
**Program Configuration Memory Device**. Full steps: [Standalone Boot Mode guide](../docs/guides/Standalone-Boot-Mode/Standalone-Boot-Mode.md) §2.

### Create a Vitis Platform

Open Vitis 2025.2 and create a new platform using `top_wrapper.xsa`. Select **standalone** OS and **microblaze_riscv_0** as the processor. (Platforms built from the pre-cache XSA must be recreated.)

### Rebuilding

`boot.mcs` = `top.bit` + `workspace-example/bootloader` ELF merged via `tools/make_boot_mcs.sh`
(uses `updatemem` + `write_cfgmem`). To modify the hardware itself, rebuild the Vivado
project from source with `RISC-V-MCU/recreate_project.tcl`.

> **Status (2026-07-03):** board-verified on real hardware (Macronix MX25L3273F). Full cycle
> confirmed: flash program, x4 fast power-on config (<1 s), UART bootloader, RAM/flash upload,
> and unattended standalone boot. Bitstream uses quad-SPI x4 @ 33 MHz + compression
> (`BITSTREAM.CONFIG` set in `board/Cmod-A7-Master.xdc`).
