# The world (v0.4.47)

## Skyrim's land
- Source: the user's Skyrim.esm through tools/skyrim_heightmap.py (tamriel_export/: heights every 128 game units,
  cells x -57..61, y -43..50, ground textures per quarter cell, water level per cell).
- tools/make_land.py turns it into data/hrimfell.land (3.1 MB, ~2 MB in the .sis): Skyrim's land at its true size
  with a vertex every 4 m, 1744 x 1376 cells (6976 x 5504 m), x east, z north, (0, 0) the south-west corner of
  Skyrim's cell (-57, -43). Engine height = Skyrim height + 206 m, so the sea (-14000 units = -200 m) is at the
  engine's water level of 6 m. One Skyrim game unit = 1.42875 cm.
  - Skyrim (X, Y) units -> engine metres: x = (X + 233472) * 0.0142875, z = (Y + 176128) * 0.0142875.
  - Heights: predicted from the left, below and below-left neighbours, a signed byte each (-128 escapes to a short).
  - Ground: Skyrim's texture names mapped to TM_* (tundra, pine forest, the Rift's autumn floor, the Reach,
    Hjaalmarch marsh, Eastmarch ash, snow, rock, sand, dirt), run-length coded; steep slopes become rock.
  - Water: per chunk (8 x 8 cells), the level of the Skyrim cell at its middle, or the sea; none where the land is
    above it. R3D::WaterAt; swimming and the water drawn per chunk use it.
- Loading: R3D::LoadLand reads the file through a callback (the app: RFile on C:, E:, D: or Z:; the emulator: a
  memory blob; the PC tests: data/hrimfell.land from the working folder). Without the file the old made-up land is
  generated. The .pkg installs the file to the drive the user picks.
- Trees: grown per chunk from the ground (dense in pine and autumn forests, a lone pine now and then on the tundra),
  never on paths or by buildings; the Rift's are golden.

## Storage and drawing
- One short of height and one byte of ground per vertex (~7 MB), a cache of 768 lit chunks with their trees, the
  far land's colours (a vertex every 16 cells, re-lit when the sun moves).
- The detailed land out to the draw distance; past it, when the far land is on, a coarse sheet out to 1.5 / 3 km,
  hazing into the sky (the fog grows with the distance squared then).
- Settings (menu, kept in iS.view): Draw distance 170 / 250 / 350 / 120 m, Far land 1.5 km / 3 km / Off.

## Places (true positions, from tools/skyrim_places.py)
- Whiterun: the city plan's origin at cell (918, 605): the plan's main gate stands on Skyrim's (the Tamriel gate door
  at game units (19312, -7424) = engine (3612, 2410) m); the lower city at the gate's sill, -50.3 m (155.7 m engine).
  Skyrim's walls span 164 x 217 m; our plan's 120 x 155 m (about 0.72 of the size): it covers the western two thirds
  of the real city, Dragonsreach about 38 m short of the real one (docs: the overlay picture in the v0.4.23 notes).
- Bleak Falls Barrow: built from Skyrim's placements (docs/maps/gen_bfb_ext.py -> src/bfb_ext.inc): the main door at
  (3292.25, 1897.23) m, sill 86.9 m (Skyrim), facing 104.6 degrees; 50 platform floors shaped into the land, 9 flights,
  48 retaining walls, 17 posts and statues, 8 great ribs, rubble, Bleak Falls Tower (NorTowerRuins01) at its spot,
  the sanctum's way out where Skyrim's is (143 m west, 64 m south of the doors), the bandits where Skyrim puts them.
- Trees and rocks: where Skyrim has them (trees.pgm / rocks.pgm), as marks in the land file (VM_TREE / VM_ROCK);
  the share of real trees among Skyrim's TREE records measured per ground (pine forest 60 %, snow 56 %, rock 33 %,
  grass 16 %, dirt 14 %, tundra 10 %).
- Ground: per vertex (ground_full.pgm).

- Riverwood: houses, inn, mill, bridge, walls, fences and yard things from Skyrim's placements
  (docs/maps/gen_riverwood.py -> src/rw_ext.inc), origin (3650, 1870) m. The people: Alvor at the forge, Sigrid, Hod,
  Gerdur and Faendal at the mill, Sven and Hilde by their door; their greetings are Skyrim's own
  (DialogueRiverwood_Revised; Hilde's from MS13).
- The Riverwood Trader (CELL_RWTRADER, v0.4.24): the cell RiverwoodRiverwoodTrader rebuilt from its 230 references
  (refs_interiors.csv; Skyrim units x 1.43 = cm, written in Skyrim units in game_cells.cpp so they can be checked):
  flagstones and fieldstone, the round stone chimney with its raised round hearth and the cooking pot, the L-shaped
  counter (CounterStraight/Right/CornerIn/Left), the shelves, cupboard, dresser and chest, horn candles (floor, wall,
  table), the hanging garlic, herbs, rabbits and pheasants; the stairs climb north along the west wall over barrels and
  firewood (as the screenshots show); upstairs at 343 cm under the thatch: the two beds, the long table with its
  benches, the round and square tables, the stag heads and slaughterfish over the balcony door, pelts, four lattice
  windows high on the east gable. Lights where Skyrim has them.
  - Lucan Valerius (the merchant, general goods, 750 gold, open day and night until the claw is home) and Camilla.
  - The Golden Claw (MS13), with Skyrim's lines: the argument scene the first time in, "Did something happen?", the
    quest (stage 10), the nag, the return once the claw door is open (400 gold, stage 200), the claw back on the
    counter; Camilla's lines for each stage.

- The Sleeping Giant Inn (CELL_RWINN, v0.4.25), from its 484 references (docs/maps/gen_sginn.py -> src/sginn_ext.inc,
  kept in Skyrim units in the code): the hall 26 x 12 m under a thatch ridge at 11.4 m, flagstones, plank walls with log
  beams at 3 and 4.5 m and the strip of lattice windows above; 14 braced posts in two rows (the horn candles on them);
  the long fire pit on its stepped plinth, three fires, the spit; the bar at the west end (Orgnar, the kegs, the
  storeroom behind); five long tables with benches; the two rooms south (the east one Delphine rents: 10 gold, her
  own lines) and Delphine's room north, their doors shut; behind her wardrobe, the stone passage down 3.7 m to the
  secret room (map table, alchemy table, enchanter's things, the practice dummy, weapon racks) - shut until her quest.
  Outside: FarmInn01 at its true origin and rotation (132 degrees), built to its own size (Skyrim's insides are larger
  than its outsides): the tall hall, the two wings, the plank porch with its railing and steps, the sign on its post.
  - Rooms drawn only when they can be seen (Obj::pad zones; R3D::iZoneMask): 3.1 -> 4.4 fps in the hall.
  - Delphine, Orgnar (food and drink, open day and night) and Embry with Skyrim's own lines.

- Riverwood's four houses (v0.4.26) built from Skyrim's own pieces, not by hand: Alvor and Sigrid's House, Gerdur's
  House, Faendal's House, Sven and Hilde's House (CELL_RWALVOR..CELL_RWSVEN). tools/skyrim_kits.py showed 77 interiors
  share the farmhouse building set (73 pieces cover all of them); tools/skyrim_meshes.py read those pieces' real
  meshes from the .bsa archives. docs/maps/gen_kit.py simplifies every model (quadric edge collapse, 101 models,
  7756 triangles), colours each part from its texture's measured average, places every static, prop, item, candle,
  light and person where Skyrim places it, and takes the floors and walls you walk on and against from the building
  pieces' own triangles -> src/kit_ext.inc, read by the generic src/game_kit.cpp. Doors are Skyrim's, both sides.
  Hilde is home; the rest keep to the street. 6-7 fps estimated inside.
  - Fixed on the way: Obj::type was one byte, so statics numbered past 255 drew and blocked as the first ones.
  - v0.4.27: Dorthe (Alvor's) and Frodnar (Gerdur's), Riverwood's children, at their Skyrim places, child-sized,
    with their own lines (DialogueRiverwood_Revised). Riverwood's people keep Skyrim's hours: out 8:00-20:00 (the
    children at the forge and the mill), home at night by the beds of their house; never moved before the player's
    eyes (not in the house, not within 60 m outside), and all set right when a door is used. The farmhouse pelts
    are grey wolf hides as their textures are, no longer the engine's brown fur. host/kitreach floods each house
    from its door: Alvor's two levels both reached; the other "upper floors" were the tops of wall pieces.

- v0.4.28: the Riverwood Trader and the Sleeping Giant rebuilt from Skyrim's own pieces as the houses are (gen_kit.py,
  "geometry only": statics, fires, lights, floors and walls; their furniture, goods, doors, people and quests stay in
  game_cells.cpp). The hand-built shells, floors and walls are gone. Collision is now taken from the pieces' full meshes
  (simplified on their own, so doorways stay open); each flight of log stairs becomes one ramp (Skyrim's physics
  walks them as a slope); steps up to 33 cm climb in these interiors; floors and walls are looked up through a
  128-unit grid. Delphine's wardrobe (the Riften cabinet) is drawn and walled shut. Statics' collision boxes now sit
  on the model's middle (corner-origin counter pieces). Townsfolk step aside when walked into; nobody is pushed into a
  wall. host/rwkitwalk walks both with the game's own movement (counter, stairs, upper floor, hall, bar, rented room,
  secret passage). 5.6 fps in the Trader (was 4.7); 3.7 in the inn's hall (was 4.4: the real hall has more in it).

