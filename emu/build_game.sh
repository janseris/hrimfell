#!/bin/sh
set -e
G=/home/claude/game
OUT=$G/emu/gout; mkdir -p $OUT; rm -f $OUT/*.o
export PATH=/home/claude/sym/gcc539/bin:$PATH
CFLAGS="-march=armv4t -mthumb-interwork -pipe -c -nostdinc -fomit-frame-pointer -O2 -mno-short-load-bytes"
for f in r3d game game_ui game_meshes game_organic game_city game_cells game_props game_dungeon game_riverwood game_kit game_stage game_fhquest; do arm-epoc-pe-gcc $CFLAGS -I $G/inc -o $OUT/$f.o $G/src/$f.cpp; done
arm-epoc-pe-gcc $CFLAGS -I $G/inc -o $OUT/glue.o $G/emu/game_glue.cpp
arm-epoc-pe-as -mapcs-32 -mthumb-interwork -o $OUT/spans.o $G/src/spans.s
LIBGCC=$(arm-epoc-pe-gcc -print-libgcc-file-name)
arm-epoc-pe-ld -o $OUT/core.exe -Ttext 0x10000 --image-base 0 -e emu_init $OUT/*.o $LIBGCC
arm-epoc-pe-nm $OUT/core.exe > $OUT/core.sym
arm-epoc-pe-size $OUT/core.exe
