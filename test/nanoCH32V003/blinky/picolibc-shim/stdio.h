/*
 * picolibc compatibility shim for ch32fun.
 *
 * ch32fun is written for newlib and provides its own `putchar()` (and friends)
 * to retarget printf onto the debug interface. Debian's picolibc defines
 * `putchar`/`getchar`/`putc`/`getc` as unconditional function-like macros, which
 * mangle ch32fun's `WEAK int putchar(int c)` definition into a syntax error.
 *
 * This header is placed first on the include path (via NEWLIB in the Makefile).
 * It pulls in the real picolibc <stdio.h> via #include_next, then undefines the
 * offending convenience macros so the plain function declarations remain.
 */
#ifndef _CH32FUN_PICOLIBC_STDIO_SHIM_H
#define _CH32FUN_PICOLIBC_STDIO_SHIM_H

#include_next <stdio.h>

#undef putchar
#undef getchar
#undef putc
#undef getc

#endif /* _CH32FUN_PICOLIBC_STDIO_SHIM_H */
