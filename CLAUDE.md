# CLAUDE.md — Project Handover & Working Guide

Written 2026-07 by the model that co-built most of this repo, as a handover to
future collaborators. Everything under "verified" was proven by build runs,
rendered output, or tests on the physical board — treat it as ground truth
until you re-verify it yourself.

## What this project is

- Teaching material for NCKU "Microprocessor Principles and Applications"
  (微處理機原理與應用). The user is the instructor.
- A Digilent **Cmod A7-35T** (Artix-7 xc7a35tcpg236-1) runs a **MicroBlaze-V
  soft RISC-V MCU**. To students it is presented as a normal commercial MCU —
  **not** as an FPGA design. This persona drives many decisions: student-facing
  docs and diagrams hide Xilinx IP names, tool parameters, and synthesis
  coefficients (write "PLL", "Memory Arbiter", "SRAM Controller", "Debug Unit" —
  never "Clocking Wizard", "SmartConnect", "EMC", "MDM", "M_AXI_*", "C_*").
- Converse with the user in Traditional Chinese; write repo docs in English.

## Non-negotiable working rules (user-mandated)

1. **NEVER hand-edit `RISC-V-MCU/top.tcl`.** Change the block design in Vivado,
   export with `write_bd_tcl -force top.tcl`, grep-verify key parameters
   survived, then prove equivalence by wipe + recreate + rebuild (see SOP).
2. **Never `git commit` or `git push` unless explicitly asked.** No AI
   co-author trailers. The user reviews and commits everything personally.
3. **Verify before claiming.** The user checks everything and will ask for
   proof ("你要確定可以run喔"). Renders get looked at page by page, builds get
   run, board claims get tested on the board. If you generate an image or PDF,
   render it to PNG and *look at it* before saying it is done.
4. **Markdown under `docs/` is the single source of truth.** All PDFs and the
   unified datasheet are build artifacts. Never edit generated output.
5. Answer the user's question before fixing anything. When they describe a
   problem, the deliverable is the diagnosis; wait for their call to change
   things (they often choose between options you present).

## System architecture (verified)

- CPU: MicroBlaze-V (`microblaze_riscv_0`), rv32imb_zicsr_zifencei_zbc_zicbom,
  100 MHz (12 MHz osc × MMCM ×25/3). 16 KB I$ + 16 KB D$, write-through,
  32-byte lines, cacheable range exactly the SRAM: `0x6000_0000–0x6007_FFFF`.
- Memory: BRAM 128 KB = 32 K bootloader @0x0 + 32 K ITCM @0x8000 + 64 K DTCM
  @0x10000 (stack top-down from 0x20000); async SRAM 512 KB @0x6000_0000 (app
  code/data, cached); QSPI NOR flash 4 MB (bitstream @0x0, app slot @0x300000,
  register-mode controller — **no XIP**, deliberate: self-programming chosen
  over execute-in-place).
- Bus: six-port topology. `axi_periph` (SmartConnect 1→22) fans DP out to 21
  peripherals + M21 (a DP window to SRAM); `smartconnect_0` (4→1) merges
  IP/IC/DC/M21 into `axi_emc_0` → SRAM. Port choice is pure address decode.
- Peripherals at class-based addresses `0x40[C]x_xxxx` (class: 0=GPIO 1=Timer
  2=PWM 3=UART 4=INTC 5=QSPI-ctrl 6=XADC 7=I2C 8=SPI). INTC has 8 inputs:
  Timer×3, UART×2, EXTI(4-bit), I2C, SPI — the xlconcat is FULL.
- I2C 100 kHz with 50 ns input glitch filters (C_SCL/SDA_INERTIAL_DELAY=5).
  SPI: **software-settable clock ≈1.6–25 MHz** (default 6.25 MHz at power-on),
  2 SS. Implementation (hidden from students): a second clk_wiz `spi_0_clk`
  (USE_DYN_RECONFIG, AXI @0x4090_0000, M22 on the 1→23 crossbar) feeds
  `spi_0`'s ext_spi_clk; spi_0 has Async_Clk=1, C_SCK_RATIO=4. The BD cell
  name `spi_0_clk` makes xparameters read XPAR_SPI_0_CLK_* — persona intact.
  UART ×2 (16550): `uart_USB` (J18/J17) and `uart_1` (DIP 11/12). 115200 8N1
  everywhere; at 100 MHz the 16550 divisor for 115200 is 54.

