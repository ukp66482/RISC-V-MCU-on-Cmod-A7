#!/usr/bin/env python3
"""
jtag_run.py — load an application over JTAG and run it, in one command.

    python3 tools/jtag_run.py build/app.elf            # load + run + show UART
    python3 tools/jtag_run.py app.elf --bit release/top.bit   # program FPGA first
    python3 tools/jtag_run.py app.elf --no-monitor     # just start it and exit

This is the CLI equivalent of clicking "Run" in Vitis: the program goes to
RAM and runs until power-off (nothing is written to flash — use upload.py to
make it permanent). Use --bit on a factory-blank board or after a failed
flash; a board that boots normally already has the FPGA configured.

Requires xsdb on PATH (comes with Vivado/Vitis). Ctrl-C stops the UART
monitor only — the program keeps running on the board.
"""
import argparse
import os
import subprocess
import sys
import tempfile

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

TCL = """\
connect
{fpga}
targets -set -nocase -filter {{name =~ "*Hart*"}}
catch {{stop}}
dow "{elf}"
con
disconnect
puts "JTAG-RUN-OK"
"""


def monitor(baud):
    try:
        import serial
        from serial.tools import list_ports
    except ImportError:
        print("(pyserial missing — skipping UART monitor; pip install pyserial)")
        return
    port = None
    for p in list_ports.comports():
        if "FTDI" in (p.manufacturer or "") or "Digilent" in (p.manufacturer or ""):
            cands = [q for q in list_ports.comports()
                     if q.serial_number == p.serial_number]
            port = sorted(c.device for c in cands)[-1]
            break
    if not port:
        print("(no FTDI serial port found — skipping UART monitor)")
        return
    print(f"--- UART {port} @ {baud} (Ctrl-C to stop watching; app keeps running)")
    with serial.Serial(port, baud, timeout=1) as s:
        try:
            while True:
                sys.stdout.write(s.read(4096).decode("ascii", "replace"))
                sys.stdout.flush()
        except KeyboardInterrupt:
            print("\n--- monitor stopped")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("elf")
    ap.add_argument("--bit", help="bitstream to program first (e.g. release/top.bit)")
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--no-monitor", action="store_true",
                    help="don't tail the UART after starting the app")
    args = ap.parse_args()

    elf = os.path.abspath(args.elf)
    if not os.path.exists(elf):
        sys.exit(f"no such file: {elf}")
    fpga = f'fpga "{os.path.abspath(args.bit)}"' if args.bit else ""

    with tempfile.NamedTemporaryFile("w", suffix=".tcl", delete=False) as f:
        f.write(TCL.format(fpga=fpga, elf=elf))
        script = f.name
    try:
        r = subprocess.run(["xsdb", script], capture_output=True, text=True,
                           timeout=180)
        sys.stdout.write(r.stdout)
        if "JTAG-RUN-OK" not in r.stdout:
            sys.stderr.write(r.stderr)
            sys.exit("xsdb failed — board plugged in? FPGA configured? "
                     "(try --bit release/top.bit)")
    finally:
        os.unlink(script)
    print("running (RAM only — power-off wipes it; upload.py makes it permanent)")
    if not args.no_monitor:
        monitor(args.baud)


if __name__ == "__main__":
    main()
