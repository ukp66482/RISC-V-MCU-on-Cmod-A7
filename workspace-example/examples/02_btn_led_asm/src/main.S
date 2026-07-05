# ============================================================
# main.S — Read board button, control board LEDs
# Target: MicroBlaze RISC-V on Cmod A7-35T
#
# board_button  @ 0x40010000  (1-bit input, bit 0)
# board_led     @ 0x40000000  (2-bit output, bits [1:0])
#
# Behavior: Button pressed (bit0=1) → LEDs ON (0x03)
#           Button released (bit0=0) → LEDs OFF (0x00)
# ============================================================

    .section .text
    .globl main
    .type  main, @function

main:
    # Load base addresses into registers
    li   t0, 0x40010000          # t0 = 0x40010000 (button base)
    li   t1, 0x40000000          # t1 = 0x40000000 (LED base)

loop:
    lw    t2, 0(t0)            # t2 = GPIO_DATA of button
    andi  t2, t2, 0x1          # isolate bit 0

    beqz  t2, led_off          # if button not pressed, jump

led_on:
    li    t3, 0x3              # both LEDs on
    sw    t3, 0(t1)            # write to LED GPIO_DATA
    j     loop

led_off:
    sw    zero, 0(t1)          # both LEDs off
    j     loop