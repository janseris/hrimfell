#!/usr/bin/env python3
# Pack an area's models and places (src/kit_ext.inc from docs/maps/gen_kit.py) into the data file the game reads
# when the area is near (data/<area>.har), instead of building them into the program.
#   python3 tools/kit_pack.py [src/kit_ext.inc] [data/riverwood.har]
#
# File (little-endian; every array starts on a multiple of 4 bytes from the start of the file):
#   "HAR1", u32 version 2, u32 models, u32 cells
#   per model: i32 nv, nf; u8 slots[32]; i16 hx, hz, hy, bx, bz, (pad); i32 lod, spin, pivz, yoff;
#              then its arrays: vertices nv * 3 i16, faces nf * 4 u16, normals nf * 3 i16, texels nf * 6 u16
#   per cell:  i32 cell, nstat, nprop, nitem, nflame, nlight, npeople, nfloor, nwall, ox, oy;
#              i32 lengths (elements) of: name (bytes), stat, prop, item, flame, light, people, floor, wall, door,
#              grid, glist;  then those arrays in that order: name (bytes, 0-ended), stat .. light, people (i32),
#              floor, wall (i16), door, grid (i32), glist (u16)
#   then (version 2) per model its name: u16 length, the editor id and the file (0-ended)
#   then (version 3; on a multiple of 4) Skyrim's textures (src/kit_tex.bin from gen_kit.py): "HTX3", u32 n, the
#   palette 256 * r, g, b, n * the average r, g, b, (to a multiple of 4), n * 64 * 64 bytes on the palette, then
#   n * 64 * 64 the same blurred (2 x 2 texels averaged), u32 nh, nh * 128 * 128 the first nh larger
import re, struct, sys, os

src = sys.argv[1] if len(sys.argv) > 1 else 'src/kit_ext.inc'
dst = sys.argv[2] if len(sys.argv) > 2 else 'data/riverwood.har'
s = open(src).read()
hdr = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'inc', 'game.h')).read()
# the game's enums (cells, prop kinds, items, people ...): each name's value, as C counts them
cellid = {}
for hp in ('items.h', 'game.h', 'r3d.h'):
    txt = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'inc', hp)).read()
    txt = re.sub(r'//[^\n]*', '', txt)
    for m in re.finditer(r'\benum\s*(?:\w+\s*)?\{([^}]*)\}', txt):
        cur = -1
        for item in m.group(1).split(','):
            item = item.strip()
            if not item: continue
            if '=' in item:
                k, v = [t.strip() for t in item.split('=', 1)]
                try: cur = int(eval(v, {}, dict(cellid)))
                except Exception: continue
            else: k = item; cur += 1
            if re.match(r'^\w+$', k): cellid[k] = cur
TX = {k: i for i, k in enumerate('TXI_GRASS TXI_ROCK TXI_SNOW TXI_SAND TXI_FUR TXI_NEEDLE TXI_FURN TXI_BLOCK TXI_DIRT TXI_LOGS '
                                'TXI_THATCH TXI_NORDIC TXI_PLANKS TXI_SHINGLE TXI_FIELD TXI_SLATE TXI_DSTAVE TXI_ROPE'.split())}
def val(t):
    t = t.strip()
    if t in cellid: return cellid[t]
    if 'SL_TEX' in t: return 0x80 | TX[t.split('|')[1].strip()]
    return int(t, 0)

arrays = {}
for typ, name, n, body in re.findall(r'static const (short|unsigned short|int) (\w+)\[(\d+)\] = \{(.*?)\};', s, re.S):
    v = [val(x) for x in body.replace('\n', ' ').split(',') if x.strip()]
    assert len(v) == int(n), name
    arrays[name] = (typ, v)

def split_top(txt):
    # the top-level comma-separated fields of an initializer (braces kept together)
    out, d, cur = [], 0, ''
    for ch in txt:
        if ch == '{': d += 1
        if ch == '}': d -= 1
        if ch == ',' and d == 0: out.append(cur.strip()); cur = ''
        else: cur += ch
    if cur.strip(): out.append(cur.strip())
    return out

mesh_rows = re.search(r'static const KitMesh KKitMesh\[\d+\] = \{(.*?)\n\s*\};', s, re.S).group(1)
cell_rows = re.search(r'static const KitCell KKitCell\[\d+\] = \{(.*?)\n\s*\};', s, re.S).group(1)
meshes = [split_top(r.strip().rstrip(',').strip()[1:-1]) for r in mesh_rows.strip().split('\n') if r.strip().startswith('{')]
cells = [split_top(r.strip().rstrip(',').strip()[1:-1]) for r in cell_rows.strip().split('\n') if r.strip().startswith('{')]

out = bytearray(b'HAR1' + struct.pack('<III', 3, len(meshes), len(cells)))
# each model's name: its Skyrim editor id and file (from gen_kit.py's comments), for the game's "what is this" display
mnames = {int(i): '%s  %s' % (e, m) for i, m, e in re.findall(r'^// (\d+): (.*?) \((.*?)\):', s, re.M)}
def align():
    while len(out) % 4: out.append(0)
def put(name, fmt):
    typ, v = arrays[name]
    align(); out.extend(struct.pack('<%d%s' % (len(v), fmt), *v))
