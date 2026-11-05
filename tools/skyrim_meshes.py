#!/usr/bin/env python3
# Skyrim's own shapes for Hrimfell: the building pieces and furniture of the Nord houses and inns, and what stands in
# Riverwood, taken out of Skyrim's .bsa archives with their textures' colours.
#
# Usage (Windows, Python 3.8+ from python.org, nothing else needed):
#   python skyrim_meshes.py
# Skyrim is looked for in C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\Data; elsewhere,
# give its path first (the Data folder, or Skyrim.esm in it).
# (the Data folder: Skyrim.esm and the "Skyrim - Meshes0/1.bsa", "Skyrim - Textures0..8.bsa" archives in it).
# Takes a few minutes and about 2 GB of memory. Only reads; nothing in the game folder is changed.
#
# Which models: everything placed in the interiors built from the farmhouse set (Skyrim's Nord houses and inns),
# every farmhouse building piece, and everything placed in and around Riverwood (cells 3..7, -13..-9), the world's
# persistent things there too (the forge, the horse trough, the sawmill's log, ...).
#
# Writes a folder "skyrim_meshes" next to this script, and packs it into skyrim_meshes.zip - send that file:
#   models.csv     every model: the objects that use it, its file, shapes, vertices, triangles, size, how it went
#   meshes\*.obj   each model's geometry (Skyrim units, x east, y north, z up), one object per shape, its diffuse
#                  texture as the material name; texture coordinates kept
#   textures.csv   every diffuse texture used: format, size, its average colour (and the darker / lighter halves')
#   textures\     each of them small (64 on its longer side), as a .ppm picture; and at 128 (_128.ppm; signs, and those
#                 Skyrim made 2048 or more, also 256)
#   raw\*.nif      a few of the original files (to check the reading against)
#   raw_failed\    some of the files no shapes could be read from (the trees first), to find out why
#   items.csv      every thing one carries (weapons, armour, food, ...): its type, editor id and model; the models
#                  of those the game has (ITEM_EDIDS below, and any in items.txt next to this script) are exported
#                  (each shape in an .obj: "# alpha test N" / "# alpha blend" where it is cut out or blended, "# double
#                  sided" where both its faces are drawn - the roofs' thatch seen from under the eaves)
#   errors.txt     anything that could not be read
import os, sys, struct, zlib, math, re, zipfile, collections, traceback

DEFAULT_DATA = r'C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\Data'

def data_arg(args):
    # the first argument when it is a path (the Data folder or Skyrim.esm in it); otherwise the usual Steam folder.
    # -> (the Data folder, the remaining arguments)
    if args and (args[0].lower().endswith('.esm') or os.path.isdir(args[0]) or '\\' in args[0] or '/' in args[0]):
        p, args = args[0], args[1:]
    else: p = DEFAULT_DATA
    if p.lower().endswith('.esm'): p = os.path.dirname(p)
    return p, args

# ---------------------------------------------------------------- Skyrim.esm (as skyrim_kits.py)
def subrecords(data):
    out = []; p = 0; big = None
    while p + 6 <= len(data):
        t = data[p:p + 4]; n = struct.unpack_from('<H', data, p + 4)[0]; p += 6
        if big is not None: n = big; big = None
        if t == b'XXXX': big = struct.unpack_from('<I', data, p)[0]; p += n; continue
        out.append((t, data[p:p + n])); p += n
    return out

def zstr(d): return d.split(b'\0')[0].decode('latin-1')

def folder(model):
    m = model.replace('/', '\\').lower()
    if m.startswith('meshes\\'): m = m[7:]
    return m.rsplit('\\', 1)[0] if '\\' in m else ''

def read_esm(path):
    with open(path, 'rb') as f: buf = f.read()
    base = {}; st = {'world': '', 'cell': None}
    cell_bases = collections.defaultdict(collections.Counter); area = set()
    def walk(pos, end):
        while pos < end:
            typ = buf[pos:pos + 4]; size = struct.unpack_from('<I', buf, pos + 4)[0]
            if typ == b'GRUP':
                label = buf[pos + 8:pos + 12]; gtype = struct.unpack_from('<i', buf, pos + 12)[0]
                saved = (st['world'], st['cell'])
                if gtype == 1: st['world'] = 'world'
                elif gtype == 0 and label == b'CELL': st['world'] = ''
                walk(pos + 24, pos + size)
                if gtype in (0, 1): st['world'], st['cell'] = saved[0], None
                pos += size; continue
            flags, fid = struct.unpack_from('<II', buf, pos + 8)
            data = buf[pos + 24:pos + 24 + size]; pos += 24 + size
            if typ in (b'LAND', b'NAVM', b'NAVI', b'WRLD', b'INFO', b'DIAL', b'QUST', b'SCEN', b'PACK'): continue
            if flags & 0x00040000:
                try: data = zlib.decompress(data[4:])
                except zlib.error: continue
            subs = subrecords(data)
            if typ == b'CELL':
                c = {'edid': '', 'interior': st['world'] == '', 'x': None, 'y': None}
                for t, d in subs:
                    if t == b'EDID': c['edid'] = zstr(d)
                    elif t == b'XCLC' and len(d) >= 8: c['x'], c['y'] = struct.unpack_from('<ii', d)
                st['cell'] = c
            elif typ in (b'REFR', b'ACHR'):
                c = st['cell']
                if c is None: continue
                b = 0; pos3 = None
                for t, d in subs:
                    if t == b'NAME' and len(d) >= 4: b = struct.unpack_from('<I', d)[0]
                    elif t == b'DATA' and len(d) >= 12: pos3 = struct.unpack_from('<3f', d)
                if c['interior']: cell_bases[c['edid']][b] += 1
                else:
                    # the cell by its grid, or (the world's persistent things: the forge, the trough, ...) by the
                    # thing's own position
                    cx, cy = c['x'], c['y']
                    if pos3 is not None: cx, cy = int(math.floor(pos3[0] / 4096.0)), int(math.floor(pos3[1] / 4096.0))
                    if cx is not None and 3 <= cx <= 7 and -13 <= cy <= -9: area.add(b)
            else:
                edid = ''; model = ''
                for t, d in subs:
                    if t == b'EDID': edid = zstr(d)
                    elif t == b'MODL' and not model: model = zstr(d)
                    elif t == b'MOD2' and not model and typ == b'ARMO': model = zstr(d)     # (armour: its model lying on the ground)
                base[fid] = (typ.decode('latin-1'), edid, model)
    sys.setrecursionlimit(10000)
    walk(0, len(buf))
    return base, cell_bases, area

