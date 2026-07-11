#!/usr/bin/env python3
"""Verify the pinout diagram against the pin specification.

Diffs every (pin number, signal name) pair in
docs/datasheet/sections/images/pinout_diagram.svg against the two authoritative
pin-map tables (sections 2 and 3) of the pin spec markdown, and checks that
every drawn row pairs pin k with pin 49-k (the DIP's physical mirror, verified
on hardware). Run after ANY edit to the pinout SVG.

The x coordinates below must match the SVG's text columns:
left number x=176, right number x=364 (anchor=end),
left signal x=148 (anchor=end), right signal x=392.
"""
import os, re, sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MD = os.path.join(REPO, "docs/datasheet/sections/pins.md")
SVG = os.path.join(REPO, "docs/datasheet/sections/images/pinout_diagram.svg")
XL_NUM, XR_NUM, XL_SIG, XR_SIG = 176, 364, 148, 392

body = open(MD).read().split("## 2. Pin Map")[1].split("## 4.")[0]
spec = {int(m.group(1)): m.group(2) for m in
        re.finditer(r"^\|\s*(\d+)\s*\|\s*\*{0,2}([A-Za-z0-9_]+)\*{0,2}\s*\|",
                    body, re.M)}

rows = {}
for x, y, t in re.findall(r'<text[^>]*x="(\d+)" y="(\d+)"[^>]*>([^<]+)</text>',
                          open(SVG).read()):
    rows.setdefault(int(y), {})[int(x)] = t

errs = pins = 0
for y, row in sorted(rows.items()):
    if XL_NUM not in row or XR_NUM not in row:
        continue  # not a pin row
    lnum, rnum = int(row[XL_NUM]), int(row[XR_NUM])
    for num, sig in ((lnum, row[XL_SIG]), (rnum, row[XR_SIG])):
        pins += 1
        if spec.get(num) != sig:
            errs += 1
            print(f"MISMATCH pin {num}: diagram={sig!r} spec={spec.get(num)!r}")
    if lnum + rnum != 49:
        errs += 1
        print(f"PAIRING ERROR: {lnum} drawn opposite {rnum} (must sum to 49)")

if pins != 48 or len(spec) != 48:
    errs += 1
    print(f"COUNT ERROR: diagram has {pins} pins, spec has {len(spec)}")
print(f"{pins} pins checked -> " +
      ("ALL MATCH, all rows pair k/(49-k)" if errs == 0 else f"{errs} ERRORS"))
sys.exit(1 if errs else 0)
