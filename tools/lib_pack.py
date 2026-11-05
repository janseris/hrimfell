#!/usr/bin/env python3
# (v0.4.56) The shared library: the models every area would carry (the same piece in two or more areas: a barrel, a
# Nordic ruin's wall, a candle) packed once into data/common.hlb with the textures used most across the areas;
# each area's file (data/<area>.har) then holds only its own pieces and its own textures, and names the library's
# by number. Areas are made by docs/maps/gen_kit.py with LIB=1 (slots name their texture 0x1000 + n; the pictures
# in kit_texpics.pkl):
#   python3 tools/lib_pack.py src/kit_ext.inc data/riverwood.har  src/area_bfb/kit_ext.inc data/bleakfalls.har ...
#
# Models: the library's are numbered 0..NL-1, each area's own NL.. (its cells' pieces, the far and middle models,
# the plants' harvest props and the items' models use these numbers). Textures: two banks of up to 64 - the
# library's (a slot's look byte 0x40 + n) and the area's (0xC0 + n, as before); a texture in neither: its colour,
# flat. Each bank: its first n2 at 256 x 256 too, its first nh at 128 x 128 (as kit_tex.bin; see gen_kit.py).
# The library file: "HAR1" version 3, its models, 0 cells, their names, its bank. An area's: as tools/kit_pack.py
# writes it (its own models, cells, names, its bank, plants, items' models) and what can be seen from where
# (host/pvs.cpp's "HPVS" at its end) carried over from the file it replaces.
import re, struct, sys, os, pickle, hashlib, collections
from PIL import Image

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
LIBMAX, AREAMAX = 64, 64
LIB_HI, LIB_HI2, AREA_HI, AREA_HI2 = 32, 8, 32, 8

cellid = {}
for hp in ('items.h', 'game.h', 'r3d.h'):
    txt = open(os.path.join(ROOT, 'inc', hp)).read()
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
def split_top(txt):
    out, d, cur = [], 0, ''
    for ch in txt:
        if ch == '{': d += 1
        if ch == '}': d -= 1
        if ch == ',' and d == 0: out.append(cur.strip()); cur = ''
        else: cur += ch
    if cur.strip(): out.append(cur.strip())
    return out

# ---------------------------------------------------------------- an area as gen_kit.py made it
def parse_area(src):
    d = os.path.dirname(os.path.abspath(src))
    s = open(src).read()
    arrays = {}
    for typ, name, n, body in re.findall(r'static const (short|unsigned short|int) (\w+)\[(\d+)\] = \{(.*?)\};', s, re.S):
        v = [val(x) for x in body.replace('\n', ' ').split(',') if x.strip()]
        assert len(v) == int(n), name
        arrays[name] = v
    mesh_rows = re.search(r'static const KitMesh KKitMesh\[\d+\] = \{(.*?)\n\s*\};', s, re.S).group(1)
    cell_rows = re.search(r'static const KitCell KKitCell\[\d+\] = \{(.*?)\n\s*\};', s, re.S).group(1)
    mrows = [split_top(r.strip().rstrip(',').strip()[1:-1]) for r in mesh_rows.strip().split('\n') if r.strip().startswith('{')]
    crows = [split_top(r.strip().rstrip(',').strip()[1:-1]) for r in cell_rows.strip().split('\n') if r.strip().startswith('{')]
    mnames = {int(i): '%s  %s' % (e, m) for i, m, e in re.findall(r'^// (\d+): (.*?) \((.*?)\):', s, re.M)}
    tp = pickle.load(open(os.path.join(d, 'kit_texpics.pkl'), 'rb'))
    order = tp['order']
    meshes = []
    for i, f in enumerate(mrows):
        slots = [val(x) for x in split_top(f[6].strip()[1:-1])]
        assert len(slots) == 32
        for k in range(3, 32, 4):
            if slots[k] >= 0x1000: slots[k] = ('T', order[slots[k] - 0x1000])
        hx, hz, hy, bx, bz, lod, spin, pivz, yoff = [int(x) for x in f[7:16]]
        meshes.append({'nv': int(f[1]), 'nf': int(f[3]), 'v': arrays[f[0]], 'f': arrays[f[2]], 'n': arrays[f[4]], 'uv': arrays[f[5]],
                       'slots': slots, 'box': (hx, hz, hy, bx, bz), 'lod': lod, 'rest': (spin, pivz, yoff), 'name': mnames.get(i, '')})
    cells = []
    for f in crows:
        cid = val(f[0]); nm = f[1].strip('"').encode('latin-1') + b'\0'
        names = [f[2], f[4], f[6], f[8], f[10], f[12], f[14], f[16], f[18], f[19], f[20]]
        counts = [int(f[3]), int(f[5]), int(f[7]), int(f[9]), int(f[11]), int(f[13]), int(f[15]), int(f[17])]
        cells.append({'id': cid, 'name': nm, 'arr': [list(arrays[n]) if n in arrays else None for n in names], 'counts': counts, 'ox': int(f[21]), 'oy': int(f[22])})
    def rd(fn, mode='rb'):
        p = os.path.join(d, fn)
        return open(p, mode).read() if os.path.exists(p) else None
    items = []
    t = rd('kit_items.txt', 'r')
    if t: items = [(cellid[a], int(b)) for a, b in (l.split() for l in t.splitlines() if l.strip()) if a in cellid]
    return {'src': src, 'dir': d, 'meshes': meshes, 'cells': cells, 'order': order, 'pics': tp['pics'], 'flora': rd('kit_flora.bin'),
            'items': items, 'itemsbin': rd('kit_items.bin')}