- v0.4.29: Riverwood outside from Skyrim's own models too: the Sleeping Giant (Inn01), Alvor's house and forge
  (Smith01), Farmhouse02/04/05/06, the lumber mill, the porch walkways, the boardwalks and their steps, and the houses'
  own doors (FarmhouseLDoor01), each at its place, turn and height in Skyrim (gen_kit.py, "RiverwoodOutside"); a far
  model of each from 35 m. Their decks, porches, boardwalks and steps are floors you walk on (the raised deck of
  Alvor's forge, the inn's porch, Sven's porch over the river), their walls stop you; outside, a deck's edge up to 45 cm
  over the land is stepped onto; coming out of a door you stand on the deck, not under it; Riverwood's people stand on
  the decks too. Sloping faces (stair sides) become walls only as high as they are along their length. The hand-built
  houses, the inn's shell and porch, the mill and the boardwalk boxes are no longer placed (the water wheel and the
  bridge still are). host/rwoutwalk walks from the street to every door and onto Alvor's deck.

- v0.4.30: everything that stands in Riverwood is Skyrim's own model now: the stone bridge (Bridge01, walked over on
  its own deck), the lumber mill's water wheel (FarmLumbermill01WaterWheel01, turning about its axle), the river's
  surfaces, rapids, falls and splashes (FXCreekFlatLong01, FXrapids..., FXWaterfall...: drawn as water and foam), the
  gate walls and their walkways, the dry-stone walls and woven fences (one line each to walk against), the cliffs,
  rocks, logs and stumps, firewood, hay, carts and their wheels, the workbench, grindstone, cooking pot, benches, chair
  and tables, the weapon racks, the signs, the bee hive, the fish rack, the spit and the campfire (with its fire); 465
  pieces, 43 models. Only Alvor's forge, the tanning rack and the horse trough are still hand-made: their models are
  world-persistent things tools/skyrim_meshes.py did not take yet (updated to take them, and to keep the tree files
  it cannot read). Faendal chops wood at the block nearest the mill by day (the swing frames; he turns to you when
  you come within 4 m). The models are drawn straight from the program's own data now (normals and texels worked
  out by gen_kit.py exactly as the engine would): the mesh arena is 450 KB smaller.

- v0.4.31: the last hand-made things in Riverwood are gone: Alvor's forge (BlacksmithForgeMarker.nif) and the tanning
  rack (TanningRackMarker.nif) - "marker" files that carry the furniture itself, which tools/skyrim_meshes.py now takes,
  standing against their own walls (a box round the forge would take in the place Alvor works at); the horse trough,
  the sawmill's log pile (walked on), Faendal's woodpile and the mill's lever. Riverwood's 36 pines (TreePineForest01..05,
  Dead03) are Skyrim's own at their places, a far model each from 22 m, a trunk to walk against; the land's grown trees
  and rocks are left out of the village. tools/skyrim_meshes.py reads skinned shapes now (their vertices are in the
  skin partition, full floats; the triangles from its shared-vertex copy): the trees, books, banners, hanging game.
  Fixed: more statics than the engine held (MAXDEFS 400 -> 600; v0.4.30 lost some of its last models), more models than
  the kit held (MAXKIT 360).

## Pending
- Whiterun at its true scale from its own city space (WhiterunWorld: 1805 placements, heights for cells 4..7, -3..-1).
- The other farmhouse interiors (77 share the set); people
  lying in their beds at night. The bridge's deck to walk on, the mill's roof.
- Names: Skyrim SE keeps its strings inside a .bsa, so names.csv / markers.csv came without names.

- v0.4.32: built from the user's full export (911 models, none failed): the hanging rabbits, the pheasant and rabbit meat
  and Alvor's steel bow come into the houses. Untextured shapes are left out of every model: Skyrim's editor previews
  in the forge marker (two skinned mannequins), the forge's skeleton helpers (camera, foot pivots, weapon boxes) and
  the water wheel's ragdoll collision object and splash emitters - the "ring" over the mill was these.

