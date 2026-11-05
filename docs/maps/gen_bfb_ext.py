#!/usr/bin/env python3
# Bleak Falls Barrow's outside, from Skyrim's own placements (tools/skyrim_places.py -> refs_area.csv, doors.csv):
# writes src/bfb_ext.inc. Positions are centimetres from the barrow's main door (engine x east, z north), heights
# centimetres above the door's sill, yaws in the engine's 1024 to a turn (0 north, 256 east).
#   python3 docs/maps/gen_bfb_ext.py path/to/skyrim_places
import csv, math, os, sys

U = 0.0142875                       # metres per Skyrim game unit
P = sys.argv[1] if len(sys.argv) > 1 else 'skyrim_places'
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'src', 'bfb_ext.inc')

doors = list(csv.DictReader(open(os.path.join(P, 'doors.csv'))))
main = [d for d in doors if d['world'] == 'Tamriel' and d['dest_cell'] == 'BleakFallsBarrow01'][0]
exitd = [d for d in doors if d['world'] == 'Tamriel' and d['dest_cell'] == 'BleakFallsBarrow02'][0]
back = [d for d in doors if d['cell'] == 'BleakFallsBarrow02' and d['dest_world'] == 'Tamriel'][0]
DX, DY, DZ = float(main['x']), float(main['y']), float(main['z'])
DYAW = float(main['rot_z'])

def eng(x, y): return ((x + 233472) * U, (y + 176128) * U)             # engine metres
def cm(x, y): return (int(round((x - DX) * U * 100)), int(round((y - DY) * U * 100)))
def hcm(z): return int(round((z - DZ) * U * 100))
def yaw(deg): return int(round(deg / 360.0 * 1024)) & 1023
def rot(lx, ly, deg):
    # a Skyrim piece's local point, turned clockwise by its heading (local +y = its heading)
    a = math.radians(deg); return (lx * math.cos(a) + ly * math.sin(a), -lx * math.sin(a) + ly * math.cos(a))

rows = [r for r in csv.DictReader(open(os.path.join(P, 'refs_area.csv')))
        if (float(r['x']) - DX) ** 2 + (float(r['y']) - DY) ** 2 < (190 / U) ** 2]

floors, stairs, walls, pillars, ribs, rubble, raised, bandits = [], [], [], [], [], [], [], []
tower = None
for r in rows:
    e = r['base_editor_id']; x, y, z = float(r['x']), float(r['y']), float(r['z']); rz = float(r['rot_z'])
    sc = float(r['scale']); b = [int(v) for v in r['bounds'].split()] if r['bounds'] else None
    def rect(x1, y1, x2, y2, top):
        # a floor: centre and half sizes in its own frame, turned
        cx, cy = rot((x1 + x2) / 2.0, (y1 + y2) / 2.0, rz)
        X, Y = cm(x + cx, y + cy)
        floors.append((X, Y, hcm(z + top), yaw(rz), int((x2 - x1) / 2 * U * 100), int((y2 - y1) / 2 * U * 100)))
    def wall(x1, y1, x2, y2, side):
        # a retaining wall along one side of a floor piece: its middle, its outward heading, its length
        if side == 'y1': mx, my, h, ln = (x1 + x2) / 2, y1, rz + 180, x2 - x1
        elif side == 'y2': mx, my, h, ln = (x1 + x2) / 2, y2, rz, x2 - x1
        elif side == 'x1': mx, my, h, ln = x1, (y1 + y2) / 2, rz - 90, y2 - y1
        else: mx, my, h, ln = x2, (y1 + y2) / 2, rz + 90, y2 - y1
        cx, cy = rot(mx, my, rz)
        X, Y = cm(x + cx, y + cy)
        walls.append((X, Y, hcm(z), yaw(h), int(ln * U * 100)))
    if not e.startswith(('NorTmpExt', 'NorTemple', 'NorTower', 'NorExtRubble', 'NorDragonStatue')) and r['base_type'] != 'NPC_':
        continue
    if e.startswith('NorTmpExtPlatMid'):
        rect(b[0], b[1], b[3], b[4], 0)
    elif e.startswith('NorTmpExtPlatWall'):
        rect(b[0], b[1], b[3], b[4], 0); wall(b[0], b[1], b[3], b[4], 'y1')
    elif e.startswith('NorTmpExtPlatCorOut'):
        rect(b[0], b[1], b[3], b[4], 0); wall(b[0], b[1], b[3], b[4], 'y1'); wall(b[0], b[1], b[3], b[4], 'x1')
    elif e.startswith('NorTmpExtPlatCorIn'):
        rect(b[0], b[1], b[3], b[4], 0)
    elif e.startswith('NorTmpExtPlatCorDblEnd'):
        rect(b[0], b[1], b[3], b[4], 0); wall(b[0], b[1], b[3], b[4], 'x1')
    elif e.startswith('NorTmpExtPlatExSmFree'):
        rect(b[0], b[1], b[3], b[4], b[5])
    elif e.startswith('NorTmpExtPlatFloorRaised'):
        cx, cy = rot((b[0] + b[3]) / 2.0, (b[1] + b[4]) / 2.0, rz); X, Y = cm(x + cx, y + cy)
        raised.append((X, Y, hcm(z), yaw(rz)))
    elif e.startswith(('NorTmpExtPlatStairMid', 'NorTmpExtPlatStairCap')):
        # down towards local -y: the top edge at y = +54 (height 0), the foot at y = -704 (height -512)
        if 'Cap' in e: continue                       # (the caps are the flight's side blocks: the wall boxes do)
        cx, cy = rot(0, 54, rz); X, Y = cm(x + cx, y + cy)
        stairs.append((X, Y, hcm(z), yaw(rz + 180), int(1024 * U * 100), int(758 * U * 100), int(512 * U * 100)))
    elif e.startswith('NorTmpExtStairsFree'):
        # a short flight along local x: treat as rising towards +y, the size of its bounds
        cx, cy = rot(0, b[4], rz); X, Y = cm(x + cx, y + cy)
        stairs.append((X, Y, hcm(z + b[5]), yaw(rz + 180), int((b[3] - b[0]) * U * 100), int((b[4] - b[1]) * U * 100), int((b[5] - b[2]) * U * 100)))
    elif e.startswith('NorTmpExtPlatTower0'):
        X, Y = cm(x, y); pillars.append((X, Y, hcm(z), yaw(rz), int(b[5] * U * 100)))
    elif e.startswith('NorTempleExteriorRib') and 'Base' not in e:
        X, Y = cm(x, y)
        w = (b[3] - b[0]) * sc * U; hgt = (b[5] - b[2]) * sc * U
        if w > 12: ribs.append((X, Y, hcm(z), yaw(rz), int(w * 100), int(hgt * 100)))
        else: pillars.append((X, Y, hcm(z), yaw(rz), int(hgt * 100)))
    elif e.startswith(('NorExtRubble', 'NorTmpExtDebris')):
        X, Y = cm(x, y); rubble.append((X, Y, hcm(z), yaw(rz), int(max(b[3] - b[0], b[4] - b[1]) * sc * U * 100)))
    elif e.startswith('NorDragonStatue'):
        X, Y = cm(x, y); pillars.append((X, Y, hcm(z), yaw(rz), int((b[5] - b[2]) * float(r['scale']) * U * 100)))
    elif e == 'NorTowerRuins01Snow':
        X, Y = cm(x, y); tower = (X, Y, hcm(z), yaw(rz))
    elif r['base_type'] == 'NPC_' and 'Bandit' in e:
        X, Y = cm(x, y); bandits.append((X, Y, hcm(z), yaw(rz)))

