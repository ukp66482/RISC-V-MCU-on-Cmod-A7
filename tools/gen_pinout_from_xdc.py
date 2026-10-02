#!/usr/bin/env python3
"""Generate the Cmod A7 DIP pinout diagram straight from the XDC.

Source of truth is the active (uncommented) PACKAGE_PIN constraints in
board/Cmod-A7-Master.xdc. Each FPGA package pin is mapped to its DIP pin with
Digilent's Cmod A7-35T table (Sch=pio[NN] in Digilent's master XDC), so the
"# DIP NN" comments in our XDC are checked, not trusted.

Cross-checks (printed, exit 1 on any error):
  - every "# DIP NN" comment in the XDC agrees with the Digilent table
  - the placed design (impl_1/top_wrapper_io_placed.rpt) puts every port on
    the same package pin as the XDC (catches a stale bitstream / XSA)
  - every DIP pin is driven by at most one port

Output: docs/datasheet/sections/images/pinout_diagram.svg by default (top
view, pin 1 top-left, FPGA package pin next to each DIP number). Render to
PNG with --png (uses Edge headless). Run check_pinout_diagram.py afterwards
to diff the result against the pin tables in pins.md.

Usage: python tools/gen_pinout_from_xdc.py [-o OUT_BASENAME] [--png]
"""
import argparse, os, re, shutil, subprocess, sys, tempfile

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
XDC = os.path.join(REPO, "board/Cmod-A7-Master.xdc")
RPT = os.path.join(REPO, "RISC-V-MCU/RISC-V-MCU.runs/impl_1/top_wrapper_io_placed.rpt")
DEFAULT_OUT = os.path.join(REPO, "docs/datasheet/sections/images/pinout_diagram")

# Digilent Cmod A7-35T: DIP pin -> FPGA package pin(s).
# 15/16 are the analog inputs (divider into XADC VAUX4 / VAUX12, p/n pair).
# 24 = VU, 25 = GND carry no FPGA I/O.
DIGILENT = {
     1: ["M3"],  2: ["L3"],  3: ["A16"],  4: ["K3"],  5: ["C15"],  6: ["H1"],
     7: ["A15"], 8: ["B15"], 9: ["A14"], 10: ["J3"], 11: ["J1"],  12: ["K2"],
    13: ["L1"], 14: ["L2"], 15: ["G3", "G2"], 16: ["H2", "J2"],
    17: ["M1"], 18: ["N3"], 19: ["P3"], 20: ["M2"], 21: ["N1"], 22: ["N2"],
    23: ["P1"], 26: ["R3"], 27: ["T3"], 28: ["R2"], 29: ["T1"], 30: ["T2"],
    31: ["U1"], 32: ["W2"], 33: ["V2"], 34: ["W3"], 35: ["V3"], 36: ["W5"],
    37: ["V4"], 38: ["U4"], 39: ["V5"], 40: ["W4"], 41: ["U5"], 42: ["U2"],
    43: ["W6"], 44: ["U3"], 45: ["U7"], 46: ["W7"], 47: ["U8"], 48: ["V8"],
}
POWER = {24: "VU", 25: "GND"}
PKG2DIP = {p: dip for dip, pins in DIGILENT.items() for p in pins}

# top_wrapper port -> datasheet signal name
FIXED_NAMES = {
    "uart_1_tx": "UART_TX", "uart_1_rx": "UART_RX",
    "i2c_ext_scl_io": "I2C_SCL", "i2c_ext_sda_io": "I2C_SDA",
    "spi_ext_sck_io": "SPI_SCLK", "spi_ext_io0_io": "SPI_MOSI",
    "spi_ext_io1_io": "SPI_MISO",
    "vauxp4": "ADC_0", "vauxn4": "ADC_0", "vauxp12": "ADC_1", "vauxn12": "ADC_1",
}
ANALOG = {"ADC_0", "ADC_1"}


