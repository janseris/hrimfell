#!/usr/bin/env python3
# Riverwood, from Skyrim's own placements (tools/skyrim_places.py -> refs_area.csv, doors.csv): writes src/rw_ext.inc.
# Positions: centimetres from Riverwood's origin (RW_OX, RW_OZ engine metres; x east, z north); heights: Skyrim's own
# in centimetres (the engine adds its land offset); yaws in 1024ths of a turn (0 north, 256 east).
#   python3 docs/maps/gen_riverwood.py path/to/skyrim_places
import csv, math, os, re, sys

U = 0.0142875
P = sys.argv[1] if len(sys.argv) > 1 else 'skyrim_places'
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'src', 'rw_ext.inc')
OX, OZ = 3650, 1870                                   # engine metres
X0, X1, Z0, Z1 = 3585, 3712, 1810, 1965               # the area taken

def eng(x, y): return ((float(x) + 233472) * U, (float(y) + 176128) * U)
def cm(x, y):
    ex, ez = eng(x, y); return (int(round((ex - OX) * 100)), int(round((ez - OZ) * 100)))
def hcm(z): return int(round(float(z) * U * 100))
def yaw(d): return int(round(float(d) / 360.0 * 1024)) & 1023
def rot(lx, ly, deg):
    a = math.radians(float(deg)); return (lx * math.cos(a) + ly * math.sin(a), -lx * math.sin(a) + ly * math.cos(a))

rows = [r for r in csv.DictReader(open(os.path.join(P, 'refs_area.csv')))
        if X0 <= eng(r['x'], r['y'])[0] <= X1 and Z0 <= eng(r['x'], r['y'])[1] <= Z1]

# the buildings: house kind, centre of its bounds, its own yaw, half sizes and height (cm)
HOUSE = {'FarmInn01': 1, 'FarmhouseSmith01': 2, 'Farmhouse02': 3, 'Farmhouse04': 4, 'Farmhouse05': 5, 'Farmhouse06': 6,
         'FarmLumbermill01': 7, 'FarmLumbermill01WaterWheel01': 8, 'Bridge01': 9, 'Farmhouse02Walkway': 10, 'Farmhouse04Walkway': 10}
houses, walls, things = [], [], []
# small things: kind codes for game_riverwood.cpp
THING = [('BarrelFood', 1), ('Barrel', 1), ('FirewoodPile', 2), ('WoodPile', 2), ('HayMound', 3), ('HandCart', 4),
         ('CraftingBlackSmithForge', 5), ('CraftingBlacksmithArmorWorkbench', 6), ('CraftingBlacksmithSharpeningWheel', 7),
         ('CraftingTanningRack', 8), ('WeaponRackMid', 9), ('WoodChoppingBlock', 10), ('FarmBench', 11), ('CommonChair', 11),
         ('FarmTable', 12), ('ExteriorWoodenTable', 12), ('SignRiverwood', 13), ('Campfire01', 14), ('BeeHive', 15),
         ('HorseTrough', 16), ('FishRack', 17), ('CraftingCookingPot', 18), ('Bedroll', 19)]
WALL = [('WalkwayCWallGate', 1), ('WalkwayCWallEnd', 2), ('WalkwayCEntWall', 2), ('WalkwayCWall', 2), ('StonewallEnd', 3),
        ('Stonewall', 3), ('FenceWoven', 4), ('Walkway', 5)]
for r in rows:
    e = r['base_editor_id']; b = [int(v) for v in r['bounds'].split()] if r['bounds'] else None
    x, y, z, rz = r['x'], r['y'], r['z'], r['rot_z']
    if e in HOUSE and b:
        cx, cy = rot((b[0] + b[3]) / 2.0, (b[1] + b[4]) / 2.0, rz)
        X, Z = cm(float(x) + cx, float(y) + cy)
        # base: the building's own origin (its ground floor); top above it; foot: how far its footing goes down
        houses.append((HOUSE[e], X, Z, hcm(z), yaw(rz), int((b[3] - b[0]) / 2 * U * 100), int((b[4] - b[1]) / 2 * U * 100),
                       int(b[5] * U * 100), int(-b[2] * U * 100)))
        continue
    k = next((v for p, v in WALL if e.startswith(p)), 0)
    if k and b and r['base_type'] == 'STAT' and not e.startswith('WalkwayStairs'):
        cx, cy = rot((b[0] + b[3]) / 2.0, (b[1] + b[4]) / 2.0, rz)
        X, Z = cm(float(x) + cx, float(y) + cy)
        walls.append((k, X, Z, hcm(float(z) + b[2]), yaw(rz), int((b[3] - b[0]) / 2 * U * 100), int((b[4] - b[1]) / 2 * U * 100),
                      int((b[5] - b[2]) * U * 100)))
        continue
    k = next((v for p, v in THING if e.startswith(p)), 0)
    if k:
        X, Z = cm(x, y); things.append((k, X, Z, hcm(z), yaw(rz)))