## Hardware truths that cost real debugging — do not re-derive them wrongly

- **MB-V has no software cache on/off.** `Xil_*CacheEnable/Disable` are empty
  macros; boot.S only sets mtvec. Cache maintenance = custom-encoded
  `cbo.flush/inval` sweeping line indexes. Caches are effectively always on
  for the cacheable range.
- **Vitis app builds default to -O0.** Benchmarks lie unless the measured
  function is marked `__attribute__((optimize("O2"),noinline))`. This once made
  D$ look broken (warm≈cold); the cache was fine. Verified numbers: cached hit
  ≈7.2 cyc/word (incl. loop), miss ≈26 cyc/word (≈151 cyc per 32 B line from
  async SRAM), write-through store ≈23.6 cyc.
- **DIP header is U-numbered**: pin k faces pin 49−k (1↔48, 24↔25). GPIO_D is
  deliberately reversed in the XDC (D0@32 … D6@26) so group D mirrors group B
  across the header, matching the A/C mirror.
- **Physical orientation** (photo-verified): VU(24)/GND(25) at the **USB end**;
  pins 1/48 at the **Pmod end**; Pmod-up top view ⇒ left column 48→25, right
  column 1→24. Silk: LD0 = RGB LED (centerline), LD1/LD2 = status LEDs to its
  right; buttons on the centerline.
- **Buttons**: user button = **BTN1 (B18)** (board interface `push_buttons_1bit`
  = "Just use BTN1"); reset = **BTN0 (A18), ACTIVE-HIGH** (board.xml
  `rst_polarity=1`, applied via board flow — the `POLARITY {ACTIVE_LOW}` on the
  BD port is stale metadata with no functional effect). The docs said A18 /
  active-low until 2026-07-06; both were wrong. `board/Board-Files/**/board.xml`
  is the ground truth for board wiring questions.
- **Artix-7 I/O absolute max** = −0.40 V to VCCO+0.55 V = **3.85 V** (DS181);
  not 5 V tolerant; no series resistors on DIP pins. Analog pins 15/16 accept
  0–3.3 V via on-board divider (2.32k/1k ≈ 0.301) into the XADC's 0–1 V.
- `axi_quad_spi` C_SCK_RATIO only allows {2,4,8,16} (component.xml). STARTUPE2
  is unique per chip and owned by the flash controller — keep `spi_0`
  C_USE_STARTUP=0. The flash-controller interrupt is intentionally unconnected.
- **Async-clock SPI (spi_0) survival rules — a full day of board debugging
  (2026-07-07), do not relearn them:**
  1. `C_SCK_RATIO=2` + standard master has a broken RX path (special-cased
     RTL; bytes vanish/overrun). Use ratio 4.
  2. **The stock `XSpi_Transfer` does not work with Async_Clk=1**: its
     FIFO-reset→refill races the reset's CDC crossing (data swallowed, TX
     stays empty, IPISR never fires), and its status-based pacing trusts
     flags that lag. Use register-level transfers.
  3. Rules for reliable register-level transfers: (a) after SRR or the CR
     FIFO-reset bits, wait ≥ a dozen SPI clocks (~10 µs at the slowest
     setting) before the next write — writes inside the window are eaten;
     (b) self-pace to ≤15 bytes in flight (FIFO=16, TX_FULL lags — unpaced
     bursts measured 15/32 bytes lost); (c) drain RX by the SR RX_EMPTY
     *level*, never by occupancy counts (lag both ways) or IPISR edges.
     Working reference: `workspace-example/examples/07_spi_clock/src/main.c`
     and demo_all stage 6.
  4. The xclk_wiz driver's `SetRateHz` is unit-broken under SDT (PrimInClkFreq
     stored in MHz, math expects Hz → always fails). Program the MMCM
     directly: `0x200=0x0801` (100 MHz ×8 → 800 MHz VCO), `0x208=N`
     (SCK = 200 MHz / N, N=8..128), `0x25C=3`, poll `0x004` bit0 (locked).
  5. The chosen clock persists across `xsdb dow` reloads; power-cycle or
     JPROGRAM restores the 25 MHz default (SCK 6.25 MHz).
  6. WNS baseline changed with this logic: +0.292 (2026-07-07). The old
     +0.225 fingerprint no longer applies.

