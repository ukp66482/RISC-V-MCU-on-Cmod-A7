# Release — Pre-built Outputs

This directory contains pre-built outputs for the MicroBlaze RISC-V MCU
design on the Cmod A7-35T (with 16 KB I/D caches and standalone boot via the
AMD/Xilinx SREC bootloader). These files can be used directly without
rebuilding the Vivado project.

## Files

| File | Description |
|------|-------------|
| `official_boot.mcs` | **Deployment flash image** — bitstream with the SREC bootloader baked into BRAM, plus the preloaded demo application at `0x220000`. Program once (see below); the board then boots stand-alone |
| `boot_srec.bit` | The same bitstream and bootloader as a JTAG image. Programming it over JTAG restarts the bootloader, which reloads the application from flash — equivalent to a power-cycle |
| `top.bit` | Plain bitstream, blank BRAM (no bootloader) — what Vitis programs during JTAG Run/Debug |
| `top_wrapper.xsa` | Hardware export (XSA) for creating a Vitis platform; contains `top_wrapper.mmi` for `updatemem` |

## Usage

### One-time board deployment (instructor)

Program `official_boot.mcs` into the QSPI flash. In the Vivado Hardware
Manager, select **Add Configuration Memory Device**
(`mx25l3273f-spi-x1_x2_x4` for Macronix, or `n25q32-3.3v-spi-x1_x2_x4` per
the IC3 marking), then select **Program Configuration Memory Device**. For
the complete procedure, see the
[standalone-boot draft](../docs/datasheet/sections/standalone-boot.md) §2.

### Create a Vitis Platform

Open Vitis 2025.2 and create a new platform using `top_wrapper.xsa`. Select
**standalone** OS and **microblaze_riscv_0** as the processor. (Platforms
built from the pre-cache XSA must be recreated.)

### Rebuilding

`boot_srec.bit` is `top.bit` with the SREC-bootloader ELF merged via
`updatemem`; `official_boot.mcs` adds the application SREC at `0x220000` via
`write_cfgmem`. The complete procedure is described in the
[Boot Image Pipeline guide](../docs/guides/Boot-Image-Pipeline/Boot-Image-Pipeline.md).
To modify the hardware, rebuild the Vivado project from source with
`RISC-V-MCU/recreate_project.tcl`.

> **Status (2026-07-11):** verified on hardware (Macronix MX25L3273F). The
> full cycle was confirmed: flash programming (range-scoped and verified),
> boot from flash, application copy to SRAM by the SREC bootloader, and
> application execution. The bitstream uses single-width SPI (x1) at 33 MHz
> with compression (`BITSTREAM.CONFIG` in `board/Cmod-A7-Master.xdc`); x1
> was chosen on 2026-07-12 because it boots regardless of the flash
> Quad-Enable bit, which the flash-programming tools clear on every
> app-slot write (an x4 bitstream cannot boot until QE is restored).
