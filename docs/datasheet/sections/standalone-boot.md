# Standalone Boot Mode

This guide describes how firmware is stored on the board and started at
power-on. The application resides in the QSPI flash. A small bootloader
(AMD/Xilinx's standard `srec_spi_bootloader`, part of the hardware image)
copies it into SRAM and starts it, with no PC attached, in under a second
after power is applied.

---

## 1. How Standalone Boot Works

```
Power-on ──► FPGA configures itself from QSPI flash (<1 s)
        ──► Bootloader (in Block RAM, part of the hardware image) starts
        ──► Reads the application image (SREC) from flash @ 0x220000
        ──► Copies it to the addresses the image names (SRAM)
        ──► Jumps to the application's entry point
```

The application is stored as an SREC image (Motorola S-record), a text format
in which every line carries its own destination address. The bootloader copies
each record to the address it names, so the linker script decides where the
program runs. The course template links to SRAM at `0x6000_0000`, so the
image is loaded at that address.

Memory roles:

| Memory | Size | Role |
|---|---|---|
| QSPI flash | 4 MB | hardware image + bootloader (first 2.2 MB) · application slot @ `0x220000` (non-volatile) |
| SRAM @ `0x60000000` | 512 KB | where your application executes (code/data/heap, I/D-cached) |
| BRAM @ `0x00000000` | 128 KB | 32 KB bootloader (untouchable) · 32 KB ITCM @ `0x8000` (your fast code) · 64 KB DTCM @ `0x10000` (stack + fast data) |

> **Note:** The bootloader is part of the hardware image and is restored from
> flash at every power-on. Like a mask ROM, it cannot be corrupted from
> software.

---

## 2. One-Time Board Setup (Instructor)

Program the deployment image `release/official_boot.mcs` (hardware image +
bootloader + preloaded demo application) into the QSPI flash:

1. Open Vivado **Hardware Manager > Open Target > Auto Connect**.
2. Right-click `xc7a35t_0` > **Add Configuration Memory Device** and select
   the part matching the IC3 chip: `mx25l3273f-spi-x1_x2_x4` (the Vivado
   catalog entry covering the board's Macronix MX25L3233F; verified working) or
   `n25q32-3.3v-spi-x1_x2_x4` (Micron, older boards).
3. Right-click the memory device > **Program Configuration Memory Device**,
   select `release/official_boot.mcs`, keep **Erase / Program / Verify**
   checked, and click **OK**.
4. Power-cycle. The preloaded demo application starts (its banner appears on
   the USB serial console at 115200 baud).

Students do not repeat this step; from this point on, the board operates as a
self-contained MCU.

---

## 3. Build Your Application

Create a Vitis application as in the
[JTAG chapter](jtag-debug.md) (sections 1–7), using the
course template `workspace-example/app_template/src/`, whose `main.c` and
`lscript.ld` replace the generated Hello World sources.

The template's linker script places:

- `.text` / `.rodata` / `.data` / `.bss` / heap in SRAM (512 KB, cached)
- the stack at the top of DTCM (64 KB BRAM @ `0x10000`; 1-cycle, never collides with the bootloader)
- functions tagged `ITCM_FUNC` in ITCM (32 KB BRAM @ `0x8000`); variables tagged `DTCM_DATA` in DTCM

ITCM code is carried inside the SRAM image and is copied out by `mcu_init()`
at the top of `main()`; this is the same startup-copy idiom an STM32H7 uses
for its ITCM. For this reason, `mcu_init();` must remain the first line of
`main()`. It also sets the UART to 115200. Place interrupt handlers and
timing-critical loops in ITCM: TCM accesses never miss, so worst-case latency
equals best-case latency.

The template prints a memory tour at boot, which shows the layout directly:

```
RISC-V MCU on Cmod A7 - memory tour
  main()       @ 0x600009D0   (SRAM,  cached)
  blink_step() @ 0x00008000   (ITCM,  1-cycle)
  blink_count  @ 0x00010000   (DTCM,  1-cycle)
  stack        @ 0x0001FFC0   (DTCM,  grows down)
```

> **Note:** The same ELF works unchanged in both modes: Vitis Run/Debug over
> JTAG during development, and flash deployment for release. No rebuild is
> required.

---

## 4. Deploy to Flash

Deployment consists of converting the built ELF to SREC and writing it into
the flash application slot at `0x220000`:

1. **Convert to SREC** with the toolchain's `objcopy`
   (`riscv32-amd-linux-gnu-objcopy -O srec app.elf app.srec`). The record
   addresses come from the linker script; with the course template they all
   fall in SRAM.
2. **Write the SREC file's bytes to flash offset `0x220000`** over JTAG. The
   erase and write are scoped to the application slot, so the hardware image
   at the start of flash is never touched.
3. **Power-cycle (or reboot from flash)**. The bootloader loads the new
   image.

(A step-by-step GUI walkthrough with screenshots is to be added.)

The slot extends from the end of the bitstream region to the end of the flash:
1.875 MB, which is larger than the SREC output of a full-SRAM application
(SREC is about 3× the binary size). In practice the size limit is the 512 KB SRAM.
Only code and initialized data count toward it; zero-initialized data and the
heap take no space in the image.

---

## 5. Boot and Verify

- Power-cycle the board. The flashed application boots automatically; no PC
  is required.
- `xil_printf` (the SDK's lightweight `printf`) output arrives on the same USB
  cable at 115200 baud. To view it, open any serial terminal, for example
  `python3 -m serial.tools.miniterm /dev/ttyUSB1 115200` (included with
  pyserial; quit with Ctrl-]). The board enumerates as two ports; the UART is
  usually the higher-numbered one.
- A useful convention is to light an LED near the top of `main()` so that a
  running board is recognizable at a glance. The template's color-cycling RGB LED
  serves this purpose.

> **Note:** The reset button (BTN0) restarts the CPU into the bootloader
> (BRAM is intact), which re-copies the application from flash, so in this design a
> reset behaves like a power-cycle. Modified `.data` globals are restored
> because the image is re-read from flash each boot.

---

## 6. Working with JTAG Debug Mode

Both modes coexist on the same board with no switching:

| | JTAG (Vitis Run/Debug) | Standalone (flash) |
|---|---|---|
| Code goes to | RAM directly (volatile) | Flash (persistent) |
| After power-cycle | back to the flashed app | your app boots |
| Breakpoints / stepping | yes | no |
| Typical use | daily development loop | shipping a finished program |

The typical development cycle is edit, build, and JTAG Run/Debug. When the
program works, deploy it to flash once. JTAG always takes priority over the bootloader: it
halts the CPU before the bootloader runs, so the flash contents cannot
interfere with a debug session.

---

## 7. How the Deployment Image Is Made (Instructor Reference)

`release/official_boot.mcs` consists of the hardware image with the bootloader
merged into its BRAM initialization, plus the demo application's SREC at
`0x220000`.
The full mechanics (BRAM init frames, `updatemem`, `write_cfgmem`, flash
layout) are in the
[Boot Image Pipeline guide](../Boot-Image-Pipeline/Boot-Image-Pipeline.md).

The bootloader is not custom code. It is the standard Vitis "SREC SPI Bootloader"
application template, built with one configuration change in `src/blconfig.h`:

```c
#define FLASH_IMAGE_BASEADDR  0x00220000
```

The following facts were verified on the board and apply when rebuilding the bootloader:

1. **The flash controller is auto-selected correctly.** Vitis 2025.2 (System
   Device Tree flow) selects `XPAR_AXI_QUAD_SPI_0_BASEADDR`, which is this
   board's flash controller. No manual correction is required.
2. **Keep `VERBOSE` off.** The template's debug prints assume an initialized
   UART divisor; on this board they hang before the jump (the CPU stalls in
   `XUartNs550_SendByte`). It is undefined by default; leave it undefined, or
   call `XUartNs550_SetBaud(...)` first.
3. The bootloader links to BRAM (`0x0`), so it can copy the application into
   SRAM without overwriting itself. Merging its ELF into the bitstream uses
   `updatemem` (about 6 seconds, no re-implementation); see the Boot Image
   Pipeline guide.
4. Applications must set their own UART baud before printing. The course
   template's `mcu_init()` does this.

---

## 8. Troubleshooting

1. **Garbage on the terminal:** The baud rate must be 115200; the application
   sets it via `mcu_init()`. The standard Hello World template does not set
   it; add
   `XUartNs550_SetBaud(XPAR_UART_USB_BASEADDR, 100000000, 115200);` or use the
   course template.
2. **Deployed, but silent after power-on:** The ELF was probably not linked
   with the course template's `lscript.ld`: the bootloader honors the SREC
   record addresses, and a default all-BRAM layout collides with the
   bootloader itself. Rebuild with the template and redeploy.
3. **No memory tour / garbage addresses:** `mcu_init()` is not the first
   line of `main()`, or it was removed; nothing tagged `ITCM_FUNC`/`DTCM_DATA`
   functions before it runs.
4. **Board boots an old application:** The flash write did not complete (check
   the programming step's verify result), or the board was power-cycled before
   the write finished. Redeploy.
5. **Nothing at all (no LED, no output):** The FPGA did not configure, which
   means the flash image is invalid; repeat the procedure in §2. JTAG remains
   available for recovery.
6. **Returning the board to a factory-blank state:** In Hardware Manager,
   right-click the memory device and select **Erase**. The FPGA then waits for
   JTAG at power-on.
