#!/usr/bin/env python3
"""Generate register bit-field SVGs for the datasheet peripheral chapter.

Every register diagram is drawn with ONE fixed geometry so the whole chapter
looks like a family: a constant named-cell width and a constant viewBox width,
with the reserved cell absorbing the remaining space (so its width tracks the
reserved bit count). Field labels are horizontal only (rotated text mis-renders
at PDF zoom, per the project's diagram lessons), which is why the low named
bits get wide cells and the high reserved bits are collapsed into one box.

Field values are NOT verified here; the caller lists them from the BSP
*_l.h headers / cmod_mcu.h. Output goes to docs/datasheet/sections/images/.

Usage: python3 tools/gen_reg_diagram.py   (regenerates every register below)
"""
import os

HERE = os.path.dirname(os.path.abspath(__file__))
IMG = os.path.join(HERE, "..", "docs", "datasheet", "sections", "images")

W, H, CELL, L = 730, 96, 48, 16          # canvas + named-cell width + left margin
TOP, CH = 42, 34                         # cell top y and cell height

HEAD = '''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" \
font-family="Liberation Sans, Helvetica, Arial, sans-serif" fill="black">
  <rect x="0" y="0" width="{W}" height="{H}" fill="white"/>
  <defs>
    <pattern id="rsv" width="7" height="7" patternUnits="userSpaceOnUse" \
patternTransform="rotate(45)">
      <line x1="0" y1="0" x2="0" y2="7" stroke="black" stroke-width="0.5"/>
    </pattern>
  </defs>
  <style>
    .cell {{ fill:white; stroke:black; stroke-width:1; }}
    .bit  {{ font-size:8px; }}
    .fld  {{ font-size:8px; font-weight:bold; }}
    .rsvt {{ font-size:8.5px; font-style:italic; }}
  </style>
'''


def gen(fields):
    """fields: list of (label, nbits) MSB-first covering the named low bits."""
    nunits = sum(n for _, n in fields)
    if nunits > 32:
        raise ValueError("fields exceed 32 bits")
    named_total = nunits * CELL
    reserved_w = W - 2 * L - named_total
    if reserved_w < 96:
        raise ValueError("too many named bits for this layout (reserved < 96)")
    p = [HEAD.format(W=W, H=H)]

    # reserved cell (31 : nunits)
    cx = L + reserved_w / 2
    p.append('  <rect class="cell" x="%d" y="%d" width="%d" height="%d"/>'
             % (L, TOP, reserved_w, CH))
    p.append('  <rect x="%d" y="%d" width="%d" height="%d" fill="url(#rsv)" '
             'stroke="none"/>' % (L, TOP, reserved_w, CH))
    p.append('  <rect x="%d" y="52" width="60" height="14" fill="white"/>'
             % (cx - 30))
    p.append('  <text class="rsvt" x="%d" y="63" text-anchor="middle">'
             'Reserved</text>' % cx)
    p.append('  <text class="bit" x="%d" y="37" text-anchor="start">31</text>'
             % (L + 6))
    p.append('  <text class="bit" x="%d" y="37" text-anchor="end">%d</text>'
             % (L + reserved_w - 6, nunits))

    # named cells, MSB-first. An empty label marks a reserved gap between
    # named fields (hatched like the main reserved cell, no field text).
    x = L + reserved_w
    bit = nunits - 1
    for label, n in fields:
        w = n * CELL
        p.append('  <rect class="cell" x="%d" y="%d" width="%d" height="%d"/>'
                 % (x, TOP, w, CH))
        if not label:
            p.append('  <rect x="%d" y="%d" width="%d" height="%d" '
                     'fill="url(#rsv)" stroke="none"/>' % (x, TOP, w, CH))
        if n == 1:
            p.append('  <text class="bit" x="%d" y="37" text-anchor="middle">'
                     '%d</text>' % (x + w / 2, bit))
        else:
            p.append('  <text class="bit" x="%d" y="37" text-anchor="start">'
                     '%d</text>' % (x + 6, bit))
            p.append('  <text class="bit" x="%d" y="37" text-anchor="end">'
                     '%d</text>' % (x + w - 6, bit - n + 1))
        if label:
            p.append('  <text class="fld" x="%d" y="63" text-anchor="middle">'
                     '%s</text>' % (x + w / 2, label))
        x += w
        bit -= n
    p.append('</svg>')
    return "\n".join(p) + "\n"