- v0.4.33 (tested in Alvor's house; every change is to the shared models and rules, so it holds wherever they are):
  the stairs: Skyrim's log steps (small separate logs in FarmInt2End01 and the like) are found by gen_kit.py,
  drawn as a box each (simplified with the rest they vanished, leaving the two stringers) and walked as one ramp
  from a rise below the first to the landing a rise above the last; their sides are no longer walls. The first log
  stood 46 cm up and its side counted as a wall, so the foot of the flight stopped a walker; at the phone's frame
  rate a frame's step (about a metre) also rose more than a floor is looked for above the feet: the player's move
  is now taken in strides of 25 cm at most. Furniture and hides: small models' textures are mapped to the engine's
  own (Dresser01 / Bar01 / NobleFurn / workbench / mounts / carts / buckets: the stained furniture wood; pelts: fur;
  baskets: straw; logs, planks); a textured face takes its colour from the texture, as the dresser and wardrobe do.
  The chest is Skyrim's Chest01 at its size (96 x 49 units, lid 14): iron corners, straps and lock plate, the whole
  lid hinged at the back with its straps and latch, the dark inside when open (the old lid covered only the back
  half). The static barrel (Barrel02Static) is the working barrel's shape at Skyrim's size in dark staves bound with
  rope (two new texture patterns: textures can now be any of 32 patterns, each in one of the 16 hues).
  tools/skyrim_meshes.py also saves every diffuse texture small (64 on the longer side) in textures\.
  Flat things on a floor (the floor pelts and rugs: every corner at one height, a third of a unit over the boards) are
  lifted 2.5 units: in the floor's plane their depth and the floor's were the same to the engine's 16 bits, and which
  showed changed as one moved. The static barrel's rope is thin (2.5 units) and a dim brown.

- v0.4.34: the world is read from files on the card, not held whole in memory (the start of loading the world by
  area, for every city and region to come). The land: a window of full detail round the player (256 x 256 vertices,
  1 km, 196 KB) read from data/hrimfell.land in tiles of 32 x 32 as they go (the window follows when they come within
  256 m of its edge; on a door to the outside, a new game or a loaded save it is moved there first), and the whole land
  coarse beside it (every 16 m, 452 KB: the far land, the map, whatever is far away); 7.2 MB before. The land file is
  baked (host/bakeland.cpp, built with R3D_FULLLAND: the game builds its world on the whole land once and writes what
  it changed into the file - the city's hill and terraces, the barrow's platforms, Riverwood's ground, the roads, rock
  on the steep, no grown trees round what was built); the game only reads it. tools/make_land.py's land is kept as
  data/hrimfell_v1.land, the baker's source. Things that take their height from the ground are settled again when
  their land is loaded, and so are the people and creatures outside. Riverwood (models, interiors, collision) is read
  from data/riverwood.har (tools/kit_pack.py packs gen_kit.py's src/kit_ext.inc) into a 1.6 MB block at the start,
  instead of being built into the program. The program: 1.89 MB -> 0.55 MB; the game's memory 14.2 MB -> 9.8 MB.
  Build steps now: gen_kit.py -> kit_pack.py -> data/riverwood.har; make_land.py -> data/hrimfell_v1.land ->
  host/bakeland -> data/hrimfell.land.
- v0.4.35: menus take every press (latched between frames, repeated when held) and keep the world still behind them;
  C crouches (Q casts), X names the model under the crosshair; the loot chest opens in 5 frames; hanging rabbits,
  pheasants and coin purses are taken (or stolen); a flame on every candle and horn of a candleholder. The Riverwood
  Trader's upstairs door (FarmhouseLDoor01 00070E68) leads to its balcony: doors carry the floor height on each side
  (DoorLink hk / iy / oy), so a door over another is told apart and you come out on its floor. The map: the world map
  fills the screen (256 pixels across the land, lit along its slopes, drawn smoothly), Z / X zoom (all of it, across
  the screen, twice that), M opens and closes it, L switches the local and the world map; the map markers (Whiterun,
  Riverwood, Bleak Falls Barrow: city, settlement, Nordic ruin icons) on the world map and on the compass within 1 km,
  black until found, white after; on the local map Riverwood's buildings (the tall walls of its Skyrim pieces) and
  their doors. A place reached for the first time: "<NAME> DISCOVERED" in large letters, wiped on and off from the
  left (found places: iS.spare[1], a bit each).
- v0.4.36: Bleak Falls Barrow's outside is Skyrim's own (tools/skyrim_area.py bleakfalls -> gen_kit.py's 4th
  argument): 262 pieces from the hidden way out of the sanctum to past Bleak Falls Tower (Skyrim x -13500..9000,
  y -48500..-40000) - the temple and its base, the great ribs, the platforms, walls, flights and towers, the dragon
  statues, the great doors (drawn shut), Bleak Falls Tower with its floors, stairs and bridges, the rocks, cliffs
  and trees round them (not the mountains' own models: the land is the mountain, and one walks on it); what Skyrim leans (rot x / y: rocks, cliffs, debris; turned about z, then y, then x,
  each clockwise - checked against the land) is its own model, turned as placed. Snowy pieces (...Snow, _SN) have
  their upward faces snow (Skyrim lays the snow by material); the ruins' stone and the rock lighter. The home-made
  barrow (its platforms shaped into the land, walls, posts, ribs, facade, tower: bfb_ext.inc's tables and their
  meshes) is gone, and the land baked again without it. Outdoor kit cells can now be many (game_kit.cpp: each its
  origin; its "door" row the bounds with no land-grown trees); the stone's floors and walls are found from where the
  pieces are solid (gen_kit.py hf_collision: sampled from above every 24 units, 12 for the tower; tops joined into
  planes; walls where one point is solid over a stretch its neighbour is not), the rocks' as before. Doors know the
  height of their floor outside (the barrow's: its sill); the bandits stand where Skyrim has them, on the platforms
  and in the tower, the tower's chest is Skyrim's (TreasBanditChest 0001EBF7, on the tower's top floor); things that
  block outdoors only block at their height. The area file grows to 2.59 MB (AREA_KB 2600; game memory 10.1 ->
  11.2 MB); MAXKIT 720, MAXDEFS 1000. Host tools: reach (where one can walk from a point), outlook (a view from a
  Skyrim point), bfbout (out of the barrow's doors), skyground (the land's height at Skyrim points), ents.
- v0.4.37: settings: Textures (off by default: textured models drawn flat in their colours, a quarter to a half
  faster: the Riverwood Trader 4.7 -> 7.3 fps in the emulator, the Sleeping Giant 3.6 -> 5.7, the barrow 4.0 -> 5.5),
  draw distance 60 .. 350 m in eight steps, far land off / 0.5 .. 6 km in eight (iS.view: bits 0-2, 3-5, 6). Faster
  too: small things drawn lit as a whole, a light only where it reaches, things under a pixel not drawn, the flat
  spans two pixels a turn, the people who stand still not looking for their floor every frame. Pines: Skyrim's are
  branch cards cut out by their texture (a tangle of triangles here); each forest pine is drawn the engine's way at
  its size instead (a trunk and eight tiers of drooping branches, closed underneath; the far model four tiers; snowy
  ones every other tier white), and the land's own pines likewise (seven tiers near, four, two far). Bleak Falls'
  eight bandits are Skyrim's (melee, archers, a heavy one, a two-handed one: Ent::looted bits 1-2); archers stand and
  shoot while they see you (arrows fall, miss a little, stop at walls and floors); no blow reaches through a floor.
  Searched, a bandit gives what Skyrim's level-1 bandits carry (its weapon, fur or iron armour, gold, sometimes
  lockpicks, food, a gem, jewellery, an ingot): new items Iron Mace, Greatsword, Battleaxe, Warhammer, Hunting Bow,
  Long Bow, Iron Arrow, Lockpick, Iron Ingot, Steel Dagger. Hit: the red edge thick on the side it came from, a marker
  pointing at it, "Hit by an arrow". The health, magicka and stamina bars show only while not full or changing. The
  map pans faster the longer a key is held.
- v0.4.38: Skyrim's own textures (Textures on): the 64 most used of the models' textures (64 x 64, one palette of
  256 colours, in the area file after the models: "HTX2", version 3) on Skyrim's own texture coordinates (carried
  from the full model to the simplified one, face by face); a slot's look byte 0xC0 + the texture's number (SL_SKY),
  its colour the texture's average (far away, or with Textures off). Each face picks by how many texels fall on a
  pixel: the picture, the picture blurred (2 x 2), or flat; perspective only on faces that recede by an eighth or
  more, in runs of 32 pixels. Faster: rooms draw their walls and big pieces first and leave out what is wholly
  behind them (the z buffer's farthest per 8 x 8 block; outdoors behind the buildings and the land); 90 pieces of
  furniture and bigger clutter get a plainer model (a third of the triangles) past 7 m; a woven fence far off is
  just its panel; near land flat where its
  corners' colours hardly differ. Emulator, Textures off (v0.4.37 -> now): the Riverwood Trader 7.3 -> 9.5 fps,
  the Sleeping Giant 5.7 -> 7.4, Alvor's forge outside 5.0 -> 6.6, the barrow 5.5 -> 5.8; Textures on: the Trader
  6.4, the inn 5.3, the forge 6.0, the barrow 5.0. Area file 2.80 MB
  (AREA_KB 2900).
  Textures with transparency that are mostly opaque (the floor pelts, rugs, banners, the hanging game) are Skyrim's
  too, drawn whole, their cut-away texels in the opaque ones' average; the leaves, grass, flowers and effects stay
  out. A slot whose texture has no picture is flat in that texture's own colour, never the engine's wood or fur (the
  pelts were brown fur with Textures on and pale without: they are pale Skyrim pelts either way). Simplifying keeps
  what faces up (floors, table tops) in its plane far more firmly.
- v0.4.39: Roads from Skyrim's own road pieces (the land's ground ROAD, 14: where their paved faces lie). Skyrim's
  plants on the land (8552 of them, 21 kinds: bushes, ferns, flowers, the herbs one picks with E - they come back
  on loading) and tufts of grass on grass within 13 m; Settings: Plants on/off. Skyrim's animals where Skyrim puts
  them (253 places: wolves, deer, elk, hares, foxes, goats, mudcrabs), appearing within 110 m; the prey run, the
  wolves and mudcrabs fight; their hides, meat and pieces when searched. Textures: a 128 x 128 picture up close
  (once the exporter is run again), and how far they reach set in Settings (off, 25, 50, 100, 200 m), measured to
  a face's nearest corner (the bridge no longer fills in triangle by triangle). Plank floors and walkways are their
  own outlines (the porches, the balcony, the platform at the inn door), the Trader's balcony fenced by its rope
  rail; the upstairs table (Skyrim's CommonTableTwoBenches) stands; nobody downstairs answers from upstairs (1.6 m).
  The Bleak Falls tower stairs are log steps on skirted sides. One steps up 45 cm outdoors (stones). Discovered:
  only outdoors, by the place. Maps pan evenly (80 px/s, up to 4x over 3 s), the world map sideways too, and zooms
  in twice more onto the land's own picture (data/hrimfell_map.bin: roads, water, woods). The inventory shows the
  items' own Skyrim models (74 of them, the area file's "HIM1" table).
- v0.4.40: Faster: outdoors the buildings are drawn first and the land, the far land and the rest leave out what they
  hide (whole chunks, and single triangles against the farthest depth of each 8 x 8 block); triangles off the screen's
  edges are drawn without cutting up (a band of 640 pixels round it); flat rows of pixels keep their state in
  registers; flat triangles set up without the colour channels; the far land's points behind or beside the view not
  worked out, and re-lit 6 rows a frame; textures recede by a quarter before perspective is corrected (an eighth).
  Emulator, Textures off (v0.4.39 -> now): the Trader 9.7 -> 9.6 (it draws its shelves' goods now), the Sleeping
  Giant 7.4 -> 8.4, Alvor's forge outside 5.7 -> 6.4, the barrow 5.1 -> 7.2, the bridge 6.0 -> 6.6; Textures 100 m:
  6.9, 6.0, 5.4, 5.6, 5.8. The items' own Skyrim models in the inventory as made (up to 2900 triangles, the
  inventory still 18 frames a second and more), with their own textures and palette (data/items.hit, one read at a
  time; the weapons stood up, their blood decals left out); the frame rate shown in the inventory too. The Trader and
  the Sleeping Giant furnished from Skyrim's references like every house (the upstairs table was lost: a furniture
  record, dropped where furniture was typed in by hand); things with no game item drawn where they lie (books,
  baskets, pots, mead); the merchants' chests under the floors left out; docs/maps/audit.txt lists what each cell
  drops and any item with nothing under it. Signs: drawn as made, cut out where Skyrim cuts them (texel 0 the key,
  HTXK), the painted ones whole (an alpha channel is transparency only where the shapes say so: the exporter now
  writes "# alpha test N"). Stairs built into a piece kept a box a step (the tower's stone stairs were a slope).
  Picked plants come back as Skyrim's cells do: 10 days after their cell was last loaded (saved; SAVE_VERSION 14).
- v0.4.41: From the new exports (the shapes' alpha, items.csv): the armour, shields, rings and necklaces in the inventory
  are Skyrim's own models too (by their records: Fur Armor ArmorBanditCuirass, Fur Boots ArmorStormcloakBoots ...),
  110 items in all; each stood on its longest side (shields, plates, swords turn showing their faces). An "alpha test
  0" cuts nothing away.

- v0.4.42: Bushes and ferns (kinds 0 1 2 4 17 18 19) are Skyrim's own leaf cards (swordfern, thicket01, tundrashrub01,
  yellowshrub01, fallforestshrub01, vinemaple01 ...) with their textures cut out (HFM2: a picture, UVs and vertex
  normals a kind; 96 triangles near, 28 past 10 m), no longer solid blobs; their colours a third greyer. Riverwood's
  farmhouse pieces simplified to up to 680 triangles / 1200 vertices (the roofs' gables and eaves closed). The 32/16 px
  texture levels (HTXM, written only with MIPS=1) measured: 1-4% fewer cache misses for 512 KB, so left off.
- v0.4.43: Frame rate. The area's models (48 faces or more) have their faces in clusters of 16 (gen_kit.py: grouped
  by the way they face, then split in space; the vertices numbered as the faces first use them): a cluster off the
  screen, behind the walls or turned away as a whole is left out, and each face turned away is found in the model's
  own space before any vertex is worked out - only the vertices the faces left use are. The rows of a triangle
  behind what the 8 x 8 occlusion map holds are left out (the land behind the houses), the rest cut to the blocks
  that can show (HalfState xlo/xhi, all walkers). A triangle that crosses no pixel row's middle is dropped before
  its colour is worked out; RasterFlat without its edge arrays; the walkers called straight; Clamp/Abs/Rgb inline;
  half_flat1 a few instructions less a row; the sun and the clouds painted last, on the sky still showing. The
  images are as before (the host's, compared). Mesh arena +16 KB.
- v0.4.44: The flat triangles set up in ARM code (spans.s tri_flat1: the same arithmetic as RasterFlat, the emulator's
  pictures compared - the same; the C one kept for those it leaves: 8 rows or more against the occlusion map).
- v0.4.45: The plainer models sooner: the area's buildings their far model from 6 m off their nearest side (was 12),
  trees from 12 m (was 18), furniture and bigger clutter from 5 m (was 7); people their plain model past 7 m outside
  and 4 m inside (were 10 and 5); the plants' far form past 7 m (was 10).
- v0.4.46: A Model detail setting (menu: Normal, High, Highest, Low - the plainer models' distances x 1, 1.6, 2.5,
  0.6: the buildings', the furniture's, the people's, the plants'). The buildings (architecture, 33) have a middle
  model too (gen_kit.py "(mid)": up to 320 triangles; KitMesh lod: the far one's number, the middle one's + 1 in the
  top half): the full one within 3 m of their nearest side, the middle one to 10 m, the far one beyond. The textures:
  the 128 pictures chosen by how big their texels are in the world (weighed by use; the signs first), 40 of them (was
  32: the pelts now among them); the cut-out ones drawn at 128 and 256 too (they were always the 64 - the pelts are
  cut out); 256 x 256 pictures (HTXH) for the first 8 that the exporter writes at 256 (the signs; and, with
  skyrim_meshes.py run again, the textures Skyrim made 2048 or more - the pelts), drawn when magnified 8 times; the
  models' texel coordinates in quarter texels (Mesh uvsh 2; were whole texels of the 64 picture). New walkers:
  half_texkq1, half_texh1, half_texkh1. MAXKIT 800, AREA_KB 5120.
- v0.4.47: The Model detail setting split in two (menu): Full models - the buildings' full model to 3 m (the default),
  5, 8, 2 m or Off (never: their middle model even close, and everything else its plain one); the furniture's, trees',
  rocks', people's and plants' full models in the same proportion - and Middle models: the buildings' middle model to
  10 m (the default), 15, 20, 30 or 6 m. iS.view bits 10-12, 13-15; R3D iLodNear, iLodFar[] by StaticDef lodCat. The
  menu's settings change with left / right too (and the press, as before). From the new export (skyrim_meshes.py:
  _256 pictures for Skyrim's 2048 and bigger, and "# double sided" on the shapes Skyrim draws both sides of - the
  books' pages, the ivy, the pines' branches): the 256 pictures chosen after the signs by how much finer Skyrim's
  picture is than its place in the world (its pixels to a unit of size): the farmhouse stone wall and the three pelts;
  9 at 256 (HI2MAX).
- v0.4.48: Whiterun's outskirts from Skyrim (tools/skyrim_area.py whiterun 4; gen_kit.py "WhiterunOutskirts"): within
  -170..240 m east and -200..60 m north of the city's middle, outside the custom city and its walled approach, 975
  pieces - the Stables, Pelagia, Chillfurrow and Battle-Born farms, the windmill (its sails turning), the fences, the
  hay, the carts, Bjorlam's carriage (CartFurnStatic01), the pines, the rocks; no civil war, roads (the land's own),
  markers or plants. Their four interiors (CELL_STABLES - Skyrim's WhiterunStables in place of the old one -,
  CELL_PELAGIA, CELL_CHILLFURROW, CELL_BATTLEBORN) with their doors. Gone: the custom stables (residence, stable
  block, horses, carriage), the levelled plain, the wheat, the windmill farm, the meadery hall, the east farm, the
  watchtowers, Arnvid. The road from the river road keeps to Skyrim's ground past the carriage, then climbs evenly to
  the approach (it was graded for the old plain, 7 m above the ground). Six people from npcs.csv / packages.csv
  (named 23..28): Skulvar Sable-Hilt and Jervar (the Stables), Severio Pelagia and Nimriel (Pelagia Farm), Wilmuth
  (Chillfurrow), Bjorlam (the carriage, day and night) - their package's place and radius 8:00-20:00, home the rest
  (where Skyrim places them inside; Severio away to his house in the city), in their outfits' colours and their skin
  tints; their greetings stand-ins until the dialogue is exported. MAXIOBJ 1400 -> 2400 (all the interiors share it:
  the four new ones were cut off at 1400), mesh arena +24 KB.
- v0.4.49: Faster, the same picture: the model's vertices turned and projected in ARM code (spans.s xform_v; the rest -
  off the screen - finished by the C one); DrawFaces' first tests in ARM code ahead of it (face_cull: only the faces
  that pass come round); R3D's big arrays (iDefs 112 KB, the land's buffer, the clip lists, the occlusion map, the
  static meshes) moved to the end of the class, so its other fields load in one instruction (they took three); the
  clouds and the sun look for the sky two pixels at a time (z_skip) and skip what the occlusion map holds; the smooth
  spans (half_gouraud1) work out their colour once for two pixels. Stables yard 9.00 -> 8.04 M instructions a frame
  (6.4 -> 7.2 fps est.); forge 7.7, bridge 8.3, by the inn 8.2, barrow 9.0, trader 11.1, Sleeping Giant 9.7, approach
  8.6. Fixed: the people's belts (slot 6, and 7) were never set - whatever the stack held (black, red, any colour);
  now a leather brown unless set.
