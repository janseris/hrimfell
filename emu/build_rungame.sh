#!/bin/sh
# (v0.4.75) the C emulator driver (emu/rungame.c): Unicorn's library linked with its path built in
U=/home/claude/unicorn
cc -O2 -Wall -Wno-unused-result -o /home/claude/game/emu/rungame /home/claude/game/emu/rungame.c -I$U/include -L$U/build -lunicorn -Wl,-rpath,$U/build -lz -lpthread -lm
