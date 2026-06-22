# nanoCH32V003 — Blinky

Minimal LED blink for the [MuseLab nanoCH32V003](../../../Documentation/nanoCH32V003/) board
(WCH CH32V003F4U6), built with the open-source [ch32fun](https://github.com/cnlohr/ch32v003fun)
toolchain and flashed with a **WCH-LinkE** probe over the 1-wire SDI interface.

The onboard user LED (D1, via R1) is on **PD6** — see [the board schematic](../../../Documentation/nanoCH32V003/hardware/nanoCH32V003.pdf).

## Layout

| File | Purpose |
|------|---------|
| `blinky.c` | Application — toggles PD6 every 250 ms |
| `funconfig.h` | ch32fun compile-time configuration (defaults are fine) |
| `Makefile` | Points `CH32FUN` at the `third_party/ch32v003fun` submodule |

## Prerequisites

1. **RISC-V bare-metal toolchain** (Debian/Ubuntu):
   ```bash
   sudo apt install gcc-riscv64-unknown-elf picolibc-riscv64-unknown-elf
   ```
   ch32fun auto-detects the `riscv64-unknown-elf-` prefix.

2. **minichlink** (the WCH-LinkE flasher bundled with ch32fun):
   ```bash
   sudo apt install libusb-1.0-0-dev libudev-dev
   make -C ../../../third_party/ch32v003fun/minichlink
   ```

3. **WCH-LinkE udev rule** so flashing works without `sudo`
   (see ch32fun docs / `minichlink/99-WCH-LinkE.rules`).

## Build & flash

```bash
make          # build blinky.bin/.elf and flash via WCH-LinkE
make clean    # remove build artifacts
```

Wiring: connect the WCH-LinkE `SWDIO/DIO`, `3V3`, and `GND` to the board
(the nanoCH32V003 exposes these on its debug header).
