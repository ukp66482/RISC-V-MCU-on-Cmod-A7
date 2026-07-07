# showcase — every major MCU feature, one program, one breadboard

One application that exercises the full peripheral set of the RISC-V MCU
and ties it together into a live control loop:

| # | Feature | Hardware | Pins |
|---|---------|----------|------|
| 1 | I2C | 16×2 character LCD (PCF8574 backpack) | DIP 13 (SCL), DIP 14 (SDA) |
| 2 | SPI | ILI9341 240×320 color TFT dashboard | DIP 35–38 + DIP 47/48 |
| 3 | PWM | hobby servo (SG90 class), 50 Hz | DIP 10 |
| 4 | ADC | potentiometer, live millivolts | DIP 15 |
| 5 | Timer IRQ | 100 Hz system tick, ISR in ITCM | — (internal) |
| 6 | Button IRQ | push button on external interrupt INTR_0 | DIP 8 |
| 7 | UART | telemetry + commands on the DIP UART | DIP 11 (TX), DIP 12 (RX) |

The pieces interact instead of running side by side: the tick interrupt
schedules everything, the knob (or the auto-sweep, or a serial command)
steers the servo, both displays and both serial ports show the same live
state, and the button — through a real GPIO interrupt — switches the
control mode.

**Modes** (cycle with the wired button, the on-board BTN1, or the `m` command):

| Mode | Name | Servo behavior |
|------|------|----------------|
| A | POT | follows the potentiometer |
| B | SWEEP | sweeps 0→180→0 by itself (7.2 s period) |
| C | MANUAL | moved ±10° by the `a` / `d` commands |

Every stage degrades gracefully: missing hardware is reported once on the
USB console and the rest keeps running, so the breadboard can be wired up
piece by piece and re-tested after each part (press reset, or re-run).

## Wiring

Power rails first — read this before plugging anything:

- **VU = DIP 24** is the 5 V rail (from USB). **GND = DIP 25.**
- **3.3 V comes from the Pmod connector (J2) VCC pins** — the DIP header
  itself has no 3.3 V pin.
- The MCU I/O pins are **3.3 V only, not 5 V tolerant**. Nothing that
  drives a DIP pin may ever pull it above 3.3 V.

| Device | Module pin | Board pin |
|--------|-----------|-----------|
| LCD1602 backpack | VCC / GND | Pmod 3.3 V / GND |
| | SCL / SDA | DIP 13 / DIP 14 |
| ILI9341 TFT | VCC / GND | VU (module has its own regulator) / GND |
| | LED (backlight) | Pmod 3.3 V (≈50 mA — never from a GPIO) |
| | CS / SCK / SDI(MOSI) / SDO(MISO) | DIP 38 / 35 / 36 / 37 |
| | DC / RESET | DIP 48 / DIP 47 |
| Servo SG90 | orange (signal) | DIP 10 |
| | red / brown | VU / GND |
| Potentiometer 10k | ends | Pmod 3.3 V and GND (**never VU**) |
| | wiper | DIP 15 |
| Push button | one side | DIP 8, plus 10 kΩ pull-up to 3.3 V |
| | other side | GND |
| USB-TTL adapter (3.3 V!) | RX / TX / GND | DIP 11 / DIP 12 / GND |

Notes:

- The LCD backpack's own 4.7 kΩ pull-ups (to its 3.3 V supply) are exactly
  what the bus needs. If the display is unreadably dim at 3.3 V even with
  the contrast pot at the end, power the module from VU **through an I2C
  level shifter** — never connect 5 V-pulled SCL/SDA directly.
- DIP 8 has no internal pull-up; without the external 10 k the pin floats.
  The firmware detects a floating/noisy INTR_0 (interrupt storm) and mutes
  the button IRQ with a console message instead of hanging.
- The servo draws its current from VU; a single unloaded SG90 is fine on
  USB power, anything bigger deserves its own 5 V supply (common GND).

## Build and run

The sources follow the standard SRAM application template (same
`lscript.ld`, `mcu_init()` first). Create a Vitis app component from these
sources against the release platform, or reuse the prebuilt component in
the workspace, then:

```
python3 tools/jtag_run.py workspace-example/showcase/build/showcase.elf   # volatile
python3 tools/upload.py   workspace-example/showcase/build/showcase.elf  # boots from flash
```

## Serial interfaces

- **USB console** (115200 8N1): boot banner with the wiring table,
  init status for every feature, a 1 Hz status line, and single-key
  commands — `m` next mode, `a`/`d` servo −/+10° (mode C), `r` reset the
  button counter, `h` help.
- **DIP UART** (115200 8N1): 1 Hz CSV telemetry
  `$MCU,<uptime_s>,<pot_mV>,<angle>,<mode>,<button_count>` and it accepts
  the same command keys — a second computer (or another MCU) can watch and
  steer the board with no USB involved.

## Code layout

Each peripheral is a self-contained module pair that can be lifted into
your own project on its own:

| File | Contents |
|------|----------|
| `main.c` | tick/button ISRs, mode state machine, scheduler, console |
| `lcd1602.c/.h` | PCF8574 backpack driver (probe, 4-bit init, 2 lines) |
| `ili9341.c/.h` | TFT driver: SPI clock setting, text, bars, dashboard |
| `font5x7.h` | 5×7 pixel font (generated, column-major) |
| `servo.c/.h` | 50 Hz PWM, glitch-free width updates |
| `adc.c/.h` | potentiometer millivolts + die temperature |
| `uart1.c/.h` | DIP-header UART, polled |
| `showcase.h` | shared tick counter and register helpers |

Teaching hooks baked in: the tick ISR runs from ITCM (the boot log prints
its address — compare with `lscript.ld`), the SPI transfer code documents
the flow-control rules for the adjustable-clock SPI unit, and the TFT
identify step runs at 2 MHz while drawing runs at 12.5 MHz — the SPI clock
is just a function call (`examples/07_spi_clock`).

## Troubleshooting

| Symptom | Likely cause |
|---------|--------------|
| `LCD1602 ... not found` | wrong address (try the other: 0x27/0x3F are probed), missing pull-ups, wrong pins |
| `ILI9341 ... no ID` but display works | module without MISO connection — cosmetic only |
| `ILI9341 ... no ID` and display dark | check LED pin has 3.3 V, RESET on DIP 47, DC on DIP 48 |
| `button IRQ muted: INTR_0 is floating` | add the 10 k pull-up to 3.3 V, then reset |
| servo twitches but won't hold | weak 5 V supply — use an external one, common GND |
| garbled TFT at long wires | lower `TFT_SCK_HZ` in `ili9341.c` (e.g. 6250000) |