# ---------------------------------------------------------------- LZ4 frames (pure Python)
def lz4_block(src, dst):
    p = 0; n = len(src)
    while p < n:
        tok = src[p]; p += 1
        ln = tok >> 4
        if ln == 15:
            while True:
                b = src[p]; p += 1; ln += b
                if b != 255: break
        dst += src[p:p + ln]; p += ln
        if p >= n: break
        off = src[p] | (src[p + 1] << 8); p += 2
        ml = tok & 15
        if ml == 15:
            while True:
                b = src[p]; p += 1; ml += b
                if b != 255: break
        ml += 4
        s = len(dst) - off
        if off >= ml: dst += dst[s:s + ml]
        else:
            for i in range(ml): dst.append(dst[s + i])
    return dst

def lz4_frame(data):
    if struct.unpack_from('<I', data, 0)[0] != 0x184D2204: raise ValueError('not an LZ4 frame')
    flg = data[4]; p = 6
    if flg & 0x08: p += 8                        # content size
    if flg & 0x01: p += 4                        # dictionary id
    p += 1                                       # header checksum
    out = bytearray()
    while p + 4 <= len(data):
        bs = struct.unpack_from('<I', data, p)[0]; p += 4
        if bs == 0: break
        raw = bs & 0x80000000; bs &= 0x7FFFFFFF
        blk = data[p:p + bs]; p += bs
        if raw: out += blk
        else: lz4_block(blk, out)
        if flg & 0x10: p += 4                    # block checksum
    return bytes(out)

# ---------------------------------------------------------------- BSA archives (Skyrim SE: version 105)
class BSA:
    def __init__(self, path):
        self.path = path; self.f = open(path, 'rb'); self.files = {}
        h = self.f.read(36)
        magic, ver, off, aflags, nfold, nfile, lfold, lfile, fflags = struct.unpack('<4sIIIIIIIH', h[:34])
        if magic != b'BSA\0': raise ValueError('not a BSA')
        self.ver, self.aflags = ver, aflags
        self.f.seek(off)
        rec = 24 if ver == 105 else 16
        folders = []
        for i in range(nfold):
            r = self.f.read(rec)
            if ver == 105: h_, cnt, _, fo = struct.unpack('<QIIQ', r)
            else: h_, cnt, fo = struct.unpack('<QII', r)
            folders.append(cnt)
        recs = []
        for cnt in folders:
            name = ''
            if aflags & 1:
                ln = self.f.read(1)[0]; name = self.f.read(ln)[:-1].decode('latin-1')
            for j in range(cnt):
                h_, size, fo = struct.unpack('<QII', self.f.read(16))
                recs.append((name, size, fo))
        names = self.f.read(lfile).split(b'\0') if aflags & 2 else [b''] * len(recs)
        for (dname, size, fo), fn in zip(recs, names):
            self.files[(dname + '\\' + fn.decode('latin-1')).lower()] = (size, fo)

    def read(self, name):
        size, off = self.files[name.lower()]
        comp = bool(self.aflags & 4) != bool(size & 0x40000000)
        size &= 0x3FFFFFFF
        self.f.seek(off)
        if self.aflags & 0x100:
            ln = self.f.read(1)[0]; self.f.read(ln); size -= ln + 1
        data = self.f.read(size)
        if not comp: return data
        orig = struct.unpack_from('<I', data, 0)[0]
        return lz4_frame(data[4:]) if self.ver == 105 else zlib.decompress(data[4:])

# ---------------------------------------------------------------- NIF (Skyrim SE: 20.2.0.7, user version 12, BS 100)
def half(h):
    s = (h >> 15) & 1; e = (h >> 10) & 31; m = h & 1023
    if e == 0: v = m / 1024.0 * 2.0 ** -14
    elif e == 31: v = 0.0
    else: v = (1 + m / 1024.0) * 2.0 ** (e - 15)
    return -v if s else v