# each model's content (the textures by name; its far and middle models by theirs): the same in two areas -> shared
def keys_of(A):
    ms = A['meshes']; memo = {}
    def key(i):
        if i in memo: return memo[i]
        m = ms[i]
        far = -1 if m['lod'] < 0 else (m['lod'] & 0xFFFF); mid = -1 if m['lod'] < 0 else (m['lod'] >> 16) - 1
        h = hashlib.sha1(repr((m['nv'], m['nf'], m['v'], m['f'], m['n'], m['uv'], m['slots'], m['box'], m['rest'],
                                key(far) if 0 <= far < len(ms) else None, key(mid) if 0 <= mid < len(ms) else None)).encode()).hexdigest()
        memo[i] = h
        return h
    return [key(i) for i in range(len(ms))]

# ---------------------------------------------------------------- a bank of textures (as gen_kit.py's kit_tex.bin)
def bank_bytes(names, pics, nh, n2):
    ims = [Image.frombytes('RGB', (64, 64), pics[tx]['64']) for tx in names]
    sheet = Image.new('RGB', (64, 64 * len(ims)))
    for i, im in enumerate(ims): sheet.paste(im, (0, 64 * i))
    q = sheet.quantize(255, method=Image.MEDIANCUT, dither=Image.NONE)
    pal = q.getpalette()[:765]; pal += [0] * (765 - len(pal)); pal = [255, 0, 255] + pal
    def qbytes(im): return bytes(v + 1 for v in im.quantize(palette=q, dither=Image.NONE).tobytes())
    def cut(b_, tx, side):
        mk = pics[tx].get('mask%d' % side)
        if not pics[tx]['key'] or not mk: return b_
        b_ = bytearray(b_)
        for k in range(side * side):
            if not mk[k]: b_[k] = 0
        return bytes(b_)
    data = bytearray()
    for i, tx in enumerate(names): data += cut(qbytes(ims[i]), tx, 64)
    blur = sheet.resize((32, 32 * len(ims)), Image.BOX).resize((64, 64 * len(ims)), Image.NEAREST)
    data += qbytes(blur)
    out = bytearray(b'HTX3' + struct.pack('<I', len(ims))) + bytes(pal)
    for im in ims:
        px = list(im.getdata()); n_ = len(px)
        out += bytes(sum(p[k] for p in px) // n_ for k in range(3))
    while len(out) % 4: out.append(0)
    out += data
    out += struct.pack('<I', nh)
    for tx in names[:nh]: out += cut(qbytes(Image.frombytes('RGB', (128, 128), pics[tx]['128'])), tx, 128)
    out += b'HTXK' + struct.pack('<I', len(names)) + bytes(1 if pics[tx]['key'] else 0 for tx in names)
    while len(out) % 4: out.append(0)
    if n2:
        out += b'HTXH' + struct.pack('<I', n2)
        for tx in names[:n2]: out += cut(qbytes(Image.frombytes('RGB', (256, 256), pics[tx]['256'])), tx, 256)
    return bytes(out)
# a bank's order: the finest (at 256) first, then those at 128, then the rest
def bank_order(names, pics, hiN, hi2N):
    best = lambda tx, k: pics[tx][k]
    c2 = [tx for tx in names if '256' in pics[tx] and '128' in pics[tx]]
    c2 = sorted(c2, key=lambda t: (not best(t, 'sign'), -best(t, 'hi2')))[:hi2N]
    c1 = sorted([tx for tx in names if '128' in pics[tx] and tx not in c2], key=lambda t: (not best(t, 'sign'), -best(t, 'hi')))[:max(0, hiN - len(c2))]
    rest = [tx for tx in names if tx not in c2 and tx not in c1]
    return c2 + c1 + rest, len(c2) + len(c1), len(c2)

# ---------------------------------------------------------------- writing
def put_model(out, m, slots, lod):
    while len(out) % 4: out.append(0)
    out += struct.pack('<ii', m['nv'], m['nf']) + bytes(slots)
    out += struct.pack('<5hh4i', *m['box'], 0, lod, *m['rest'])
    for arr, fmt in ((m['v'], 'h'), (m['f'], 'H'), (m['n'], 'h'), (m['uv'], 'H')):
        while len(out) % 4: out.append(0)
        out += struct.pack('<%d%s' % (len(arr), fmt), *arr)
def put_names(out, names):
    for nm in names:
        b_ = nm.encode('latin-1', 'replace')[:250] + b'\0'
        out += struct.pack('<H', len(b_)) + b_
def align(out):
    while len(out) % 4: out.append(0)

def main(argv):
    pairs = [(argv[i], argv[i + 1]) for i in range(0, len(argv) - 1, 2)]
    areas = [parse_area(s) for s, _ in pairs]
    for A in areas: A['keys'] = keys_of(A)
    # (v0.4.68) a model shared only among the ruins (Bleak Falls', Forelhost's: their Nordic halls) stays in each ruin's
    # own file: the library is always in memory, and the halls are only needed inside them
    RUINS = ('bleakfalls.har', 'forelhost.har')
    seen = collections.Counter(); town = collections.Counter()
    for A, (_, dst) in zip(areas, pairs):
        for k in set(A['keys']):
            seen[k] += 1
            if os.path.basename(dst) not in RUINS: town[k] += 1
    for k in list(seen):
        if not town[k]: seen[k] = 1
    # the library's models: in two or more areas (in the first area's order, then the next's)
    lib = []; libix = {}; libm = []
    for A in areas:
        for i, k in enumerate(A['keys']):
            if seen[k] >= 2 and k not in libix: libix[k] = len(lib); lib.append(k); libm.append(A['meshes'][i])
    NL = len(lib)
    # textures: their use by area (gen_kit's weighed counts), and which are on the library's models
    pics = {}
    for A in areas:
        for tx, p in A['pics'].items():
            if tx not in pics: pics[tx] = dict(p); pics[tx].setdefault('wa', 0)
            else:
                q = pics[tx]; q['use'] += p['use']; q['wa'] += p.get('wa', 0); q['hi'] = max(q['hi'], p['hi']); q['hi2'] = max(q['hi2'], p['hi2'])
    # (a texture's worth: how many faces use it and how much of the world it covers - the ground pieces' few big faces)
    tu = sum(p['use'] for p in pics.values()) or 1; tw = sum(p['wa'] for p in pics.values()) or 1
    def worth(tx, P=None):
        P = P or pics
        return P[tx]['use'] / tu + P[tx].get('wa', 0) / tw
    def texs(m): return [s[1] for s in m['slots'][3::4] if isinstance(s, tuple)]
    def flora_texs(A):
        out = []
        for ent in flora_entries(A['flora']):
            if ent[1] != 255 and ent[1] < len(A['order']): out.append(A['order'][ent[1]])
        return out
    libuse = collections.Counter()
    for m in libm:
        for tx in texs(m): libuse[tx] += 1
    inareas = collections.Counter()
    for A in areas:
        own = set()
        for i, m in enumerate(A['meshes']):
            if A['keys'][i] not in libix: own.update(texs(m))
        own.update(flora_texs(A))
        for tx in own: inareas[tx] += 1
    cand = set(libuse) | {tx for tx, n in inareas.items() if n >= 2}
    libtex = sorted(cand, key=lambda t: -worth(t))[:LIBMAX]
    libtex, lnh, ln2 = bank_order(libtex, pics, LIB_HI, LIB_HI2)
    if os.environ.get('BANKS'): print('library bank: %s' % ', '.join('%02x %s' % (0x40 | i, os.path.basename(t) if isinstance(t, str) else t) for i, t in enumerate(libtex)))
    lti = {tx: i for i, tx in enumerate(libtex)}
    def code_lib(s):
        return 0x40 | lti[s[1]] if isinstance(s, tuple) and s[1] in lti else (1 if isinstance(s, tuple) else s)
    # (the library's own far and middle models: by number in it)
    out = bytearray(b'HAR1' + struct.pack('<III', 3, NL, 0))
    owner = {}
    for A in areas:
        for i, k in enumerate(A['keys']):
            if k in libix and k not in owner: owner[k] = (A, i)
    def lod_of(A, m, glob):
        if m['lod'] < 0: return -1
        far = m['lod'] & 0xFFFF; mid = (m['lod'] >> 16) - 1
        gf = glob(A, far) if 0 <= far < len(A['meshes']) else -1
        gm = glob(A, mid) if 0 <= mid < len(A['meshes']) else -1
        return (gf & 0xFFFF) | ((gm + 1) << 16) if gf >= 0 else -1
    for k in lib:
        A, i = owner[k]; m = A['meshes'][i]
        put_model(out, m, [code_lib(s) if j % 4 == 3 else s for j, s in enumerate(m['slots'])], lod_of(A, m, lambda A_, j: libix[A_['keys'][j]]))
    put_names(out, [owner[k][0]['meshes'][owner[k][1]]['name'] for k in lib])
    align(out)
    if libtex: out += bank_bytes(libtex, pics, lnh, ln2)
    libfile = os.path.join(os.path.dirname(pairs[0][1]), 'common.hlb')
    open(libfile, 'wb').write(out)
    print('%s: %d models, %d textures (%d at 128, %d at 256), %d bytes' % (libfile, NL, len(libtex), lnh, ln2, len(out)))
    # each area: its own models and textures
    PT_HARVEST = cellid['PT_HARVEST']; PT_DOOR = cellid['PT_DOOR']
    for A, (src, dst) in zip(areas, pairs):
        own = [i for i, k in enumerate(A['keys']) if k not in libix]
        oix = {i: NL + j for j, i in enumerate(own)}
        def glob(A_, i): return libix[A_['keys'][i]] if A_['keys'][i] in libix else oix[i]
        use = collections.Counter()
        for tx in set(t for i in own for t in texs(A['meshes'][i])): use[tx] += worth(tx, A['pics'])
        for tx in flora_texs(A): use[tx] += 1e6
        atex = [tx for tx, _ in sorted(use.items(), key=lambda kv: -kv[1]) if tx not in lti][:AREAMAX]
        atex, anh, an2 = bank_order(atex, pics, AREA_HI, AREA_HI2)
        if os.environ.get('BANKS'): print('%s bank: %s' % (dst, ', '.join('%02x %s' % (0xC0 | i, os.path.basename(t) if isinstance(t, str) else t) for i, t in enumerate(atex))))
        ati = {tx: i for i, tx in enumerate(atex)}
        def code(s):
            if not isinstance(s, tuple): return s
            if s[1] in lti: return 0x40 | lti[s[1]]
            if s[1] in ati: return 0xC0 | ati[s[1]]
            return 1
        out = bytearray(b'HAR1' + struct.pack('<III', 3, len(own), len(A['cells'])))
        for i in own:
            m = A['meshes'][i]
            put_model(out, m, [code(s) if j % 4 == 3 else s for j, s in enumerate(m['slots'])], lod_of(A, m, glob))
        for c in A['cells']:
            arr = [list(a) if a is not None else None for a in c['arr']]
            if arr[0]:
                for r in range(0, len(arr[0]), 7):
                    if 0 <= arr[0][r] < len(A['meshes']): arr[0][r] = glob(A, arr[0][r])
            if arr[1]:
                for r in range(0, len(arr[1]), 6):
                    if arr[1][r] == PT_HARVEST and 0 <= (arr[1][r + 5] >> 4) < len(A['meshes']):
                        arr[1][r + 5] = (glob(A, arr[1][r + 5] >> 4) << 4) | (arr[1][r + 5] & 15)
                    # (v0.4.74) a ruin's door: its model in the low 11 bits (gen_kit.py), the leaf's width and hinge above
                    if arr[1][r] == PT_DOOR and arr[1][r + 5] > 0 and 0 <= (arr[1][r + 5] & 2047) < len(A['meshes']):
                        g_ = glob(A, arr[1][r + 5] & 2047); assert g_ < 2048
                        arr[1][r + 5] = (arr[1][r + 5] & ~2047) | g_
            align(out)
            out += struct.pack('<11i', c['id'], *c['counts'], c['ox'], c['oy'])
            out += struct.pack('<12i', len(c['name']), *[len(a) if a is not None else 0 for a in arr])
            align(out); out += c['name']
            for a, fm in zip(arr, ['i', 'i', 'i', 'i', 'i', 'i', 'h', 'h', 'i', 'i', 'H']):
                align(out)
                if a is not None: out += struct.pack('<%d%s' % (len(a), fm), *a)
        put_names(out, [A['meshes'][i]['name'] for i in own])
        align(out)
        if atex: out += bank_bytes(atex, pics, anh, an2)
        align(out)
        if A['flora']:
            fb = bytearray(A['flora'])
            for off, t in flora_entries(A['flora']):
                if t == 255 or t >= len(A['order']): continue
                tx = A['order'][t]
                fb[off] = 0x40 | lti[tx] if tx in lti else (0xC0 | ati[tx] if tx in ati else 255)
            out += fb
        align(out)
        if A['items']:
            out += b'HIM1' + struct.pack('<I', len(A['items']))
            for it, mk in A['items']: out += struct.pack('<HH', it, glob(A, mk) if 0 <= mk < len(A['meshes']) else 0xFFFF)
        # what can be seen from where: the old file's, its pieces the same
        if os.path.exists(dst):
            old = open(dst, 'rb').read()
            if len(old) >= 16 and old[-4:] == b'PVS1':
                q = struct.unpack_from('<I', old, len(old) - 8)[0]
                align(out); at = len(out)
                out += old[q:len(old) - 8] + struct.pack('<I', at) + b'PVS1'
        open(dst, 'wb').write(out)
        print('%s: %d own models (%d from the library), %d cells, %d textures of its own (%d at 128, %d at 256), %d bytes'
              % (dst, len(own), len(A['meshes']) - len(own), len(A['cells']), len(atex), anh, an2, len(out)))
        if A['itemsbin']: write_items(A['itemsbin'], os.path.join(os.path.dirname(dst), 'items.hit'))

# the plants' kinds (kit_flora.bin "HFL1" .. "HAN1" .. "HFM2"): (offset of each kind's picture byte, its number)
def flora_entries(b):
    if not b: return []
    rd = lambda p: struct.unpack_from('<i', b, p)[0]
    p = 0; out = []
    if b[p:p + 4] == b'HFL1':
        n, gw, gh = rd(p + 4), rd(p + 20), rd(p + 24)
        pf = p + 28; pe = (pf + (gw * gh + 1) * 2 + 3) & ~3
        p = pe + n * 6
    if b[p:p + 4] == b'HAN1': p += 8 + rd(p + 4) * 12
    p = (p + 3) & ~3
    if b[p:p + 4] != b'HFM2': return []
    nk = rd(p + 4); p += 8
    for k in range(nk):
        tex, fl = b[p + 6], b[p + 7]
        nv = b[p + 8] | (b[p + 9] << 8); nf = b[p + 10] | (b[p + 11] << 8)
        out.append((p + 6, tex))
        p += 12
        if not nv: continue
        p += nv * 6; p = (p + 3) & ~3; p += nf * 8 + nf * 6
        if fl & 1: p += nv * 6
        if fl & 2: p += nf * 12
        p = (p + 3) & ~3
        nv2 = b[p] | (b[p + 1] << 8); nf2 = b[p + 2] | (b[p + 3] << 8); p += 4
        p += nv2 * 6; p = (p + 3) & ~3; p += nf2 * 8 + nf2 * 6
        if fl & 2: p += nf2 * 12
        p = (p + 3) & ~3
    return out

def write_items(b, di):
    p = 0; ents = []
    while p < len(b):
        n = b[p]; name = b[p + 1:p + 1 + n].decode(); size = struct.unpack_from('<I', b, p + 1 + n)[0]
        blob = b[p + 5 + n:p + 5 + n + size]; p += 5 + n + size
        if name in cellid: ents.append((cellid[name], blob))
    hdr = 8 + 12 * len(ents); body = bytearray(); tab = bytearray()
    for idn, blob in ents:
        while (hdr + len(body)) % 4: body.append(0)
        tab += struct.pack('<HHII', idn, 0, hdr + len(body), len(blob)); body += blob
    open(di, 'wb').write(b'HIT1' + struct.pack('<I', len(ents)) + bytes(tab) + bytes(body))
    print('%s: %d items' % (di, len(ents)))

if __name__ == '__main__': main(sys.argv[1:])