doors = list(csv.DictReader(open(os.path.join(P, 'doors.csv'))))
NAMES = {'RiverwoodRiverwoodTrader': 'Riverwood Trader', 'RiverwoodSleepingGiantInn': 'Sleeping Giant Inn',
         'RiverwoodAlvorsHouse': "Alvor and Sigrid's House", 'RiverwoodGerdursHouse': "Gerdur's House",
         'RiverwoodFaendalsHouse': "Faendal's House", 'RiverwoodSvensHouse': "Sven and Hilde's House"}
out_doors = []
for d in doors:
    if d['world'] == 'Tamriel' and d['dest_cell'] in NAMES:
        X, Z = cm(d['x'], d['y'])
        if not (X0 <= eng(d['x'], d['y'])[0] <= X1): continue
        # where you come out: the house door's own destination (in Tamriel), else in front of the door
        back = sorted([b for b in doors if b['cell'] == d['dest_cell'] and b['dest_world'] == 'Tamriel'],
                      key=lambda b: (float(b['dest_x']) - float(d['x'])) ** 2 + (float(b['dest_y']) - float(d['y'])) ** 2 + (float(b['dest_z']) - float(d['z'])) ** 2)[:1]
        if back: OXc, OZc = cm(back[0]['dest_x'], back[0]['dest_y']); oy = yaw(back[0]['dest_rot_z']); oh = hcm(back[0]['dest_z'])
        else: OXc, OZc, oy, oh = X, Z, yaw(d['rot_z']), hcm(d['z'])
        out_doors.append((X, Z, hcm(d['z']), yaw(d['rot_z']), OXc, OZc, oh, oy, list(NAMES).index(d['dest_cell'])))

with open(OUT, 'w') as f:
    f.write('// generated by docs/maps/gen_riverwood.py from Skyrim.esm (tools/skyrim_places.py) - do not edit by hand\n')
    f.write('// cm from (RW_OX, RW_OZ) engine metres; heights: Skyrim cm (add the land offset); yaws 0..1023 (0 north)\n')
    f.write('enum { RW_OX = %d, RW_OZ = %d };\n' % (OX, OZ))
    def arr(name, rows, n, what):
        f.write('// %s\nenum { %s_N = %d };\nstatic const int %s[%d] = {\n' % (what, name, len(rows), name, max(1, len(rows) * n)))
        for r in rows: f.write('    ' + ', '.join(str(v) for v in r) + ',\n')
        if not rows: f.write('    0\n')
        f.write('    };\n')
    arr('KRwHouse', houses, 9, 'buildings: kind (1 inn, 2 smithy house, 3..6 houses, 7 mill, 8 water wheel, 9 bridge, 10 a porch boardwalk), x, z (middle of its bounds), base (its origin), yaw, half x, half z, top above the base, footing below it')
    arr('KRwWall', walls, 8, 'walls: kind (1 gate, 2 palisade, 3 stone wall, 4 woven fence, 5 walkway), x, z, base, yaw, half x, half z, height')
    arr('KRwThing', things, 5, 'things: kind (1 barrel, 2 firewood, 3 hay, 4 cart, 5 forge, 6 workbench, 7 grindstone, 8 tanning rack, 9 weapon rack, 10 chopping block, 11 bench, 12 table, 13 sign, 14 campfire, 15 beehive, 16 trough, 17 fish rack, 18 cooking pot, 19 bedroll), x, z, height, yaw')
    arr('KRwDoor', out_doors, 9, "doors: x, z, height, yaw; where you come out: x, z, height, facing; which house (0 Trader, 1 Inn, 2 Alvor's, 3 Gerdur's, 4 Faendal's, 5 Sven's)")
print('houses %d walls %d things %d doors %d' % (len(houses), len(walls), len(things), len(out_doors)))