class NIF:
    def __init__(self, data):
        self.d = data
        e = data.index(b'\n'); p = e + 1
        self.ver, endian, self.uver, nblk = struct.unpack_from('<IBII', data, p); p += 13
        self.bsver = struct.unpack_from('<I', data, p)[0]; p += 4
        for k in range(3 if self.bsver < 130 else 4):          # author, process script, export script (, max filepath)
            ln = data[p]; p += 1 + ln
        ntype = struct.unpack_from('<H', data, p)[0]; p += 2
        self.types = []
        for k in range(ntype):
            ln = struct.unpack_from('<I', data, p)[0]; p += 4
            self.types.append(data[p:p + ln].decode('latin-1')); p += ln
        self.btype = struct.unpack_from('<%dH' % nblk, data, p); p += 2 * nblk
        sizes = struct.unpack_from('<%dI' % nblk, data, p); p += 4 * nblk
        nstr, _ = struct.unpack_from('<II', data, p); p += 8
        self.strings = []
        for k in range(nstr):
            ln = struct.unpack_from('<I', data, p)[0]; p += 4
            self.strings.append(data[p:p + ln].decode('latin-1')); p += ln
        ngr = struct.unpack_from('<I', data, p)[0]; p += 4 + 4 * ngr
        self.off = []
        for s in sizes: self.off.append(p); p += s
        self.n = nblk

    def tname(self, i): return self.types[self.btype[i] & 0x7FFF]

    def avobject(self, p):
        # NiObjectNET then NiAVObject: name, extra data, controller, flags, transform, collision
        d = self.d
        name = struct.unpack_from('<i', d, p)[0]; p += 4
        nx = struct.unpack_from('<I', d, p)[0]; p += 4 + 4 * nx
        p += 4                                                   # controller
        p += 4 if self.bsver > 26 else 2                         # flags
        t = struct.unpack_from('<3f', d, p); p += 12
        r = struct.unpack_from('<9f', d, p); p += 36
        s = struct.unpack_from('<f', d, p)[0]; p += 4
        if self.bsver <= 34:
            np_ = struct.unpack_from('<I', d, p)[0]; p += 4 + 4 * np_
        p += 4                                                   # collision
        nm = self.strings[name] if 0 <= name < len(self.strings) else ''
        return p, nm, (t, r, s)

    def texture(self, shader):
        # BSLightingShaderProperty / BSEffectShaderProperty -> its first texture
        self.last_double = False
        if shader < 0 or shader >= self.n: return ''
        d = self.d; p = self.off[shader]; tn = self.tname(shader)
        try:
            if tn == 'BSLightingShaderProperty':
                if self.bsver < 130: p += 4                      # shader type
                p += 4; nx = struct.unpack_from('<I', d, p)[0]; p += 4 + 4 * nx; p += 4
                self.last_double = bool(struct.unpack_from('<I', d, p + 4)[0] & 0x10)   # (flags 2: Double_Sided)
                p += 8 + 16                                      # flags, uv offset, uv scale
                ts = struct.unpack_from('<i', d, p)[0]
                if ts < 0 or ts >= self.n: return ''
                q = self.off[ts]
                cnt = struct.unpack_from('<I', d, q)[0]; q += 4
                if cnt == 0: return ''
                ln = struct.unpack_from('<I', d, q)[0]
                return d[q + 4:q + 4 + ln].decode('latin-1')
            if tn == 'BSEffectShaderProperty':
                p += 4; nx = struct.unpack_from('<I', d, p)[0]; p += 4 + 4 * nx; p += 4
                self.last_double = bool(struct.unpack_from('<I', d, p + 4)[0] & 0x10)
                p += 8 + 16
                ln = struct.unpack_from('<I', d, p)[0]
                return d[p + 4:p + 4 + ln].decode('latin-1')
        except Exception: pass
        return ''

    def alpha_mode(self, a):
        # NiAlphaProperty -> 'test N' (cut out where the texture's alpha is under N), 'blend', or '' (none: the
        # texture's alpha is then not transparency - a sign's paint, a mask for the shine)
        if a < 0 or a >= self.n or self.tname(a) != 'NiAlphaProperty': return ''
        d = self.d; p = self.off[a]
        try:
            p += 4; nx = struct.unpack_from('<I', d, p)[0]; p += 4 + 4 * nx; p += 4
            fl = struct.unpack_from('<H', d, p)[0]; th = d[p + 2]
        except Exception: return ''
        if fl & 0x200: return 'test %d' % th
        if fl & 1: return 'blend'
        return ''

    def shape(self, i):
        d = self.d; p, nm, xf = self.avobject(self.off[i])
        p += 16                                                  # bounding sphere
        skin, shader, alpha = struct.unpack_from('<iii', d, p); p += 12
        self.last_alpha = self.alpha_mode(alpha)
        desc = struct.unpack_from('<Q', d, p)[0]; p += 8
        if self.bsver >= 130: ntri = struct.unpack_from('<I', d, p)[0]; p += 4
        else: ntri = struct.unpack_from('<H', d, p)[0]; p += 2
        nv = struct.unpack_from('<H', d, p)[0]; p += 2
        dsize = struct.unpack_from('<I', d, p)[0]; p += 4
        vsize = (desc & 15) * 4
        uvo = ((desc >> 8) & 15) * 4
        colo = ((desc >> 24) & 15) * 4
        attr = (desc >> 44) & 0xFFF
        full = (attr & 0x400) or self.bsver < 130
        verts, uvs, cols = [], [], []
        if dsize and vsize:
            for k in range(nv):
                q = p + k * vsize
                if full: verts.append(struct.unpack_from('<3f', d, q))
                else: verts.append(tuple(half(v) for v in struct.unpack_from('<3H', d, q)))
                if attr & 2: uvs.append(tuple(half(v) for v in struct.unpack_from('<2H', d, q + uvo)))
                if attr & 0x20: cols.append(struct.unpack_from('<4B', d, q + colo))
            p += nv * vsize
        tris = [struct.unpack_from('<3H', d, p + 6 * k) for k in range(ntri)] if dsize else []
        if not dsize and skin >= 0:
            # a skinned shape (the trees' swaying branches, books, banners): its vertices live in the skin partition
            try: verts, uvs, cols, tris = self.skinned(skin)
            except Exception: pass
        return nm, xf, verts, uvs, cols, tris, self.texture(shader)

    def skinned(self, skin):
        # NiSkinInstance -> NiSkinPartition (Skyrim SE): the vertex block, then the partitions' triangles
        d = self.d; q = self.off[skin]
        sdata, spart = struct.unpack_from('<ii', d, q)
        if spart < 0 or spart >= self.n or self.tname(spart) != 'NiSkinPartition': return [], [], [], []
        p = self.off[spart]
        nparts = struct.unpack_from('<I', d, p)[0]; p += 4
        dsize, vsize = struct.unpack_from('<II', d, p); p += 8
        desc = struct.unpack_from('<Q', d, p)[0]; p += 8
        vstart = p; p += dsize
        uvo = ((desc >> 8) & 15) * 4; colo = ((desc >> 24) & 15) * 4
        attr = (desc >> 44) & 0xFFF
        full = True                                                # the skin's own copy keeps full floats
        nv = dsize // vsize if vsize else 0
        verts, uvs, cols = [], [], []
        for k in range(nv):
            r = vstart + k * vsize
            if full: verts.append(struct.unpack_from('<3f', d, r))
            else: verts.append(tuple(half(v) for v in struct.unpack_from('<3H', d, r)))
            if attr & 2: uvs.append(tuple(half(v) for v in struct.unpack_from('<2H', d, r + uvo)))
            if attr & 0x20: cols.append(struct.unpack_from('<4B', d, r + colo))
        tris = []
        for k in range(nparts):
            pnv, pnt, pnb, pns, pnw = struct.unpack_from('<5H', d, p); p += 10
            p += 2 * pnb                                                   # bones
            hasmap = d[p]; p += 1
            vmap = struct.unpack_from('<%dH' % pnv, d, p) if hasmap else list(range(pnv)); p += 2 * pnv if hasmap else 0
            hasw = d[p]; p += 1
            if hasw: p += 4 * pnv * pnw
            slen = struct.unpack_from('<%dH' % pns, d, p) if pns else (); p += 2 * pns
            hasf = d[p]; p += 1
            if hasf:
                # (the partition's own triangles; the copy at its end, in the shared vertices' numbers, is what is used)
                if pns: p += 2 * sum(slen)
                else: p += 6 * pnt
            hasb = d[p]; p += 1
            if hasb: p += pnv * pnw
            p += 1 + 1 + 8                                                 # lod level, global vertex buffer, vertex desc
            for j in range(pnt):
                t3 = struct.unpack_from('<3H', d, p + 6 * j)
                if max(t3) < nv: tris.append(t3)
            p += 6 * pnt
        return verts, uvs, cols, tris

    def children(self, i):
        p, nm, xf = self.avobject(self.off[i])
        nc = struct.unpack_from('<I', self.d, p)[0]
        return xf, struct.unpack_from('<%di' % nc, self.d, p + 4)

