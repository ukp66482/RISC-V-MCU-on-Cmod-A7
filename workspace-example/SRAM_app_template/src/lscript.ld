/******************************************************************************
* Copyright (C) 2023 - 2025 Advanced Micro Devices, Inc. All Rights Reserved.
* SPDX-License-Identifier: MIT
******************************************************************************/

_STACK_SIZE = DEFINED(_STACK_SIZE) ? _STACK_SIZE : 0x2000;
_HEAP_SIZE = DEFINED(_HEAP_SIZE) ? _HEAP_SIZE : 0x800;

/* Define Memories in the system.
 * itcm/dtcm are the two halves of the on-chip BRAM left to the application
 * (the lower 32 KB belong to the bootloader): 1-cycle, never cached. */

MEMORY
{
	sram : ORIGIN = 0x60000000, LENGTH = 0x80000
	itcm : ORIGIN = 0x00008000, LENGTH = 0x8000
	dtcm : ORIGIN = 0x00010000, LENGTH = 0x10000
}

/* Specify the default entry point to the program */

ENTRY(_boot)

/* Define the sections, and where they are mapped in memory */

SECTIONS
{
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

/* Fast code (ISRs, control loops): executes from ITCM but is STORED inside
 * the SRAM image (AT>) — tcm_init() in main.c copies it out at startup, the
 * same way STM32H7 startup code copies ITCM routines out of flash.
 * Tag functions with __attribute__((section(".itcm.text"))). */
.itcm.text : ALIGN(4) {
   __itcm_start = .;
   *(.itcm.text)
   *(.itcm.text.*)
   . = ALIGN(4);
   __itcm_end = .;
} > itcm AT> sram

__itcm_lma = LOADADDR(.itcm.text);

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

_SDA_BASE_ = __sdata_start + ((__sbss_end - __sdata_start) / 2 );

_SDA2_BASE_ = __sdata2_start + ((__sbss2_end - __sdata2_start) / 2 );

/* Generate Stack and Heap definitions */

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

/* Zero-initialised fast data in DTCM (cleared by tcm_init()).
 * Tag variables with __attribute__((section(".dtcm"))). */
.dtcm (NOLOAD) : ALIGN(8) {
   __dtcm_start = .;
   *(.dtcm)
   *(.dtcm.*)
   . = ALIGN(8);
   __dtcm_end = .;
} > dtcm

/* Stack: everything left in DTCM, top-down (SP starts at 0x20000). */
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
