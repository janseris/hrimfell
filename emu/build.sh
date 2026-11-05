#!/bin/sh
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
B=$HERE/../
OUT=/home/claude/bench/emu/out; mkdir -p $OUT; rm -f $OUT/*.o
export PATH=/home/claude/sym/gcc539/bin:$PATH
CFLAGS="-march=armv4t -mthumb-interwork -pipe -c -nostdinc -fomit-frame-pointer -O2 -mno-short-load-bytes $EXTRA"
arm-epoc-pe-gcc $CFLAGS -I $B/inc -o $OUT/r3d.o $B/src/r3d.cpp
arm-epoc-pe-gcc $CFLAGS -I $B/inc -o $OUT/glue.o $HERE/emu_glue.cpp
arm-epoc-pe-as -mapcs-32 -mthumb-interwork -o $OUT/spans.o $B/src/spans.s
LIBGCC=$(arm-epoc-pe-gcc -print-libgcc-file-name)
arm-epoc-pe-ld -o $OUT/core.exe -Ttext 0x10000 --image-base 0 -e emu_init $OUT/*.o $LIBGCC
arm-epoc-pe-nm $OUT/core.exe > $OUT/core.sym
arm-epoc-pe-size $OUT/core.exe 2>/dev/null || true
