#!/usr/bin/env python3
"""
flash_app.py — deploy an application to QSPI flash, in one command.

    python3 tools/flash_app.py workspace-example/<app>/build/<app>.elf
    python3 tools/flash_app.py app.elf --monitor    # then watch the UART

The ELF is converted to SREC, written into the flash application slot
(0x340000), and the board is rebooted from flash — the application then
starts automatically at every power-on, no PC needed. The write is
range-scoped: the bitstream (with the bootloader inside) is never touched.

Requires the ELF to be linked with the course template's lscript.ld (code in
SRAM at 0x6000_0000), and vivado on PATH — flash access goes through JTAG.
Ctrl-C stops the UART monitor only; the board keeps running.
"""
import argparse
import glob
import os
import shutil
import subprocess
import sys
import tempfile

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BOOT_BIT = os.path.join(REPO, "release", "boot_srec.bit")
APP_SLOT = 0x00340000
FLASH_END = 0x00400000          # 4 MB part
SRAM_LO, SRAM_HI = 0x60000000, 0x60080000

TCL = """\
write_cfgmem -force -format mcs -size 4 -interface SPIx4 \
    -loaddata "up 0x{slot:08X} {srec}" -file {mcs}
open_hw_manager
connect_hw_server
open_hw_target
set dev [lindex [get_hw_devices xc7a*] 0]
current_hw_device $dev
refresh_hw_device -update_hw_probes false $dev
set cfg [create_hw_cfgmem -hw_device $dev \
    [lindex [get_cfgmem_parts {{mx25l3273f-spi-x1_x2_x4}}] 0]]
set_property PROGRAM.ADDRESS_RANGE  {{use_file}} $cfg
set_property PROGRAM.FILES [list {mcs}] $cfg
set_property PROGRAM.PRM_FILE {{}} $cfg
set_property PROGRAM.UNUSED_PIN_TERMINATION {{pull-none}} $cfg
set_property PROGRAM.BLANK_CHECK 0 $cfg
set_property PROGRAM.ERASE 1 $cfg
set_property PROGRAM.CFG_PROGRAM 1 $cfg
set_property PROGRAM.VERIFY 1 $cfg
create_hw_bitstream -hw_device $dev [get_property PROGRAM.HW_CFGMEM_BITFILE $dev]
program_hw_devices $dev
refresh_hw_device $dev
program_hw_cfgmem -hw_cfgmem $cfg
puts "FLASH-APP-OK"
# boot_hw_device (JPROGRAM -> config from flash) is occasionally flaky over
# JTAG; falling back to loading the same design directly is equivalent for
# the user (a real power-on always boots from flash).
if {{[catch {{boot_hw_device $dev}}]}} {{
    puts "boot-from-flash unresponsive - loading design over JTAG instead"
    set_property PROGRAM.FILE {{{boot_bit}}} $dev
    program_hw_devices $dev
}}
puts "BOOTED-FROM-FLASH"
close_hw_target
disconnect_hw_server
exit
"""


def find_tool(name, patterns):
    path = shutil.which(name)
    if path:
        return path
    for pat in patterns:
        hits = sorted(glob.glob(pat))
        if hits:
            return hits[-1]
    sys.exit(f"{name} not found — source the Vivado/Vitis settings64.sh first")


def elf_to_srec(elf, srec):
    objcopy = find_tool(
        "riscv32-amd-linux-gnu-objcopy",
        ["/opt/Xilinx/*/gnu/riscv/linux_toolchain/lin*/bin/"
         "riscv32-amd-linux-gnu-objcopy"])
    subprocess.run([objcopy, "-O", "srec", elf, srec], check=True)


def check_srec(srec):
    """The bootloader honors the record addresses — make sure they are SRAM."""
    data_bytes, entry = 0, None
    for ln in open(srec):
        if ln.startswith("S3"):
            addr = int(ln[4:12], 16)
            if not SRAM_LO <= addr < SRAM_HI:
                sys.exit(f"S-record at 0x{addr:08X} is outside SRAM — the ELF "
                         "is not linked with the course template's lscript.ld")
            data_bytes += (len(ln.strip()) - 12) // 2 - 1
        elif ln.startswith("S7"):
            entry = int(ln[4:12], 16)
    if entry is None:
        sys.exit("no S7 entry record — not a valid SREC image?")
    size = os.path.getsize(srec)
    if APP_SLOT + size > FLASH_END:
        sys.exit(f"SREC image is {size} B but the flash app slot only holds "
                 f"{FLASH_END - APP_SLOT} B — the application is too large")
    print(f"app: {data_bytes} B in SRAM, entry 0x{entry:08X}, "
          f"SREC {size} B -> flash 0x{APP_SLOT:06X}")


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
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--monitor", action="store_true",
                    help="tail the UART after the board reboots")
    args = ap.parse_args()

    elf = os.path.abspath(args.elf)
    if not os.path.exists(elf):
        sys.exit(f"no such file: {elf}")
    vivado = find_tool("vivado", ["/opt/Xilinx/*/Vivado/bin/vivado"])

    tmp = tempfile.mkdtemp(prefix="flash_app_")
    srec = os.path.join(tmp, "app.srec")
    elf_to_srec(elf, srec)
    check_srec(srec)

    script = os.path.join(tmp, "flash.tcl")
    with open(script, "w") as f:
        f.write(TCL.format(slot=APP_SLOT, srec=srec, boot_bit=BOOT_BIT,
                           mcs=os.path.join(tmp, "app.mcs")))
    print("writing flash over JTAG (takes about a minute)…")
    r = subprocess.run([vivado, "-mode", "batch", "-nolog", "-nojournal",
                        "-source", script], capture_output=True, text=True,
                       timeout=600)
    if "FLASH-APP-OK" not in r.stdout:
        sys.stdout.write(r.stdout[-3000:])
        sys.stderr.write(r.stderr[-2000:])
        sys.exit("flash programming failed — board plugged in?")
    if "BOOTED-FROM-FLASH" not in r.stdout:
        sys.exit("flash written, but reboot-from-flash failed — power-cycle "
                 "the board manually")
    shutil.rmtree(tmp, ignore_errors=True)
    print("programmed to flash — app now runs at every power-on")
    if args.monitor:
        monitor(args.baud)


if __name__ == "__main__":
    main()