def xf_apply(xf, v):
    t, r, s = xf
    return (t[0] + s * (r[0] * v[0] + r[1] * v[1] + r[2] * v[2]),
            t[1] + s * (r[3] * v[0] + r[4] * v[1] + r[5] * v[2]),
            t[2] + s * (r[6] * v[0] + r[7] * v[1] + r[8] * v[2]))

SHAPES = ('BSTriShape', 'BSLODTriShape', 'BSMeshLODTriShape', 'BSDynamicTriShape', 'BSSubIndexTriShape')
def nif_shapes(data):
    nif = NIF(data); out = []; nif.alphas = []; nif.doubles = []
    def visit(i, chain, depth):
        if i < 0 or i >= nif.n or depth > 40: return
        tn = nif.tname(i)
        if tn in SHAPES:
            nm, xf, verts, uvs, cols, tris, tex = nif.shape(i)
            if tn == 'BSDynamicTriShape': return                 # its vertices live elsewhere (heads); not needed
            pts = verts
            for x in [xf] + chain: pts = [xf_apply(x, v) for v in pts]
            out.append((nm, tex, pts, uvs, cols, tris)); nif.alphas.append(getattr(nif, 'last_alpha', '')); nif.doubles.append(getattr(nif, 'last_double', False))
        elif 'Node' in tn or tn in ('BSFadeNode', 'BSLeafAnimNode', 'BSTreeNode', 'BSOrderedNode', 'BSMultiBoundNode'):
            try: xf, kids = nif.children(i)
            except Exception: return
            nc = [xf] + chain
            for k in kids: visit(k, nc, depth + 1)
    visit(0, [], 0)
    return nif, out

