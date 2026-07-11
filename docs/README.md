# Documentation

All documentation for the RISC-V MCU platform. Each specification and guide
also ships as a PDF next to its markdown source (rendered with `pdf-style.css`;
rebuild with `python3 tools/gen_spec_pdfs.py`).

## Specifications

| Document | Description |
|----------|-------------|
| [datasheet/](datasheet/Cmod_A7_MCU_Datasheet.pdf) | **Unified datasheet** — all specs and guides below in one PDF (`build_datasheet.py`) |
| [IP-Specification/](IP-Specification/Cmod_A7_IP_Peripheral_Reference.md) | Peripheral capabilities, register base addresses, interrupt mapping |
| [Pin-Specification/](Pin-Specification/Cmod_A7_Pin_Specification.md) | DIP pin assignments, directions, electrical characteristics |
| [Power-Specification/](Power-Specification/Cmod_A7_Power_Specification.md) | Power rails, input options, VU pin behavior |

## Guides

| Document | Description |
|----------|-------------|
| [JTAG Debug Mode](guides/JTAG-Debug-Mode/JTAG-Debug-Mode.md) | Develop and debug over JTAG in Vitis (code runs from RAM) |
| [Standalone Boot Mode](guides/Standalone-Boot-Mode/Standalone-Boot-Mode.md) | Deploy applications to flash; boots at every power-on (AMD/Xilinx SREC bootloader) |
| [Boot Image Pipeline](guides/Boot-Image-Pipeline/Boot-Image-Pipeline.md) | From block design to power-on: bitstream, BRAM init, `updatemem`, flash layout |
| [Vitis quick reference](guides/README.md) | Platform and application concepts, XSDB commands |

## Diagrams

| File | Description |
|------|-------------|
| [system_architecture.svg](images/system_architecture.svg) | System overview: CPU, memories, peripherals |
| [dp_ip_topology.svg](images/dp_ip_topology.svg) | Bus and cache topology: AXI masters and interconnects |
