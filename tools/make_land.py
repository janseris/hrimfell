#!/usr/bin/env python3
# Turn the export of skyrim_heightmap.py (tamriel_export/) into Hrimfell's land file (data/hrimfell.land).
#   python3 tools/make_land.py path/to/tamriel_export [path/to/skyrim_places [path/to/skyrim_meshes]]
# (with the meshes: Skyrim's roads - its RoadChunk / RoadStraight ... pieces laid on the land - painted into the
#  ground where their paving lies)
#
# The game's land is Skyrim's at its true size: a vertex every 4 m (Skyrim's are 1.83 m apart, so the heights are
# sampled bilinearly), 1744 x 1376 cells = 6976 x 5504 m, x east, z north. Heights are Skyrim's in metres plus
# OFFSET_M, so the sea (-14000 game units = -200 m) lands on the engine's water level of 6 m.
#
# File (little-endian):
#   "HLND", u32 version 1, u32 gx, u32 gz (cells), i32 offset (cm)
#   heights: (gz + 1) rows from the south, (gx + 1) values each, in engine steps (1/32 m); stored as the
#            difference from the prediction left + below - below-left; one signed byte, or -128 then an int16
#   materials: TM_* per vertex, run-length coded: (count - 1, value) byte pairs
#   water: per chunk of 8 x 8 cells, int16 engine steps, -32768 = none; (gz / 8) rows of (gx / 8)
import os, sys, struct, csv
import numpy as np

GX, GZ, CHUNK = 1744, 1376, 8
CELL_M = 4.0
UNIT_M = 0.0142875               # one Skyrim game unit in metres
SRC_STEP = 128 * UNIT_M          # Skyrim vertex spacing (1.8288 m)
OFFSET_M = 206.0                 # Skyrim height + OFFSET_M = engine height (sea -200 m -> 6 m)
HS_PER_M = 32                    # engine steps per metre (METER 256 / HS 8)
CELL_X0, CELL_Y0, CELL_Y1 = -57, -43, 50

# Hrimfell ground materials (inc/r3d.h TM_*)
TM = dict(GRASS=0, FOREST=1, ROCK=2, SNOW=3, SAND=4, DIRT=5, BARROW=6, COBBLE=7, FIELD=8,
          TUNDRA=9, FALL=10, REACH=11, MARSH=12, ASH=13, ROAD=14)

def paint_roads(mat, places, meshes):
    # every road piece placed in the exported area (refs_area.csv): its paved faces (Road01), turned and placed, laid
    # on the land's vertices; the vertices inside become road (and lose their tree or rock)
    import re
    objs = {}
    def faces(model):
        if model in objs: return objs[model]
        stem = re.sub(r'[^A-Za-z0-9_.-]', '_', model.replace('\\', '_').replace('/', '_'))[:-4]
        fn = os.path.join(meshes, 'meshes', stem + '.obj'); V = []; T = []; cur = ''
        if os.path.exists(fn):
            for l in open(fn, encoding='latin-1'):
                if l.startswith('v '): V.append(tuple(float(x) for x in l.split()[1:4]))
                elif l.startswith('usemtl'): cur = l.split(None, 1)[1].strip().lower()
                elif l.startswith('f ') and 'roads' in cur:
                    T.append(tuple(V[int(q.split('/')[0]) - 1] for q in l.split()[1:4]))
        objs[model] = T; return T
    models = {}
    for r in csv.DictReader(open(os.path.join(meshes, 'models.csv'), encoding='utf-8')):
        for e in r['objects'].split(';'): models[e] = r['model']
    n = 0; marked = np.zeros(mat.shape, bool)
    for r in csv.DictReader(open(os.path.join(places, 'refs_area.csv'), encoding='utf-8')):
        e = r['base_editor_id']
        if r['world'] != 'Tamriel' or not e.startswith('Road') or e.startswith('RoadSign') or e not in models: continue
        T = faces(models[e])
        if not T: continue
        x, y, rz, sc = float(r['x']), float(r['y']), np.radians(float(r['rot_z'])), float(r['scale'] or 1)
        ca, sa = np.cos(rz), np.sin(rz); n += 1
        for A, B, C in T:
            P = [((x + sc * (p[0] * ca + p[1] * sa)) - CELL_X0 * 4096) * UNIT_M / CELL_M for p in (A, B, C)]
            Q = [((y + sc * (-p[0] * sa + p[1] * ca)) - CELL_Y0 * 4096) * UNIT_M / CELL_M for p in (A, B, C)]
            # points over the face a metre apart (a quarter of a vertex step), each marking its nearest vertex: a
            # road leaves an unbroken line of road vertices however narrow it is
            L = max(abs(P[1] - P[0]) + abs(Q[1] - Q[0]), abs(P[2] - P[0]) + abs(Q[2] - Q[0]), 0.25)
            k = int(L * 4) + 2
            u, v = np.meshgrid(np.linspace(0, 1, k), np.linspace(0, 1, k)); ok = u + v <= 1
            u, v = u[ok], v[ok]
            px = P[0] + u * (P[1] - P[0]) + v * (P[2] - P[0]); pz = Q[0] + u * (Q[1] - Q[0]) + v * (Q[2] - Q[0])
            ii = np.clip(np.round(px).astype(int), 0, GX); jj = np.clip(np.round(pz).astype(int), 0, GZ)
            marked[jj, ii] = True
    mat[marked] = TM['ROAD']
    print('roads: %d pieces, %d vertices' % (n, marked.sum()))
    return mat