# ---------------------------------------------------------------- DDS: the average colour of the smallest full mip
def rgb565(c): return (((c >> 11) & 31) * 255 // 31, ((c >> 5) & 63) * 255 // 63, (c & 31) * 255 // 31)
def bc1_block(b):
    c0, c1, bits = struct.unpack('<HHI', b)
    a, bb = rgb565(c0), rgb565(c1)
    pal = [a, bb] + ([tuple((2 * a[i] + bb[i]) // 3 for i in range(3)), tuple((a[i] + 2 * bb[i]) // 3 for i in range(3))] if c0 > c1
                     else [tuple((a[i] + bb[i]) // 2 for i in range(3)), (0, 0, 0)])
    return [pal[(bits >> (2 * k)) & 3] for k in range(16)]

# (v0.4.74) an uncompressed DDS's channel order, from its pixel format's red mask (offset 92): 0x000000FF - red first
# (A8B8G8R8, B8G8R8); else blue first (A8R8G8B8, R8G8B8: red in 0x00FF0000). (It was always read blue first: the
# textures Skyrim keeps red first - Whiterun's interior wood, wrplasterint01low - came out with red and blue swapped)
def dds_rgb_first(data):
    if len(data) < 128 or data[84:88] != b'\0\0\0\0': return False
    return struct.unpack_from('<I', data, 92)[0] == 0xFF

def dds_colour(data):
    if data[:4] != b'DDS ': return None
    h, w = struct.unpack_from('<II', data, 12)
    mips = max(1, struct.unpack_from('<I', data, 28)[0])
    four = data[84:88]; p = 128; fmt = four.decode('latin-1', 'replace')
    bpp = 0; blk = 0
    if four == b'DXT1': blk = 8
    elif four in (b'DXT3', b'DXT5'): blk = 16
    elif four == b'DX10':
        dxgi = struct.unpack_from('<I', data, 128)[0]; p += 20; fmt = 'dxgi%d' % dxgi
        if dxgi in (71, 72): blk = 8
        elif dxgi in (74, 75, 77, 78): blk = 16
        elif dxgi in (98, 99): return ('bc7', w, h, None)
        elif dxgi in (28, 29, 87, 91): bpp = 4
        else: return (fmt, w, h, None)
    else:
        bits = struct.unpack_from('<I', data, 88)[0]
        if bits in (24, 32): bpp = bits // 8; fmt = 'rgb%d' % bits
        else: return (fmt, w, h, None)
    rgb_first = four == b'DX10' or dds_rgb_first(data)
    # walk the mip chain to the last level that is still at least 4 x 4
    lw, lh = w, h
    for k in range(mips):
        sz = (max(1, (lw + 3) // 4) * max(1, (lh + 3) // 4) * blk) if blk else lw * lh * bpp
        if k == mips - 1 or lw < 8 or lh < 8: break
        p += sz; lw = max(1, lw // 2); lh = max(1, lh // 2)
    px = []
    if blk:
        nb = max(1, (lw + 3) // 4) * max(1, (lh + 3) // 4)
        for b in range(min(nb, 64)):
            q = p + b * blk
            px += bc1_block(data[q + (8 if blk == 16 else 0):q + (16 if blk == 16 else 8)])
    else:
        for k in range(min(lw * lh, 1024)):
            q = p + k * bpp
            if rgb_first: px.append((data[q], data[q + 1], data[q + 2]))
            else: px.append((data[q + 2], data[q + 1], data[q]))
    if not px: return (fmt, w, h, None)
    lum = sorted(px, key=lambda c: c[0] * 3 + c[1] * 6 + c[2])
    def avg(l): return tuple(sum(c[i] for c in l) // len(l) for i in range(3))
    return (fmt, w, h, (avg(px), avg(lum[:len(lum) // 2] or lum), avg(lum[len(lum) // 2:] or lum)))

# a texture as a small picture: the mip level whose longer side is 64 (or the smallest there is above that), decoded
# whole (BC1, the colour half of BC2 / BC3, plain 24 / 32-bit) -> (width, height, [(r, g, b)] row by row) or None
def dds_image(data, side=64):
    if data[:4] != b'DDS ': return None
    h, w = struct.unpack_from('<II', data, 12)
    mips = max(1, struct.unpack_from('<I', data, 28)[0])
    four = data[84:88]; p = 128; bpp = 0; blk = 0; bgr = True
    if four == b'DXT1': blk = 8
    elif four in (b'DXT3', b'DXT5'): blk = 16
    elif four == b'DX10':
        dxgi = struct.unpack_from('<I', data, 128)[0]; p += 20
        if dxgi in (71, 72): blk = 8
        elif dxgi in (74, 75, 77, 78): blk = 16
        elif dxgi in (28, 29): bpp = 4; bgr = False
        elif dxgi in (87, 91): bpp = 4
        else: return None
    else:
        bits = struct.unpack_from('<I', data, 88)[0]
        if bits in (24, 32): bpp = bits // 8; bgr = not dds_rgb_first(data)
        else: return None
    lw, lh = w, h
    for k in range(mips):
        if max(lw, lh) <= side or k == mips - 1: break
        nw, nh = max(1, lw // 2), max(1, lh // 2)
        if max(nw, nh) < side: break
        p += (max(1, (lw + 3) // 4) * max(1, (lh + 3) // 4) * blk) if blk else lw * lh * bpp
        lw, lh = nw, nh
    if lw < 4 or lh < 4: return None
    px = [(0, 0, 0)] * (lw * lh); al = [255] * (lw * lh)
    dxt3 = four == b'DXT3' or (four == b'DX10' and struct.unpack_from('<I', data, 128)[0] in (74, 75))
    if blk:
        bw = (lw + 3) // 4
        for by in range((lh + 3) // 4):
            for bx in range(bw):
                q0 = p + (by * bw + bx) * blk; q = q0 + (8 if blk == 16 else 0)
                if q + 8 > len(data): return None
                c = bc1_block(data[q:q + 8])
                c0, c1, bits = struct.unpack_from('<HHI', data, q)
                if blk == 8: a = [0 if (c0 <= c1 and ((bits >> (2 * k)) & 3) == 3) else 255 for k in range(16)]
                elif dxt3:
                    ab = struct.unpack_from('<Q', data, q0)[0]; a = [((ab >> (4 * k)) & 15) * 17 for k in range(16)]
                else:
                    a0, a1 = data[q0], data[q0 + 1]; ai = int.from_bytes(data[q0 + 2:q0 + 8], 'little')
                    pal = [a0, a1] + ([((7 - j) * a0 + j * a1) // 7 for j in range(1, 7)] if a0 > a1 else
                                      [((5 - j) * a0 + j * a1) // 5 for j in range(1, 5)] + [0, 255])
                    a = [pal[(ai >> (3 * k)) & 7] for k in range(16)]
                for k in range(16):
                    x, y = bx * 4 + (k & 3), by * 4 + (k >> 2)
                    if x < lw and y < lh: px[y * lw + x] = c[k]; al[y * lw + x] = a[k]
    else:
        for k in range(lw * lh):
            q = p + k * bpp
            if q + 3 > len(data): return None
            px[k] = (data[q + 2], data[q + 1], data[q]) if bgr else (data[q], data[q + 1], data[q + 2])
            if bpp == 4 and q + 4 <= len(data): al[k] = data[q + 3]
    return (lw, lh, px, al if min(al) < 250 else None)

def texture_path(t):
    # a texture's path in the archives: some meshes name it from the artist's own folders
    # (skyrimhd\build\pc\data\textures\...); the archives keep it under textures\...
    t = t.lower().replace('/', '\\')
    k = t.find('textures\\')
    if k > 0: t = t[k:]
    return t if t.startswith('textures\\') else 'textures\\' + t

def save_texture(outdir, p, dd, errors):
    # the texture small: textures\<its path>.ppm, and its transparency (cut-out leaves, branches, fences) as
    # <its path>_a.pgm when it has any; -> its colours (as dds_colour)
    res = dds_colour(dd)
    im = dds_image(dd)
    if im:
        iw, ih, ipx, ial = im
        stem = re.sub(r'[^A-Za-z0-9_.-]', '_', p[len('textures\\'):].replace('\\', '_'))[:-4]
        with open(os.path.join(outdir, 'textures', stem + '.ppm'), 'wb') as tf:
            tf.write(b'P6 %d %d 255\n' % (iw, ih)); tf.write(bytes(v for c in ipx for v in c))
        if ial:
            with open(os.path.join(outdir, 'textures', stem + '_a.pgm'), 'wb') as tf:
                tf.write(b'P5 %d %d 255\n' % (iw, ih)); tf.write(bytes(ial))
    elif res: errors.append('%s: %s %dx%d, not saved as a picture' % (p, res[0], res[1], res[2]))
    # and larger, for the faces seen close (the game draws the most used at 128 x 128) and the signs (256: their
    # lettering): <its path>_128.ppm / _256.ppm, their transparency _128_a.pgm / _256_a.pgm. (v0.4.46: 256 too for
    # every texture Skyrim made 2048 or more - the pelts, 4096: the game draws the ones with the biggest texels at 256)
    big = bool(res) and max(res[1], res[2]) >= 2048
    for side in ((128, 256) if 'signage' in p or 'roadsign' in p or big else (128,)):
        imh = dds_image(dd, side)
        if not imh or max(imh[0], imh[1]) < side: continue
        iw, ih, ipx, ial = imh
        stem = re.sub(r'[^A-Za-z0-9_.-]', '_', p[len('textures\\'):].replace('\\', '_'))[:-4] + '_%d' % side
        with open(os.path.join(outdir, 'textures', stem + '.ppm'), 'wb') as tf:
            tf.write(b'P6 %d %d 255\n' % (iw, ih)); tf.write(bytes(v for c in ipx for v in c))
        if ial:
            with open(os.path.join(outdir, 'textures', stem + '_a.pgm'), 'wb') as tf:
                tf.write(b'P5 %d %d 255\n' % (iw, ih)); tf.write(bytes(ial))
    return res

# the things one carries (items.csv lists them all), and those the game has, by their editor ids (their models are
# taken whether or not one lies in the area)
ITEM_TYPES = ('WEAP', 'ARMO', 'AMMO', 'MISC', 'INGR', 'ALCH', 'BOOK', 'KEYM', 'SLGM', 'LIGH', 'SCRL')
ITEM_EDIDS = ['IronSword', 'SteelSword', 'IronWarAxe', 'IronDagger', 'DraugrSword', 'DraugrWarAxe', 'IronMace',
    'IronGreatsword', 'IronBattleaxe', 'IronWarhammer', 'HuntingBow', 'LongBow', 'IronArrow', 'SteelDagger',
    'ArmorBanditCuirass', 'ArmorBanditHelmet', 'ArmorBanditGauntlets', 'ArmorBanditBoots', 'ArmorStormcloakBoots',
    'ArmorStormcloakGauntlets', 'ArmorIronCuirass', 'ArmorIronHelmet', 'ArmorIronBoots', 'ArmorIronGauntlets',
    'ArmorIronShield', 'ArmorHideCuirass', 'ArmorHideHelmet', 'ArmorHideBoots', 'ArmorHideGauntlets', 'ArmorHideShield',
    'ArmorLeatherCuirass', 'ArmorLeatherHelmet', 'ArmorLeatherBoots', 'ArmorLeatherGauntlets', 'ArmorStuddedCuirass',
    'JewelryRingSilver', 'JewelryRingGold', 'JewelryNecklaceSilver', 'JewelryNecklaceGold',
    'RestoreHealth01', 'RestoreHealth02', 'RestoreMagicka01', 'RestoreStamina01', 'CureDisease', 'DamageHealth01',
    'ResistFire25', 'FoodBread01A', 'FoodApple', 'FoodCabbage', 'FoodLeek', 'FoodPotato', 'FoodCarrot', 'FoodVenison',
    'FoodSalmon', 'FoodBeef', 'FoodSalmonCooked', 'FoodBeefCooked', 'FoodVegetableSoup', 'FoodCabbagePotatoSoup',
    'FoodAppleCabbageStew', 'FoodVenisonStew', 'FoodRabbit', 'FoodPheasant', 'FoodGoatMeat',
    'FrostMirriam', 'Garlic', 'SaltPile', 'BoneMeal', 'MountainFlower01Blue', 'MountainFlower01Purple',
    'MountainFlower01Red', 'JuniperBerries', 'FireSalts', 'VoidSalts', 'glowDust', 'HagravenFeathers', 'TrollFat',
    'SkeeverTail', 'SpiderEgg', 'DragonflyOrange', 'DragonflyBlue', 'Taproot', 'SlaughterfishScales', 'IceWraithTeeth',
    'Ectoplasm', 'SprigganSap', 'FrostbiteVenom', 'Thistle01', 'Snowberry', 'AntlersSmall', 'AntlersLarge', 'MudcrabChitin',
    'WolfPelt', 'DeerHide', 'FoxPelt', 'GoatHide', 'GemGarnet', 'GemAmethyst', 'BasicTankard01', 'BasicWoodenBowl01',
    'Lockpick', 'IngotIron', 'Torch01', 'MS13GoldenClaw', 'MQ103Dragonstone', 'MS13ArvelsJournal',
    'Book3ValuableDragonborn', 'MG03Book01', 'SkillAlchemy5', 'DragonScales', 'TrollPelt' ]

# ---------------------------------------------------------------- main
def main():
    data_dir, _ = data_arg(sys.argv[1:])
    if not os.path.exists(os.path.join(data_dir, 'Skyrim.esm')): print('Skyrim.esm not found in', data_dir, '- give the Data folder: skyrim_meshes.py path\\to\\Data'); return
    outdir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'skyrim_meshes')
    for sub in ('meshes', 'raw', 'textures'): os.makedirs(os.path.join(outdir, sub), exist_ok=True)
    errors = []
    print('reading Skyrim.esm')
    base, cell_bases, area = read_esm(os.path.join(data_dir, 'Skyrim.esm'))
    # the interiors built from the farmhouse set: most of their architecture pieces come from it
    want = collections.defaultdict(set)                          # model -> editor ids
    farm = 0
    for cell, bc in cell_bases.items():
        arch = collections.Counter()
        for b, n in bc.items():
            m = base.get(b, ('', '', ''))[2]
            fo = folder(m)
            if fo.startswith(('architecture', 'dungeons')): arch[fo.split('\\')[0] + '\\' + fo.split('\\')[1] if '\\' in fo else fo] += n
        if arch and arch.most_common(1)[0][0] == 'architecture\\farmhouse':
            farm += 1
            for b in bc:
                t, e, m = base.get(b, ('', '', ''))
                if m and not e.startswith(('XMarker', 'COCMarker', 'NorthMarker')) and ('marker' not in m.lower() or m.lower().startswith('furniture')): want[m].add(e)
    for b in area:
        t, e, m = base.get(b, ('', '', ''))
        # (the furniture "markers" carry the thing itself too: the forge, the tanning rack)
        if m and ('marker' not in m.lower() or m.lower().startswith('furniture')): want[m].add(e)
    for b, (t, e, m) in base.items():
        if m and folder(m).startswith('architecture\\farmhouse'): want[m].add(e)
    # the things one carries: every one's record and model listed (items.csv); the models of those the game has (and
    # any editor ids given in a file items.txt next to this script, one a line) taken too
    item_ids = set(ITEM_EDIDS)
    extra = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'items.txt')
    if os.path.exists(extra): item_ids |= set(l.strip() for l in open(extra) if l.strip())
    with open(os.path.join(outdir, 'items.csv'), 'w', encoding='utf-8') as f:
        f.write('type,editor_id,model\n')
        for b, (t, e, m) in sorted(base.items()):
            if t in ITEM_TYPES and e: f.write('%s,%s,"%s"\n' % (t, e, ''.join(ch for ch in m if 32 <= ord(ch) < 127 and ch != '"')))
            if e in item_ids and m: want[m].add(e)
    print('farmhouse interiors %d, models wanted %d' % (farm, len(want)))
    # the archives
    meshes, textures = [], []
    for fn in sorted(os.listdir(data_dir)):
        if fn.lower().endswith('.bsa') and fn.lower().startswith('skyrim'):
            try:
                a = BSA(os.path.join(data_dir, fn))
                (meshes if 'mesh' in fn.lower() else textures if 'texture' in fn.lower() else []).append(a)
                print('  %s: %d files' % (fn, len(a.files)))
            except Exception as ex: errors.append('%s: %s' % (fn, ex))
    def find(arcs, name):
        name = name.replace('/', '\\').lower()
        for a in arcs:
            if name in a.files: return a
        return None
    rows = []; texused = collections.Counter(); raw_kept = 0; failed_kept = 0; failed_tree = 0
    for k, (model, eds) in enumerate(sorted(want.items())):
        path = 'meshes\\' + model.replace('/', '\\').lower().lstrip('\\')
        if path.startswith('meshes\\meshes\\'): path = path[7:]
        a = find(meshes, path)
        if a is None:
            loose = os.path.join(data_dir, path)
            data = open(loose, 'rb').read() if os.path.exists(loose) else None
        else:
            try: data = a.read(path)
            except Exception as ex: errors.append('%s: %s' % (path, ex)); data = None
        if data is None:
            rows.append((';'.join(sorted(eds)), model, 'not found', 0, 0, 0, '')); continue
        stem = re.sub(r'[^A-Za-z0-9_.-]', '_', model.replace('\\', '_').replace('/', '_'))
        if raw_kept < 8 and folder(model).startswith('architecture\\farmhouse') and len(data) < 400000:
            open(os.path.join(outdir, 'raw', stem), 'wb').write(data); raw_kept += 1
        try:
            nif, shapes = nif_shapes(data)
        except Exception:
            errors.append('%s:\n%s' % (path, traceback.format_exc())); rows.append((';'.join(sorted(eds)), model, 'unreadable', 0, 0, 0, '')); continue
        nv = sum(len(s[2]) for s in shapes); nt = sum(len(s[5]) for s in shapes)
        bb = ''
        if nv:
            xs = [v[0] for s in shapes for v in s[2]]; ys = [v[1] for s in shapes for v in s[2]]; zs = [v[2] for s in shapes for v in s[2]]
            bb = '%d %d %d %d %d %d' % (min(xs), min(ys), min(zs), max(xs), max(ys), max(zs))
        with open(os.path.join(outdir, 'meshes', stem[:-4] + '.obj'), 'w') as f:
            f.write('# %s (%s)\n' % (model, ', '.join(sorted(eds)[:6])))
            vb = 1
            for k_, (nm, tex, pts, uvs, cols, tris) in enumerate(shapes):
                texused[tex.lower()] += 1
                f.write('o %s\nusemtl %s\n' % (nm or 'shape', tex or '-'))
                if k_ < len(nif.alphas) and nif.alphas[k_]: f.write('# alpha %s\n' % nif.alphas[k_])
                if k_ < len(nif.doubles) and nif.doubles[k_]: f.write('# double sided\n')
                if cols:
                    c = [sum(x[i] for x in cols) // len(cols) for i in range(4)]
                    f.write('# vertex colour average %d %d %d %d\n' % tuple(c))
                for v in pts: f.write('v %.1f %.1f %.1f\n' % v)
                for uv in uvs: f.write('vt %.3f %.3f\n' % uv)
                if uvs:
                    for t in tris: f.write('f %d/%d %d/%d %d/%d\n' % (t[0] + vb, t[0] + vb, t[1] + vb, t[1] + vb, t[2] + vb, t[2] + vb))
                else:
                    for t in tris: f.write('f %d %d %d\n' % (t[0] + vb, t[1] + vb, t[2] + vb))
                vb += len(pts)
        rows.append((';'.join(sorted(eds)), model, 'ok' if nv else 'no shapes (%s)' % ','.join(sorted(set(nif.types))[:6]), len(shapes), nv, nt, bb))
        tree = 'trees' in path
        if not nv and (failed_kept < 12 or (tree and failed_tree < 6)) and len(data) < 3000000:
            if tree: failed_tree += 1
            # kept to find out why (the trees: their shapes are not read yet)
            os.makedirs(os.path.join(outdir, 'raw_failed'), exist_ok=True)
            open(os.path.join(outdir, 'raw_failed', stem), 'wb').write(data); failed_kept += 1
            errors.append('%s: no shapes; block types: %s' % (path, ', '.join(sorted(set(nif.types)))))
        if k % 100 == 0: print('  models %d / %d' % (k, len(want)))
    with open(os.path.join(outdir, 'models.csv'), 'w', encoding='utf-8') as f:
        f.write('objects,model,status,shapes,vertices,triangles,bounds\n')
        for r in rows: f.write('"%s","%s",%s,%d,%d,%d,%s\n' % r)
    print('textures:', len(texused))
    with open(os.path.join(outdir, 'textures.csv'), 'w', encoding='utf-8') as f:
        f.write('texture,used_by_shapes,format,width,height,average_rgb,darker_half_rgb,lighter_half_rgb\n')
        for t, n in texused.most_common():
            if not t: continue
            p = texture_path(t)
            a = find(textures, p); res = None
            try:
                if a is not None: res = save_texture(outdir, p, a.read(p), errors)
            except Exception as ex: errors.append('%s: %s' % (p, ex))
            if res is None: f.write('%s,%d,missing,,,,,\n' % (t, n)); continue
            fmt, w, h, cols = res
            cs = ['%02X%02X%02X' % c for c in cols] if cols else ['', '', '']
            f.write('%s,%d,%s,%d,%d,%s,%s,%s\n' % (t, n, fmt, w, h, cs[0], cs[1], cs[2]))
    with open(os.path.join(outdir, 'errors.txt'), 'w') as f: f.write('\n'.join(errors) if errors else 'none\n')
    print('done ->', outdir, '(%d problems, see errors.txt)' % len(errors))
    make_zip(outdir)

def make_zip(outdir):
    # pack the output folder into <folder>.zip next to it, ready to send
    zp = outdir + '.zip'
    with zipfile.ZipFile(zp, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for root, dirs, files in os.walk(outdir):
            for fn in sorted(files):
                full = os.path.join(root, fn)
                z.write(full, os.path.relpath(full, os.path.dirname(outdir)))
    print('zipped ->', zp, '(%.1f MB) - send this file' % (os.path.getsize(zp) / 1048576.0))

if __name__ == '__main__':
    main()
