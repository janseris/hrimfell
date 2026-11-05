#!/usr/bin/env python3
# (v0.4.57) A world's own land laid into the land file (version 1, tools/make_land.py's): where Skyrim gives a city its
# own land (WhiterunWorld's cells: the streets, the terraces), its heights and ground replace Tamriel's there, and no
# grown trees or rocks stand on it. The original is kept as data/hrimfell_v1_tamriel.land; host/bakeland then makes
# the game's file from the result.
#   python3 tools/patch_land.py /home/claude/whiterun_in4/skyrim_whiterun/land_WhiterunWorld.bin
import os, sys, struct, shutil
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from make_land import GX, GZ, CHUNK, CELL_M, UNIT_M, OFFSET_M, HS_PER_M, CELL_X0, CELL_Y0, material

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'data')
orig = os.path.join(ROOT, 'hrimfell_v1_tamriel.land')
dst = os.path.join(ROOT, 'hrimfell_v1.land')
if not os.path.exists(orig): shutil.copy(dst, orig)
b = open(orig, 'rb').read()
assert b[:4] == b'HLND' and struct.unpack_from('<I', b, 4)[0] == 1
gx, gz, off = struct.unpack_from('<IIi', b, 8)
assert (gx, gz) == (GX, GZ)
p = 20
nh = struct.unpack_from('<I', b, p)[0]; p += 4
hb = b[p:p + nh]; p += nh
res = []; i = 0
while i < len(hb):
    v = hb[i]
    if v == 128: res.append(struct.unpack_from('<h', hb, i + 1)[0]); i += 3
    else: res.append(v - 256 if v > 128 else v); i += 1
res = np.array(res, dtype=np.int64).reshape(GZ + 1, GX + 1)
hs = np.cumsum(np.cumsum(res, axis=0), axis=1)          # (each value: left + below - below-left + its own)
nm = struct.unpack_from('<I', b, p)[0]; p += 4
mb = b[p:p + nm]; p += nm
m = []
for k in range(0, len(mb), 2): m += [mb[k + 1]] * (mb[k] + 1)
mat = np.array(m, dtype=np.uint8).reshape(GZ + 1, GX + 1)
rest = b[p:]

# the world's own cells
for src in [a for a in sys.argv[1:] if a.endswith('.bin')]:
    lb = open(src, 'rb').read()
    names = [l.split(' ', 1)[1].strip() for l in open(src[:-4] + '.txt') if l[:1].isdigit()]
    lut = [material(n) for n in names]
    n, nt = struct.unpack_from('<II', lb, 4); q = 12; done = 0
    for _ in range(n):
        cx, cy = struct.unpack_from('<ii', lb, q); q += 8
        H = np.array(struct.unpack_from('<1089f', lb, q), dtype=np.float64).reshape(33, 33); q += 1089 * 4
        T = np.array(struct.unpack_from('<1089H', lb, q)).reshape(33, 33); q += 1089 * 2
        # the engine's vertices in this cell
        x0, y0 = cx * 4096, cy * 4096
        gxa = int(np.ceil((x0 - CELL_X0 * 4096) * UNIT_M / CELL_M)); gxb = int(np.floor((x0 + 4096 - CELL_X0 * 4096) * UNIT_M / CELL_M))
        gza = int(np.ceil((y0 - CELL_Y0 * 4096) * UNIT_M / CELL_M)); gzb = int(np.floor((y0 + 4096 - CELL_Y0 * 4096) * UNIT_M / CELL_M))
        for vz in range(max(0, gza), min(GZ, gzb) + 1):
            for vx in range(max(0, gxa), min(GX, gxb) + 1):
                sx = (CELL_X0 * 4096 + vx * CELL_M / UNIT_M - x0) / 128.0; sy = (CELL_Y0 * 4096 + vz * CELL_M / UNIT_M - y0) / 128.0
                c0 = min(31, max(0, int(sx))); r0 = min(31, max(0, int(sy))); fx = min(1, max(0, sx - c0)); fy = min(1, max(0, sy - r0))
                h = (H[r0, c0] * (1 - fx) + H[r0, c0 + 1] * fx) * (1 - fy) + (H[r0 + 1, c0] * (1 - fx) + H[r0 + 1, c0 + 1] * fx) * fy
                hs[vz, vx] = int(round((h * UNIT_M + OFFSET_M) * HS_PER_M))
                t = T[int(round(sy)) if round(sy) <= 32 else 32, int(round(sx)) if round(sx) <= 32 else 32]
                mat[vz, vx] = lut[t] if t < len(lut) else mat[vz, vx] & 15        # (no grown tree, no rock: bits 6, 7 off)
                done += 1
    print('%s: %d cells, %d vertices of the land replaced' % (os.path.basename(src), n, done))

