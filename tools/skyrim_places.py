#!/usr/bin/env python3
# Export Skyrim's placed things (doors, buildings, trees, map markers, Whiterun's own city space) from
# Skyrim.esm, for Hrimfell. The companion of skyrim_heightmap.py.
#
# Usage (Windows, Python 3.8+ from python.org, nothing else needed):
#   python skyrim_places.py
# Skyrim is looked for in C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\Data; elsewhere,
# give its path first (the Data folder, or Skyrim.esm in it).
# Takes a few minutes and about 2 GB of memory.
#
# Writes into a folder "skyrim_places" next to this script:
#   doors.csv              every door placed in Tamriel and in Whiterun's city space that leads somewhere:
#                          where it stands, which way it faces, and which cell (and which spot) it leads to
#   refs_area.csv          everything placed in Tamriel between Whiterun, Riverwood, Bleak Falls Barrow and
#                          Helgen (cells x -8..14, y -22..4): what it is (type, editor id, size) and where
#   refs_whiterun.csv      everything placed in Whiterun's own city space (WhiterunWorld)
#   refs_interiors.csv     everything placed inside the barrow, Whiterun's and Riverwood's houses (by cell name)
#   whiterun_height.raw    Whiterun's city space heights (same format as tamriel_height.raw) + whiterun_info.txt
#   markers.csv            the map markers of Tamriel (position, kind, name if the strings file is found)
#   trees.pgm              trees per 7.3 m square over all of Tamriel (8 x 8 per cell, north at the top)
#   rocks.pgm              the same for rocks, boulders and cliffs
#   ground_full.pgm        the strongest ground texture at every vertex (3809 x 3009, like the heights)
#                          + ground_full.txt (the texture names)
#   names.csv              location and cell names, when the strings file is found
# At the end the folder is packed into a .zip next to it (e.g. skyrim_places.zip): send that file.
#
# Only Skyrim.esm (and, if present, Data\Strings\Skyrim_English.STRINGS) is read.
import os, sys, struct, zlib, math, re

DEFAULT_DATA = r'C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\Data'

def esm_arg(args):
    # the first argument when it is a path (Skyrim.esm or the Data folder); otherwise Skyrim.esm in the usual Steam
    # folder. -> (path to Skyrim.esm, the remaining arguments)
    if args and (args[0].lower().endswith('.esm') or os.path.isdir(args[0]) or '\\' in args[0] or '/' in args[0]):
        p, args = args[0], args[1:]
    else: p = DEFAULT_DATA
    if os.path.isdir(p): p = os.path.join(p, 'Skyrim.esm')
    return p, args

GRID = 32
AREA = (-8, 14, -22, 4)          # cells x0, x1, y0, y1 (inclusive) for refs_area.csv
INTERIOR_PREFIXES = ('BleakFalls', 'Whiterun', 'Dragonsreach', 'Jorrvaskr', 'Riverwood', 'BanneredMare',
                     'Warmaidens', 'Belethor', 'Arcadia', 'Breezehome', 'HallOfTheDead', 'Drunken',
                     'Skyforge', 'Sleeping', 'Embershard', 'Helgen', 'Ysolda', 'Carlotta', 'AmrensHouse',
                     'Uthgerd', 'HouseGrayMane', 'HouseBattleBorn', 'Olava', 'Severio', 'Heimskr',
                     'Fralia', 'Proventus', 'Nazeem', 'Underforge', 'Gerdur', 'Alvor', 'Hod', 'Faendal',
                     'Sven', 'Lucan')
TREE_RE = re.compile(r'tree|pine|aspen|birch|juniper', re.I)
ROCK_RE = re.compile(r'rock|boulder|cliff|mountain', re.I)

def subrecords(data):
    out = []
    p = 0
    big = None
    while p + 6 <= len(data):
        t = data[p:p + 4]
        n = struct.unpack_from('<H', data, p + 4)[0]
        p += 6
        if big is not None: n = big; big = None
        if t == b'XXXX':
            big = struct.unpack_from('<I', data, p)[0]
            p += n
            continue
        out.append((t, data[p:p + n]))
        p += n
    return out

def zstr(d): return d.split(b'\0')[0].decode('latin-1')

def load_strings(esm):
    p = os.path.join(os.path.dirname(os.path.abspath(esm)), 'Strings', 'Skyrim_English.STRINGS')
    if not os.path.exists(p): return {}
    with open(p, 'rb') as f: b = f.read()
    n, size = struct.unpack_from('<II', b, 0)
    base = 8 + n * 8
    out = {}
    for i in range(n):
        sid, off = struct.unpack_from('<II', b, 8 + i * 8)
        e = b.find(b'\0', base + off)
        out[sid] = b[base + off:e].decode('cp1252', 'replace')
    print('strings:', len(out))
    return out