- v0.4.50: Three settings for speed (menu, below Middle models; the menu scrolls - 19 entries, 16 shown):
  Rows: Full / Half (iS.view bit 16: the world drawn on 100 rows, on the even lines - R3D iH 100, iFy / 2, the colour
  stride doubled - and R3D::EndWorld copies each to the line below, after the view model, before the weather and the
  menus), Near land: Smooth / Flat (bit 17: LandTri always flat), Small things: All / Only near (bit 18: outside,
  things under 2 m across - not trees - only within 25 m). Stables yard 7.2 -> 8.5 (half rows), 7.5 (flat land), 8.9
  (all three); by the inn 8.2 -> 10.2, the forge 7.7 -> 9.8 (est.). Riverwood's doors and windows were drawn pale and
  flat, like holes to the sky: their planks' picture (farmhouse01) had fallen out of the 64 numbered ones (the
  Whiterun interiors' pieces) - a texture's use now weighed by how often its models stand in the world (square
  root); the pictures that will be drawn at 256 kept among the 64 and the 128s (HI2MAX 12: the signs took the pelts'
  places - pelt01 at 256 again); and a shape Skyrim darkens with its vertex colours (under 120: a door's black backing)
  its own slot, its flat colour times them (gen_kit.py "#vc").
- v0.4.51: Auto speed (menu: Off / 8 fps / 10 fps; iS.view bits 19-20): the frames' time summed over 1.5 s of play
  (the phone's clock ticks at 64 Hz: one frame's time is not to be trusted); slower than the aim (+5%), a step on -
  the small things only near, then the near land flat, then half rows (on top of what the settings ask); under 78%
  of it twice running, a step back (Game::ReportTimes, ApplySpeed; the menu shows the step). All three pelts at 256
  (HI2MAX 14). tools/skyrim_area.py writes dialogue.csv: every INFO whose conditions name a placed person (GetIsID)
  or their voice (VTCK; GetIsVoiceType), with its topic's kind (SNAM: HELO, IDLE ...), its responses from
  Skyrim_English.ILSTRINGS, and its conditions as written - for the people's own lines next.
- v0.4.52: Nothing built here left out any more (host/mcheck: 0 parts dropped, was 447): the barrow's halls (game
  _dungeon.cpp DunMesh asked for room against MAXMV, 2048, while MeshInit keeps the meshes built here to 512 since
  v0.4.40 - the rock and stonework past it were left out of 13 of its pieces), the deer's and the elk's antlers (their
  last tine: 160 vertices for the antlered), the grass tufts' third blade (room for 12 vertices, 18 needed). Mesh
  arena +54 KB (5048).
