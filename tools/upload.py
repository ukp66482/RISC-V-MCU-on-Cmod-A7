#!/usr/bin/env python3
"""
upload.py — program an application into the RISC-V MCU on Cmod A7 over USB UART.

Usage:
    python3 upload.py app.elf                 # write to flash, then run (persistent)
    python3 upload.py app.elf --ram           # run from RAM only (volatile, faster)
    python3 upload.py app.bin --entry 0x60000000
    python3 upload.py app.elf --port /dev/ttyUSB1

Accepts an ELF (entry point and load segments are parsed directly — no
toolchain needed) or a raw .bin (specify --entry, image assumed based at
0x60000000).

Protocol counterpart: bootloader.c running from BRAM on the board.
Hold the on-board button while plugging in USB to force bootloader mode.
"""
import argparse
import struct
import sys
import time
import zlib

try:
    import serial
    from serial.tools import list_ports
except ImportError:
    sys.exit("pyserial missing:  pip install pyserial")

SRAM_BASE = 0x60000000
SRAM_SIZE = 512 * 1024
CHUNK = 4096


def parse_elf(data: bytes):
    """Minimal ELF32 little-endian parser -> (entry, flat_image_based_at_SRAM_BASE)."""
    if data[:4] != b"\x7fELF":
        raise ValueError("not an ELF file")
    if data[4] != 1 or data[5] != 1:
        raise ValueError("need 32-bit little-endian ELF")
    entry, phoff = struct.unpack_from("<II", data, 0x18)
    phentsize, phnum = struct.unpack_from("<HH", data, 0x2A)

    segs = []
    for i in range(phnum):
        p_type, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz = struct.unpack_from(
            "<IIIIII", data, phoff + i * phentsize)
        if p_type == 1 and p_filesz > 0:  # PT_LOAD
            # Place segments by LMA (p_paddr), like xsdb "dow": ITCM code has
            # its run address (vaddr) in BRAM but is stored inside the SRAM
            # image and copied out by the app's tcm_init() at startup.
            lma = p_paddr if p_paddr else p_vaddr
            segs.append((lma, data[p_offset:p_offset + p_filesz]))

    loadable = [(v, d) for v, d in segs if SRAM_BASE <= v < SRAM_BASE + SRAM_SIZE]
    dropped = [(v, d) for v, d in segs if not (SRAM_BASE <= v < SRAM_BASE + SRAM_SIZE)]
    for v, d in dropped:
        print(f"  warning: dropping segment @0x{v:08X} ({len(d)} B) outside SRAM "
              f"(BRAM sections like .stack are fine to drop)")
    if not loadable:
        raise ValueError("no loadable segments inside SRAM — wrong linker script?")

    # The bootloader always places the image at SRAM_BASE, so build the flat
    # image based there (zero-padding any gap up to the first segment).
    hi = max(v + len(d) for v, d in loadable)
    img = bytearray(hi - SRAM_BASE)
    for v, d in loadable:
        img[v - SRAM_BASE:v - SRAM_BASE + len(d)] = d
    return entry, SRAM_BASE, bytes(img)


def find_port():
    for p in list_ports.comports():
        if "FTDI" in (p.manufacturer or "") or "Digilent" in (p.manufacturer or ""):
            cands = [q for q in list_ports.comports()
                     if q.serial_number == p.serial_number]
            return sorted(c.device for c in cands)[-1]  # UART is channel B
    sys.exit("no FTDI/Digilent serial port found — is the board plugged in? (--port)")


class Board:
    def __init__(self, port, baud):
        self.s = serial.Serial(port, baud, timeout=2)

    def expect_ack(self, what):
        r = self.s.read(1)
        if r != b"K":
            sys.exit(f"{what}: board answered {r!r} (expected 'K')")

    def sync(self, seconds=8):
        print("syncing (power-cycle the board now, or hold BTN while plugging in)…")
        end = time.time() + seconds
        self.s.timeout = 0.05
        while time.time() < end:
            # a running app may be chattering on the same UART; the bootloader
            # is silent. Drain first, then require the reply to be EXACTLY one
            # 'K' — anything else is app noise, not a handshake.
            self.s.reset_input_buffer()
            self.s.write(b"U")
            if self.s.read(2) == b"K":
                self.s.timeout = 5
                return
        sys.exit("no answer from bootloader — power-cycle the board and retry")

    def send_header(self, size, entry, crc):
        self.s.write(b"H" + struct.pack("<III", size, entry, crc))
        self.expect_ack("header")

    def send_data(self, img):
        sent = 0
        while sent < len(img):
            chunk = img[sent:sent + CHUNK]
            for attempt in range(3):
                self.s.write(b"D" + struct.pack("<H", len(chunk)) + chunk
                             + struct.pack("<I", zlib.crc32(chunk)))
                if self.s.read(1) == b"K":
                    break
                if attempt == 2:
                    sys.exit(f"chunk @{sent} failed 3 times")
            sent += len(chunk)
            pct = 100 * sent // len(img)
            print(f"\r  {sent}/{len(img)} bytes ({pct}%)", end="", flush=True)
        print()

    def go(self, ram_only):
        self.s.write(b"R" if ram_only else b"G")
        self.s.timeout = 30 if not ram_only else 5   # flash erase takes seconds
        self.expect_ack("run" if ram_only else "flash+run")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("image", help=".elf or raw .bin")
    ap.add_argument("--port", default=None)
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--ram", action="store_true", help="run from RAM, don't touch flash")
    ap.add_argument("--entry", type=lambda x: int(x, 0), default=SRAM_BASE,
                    help="entry address for raw .bin (default SRAM base)")
    args = ap.parse_args()

    data = open(args.image, "rb").read()
    if data[:4] == b"\x7fELF":
        entry, base, img = parse_elf(data)
        print(f"ELF: entry=0x{entry:08X}, image {len(img)} B @0x{base:08X}")
    else:
        entry, img = args.entry, data
        print(f"BIN: entry=0x{entry:08X}, image {len(img)} B @0x{SRAM_BASE:08X}")

    if len(img) > SRAM_SIZE:
        sys.exit(f"image {len(img)} B exceeds SRAM ({SRAM_SIZE} B)")

    crc = zlib.crc32(img)
    b = Board(args.port or find_port(), args.baud)
    b.sync()
    b.send_header(len(img), entry, crc)
    b.send_data(img)
    b.go(args.ram)
    print("running from RAM (volatile)" if args.ram
          else "programmed to flash — app now runs at every power-on")


if __name__ == "__main__":
    main()