floors.sort(key=lambda f: f[2])                      # low first: where two meet, the higher one is laid last
ex, ey = cm(float(back['dest_x']), float(back['dest_y']))
hx, hy = cm(float(exitd['x']), float(exitd['y']))
mex, mez = eng(DX, DY)
with open(OUT, 'w') as f:
    f.write('// generated by docs/maps/gen_bfb_ext.py from Skyrim.esm (tools/skyrim_places.py) - do not edit by hand\n')
    f.write('// centimetres from the main door (x east, z north), heights above its sill, yaws 0..1023 (0 north)\n')
    f.write('enum { BFE_DOOR_XM = %d, BFE_DOOR_ZM = %d, BFE_DOOR_XCM = %d, BFE_DOOR_ZCM = %d, BFE_DOOR_H = %d, BFE_DOOR_YAW = %d };\n'
            % (int(mex), int(mez), int(round((mex - int(mex)) * 100)), int(round((mez - int(mez)) * 100)), int(round(DZ * U * 100)), yaw(DYAW)))
    f.write('// the way out of the sanctum: where you come out, facing; the hidden door behind you\n')
    f.write('enum { BFE_OUT_X = %d, BFE_OUT_Z = %d, BFE_OUT_H = %d, BFE_OUT_YAW = %d, BFE_HOLE_X = %d, BFE_HOLE_Z = %d };\n'
            % (ex, ey, hcm(float(back['dest_z'])), yaw(float(back['dest_rot_z'])), hx, hy))
    f.write('enum { BFE_TOWER_X = %d, BFE_TOWER_Z = %d, BFE_TOWER_H = %d, BFE_TOWER_YAW = %d };\n' % tower)
    def arr(name, rows, n, what):
        f.write('// %s\n' % what)
        f.write('enum { %s_N = %d };\nstatic const int %s[%d] = {\n' % (name, len(rows), name, max(1, len(rows) * n)))
        for r in rows: f.write('    ' + ', '.join(str(v) for v in r) + ',\n')
        if not rows: f.write('    0\n')
        f.write('    };\n')
    arr('KBfeFloor', floors, 6, 'floors (platform tops): x, z, top, yaw, half width (its x), half depth (its z)')
    arr('KBfeStair', stairs, 7, 'flights: top edge middle x, z, top height, yaw (the way down), width, run, drop')
    arr('KBfeWall', walls, 5, 'retaining walls: middle of the face x, z, top height, yaw (outwards), length')
    arr('KBfePillar', pillars, 5, 'posts and statues: x, z, foot height, yaw, height')
    arr('KBfeRib', ribs, 6, 'the great ribs: x, z, foot height, yaw, width, height')
    arr('KBfeRubble', rubble, 5, 'rubble: x, z, height, yaw, size')
    arr('KBfeRaised', raised, 4, 'raised floor slabs: x, z, height, yaw')
    arr('KBfeBandit', bandits, 4, 'bandits: x, z, height, yaw')
print('floors %d stairs %d walls %d pillars %d ribs %d rubble %d bandits %d' % (len(floors), len(stairs), len(walls), len(pillars), len(ribs), len(rubble), len(bandits)))
print('door at', eng(DX, DY), 'height', DZ * U, 'yaw', DYAW)