- v0.4.53: The outskirts' people say Skyrim's own greetings (the new export's dialogue.csv; gen_kit.py writes
  src/kit_dialogue.inc): every HELO line that is theirs alone (GetIsID) and whose conditions the game can judge -
  them, alive (GetDead == 0), a male player (131) - the rest (a quest's stage, the father dead) left out: Skulvar 4,
  Jervar 4, Severio 3; a different one each time (talking: iDlgPage -1000 - (named * 64 + line)). Nimriel, Wilmuth and
  Bjorlam have none of their own: theirs come from their voice type and factions (the carriage drivers'), which the
  exporter now writes too (npcs.csv voice, factions; dialogue.csv: their factions' lines) - next export. Generator
  input now /home/claude/whiterun_in2/skyrim_whiterun.
- v0.4.54: Nimriel, Wilmuth and Bjorlam say Skyrim's lines too (the export of 2026-10-30 10:30, whiterun_in3: npcs.csv
  voice and factions): who has none of their own, their factions' greetings (Bjorlam: CarriageSystemFaction - "Need a
  ride?"), else their voice type's everyday ones (DialogueGenericHello, lines shared by several voices and asking
  nothing more: Nimriel FemaleYoungEager, Wilmuth MaleOldGrumpy). Generator input: /home/claude/whiterun_in3.
- v0.4.55: Bleak Falls Barrow is Skyrim's own: its temple (BleakFallsBarrow01) and sanctum (BleakFallsBarrow02) from
  the bleakfalls export, in their own area file (data/bleakfalls.har, loaded on going in - Game::UseArea; the main
  one back on coming out). Walking on Skyrim's navmeshes (floors its triangles, walls its open edges). The working
  parts where Skyrim has them (docs/maps/gen_bfb_kit.py -> src/bfb_kit.inc): the pillars and lever, the gates, the
  pull chains, blades, plates, spike wall, darts, oil and lamps, the claw door, the word wall, the sarcophagus;
  bandits, Arvel (his run along the navmesh: host/bfbkpath), the spider, skeevers and draugr where Skyrim places them.
  What can be seen from where (host/pvs.cpp, at the area file's end): the pieces behind the walls not drawn - the
  entry hall 2.3 -> 7.8 fps (textures on), 11.6 (off). Inside, plants' card pictures drawn cut out; creature-exit
  furniture (a big brown sheet at Arvel's web) left out. Whole quest checked: host/bfbquest.
- v0.4.56: The shared library (data/common.hlb, tools/lib_pack.py): a piece used by two or more areas is kept once,
  with the textures most used across them, loaded at the start and kept; each area's file holds only its own pieces
  and textures. Two banks of textures (the library's and the area's, 64 each) - Riverwood can draw from 95 now (the
  Sleeping Giant's floor textured). Pieces' collision boxes left out in navmesh cells by the game (GameKitNav).
- v0.4.57: Skyrim's own Whiterun (data/whiterun.har: the city as Skyrim builds it - WhiterunWorld's 777 pieces - with
  the outskirts and the stables' and farms' houses, moved out of the main area). Walked on Skyrim's navmesh of the
  city; its own land laid into the land file inside the walls (tools/patch_land.py, then host/bakeland), Tamriel's
  kept under its floors elsewhere. The game's Vindgard is not built when the file is there. Outside, crossing the
  plain between Riverwood and the outskirts switches between the two areas' files (a pause while it reads). The
  Hall of the Dead, Arcadia's Cauldron, the Bannered Mare (both doors) and Dragonsreach open where Skyrim's doors are
  (the game's own rooms behind them still); Carlotta's, Anoriath's and Fralia's stalls where Skyrim's merchants lean.
- v0.4.58: Menu "Update from server": http://192.168.137.1:8000/skyrim.sis (the PC's server on the USB link) read
  with a plain HTTP/1.0 GET into <the game's drive>:\System\Temp\skyrim.sis in 64 KB writes (v0.4.60: two 64 KB buffers, one written asynchronously while the other fills; receive window 64 KB asked for) (hrimfell_app.cpp
  CUpdater, an active object: the game keeps running, progress every MB as messages; Content-Length checked), then
  handed to the system installer (RApaLsSession::StartDocument) and the game closes. Links esock, insock, apgrfx.
- v0.4.61: The barrow's halls, tunnels and caves finer: their pieces (dungeons\...) as the buildings outside - up to
  480 triangles near (were cut to 90: the tunnels' arches and walls a few flat facets), a middle model (320) from 3 m
  and a far one (160) from 10 m off their nearest side (the Middle models setting); the interiors now choose the
  middle model too (R3D::DrawInterior). What can be seen from where recomputed. bleakfalls.har 6.45 MB.
  fps (textures 50 m / off): entry 6.2 / 9.1, spider 5.9 / 8.4, word wall 4.0 / 5.5.
- v0.4.62: The barrow's ambushers as Skyrim has them: asleep standing in their wall alcoves (CreatureAlcove(Bg)Marker,
  11 in the temple, 1 in the sanctum) or lying in a sarcophagus out of sight (the temple's NorSarcophagus one); come
  within 5.5 m (4 m for the sarcophagus) or strike one and it wakes, steps out to where Skyrim stands it, then
  fights (gen_bfb_kit.py: K..Out tables; DunEntUpdate pad2 32/8/128). host/bfbambush tests it.
- v0.4.63: The sarcophagi with a draugr asleep in them (one in the temple, one in the sanctum): Skyrim's own lid
  (NorSarcophagusTop: its piece in the cell taken out, GameBarrowLids) grinds aside in half a second as the draugr
  wakes; the Overlord's lid is Skyrim's too now. The alcove sleepers lunge out (the first moment twice as fast, arms
  swinging - the attack's frames) before walking to their place.
- v0.4.64: The update has its own screen and the game stops while it runs (the game's loop running beside the
  download starved the phone: it stopped answering Windows' pings and the USB link dropped): the game's loop and
  screen stopped, the download's progress on a plain screen (every 256 KB), Esc cancels (the file deleted) and goes
  back to the game; when the installer has the package the game ends.
- v0.4.65: Whiterun faster: what can be seen from where outside (host/pvs: cell 0x7FFF in whiterun.har - every
  static the area puts outside, looked at from the city's navmesh floors on a 128-unit lattice, regions in Skyrim's
  world units; GameKitPvs outside; R3D iOutPvsBits skips the unseen in the outdoor loop; anything within 8 m of the
  eyes always kept). The library's cut-out textures no longer hide what is behind them in the table (the barrow's
  rerun too). Whiterun's own people in its streets (gen_kit.py -> src/wr_folk.inc, from npcs.csv/packages.csv: 24
  people, 27 places by the hour - the market, the Gildergreen, Jorrvaskr's yard, the Skyforge, Olava's porch;
  away in the hours Skyrim has them indoors), with their own greetings; Proventus at the Jarl's side in Dragonsreach,
  Adrianne at her forge by Warmaiden's; Vindgard's five townsfolk gone. MAXE 160 (save version 15).
- v0.4.66: The Rift's autumn forests round Riften as Skyrim has them: every tree and plant the riften export placed
  in Tamriel's cells x 33..52, y -35..-15 (4181 aspens, ~900 pines, ~7000 shrubs, thickets, ferns and flowers) in
  the plants' file (kit_flora.bin: kinds 21-23 aspens - Skyrim's TreeAspen03/05/04: bark simplified, branch cards
  cut out with TreeAspenBranchComp, 234 / ~28 triangles; 24 pines, 25 snowy pines as the land's pines), the trees
  drawn out to the object distance (the Plants setting or not), far form past 16 m; the land grows no trees in those
  cells (tools/patch_land.py takes an export folder: 5988 marks off). The land's grown trees elsewhere on the Rift's
  ground are drawn as Skyrim's aspen too (R3D iFallTree). Emulator: rift forest 6.9 / 7.8 fps (scenes 32, 33).
- v0.4.67: People and creatures by place (src/game_stage.cpp): groups G_* - the barrow's cells (G_BFTEMPLE, CRYPT,
  SANCTUM), its outside bandits (G_BFOUT, 220 m round its doors), the catacombs, Whiterun's people (G_WRFOLK, 250 m
  round the Gildergreen, or in a house when the player left the open world there) - out only while the player is
  there (UpdateStage each tick and on every door), spawned from their place's tables (SpawnDunEnts, SpawnWrFolk) and
  numbered (Ent grp, sid); taken in when he leaves, the dead kept in the save's roster (256: where they lie, looted;
  the place's own corpses untouched are not kept). Room made for them: the land's animals give way. MAXE back to
  100 (76 at the start, 93 in the temple); save version 16. The towns' people with their days, the merchants, the
  inns' and Dragonsreach's people stay always out (to be grouped as places come). host/stage tests it.
  The Rift's trees: their far form past 8 m (was 16) and plainer (aspens 10-12 triangles, pines 18): rift forest
  6.9 -> 8.7 fps, east 7.8 -> 10.1 (leaf fill - half_texk1 - is a quarter of the frame).
- v0.4.68: Forelhost (the user's forelhost export: tools/skyrim_area.py forelhost - navmesh, people, dialogue): its
  three halls from Skyrim's own pieces (gen_kit AREA=fh -> data/forelhost.har, area 3; floors and walls its
  navmeshes'; host/pvs 3 for what can be seen) - CELL_FHSTRONG (Forelhost01, Stronghold), CELL_FHCRYPT (02),
  CELL_FHREFECT (03, Refectory); the ruin on its mountain (ForelhostOutside in the main area, the riften export's
  Tamriel pieces round its door; gen_kit ruin_outside(), Bleak Falls' too); its four doors where Skyrim has them
  (GameForelhostDoors: the mountain door, Stronghold - Crypt, Crypt - Refectory, the Refectory's great door onto the
  top); its working parts from docs/maps/gen_dun_kit.py (any ruin; src/fh_kit.inc): the chests, the levers and the
  portcullises they work, the claw door (the Glass Claw: Fox, Owl, Snake - the puzzle generalised, iPuzDoor), the
  Glass Claw on its pedestal, the draugr asleep in alcoves and sarcophagi, the Crypt's boss (Draugr Wight, pad1 5),
  the cult's ghosts (pad1 3), Rahgot (pad1 4: his mask - IT_RAHGOT, heavy helmet 23 / 9 / 962, its +70 stamina not
  yet), skeevers, the dead; the word wall on the top teaches STRUN (Storm Call - its shout not yet). State in
  GameState::dun (FH_*), save version 17; staging groups G_FH*. Shared models: only those a town shares go to the
  library (tools/lib_pack.py RUINS) - the ruins keep their halls' pieces; MAXKIT 1500. host/fhquest walks it.
- v0.4.69: Siege on the Dragon Cult (src/game_fhquest.cpp; Skyrim's lines and journal entries from the forelhost
  export's dialogue.csv/quests.csv via docs/maps/gen_fh_quest.py -> fh_quest.inc): Valmir (named 53) in Forelhost's
  courtyard 17 m from its door calls out (dunForelhostForceGreetIntro) and tells his mission (stage 10); Skorm
  Snow-Strider's journal in the Stronghold (20), shown to him he reads it and keeps it (30: the well's key in the
  crypt's chest by the Draugr Wight); the well (PT_WELL, the crypt's well marker) takes you with its key to the
  Refectory's marker (40); Rahgot's mask (60): Valmir on the battlements in Stormcloak armour luring a soldier (named
  54) - the scene's lines - found out he fights (70; E_BANDIT pad1 2); killed: done (200). Stages are the game's own
  numbering by Skyrim's objectives; FH_QUEST in iS.dun; staging group G_FHOUT (200 m round the door) respawns by stage.
  The key's name ("Forelhost Well Key") is the game's guess - Skyrim creates it by script; the staff reward is not
  in (no staves). Save version 18. host/fhsiege walks it.
- v0.4.70: Faster where much is built (user 15:08: "try more ARM assembly, do your best"). The custom statics of each
  pass gathered, then drawn nearest first (counting sort by metre to their nearest side; R3D::DrawStatic); while they
  are drawn the occlusion map is rebuilt from the depths so far (after the 16th, 48th, 128th piece, only where 500 /
  1000 triangles have gone by), so what the near ones hide is left out whole (clusters, triangles); the occlusion map
  built in ARM code (spans.s occ_build: two lanes compared as whole words, a block left at its first empty row); the
  textured span loops branch past the colour work where the depth test fails (the cut-out ones keep their
  conditions - leaves are mostly in front); a static under 8 pixels across its radius uses its far model, under 20 its
  middle one. Emulator: city approach 5.1 -> 6.5, market 5.1 -> 5.2, Wind District 7.7 -> 8.0, stables yard 5.7 -> 5.9.
  Tried and dropped: plainer models for the towns' small pieces (more pieces passed the distances: stables 5.9 -> 5.1),
  the slots' pictures decoded once a model (no gain).
- v0.4.71: Forelhost as Skyrim has it (user 18:55) and faster still. Siege on the Dragon Cult uses Skyrim's own stage
  numbers (dunForelhostQST: 10 started, 20 the journal found, 26 shown to Valmir, 40 down the well, 50 Rahgot fallen -
  on his death, 60 his mask taken, 70 found out, 200 done; KStageObj maps them to the objectives); at the end the
  Stormcloak soldier (named 54, E_BANDIT pad1 3) fights beside Valmir and both must die. Rahgot's Dragon Priest Staff
  (IT_RAHGOT_STAFF, Skyrim's 1570 gold / 8 weight): swung, it casts Wall of Flames - five flames across the way 4 m
  ahead, 30 s, 50 fire damage a second to what stands in them, ten walls at most (Game::iFireWall, not saved; no charge
  yet - no soul gems). The well key's name is still the game's guess: tools/skyrim_area.py now exports what quests
  create (QUST ALCO -> quests.csv created, created_name) and the books' texts (books.csv); the user's re-export will give
  it. Speed (emulator, the same images to the pixel): the far land looks only at each row's part within its ring and
  trims the ends wholly out of sight, quads drawn only where both rows were worked out; textured triangles set up by
  R3D::RasterTex (no copies, w/u/v by name, RasterFlat's edge scheme); perspective rows drawn whole in ARM
  (spans.s persp_tex*: runs of up to 16-64 pixels, u/v exact at each end, no call a run); a slot's picture byte decoded
  once a frame (R3D::SlotInfo, iSlotInfo[256]); hidden pixels one instruction shorter in the textured walkers.
  Market 10.50 -> 9.94 M instructions a frame, approach 8.88 -> 8.50, stables 10.42 -> 9.65, barrow entry
  8.97 -> 8.46, Forelhost's Stronghold 9.05 -> 8.52, Riften's forest 6.59 -> 6.33.
- v0.4.72: Forelhost's books (the user's third forelhost export, 20:23, /home/claude/fh_in3: quests.csv now names what
  quests create - the well key is Skyrim's own "Forelhost Well Key" (DunForelhostWaterKey), so the name stays; books.csv
  the books' texts). docs/maps/gen_fh_books.py -> src/fh_books.inc: Skorm Snow-Strider's Journal now readable (IC_BOOK,
  8 pages), the cult's notes - Alchemist's Note (Froda), Rahgot's Reply, Orders, Note (the well poisoned), Note
  (Alduin's return) - The Exodus and Spell Tome: Flames, each where Skyrim leaves it (Refectory, Crypt); Skyrim's
  markup turned into the game's pages ([pagebreak] a page, the illuminated letter its letter, the rest dropped), pages
  filled as DrawWrapped lays them at 320 (44 a line, 14 lines), which now keeps '\n' as a line of its own. IM_NOTE: a
  sheet of paper (23 x 14 cm). Values / weights: notes 0 / 0, The Exodus and the tome 50 / 1, the journal 0 / 1 - not
  from the export (Skyrim's BOOK DATA): tools/skyrim_area.py now writes them to books.csv (value, weight) for the next.
  No spells or skills yet: the tome and the skill book are read only. host/fhbooks: each placed, opened, paged.
- v0.4.73: Faster again, in ARM (user 21:01: "continue with performance"), every scene identical to the pixel. The
  lights at a point (spans.s point_light) over a compact list of the lights near the mesh (R3D::SetLightList): 253 ->
  85 instructions a call. A textured triangle's set-up (tex_tri): sorted, premultiplied, the steps across, both halves'
  walker states filled whole (the second half's start the first's carried down - straight lines), C only calls
  RunBand (C fallback for triangles too big for the reciprocal table). Each face's picture / sharpness / light / fog /
  palette (face_shade) in batches of 64 records (iFaceRec), the C setting up the triangles from them; SlotInfo's table
  filled whole once a frame. RunHalfPersp's rows in ARM (persp_rows*: each row's clip, u/v at its start, run length,
  then the runs). Books from the user's export (fh_in4): The Exodus 55, Skorm's journal 5 (Skyrim's values).
  Emulator, M instructions a frame (v0.4.72 -> v0.4.73): stables 9.65 -> 8.49 (6.0 -> 6.8 fps), Forelhost's
  Stronghold 8.52 -> 7.74 (6.8 -> 7.5), barrow entry 8.46 -> 7.88, market 9.94 -> 9.61, approach 8.50 -> 8.36,
  tree rise 6.67 -> 6.37. emu/run_game.py prof now shows calls a frame and instructions a call.
  Fixed with it (latent since v0.4.71): SlotInfo's table is made again whenever the pictures it describes change
  (R3D::SlotInfoCheck: the areas' pictures are swapped in and out mid-frame) - a table made for another area's could
  choose a 256 picture that was not loaded (emu/check_align_ui case 0 crashed reading through a null picture).
- v0.4.74: Whiterun's floor: its destroyed/battle variants (children of the disabled markers 000FC90B, 000376E0) and
  the river's water-effect slabs (WR*River01ACT, Waterfall*, RapidRocks*) were drawn over the streets - gen_kit.py
  ref_on() follows enable-parent chains (refs.csv in every export folder; "enable opposite" honoured), WATERFX skipped
  (Whiterun 1,091 -> 1,033 models). Doors: the player set down clear of the landing (FreeSpot after EnterCell); the
  navmesh's edges across hinged doors' openings dropped (gen_kit); ruin doors are the game's hinged doors drawn with
  Skyrim's model (NorWoodDoor01, NorDoorSm02...), swung on their hinge. Stuck: when no way out (WaysOut 0) a step is
  allowed. Loading: DeferLoad/DoPendingLoad - the frame shows "Loading..." before an area or a cell is built (the
  phone; tests load at once). Map: T travels to the map's centre (any place - Riften's forest). Far land: 50, 100,
  150, 200, 300, 400 m steps (KFarM). Far models: under 8 px (5 for others) a building is a box of its far model's
  bounds and colours; Whiterun's and Riverwood's pieces merged into far groups (64 m squares, an 8 m grid, beyond
  120 m one mesh each): the city at 250 m 13.2 -> 9.40 M instructions, at 400 m 9.13 -> 6.40. Houses' light warm
  (R3D::SetLightColour 256/214/164 in houses; ruins and catacombs neutral - Skyrim's plaster WRPlasterInt01 is
  grey-green, its hearths make it warm): palettes, the flat faces (C and face_shade, FaceCtx.lr..lb at 184).
  Door prompts name the place the door opens onto (GameLoc), not the cell number's town.
  gen_kit's tex_tint keeps only the hue of Skyrim's average vertex colours (their darkness is baked shading: a cave's
  rocks multiplied by ~50 grey had drawn near black). Emulator, M instructions a frame (v0.4.73 -> v0.4.74): Stronghold
  7.74 -> 9.69 (its doors now Skyrim's models), barrow entry 7.88 -> 7.92, market 9.61 -> 9.63, stables 8.49 -> 8.52,
  the city from the plains at 250 m 13.2 -> 9.95, at 400 m 9.13 -> 6.77, at 600 m 4.03, facing away 4.6.
  Ruin doors hide what is behind them while shut (host/pvs draws them as occluders); open, the views from both
  sides of the door are joined (GameKitPvs, R3D::iPvsMix). Props aimed at: the nearest along the view, one only
  grazed up to a metre farther (a note beats the alchemy table it lies on).
  Review of janseris/symbian-s80-build-tools and symbian-s80v2-dev-tips-and-tricks (Nokia 9300 measurements from
  Maps 9300): a cache miss ~265 ns (~40 cycles, ~30 instructions); sequential reads no faster than random (~55 MB/s);
  writes don't allocate (a list written in one pass misses again when read); with small triangles each costs ~1,500
  cycles whatever its size, so the count matters more than the pixels; front to back with a bit a pixel rejected
  5,500 of 8,000 triangles before set-up; GCC 2.9: 16-bit C loads are byte pairs (we build with
  -mno-short-load-bytes), `T x[3] = {..}` calls memset, a fast path inside a big function pays its whole prologue;
  ARMv4: MUL/MLA Rd != Rm, SMULL's three registers different (tools/armv4_check.py: spans.s clean); the 15.6 ms
  tick (time 32 frames); User::Available walks the heap (we read it every 30 frames); copy only the view (done).
  Their emulator kit (armemu.py, asmcmp.py, a cache model) matches ours (emu/run_game.py, CACHE=1).
- v0.4.75 (tools): emu/rungame - the ARM emulator driver in C (Unicorn's C API: the per-block count and the cache
  model's per-read hook without a Python call each): the same numbers and pictures as emu/run_game.py, ~40x faster;
  ALIGN=all|ui|selftest replace emu/check_align_*.py. host/pvs works its points on all cores (THREADS=n), the same
  tables as one thread.
- Repo review, the rest read (Nov 5 21:20; every .md of both repos): checked against Hrimfell - the phone build is
  -O2 -mno-short-load-bytes (build.cfg) and the emulator's the same; the frame goes to the screen by direct screen
  access, a row at a time with Mem::Copy (the view's width only, from an even x); the biggest stack frames 2.1 KB
  (CUpdater::Headers) and 1.9 KB (DrawLocalMap) against the main thread's ~8 KB; r3d_memmove is a byte loop but not
  in any frame's top 25. For later: sound - mix everything into ONE PCM16 CMdaAudioOutputStream (two players at
  once: both die with KErrDied), IMA ADPCM 11 kHz for voices (~0 % CPU), MP3/AAC music decoded on the DSP (~0 %);
  keys - (not our case: Hrimfell never turns the virtual cursor on, and reads every key, Shift and Space too, by its own
  EEventKeyDown / EEventKeyUp scan codes - hrimfell_app.cpp KeyBit) with Maps' virtual cursor Space comes as a pointer
  toggle, and the EModifierShift flag stays on after Shift is let go; Chr is EModifierFunc, the joystick's scan codes
  170-174; timing on the phone - FastCounter and the tick are
  64 Hz: time 32 frames, A/B with a key in one run; saves - write the card in big pieces, never per event.
  The DSP (Maps 9300's 15 probes): ~3x the ARM at arithmetic, as fast at pixels from its own ~200 KB of RAM, but
  textures read from SDRAM cost 48 cycles a pixel and stall the ARM - drawing on it lost on a real frame; geometry
  in C55x assembly overlapped the ARM's drawing (~13 % ARM cost). Not for Hrimfell now (our pictures ~0.5 MB do not
  fit its RAM; needs TI's compiler and a node of our own).
- v0.4.75: interiors nearest first (as outside), the occlusion map made again among their pieces while it pays
  (judged frame by frame): Forelhost's Stronghold 9.74 -> 8.13 M instructions (6.0 -> 7.1 fps est.), the city from
  400 m 6.77 -> 6.38. The FPS shown averages whole frames over a second.
