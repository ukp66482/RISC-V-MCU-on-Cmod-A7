# RISC-V MCU — Vivado Hardware Design

Tcl rebuild script and IP peripheral reference for the MicroBlaze RISC-V MCU block design on the Cmod A7-35T.

## Files

| File / Directory | Description |
|------------------|-------------|
| `recreate_project.tcl` | **Entry point** — custom wrapper that sets up board files, creates the project, sources `top.tcl`, adds XDC constraints, and creates the HDL wrapper. **This file is manually maintained and will not be overwritten by Vivado.** |
| `top.tcl` | Vivado-generated block design Tcl script — reconstructs the full MicroBlaze RISC-V BD design. Regenerated via `write_bd_tcl -force top.tcl` after design updates. |
| [../docs/IP-Specification/](../docs/IP-Specification/Cmod_A7_IP_Peripheral_Reference.md) | AXI IP list, base addresses, parameters, interrupt mapping |

## Usage

Open Vivado 2025.2 and run:

```tcl
source RISC-V-MCU/recreate_project.tcl
```

The script will:
1. Register the local board files for the Cmod A7-35T
2. Create a Vivado project named `RISC-V-MCU` inside this directory
3. Reconstruct the full MicroBlaze RISC-V block design (via `top.tcl`)
4. Add the XDC constraints from `board/Cmod-A7-Master.xdc`
5. Generate and set the HDL wrapper as the top module
