# Hrimfell — work plan and city map (for picking up the next task)

In-game names follow docs/NAME_MAP.md (Skyrim original ↔ Hrimfell name, for people, places and items).

Vindgard is built from the plan (v0.4.10): every position inside the walls comes from the reference map
`docs/maps/ref_whiterun_city_map.png`, the outside from the approach images. `docs/maps/gen_layout.py` turns the
plan into `src/city_plan.inc` (walls, towers, terraces, buildings, stairs, piers, water, the approach) and
`src/plan_pos.inc` (positions other files need: stalls, doors and their names, gates, stables, caravan...).
To move something: change it in gen_layout.py (map pixels), run it, rebuild. Map pixel -> city metres:
x = (px - 725) * 0.085 - 2, z = 23 - (py - 1060) * 0.085 (the Elder Ash at (-2, 23)); `host/topmap` draws the result.
Levels above the lower city (h0): the inn's rise +2 m, Wind District +4, Jorrvaskr's terrace +6, the Skyforge's rock +9,
the pool +8 (its platform +9), Highhall's terrace +18. Main gate at (-60, -12) facing west.

## Reference maps (docs/maps/)
- `ref_whiterun_numbered_key.png` - the numbered key map (1-26): which building is which; used with the city map.
- `ref_whiterun_city_map.png` - THE reference for the city's layout (the user's choice): drawn top-down map, north up,
  main gate at the left. Walls, towers, every building, the canals, the stairs (four flights up to Dragonsreach)
  are taken from it and nothing else.
- `ref_approach_topdown.png`, `ref_approach_road_marked.png` - only for the outside, which the city map does not
  show: river road -> carriage -> stables (east) / caravan (west) -> outer gate -> north along the west side over
  the drawbridge -> east along the top -> south through the inner gate -> east into the city gate.
- `ref_official_map.png` - the official online map (top-down render) of the city and its surroundings: the approach,
  the stables and farms, the river south and east, the falls out of the city's south side. Used for everything
  outside the walls. `overlay_city_on_official.png` (made by `overlay.py`) lays the city map over it: an affine fit
  on 7 landmarks (Gildergreen, Jorrvaskr, Dragonsreach, main gate, both ends of the tier wall, the south
  gatehouse), 1 official-map px = 2.65 city-map px.
- `vindgard_plan_draft.png` (made by `draw_whiterun.py`) - our plan in the reference map's pixels (outside at
  negative x), being corrected with the user before it is built.

## Districts and buildings (numbers = the numbered key map; interiors: TODO unless marked)
Lower city: 1 main gate (-60,-12); 2 Vindgard Barracks; 13 Sigrun's Anvil + forge; 3 The Antler & Arrow; 4 Aldo Varro's
House; 12 Windrest; 10 Torvald's Sundries; 8 The Bitterroot; 11 Old Gerd's House; 9 Saga's House; 5 the market (well,
5 stalls) (-4,-14); 6 The Gilded Stag on its rise, stairs up from the market; 7 East Barracks at the tier wall's east end.
Wind District: 14 the Elder Ash (-2,23); 15 Temple of the Sky-Mother; 16 Hakon's House; 17 Shrine of the Storm-King;
20 House Frostmane; 21 Hjordis's House; 22 Bjarke's House; 23 Ylva's House; 24 House Hardrede; 25 Hall of the Dead.
18 Skjoldhall on its terrace (bridge-stair from the plaza); 19 the sky-forge on its rock (stairs from the terrace).
26 Highhall: flight 1 from the plaza to the pool platform, flight 2 onto pier A, flight 3 to pier B, the causeway (pier
C) and flight 4 onto the terrace; Hrolf waits before the doors.
Outside: the approach (outer gate, drawbridge, inner gate), the stables (door enterable) and carriage east of the road,
the caravan west of it, the farm with the windmill, the meadery by the river road.
Every numbered building shows its name at its door ("Closed"); the local map (M, then Space) shows them all.

## Fortifications and water
- Outer wall and towers after the map; the tier wall with the wide stairs' gap and the small west stairs' gap; the
  walls round Highhall's terrace (open where the causeway crosses) and Jorrvaskr's terrace.