def port_label(port):
    if port in FIXED_NAMES:
        return FIXED_NAMES[port]
    m = re.fullmatch(r"gpio_([A-Z])_tri_io\[(\d+)\]", port)
    if m:
        return f"GPIO_{m.group(1)}{m.group(2)}"
    m = re.fullmatch(r"intr_tri_i\[(\d+)\]", port)
    if m:
        return f"INTR_{m.group(1)}"
    m = re.fullmatch(r"spi_ext_ss_io\[(\d+)\]", port)
    if m:
        return f"SPI_SS{m.group(1)}"
    m = re.fullmatch(r"pwm_(\d+)", port)
    if m:
        return f"PWM_{m.group(1)}"
    return port  # unknown port: show raw name so it is noticed


def parse_xdc():
    """[(pkg_pin, port, dip_comment or None)] for active constraints."""
    out = []
    for line in open(XDC, encoding="utf-8"):
        if line.lstrip().startswith("#"):
            continue
        m = re.search(r"PACKAGE_PIN\s+(\w+).*?get_ports\s*\{\s*([^}\s]+)\s*\}", line)
        if not m:
            continue
        c = re.search(r"#\s*DIP\s+(\d+)", line)
        out.append((m.group(1), m.group(2), int(c.group(1)) if c else None))
    return out


def parse_rpt():
    """{port: pkg_pin} from the placed-IO report, or None if absent."""
    if not os.path.exists(RPT):
        return None
    placed = {}
    for line in open(RPT, encoding="utf-8", errors="replace"):
        cols = [c.strip() for c in line.split("|")]
        if len(cols) > 3 and re.fullmatch(r"[A-Z]+\d+", cols[1]) and cols[2]:
            placed[cols[2]] = cols[1]
    return placed


def check(xdc, placed):
    errs = []
    dip_ports = {}
    for pkg, port, dip_c in xdc:
        dip = PKG2DIP.get(pkg)
        if dip is not None:
            dip_ports.setdefault(dip, []).append((port, pkg))
        if dip_c is not None and dip_c != dip:
            errs.append(f"XDC comment says DIP {dip_c} for {port} ({pkg}), "
                        f"Digilent table says {'DIP %d' % dip if dip else 'not a DIP pin'}")
        if placed is not None and placed.get(port) != pkg:
            errs.append(f"{port}: XDC {pkg}, placed design {placed.get(port)} "
                        f"(bitstream older than XDC?)")
    for dip, ports in dip_ports.items():
        labels = {port_label(p) for p, _ in ports}
        if len(labels) > 1:
            errs.append(f"DIP {dip} driven by several signals: {ports}")
    return dip_ports, errs


# ── SVG ─────────────────────────────────────────────────────────────────────
STATIC = """<rect x="0" y="0" width="660" height="840" fill="white"/>
<rect x="170" y="40" width="200" height="700" rx="6" fill="white" stroke="black" stroke-width="1.5"/>
<rect x="174" y="44" width="192" height="64" fill="white" stroke="black" stroke-width="1.3"/>
{pmod}
<polyline class="dim" points="326,55 331,55 331,87 326,87" fill="none"/>
<line class="dim" x1="331" y1="71" x2="384" y2="71"/>
<text class="ti" x="388" y="74">3.3 V (2)</text>
<polyline class="dim" points="306,55 311,55 311,87 306,87" fill="none"/>
<polyline class="dim" points="311,87 311,96 384,96" fill="none"/>
<text class="ti" x="388" y="99">GND (2)</text>
<text class="ti" x="270" y="103" text-anchor="middle">Pmod (JA) &#183; 2&#215;6 &#183; data pins not implemented</text>
<rect x="250" y="148" width="24" height="24" fill="white" stroke="black" stroke-width="1.2"/>
<circle cx="262" cy="160" r="7" fill="white" stroke="black" stroke-width="1.2"/>
<text class="ti" x="280" y="163">BTN0 &#183; reset</text>
<rect x="250" y="192" width="24" height="24" fill="white" stroke="black" stroke-width="1.2"/>
<circle cx="262" cy="204" r="7" fill="white" stroke="black" stroke-width="1.2"/>
<text class="ti" x="280" y="207">BTN1 &#183; user</text>
<rect x="264" y="236" width="12" height="8" fill="white" stroke="black" stroke-width="1"/>
<text class="ti" x="258" y="243" text-anchor="end">LD0 &#183; RGB</text>
<rect x="300" y="236" width="12" height="8" fill="white" stroke="black" stroke-width="1"/>
<text class="ti" x="318" y="243">LD1</text>
<rect x="300" y="258" width="12" height="8" fill="white" stroke="black" stroke-width="1"/>
<text class="ti" x="318" y="265">LD2</text>
<rect x="215" y="300" width="110" height="60" fill="white" stroke="black" stroke-width="1.3"/>
<text class="t" x="270" y="325" text-anchor="middle" font-weight="bold">SRAM</text>
<text class="t" x="270" y="339" text-anchor="middle">512 KB</text>
<rect x="200" y="385" width="140" height="130" fill="white" stroke="black" stroke-width="1.3"/>
<text class="t" x="270" y="443" text-anchor="middle" font-weight="bold" font-size="12">RISC-V</text>
<text class="t" x="270" y="459" text-anchor="middle" font-weight="bold" font-size="12">MCU</text>
<rect x="222" y="550" width="96" height="50" fill="white" stroke="black" stroke-width="1.3"/>
<text class="t" x="270" y="571" text-anchor="middle" font-weight="bold">QSPI Flash</text>
<text class="t" x="270" y="585" text-anchor="middle">4 MB</text>
<rect x="222" y="628" width="96" height="40" fill="white" stroke="black" stroke-width="1.3"/>
<text class="t" x="270" y="652" text-anchor="middle">USB bridge</text>
<rect x="235" y="722" width="70" height="36" fill="white" stroke="black" stroke-width="1.3"/>
<text class="t" x="270" y="776" text-anchor="middle">Micro-USB &#183; JTAG + UART + power</text>"""

