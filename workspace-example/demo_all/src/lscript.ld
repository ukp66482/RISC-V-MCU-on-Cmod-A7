/* ===========================================================================
 * lscript.ld — memory layout for applications on the Cmod A7 RISC-V MCU
 *
 * The MCU has three memories your program can use:
 *
 *   address        size    what it is            what goes there
 *   -----------    ------  --------------------  --------------------------
 *   0x6000_0000    512 KB  SRAM   (cached)       code, data, heap  <- default
 *   0x0000_8000     32 KB  ITCM   (1-cycle BRAM) fast code:  ITCM_FUNC
 *   0x0001_0000     64 KB  DTCM   (1-cycle BRAM) stack + fast data: DTCM_DATA
 *
 *   (0x0000_0000..0x7FFF is the bootloader's 32 KB — never touched by apps.)
 *
 * Everything lives in SRAM by default; the caches make hot code fast.
 * ITCM/DTCM are for the cases where "fast on average" is not enough:
 * interrupt handlers and timing-critical loops, where BRAM's 1-cycle access
 * means the WORST case equals the best case. Tag such code/data in C with
 * the ITCM_FUNC / DTCM_DATA macros (see main.c) — this script places them.
 *
 * Things you may safely change:
 *   _STACK_SIZE / _HEAP_SIZE below (or in the Vitis linker settings page).
 * Everything else is plumbing that libc and the C runtime expect — reorder
 * or delete sections only if you know exactly why.
 * =========================================================================*/

_STACK_SIZE = DEFINED(_STACK_SIZE) ? _STACK_SIZE : 0x2000;
_HEAP_SIZE  = DEFINED(_HEAP_SIZE)  ? _HEAP_SIZE  : 0x800;

MEMORY
{
	sram : ORIGIN = 0x60000000, LENGTH = 0x80000
	itcm : ORIGIN = 0x00008000, LENGTH = 0x8000
	dtcm : ORIGIN = 0x00010000, LENGTH = 0x10000
}

ENTRY(_boot)

