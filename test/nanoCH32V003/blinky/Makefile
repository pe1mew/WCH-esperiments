all : flash

TARGET:=blinky

TARGET_MCU?=CH32V003

# Debian/Ubuntu ships the RISC-V C headers via the picolibc-riscv64-unknown-elf
# package. picolibc isn't directly compatible with ch32fun (it macro-defines
# putchar/getchar, which clash with ch32fun's own definitions), so we put a small
# shim dir first on the include path and add the real picolibc headers after it.
PICOLIBC_INCLUDE?=/usr/lib/picolibc/riscv64-unknown-elf/include
NEWLIB?=picolibc-shim
EXTRA_CFLAGS+=-I$(PICOLIBC_INCLUDE)

# ch32fun lives in the third_party submodule at the repo root.
CH32FUN?=../../../third_party/ch32v003fun/ch32fun
include $(CH32FUN)/ch32fun.mk

# Targets:
#   make / make flash  -> compile and flash to the board via WCH-LinkE
#   make build         -> compile only (produces blinky.elf/.bin/.hex)
#   make clean         -> remove build artifacts
build : $(TARGET).bin
flash : cv_flash
clean : cv_clean

.PHONY : build flash clean