def material(name):
    n = name.lower()
    if 'snowcobble' in n: return TM['COBBLE']
    if 'ice' in n or 'snow' in n: return TM['SNOW']
    if 'coast' in n or 'beach' in n: return TM['SAND']
    if 'river' in n or 'mud' in n: return TM['DIRT']
    if 'volcanic' in n: return TM['ASH']
    if 'fallforest' in n: return TM['FALL']
    if 'reach' in n: return TM['REACH']
    if 'frozenmarsh' in n: return TM['MARSH']
    if 'pineforest' in n: return TM['FOREST']
    if 'tundra' in n: return TM['TUNDRA']
    if 'dirt' in n or 'path' in n or 'road' in n: return TM['DIRT']
    if 'rock' in n or 'cliff' in n: return TM['ROCK']
    if 'grass' in n or 'field' in n: return TM['GRASS']
    return TM['TUNDRA']

def main():
    src = sys.argv[1] if len(sys.argv) > 1 else 'tamriel_export'
    places = sys.argv[2] if len(sys.argv) > 2 else None
    info = open(os.path.join(src, 'tamriel_info.txt')).read()
    W, H = 3809, 3009
    a = np.fromfile(os.path.join(src, 'tamriel_height.raw'), dtype='<i2').reshape(H, W).astype(np.float64) * 8 * UNIT_M
    a = a[::-1]                                   # row 0 = south
    # engine vertex (gx, gz) -> source (col, row) from the south-west corner
    gx = np.arange(GX + 1) * CELL_M / SRC_STEP
    gz = np.arange(GZ + 1) * CELL_M / SRC_STEP
    c0 = np.clip(np.floor(gx).astype(int), 0, W - 2); fx = np.clip(gx - c0, 0, 1)
    r0 = np.clip(np.floor(gz).astype(int), 0, H - 2); fz = np.clip(gz - r0, 0, 1)
    A = a[r0][:, c0]; B = a[r0][:, c0 + 1]; C = a[r0 + 1][:, c0]; D = a[r0 + 1][:, c0 + 1]
    FX = fx[None, :]; FZ = fz[:, None]
    h = (A * (1 - FX) + B * FX) * (1 - FZ) + (C * (1 - FX) + D * FX) * FZ
    hs = np.clip(np.round((h + OFFSET_M) * HS_PER_M), -32000, 32000).astype(np.int32)
    print('heights %.1f .. %.1f m (engine %d .. %d steps)' % (h.min(), h.max(), hs.min(), hs.max()))

    # ground: the per-vertex map if skyrim_places sent one, else the quarter-cell map with ragged borders
    if places and os.path.exists(os.path.join(places, 'ground_full.pgm')):
        names = [l.split(' ', 1)[1].strip() for l in open(os.path.join(places, 'ground_full.txt'))]
        raw = open(os.path.join(places, 'ground_full.pgm'), 'rb').read()
        g = np.frombuffer(raw[raw.index(b'255\n') + 4:], dtype=np.uint8).reshape(H, W)[::-1]
        gsx = gx; gsz = gz
    else:
        names = [l.split(' ', 1)[1].strip() for l in open(os.path.join(src, 'tamriel_ground.txt'))]
        raw = open(os.path.join(src, 'tamriel_ground.pgm'), 'rb').read()
        gw, gh = 238, 188
        g = np.frombuffer(raw[raw.index(b'255\n') + 4:], dtype=np.uint8).reshape(gh, gw)[::-1]
        gsx = gx / 16.0; gsz = gz / 16.0           # 16 source vertices per quarter cell
    lut = np.array([material(n) for n in names], dtype=np.uint8)
    rng = np.random.RandomState(7)
    jx = rng.uniform(-0.55, 0.55, (GZ + 1, GX + 1)) if g.shape[0] < 1000 else 0.5
    jz = rng.uniform(-0.55, 0.55, (GZ + 1, GX + 1)) if g.shape[0] < 1000 else 0.5
    ix = np.clip(np.floor(gsx[None, :] + jx).astype(int), 0, g.shape[1] - 1)
    iz = np.clip(np.floor(gsz[:, None] + jz).astype(int), 0, g.shape[0] - 1)
    mat = lut[g[iz, ix]]
    mat[g[iz, ix] == 0] = TM['ROCK']               # no texture: the land beyond the borders
    print('materials:', {k: int((mat == v).sum()) for k, v in TM.items()})
    if places and len(sys.argv) > 3: mat = paint_roads(mat, places, sys.argv[3])
    # water: the cell's own level, else the sea
    wl = {}
    for r in csv.DictReader(open(os.path.join(src, 'tamriel_water.csv'))):
        wl[(int(r['cell_x']), int(r['cell_y']))] = float(r['water'])
    ncx, ncz = GX // CHUNK, GZ // CHUNK
    water = np.full((ncz, ncx), -32768, dtype=np.int32)
    for cz in range(ncz):
        for cx in range(ncx):
            xm = (cx + 0.5) * CHUNK * CELL_M; zm = (cz + 0.5) * CHUNK * CELL_M
            sx = int(np.floor(xm / (32 * SRC_STEP))) + CELL_X0
            sy = int(np.floor(zm / (32 * SRC_STEP))) + CELL_Y0
            lev = wl.get((sx, sy), -14000.0) * UNIT_M
            water[cz, cx] = int(round((lev + OFFSET_M) * HS_PER_M))
    # a chunk whose land is all above its water has none
    for cz in range(ncz):
        for cx in range(ncx):
            blk = hs[cz * CHUNK:cz * CHUNK + CHUNK + 1, cx * CHUNK:cx * CHUNK + CHUNK + 1]
            if blk.min() >= water[cz, cx]: water[cz, cx] = -32768
    print('chunks with water: %d of %d' % ((water != -32768).sum(), ncx * ncz))

    # trees and rocks where Skyrim has them (skyrim_places: trees.pgm / rocks.pgm, counts per 7.3 m square):
    # a vertex is marked with the chance that leaves as many marks as Skyrim has things there
    if places and os.path.exists(os.path.join(places, 'trees.pgm')):
        def dens(fn):
            raw = open(os.path.join(places, fn), 'rb').read()
            d = np.frombuffer(raw[raw.index(b'255\n') + 4:], dtype=np.uint8).reshape(752, 952)[::-1].astype(float)
            sq = 32 * SRC_STEP / 8                      # 7.31 m
            ix = np.clip((np.arange(GX + 1) * CELL_M / sq).astype(int), 0, 951)
            iz = np.clip((np.arange(GZ + 1) * CELL_M / sq).astype(int), 0, 751)
            per = (sq / CELL_M) ** 2                    # vertices per square
            return d[iz][:, ix] / per
        # (trees.pgm counts every TREE record: in the Whiterun - Riverwood area 29 % of them are trees, the rest
        #  shrubs, ferns and flowers)
        # the share of real trees among them, by ground (measured in that area; the Rift, the Reach, the marsh
        # and the ash lands guessed)
        share = np.array([0.16, 0.60, 0.33, 0.56, 0.10, 0.14, 0.2, 0.1, 0.1, 0.10, 0.5, 0.3, 0.3, 0.2, 0.2, 0.2])
        pt = dens('trees.pgm') * share[mat & 15]
        pr = dens('rocks.pgm') * 0.5
        rt = rng.uniform(0, 1, pt.shape); rr = rng.uniform(0, 1, pr.shape)
        wv = np.kron(water, np.ones((CHUNK, CHUNK), dtype=np.int32))
        wv = np.pad(wv, ((0, GZ + 1 - wv.shape[0]), (0, GX + 1 - wv.shape[1])), mode='edge')
        tree = (rt < pt) & (hs > wv + HS_PER_M) & ((mat & 15) != TM['SAND'])
        rock = (rr < pr) & ~tree
        if len(sys.argv) > 3: tree &= (mat & 15) != TM['ROAD']; rock &= (mat & 15) != TM['ROAD']
        mat = mat | (tree.astype(np.uint8) << 6) | (rock.astype(np.uint8) << 7)
        print('trees %d, rocks %d' % (tree.sum(), rock.sum()))


    out = bytearray(b'HLND' + struct.pack('<IIIi', 1, GX, GZ, int(OFFSET_M * 100)))
    # heights, predicted
    P = np.zeros_like(hs)
    P[0, 1:] = hs[0, :-1]
    P[1:, 0] = hs[:-1, 0]
    P[1:, 1:] = hs[1:, :-1] + hs[:-1, 1:] - hs[:-1, :-1]
    res = (hs - P).ravel()
    small = (res > -128) & (res < 128)
    hb = bytearray()
    for v, s in zip(res.tolist(), small.tolist()):
        if s: hb.append(v & 255)
        else: hb.append(128); hb += struct.pack('<h', v)
    out += struct.pack('<I', len(hb)) + hb
    print('heights: %d bytes (%.2f per vertex, %d escapes)' % (len(hb), len(hb) / res.size, (~small).sum()))
    # materials, run-length
    m = mat.ravel().tolist()
    mb = bytearray(); i = 0
    while i < len(m):
        j = i
        while j < len(m) and m[j] == m[i] and j - i < 256: j += 1
        mb.append(j - i - 1); mb.append(m[i]); i = j
    out += struct.pack('<I', len(mb)) + mb
    print('materials: %d bytes' % len(mb))
    out += struct.pack('<I', water.size * 2) + water.astype('<i2').tobytes()
    dst = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'data', 'hrimfell_v1.land')     # (host/bakeland makes the game's from it)
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    open(dst, 'wb').write(out)
    import zlib
    print('wrote %s: %d bytes (%d zipped)' % (dst, len(out), len(zlib.compress(bytes(out), 9))))
    # a picture to check by eye: the materials lit by the heights, water blue, north up
    from PIL import Image
    cols = np.array([[78,116,48],[54,86,40],[112,108,102],[232,238,248],[186,170,118],[124,98,64],[86,80,66],
                     [132,128,120],[188,164,84],[150,140,80],[150,104,48],[122,100,70],[104,108,82],[96,90,84],[104,100,88]], float)
    img = cols[mat & 15]
    gyy, gxx = np.gradient(hs.astype(float))
    img *= np.clip(1 + (gxx - gyy) / 40.0, 0.4, 1.6)[..., None]
    wv = np.kron(water, np.ones((CHUNK, CHUNK)))[:GZ + 1, :GX + 1]
    wv = np.pad(wv, ((0, GZ + 1 - wv.shape[0]), (0, GX + 1 - wv.shape[1])), constant_values=-32768)
    img[hs < wv] = [44, 84, 124]
    Image.fromarray(np.clip(img[::-1], 0, 255).astype(np.uint8)).save(dst.replace('.land', '_land.png'))
    # the world map's close-up (the game's map zoomed past its own picture): every vertex a byte, rows from the
    # north; a byte is the ground (mat * 16) and its light along the slope (0..15), trees a shade darker, water
    # 240..247 by depth. "HMAP", u32 w, u32 h, 256 * r, g, b, then the rows (data/hrimfell_map.bin)
    sh = np.clip(np.round((np.clip(1 + (gxx - gyy) / 40.0, 0.4, 1.6) - 0.4) / 1.2 * 15), 0, 15).astype(int)
    sh = np.where((mat >> 6) & 1, np.maximum(sh - 4, 0), sh)
    idx = (np.minimum(mat & 15, 14).astype(int) * 16 + sh)
    depth = np.clip((wv - hs) // (HS_PER_M * 2), 0, 7)
    idx = np.where(hs < wv, 240 + depth, idx).astype(np.uint8)
    pal = []
    for m in range(15):
        for k in range(16): pal += [int(min(255, c * (0.4 + k * 1.2 / 15))) for c in cols[m]]
    for k in range(16): pal += [max(0, 60 - k * 5), max(0, 100 - k * 7), max(0, 140 - k * 7)]
    mp = bytearray(b'HMAP' + struct.pack('<II', GX + 1, GZ + 1)) + bytes(pal[:768]) + idx[::-1].tobytes()
    open(os.path.join(os.path.dirname(dst), 'hrimfell_map.bin'), 'wb').write(mp)
    print('map close-up: %d bytes' % len(mp))

if __name__ == '__main__':
    main()
