# Vitis Unified Software Platform — Quick Reference

This guide introduces the core concepts of the **AMD Vitis** toolchain used to develop software for MicroBlaze RISC-V processors on FPGA.

## Core Concepts

### XSA (Xilinx Support Archive)

An `.xsa` file is the hardware handoff file exported from **Vivado**. It contains:

- Hardware description (address map, peripherals, memory ranges)
- Bitstream (optional)
- Processor configuration parameters

The XSA is the bridge between hardware design (Vivado) and software development (Vitis). Every Vitis project starts by importing an XSA.

### Platform

A **Platform** wraps an XSA file and provides the Board Support Package (BSP) layer. It defines:

- Which processor to target (e.g. MicroBlaze RISC-V)
- Available libraries and drivers for the peripherals in the hardware design
- Linker script and memory map

A platform can be reused across multiple applications. When the hardware design changes, you update the platform's XSA and rebuild.

### Application

An **Application** is your actual software project (C/C++) that runs on the processor. It is always associated with a platform. Vitis provides templates such as:

- **Hello World** — minimal UART print
- **Memory Tests** — verify DDR/BRAM connectivity
- **Peripheral Tests** — exercise GPIO, SPI, etc.

The build output is an `.elf` file that can be loaded via JTAG or programmed into flash.

### XSDB (Xilinx System Debugger)

XSDB is a **command-line debug and download tool**. Common use cases:

| Command | Description |
|---|---|
| `connect` | Connect to the hardware server |
| `targets` | List available debug targets (processors) |
| `fpga <bitstream>.bit` | Program the FPGA with a bitstream |
| `dow <app>.elf` | Download an ELF binary to the processor |
| `bpadd <addr>` | Set a breakpoint |
| `con` / `stop` | Continue / stop execution |
| `rrd` | Read processor registers |
| `mrd <addr> <count>` | Read memory |

XSDB can be used interactively or scripted with Tcl for automated workflows.

### System Debugger (GUI)

Vitis also provides a graphical debugger built on top of XSDB. It supports:

- Breakpoints, watchpoints, stepping
- Register and memory inspection
- Multi-processor debugging

## Typical Workflow

```
Vivado                          Vitis
┌─────────────┐   Export XSA   ┌─────────────────┐
│ Hardware     │ ────────────► │ Create Platform  │
│ Design       │               │   (BSP + Drivers)│
└─────────────┘               └────────┬──────────┘
                                       │
                                       ▼
                              ┌─────────────────┐
                              │ Create App       │
                              │   (C/C++ code)   │
                              └────────┬──────────┘
                                       │ Build (.elf)
                                       ▼
                              ┌─────────────────┐
                              │ Run / Debug      │
                              │  (JTAG or Flash) │
                              └─────────────────┘
```

## Guides in This Directory

- [JTAG Debug Mode](JTAG-Debug-Mode/JTAG-Debug-Mode.md) — Load and debug applications over JTAG
- [Standalone Boot Mode](Standalone-Boot-Mode/Standalone-Boot-Mode.md) — Program flash for standalone boot