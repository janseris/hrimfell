# Hrimfell

A 3D role-playing engine for the **Nokia 9300 / 9500 Communicator** (Symbian Series 80 v2, Symbian 7.0s,
EKA1, ARM925T at 150 MHz, no FPU), built with the SDK's GCC 2.9. Whiterun, Riverwood, Bleak Falls Barrow and
Forelhost, drawn by its own fixed-point software renderer (`src/r3d.cpp`, hot loops in ARM assembly in `src/spans.s`).

> Hrimfell is not affiliated with or endorsed by Bethesda Softworks, ZeniMax Media or Microsoft. *The Elder Scrolls*
> and *Skyrim* are trademarks of Bethesda Softworks / ZeniMax Media, named here only to say what the tools read. See
> [LEGAL.md](LEGAL.md).

**No Skyrim data is in this repository.** The game's world (models, textures, terrain, placements, people, texts) is
made from the user's own copy of Skyrim Special Edition by the exporter and generators below; those outputs are listed
in `.gitignore` and must be generated locally before the game builds.

## Layout

| Where | What |
|---|---|
| `src/`, `inc/` | The game (`game*.cpp`), the renderer (`r3d.cpp`, `spans.s`), the Symbian app shell (`hrimfell_app.cpp`) |
| `tools/skyrim_area.py`, `skyrim_meshes.py` | The exporter: run on a PC with Skyrim SE installed (`python skyrim_area.py whiterun`, ...), writes `skyrim_<area>.zip` |
| `docs/maps/gen_*.py`, `tools/lib_pack.py` | From the exports: the generated sources (`src/kit_ext.inc`, `src/area_*/`, ...) and the packed data (`data/*.har`, `common.hlb`, `items.hit`) |
| `host/` | Host tests and tools: the game's own code built for the PC (`host/pvs.cpp`: what can be seen from where) |
| `emu/` | The phone build run in the Unicorn ARM emulator: `emu/rungame` (C driver: instructions a frame, profiles, cache model, alignment checks), `emu/run_game.py` (the reference) |
| `docs/WORLD.md`, `docs/WORKPLAN.md` | What the world is made of and how; the work log |
| `sis/hrimfell.pkg` | The install package's description |

## Building

1. Exports: `python tools/skyrim_area.py <area>` for riverwood, whiterun, bleakfalls, forelhost (and the meshes,
   kits, places exports the generators name - see `docs/WORLD.md`).
2. Generate and pack: `docs/maps/gen_kit.py` per area (`AREA=main|bfb|wr|fh`), `gen_dun_kit.py`, `gen_fh_books.py`,
   then `tools/lib_pack.py`, then `host/pvs` for each area file (what can be seen from where).
   The people's lines (`src/game_lines.inc`) are game text too: without your own, copy the placeholders -
   `cp src/game_lines_stub.inc src/game_lines.inc`.
3. The phone app: `build_app.sh` with `build.cfg` (GCC 2.9 `arm-epoc-pe-gcc -O2 -mno-short-load-bytes`, no writable
   static data), then `makesis sis/hrimfell.pkg` (the program) and `makesis sis/hrimfell_data.pkg` (the world's data:
   its own package, installed after the program and again only when the data changes).
   On the phone, the menu's *Benchmark (send to PC)* measures every place and sends its log to the local server.
4. Tests: `emu/build_game.sh` + `emu/build_rungame.sh`, `SC=<scenes> VIEW=64 emu/rungame [prof]`,
   `ALIGN=all emu/rungame`; the host tests in `host/`.

## Licence

[PolyForm Noncommercial 1.0.0](https://polyformproject.org/licenses/noncommercial/1.0.0): free for any noncommercial
use, not for sale. See `LICENSE` and `LEGAL.md`. The licence covers only this project's own code and documentation:
it grants no rights to any third party's games, data, trademarks or file formats.