def main():
    path, _ = esm_arg(sys.argv[1:])
    if not os.path.exists(path): print('Skyrim.esm not found at', path, '- give its path: skyrim_places.py path\\to\\Skyrim.esm'); return
    print('reading', path)
    with open(path, 'rb') as f: buf = f.read()
    strings = load_strings(path)
    def name(d):
        if len(d) == 4 and strings:
            return strings.get(struct.unpack('<I', d)[0], '')
        return zstr(d) if len(d) != 4 else ''
    outdir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'skyrim_places')
    os.makedirs(outdir, exist_ok=True)

    base = {}            # fid -> (type, edid, obnd)
    worlds = {}          # fid -> edid
    ltex = {}
    refcell = {}         # ref fid -> (world edid or '', cell edid, cell x, cell y)
    doors = []           # (world, cell, fid, base, pos, rot, dest fid, dest pos, dest rot)
    area, wrun, inter, markers = [], [], [], []
    st = {'world': '', 'cell': None}
    wland = {}           # WhiterunWorld (x, y) -> heights
    tland = {}           # Tamriel (x, y) -> per vertex texture index (17x17 per quarter merged to 33x33)
    names = []
    tex_idx = {0: 0}; tex_names = ['(none)']
    trees, rocks = {}, {}

    def tex_id(fid):
        if fid not in tex_idx:
            tex_idx[fid] = len(tex_names); tex_names.append(ltex.get(fid, '#%08X' % fid))
        return tex_idx[fid]

    def refrow(typ, fid, subs):
        b = 0; pos = rot = None; sc = 1.0; tel = None; mk = None; full = ''
        for t, d in subs:
            if t == b'NAME' and len(d) >= 4: b = struct.unpack_from('<I', d)[0]
            elif t == b'DATA' and len(d) >= 24: v = struct.unpack_from('<6f', d); pos, rot = v[:3], v[3:]
            elif t == b'XSCL' and len(d) >= 4: sc = struct.unpack_from('<f', d)[0]
            elif t == b'XTEL' and len(d) >= 28: tel = struct.unpack_from('<I6f', d)
            elif t == b'XMRK': mk = {}
            elif t == b'FULL' and mk is not None: mk['name'] = name(d)
            elif t == b'TNAM' and mk is not None and len(d) >= 1: mk['kind'] = d[0]
        return b, pos, rot, sc, tel, mk

    def deg(r): return '%.1f' % (math.degrees(r) % 360.0)

    def walk(pos, end):
        while pos < end:
            typ = buf[pos:pos + 4]
            size = struct.unpack_from('<I', buf, pos + 4)[0]
            if typ == b'GRUP':
                label = buf[pos + 8:pos + 12]
                gtype = struct.unpack_from('<i', buf, pos + 12)[0]
                saved = (st['world'], st['cell'])
                if gtype == 1: st['world'] = worlds.get(struct.unpack('<I', label)[0], '?')
                elif gtype == 0 and label == b'CELL': st['world'] = ''
                walk(pos + 24, pos + size)
                if gtype in (0, 1): st['world'], st['cell'] = saved[0], None
                pos += size
                continue
            flags, fid = struct.unpack_from('<II', buf, pos + 8)
            data = buf[pos + 24:pos + 24 + size]
            pos += 24 + size
            if typ not in (b'REFR', b'ACHR', b'CELL', b'LAND', b'WRLD', b'LTEX', b'NAVM', b'NAVI', b'LCTN') and typ[:1] != b'\0':
                if flags & 0x00040000:
                    try: data = zlib.decompress(data[4:])
                    except zlib.error: continue
                edid = ''; ob = None
                for t, d in subrecords(data):
                    if t == b'EDID': edid = zstr(d)
                    elif t == b'OBND' and len(d) >= 12: ob = struct.unpack_from('<6h', d)
                base[fid] = (typ.decode('latin-1'), edid, ob)
                continue
            if typ in (b'NAVM', b'NAVI'): continue
            if flags & 0x00040000:
                try: data = zlib.decompress(data[4:])
                except zlib.error: continue
            subs = subrecords(data)
            if typ == b'LTEX':
                for t, d in subs:
                    if t == b'EDID': ltex[fid] = zstr(d)
            elif typ == b'WRLD':
                for t, d in subs:
                    if t == b'EDID': worlds[fid] = zstr(d)
            elif typ == b'LCTN':
                e = ''; n = ''
                for t, d in subs:
                    if t == b'EDID': e = zstr(d)
                    elif t == b'FULL': n = name(d)
                names.append(('location', e, n, '', ''))
            elif typ == b'CELL':
                c = {'edid': '', 'x': None, 'y': None, 'name': ''}
                for t, d in subs:
                    if t == b'EDID': c['edid'] = zstr(d)
                    elif t == b'XCLC' and len(d) >= 8: c['x'], c['y'] = struct.unpack_from('<ii', d)
                    elif t == b'FULL': c['name'] = name(d)
                st['cell'] = c
                if c['name']: names.append(('cell', c['edid'], c['name'], c['x'] if c['x'] is not None else '', c['y'] if c['y'] is not None else ''))
            elif typ == b'LAND' and st['cell'] is not None and st['cell']['x'] is not None:
                c = st['cell']
                if st['world'] == 'WhiterunWorld':
                    for t, d in subs:
                        if t == b'VHGT' and len(d) >= 4 + 1089:
                            off = struct.unpack_from('<f', d, 0)[0] * 8
                            g = struct.unpack_from('<1089b', d, 4)
                            h = [0] * 1089; ro = 0.0
                            for i in range(1089):
                                r, col = divmod(i, 33)
                                v = g[i] * 8
                                if col == 0: off += v; ro = 0.0
                                else: ro += v
                                h[i] = off + ro
                            wland[(c['x'], c['y'])] = h
                elif st['world'] == 'Tamriel':
                    # per vertex: the base texture of the quarter, painted over by every layer above 50 %
                    grid = [0] * 1089
                    cur = None
                    for t, d in subs:
                        if t == b'BTXT' and len(d) >= 8:
                            tid, q = struct.unpack_from('<IB', d)
                            k = tex_id(tid); qx, qy = (q & 1) * 16, (q >> 1) * 16
                            for r in range(17):
                                for col in range(17): grid[(qy + r) * 33 + qx + col] = k
                        elif t == b'ATXT' and len(d) >= 8:
                            tid, q = struct.unpack_from('<IB', d); cur = (tex_id(tid), q)
                        elif t == b'VTXT' and cur is not None:
                            k, q = cur; qx, qy = (q & 1) * 16, (q >> 1) * 16
                            for j in range(0, len(d) - 7, 8):
                                p, _, _, op = struct.unpack_from('<HBBf', d, j)
                                if op > 0.5 and p < 289:
                                    r, col = divmod(p, 17)
                                    grid[(qy + r) * 33 + qx + col] = k
                            cur = None
                    tland[(c['x'], c['y'])] = grid
            elif typ in (b'REFR', b'ACHR'):
                b, p, r, sc, tel, mk = refrow(typ, fid, subs)
                c = st['cell'] or {'edid': '', 'x': None, 'y': None}
                w = st['world']
                refcell[fid] = (w, c['edid'], p)
                if p is None: continue
                bt, be, ob = base.get(b, ('?', '#%08X' % b, None))
                cx, cy = int(math.floor(p[0] / 4096)), int(math.floor(p[1] / 4096))
                row = (typ.decode(), bt, be, '%.1f' % p[0], '%.1f' % p[1], '%.1f' % p[2], deg(r[0]), deg(r[1]), deg(r[2]),
                       '%.3f' % sc, ob and '%d %d %d %d %d %d' % ob or '', w, c['edid'], cx, cy, '%08X' % fid)
                if tel is not None:
                    doors.append((w, c['edid'], fid, be, p, r, tel))
                if w == 'Tamriel':
                    if AREA[0] <= cx <= AREA[1] and AREA[2] <= cy <= AREA[3]: area.append(row)
                    key = (int(math.floor(p[0] / 512)), int(math.floor(p[1] / 512)))
                    if bt == 'TREE' or (bt in ('STAT', 'MSTT') and TREE_RE.search(be)): trees[key] = trees.get(key, 0) + 1
                    elif bt in ('STAT', 'MSTT') and ROCK_RE.search(be): rocks[key] = rocks.get(key, 0) + 1
                    if mk is not None: markers.append(('%.1f' % p[0], '%.1f' % p[1], '%.1f' % p[2], mk.get('kind', ''), mk.get('name', ''), c['edid']))
                elif w == 'WhiterunWorld': wrun.append(row)
                elif w == '' and c['edid'].startswith(INTERIOR_PREFIXES): inter.append(row)

    sys.setrecursionlimit(10000)
    walk(0, len(buf))
    print('base records %d, refs %d, doors %d' % (len(base), len(refcell), len(doors)))

    hdr = 'kind,base_type,base_editor_id,x,y,z,rot_x,rot_y,rot_z,scale,bounds,world,cell,cell_x,cell_y,ref_id\n'
    for fn, rows in (('refs_area.csv', area), ('refs_whiterun.csv', wrun), ('refs_interiors.csv', inter)):
        with open(os.path.join(outdir, fn), 'w', encoding='utf-8') as f:
            f.write(hdr)
            for r in rows: f.write(','.join(str(v) for v in r) + '\n')
    with open(os.path.join(outdir, 'doors.csv'), 'w', encoding='utf-8') as f:
        f.write('world,cell,ref_id,base_editor_id,x,y,z,rot_z,dest_world,dest_cell,dest_x,dest_y,dest_z,dest_rot_z\n')
        for w, c, fid, be, p, r, tel in doors:
            dw, dc, _ = refcell.get(tel[0], ('?', '?', None))
            f.write('%s,%s,%08X,%s,%.1f,%.1f,%.1f,%s,%s,%s,%.1f,%.1f,%.1f,%s\n' % (w, c, fid, be, p[0], p[1], p[2], deg(r[2]),
                    dw, dc, tel[1], tel[2], tel[3], deg(tel[6])))
    with open(os.path.join(outdir, 'markers.csv'), 'w', encoding='utf-8') as f:
        f.write('x,y,z,kind,name,cell\n')
        for m in markers: f.write(','.join('"%s"' % v if i == 4 else str(v) for i, v in enumerate(m)) + '\n')
    with open(os.path.join(outdir, 'names.csv'), 'w', encoding='utf-8') as f:
        f.write('kind,editor_id,name,cell_x,cell_y\n')
        for n in names: f.write('%s,%s,"%s",%s,%s\n' % n)
    # tree and rock density, 8 x 8 per cell over Tamriel's cell range (-57..61, -43..50), north at the top
    X0, X1, Y0, Y1 = -57, 61, -43, 50
    gw, gh = (X1 - X0 + 1) * 8, (Y1 - Y0 + 1) * 8
    for fn, dens in (('trees.pgm', trees), ('rocks.pgm', rocks)):
        g = bytearray(gw * gh)
        for (kx, ky), n in dens.items():
            x = kx - X0 * 8; y = (Y1 + 1) * 8 - 1 - ky
            if 0 <= x < gw and 0 <= y < gh: g[y * gw + x] = min(255, n)
        with open(os.path.join(outdir, fn), 'wb') as f: f.write(b'P5 %d %d 255\n' % (gw, gh)); f.write(bytes(g))
    # per vertex ground
    W, H = (X1 - X0 + 1) * GRID + 1, (Y1 - Y0 + 1) * GRID + 1
    g = bytearray(W * H)
    for (cx, cy), grid in tland.items():
        bx = (cx - X0) * GRID
        for r in range(33):
            row = (Y1 - cy) * GRID + (GRID - r)
            if 0 <= row < H:
                g[row * W + bx: row * W + bx + 33] = bytes(min(255, v) for v in grid[r * 33:(r + 1) * 33])
    with open(os.path.join(outdir, 'ground_full.pgm'), 'wb') as f: f.write(b'P5 %d %d 255\n' % (W, H)); f.write(bytes(g))
    with open(os.path.join(outdir, 'ground_full.txt'), 'w') as f:
        for i, n in enumerate(tex_names): f.write('%d %s\n' % (i, n))
    # Whiterun's city space heights
    if wland:
        xs = [k[0] for k in wland]; ys = [k[1] for k in wland]
        x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
        W, H = (x1 - x0 + 1) * GRID + 1, (y1 - y0 + 1) * GRID + 1
        hm = [-32768] * (W * H)
        for (cx, cy), h in wland.items():
            bx = (cx - x0) * GRID
            for r in range(33):
                row = (y1 - cy) * GRID + (GRID - r)
                for col in range(33): hm[row * W + bx + col] = max(-32767, min(32767, int(round(h[r * 33 + col] / 8))))
        with open(os.path.join(outdir, 'whiterun_height.raw'), 'wb') as f: f.write(struct.pack('<%dh' % len(hm), *hm))
        with open(os.path.join(outdir, 'whiterun_info.txt'), 'w') as f:
            f.write('cells x %d..%d y %d..%d; vertices %d x %d, north at the top; raw * 8 = game units; -32768 = no land\n' % (x0, x1, y0, y1, W, H))
    print('done ->', outdir)
    make_zip(outdir)

def make_zip(outdir):
    # pack the output folder into <folder>.zip next to it, ready to send
    import zipfile
    zp = outdir + '.zip'
    with zipfile.ZipFile(zp, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for root, dirs, files in os.walk(outdir):
            for fn in sorted(files):
                full = os.path.join(root, fn)
                z.write(full, os.path.relpath(full, os.path.dirname(outdir)))
    print('zipped ->', zp, '(%.1f MB) - send this file' % (os.path.getsize(zp) / 1048576.0))

if __name__ == '__main__':
    main()
