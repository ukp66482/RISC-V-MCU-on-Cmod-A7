/* ===========================================================================
 * cmod_mcu.ld - linker script for applications on the Cmod A7 RISC-V MCU
 *
 *   memory  address       size    holds
 *   ITCM    0x0000_8000    32 KB  ITCM_FUNC code, the interrupt entry path
 *   DTCM    0x0001_0000    64 KB  DTCM_DATA variables, then the stack
 *   SRAM    0x6000_0000   512 KB  everything else, then the heap (cached)
 *
 * 0x0000_0000 - 0x0000_7FFF holds the bootloader, which runs again at every
 * reset. It is deliberately not listed, so nothing can be placed there.
 *
 * The loader (the bootloader from flash, or JTAG) writes all three memories
 * directly: ITCM_FUNC code and DTCM_DATA variables, initial values included,
 * are in place before main() runs. Nothing is copied at startup.
 *
 * Interrupts: the BSP's trap entry and the interrupt controller's dispatch
 * code run from ITCM, and the tables they read live in DTCM. Together with
 * an ITCM_FUNC handler, nothing on the way from the interrupt to your code
 * waits for SRAM or a cache line fill.
 *
 * Sizes (edit here, or pass -Wl,--defsym=_STACK_SIZE=...):
 *   _STACK_SIZE  minimum stack. The stack gets all DTCM after DTCM_DATA.
 *   _HEAP_SIZE   minimum heap.  The heap gets all SRAM after the program.
 * The link stops with an error when a minimum does not fit.
 * =========================================================================*/

_STACK_SIZE = DEFINED(_STACK_SIZE) ? _STACK_SIZE : 8K;
_HEAP_SIZE  = DEFINED(_HEAP_SIZE)  ? _HEAP_SIZE  : 8K;

MEMORY
{
    ITCM (rx)  : ORIGIN = 0x00008000, LENGTH = 32K
    DTCM (rw)  : ORIGIN = 0x00010000, LENGTH = 64K
    SRAM (rwx) : ORIGIN = 0x60000000, LENGTH = 512K
}

ENTRY(_boot)

/* Load segments: code read+execute, data read+write. */
PHDRS
{
    itcm PT_LOAD FLAGS(5);
    dtcm PT_LOAD FLAGS(6);
    text PT_LOAD FLAGS(5);
    data PT_LOAD FLAGS(6);
}

SECTIONS
{
    /* ---------------- ITCM: 1-cycle code, never cached ---------------- */
    .itcm : ALIGN(4) {
        *(.itcm.text .itcm.text.*)                   /* ITCM_FUNC        */
        *libxilstandalone.a:trap_handler.S.*(.text)  /* trap entry       */
        *(.text.XIntc_InterruptHandler)              /* INTC dispatch    */
        *(.text.XIntc_DeviceInterruptHandler)
    } > ITCM :itcm

    /* ---------------- DTCM: 1-cycle data, never cached ---------------- */
    .dtcm : ALIGN(8) {
        *(.dtcm .dtcm.*)                             /* DTCM_DATA        */
        *libxilstandalone.a:trap_handler.S.*(.data)  /* trap vectors     */
        *libxil.a:xintc_g.c.*(.drvcfg_sec)           /* INTC handlers    */
        . = ALIGN(16);
        _stack_end = .;                              /* stack: the rest  */
    } > DTCM :dtcm

    /* ---------------- SRAM: code and constants (cached) --------------- */
    .text : {
        KEEP(*(.text.init))              /* _boot, the entry point, first */
        *(.text .text.*)
        KEEP(*(.init))
        KEEP(*(.fini))
    } > SRAM :text

    .rodata : ALIGN(8) {
        *(.rodata .rodata.* .srodata .srodata.*)
    } > SRAM

    /* Every build carries a build-id note. Left unplaced it would land at
     * address 0, and the bootloader would copy it over its own code. */
    .note.gnu.build-id : { KEEP(*(.note.gnu.build-id)) } > SRAM

    /* C runtime tables: constructors and destructors (run around main),
     * and the unwind frames crtbegin.o always contributes. */
    .preinit_array : {
        __preinit_array_start = .;
        KEEP(*(.preinit_array))
        __preinit_array_end = .;
    } > SRAM
    .init_array : {
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
    } > SRAM
    .fini_array : {
        __fini_array_start = .;
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        __fini_array_end = .;
    } > SRAM
    .eh_frame          : { *(.eh_frame) } > SRAM
    .gcc_except_table  : { *(.gcc_except_table .gcc_except_table.*) } > SRAM

    /* ---------------- SRAM: variables (cached, write-through) --------- */
    .data : ALIGN(8) {
        *(.data .data.*)
        *(.drvcfg_sec)                   /* driver configuration tables  */
        __global_pointer$ = . + 0x800;   /* gp, set by crt0              */
        *(.sdata .sdata.*)
    } > SRAM :data

    .bss (NOLOAD) : ALIGN(8) {
        __bss_start = .;                 /* crt0 zeroes __bss_start.._end */
        *(.sbss .sbss.*)
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(8);
        _end = .;
    } > SRAM

    /* ---------------- stack and heap: what is left -------------------- */
    __stack     = ORIGIN(DTCM) + LENGTH(DTCM) - 16;  /* boot.S stores argc
                    at 0(sp): sp must start inside DTCM, because 0x2_0000
                    is not memory and wraps onto the bootloader           */
    _stack      = __stack;
    _heap_start = _end;                  /* malloc (BSP sbrk) grows up    */
    _heap_end   = ORIGIN(SRAM) + LENGTH(SRAM);

    ASSERT(__stack - _stack_end >= _STACK_SIZE,
           "cmod_mcu.ld: DTCM_DATA leaves less than _STACK_SIZE of stack")
    ASSERT(_heap_end - _heap_start >= _HEAP_SIZE,
           "cmod_mcu.ld: the program leaves less than _HEAP_SIZE of heap")
}
