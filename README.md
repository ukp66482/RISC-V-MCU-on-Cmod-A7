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

## Using the board

For development, build the application in Vitis and run it over JTAG — see the
[JTAG Debug Mode guide](Vitis-Software-Dev-Guide/JTAG-Debug-Mode/JTAG-Debug-Mode.md).
Code runs from RAM and is lost at power-off.

To keep a program on the board, write it to flash over the USB serial port:

```
python3 tools/upload.py build/app.elf
```

The application then starts automatically at every power-on. Details are in the
[Standalone Boot Mode guide](Vitis-Software-Dev-Guide/Standalone-Boot-Mode/Standalone-Boot-Mode.md).
The same ELF works in both cases, and a new board only needs `release/boot.mcs`
programmed once with Vivado Hardware Manager (guide, section 2).

## Repository layout

```
release/                  prebuilt outputs: boot.mcs, top.bit, top_wrapper.xsa
RISC-V-MCU/               Vivado project (recreate_project.tcl) and IP reference
Cmod-A7-spec/             board files, constraints, pin/power specs, KiCad symbol
Vitis-Software-Dev-Guide/ JTAG and standalone-boot guides
workspace-example/
  demo_all/               demo that exercises all peripherals (preloaded on the board)
  examples/01...06        course examples: GPIO, assembly, PWM, UART, ADC, memory
  SRAM_app_template/      template for new applications
  bootloader/             UART flash bootloader source
tools/                    upload.py, make_boot_mcs.sh
```

## Rebuilding the hardware

Requires Vivado 2025.2:

```tcl
source RISC-V-MCU/recreate_project.tcl
```

After implementation, export the XSA and create a Vitis platform (standalone,
`microblaze_riscv_0`), or use the prebuilt `release/top_wrapper.xsa`.

## Other documents

- [Pin specification](Cmod-A7-spec/Pin-Specification/Cmod_A7_Pin_Specification.md) — DIP pin assignments and electrical characteristics
- [Power specification](Cmod-A7-spec/Power-Specification/Cmod_A7_Power_Specification.md) — power rails and supply options
- [Vitis quick reference](Vitis-Software-Dev-Guide/README.md) — platform and application concepts, XSDB commands
- [Bus topology diagram](docs/images/dp_ip_topology.svg) — AXI masters, caches, and interconnect wiring

## License

Developed for educational use at National Cheng Kung University (NCKU).
