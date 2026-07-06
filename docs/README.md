# Documentation

All documentation for the RISC-V MCU platform. Each specification and guide
also ships as a PDF next to its markdown source (rendered with `pdf-style.css`).

## Specifications

| Document | Description |
|----------|-------------|
| [IP-Specification/](IP-Specification/Cmod_A7_IP_Peripheral_Reference.md) | Register base addresses, IP parameters, interrupt mapping |
| [Pin-Specification/](Pin-Specification/Cmod_A7_Pin_Specification.md) | DIP pin assignments, directions, electrical characteristics |
| [Power-Specification/](Power-Specification/Cmod_A7_Power_Specification.md) | Power rails, input options, VU pin behavior |

## Guides

| Document | Description |
|----------|-------------|
| [JTAG Debug Mode](guides/JTAG-Debug-Mode/JTAG-Debug-Mode.md) | Develop and debug over JTAG in Vitis (code runs from RAM) |
| [Standalone Boot Mode](guides/Standalone-Boot-Mode/Standalone-Boot-Mode.md) | Deploy to flash with `upload.py`; the boot flow explained |
| [Vitis quick reference](guides/README.md) | Platform and application concepts, XSDB commands |

## Diagrams

| File | Description |
|------|-------------|
| [system_architecture.svg](images/system_architecture.svg) | System overview: CPU, memories, peripherals |
| [dp_ip_topology.svg](images/dp_ip_topology.svg) | Bus and cache topology: AXI masters and interconnects |
