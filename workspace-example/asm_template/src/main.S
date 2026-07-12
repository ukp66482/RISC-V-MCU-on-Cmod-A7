/* ===========================================================================
 * main.S — RISC-V assembly template for the Cmod A7 MCU
 *
 * A complete program written directly in assembly: no C sources. The BSP
 * startup code (boot.S + crt0) runs first — it sets the stack pointer (top
 * of DTCM, from lscript.ld), clears .bss, and then calls main below.
 *
 * What the demo does:
 *   - cycles the on-board RGB LED through its 8 colors
 *   - while the user button (BTN1) is held, the color freezes
 *
 * Peripherals used (memory-mapped, see the datasheet):
 *   0x4002_0000  RGB LED   (3-bit GPIO output: data @+0x0, direction @+0x4)
 *   0x4001_0000  BTN1      (1-bit GPIO input, reads 1 while pressed)
 *
 * Register allocation in this program:
 *   s0 = RGB LED base address     s1 = button base address
 *   s2 = current color (0..7)     t0, t1 = scratch
 * =========================================================================*/

    .text
    .globl main
main:
    li      s0, 0x40020000          # RGB LED GPIO base
    li      s1, 0x40010000          # button GPIO base
    sw      zero, 4(s0)             # direction register: all bits output

    li      s2, 1                   # first color

loop:
    sw      s2, 0(s0)               # drive R/G/B from the low 3 bits

    lw      t0, 0(s1)               # read the button
    andi    t0, t0, 1
    bnez    t0, hold                # held -> keep the current color

    addi    s2, s2, 1               # next color
    andi    s2, s2, 7               # wrap 0..7

hold:
    li      t1, 8000000             # software delay, ~0.2 s
delay:
    addi    t1, t1, -1
    bnez    t1, delay

    j       loop