# label lists are MSB-first; verified against the BSP *_l.h headers / cmod_mcu.h
REGS = {
    "reg_timer_tcsr": [(s, 1) for s in
        ("CASC", "ENALL", "PWMA", "TINT", "ENT", "ENIT",
         "LOAD", "ARHT", "CAPT", "GENT", "UDT", "MDT")],
    "reg_uart_lcr": [("DLAB", 1), ("BRK", 1), ("SP", 1), ("EPS", 1),
                     ("PEN", 1), ("STB", 1), ("WLS[1:0]", 2)],
    "reg_uart_lsr": [(s, 1) for s in
        ("RFE", "TEMT", "THRE", "BI", "FE", "PE", "OE", "DR")],
    "reg_spi_cr": [(s, 1) for s in
        ("INHIBIT", "MSS", "RXRST", "TXRST", "CPHA",
         "CPOL", "MSTR", "SPE", "LOOP")],
    "reg_spi_sr": [(s, 1) for s in
        ("MODF", "TX_FULL", "TX_EMPTY", "RX_FULL", "RX_EMPTY")],
    "reg_i2c_cr": [(s, 1) for s in
        ("GC", "RSTA", "TXAK", "TX", "MSMS", "TXFIFO", "EN")],
    "reg_i2c_sr": [(s, 1) for s in
        ("TX_EMPTY", "RX_EMPTY", "RX_FULL", "TX_FULL", "SRW", "BB", "AAS", "GC")],
    "reg_intc_ier": [(s, 1) for s in
        ("SPI", "I2C", "EXTI", "USB", "UART1", "TMR2", "TMR1", "TMR0")],
    "reg_intc_mer": [("HIE", 1), ("ME", 1)],
    "reg_uart_ier": [(s, 1) for s in ("EMS", "ERLS", "ETHRE", "ERDA")],
    "reg_uart_fcr": [("RXTRIG[7:6]", 2), ("", 3), ("TXRST", 1),
                     ("RXRST", 1), ("FEN", 1)],
    "reg_xadc_sr": [("JTBSY", 1), ("JTMOD", 1), ("JTLCK", 1), ("BUSY", 1),
                    ("EOS", 1), ("EOC", 1), ("CHANNEL[5:0]", 6)],
    "reg_gpio_data": [(s, 1) for s in
        ("D6", "D5", "D4", "D3", "D2", "D1", "D0")],
    "reg_gpio_tri": [(s, 1) for s in
        ("T6", "T5", "T4", "T3", "T2", "T1", "T0")],
    "reg_exti_data": [(s, 1) for s in ("INT3", "INT2", "INT1", "INT0")],
    "reg_exti_int": [("CH1", 1)],
    # on-board GPIO, fixed direction: DATA carries the signal, TRI is hardwired
    # (0 = output, 1 = input) so its cells show the fixed value.
    "reg_led_data": [("LD2", 1), ("LD1", 1)],
    "reg_led_tri": [("0", 1), ("0", 1)],
    "reg_btn_data": [("BTN1", 1)],
    "reg_btn_tri": [("1", 1)],
    "reg_rgb_data": [("Red", 1), ("Green", 1), ("Blue", 1)],
    "reg_rgb_tri": [("0", 1), ("0", 1), ("0", 1)],
}


def main():
    for name, fields in REGS.items():
        svg = gen(fields)
        with open(os.path.join(IMG, name + ".svg"), "w") as f:
            f.write(svg)
        print("wrote", name + ".svg")


if __name__ == "__main__":
    main()