- The west canal, the channels beside the wide stairs (ending in grated drains at the stairs' foot), the pool and the
  moat round the piers. The canal spills out through the west wall into the approach and on to the pond.
- TODO: waterfalls (the water falling from the piers' arches and out of the south wall), guards, the south wall's
  gatehouses as buildings.

## Workflow for every location
1. Look it up on UESP (en.uesp.net/wiki/Skyrim:<Place>) or the Fandom wiki first (layout, rooms, NPCs, merchant
   lists, containers, unique items).
2. If those don't describe the layout well enough, ask the user: they will get a detailed description
   (layout, contents, dimensions) from Google Gemini and paste it.
3. Original names in the game (no Bethesda names); the table keeps the Skyrim original for reference.
4. Build it with the steps below, test (host shots + emulator), release, mark it DONE here.

## Surroundings of Vindgard to build (from the user's Whiterun outskirts notes)
| # | Skyrim place | What it is | Plan |
|---|---|---|---|
| S1 | Whiterun Stables | stable + carriage (Bjorlam), stablemaster | DONE (interior); carriage travel TODO |
| S2 | Caravan campsite (Ri'saad) | cat-folk caravan opposite the stables | DONE (tents, bedrolls, knapsacks, crates); tanning rack crafting TODO |
| S3 | Pelagia Farm | wheat farm just south of the stables, windmill; giant fight with the Companions | TODO (move the current west farm/windmill there) |
| S4 | Chillfurrow Farm | wheat farm further east (owner in the city, worker lives there) | TODO |
| S5 | Battle-Born Farm | wheat/vegetable farm next to Chillfurrow, grinding mill, stone quarry | TODO |
| S6 | Tundra Homestead | player house east of Battle-Born farm: garden, apiaries, outdoor smithy | TODO |
| S7 | Honningbrew Meadery | brewery + warehouse at the crossroads south of the city, by the river bridge | TODO (move the current meadery hall there) |
| S8 | White River Bridge | heavy stone bridge over the river south of the city | TODO (needs the river) |
| S9 | Western Watchtower | stone tower west on the tundra road; first dragon fight | TODO (one watchtower exists; rework + dragon event) |
| S10 | White River Watch | bandit cave on the cliffs across the river above the meadery | TODO |
| S11 | Smuggler's Den | hidden canyon camp under the cliffs NW of Highhall: fire, barrels, 4 bottles of a drug, a skill book on a table, locked chest that triggers an ambush of 3 bandits, a horse | TODO |
| S12 | Fallen tree bridge | tree trunk across a gorge farther east, a bandit on it, two dead bandits below, locked chest, bow, skill book | TODO |
| S13 | Lady of the Lake pond | misty pond NW of the stables, skeletal hand holding a sword, a rare plant on the bank | TODO |
| S14 | White River cape chest | sandbar in the river by Tundra Homestead, locked chest under driftwood, a sabre cat | TODO |
| S15 | Lucky Lorenz's house | shack crushed by a fallen tree on the east bank; body with treasure map, skill book, a bawdy book | TODO |
| S16 | Halted Stream Camp | bandit camp north, wooden palisade over an iron mine, mammoth tusks, a spell tome | TODO |
| S17 | Silent Moons Camp | bandit camp NW in tiered Nordic ruins with the Lunar Forge | TODO |

## Region (terrain work, from the user's notes)
- The city stands on an isolated glacial hill with a sheer bluff under Highhall; three terraces (done).
- Tundra plain round it: rolling low hills, rocky outcrops, yellow-brown heath (terrain colours, more rocks).
- The White River: out of a lake to the south-west, through a boulder gorge just south of the city, wider
  wetlands to the east (needs river water like the stream, a gorge, the bridge S8).
- Mountains wall the south/south-east (sheer cliffs, pine foothills, a mountain village beyond); the highest peak
  looms to the south-east (our Skarnhorn could take that role); flats west turning into rough hill country.

## How to add an interior (done once for the stables)
1. `CELL_*` id in game.h; interior origin is (100 m, 100 m), floor `IN_FLOOR`.
2. Walls: rectangles in cm (`KStablesWalls` style) and a case in `Game::BlockedAt`.
3. Shell + furniture meshes in `BuildInteriors` (`IN_*`, max 32), placement in a `Build<Name>Interior()` with `IPlace`,
   `WallTorch`, `ILight`.
4. Interactive things are props (`SpawnProps`): containers, beds, chairs, doors, items.
5. Door link: `DoorTable` (game_cells.cpp) — exterior door point (world) <-> interior point; bump `NDOORS`.
6. Travel menu entry (`Travel`), host shots (`host/int2.cpp`), then the release checks below.
7. Budget: arena (`host/arena`, keep > 150 KB free), props ≤ `MAXPROP`, containers ≤ `MAXCONT`, interior statics ≤ `MAXIOBJ`.

## Next tasks (in order)
1. Bannered Mare (inn: bar counter, fire pit, tables, rooms upstairs as beds to rent, innkeeper trading food).
2. Warmaiden's + outdoor forge working (smithing later), Sigrun trades weapons/armour.
3. Belethor's general goods (merchant buys everything), Arcadia's (alchemist; potions/ingredients).
4. Breezehome (player house, buyable), then the other homes one by one.
5. Jorrvaskr (mead hall, living quarters below), temple, hall of the dead.
6. Highhall interior (throne hall, steward Hrolf, wizard's study).
7. Camps in the wild with Nordic fur tents (bandit / hunter): small A-frame of mismatched furs (~1.5 m high,
   1.7 m wide), and a wide open lean-to; hay bedrolls or flat pelts inside, a locked boss chest beside the
   leader's tent, cooking pot over the fire, chopping block, hanging game, crates of food; bear traps in front.
8. Skyrim-style item and object records are in place (`ItemDef.form/kw`, `BaseDef`); extend with: crafting
   stations (forge, tanning rack, alchemy lab = `BO_FURN` + `FM_CRAFT` markers), stolen flags, owners.

## Release checks (every build)
- `host/mcheck` → parts dropped 0; `host/arena` → arena over 0
- `host/props_test`, `host/furn`, `host/throw` under `-fsanitize=address,undefined`
- `emu/build_game.sh`; `emu/run_game.py` (fps estimates); `emu/check_align_all.py`, `emu/check_align_ui.py` → all `{}`
- `bash /home/claude/tools/build_app.sh /home/claude/game`; `.data`/`.bss` of build/*.o must be empty
- bump `sis/hrimfell.pkg` version, `makesis` (gnupoc/tools/makesis-2.0.0/src/makesis), send the .sis

## FPS plan (agreed 2026-10-30; v0.4.49 did A1, A3, A4 in part, A5; v0.4.50 B6, B7, B10 and the pale doors)
Profile, stables yard (emu scene 9), v0.4.49: half_flat1 14%, half_gouraud1 10%, DrawFaces 8.5%, DrawFarLand 6%,
tri_flat1 6%, xform_v 5.5%, Raster 3.9%, SkySpan 3.9%, face_cull 3.4%, DrawChunk 3.3%, DrawMesh 2.9%, DrawObjects
2.7%, OccBuild 2.4%. 8 fps needs ~7.2 M instructions, 10 fps ~5.7 M (2.5 cycles an instruction estimate; the MP4
player's phone measurements say ~3, 20 ns: our estimates are ~20% optimistic).
- A2 (merging a building's pieces into one model): the user said NO.
- B6 (setting) half rows: render 100 rows (iH 100, iFy / 2, iCy16, iHH, iLenY; iCStride = stride * 2, rows on the
  even lines), then R3D::EndWorld copies each even row to the odd one below and restores full-res values. Call it
  in Game::RenderFrame at the end of the world branch, before the frozen-menu copy; draw the view model before it
  when half (it z-tests against the world's buffer) and skip the later DrawViewModel. Expect -15% (pixel work ~35%).
- B7 (setting) flat-shaded near land (LandTri: K_FLAT instead of K_GOURAUD). Expect -5..7%.
- B10 (setting) clutter only near: small outdoor kit pieces (rocks, barrels, fence bits) only within ~25 m.
- B8 view distance and B9 model detail already exist (Full models / Middle models, view distances).
- Idea from the MP4 player: adapt detail automatically to the measured frame time with hysteresis.
- User note: Riverwood's buildings are partly see-through (the water etc. behind them drawn): Skyrim's are closed.
  Find the gaps (missing back faces? pieces not in the occlusion map? far models with holes) and close them - a
  closed house hides everything behind it (OccBuild after the buildings).
- Next too: Whiterun people's real dialogue (exporter: DIAL/INFO + strings).
- (v0.4.50) The see-through houses: the doors and windows drawn pale (their texture out of the 64) - fixed. If the
  user still sees gaps, find which pieces (shot.cpp NEARLIST=1 FACEOBJ=n lists what stands round a model).
- pelt02 / pelt03 not at 256 (only pelt01): the 256 list is the signs (7) + 5 by src/mag; raise HI2MAX or prefer pelts.
- (v0.4.53) Own greetings in (Skulvar, Jervar, Severio). Still stand-ins: Nimriel, Wilmuth, Bjorlam - need the next
  export (npcs.csv voice + factions; dialogue.csv faction lines) and then gen_kit: their voice's / faction's HELO lines
  (GetIsVoiceType 426 == their voice, GetInFaction 71), with more conditions judged (time of day, weather, location).
- Generator command now uses /home/claude/whiterun_in2/skyrim_whiterun (the export of 2026-10-30 with dialogue.csv).
- (v0.4.52) host/mcheck's dropped parts fixed (0): my own v0.4.40 change (MeshInit's 512 cap) had cut the barrow's
  halls short; the antlers and the grass tufts were short of room from the start.
- Later: the real Bleak Falls Barrow interior and Skyrim's deer / elk instead of the built ones.

## Keeping track (the user's request, 2026-10-30)
Only this one chat has worked on the game: every bug found is from this work. Before the conversation gets long,
write what is being done and what is still open here (this file) and in WORLD.md, so nothing is lost when the
earlier part is summarised; when unsure of something said before, read the chat's history
(/root/.claude/projects/-home-claude/*.jsonl) rather than guess.

## Now (2026-10-30 10:30): Bleak Falls Barrow's interior from Skyrim (user's request)
v0.4.54 done (all six outskirts people speak Skyrim's lines). Next: replace the custom-built barrow halls
(game_dungeon.cpp: CELL_BFTEMPLE / BFCRYPT / BFSANCTUM, iMDun, DunData) with Skyrim's own cells, as the Whiterun
interiors were done (gen_kit kit cells: statics, floors/walls from the pieces, doors, props, lights, people). Keep
the game's barrow quest working (the claw, the puzzle doors, the word wall, Arvel, the draugr). Data: the
bleakfalls export (/home/claude/bfb_in2/skyrim_bleakfalls: refs incl. the interiors BleakFallsBarrow01..03?).
Plan (2026-10-30 10:45):
1. Kit cells from interiors_all: CELL_BFTEMPLE <- BleakFallsBarrow01 (Skyrim's "Temple": entry hall .. spider ..
   burial passages .. caverns: the old CELL_BFCRYPT's part too), CELL_BFSANCTUM <- BleakFallsBarrow02; CELL_BFCRYPT
   left empty. Limits to raise: MAXKIT (1199 used of 1300; +~450 models), MAXDEFS, MAXIOBJ (2218 of 2400; +~2800),
   AREA_KB (6.7 of 7 MB). Check memory on the phone (RAM shown with Show FPS).
2. Doors: outside main door <-> Temple's NorLargeDoor; Temple NorDoorMediumLoad01 <-> Sanctum's; Sanctum's
   CaveGSecretDoor01 -> outside (the hidden exit). (bfb export doors.csv)
3. Mechanics from Skyrim's refs, same code: a generator writes the DunData tables (props: lever NorLever01,
   pillars NorDefaultPuzzlePillar01 x3, gates NorPortcullis / NorPortcullisLarge01, pull chains, claw door
   PuzzleDoorBase01 + wheels, word wall NorWordWallWordFus, sarcophagus; chests TreasDraugrChest..; ents: bandits
   MS13BanditCampfire01-03, MS13Arvel, MS13FrostbiteSpider, draugr, skeevers; fix: blades TrapBladeSwinging01,
   plate TrapStonePressurePlate02, oil TrapOilPool01, lanterns TrapFallingOilLamp01, darts, spike wall TrapWallWood)
   in the kit frame; DunFloor/DunBlocked use the kit's floors/walls + the gates' and claw door's spans.
   Code that tests CELL_BFCRYPT (spider, Arvel) -> CELL_BFTEMPLE.
4. Old custom geometry (BuildDungeons tiles, PlaceDecor) off when the kit cells are there.
5. Tests: walk the quest (host/bfbwalk.cpp to redo), shots, fps, phone memory.
Progress (11:30): step 1-2 done in code: AREA=bfb gen_kit run -> src/area_bfb/ -> data/bleakfalls.har (565 models,
5.7 MB; collision: dungeon pieces in 'arch' with coarse_coll, NoCol/Vine/Rubble/RockPileS left out). Game: UseArea(0/1)
in EnterCell (marks iAreaDefs/iAreaArena/iAreaIObj taken before LoadArea in BuildInteriors; old area's kit lights
removed); iBfbKit when bleakfalls.har is there: no BuildDungeons, no old props/ents (SpawnDungeons), DunBlocked 0
for kit cells, FloorAt -> kit. GameBarrowDoors(d, kit): Skyrim's door positions. MAXIOBJ 3600, ARENA_KB 4626 (-450),
MAXLSRC 240. Next: step 3 (mechanics tables from Skyrim refs).
Generator commands now:  main: python3 docs/maps/gen_kit.py <6 dirs incl. whiterun_in3>; python3 tools/kit_pack.py
  barrow: AREA=bfb python3 docs/maps/gen_kit.py <same>; python3 tools/kit_pack.py src/area_bfb/kit_ext.inc data/bleakfalls.har
  (sis must also carry data/bleakfalls.har)
Progress (12:30): step 3 code done (src/bfb_kit.inc from docs/maps/gen_bfb_kit.py; DunOfK; KitAcross gates/claw door;
spider/Arvel in TEMPLE; word wall; travel 17-19; BF_KITSPAWN: SpawnKit for the barrow on first UseArea(1); NewGame
UseArea(0); LoadDone UseArea by cell). PROBLEM: collision from the simplified pieces is unusable (host/bfbkpath: no
way from the doors; the search climbs rock slopes to +47 m). FIX under way: Skyrim's navmeshes - tools/skyrim_area.py
now writes navmesh.txt.gz (NAVM/NVNM: verts, tris with edges/flags; interiors + city worldspace), gen_kit
nav_collision() uses them for AREA != main (floors = navmesh triangles, walls = edges with no neighbour and no link
flag, -20..+200 units). Asked the user for: python skyrim_area.py bleakfalls (new exporter). Then: rebuild bfb area,
host/bfbkpath (paths: door->door, web->plate, sanctum door->exit), Arvel path -> gen_bfb_kit.py arvel.txt.
Progress (2026-10-30 afternoon): navmesh collision works (bfbkpath: all three paths). Emulator scenes 17-20 moved to
Skyrim's barrow (emu/run_game.py also maps data/bleakfalls.har; EmuDataRead 'b'). NOTE the emulator's default view
(VIEW unset = 0) draws textures OFF: use VIEW=64 to see them (Textures 50 m).
Barrow fps was 2.3-2.6 (15k triangles: whole halls behind the walls drawn). FIX: what can be seen from where -
host/pvs.cpp (cube of six 128 pictures from every 64-unit spot of the navmesh, standing and crouched; regions of 256
units x feet bands of 160, each with its neighbours') appended to the .har (HPVS + trailer "PVS1"; LoadArea ->
KitCell.pvs; BuildKit records iKitObj0; GameKitPvs per frame -> R3D::iPvsBits; DrawInterior skips the hidden). Off
the regions: everything drawn. RERUN after every kit_pack of the area:  ./host/pvs 1 data/bleakfalls.har  (8.5 min).
Result: entry 2.3 -> 7.8 fps, spider 5.4 -> 8.3, word wall 5.2 -> 6.2.
gen_kit: inside, plants' card pictures (dead shrubs, roots) drawn cut out (were dark flat sheets); FURN *Ambush*
(creature-exit furniture: FrostSpiderAmbush01 was a big brown sheet) skipped.
host/bfbquest: the quest end to end (talk, lever man, pillars+lever -> gate, spider, Arvel's run, claw, chains, claw
door, word wall, Overlord, Dragonstone, way out) - all pass (word wall: kit trigger's middle is ~2 m over the dais).

## Analysis (2026-10-30 14:35): Skyrim's Whiterun as an open city (user asked; not started)
Data (whiterun_in3): WhiterunWorld is in Tamriel's own coordinates (main gate door Tamriel 19312,-7424 -> inside
19516,-7428): the city core x 20037..29682, y -9499..1745 (about 140 x 160 m). 1804 refs (865 STAT, 170 MSTT, 147
TREE, 75 NPC_, 44 DOOR, 33 CONT), 263 distinct models, ~900k raw triangles (Temple of Kynareth 36.5k, Jorrvaskr
24k, Skyforge 23k, Dragonsreach 17k). Land: 7 cells of its own (land_WhiterunWorld.bin) - to bake into
hrimfell.land inside the walls. 27 Whiterun interiors in the export. NO navmesh in whiterun_in3 (the exporter
writes it now: needs a fresh "skyrim_area.py whiterun").
Limits: main area is full (riverwood.har 6.7 of 7 MB, MAXKIT 1199/1300, the 64 textures already crowded) -> the city
can not join it. Plan: areas by region outside too: whiterun.har = the city + the outskirts (moved out of main) +
the Whiterun interiors; UseArea switched outdoors at a line between Riverwood and Whiterun (with hysteresis; a
1-2 s hitch reading the file - to measure on the phone), each area its own 64 textures. Vindgard (game_city.cpp)
and its built interiors replaced; the main gate open (walk through). Collision from the navmesh; host/pvs for the
city's points (walls and houses hide most); far/mid models for the plain's view of it. People: 75 with schedules
(packages.csv) and dialogue - after the buildings.
User (14:36): barrows and cities share many models and textures; items are used everywhere; barrows share models
with other barrows. Measured: main vs barrow today share 19 models (23 KB) and 13 of the 64 textures - and those 13
are mostly the land's plant cards, which the barrow file should not carry at all (gen_kit numbers the flora's cards
for AREA bfb too: wasted slots - fix). items.hit (5 MB) is already one file for every area.
Plan: a shared library file (common models + textures used by 2+ areas, and whole kits: Nordic ruins, caves, common
clutter, Whiterun's pieces), loaded once and kept; area files only their own pieces and refer to the library's by
number. One gen_kit run over all areas decides what goes where. Textures: two banks (library + area); the slot
byte's 6-bit number needs a second bank flag (format change in r3d DrawFaces). Area switching reads less (shorter
pause). gen_kit now writes kit_tex_names.txt (the 64, in order).
Export skyrim_whiterun_3.zip (2026-10-30 14:42) -> /home/claude/whiterun_in4/skyrim_whiterun: errors none; 85060 refs
(Tamriel 31972 with margin 8: cells x -8..18, y -13..10; WhiterunWorld 1804; 74 interiors 51284 - all Whiterun's 27,
Riverwood's 6, the farms, and dungeons: Bleak Falls 01/02, Dustman's Cairn, Korvanjund, Volunruud, Labyrinthian01,
Fellglow, Irkngthand, Ilinalta's Deep, Shimmermist, Fort Greymoor, White River Watch, Brittleshin, ...;
E3demoBleakFallsBarrow is a test cell - skip). navmesh.txt.gz: all 74 interiors (45221 tris) + WhiterunWorld (10
meshes, 4705 tris); none for Tamriel. 3914 meshes (1 bad: StockadeBarricade01Activator), 2772 textures; npcs 527,
packages 472, dialogue 9228. City: 37 doors into interiors; WhiterunWorld land 7 cells (same as before).
Sharing (distinct models by group): 3894 overall, 1276 in 2+ groups; barrows 1054 (654 shared), city 340 (231),
Whiterun interiors 1082 (451), caves/forts 1654 (946), Tamriel 1531 (817); 582 of 1124 barrow models in 2+ barrows.
-> the shared library is worth it (step 1), then Whiterun's area.
Progress (2026-10-30 15:55): SHARED LIBRARY DONE (v0.4.56).
 - gen_kit.py LIB=1: every sky_ok texture numbered (slots 0x1000+n), pictures to kit_texpics.pkl (64 all, 128 where
   Skyrim has it, 256 the 48 finest); navmesh areas keep their models' boxes (the game skips them: GameKitNav).
 - tools/lib_pack.py <inc har>...: models identical in 2+ areas (content hash, textures by name, far/mid included)
   -> data/common.hlb (numbers 0..NL-1); each area's own NL..; textures: library bank (shared/on library models,
   64 max: look byte 0x40+n) + area bank (0xC0+n); flora's picture byte rewritten to the look byte; HIM1, harvest
   props, lods remapped; the old file's PVS carried over. Caps: LIB_HI 32 / LIB_HI2 8, AREA_HI 32 / AREA_HI2 8.
 - Game: LoadArea(name, aLib) (common.hlb first, its meshes built once before the area marks: BuildKitMesh),
   KitM(i), LIB_KB 2048; R3D: iLib* bank + iLibShade (ShadeOf); DrawFaces picks the bank per slot.
 - Now: common.hlb 18 models, 31 textures (1.1 MB); riverwood.har 1181 own; bleakfalls.har 507 own. Riverwood has 95
   textures to draw from (was 64): the Sleeping Giant's floor textured; textured 6.9 -> 6.5 fps there (more
   texture), untextured unchanged. Emulator: run_game.py maps common.hlb ('c') and whiterun.har ('w').
 Commands now: LIB=1 python3 docs/maps/gen_kit.py <6 dirs>; LIB=1 AREA=bfb ... ; then
   python3 tools/lib_pack.py src/kit_ext.inc data/riverwood.har src/area_bfb/kit_ext.inc data/bleakfalls.har
   (kit_pack.py is the old single-area way). PVS: kept by lib_pack; rerun host/pvs only when a barrow's pieces change.
NEXT: Whiterun open city (area 'wr': data/whiterun.har) - plan above (14:35 analysis).

## Whiterun open city - the plan in steps (2026-10-30 16:00; user: "start with the shared library and then the city")
Data: /home/claude/whiterun_in4/skyrim_whiterun (export 14:42, with the city navmesh). Area 'wr' -> data/whiterun.har.
A. Generator AREA=wr (gen_kit): an outside kit cell "WhiterunCity" from the WhiterunWorld refs (everything inside the
   walls: houses, walls, Dragonsreach, Gildergreen, the Skyforge, stalls, clutter, lights, trees), origin the city's
   middle (WR_OX, WR_OY = 23678, -6609 already used for the outskirts); the outskirts cell moved here from main (it is
   in sight of the walls); the farms' and the stables' interiors too. Its pieces' boxes kept (lib-identical).
B. Land: the city's own land (land_WhiterunWorld.bin, 7 cells, Tamriel coordinates) baked into hrimfell.land inside
   the walls (host/bakeland or the land tool); Vindgard's BuildCityTerrain/BuildCity/its props and people off when
   whiterun.har is there (as iBfbKit did for the barrow).
C. Walking in the city: its navmesh (navmesh.txt.gz, WhiterunWorld 10 meshes) as the outside cell's floors and walls
   (nav_collision for an outside cell, origin-relative); GameKitOutFloor/OutBlocked use them where they are.
D. Area switching outside: UseArea(2) when the player is east of a line between Riverwood and the outskirts (with
   a margin each way, so it does not flip); main keeps Riverwood and Bleak Falls outside. Doors table: the city's
   37 doors - first to the game's existing interiors where they match (Dragonsreach -> CELL_HIGHHALL (Farengar), the
   Bannered Mare -> CELL_INN, Belethor's -> CELL_SHOP, Arcadia's -> apothecary, the Hall of the Dead ->
   CELL_HALLDEAD), the rest shut ("locked") until step F; the main gate open (no door: walk in).
E. host/pvs for the city's points (outside: regions from the navmesh), far/mid models from the plain; fps scenes.
F. Skyrim's Whiterun interiors as kit cells (new CELL ids 19..: 27 of them), the quest people moved into them.
G. The city's people (75) with their schedules (packages.csv) and Skyrim's lines (dialogue.csv).
Runtime notes for D (16:10): outdoor statics are a flat list R3D::iObj (AddStatic), linked into chunks at the end of
FinishWorld (iChunkObj/next). Kit outside cells are placed by PlaceKitOutside (BuildRiverwood) -> for switching:
take iOutObjMark = iObjCount before PlaceKitOutside; on UseArea between 0 and 2: iObjCount = mark, restore the area
marks, LoadArea, BuildKit, PlaceKitOutside, relink the chunks (new R3D::RelinkStatics: the link loop of FinishWorld).
Vindgard (BuildCity, BuildCityTerrain, city props/people) skipped when whiterun.har is there (iWrKit). Switch line:
Riverwood's north edge is ez 1965 m, the outskirts' south edge ~2222 m (city middle 3674 E, 2422 N in the game's
metres) -> main below ez 2080, wr above ez 2120 (else keep the current one). The land file's grown trees were baked
with Vindgard's clearings: re-bake after the city (bakeland) so none grow in Skyrim's streets.
Progress (2026-10-30 16:30) city step A-D in code (v0.4.57 under way):
 - gen_kit AREA=wr (LIB=1, 6th dir whiterun_in4): CELLS = stables + 3 farms; outskirts cell; "WhiterunCity" outside
   cell from WhiterunWorld refs (WRC_SKIP/WRC_BIG; terrain pieces WRTer*, WRMainRoad*, *Base/*Platform are big);
   collision from the WhiterunWorld navmesh (NAVS keyed by world for world navmeshes; nav_collision(name, origin)).
   Main with WRAREA=1: without the Whiterun cells. Flora written for main and wr.
 - lib_pack: banks ranked by worth = use share + world-area share (pickle 'wa'): the ground pieces' textures
   (WRBaseDirt01, WRFieldGrass01) were flat brown/green.
 - tools/patch_land.py: data/hrimfell_v1.land <- hrimfell_v1_tamriel.land + land_WhiterunWorld.bin (7 cells) and
   Tamriel's land kept 0.5 m under the city's navmesh elsewhere; then host/bakeland (build line in its header).
 - Game: iWrKit (whiterun.har there): no BuildCityTerrain/BuildCity (Vindgard); AreaOf(cell, x, z); AreaOutside
   (ez > 2120 m -> 2, < 2080 -> 0); Tick switches outside; UseArea re-places outside statics (iOutObjMark from
   BuildRiverwood, iOutArea; R3D::RelinkStatics); BF_WRSPAWN (SpawnKit on first load of area 2); NewGame:
   UseArea(2) + SpawnWhiterun; doors 1, 3, 4, 5, 6 (Hall of the Dead, Arcadia's, Bannered Mare x2, Dragonsreach)
   moved to Skyrim's door positions (KWrOut), still the game's own rooms behind them.
 - host/shot: UP=n env for "sky" (stand on what is built). City shots: /tmp/.../scratchpad/city*.png.
 TODO next: check shots after the re-rank; emulator scenes 5/6 (city market/approach) -> Skyrim coords; fps; PVS
 for the city (outside regions); merchants/props/people of Vindgard (SpawnMerchants, SpawnProps) - placed at
 Vindgard positions still: move or hide; sis 0.4.57 with whiterun.har.
Progress (16:50): v0.4.57 delivered - Skyrim's Whiterun city drawn and walkable (first pass). Textures: ground
pieces' grass (WRFieldGrass01: mostly opaque, foliage-named) now numbered (foliage textures on non-tree pieces ->
CARD_TEX_L, any alpha). MAXDEFS 1800 (1354 used), ARENA_KB 4882. host/areaswitch (walk between areas, stables, the
barrow; its last check expects the barrow from the stables - test order wrong, not the game), /tmp defs probe.
fps (emulator, textures 50 m): city market 4.4, approach 5.1, wind district 6.8, stables yard 5.9 (untextured 5.5 /
5.7 / 9.7 / 6.7). NEXT for the city: speed (outside "what can be seen" for the city: walls hide most - R3D static
drawing needs a hook like iPvsBits for iObj ranges; far/mid distances), the townsfolk and smith (still Vindgard's
spots), Skyrim's interiors (step F), people (G), the bake's trees in the city (check), the map image (Vindgard drawn?).
Self-update (user, 17:01): menu "Update from server" -> http://192.168.137.1:8000/skyrim.sis (port 8000: the user corrected 800; the
wrote; the draft said 8000). Release step from now on: copy sis/hrimfell.sis to the server folder as skyrim.sis.
v0.4.61 (17:50): barrow pieces finer (gen_kit dungeon_models -> outside_models + mid_of; DrawInterior mid model);
host/pvs rerun (it must be rerun whenever the barrow's pieces change: lib_pack only carries the old one over).
host/shot NOPVS=1 env: draw without the visibility table. Word wall chamber the slowest (4.0 textured).

## Skyrim's own draugr (user 18:10: "i dont want the custom gray guy"; 18:13 "yes" to the plan)
- tools/skyrim_actors.py (new; needs skyrim_meshes.py beside it): actors\draugr, armor\draugr, weapons\draugr,
  skeleton, skeever, frostbitespider, troll, canine: .obj (rest pose) + .skin.txt (bones, NiSkinData bind
  transforms, partitions, per-vertex 4 bones + weights) + skeletons\*.txt + records.csv (RACE/ARMO/ARMA/OTFT/LVLN/
  NPC_/HDPT/WEAP). Tested on a skinned SSE banner only. Sent to the user to run -> skyrim_actors.zip.
- Then: generator poses Skyrim's draugr body (+head, armour) into the game's 25 frames (0-15 walk, 16-23 attack,
  24 dead) by linear blend skinning with the game's own joint angles; bone numbers in 'w' lines: global or the
  partition's - decide by which puts each vertex nearer its bone. A creatures section in common.hlb.
- Meanwhile: the temple sarcophagus lid as a moving part (like the Overlord's: fix kind 8), and a break-out motion
  for the alcove sleepers.
v0.4.65 (Oct 31): outside PVS for Whiterun (host/pvs: outside job after the interior cells, run `./host/pvs 2
data/whiterun.har` ~25 min; rerun whenever the city's statics change - nobj must match iOutObjN); host/outpvs
compares pictures with and without it at random street points; host/wrfolk lists the city's people by the hour.
gen_kit.py main run writes src/wr_folk.inc (WR_FOLK: named 29..52) and adds them to SAY_WHO (KKitSayIx now u16).
Left for the city: evenings (the Bannered Mare's packages: inside - the game's inn), guards (posts and patrols:
GuardWhiterun* packages), Skyrim's interiors (step F), the map image.
User (12:08): MAXE 160 is not the answer - the game is meant to take more places (Riften, ...). PLAN (next, v0.4.66):
people and creatures streamed by place like the land's animals already are: live slots (Ent e[], kept ~100) only
for what is loaded now (the area's outside near the player + the interior cell); everything else off-stage as a
compact per-place roster in the save (per placed actor from the data: alive, looted, a few state bits; the named
people with schedules: their row/cell), spawned from the place's tables + roster on load, folded back on unload.
Then the same for props/containers (MAXPROP/MAXCONT: only changed ones kept, Skyrim's respawn timer like flora).
Save size then grows with places by bits per actor, not by slots. 0.4.65 ships with MAXE 160 as a stopgap.
v0.4.66 (13:13, user 12:44 "continue with adding the autumn forests around riften"): done as in WORLD.md. Rebuild
order: patch_land.py <WhiterunWorld.bin> <riften export folder>, bakeland (build line: exclude src/*_app.cpp), gen_kit
main/bfb/wr, lib_pack (PVS carried over). Open: the rest of the Rift (only the export's 20 x 21 cells have Skyrim's
placements - wider export from the user would cover it), tree collision (none, as the grown trees), denser crowns
(leaf cards simplified 1555 -> 110). Next after this: people/creatures streamed per place (v0.4.65 note).
v0.4.67 (13:52): streaming by place done (see WORLD.md); phone build lesson: a new source must go into build.cfg
SRCS (the phone link failed silently - check build/hrimfell.app's time), emu/build_game.sh and hb.sh. User 13:22:
"7 fps?" - forest made cheaper (far form 8 m, plainer); next possible: grass tufts and far land cost, props/containers
by place (MAXPROP/MAXCONT), Riften itself.

## Forelhost (user 14:02 "add forelhost")
Data: riften export (riften_in3) has Forelhost's exterior (Tamriel cells ~44,-28; ruin Nor pieces x 180955..187225,
y -116752..-108756, z 16369..18711) + kits interiors_all.csv.gz has Forelhost01 (Stronghold), 02 (Crypt), 03
(Refectory) refs with bounds; meshes in riften_in3. No navmesh/npcs/dialogue for it: asked the user (14:10) to run the
updated tools/skyrim_area.py forelhost (area added: cells 42..46, -30..-26) -> skyrim_forelhost.zip.
Doors (doors.csv): Tamriel NorDoorSmLoad02 (182929.9,-113715.4,17862.3 rot 229.9) <-> Forelhost01 (513.4,-1401,0 rot 90;
out at 513.4,-1337); 01 NorDoorMediumLoad01 (-1152,3328,-256) <-> 02 (-1024,3072,-512; arrive -1028.3,3254.4);
02 NorDoorSmLoad02 (2944,7552,-256) <-> 03 (2688,8192,256); 03 NorLargeDoor (1280,-1072,2176) <-> Tamriel NorLargeDoor
(181590.2,-112079.2,18662.3 rot 139.9; arrive 181655.7,-112152.9,18726.3) - the top, where the word wall is.
Word wall outside: NorWordWallWord Strun/Bah/Qo at (182066.6,-115940.7,18620.5): Storm Call, first word STRUN.
Glass Claw (dunForelhostGlassClaw) in 03 at (1543.8,9151.9,1252.3) on a pedestal; claw door (PuzzleDoorBase01 +
KeyHole01Glass) in 03; combination Fox, Owl, Snake (skyrimcodex.com dragon-claws, top to bottom; Golden is listed
Bear, Moth, Owl the same way = outer..inner). Rahgot: DunForelhostDragonPriestRahgot in 03 (+ boss chest).
Ghosts dunForelhostLvlGhost* (01, 02), draugr (02 most), frostbite spider + skeevers (02), levers NorLever01 (01, 03),
NorPortcullis x3.
Plan v0.4.68: AREA=fh in gen_kit (CELL_FHSTRONG/CRYPT/REFECT = 19/20/21) -> data/forelhost.har (area 3); dungeon
generator generalised (gen_dun_kit.py: chests, draugr ambush, ghosts, Rahgot, levers->gates, claw door, claw item);
ForelhostOutside in the main area (like BleakFallsOutside) + word wall prop; door table; staging groups; save v17
(iS.dun[] generic dungeon state).
v0.4.68 progress (14:40): all of the above done and host/fhquest passes; PVS for forelhost.har computing
(./host/pvs 3 data/forelhost.har, ~1 h); then emulator scenes 34-36, phone build, sis. Rebuild everything:
scratchpad/regen.sh (gen_kit main/bfb/wr/fh in parallel + lib_pack), then gen_dun_kit.py for src/fh_kit.inc:
  python3 docs/maps/gen_dun_kit.py /home/claude/fh_in/skyrim_forelhost FH src/fh_kit.inc Forelhost01=FH1 Forelhost02=FH2 Forelhost03=FH3
Open for Forelhost: Siege on the Dragon Cult (Valdr? no - the quest giver is Valdar? check dialogue.csv), the traps
(spears, oil, darts, flamethrowers drawn but still), Storm Call's shout, the spear gate when the claw is taken.
v0.4.68 delivered 14:56 (Forelhost; fps stronghold 6.1, crypt 9.5, refectory 11.3; MAXIOBJ 5600 - the Refectory's
pieces had not fitted, host/pvs CELL=n now keeps the other cells' records). User 14:49: the quest starts at Forelhost
itself (Captain Valmir in the courtyard: ValmirSons/ValmirImperial refs 00090F06/0007E5B7 at 184084,-113775 - a
Thalmor; UESP stages 10 journal (Skorm Snow-Strider's), 20, 26/40 well key, 50 Rahgot, 60 back to Valmir, 70 he is an
impostor on the battlements). User 14:52: extend the exporter -> tools/skyrim_area.py now reads QUST (stages + log
CNAM in DLSTRINGS, objectives, aliases ALID/ALFR/ALUA), DIAL QNAM; writes quests.csv for the area's quests (an alias
forced onto a placed ref or one of its people) and every line of their topics into dialogue.csv (speaker
"quest:<edid>" when no person is named). Sent to the user to rerun: python skyrim_area.py forelhost.
Forelhost quest data (user's export 15:07, /home/claude/fh_in2/skyrim_forelhost: quests.csv + dialogue speaker
"quest:dunForelhostQST"; 566:TravelMarker = GetIsAliasRef alias 2 Grifter (Valmir), 566:MapMarker = alias 16
EndSoldierActual; 58:#00035354 = GetStage(dunForelhostQST); 629:#00035354==1 the Imperial variant): stages 10 (log:
asked to defeat the priest, journal), 20 journal found, 40 the well, 50, 60 mask taken -> return to Valmir, 70 kill
the impostor, 200. Refs: ValmirImperial 0007E5B7 (184168.7,-113795.9,17934.5) / ValmirSons 00090F06 (184084,-113775);
soldiers 00062243 Imp (184098.9,-114044.1) 00062244 Sons (183959.5,-113945.5) disabled until the end; journal
0007E5B9 BOOK in Forelhost01 (1785.5,1795.3,447.6) "Skorm Snow-Strider's Journal"; CryptChest 0003A497 (Forelhost02
2656,1056,-1440) holds the well key; Well marker 00082E1C (02: 240,4752,-784) -> RefectoryMarker 00082E1A (03:
2446.1,8065.1,239.9). No hand-in lines at 60: Valmir moves to the battlements; the end scene lines are SCEN rows.
User 15:07/15:08: "at 4:21 PM continue and work on framerates of whiterun - try more ARM assembly, do your best"
(scheduled a reminder into this session for 16:21 Prague).
v0.4.69 delivered 15:15 (the quest). NEXT (16:21 reminder): Whiterun frame rates, ARM assembly.
v0.4.70 delivered 18:00 (frame rates). Ideas left: packed u/v in half_tex1 (2 of 15 instructions a pixel), the far land's ring transforms inside the city (4% of the market's frame), DrawFaces' per-face C (370 instructions a face).
User 18:55: "ok try improve this" (the three costs) "and fix these from Forelhost" (soldier fights, staff given, the key's
real name, Skyrim's stage numbers). v0.4.71 delivered 20:10: all but the key's name (exporter sent: QUST ALCO ->
quests.csv created/created_name, and books.csv; waiting for the user's re-export of forelhost - then take the WellKey
alias's created_name into game.cpp's IT_FH_WELLKEY name). Speed done: far land rows trimmed, RasterTex, persp_tex* ARM
rows, SlotInfo table, hidden pixels shorter; all scenes identical to the pixel. Left: DrawFaces per face (~11%: spills -
needs ARM or splitting), RasterTex's per-triangle arithmetic (~500 instructions a triangle in the stables: ARM set-up
would halve it), PointLight per face in lit interiors (7% in the stables). host/props_test and host/furn look up props
that are not there (index -1) since the game starts at Forelhost (v0.4.69) - the tests need their place set.
User 20:23 sent skyrim_forelhost_2.zip (-> /home/claude/fh_in3): the key's name is Skyrim's "Forelhost Well Key" (kept).
v0.4.72 delivered 20:45: Forelhost's books and notes with Skyrim's texts (gen_fh_books.py). The exporter now also writes
books' value/weight (not yet re-exported - the item values for the notes/books are from memory, check on the next
export). host/booktest, props_test, furn need a place set (they assume the Riverwood start).
User 21:01 (skyrim_forelhost_3.zip -> /home/claude/fh_in4: books.csv with value/weight - applied): "continue with
performance" (DrawFaces per face, triangle set-up, torch light). v0.4.73: point_light, tex_tri, face_shade, persp_rows
in ARM. Left (stables): half_tex1 ~27% (pixels: ~15 a visible pixel, 7 a hidden one), tex_tri ~356 a triangle,
face_shade ~260 a face (its work, not spills), xform_v. Ideas: fewer pixels (overdraw ~2.5), packed u/v.
User Oct 31 23:10 / Nov 5 17:21-17:28 (v0.4.74): Whiterun's broken floor, Riften unreachable (map: "T" to go where the
map points), stuck at Forelhost's door and around Whiterun, Forelhost's doors, collision too strict, a loading screen;
light-blue wooden walls in the new houses outside Whiterun; far-land steps 50-400 m and simpler far houses (the city
halves the frame rate on the plains); review janseris/symbian-s80-build-tools and symbian-s80v2-dev-tips-and-tricks.
v0.4.74 notes: tools/lib_pack.py carries the old "what can be seen from where" (HPVS) over to the area file it
replaces - after gen_kit changes the pieces' order those tables hide the wrong pieces (the barrow's rocks went
missing, Forelhost's Stronghold drew 13k triangles): ALWAYS run scratchpad/pvsall.sh after regen.sh (host/pvs for all
four areas, two lanes; Forelhost ~1 h). The packer now also renumbers the ruin doors' models (PT_DOOR item & 2047)
into the library's numbering, like the harvest props' (they had drawn as barrels and a road sign).
Repo review (janseris, Nov 5) - what applies here, see WORLD.md v0.4.74: per-triangle cost (~1,500 cycles on the
phone with small triangles) is the thing to cut (-> far groups, boxes); a miss ~40 cycles; MUL Rd != Rm
(tools/armv4_check.py); measure 32 frames (15.6 ms tick). Not taken yet: front-to-back with a coverage bit a pixel
(Maps 9300 1.59: 5,500 of 8,000 triangles wholly hidden), sort faces by chunk at load, build a triangle's record in
the loop that transforms it (no list written in one pass and read in the next).
v0.4.74 doors and "what can be seen": host/pvs counts the ruins' hinged doors shut (their triangles hide, they are no
pieces); GameKitPvs adds, for each open ruin door within 40 m, the regions 1.5 m out either side of it (R3D::iPvsMix) -
the Stronghold had drawn the next halls through its doors (12.1 M instructions against 7.7). Run scratchpad/pvsruins.sh
(Forelhost and the barrow) after any change to the ruins' pieces or doors.
User 19:30/19:31: exits outside towns say "Skyrim" in Skyrim; some outdoor places have their own names (large dungeons
with outside parts). Skyrim's rule: a load door out shows the destination exterior cell's name if it has one, else its
worldspace's (worldspaces.csv: Tamriel "Skyrim", WhiterunWorld "Whiterun", Blackreach, Shalidor's Maze, ...). Done:
Whiterun's street doors "Whiterun", every other way out "Skyrim". tools/skyrim_area.py now writes ext_cells.csv (the
named exterior cells) - sent to the user; on the re-export, take any named cells our doors open onto (Riverwood?).
User 19:40 sent skyrim_riverwood.zip (-> /home/claude/rw_in5) with the new exporter: ext_cells.csv is empty - of 8,441
exterior cells read (with XCLC and regions) none has a name in Skyrim.esm, so a way out is labelled by its worldspace
alone (Tamriel "Skyrim"); all seven Riverwood doors lead into Tamriel -> "Skyrim" (done). The export also has
Riverwood's 17 books with Skyrim's texts, values, weights and places (Faendal's 5, the Trader's 4 Songs of the Return,
the inn's 4, Gerdur's 2, Alvor's A Gentleman's Guide to Whiterun) - in the game they are still unreadable clutter
models (kit_ext); gen_fh_books.py's way would make them readable. Offered, not done.
User 19:43-19:49: tests in faster languages? Agreed next (after v0.4.74): host/pvs multithreaded, recompute single halls
(CELL=n) instead of whole areas, a C emulator driver (libunicorn.a in /home/claude/unicorn/build) in place of
run_game.py's Python block hook (and the cache model's read hook) - same outputs, so the references stay comparable.
v0.4.74 found late: gen_kit's vertex-colour tint (added this version) had made the caves' rocks near black (scenes 17,
35, 36): now hue only. Regen without pvs is fine when the pieces' order is unchanged (texture-only changes).
v0.4.74 delivered 20:25 (skyrim.sis). host/pvs now multithreaded (THREADS=n, default all cores; build with -pthread):
each point's view in its own thread, joined in the points' order - identical tables (riverwood.har byte for byte), 2
cores 42 s -> 22 s. Use CELL=n for a single hall. NEXT: the C emulator driver.
v0.4.75 tools (Nov 5 20:40): emu/rungame (emu/rungame.c, build emu/build_rungame.sh) - run_game.py in C: the same text,
pictures, profile (prof, PROFFN) and cache model (CACHE, CACHEMAP) to the byte, checked on scenes 0, 8, 34; 3 scenes
174 s -> 4 s, prof 150 -> 2 s, CACHE 97 -> 2 s. ALIGN=all (every scene 0-40 now, was 27 of them) 36 s, ALIGN=ui 5 s
(were ~25 min together), ALIGN=selftest proves the hook catches a misaligned load. /tmp/claude-0/cmp.sh uses it.
run_game.py / check_align_*.py kept as the reference. host/pvs multithreaded (above).
User 22:25 "work on the optimizations you found". v0.4.75 (22:40): interiors drawn nearest first in each pass
(DrawInterior sorts like DrawObjects; DrawIStatic) with the occlusion map made among the pieces; those mid-pass builds
judged frame by frame (OccMidBuild / OccMidOk: kept only while what they leave out repays ~0.2 M each, tried again every
8th frame) - Stronghold 9.74 -> 8.13 M, city at 400 m 6.77 -> 6.38, stables/inn/barrow as before; pictures the same but
for 1-33 pixels of coplanar ties in 10 of 41 scenes. FPS shown: whole frames over a second (the 64 Hz clock).
Tried and dropped (measured): an occlusion map kept up to date per triangle - dirty blocks made again on demand
(3,500 block rebuilds a frame: +8 %), or blocks wholly inside big triangles raised (few covered, +12 %).
Still hidden after set-up: market 53 %, city 48 %, Stronghold 37 % - the next step would be a coverage test before
the triangle set-up (Maps 9300's bit a pixel, front to back), a rasteriser change. The other session's (21:46) edits
kept: emu_buf / BUFLIST, emu_occ / OCC in rungame (game_glue's MAXOBJ fixed to 4600).
User 22:45-23:02: commit and push (no Skyrim assets) to a new repository - git initialised in /home/claude/game
(.gitignore keeps out data/, every generated-from-Skyrim source, the reference maps and screenshots, builds); README.md,
LEGAL.md (modelled on Mudcrab-Team/mudcrab's LEGAL.md: independent, trademarks only descriptive, no assets, own copy,
interoperability, licence own code only). Waiting: the repository's owner/name (no GitHub linked here yet) and the
licence. Planned (user's idea): an in-game menu choosing places, the PC's local server building them from the user's
own Skyrim (exporter + generators + lib_pack + pvs) and the phone downloading the area files (as the DLL updates do) -
the .sis then ships no Skyrim data; the base files (library, items, land) the first download. Then the coverage-bit
rasteriser.
Nov 5 23:25: pushed to https://github.com/janseris/hrimfell (private; the Claude GitHub app on that one repository) -
one commit 45448ed: the engine, tools, docs, LICENSE (PolyForm Noncommercial 1.0.0, Required Notice: janseris), LEGAL.md,
README.md; no game data (.gitignore), the people's lines in src/game_lines.inc (ignored; game_lines_stub.inc tracked).
Work in /home/claude/game, push from there (origin = janseris/hrimfell, branch main). NEXT (user 23:28 "do it"): the
coverage rework - reject wholly hidden triangles before their set-up (market 53 %, city 48 %, Stronghold 37 % of the
triangles set up show nothing).
Nov 5 23:45 - the hidden triangles, measured (host/scenestats, R3D_STATS: set up before the occlusion map exists /
after): market 899 / 623 of 1522, city at 250 m 896 / 805, Stronghold 47 / 594, stables 618 / 182. Tried: an exact
depth test of a small triangle's box before its set-up (ZHiddenTri: the middle pixel first, boxes up to 64-1024 px) -
it left out 900 of the market's 2847 triangles but cost 353 instructions a call in C against a hidden triangle's set-up
of ~240-370 (they are small, mostly flat, the set-up already ARM): every scene slower (+2-4 %), the cache model no
better. Removed. Why Maps 9300 gained and we do not: it has no depth buffer - a bit a pixel, 32 pixels a word compare,
triangles strictly nearest first (bucketed) - so its test is ~16x cheaper and its spans skip hidden runs whole. The real
prize here would be the hidden PIXELS (overdraw ~2.5, ~7 instructions each, and the depth write of every shown pixel),
which only that whole design removes: all of a frame's triangles bucketed by depth before any is drawn, coverage bits
in place of the depth test in every span loop (ARM), the later passes (props, people, the view model) in the same
buckets. A big rewrite - asked the user before starting.
Nov 5 23:40 user: "I want more optimizations", pointed at janseris/maps9300 (cloned read-only to /home/claude/maps9300).
Coverage bits instead of depth (Maps' design) rejected: its strict nearest-first order breaks on our big floor / wall /
road triangles (a floor bucketed by its near end would hide whatever stands on it). Taken from Maps' r3dspan.c instead
(v0.4.76): half_tex1 with the texel's two coordinates packed in one register (one add a pixel; 6.10 fixed each, 64 x 64
pictures; the packed step once a call; rows start exact) and the colour store off the depth pointer (one pointer a
pixel): -2 instructions a pixel, hidden or shown. All 41 scenes 274.4 -> 272.8 M (-0.6 %, up to -2.2 %); pixels: texel
edges a pixel apart in places (Gerdur's house against a wall: 1,671). Rows are short here (a few pixels), so what is
done once a row decides: next, the same packing in the other textured rows (half_texq1, persp_rows*), and two hidden
pixels a depth word.
Nov 5 23:50: the MP4 player chat's lessons (user's attachment) checked against Hrimfell - already so: direct screen
access from an even x, 2-pixel stores for the doubled width, no float, const tables (.data/.bss 0), a chunk heap, the
loop on a CIdle (no 64 Hz timer snapping), auto speed with a gap between its switch points. Taken: (1) the emulator's
fps estimate now 3 cycles an instruction (their phone: ~20 ns each; ours was 2.5 - estimates were ~15-20 % high);
(2) an area file in one read (the phone's reader opened, sought, read and closed it every 64 KB: ~95 times for 6 MB);
(3) Show FPS adds "d<ms> c<ms>": drawing and the copy to the screen, averaged over the second. OPEN: the copy is a full
pass over the view (theirs: ~5 ms for 320 x 200) - drawing straight into the screen would save it but may show half-
drawn frames (the sky, then walls, then things over them); decide from the phone's "c" figure.
Nov 6 00:20 (user: "read maps9300 source and deduce optimizations"). Maps' renderer against ours: corners once a
mesh, culling in ARM, packed rows (taken) - already or done; Maps' biggest later wins were the data cache. Our cache
model by function (rungame CACHE=1 CACHEMAP=1, from the cut-off turn) showed the Stronghold's misses led by TopAt
(17 %) and BlockedIn (5 %): game lookups scanning every loaded hall's pieces. v0.4.77: R3D::IGridMake - the drawn
interior's pieces listed (DrawInterior reads only them, a hidden one's record not at all) and on a 4 m grid for
TopAt/BlockedIn; lookups for other halls (their people, Forelhost's draugr) scan only that hall's span (IRangeOf) -
no grid made for them (made per question it thrashed). Misses a frame: Stronghold 172 -> 150 k, crypt 143 -> 104 k;
pixels, tests, doors and the stuck probe unchanged. Left from Maps: the row loop without clamps for triangles wholly on
the screen; TopAt's remaining misses (still 7-10 %: the props' and people's lookups in the drawn hall's squares).
v0.4.77 + the dragon icon (aif/v11, from the user's make_dragon_below_icon.py: 25 x 20 and 64 x 50, 16-bit (c16, as its notes ask) with 1-bit masks; built: bmconv work/aif.mbm /c16v11/icon25.bmp /1v11/icon25_mask.bmp /c16v11/icon64.bmp /1v11/icon64_mask.bmp, genaif -u 0x0E3D9301 work/spec.txt work/out.aif, copied to build/hrimfell.aif).
Nov 6 00:30 v0.4.78 (user: "do 1, 2, 3"): (1) half_tex1 / half_texq1 rows without the clamps when the half lies
within xlo..xhi (checked once a call at its first and last row: the edges are straight), and the row's end steps w, u,
v only (not the unused fourth): all scenes 272.2 -> 270.9 M, the same pixels; (2) half_texq1 (128 x 128 pictures)
packed like half_tex1 (7.9 fixed), generated from its macros (HTQPIX/HTQROWS): 270.9 -> 269.8 M, caves most (sunlit
cavern -3.6 %), texel edges a pixel over in places; (3) two hidden pixels from one depth word - worked out exactly
(the whole word against w1 << 8 is a safe test for the upper one): 12 instructions for two against 6 + 6 now - no
gain, not done. Since v0.4.75: 274.4 -> 269.8 M (-1.7 %) besides the cache-miss cut of v0.4.77.
Nov 6 00:40 v0.4.79 (user: "do it" - the rows' state in registers): our pixel loop needs all 14 registers (the depth
buffer), so the row state can't stay in registers as Maps 9300's does; what is the same for every row now does -
r4 the depth step, r7 the packed texel step, r10 the palette, r11 the picture, loaded once a call - and the pixels'
scratch is r8, so the state pointer r0 is no longer pushed and popped each row. half_tex1 and half_texq1 (generated
from it): all scenes 269.8 -> 268.1 M (-0.6 %, -1.0..-1.4 % in the textured interiors and caves), the same pixels.
Since v0.4.75: 274.4 -> 268.1 M (-2.3 %). Left: the perspective rows (persp_rows*) the same way; the row's own
~50 instructions are now mostly its edges, its start values (3 mla) and the state's updates.
Nov 6 00:50 v0.4.80: the perspective rows (persp_rows1/q1/h1: PERSPROWS's non-cutout segments) packed as half_tex1:
the two texel coordinates in one register from each segment's exact start (16 - ls whole, ls fraction bits:
PRPIX), the colour store off the depth pointer, two pixels a turn. All scenes 268.1 -> 265.5 M (-1.0 %; the Hall of
the Dead -4.2 %, the catacombs -4.4 %, the trader -2.6 %); single texels a pixel over inside textures. The cutout
segments (key: leaves, webs) keep their conditional loop. Since v0.4.75: 274.4 -> 265.5 M (-3.2 %).
Nov 6 00:55 v0.4.81: PERSPROWS writes the call's constant values (the depth step, the texel steps, the picture, the
palette) into the walker's context once a call instead of once a row: 265.5 -> 264.9 M, the same pixels. A
perspective call is ~6 short rows: ~100 instructions of row set-up and ~60 a segment against ~120 of pixels a row.
What is left there is register-bound (the row pointers saved and reloaded round each segment's set-up, which uses
every register). Since v0.4.75: 274.4 -> 264.9 M (-3.5 %), and v0.4.77's cache misses -13..-27 % in Forelhost.
Nov 6 13:50 the user's phone figures for v0.4.81 (fresh New Game; their settings: VIEW=2097248 - 170 m, far land
500 m, textures 50 m, the rest default; hands sheathed). The emulator at the same travel places (rungame SC=1xx: the
travel list's place xx as Teleport puts it, 2xx: then facing Whiterun's middle) - phone / emulator: Riverwood street
5.2-5.4 / 5.3 fps (2257 / 2257 t, d ~185 / 182 ms), Stronghold 5.4-5.6 / 5.7, Crypt 7.7-8.5 / 9.3 (d ~113 / 102:
the phone ~10 % slower - the cache misses, Forelhost's interiors), Market 6.3 / 6.4, Stables inside 5.1-5.3 / 5.9
(3434 / 2861 t: people there on the phone), Windmill 12.4-13.3 / 12.4, facing Whiterun 4.2 / 4.1 (4473 / 5008 t),
Hall of the Dead 12.0 / 12.4. The estimate (3 cycles an instruction) holds within a few %. Decided: c (the copy to
the screen) is ~5 ms (0-13): drawing into the screen would gain 2-6 % at most - NOT done. Cache work pays only in
Forelhost's interiors. Loading 9-12 s on the phone; the emulator's set-up of a place in another area 200-360 M
instructions (4-7 s of it is computing - to look at: LOADI=1 rungame prints a scene's set-up).
Nov 6 14:00 v0.4.82 (user: the manual test took ~1 hour with the 13 min download and 10 min install):
(1) Menu "Benchmark (send to PC)" (Game::Bench*, game_cells.cpp): every travel place (and the Windmill again facing
Whiterun), each: Teleport, its loading time, 1.5 s to settle, 3 s measured (frames, fps, triangles avg/min/max, d
avg/min/max, c avg/max, the slowest frame), where the player stood (cell x z y yaw pitch, hour) - the player still,
weapon sheathed, Auto speed off then put back; Esc stops. The log (in Game, 16 KB) is saved to
C:\Data\Hrimfell\bench.txt and POSTed to http://192.168.137.1:8000/results?name=hrimfell_bench (as Maps 9300's
log: the server's uploads folder). ~3-4 minutes. Emulator: BENCH=1 VIEW=... emu/rungame (the same log, a frame's
time from its instructions). (2) The package split: sis/hrimfell.pkg the program only (347 KB: skyrim.sis),
sis/hrimfell_data.pkg (UID 0x0E3D9302, 20 MB: skyrim_data.sis) the data - sent only when data/ changes. First
install: skyrim.sis, then skyrim_data.sis (the upgrade removes the old package's data).
Release step from now on: makesis both when data/ changed, else only hrimfell.pkg; send skyrim.sis (and
skyrim_data.sis when it changed).
Nov 7 00:45 the MP4 Player chat's drawing-speed notes (the user's FOR-HRIMFELL-drawing-speed.md), measured
(emulator, the user's settings, travel places): rungame now counts the writes past the cache (CACHE=1: their
bytes and the ms at the phone's ~63 MB/s; CACHEMAP=1: by function) and takes GAMMA=0..3 (the Brightness setting).
- The back buffer + Mem::Copy: right, kept (c ~5 ms on the phone).
- Brightness (ApplyGamma, 128 KB table): off at Normal (the user's - the default). Brighter costs +0.35 M
  instructions and +11 k read misses a frame (~10 ms, ~5 %): if the user ever plays brighter, put the curve into
  the palettes/colours instead (0 a frame).
- Writes past the cache: 555-840 KB a frame (9-14 ms at 63 MB/s), mostly overlapped with the work (the 3 cycles an
  instruction estimate matches the phone with them). The one pure write stream: fill32 250 KB (the sky's fill
  125 KB + ClearZ 125 KB at 320x200), ~4 ms a frame. Alternating depth ranges would save ClearZ's ~2 ms (1-3 %)
  but needs both compare directions in every span loop: NOT done. Then xform_v (TV 40 bytes a vertex: 97 KB in
  Riverwood's street) - a smaller TV would cut both its writes and the read misses after; a big change, later.
- Info notes: the game shows none of the phone's; StartDirect is tried every frame: right already.
Nov 7 00:55 v0.4.83 (the user plays at Brightness: Brightest): the Brightness curve put into the colours where they
are made, not run over each finished frame. R3D::iCurve (256 bytes, identity at Normal; the 128 KB iGammaLut and
ApplyGamma gone): RgbC (the shaded palettes - iShadePal, iLibShade, iSkyShade, iBasePal -, the sky's rows, stars,
sun, clouds, the slot-coloured flat faces), CurveTV (the vertices' colours: FogColour - land, far land, water - and
the gouraud meshes), face_shade's flat colours (FaceCtx.curve, FC_CURVE 196), the inventory preview's palette.
The flat faces averaged from vertex colours use ::Rgb (curved already - the first try curved them twice: half the
screen pale). The UI (menus, HUD, map, books) is no longer brightened: its own colours. Emulator, GAMMA=3, the 41
scenes + 7 travel places: 331.0 -> 316.4 M instructions (-4.4 %; Brightest now costs +0.6 % over Normal) and the
table's ~11 k read misses a frame gone; the pictures as before (mean difference 4-6 /765 a pixel: the curve now
before the 5-6-5 rounding - truer), the inventory's background darker (the UI not brightened).
Nov 7 16:08 the user's first phone benchmark (v0.4.83, 221 s; uploads/..._hrimfell_bench.txt). Its settings were
all the defaults (far land 1.5 km, textures off, Brightness Normal): the settings lived only in the game's state -
not kept across a restart. Emulator BENCH=1 VIEW=0 against it, place by place: the triangles the same at every
place (to a few); d phone/emulator 0.79-0.92 outside (textures off: flat and gouraud spans - fewer loads an
instruction than the textured ones the 3-cycle figure was matched on), 0.92-1.03 inside (textured). The cost model
should price instruction kinds (loads, stores, ldm/stm, multiplies, branches) - fit to these 35 places + the 8 of
Nov 6. The phone's timer ticks 15.625 ms (1/64 s): a frame's d and c are whole ticks - averages over a place good,
min/max not. Loading (only where an area's file is read): 9.0-11.0 s - the next target (~45 s of the 221).
User: Alvor's forge's place faced a post filling half the view - not representative.
v0.4.84: (1) the settings (Brightness, the view's ones, Display, Show FPS) in their own file
C:\System\Apps\Hrimfell\hrimfell.cfg: read before the start-up's last stage (Game::iCfg, CfgUse), written when they
changed and the menu is shut (CfgCheck once a frame -> iRequest 6); kept by New Game (Display too now) and by
loading a save. (2) Travel "Riverwood - Alvor's forge": 8 m back (z -6 m), facing west (768): the forge, its roof,
Alvor (found with the emulator: SC=300+k moves place 22 by a grid of offsets and yaws).