# the city's cells without land of their own (Skyrim builds their ground from pieces: WRTerCloud, WRMainRoadPlains, the
# houses' bases): Tamriel's land there kept under the city's floor (its navmesh), half a metre below the lowest of it
# within 6 m, so no hill of it shows through the streets
import gzip
navs = sorted(set(a for a in sys.argv[1:] if a.endswith('.bin')))
for src in navs:
    d_ = os.path.dirname(src); world = os.path.basename(src)[5:-4]
    nf = os.path.join(d_, 'navmesh.txt.gz')
    if not os.path.exists(nf): continue
    have = set()
    lb = open(src, 'rb').read(); n = struct.unpack_from('<I', lb, 4)[0]; q = 12
    for _ in range(n):
        have.add(struct.unpack_from('<ii', lb, q)); q += 8 + 1089 * 6
    pts = []; cur = None
    for l in gzip.open(nf, 'rt'):
        w = l.split()
        if not w: continue
        if w[0] == 'N': cur = w[1] == world
        elif w[0] == 'v' and cur: pts.append((float(w[1]), float(w[2]), float(w[3])))
    grid = {}
    for x, y, z in pts:
        k = (int(x // 420), int(y // 420)); grid[k] = min(grid.get(k, 1e9), z)
    low = 0
    for (kx, ky), z in grid.items():
        pass
    xs = [p_[0] for p_ in pts]; ys = [p_[1] for p_ in pts]
    gxa = int((min(xs) - CELL_X0 * 4096) * UNIT_M / CELL_M); gxb = int((max(xs) - CELL_X0 * 4096) * UNIT_M / CELL_M) + 1
    gza = int((min(ys) - CELL_Y0 * 4096) * UNIT_M / CELL_M); gzb = int((max(ys) - CELL_Y0 * 4096) * UNIT_M / CELL_M) + 1
    for vz in range(max(0, gza), min(GZ, gzb) + 1):
        for vx in range(max(0, gxa), min(GX, gxb) + 1):
            sx = CELL_X0 * 4096 + vx * CELL_M / UNIT_M; sy = CELL_Y0 * 4096 + vz * CELL_M / UNIT_M
            if (int(sx // 4096), int(sy // 4096)) in have: continue
            kx, ky = int(sx // 420), int(sy // 420)
            zs = [grid[(kx + a, ky + b)] for a in (-1, 0, 1) for b in (-1, 0, 1) if (kx + a, ky + b) in grid]
            if not zs: continue
            top = int(round(((min(zs) - 35) * UNIT_M + OFFSET_M) * HS_PER_M))
            if hs[vz, vx] > top: hs[vz, vx] = top; mat[vz, vx] &= 15; low += 1
    print('%s: %d vertices of Tamriel\'s land kept under its floor' % (world, low))

# (v0.4.66) an export with the land's every placement (a folder: tools/skyrim_area.py's, e.g. riften - its area.txt's
# Tamriel cells): its trees are placed as Skyrim has them (gen_kit.py: the plants' file), so none grown in its cells
import re as _re
for src in sys.argv[1:]:
    fa = os.path.join(src, 'area.txt')
    if not os.path.isdir(src) or not os.path.exists(fa): continue
    m_ = _re.search(r'tamriel cells x (-?\d+)\.\.(-?\d+), y (-?\d+)\.\.(-?\d+)', open(fa).read())
    if not m_: continue
    x0, x1, y0, y1 = (int(v) for v in m_.groups())
    if not (x0 > 14 or x1 < -8 or y0 > 4 or y1 < -22): continue          # (Whiterun's hold: its trees stay the land's)
    gxa = max(0, int(np.ceil((x0 - CELL_X0) * 4096 * UNIT_M / CELL_M))); gxb = min(GX, int(np.floor((x1 + 1 - CELL_X0) * 4096 * UNIT_M / CELL_M)))
    gza = max(0, int(np.ceil((y0 - CELL_Y0) * 4096 * UNIT_M / CELL_M))); gzb = min(GZ, int(np.floor((y1 + 1 - CELL_Y0) * 4096 * UNIT_M / CELL_M)))
    blk = mat[gza:gzb + 1, gxa:gxb + 1]
    n_ = int(((blk & 64) != 0).sum())
    mat[gza:gzb + 1, gxa:gxb + 1] = blk & ~np.uint8(64)
    print('%s: cells x %d..%d, y %d..%d: %d grown trees taken off (its own placed)' % (os.path.basename(src), x0, x1, y0, y1, n_))

out = bytearray(b'HLND' + struct.pack('<IIIi', 1, GX, GZ, off))
P = np.zeros_like(hs)
P[0, 1:] = hs[0, :-1]; P[1:, 0] = hs[:-1, 0]; P[1:, 1:] = hs[1:, :-1] + hs[:-1, 1:] - hs[:-1, :-1]
r_ = (hs - P).ravel().tolist()
hb2 = bytearray()
for v in r_:
    if -128 < v < 128: hb2.append(v & 255)
    else: hb2.append(128); hb2 += struct.pack('<h', v)
out += struct.pack('<I', len(hb2)) + hb2
mm = mat.ravel().tolist(); mb2 = bytearray(); i = 0
while i < len(mm):
    j = i
    while j < len(mm) and mm[j] == mm[i] and j - i < 256: j += 1
    mb2.append(j - i - 1); mb2.append(mm[i]); i = j
out += struct.pack('<I', len(mb2)) + mb2 + rest
open(dst, 'wb').write(out)
print('%s: %d bytes' % (dst, len(out)))