FOOTER = """<circle cx="188" cy="131" r="2.5"/>
<text class="ti" x="148" y="128" text-anchor="end">pin 1</text>
<line class="dim" x1="520" y1="40" x2="520" y2="740" marker-start="url(#db)" marker-end="url(#da)"/>
<line class="dim" x1="510" y1="40" x2="530" y2="40"/>
<line class="dim" x1="510" y1="740" x2="530" y2="740"/>
<text class="ti" x="534" y="390" transform="rotate(-90 534 390)" text-anchor="middle">2.75 in (70 mm)</text>
<line class="dim" x1="170" y1="790" x2="370" y2="790" marker-start="url(#db)" marker-end="url(#da)"/>
<line class="dim" x1="170" y1="780" x2="170" y2="800"/>
<line class="dim" x1="370" y1="780" x2="370" y2="800"/>
<text class="ti" x="270" y="812" text-anchor="middle">0.7 in (17.8 mm)</text>
<rect x="192" y="827" width="14" height="8" fill="black" stroke="black" stroke-width="1.2"/>
<text class="ti" x="212" y="834">= analog input (0&#8211;3.3 V via divider) &#183; NC = spare, not connected &#183; grey = FPGA pin (from XDC)</text>"""


def pin_cells(dip, dip_ports):
    """(label, css class, bold, analog, fpga pin text) for one DIP pin."""
    if dip in POWER:
        return POWER[dip], "s", True, False, ""
    ports = dip_ports.get(dip)
    if not ports:
        return "NC", "nc", False, False, DIGILENT[dip][0]
    label = port_label(ports[0][0])
    return label, "s", False, label in ANALOG, "/".join(DIGILENT[dip])


