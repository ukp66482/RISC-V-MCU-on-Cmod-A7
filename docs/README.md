# Documentation

The deliverable document is the unified datasheet,
[datasheet/Cmod_A7_MCU_Datasheet.pdf](datasheet/Cmod_A7_MCU_Datasheet.pdf).
It covers the device overview, system architecture, memory hierarchy, pinout,
peripherals, electrical characteristics, power, and the JTAG debug
walkthrough in one PDF. Rebuild it with
`python3 docs/datasheet/build_datasheet.py`.

The chapter sources are the markdown files under
[datasheet/sections/](datasheet/sections/). These files are the single source
of truth; the PDF is a build artifact.

| Chapter source | Contents |
|----------------|----------|
| [sections/overview.md](datasheet/sections/overview.md) | Device overview, features, block diagram |
| [sections/memory.md](datasheet/sections/memory.md) | Memory hierarchy, address map, caches, measured latencies |
| [sections/pins.md](datasheet/sections/pins.md) | DIP pin assignments, directions, electrical characteristics |
| [sections/peripherals.md](datasheet/sections/peripherals.md) | Peripheral capabilities, register base addresses, interrupt mapping |
| [sections/power.md](datasheet/sections/power.md) | Power rails, input options, VU pin behavior |
| [sections/jtag-debug.md](datasheet/sections/jtag-debug.md) | JTAG debug walkthrough (Vitis Unified IDE) |
| [sections/standalone-boot.md](datasheet/sections/standalone-boot.md) | Standalone boot walkthrough (Vitis Unified IDE) |

## Diagrams

| File | Description |
|------|-------------|
| [images/system_architecture.svg](images/system_architecture.svg) | System overview: CPU, memories, peripherals |
| [images/dp_ip_topology.svg](images/dp_ip_topology.svg) | Bus and cache topology: AXI masters and interconnects |

Diagram wiring for the pinout figure is checked against the pin tables with
`python3 tools/check_pinout_diagram.py`.
