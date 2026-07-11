# RISC-V MCU on Cmod A7

Soft-core RISC-V MCU system on the Digilent Cmod A7-35T, used in the NCKU
"Microprocessor Principles and Applications" course. The FPGA design is
provided prebuilt, so students can treat the board as a normal microcontroller
and work purely at the firmware level.

![Digilent Cmod A7-35T](docs/images/cmod-a7-0.png)

## System overview

- CPU
  - MicroBlaze-V, RV32IM + Bitmanip, 100 MHz
  - 16 KB I-cache + 16 KB D-cache
- Memory
  - 128 KB block RAM — 32 KB bootloader + 32 KB ITCM + 64 KB DTCM
  - 512 KB SRAM — application code and data
  - 4 MB QSPI flash — bitstream and application storage
- Peripherals
  - GPIO: 28 pins in 4 groups, plus on-board LED / RGB / button
  - PWM: 3 channels
  - UART: 2 (USB and external)
  - I2C and SPI masters
  - XADC: 12-bit, 2 external channels
  - Timers: 3, with interrupts
  - Interrupt controller: 8 inputs

![System Architecture](docs/images/system_architecture.svg)

## Boot modes

### JTAG mode

For development: run over JTAG from Vitis (**Run/Debug**) — code goes to RAM,
the debugger works, and a power-cycle restores whatever is in flash. See the
**JTAG Debug Mode** chapter of the
[datasheet](docs/datasheet/Cmod_A7_MCU_Datasheet.pdf).

### Standalone boot

To keep a program on the board, deploy it into the QSPI flash — it then starts
automatically at every power-on. The bootloader is AMD/Xilinx's standard SREC
bootloader, embedded in the hardware image. The datasheet's **Standalone
Boot** chapter is in preparation; until then the draft procedure is in
[docs/datasheet/sections/standalone-boot.md](docs/datasheet/sections/standalone-boot.md).

The same ELF works in both modes, and a new board only needs
`release/official_boot.mcs` programmed once with Vivado Hardware Manager.

## Repository layout

```
release/                  prebuilt outputs: official_boot.mcs, boot_srec.bit, top.bit, top_wrapper.xsa
RISC-V-MCU/               Vivado project (recreate_project.tcl)
board/                    Cmod A7 hardware: constraints, board files, KiCad symbol
docs/                     specs (pin / power / IP), guides (JTAG / standalone boot), diagrams
workspace-example/
  showcase/               full-feature demo: SPI+I2C to an ESP32, servo, ADC, interrupts (preloaded on the board)
  app_template/           template for new applications
  mem_bench/              measures the memory-hierarchy latencies in the datasheet
tools/                    maintainer helper scripts (app scaffolding, JTAG run, flash deploy, doc builds)
```

## Rebuilding the hardware

Requires Vivado 2025.2:

```tcl
source RISC-V-MCU/recreate_project.tcl
```

After implementation, export the XSA and create a Vitis platform (standalone,
`microblaze_riscv_0`), or use the prebuilt `release/top_wrapper.xsa`.

## Other documents

- [Unified MCU datasheet](docs/datasheet/Cmod_A7_MCU_Datasheet.pdf) — **the** document: overview, architecture, memory, pinout, peripherals, power, and the JTAG debug walkthrough (chapter sources under [docs/datasheet/sections/](docs/datasheet/sections/))
- [Boot image pipeline](docs/guides/Boot-Image-Pipeline/Boot-Image-Pipeline.md) — how the boot images are built, from block design to power-on
- [Vitis quick reference](docs/guides/README.md) — platform and application concepts, XSDB commands
- [Bus topology diagram](docs/images/dp_ip_topology.svg) — AXI masters, caches, and interconnect wiring

## License

Developed for educational use at National Cheng Kung University (NCKU).
