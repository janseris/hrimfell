# Legal notices

## 1. An independent project

Hrimfell is an independent, non-commercial hobby project: a 3D role-playing engine for the Nokia 9300 / 9500
Communicator. It is not affiliated with, endorsed by or connected to Bethesda Softworks, ZeniMax Media or Microsoft.

## 2. Trademarks

*The Elder Scrolls*, *Skyrim* and the names of their places, characters and items belong to Bethesda Softworks /
ZeniMax Media. Hrimfell uses them only to say what its tools read and what its world is built from - in this
repository's documentation, code comments and identifiers. It claims no ownership of them and is not presented as a
Bethesda product, port or release.

## 3. No game data here

This repository contains no data from any Bethesda game: no models, textures, sounds, terrain, placements, texts or
plugin files, and nothing generated from them. Characters' spoken lines are kept out the same way (`src/game_lines.inc`; the
repository has placeholders in `src/game_lines_stub.inc`). The files the tools generate from a user's game (see `.gitignore`)
are never committed, and builds containing them are not published.

## 4. Your own copy of the game

To have a world to play in, you need your own legally acquired copy of *The Elder Scrolls V: Skyrim Special
Edition*. The exporter (`tools/skyrim_area.py`) reads it on your own computer, and the generators turn what it writes
into Hrimfell's own formats there; the result stays on your computer and your phone.

## 5. Interoperability

The engine, renderer and tools are this project's own code, written for the Nokia 9300's hardware. The readers of
the game's file formats exist only so that the engine can work with data the user already owns.

## 6. Licence

The licence in `LICENSE` (PolyForm Noncommercial 1.0.0: no commercial use) covers only this project's own source code and documentation. It grants no rights to any
third party's games, data, trademarks or file formats.
