#!/bin/bash
# make_boot_mcs.sh — build a QSPI flash image (boot.mcs) from an application ELF.
#
# Usage:
#   tools/make_boot_mcs.sh <bootloader.elf> [--x1] [--hw <dir-with-bit-and-mmi>]
#
#   <bootloader.elf>  the BRAM UART bootloader ELF (workspace-example/bootloader).
#                     Merged into the bitstream's BRAM init to make the deployment
#                     image. (Student apps are NOT merged here — they upload via
#                     tools/upload.py.)
#   --x1        force a SPIx1 flash image (slow power-on config, ~6 s). The default
#               is SPIx4 to match the repo XDC (BITSTREAM.CONFIG.SPI_BUSWIDTH 4),
#               which boots in well under a second.
#   --hw <dir>  directory containing top_wrapper.bit + top_wrapper.mmi
#               (default: release/top_wrapper, auto-extracted from release/top_wrapper.xsa)
#
# Output: boot.mcs (and download.bit) next to the ELF.
set -e

REPO="$(cd "$(dirname "$0")/.." && pwd)"
ELF=""
IFACE="SPIx4"
HWDIR="$REPO/release/top_wrapper"

while [ $# -gt 0 ]; do
    case "$1" in
        --x1) IFACE="SPIx1"; shift ;;
        --x4) IFACE="SPIx4"; shift ;;
        --hw) HWDIR="$2"; shift 2 ;;
        *)    ELF="$1"; shift ;;
    esac
done

if [ -z "$ELF" ] || [ ! -f "$ELF" ]; then
    echo "usage: $0 <app.elf> [--x4] [--hw <dir>]" >&2
    exit 1
fi

command -v updatemem >/dev/null || { echo "updatemem not on PATH — source Vivado settings64.sh first" >&2; exit 1; }

BIT="$HWDIR/top_wrapper.bit"
MMI="$HWDIR/top_wrapper.mmi"
if [ ! -f "$BIT" ] || [ ! -f "$MMI" ]; then
    echo "Extracting $REPO/release/top_wrapper.xsa ..."
    unzip -o -q "$REPO/release/top_wrapper.xsa" -d "$HWDIR"
fi
test -f "$BIT" && test -f "$MMI"

OUTDIR="$(cd "$(dirname "$ELF")" && pwd)"
DL="$OUTDIR/download.bit"
MCS="$OUTDIR/boot.mcs"

echo "== updatemem: merging $(basename "$ELF") into bitstream =="
updatemem -force \
    -meminfo "$MMI" \
    -data    "$ELF" \
    -bit     "$BIT" \
    -proc    top_i/microblaze_riscv_0 \
    -out     "$DL"

echo "== write_cfgmem: $IFACE flash image =="
TCL="$(mktemp --suffix=.tcl)"
cat > "$TCL" <<EOF
write_cfgmem -force -format mcs -size 4 -interface $IFACE \
    -loadbit "up 0x0 $DL" -file $MCS
exit
EOF
vivado -mode batch -nojournal -nolog -notrace -source "$TCL"
rm -f "$TCL"

echo ""
echo "OK: $MCS"
echo "Next: Vivado Hardware Manager > Add Configuration Memory Device"
echo "      (mx25l3273f-spi-x1_x2_x4 (Macronix) or n25q32-3.3v, check IC3 marking)"
echo "      > Program Configuration Memory Device > $MCS, then power-cycle the board."
