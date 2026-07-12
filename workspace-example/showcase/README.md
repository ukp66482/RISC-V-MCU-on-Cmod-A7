# showcase — every major MCU feature, one program, one breadboard

This application exercises the full peripheral set of the RISC-V MCU
and ties the peripherals together into a live control loop:

| # | Feature | Hardware | Pins |
|---|---------|----------|------|
| 1 | SPI | frames to an ESP32 receiver station | DIP 35–38 |
| 2 | I2C | telemetry to the same receiver (slave 0x28) | DIP 13 (SCL), DIP 14 (SDA) |
| 3 | PWM | hobby servo (SG90 class), 50 Hz | DIP 10 |
| 4 | ADC | potentiometer, live millivolts | DIP 15 |
| 5 | Timer IRQ | 100 Hz system tick, ISR in ITCM | — (internal) |
| 6 | Button IRQ | push button on external interrupt INTR_0 | DIP 8 |
| 7 | UART | telemetry + commands on the DIP UART | DIP 11 (TX), DIP 12 (RX) |

The features interact rather than running independently: the tick
interrupt schedules all activity, the potentiometer (or the automatic
sweep, or a serial command) steers the servo, both serial ports and both
buses carry the same live state, and the button switches the control mode
through a GPIO interrupt.

**Modes** (cycle with the wired button, the on-board BTN1, or the `m` command):

| Mode | Name | Servo behavior |
|------|------|----------------|
| A | POT | follows the potentiometer |
| B | SWEEP | sweeps 0→180→0 by itself (7.2 s period) |
| C | MANUAL | moved ±10° by the `a` / `d` commands |

Every stage degrades gracefully: missing hardware is reported once on the
USB console and the remaining features continue to run, so the breadboard
can be wired and re-tested one part at a time (press reset, or re-run).

## The receiver station (SPI + I2C demo)

Flash `esp32_bridge/esp32_bridge.ino` to an ESP32 DevKit with the Arduino
toolchain, power it from its own USB, and open its serial monitor at
115200. The showcase application detects the receiver at boot (I2C
address 0x28) and then streams the telemetry line over both buses once a
second; `[I2C]` and `[SPI]` lines appear on the monitor. The `n` command
runs a round-trip PASS/FAIL test on both buses.

All signals are 3.3 V and are wired directly with no level shifting. The ESP32 has
hardware I2C and SPI slaves, so no particular power-up order is required.

## Wiring

Read the power-rail notes before wiring anything:

- **VU = DIP 24** is the 5 V rail (from USB). **GND = DIP 25.**
- **3.3 V comes from the Pmod connector (J2) VCC pins**; the DIP header
  itself has no 3.3 V pin.
- The MCU I/O pins are **3.3 V only, not 5 V tolerant**.

| Device | Module pin | Board pin |
|--------|-----------|-----------|
| ESP32 DevKit | GPIO 22 (SCL) / GPIO 21 (SDA) | DIP 13 / DIP 14 |
| | GPIO 18 / 23 / 19 / 5 | DIP 35 (SCK) / 36 (MOSI) / 37 (MISO) / 38 (CS) |
| | GND | GND (power from its own USB) |
| Servo SG90 | orange (signal) | DIP 10 |
| | red / brown | VU / GND |
| Potentiometer 10k | ends | Pmod 3.3 V and GND (**never VU**) |
| | wiper | DIP 15 |
| Push button | one side | DIP 8, plus 10 kΩ pull-up to 3.3 V |
| | other side | GND |
| USB-TTL adapter (3.3 V!) | RX / TX / GND | DIP 11 / DIP 12 / GND |

Notes:

- DIP 8 has no internal pull-up; without the external 10 k the pin floats.
  The firmware detects a floating/noisy INTR_0 (interrupt storm) and mutes
  the button IRQ with a console message instead of hanging.
- The servo draws its current from VU. A single unloaded SG90 operates
  correctly on USB power; larger loads require a separate 5 V supply
  (common GND).

## Build and run

```
python3 tools/vitis_build.py showcase                                     # build
python3 tools/jtag_run.py workspace-example/showcase/build/showcase.elf  # volatile
```

To make the application permanent, deploy the ELF to flash; see the
[Standalone Boot chapter](../../docs/datasheet/sections/standalone-boot.md).

Build and upload the receiver sketch once with arduino-cli:

```
arduino-cli compile --fqbn esp32:esp32:esp32 workspace-example/showcase/esp32_bridge
arduino-cli upload -p /dev/ttyUSBx --fqbn esp32:esp32:esp32 workspace-example/showcase/esp32_bridge
```

## Serial interfaces

- **USB console** (115200 8N1): boot banner with the wiring table,
  init status for every feature, a 1 Hz status line, and single-key
  commands: `m` next mode, `a`/`d` servo −/+10° (mode C), `r` reset the
  button counter, `n` bridge round-trip test, `h` help.
- **DIP UART** (115200 8N1): 1 Hz CSV telemetry
  `$MCU,<uptime_s>,<pot_mV>,<angle>,<mode>,<button_count>`; the same
  command keys are accepted, so a second computer (or another MCU) can
  monitor and control the board with no USB connection.

## Code layout

Each peripheral is implemented as a self-contained module pair that can
be reused independently in another project:

| File | Contents |
|------|----------|
| `main.c` | tick/button ISRs, mode state machine, scheduler, console, bridge |
| `spi0.c/.h` | external SPI port: clock setting + watchdogged transfers |
| `servo.c/.h` | 50 Hz PWM, glitch-free width updates |
| `adc.c/.h` | potentiometer millivolts + die temperature |
| `uart1.c/.h` | DIP-header UART, polled |
| `showcase.h` | shared tick counter and register helpers |
| `esp32_bridge/` | Arduino sketch for the receiver station |

Notable implementation details: the tick ISR runs from ITCM (the boot log
prints its address; compare with `lscript.ld`), `spi0.c` documents the
flow-control rules for the adjustable-clock SPI unit, and the SPI clock is
set with a single function call (`spi0_set_clock()`).

## Troubleshooting

| Symptom | Likely cause |
|---------|--------------|
| `ESP bridge ... not detected` | bridge not powered/wired, or wrong pins — its monitor must show "ready" first; then press `b` to re-probe |
| bridge SPI FAIL, I2C PASS | check MISO (DIP 37 ← GPIO 19); to isolate the MCU side, jumper DIP 36 ↔ DIP 37 and the SPI leg should echo |
| `button IRQ muted: INTR_0 is floating` | add the 10 k pull-up to 3.3 V, then reset |
| servo twitches but does not hold | weak 5 V supply — use an external one, common GND |