## Rebuild SOP (hardware change)

```
# after editing the BD and write_bd_tcl -force top.tcl:
cd RISC-V-MCU && rm -rf RISC-V-MCU.* .Xil       # wipe project (top.tcl/recreate stay)
vivado -mode batch -source recreate_project.tcl # recreate from tcl
# in the same tcl or a follow-up: launch_runs impl_1 -to_step write_bitstream
# -jobs 12; wait_on_run; assert PROGRESS==100%; open_run impl_1
# WNS fingerprint: identical WNS to 3 decimals across a wipe+rebuild proves the
# design is unchanged (deterministic builds). 2026-07-06 baseline: WNS +0.225.
write_hw_platform -fixed -include_bit -force ../release/top_wrapper.xsa
cp RISC-V-MCU.runs/impl_1/top_wrapper.bit ../release/top.bit
rm -rf ../release/top_wrapper/   # so make_boot_mcs.sh re-extracts the new XSA
tools/make_boot_mcs.sh <bootloader.elf> --hw release/top_wrapper/  # -> boot.mcs
```
Also regression-check gpio_D LOCs (W2,U1,T2,T1,R2,T3,R3 for bits 0..6) after any
XDC/BD change. Flashing with ADDRESS_RANGE use_file preserves the app slot.

## Boot & software invariants

- Bootloader (32 K BRAM, part of the bitstream, restored every power-on =
  unbrickable) listens on UART ~1 s: upload protocol → flash+run; silence →
  copy app from flash slot 0x300000 to SRAM → run. Hold the user button (BTN1)
  at power-on to stay in the bootloader.
- The bootloader stays **dumb**: it moves one contiguous blob to SRAM_BASE.
  Scatter (ITCM copy) is the app startup's job — `mcu_init()` in the template
  (renamed from `tcm_init` 2026-07-07) copies `.itcm.text` from its SRAM load
  address (AT> sram), zeroes DTCM, `fence.i`, and sets the UART to 115200.
  Template rule taught to students: `mcu_init();` is the first line of main().
  `tools/upload.py` places PT_LOAD segments by LMA (p_paddr, fallback vaddr) =
  same semantics as `xsdb dow`.
- Same ELF runs under Vitis JTAG Run/Debug and upload.py — no rebuild.
- `workspace-example/SRAM_app_template/src/lscript.ld` is the canonical linker
  script (sram/itcm/dtcm regions, `ITCM_FUNC`/`DTCM_DATA`, stack in DTCM,
  ASSERT on stack size). Vitis GUI linker editor and hand-edits don't mix —
  the GUI regenerates and destroys AT>/ASSERT; edit by hand only.
- The bootloader's own `lscript.ld` (local_memory LENGTH shrunk to 0x8000) is
  checked in at `workspace-example/bootloader/src/` — rescued 2026-07-07 after
  it was found to exist only in a session workspace. If Vitis regenerates it,
  LENGTH reverts to 0x10000 and the bootloader would overlap the ITCM.
