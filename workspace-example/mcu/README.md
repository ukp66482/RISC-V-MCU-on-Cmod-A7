# mcu — device support for the Cmod A7 RISC-V MCU

Every application in this workspace builds against the two files in this
folder:

| File | Content |
|---|---|
| `cmod_mcu.h` | Device header: base address, register offsets, and bit masks of every peripheral, for C and assembly. For C it also provides `REG32()`, the `ITCM_FUNC` / `DTCM_DATA` placement attributes, and `mcu_init()`. |
| `cmod_mcu.ld` | Linker script: places the program in ITCM, DTCM, and SRAM. |

## Use it in an application component

Open the component's **Settings > UserConfig.cmake** and set:

| Setting | Value |
|---|---|
| Compiler Settings > Includes > Include paths (-I) | `${CMAKE_SOURCE_DIR}/../../mcu` |
| Linker Settings > Linker Script > Linker script file | `${CMAKE_SOURCE_DIR}/../../mcu/cmod_mcu.ld` |

Then delete the `lscript.ld` that Vitis generated in `src/`; it is no longer
used. `${CMAKE_SOURCE_DIR}` is the component's `src/` folder, so these values
work for every component in this workspace. A component in another workspace
needs a copy of this folder next to it.

In C:

```c
#include "xil_types.h"
#include "cmod_mcu.h"

DTCM_DATA static u32 samples[64];             /* 1-cycle data          */

ITCM_FUNC static void tick_isr(void *ref)     /* 1-cycle code          */
{
    (void)ref;
    REG32(TIMER0_BASE + TMR_TCSR0) |= TMR_TINT;   /* clear the flag    */
}

int main(void)
{
    mcu_init();     /* first: interrupts quiet, console at 115200 baud */
    /* ... */
}
```

In assembly (`.S`, preprocessed):

```asm
#include "cmod_mcu.h"

    li   t0, RGB_BASE
    li   t1, RGB_GREEN
    sw   t1, GPIO_DATA(t0)        # RGB LED green
```

## Memory layout

| Memory | Address | Size | Holds |
|---|---|---|---|
| Bootloader | `0x0000_0000` | 32 KB | the SREC bootloader; nothing is linked here |
| ITCM | `0x0000_8000` | 32 KB | `ITCM_FUNC` code, the trap entry, and the interrupt controller's dispatch code |
| DTCM | `0x0001_0000` | 64 KB | `DTCM_DATA` variables and the interrupt tables, then the stack (down from `0x0001_FFF0`) |
| SRAM | `0x6000_0000` | 512 KB | everything else, through the caches, then the heap |

- The loader (the bootloader from flash, or JTAG) writes the ITCM and DTCM
  directly, so `ITCM_FUNC` code and `DTCM_DATA` variables, initial values
  included, are in place before `main()` runs.
- `_STACK_SIZE` and `_HEAP_SIZE` at the top of `cmod_mcu.ld` are minimums;
  the stack and the heap take everything that is left.
- ITCM code stays fast only while what it calls is in the ITCM too. Use
  `REG32()` there: at `-O0`, `Xil_In32()` and `Xil_Out32()` are function
  calls into SRAM.
- With an `ITCM_FUNC` handler and the `XIntc` instance in DTCM, an interrupt
  reaches the handler in 187 cycles (1.87 µs), cold caches included. The
  datasheet's Memory Specification lists the measurements.