def build_svg(dip_ports):
    pmod = "\n".join(
        f'<rect x="{x}" y="{y}" width="12" height="12" fill="white" stroke="black" stroke-width="1"/>'
        for y in (54, 76) for x in range(214, 334, 20))
    rows = []
    for i in range(24):
        # top view, standard DIP numbering: pin 1 top-left, counter-clockwise
        ldip, rdip = 1 + i, 48 - i
        ry, ty = 136 + 24 * i, 143 + 24 * i
        L = pin_cells(ldip, dip_ports)
        R = pin_cells(rdip, dip_ports)
        bold = lambda c: ' font-weight="bold"' if c[2] else ""
        fill = lambda c: "black" if c[3] else "white"
        rows += [
            f'<rect x="156" y="{ry}" width="14" height="8" fill="{fill(L)}" stroke="black" stroke-width="1.2"/>',
            f'<rect x="370" y="{ry}" width="14" height="8" fill="{fill(R)}" stroke="black" stroke-width="1.2"/>',
            f'<text class="n" x="176" y="{ty}">{ldip}</text>',
            f'<text class="n" x="364" y="{ty}" text-anchor="end">{rdip}</text>',
            f'<text class="{L[1]}" x="148" y="{ty}" text-anchor="end"{bold(L)}>{L[0]}</text>',
            f'<text class="{R[1]}" x="392" y="{ty}"{bold(R)}>{R[0]}</text>',
        ]
        if L[4]:
            rows.append(f'<text class="pk" x="98" y="{ty}" text-anchor="end">{L[4]}</text>')
        if R[4]:
            rows.append(f'<text class="pk" x="452" y="{ty}">{R[4]}</text>')
    return "\n".join([
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 660 840" '
        'font-family="Liberation Sans, Helvetica, Arial, sans-serif" fill="black">',
        '<defs><marker id="da" markerWidth="7" markerHeight="7" refX="6" refY="3.5" orient="auto">'
        '<path d="M0,0 L7,3.5 L0,7 z"/></marker><marker id="db" markerWidth="7" markerHeight="7" '
        'refX="1" refY="3.5" orient="auto"><path d="M7,0 L0,3.5 L7,7 z"/></marker></defs>',
        '<style>.n{font-size:8px;}.s{font-size:9px;}.t{font-size:9px;}.ti{font-size:8px;font-style:italic;}'
        '.dim{stroke:black;stroke-width:0.8;}.nc{font-size:9px;font-style:italic;fill:#888;}'
        '.pk{font-size:7px;fill:#888;font-family:Consolas,monospace;}</style>',
        STATIC.format(pmod=pmod), *rows, FOOTER, "</svg>", ""])


def render_png(svg_path, png_path, scale=2):
    edge = shutil.which("msedge") or r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"
    if not os.path.exists(edge):
        print("Edge not found, PNG skipped")
        return
    w, h = 660 * scale, 840 * scale
    html = os.path.join(tempfile.mkdtemp(), "p.html")
    with open(html, "w", encoding="utf-8") as f:
        f.write(f'<html><body style="margin:0"><img src="file:///{svg_path.replace(os.sep, "/")}" '
                f'width="{w}" height="{h}"></body></html>')
    subprocess.run([edge, "--headless", "--disable-gpu", "--hide-scrollbars",
                    f"--window-size={w},{h}", f"--screenshot={png_path}", html],
                   check=True, capture_output=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--out", default=DEFAULT_OUT, help="output basename (no extension)")
    ap.add_argument("--png", action="store_true", help="also render PNG with Edge headless")
    a = ap.parse_args()

    xdc = parse_xdc()
    placed = parse_rpt()
    dip_ports, errs = check(xdc, placed)

    print(f"{len(xdc)} active PACKAGE_PIN constraints in XDC")
    print("placed-IO report: " + ("not found, skipped" if placed is None else "compared"))
    for dip in range(1, 49):
        label, *_ , pk = pin_cells(dip, dip_ports)
        ports = ", ".join(p for p, _ in dip_ports.get(dip, []))
        print(f"  DIP {dip:2d}  {pk or '-':6s} {label:9s} {ports}")
    off_dip = [(pkg, p) for pkg, p, _ in xdc if pkg not in PKG2DIP]
    if off_dip:
        print("constrained, not on the DIP header: " +
              ", ".join(f"{p}@{pkg}" for pkg, p in off_dip))

    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    svg = os.path.abspath(a.out + ".svg")
    with open(svg, "w", encoding="utf-8") as f:
        f.write(build_svg(dip_ports))
    print(f"wrote {svg}")
    if a.png:
        render_png(svg, os.path.abspath(a.out + ".png"))
        print(f"wrote {os.path.abspath(a.out + '.png')}")

    for e in errs:
        print("ERROR: " + e)
    print("OK, XDC / Digilent table / placed design all agree" if not errs
          else f"{len(errs)} ERRORS")
    sys.exit(1 if errs else 0)


if __name__ == "__main__":
    main()