- BSP gotcha: stdin/stdout must be `uart_USB` (default may be uart_1), and
  plain templates need `XUartNs550_SetBaud(..., 115200)` at the top of main
  (the course template's `mcu_init()` already does it).
- One-command flows (added 2026-07-07, all board-verified):
  `tools/vitis_new_app.py <name>` creates platform (stdin/stdout preset) + app
  from the template and builds it (re-execs itself under `vitis -s`);
  `tools/jtag_run.py <elf> [--bit …]` = xsdb dow+con + UART tail;
  `tools/upload.py <elf> --monitor` flashes and tails. The template prints a
  "memory tour" (main/ITCM/DTCM/stack addresses) at boot — it is both a
  teaching aid and the standard runtime check that the linker script works.
- **Trap: `release/top.bit` has an EMPTY BRAM — no bootloader.** JPROGRAMming
  it leaves the CPU executing nothing and the UART totally silent (cost an
  hour on 2026-07-07). `release/boot.bit` is the updatemem-merged twin
  (bitstream + bootloader): JPROGRAMming it == a remote power-cycle into the
  bootloader. Automation idiom for hostless upload tests: start
  `upload` with `sync(seconds=60)` FIRST, JPROGRAM `boot.bit` in parallel —
  the bootloader finds the host's 'U' already in the 16550 FIFO when it wakes.
  Regenerate after hardware changes:
  `updatemem -force -meminfo <top_wrapper.mmi from XSA> -data bootloader.elf
   -bit release/top.bit -proc top_i/microblaze_riscv_0 -out release/boot.bit`.

## Documentation pipeline

- Per-spec PDFs (next to each .md): `python3 tools/gen_spec_pdfs.py` — drives
  the VSCode markdown-pdf extension's bundled Chromium m80 over CDP with
  `--no-sandbox --disable-software-rasterizer --disable-dev-shm-usage`;
  CSS = extension styles + `docs/pdf-style.css`.
- Unified datasheet: `python3 docs/datasheet/build_datasheet.py` (needs the
  `typst` CLI — installed at `~/.local/bin/typst`, single static binary).
  Section-level reassembly driven by `RECIPE`; deliberate omissions live in
  `DROPPED` with reasons; a **coverage check fails the build** if a source
  section is renamed/added without updating them — that is by design. A
  reference-repair table rewrites cross-doc phrases into live `@` refs and
  warns when a pattern stops matching. Chapter 1 content lives in
  `docs/datasheet/overview.md` (the only datasheet-specific source).
  The Vitis quick reference and the JTAG IDE walkthrough are deliberately NOT
  in the datasheet (user's call: datasheet = what the chip is; guides = how to
  use tools). Don't re-add them.
- Diagrams: hand-written B&W SVGs, Microchip-datasheet style. Rules: explicit
  white background rect (GitHub dark mode), Liberation Sans, thick bus lines
  with /32 slashes, thin = single-beat, generic MCU vocabulary only. Pinout
  diagram wiring must be re-verified after any edit:
  `python3 tools/check_pinout_diagram.py` (diffs SVG vs pin-spec tables and
  checks the k/49−k pairing).
- Verify visual output by rendering:
  `typst compile --format png --ppi 100 wrapper.typ out.png` then Read the PNG.

## Style rules for student-facing docs

- Pin spec = "純純的pin腳": pins only — no AXI addresses, no IP names, no HDL
  port names.
- IP spec = qualitative interface description: capabilities, addresses, pins,
  interrupts, drivers — no `C_*` parameters, no IP version strings ("the real
  parameters live in top.tcl; the doc describes behavior").
- Figure captions: no provenance notes ("redrawn from …" was removed on request).
- Real Xilinx parameter names belong in top.tcl and nowhere else.

## Open threads (as of 2026-07-07)

- `demo_all` flashed on the board still has the -O0 benchmark → its printed
  memory numbers are misleading; fix = BENCH attribute + reflash (offered,
  never scheduled).
- Planned labs: ISR-in-ITCM vs ISR-in-SRAM latency-jitter demo; bit-bang I2C;
  SPI max-clock breadboard failure demo. Sensor examples when I2C/SPI modules
  arrive.
- Datasheet polish backlog (user aware, undecided): revision-history page,
  glossary, abs-max vs recommended-operating split in §5.1, instructor
  sections → appendix, power LED in the pinout figure.
- `examples/` + `demo_all` still use the old all-BRAM linker script (they work;
  migrate to the template whenever convenient).

## How to work with this user

- They know this board better than any photo — when they correct a physical
  detail, they are right; update the docs and say what you verified.
- They iterate visuals in small rounds ("往中間一點", "順序不對") — expect
  several quick refinement cycles; keep each one fast and re-render every time.
- They value evidence over confidence and honesty over polish: if something
  failed or you didn't run it, say so plainly. Corrections of your own earlier
  claims are welcomed, not punished.
- Small, verifiable steps beat big-bang changes. Propose labs/ideas at the end
  of an answer without asking permission to continue routine work.
