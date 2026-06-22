# WCH CH32V003 Toolchain Setup (Linux / Ubuntu)

A complete, reproducible guide for developing and flashing the **WCH CH32V003**
using the open-source **ch32fun** toolchain and a **WCH-LinkE** programmer.
Written so the whole thing can be redone from scratch without assistance.

Verified on:

- **OS:** Ubuntu 24.04.4 LTS
- **Compiler:** `riscv64-unknown-elf-gcc` 13.2.0 (Ubuntu apt package)
- **C library:** picolibc 1.8.6 (Ubuntu apt package)
- **Toolchain:** [ch32fun](https://github.com/cnlohr/ch32v003fun) (commit `afa13aa`)
- **Board:** MuseLab **nanoCH32V003** (CH32V003F4U6)
- **Programmer:** WCH-LinkE (firmware LinkE v2.15)

---

## 1. Background — what the hardware can and cannot do

- The **CH32V003** is a RISC-V **RV32EC** MCU: 16 KB flash, 2 KB RAM.
  The `EC` ISA is a 16-register embedded variant — the compiler must target
  `-march=rv32ec -mabi=ilp32e`. A normal `rv32imac` build will **not** run.
- The chip has **no USB peripheral at all**. You therefore **cannot** program it
  by plugging the chip's USB into the PC. On the nanoCH32V003 the **USB-C port is
  power only**.
- Programming happens over WCH's **single-wire SDI debug interface** on pin
  `PD1 / SWIO`. The **WCH-LinkE** probe bridges your PC's USB to that single wire.
- (Alternative not used here: the '003 also has a UART ISP bootloader on
  `PD5`/`PD6`, usable with a USB-serial adapter. The SDI/WCH-LinkE route is simpler.)

### nanoCH32V003 board facts

- On-board 24 MHz crystal, reset button, user LED.
- **User LED (D1, via R1) is on `PD6`.** (Confirmed from the board schematic.)
- Note `PD6` is also the UART-ISP RX pin — irrelevant for SDI flashing, but worth
  knowing if you ever use the serial bootloader.

### Wiring (WCH-LinkE → board)

| WCH-LinkE | nanoCH32V003 | Notes |
|-----------|--------------|-------|
| `SWDIO` / `DIO` | `SWIO` (PD1) | the single data wire |
| `3V3` | `3V3` | power/reference |
| `GND` | `GND` | common ground |

The WCH-LinkE's own USB-C goes to the PC. The board's USB-C is only needed if you
want to power it separately; for flashing, the LinkE can supply 3V3.

---

## 2. Install the toolchain

```bash
# RISC-V bare-metal compiler + C library headers
sudo apt install gcc-riscv64-unknown-elf picolibc-riscv64-unknown-elf

# Build tools and USB libraries needed to compile minichlink (the flasher)
sudo apt install build-essential libusb-1.0-0-dev libudev-dev
```

`build-essential` is required even though we cross-compile, because **minichlink
is a native (host) program** and needs the host `gcc` + `libc6-dev` headers.

---

## 3. Get ch32fun and build minichlink

ch32fun is included in this repo as a git submodule at
`third_party/ch32v003fun`. After a fresh clone:

```bash
git submodule update --init --recursive
```

If starting a brand-new repo instead:

```bash
git submodule add https://github.com/cnlohr/ch32v003fun third_party/ch32v003fun
```

Build the flasher once:

```bash
make -C third_party/ch32v003fun/minichlink
```

> The core library directory inside ch32fun is named **`ch32fun`** (not
> `ch32v003fun`). So the include/makefile path is
> `third_party/ch32v003fun/ch32fun`.

---

## 4. udev rule (flash without root)

By default the WCH-LinkE USB device is root-only. Install ch32fun's rule:

```bash
sudo cp third_party/ch32v003fun/minichlink/99-minichlink.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
```

Then **unplug and replug the WCH-LinkE** (udev rules only apply on a fresh
enumeration). The rule grants access via the `plugdev` group, so make sure your
user is in it:

```bash
id | grep -o plugdev || sudo usermod -aG plugdev "$USER"   # log out/in if added
```

---

## 5. Project layout

A minimal ch32fun project (see `test/nanoCH32V003/blinky/`):

```
blinky/
├── Makefile          # selects MCU, points at ch32fun, applies picolibc fix
├── blinky.c          # application
├── funconfig.h       # ch32fun compile-time config (defaults are fine)
└── picolibc-shim/
    └── stdio.h       # picolibc compatibility shim (see Lesson 1)
```

### Makefile (key parts)

```make
TARGET:=blinky
TARGET_MCU?=CH32V003

# picolibc compatibility (see Lessons section)
PICOLIBC_INCLUDE?=/usr/lib/picolibc/riscv64-unknown-elf/include
NEWLIB?=picolibc-shim
EXTRA_CFLAGS+=-I$(PICOLIBC_INCLUDE)

CH32FUN?=../../../third_party/ch32v003fun/ch32fun
include $(CH32FUN)/ch32fun.mk

build : $(TARGET).bin     # compile only
flash : cv_flash          # compile + flash
clean : cv_clean
.PHONY : build flash clean
```

### Build & flash

```bash
cd test/nanoCH32V003/blinky
make            # compile + flash  (same as `make flash`)
make build      # compile only, no hardware needed
make clean      # remove build artifacts
```

A successful flash ends with `Image written.` and reports the detected part:

```
Found WCH Link
WCH Programmer is LinkE version 2.15
Detected CH32V003
Flash Storage: 16 kB
Image written.
```

---

## 6. Lessons learned (the things that cost time)

These are the non-obvious problems hit during setup and exactly how each was fixed.

### Lesson 1 — picolibc vs newlib: the `putchar` macro clash ⚠️ (biggest one)

**Symptom:**

```
/usr/lib/picolibc/.../stdio.h:153: error: expected declaration specifiers ... before 'stdout'
  #define stdout stdout
  #define putchar(__c) fputc(__c, stdout)
ch32fun.c:1791: note: in expansion of macro 'putchar'
  WEAK int putchar(int c)
```

**Cause:** ch32fun is written for **newlib**, but Ubuntu's apt toolchain uses
**picolibc**. picolibc's `<stdio.h>` defines `putchar`/`getchar`/`putc`/`getc` as
unconditional function-like **macros**. ch32fun provides its own
`WEAK int putchar(int c)` (to retarget `printf` onto the debug interface), and the
macro mangles that definition into a syntax error. ch32fun includes `<stdio.h>`
**unconditionally**, so no `funconfig.h` option can avoid it.

**Fix — a tiny shim header**, placed *first* on the include path. It pulls in the
real picolibc header via `#include_next`, then neutralizes the offending macros:

`picolibc-shim/stdio.h`:

```c
#ifndef _CH32FUN_PICOLIBC_STDIO_SHIM_H
#define _CH32FUN_PICOLIBC_STDIO_SHIM_H
#include_next <stdio.h>
#undef putchar
#undef getchar
#undef putc
#undef getc
#endif
```

In the Makefile, point ch32fun's `NEWLIB` variable at the shim directory and add
the *real* picolibc headers afterwards (for `string.h`, `stdint.h`, etc.):

```make
NEWLIB?=picolibc-shim
EXTRA_CFLAGS+=-I/usr/lib/picolibc/riscv64-unknown-elf/include
```

Because ch32fun emits the include path as `-I$(NEWLIB) ... -I. ... $(EXTRA_CFLAGS)`,
the shim dir is searched **first** (so `<stdio.h>` → shim) while everything else
falls through to the real picolibc dir at the end.

> Alternative fix (not used): install the newlib-based **xpack
> `riscv-none-elf-gcc`** toolchain that ch32fun is actually tested against. That
> avoids the shim entirely but is a manual ~150 MB download instead of two apt
> packages.

### Lesson 2 — ch32fun's default `NEWLIB` path is wrong on Ubuntu

ch32fun defaults to `NEWLIB?=/usr/include/newlib`, which doesn't exist here.
Symptom: `fatal error: stdio.h: No such file or directory` from the **cross**
compiler. picolibc's headers live at
`/usr/lib/picolibc/riscv64-unknown-elf/include` — that's what the Makefile points to.

### Lesson 3 — relative `NEWLIB` path must be relative to the *build* dir

First attempt set `NEWLIB?=$(dir $(lastword $(MAKEFILE_LIST)))picolibc-shim`.
That expanded against **ch32fun.mk's** location (in `third_party/...`), not the
project, producing `third_party/.../ch32fun/picolibc-shim` — which doesn't exist,
so the shim was silently skipped and the error came back.
**Fix:** `make` runs in the project directory, so a plain relative path works:
`NEWLIB?=picolibc-shim`.

### Lesson 4 — minichlink needs host headers + USB dev libs

Building minichlink failed with `stdio.h: No such file or directory` from the
**host** gcc, and later with missing `-lusb-1.0` / `-ludev`.
**Fix:** `sudo apt install build-essential libusb-1.0-0-dev libudev-dev`.

### Lesson 5 — WCH-LinkE ships in "ARM mode"; it must be in "RISC-V mode"

**Symptom:**

```
Warning: found at least one WCH-LinkE in ARM programming mode.
Found programmer in ARM mode, but couldn't open it.
```

`lsusb` shows the probe as `1a86:8012` (**ARM mode**). For the CH32V003 it must be
in **RISC-V mode**, where it enumerates as `1209:b003`.

minichlink switches the mode **automatically**, but two things must be true:

1. The udev rule (Lesson/Section 4) must be installed so minichlink can *open* the
   ARM-mode device — otherwise it can't issue the switch (`couldn't open it`).
2. Switching causes the probe to **re-enumerate**, so the very first `make` after
   a replug may switch-then-bail with a "re-attempt" message. **Just run `make`
   again** and it flashes.

(If a particular LinkE has a physical mode button/jumper, that can also hold it in
ARM mode.)

### Lesson 6 — re-flashing identical firmware shows no visible change

Flashing the same `.bin` repeatedly produces an identical LED pattern — `make
clean` doesn't change that, since the same source rebuilds to the same bytes. To
**prove** a flash took effect, change something visible (e.g. make the on/off
delays different) and reflash; the changed pattern confirms the new image is
running.

---

## 7. Quick reference

```bash
# One-time setup
sudo apt install gcc-riscv64-unknown-elf picolibc-riscv64-unknown-elf \
                 build-essential libusb-1.0-0-dev libudev-dev
git submodule update --init --recursive
make -C third_party/ch32v003fun/minichlink
sudo cp third_party/ch32v003fun/minichlink/99-minichlink.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
# (replug the WCH-LinkE)

# Per build
cd test/nanoCH32V003/blinky
make            # build + flash
make build      # build only
make clean      # clean

# Diagnostics
lsusb | grep 1a86          # 1a86:8012 = ARM mode, 1209:b003 = RISC-V mode
third_party/ch32v003fun/minichlink/minichlink -h
```

---

## 8. References

- ch32fun: https://github.com/cnlohr/ch32v003fun
- WCH-LinkE ARM/RISC-V mode issue: https://github.com/cnlohr/ch32v003fun/issues/227
- nanoCH32V003 board: `Documentation/nanoCH32V003/`
- WCH-LinkE manual: `Documentation/WCH-LinkUserManual.PDF`