for f in meshes:
    # v, nv, f, nf, n, uv, {slots}, hx, hz, hy, bx, bz, lod, spin, pivz, yoff
    vname, nv, fname, nf, nname, tname = f[0], int(f[1]), f[2], int(f[3]), f[4], f[5]
    slots = [val(x) for x in split_top(f[6].strip()[1:-1])]
    assert len(slots) == 32
    hx, hz, hy, bx, bz, lod, spin, pivz, yoff = [int(x) for x in f[7:16]]
    align()
    out.extend(struct.pack('<ii', nv, nf)); out.extend(bytes(slots))
    out.extend(struct.pack('<5hh4i', hx, hz, hy, bx, bz, 0, lod, spin, pivz, yoff))
    assert len(arrays[vname][1]) == nv * 3 and len(arrays[fname][1]) == nf * 4
    assert len(arrays[nname][1]) == nf * 3 and len(arrays[tname][1]) == nf * 6
    put(vname, 'h'); put(fname, 'H'); put(nname, 'h'); put(tname, 'H')
for f in cells:
    # cell, name, stat, nstat, prop, nprop, item, nitem, flame, nflame, light, nlight, people, npeople, floor, nfloor,
    # wall, nwall, door, grid, glist, ox, oy
    cid = val(f[0]); nm = f[1].strip('"').encode('latin-1') + b'\0'
    names = [f[2], f[4], f[6], f[8], f[10], f[12], f[14], f[16], f[18], f[19], f[20]]
    counts = [int(f[3]), int(f[5]), int(f[7]), int(f[9]), int(f[11]), int(f[13]), int(f[15]), int(f[17])]
    ox, oy = int(f[21]), int(f[22])
    lens = [len(arrays[n][1]) if n in arrays else 0 for n in names]
    align()
    out.extend(struct.pack('<11i', cid, *counts, ox, oy))
    out.extend(struct.pack('<12i', len(nm), *lens))
    align(); out.extend(nm)
    fmts = ['i', 'i', 'i', 'i', 'i', 'i', 'h', 'h', 'i', 'i', 'H']
    for n, fm in zip(names, fmts):
        if n in arrays: put(n, fm)
        else: align()
# version 2: the models' names, after the cells (a u16 length, the bytes, a 0)
for i in range(len(meshes)):
    nm = mnames.get(i, '').encode('latin-1', 'replace')[:250] + b'\0'
    out.extend(struct.pack('<H', len(nm))); out.extend(nm)
# version 3: Skyrim's textures (the slots whose look byte is 0xC0 + a number)
tf = os.path.join(os.path.dirname(os.path.abspath(src)), 'kit_tex.bin')
align()
if os.path.exists(tf): out.extend(open(tf, 'rb').read())
# then (to a multiple of 4) the land's plants: src/kit_flora.bin ("HFL1", see gen_kit.py)
ff = os.path.join(os.path.dirname(os.path.abspath(src)), 'kit_flora.bin')
align()
if os.path.exists(ff): out.extend(open(ff, 'rb').read())
# then the items' own models (gen_kit.py's kit_items.txt): "HIM1", u32 n, n * (u16 item, u16 model)
fi = os.path.join(os.path.dirname(os.path.abspath(src)), 'kit_items.txt')
align()
if os.path.exists(fi):
    pairs = [(cellid[a], int(b)) for a, b in (l.split() for l in open(fi) if l.strip()) if a in cellid]
    out.extend(b'HIM1' + struct.pack('<I', len(pairs)))
    for a, b in pairs: out.extend(struct.pack('<HH', a, b))
open(dst, 'wb').write(out)
print('%s: %d models, %d cells, %d bytes' % (dst, len(meshes), len(cells), len(out)))
# the items' own models (gen_kit.py: src/kit_items.bin, by name) -> data/items.hit, by number:
#   "HIT1", u32 n, n * (u16 item, u16 0, u32 offset, u32 size), the models (each at a multiple of 4)
fb = os.path.join(os.path.dirname(os.path.abspath(src)), 'kit_items.bin')
if os.path.exists(fb):
    b = open(fb, 'rb').read(); p = 0; ents = []
    while p < len(b):
        n = b[p]; name = b[p + 1:p + 1 + n].decode(); size = struct.unpack_from('<I', b, p + 1 + n)[0]
        blob = b[p + 5 + n:p + 5 + n + size]; p += 5 + n + size
        if name in cellid: ents.append((cellid[name], blob))
        else: print('   item %s: not in items.h' % name)
    hdr = 8 + 12 * len(ents); body = bytearray(); tab = bytearray()
    for idn, blob in ents:
        while (hdr + len(body)) % 4: body.append(0)
        tab += struct.pack('<HHII', idn, 0, hdr + len(body), len(blob)); body += blob
    hit = b'HIT1' + struct.pack('<I', len(ents)) + bytes(tab) + bytes(body)
    di = os.path.join(os.path.dirname(dst), 'items.hit')
    open(di, 'wb').write(hit)
    print('%s: %d items, %d bytes, the largest %d' % (di, len(ents), len(hit), max(len(x[1]) for x in ents)))