SECTIONS
{
/* ------------------------------------------------------------------ *
 *  1. Program code — runs from SRAM through the instruction cache    *
 * ------------------------------------------------------------------ */
.text.init : {
   __text.init_start = .;
   *(.text.init)
   __text.init_end = .;
} > sram

.text : {
   . = ALIGN(2048);
   *(.text)
   *(.text.*)
   *(.gnu.linkonce.t.*)
} > sram

/* ------------------------------------------------------------------ *
 *  2. Fast code in ITCM                                              *
 *                                                                    *
 *  Runs at 0x8000 (1-cycle BRAM) but is STORED inside the SRAM image *
 *  ("AT> sram"): the loader only ever writes SRAM, and mcu_init() in *
 *  main.c copies this section out at startup — the same trick an     *
 *  STM32H7 uses to fill its ITCM from flash. If you put too much     *
 *  code here the linker stops with "region `itcm' overflowed".       *
 * ------------------------------------------------------------------ */
.itcm.text : ALIGN(4) {
   __itcm_start = .;
   *(.itcm.text)
   *(.itcm.text.*)
   . = ALIGN(4);
   __itcm_end = .;
} > itcm AT> sram

__itcm_lma = LOADADDR(.itcm.text);   /* where the copy lives in SRAM */

/* ------------------------------------------------------------------ *
 *  3. C runtime plumbing — constructors, exception tables, etc.      *
 *     libc's startup expects all of these; keep them as-is.          *
 * ------------------------------------------------------------------ */
.note.gnu.build-id : {
   KEEP (*(.note.gnu.build-id))
} > sram

.init : {
   KEEP (*(.init))
} > sram

.fini : {
   KEEP (*(.fini))
} > sram

.ctors : {
   __CTOR_LIST__ = .;
   ___CTORS_LIST___ = .;
   KEEP (*crtbegin.o(.ctors))
   KEEP (*(EXCLUDE_FILE(*crtend.o) .ctors))
   KEEP (*(SORT(.ctors.*)))
   KEEP (*(.ctors))
   __CTOR_END__ = .;
   ___CTORS_END___ = .;
} > sram

.dtors : {
   __DTOR_LIST__ = .;
   ___DTORS_LIST___ = .;
   KEEP (*crtbegin.o(.dtors))
   KEEP (*(EXCLUDE_FILE(*crtend.o) .dtors))
   KEEP (*(SORT(.dtors.*)))
   KEEP (*(.dtors))
   __DTOR_END__ = .;
   ___DTORS_END___ = .;
} > sram

/* ------------------------------------------------------------------ *
 *  4. Constants and initialised data (SRAM, D-cached)                *
 *     The s* variants are RISC-V "small data": the compiler reaches  *
 *     them through the gp register in one instruction.               *
 * ------------------------------------------------------------------ */
.rodata : {
   __rodata_start = .;
   *(.rodata)
   *(.rodata.*)
   *(.gnu.linkonce.r.*)
   __rodata_end = .;
} > sram

.srodata : {
   __srodata_start = .;
   *(.srodata)
   __srodata_end = .;
} > sram

.sdata2 : {
   __sdata2_start = .;
   *(.sdata2)
   *(.sdata2.*)
   *(.gnu.linkonce.s2.*)
   __sdata2_end = .;
} > sram

.data : {
   __data_start = .;
   *(.data)
   *(.data.*)
   *(.gnu.linkonce.d.*)
   _edata = .;
} > sram

.eh_frame : {
   *(.eh_frame)
} > sram

.gcc_except_table : {
   *(.gcc_except_table)
} > sram

.init_array : {
   __init_array_start = .;
   KEEP (*(SORT(.init_array.*)))
   KEEP (*(.init_array))
   __init_array_end = .;
} > sram

.fini_array : {
   __fini_array_start = .;
   KEEP (*(SORT(.fini_array.*)))
   KEEP (*(.fini_array))
   __fini_array_end = .;
} > sram

.sdata : {
   __sdata_start = .;
   *(.sdata)
   *(.sdata.*)
   *(.gnu.linkonce.s.*)
   __sdata_end = .;
} > sram

/* ------------------------------------------------------------------ *
 *  5. Zero-initialised data (BSS) — crt0 clears this before main()   *
 * ------------------------------------------------------------------ */
.bss (NOLOAD) : {
   __bss_start = .;
   *(.bss)
   *(.bss.*)
   *(.gnu.linkonce.b.*)
   *(COMMON)
   __bss_end = .;
} > sram

.sbss (NOLOAD) : {
   __sbss_start = .;
   *(.sbss)
   *(.sbss.*)
   *(.gnu.linkonce.sb.*)
   __sbss_end = .;
} > sram

.sbss2 : {
   __sbss2_start = .;
   *(.sbss2)
   *(.sbss2.*)
   *(.gnu.linkonce.sb2.*)
   __sbss2_end = .;
} > sram

/* centres of the small-data windows used by gp-relative addressing */
_SDA_BASE_ = __sdata_start + ((__sbss_end - __sdata_start) / 2 );

_SDA2_BASE_ = __sdata2_start + ((__sbss2_end - __sdata2_start) / 2 );

/* ------------------------------------------------------------------ *
 *  6. Heap (malloc) — grows inside SRAM                              *
 * ------------------------------------------------------------------ */
.heap (NOLOAD) : {
   . = ALIGN(8);
   _heap = .;
   _heap_start = .;
   . += _HEAP_SIZE;
   _heap_end = .;
   /* _end MUST stay on the SRAM side: crt0 zeroes __bss_start.._end.
    * (With .stack moved to BRAM, a trailing "_end = .;" would land in
    *  BRAM below __bss_start and the BSS clear loop would never run.) */
   _end = .;
} > sram

/* ------------------------------------------------------------------ *
 *  7. Fast data in DTCM — zeroed by mcu_init(), never cached         *
 * ------------------------------------------------------------------ */
.dtcm (NOLOAD) : ALIGN(8) {
   __dtcm_start = .;
   *(.dtcm)
   *(.dtcm.*)
   . = ALIGN(8);
   __dtcm_end = .;
} > dtcm

/* ------------------------------------------------------------------ *
 *  8. Stack — all remaining DTCM, growing down from 0x2_0000.        *
 *     1-cycle accesses, and it can never collide with code or heap   *
 *     because it lives in a different physical memory.               *
 * ------------------------------------------------------------------ */
.stack (NOLOAD) : {
   . = ALIGN(8);
   _stack_end = .;
   . = ORIGIN(dtcm) + LENGTH(dtcm);
   _stack = .;
   __stack = _stack;
} > dtcm

ASSERT(_stack - _stack_end >= _STACK_SIZE, "DTCM stack smaller than _STACK_SIZE")

__global_pointer$ = MIN(__sdata_start + 0x800, MAX(__data_start + 0x800, __bss_end - 0x800));
}
