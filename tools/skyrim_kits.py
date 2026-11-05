#!/usr/bin/env python3
# Which building pieces and furnishings Skyrim's interiors share - for Hrimfell (one model per piece, every
# interior built from its own placement list).
#
# Usage (Windows, Python 3.8+ from python.org, nothing else needed):
#   python skyrim_kits.py
# Skyrim is looked for in C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\Data; elsewhere,
# give its path first (the Data folder, or Skyrim.esm in it).
# Takes a few minutes and about 2 GB of memory. Reads only Skyrim.esm (and, if present,
# Data\Strings\Skyrim_English.STRINGS for the cells' names). The model path of every object is in Skyrim.esm
# itself, so the .bsa archives are not needed.
#
# Writes into a folder "skyrim_kits" next to this script:
#   cells.csv            every interior cell: editor id, name, how many things are placed in it, its lighting
#                        (ambient, directional and fog colours), and the building set most of its pieces come from
#   bases.csv            every placed base object: type, editor id, model (.nif path), size, how often it is placed
#                        inside and outside, in how many interior cells
#   kits.csv             per model folder (e.g. architecture\farmhouse\interior): how many interior cells use it,
#                        how many placements, how many different pieces
#   cell_kits.csv        per interior cell, the model folders its placements come from (counts)
#   interiors_all.csv.gz everything placed in every interior cell (the same columns as refs_interiors.csv)
# At the end the folder is packed into a .zip next to it (e.g. skyrim_kits.zip): send that file.
import os, sys, struct, zlib, math, gzip, collections

DEFAULT_DATA = r'C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\Data'

def esm_arg(args):
    # the first argument when it is a path (Skyrim.esm or the Data folder); otherwise Skyrim.esm in the usual Steam
    # folder. -> (path to Skyrim.esm, the remaining arguments)
    if args and (args[0].lower().endswith('.esm') or os.path.isdir(args[0]) or '\\' in args[0] or '/' in args[0]):
        p, args = args[0], args[1:]
    else: p = DEFAULT_DATA
    if os.path.isdir(p): p = os.path.join(p, 'Skyrim.esm')
    return p, args

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

def folder(model):
    # the building set: the model's folder without "meshes\", lower case, e.g. architecture\farmhouse\interior
    m = model.replace('/', '\\').lower()
    if m.startswith('meshes\\'): m = m[7:]
    return m.rsplit('\\', 1)[0] if '\\' in m else '(none)'

def main():
    path, _ = esm_arg(sys.argv[1:])
    if not os.path.exists(path): print('Skyrim.esm not found at', path, '- give its path: skyrim_kits.py path\\to\\Skyrim.esm'); return
    print('reading', path)
    with open(path, 'rb') as f: buf = f.read()
    strings = load_strings(path)
    def name(d):
        if len(d) == 4 and strings: return strings.get(struct.unpack('<I', d)[0], '')
        return zstr(d) if len(d) != 4 else ''
    outdir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'skyrim_kits')
    os.makedirs(outdir, exist_ok=True)

    base = {}                                     # fid -> (type, edid, model, bounds)
    cells = {}                                    # interior cell edid -> dict
    st = {'world': '', 'cell': None}
    placed_in = collections.Counter()             # base fid -> placements in interiors
    placed_out = collections.Counter()            # base fid -> placements outside
    base_cells = collections.defaultdict(set)     # base fid -> interior cells
    refs = []                                     # interior rows (written at the end, once all bases are known)

    def deg(r): return '%.1f' % (math.degrees(r) % 360.0)

    def walk(pos, end):
        while pos < end:
            typ = buf[pos:pos + 4]
            size = struct.unpack_from('<I', buf, pos + 4)[0]
            if typ == b'GRUP':
                label = buf[pos + 8:pos + 12]
                gtype = struct.unpack_from('<i', buf, pos + 12)[0]
                saved = (st['world'], st['cell'])
                if gtype == 1: st['world'] = 'world'
                elif gtype == 0 and label == b'CELL': st['world'] = ''
                walk(pos + 24, pos + size)
                if gtype in (0, 1): st['world'], st['cell'] = saved[0], None
                pos += size
                continue
            flags, fid = struct.unpack_from('<II', buf, pos + 8)
            data = buf[pos + 24:pos + 24 + size]
            pos += 24 + size
            if typ in (b'LAND', b'NAVM', b'NAVI', b'WRLD', b'INFO', b'DIAL', b'QUST', b'SCEN', b'PACK'): continue
            if flags & 0x00040000:
                try: data = zlib.decompress(data[4:])
                except zlib.error: continue
            subs = subrecords(data)
            if typ == b'CELL':
                c = {'edid': '', 'name': '', 'interior': st['world'] == '', 'refs': 0, 'light': ''}
                for t, d in subs:
                    if t == b'EDID': c['edid'] = zstr(d)
                    elif t == b'FULL': c['name'] = name(d)
                    elif t == b'XCLL' and len(d) >= 12:
                        a, dcol, fog = d[0:3], d[4:7], d[8:11]
                        c['light'] = '%02X%02X%02X %02X%02X%02X %02X%02X%02X' % (a[0], a[1], a[2], dcol[0], dcol[1], dcol[2], fog[0], fog[1], fog[2])
                st['cell'] = c
                if c['interior']: cells[c['edid']] = c
            elif typ in (b'REFR', b'ACHR'):
                b = 0; p = r = None; sc = 1.0
                for t, d in subs:
                    if t == b'NAME' and len(d) >= 4: b = struct.unpack_from('<I', d)[0]
                    elif t == b'DATA' and len(d) >= 24: v = struct.unpack_from('<6f', d); p, r = v[:3], v[3:]
                    elif t == b'XSCL' and len(d) >= 4: sc = struct.unpack_from('<f', d)[0]
                c = st['cell']
                if c is None or p is None: continue
                if c['interior']:
                    c['refs'] += 1
                    placed_in[b] += 1; base_cells[b].add(c['edid'])
                    refs.append((typ.decode(), b, p, r, sc, c['edid'], fid))
                else:
                    placed_out[b] += 1
            else:
                edid = ''; ob = None; model = ''
                for t, d in subs:
                    if t == b'EDID': edid = zstr(d)
                    elif t == b'OBND' and len(d) >= 12: ob = struct.unpack_from('<6h', d)
                    elif t == b'MODL' and not model: model = zstr(d)
                base[fid] = (typ.decode('latin-1'), edid, model, ob)

    sys.setrecursionlimit(10000)
    walk(0, len(buf))
    print('base records %d, interior cells %d, interior placements %d' % (len(base), len(cells), len(refs)))

    def info(b): return base.get(b, ('?', '#%08X' % b, '', None))
    # everything placed inside
    with gzip.open(os.path.join(outdir, 'interiors_all.csv.gz'), 'wt', encoding='utf-8') as f:
        f.write('kind,base_type,base_editor_id,x,y,z,rot_x,rot_y,rot_z,scale,bounds,world,cell,cell_x,cell_y,ref_id\n')
        for k, b, p, r, sc, cell, fid in refs:
            bt, be, model, ob = info(b)
            f.write('%s,%s,%s,%.1f,%.1f,%.1f,%s,%s,%s,%.3f,%s,,%s,,,%08X\n' % (k, bt, be, p[0], p[1], p[2], deg(r[0]), deg(r[1]), deg(r[2]),
                    sc, ob and '%d %d %d %d %d %d' % ob or '', cell, fid))
    # base objects
    with open(os.path.join(outdir, 'bases.csv'), 'w', encoding='utf-8') as f:
        f.write('base_type,base_editor_id,model,bounds,placed_inside,placed_outside,interior_cells\n')
        for b in sorted(set(placed_in) | set(placed_out), key=lambda b: -len(base_cells.get(b, ()))):
            bt, be, model, ob = info(b)
            f.write('%s,%s,"%s",%s,%d,%d,%d\n' % (bt, be, model, ob and '%d %d %d %d %d %d' % ob or '', placed_in[b], placed_out[b], len(base_cells.get(b, ()))))
    # model folders (the building sets)
    kit_cells = collections.defaultdict(set); kit_refs = collections.Counter(); kit_pieces = collections.defaultdict(set)
    cell_kits = collections.defaultdict(collections.Counter)
    for k, b, p, r, sc, cell, fid in refs:
        model = info(b)[2]
        if not model: continue
        fo = folder(model)
        kit_cells[fo].add(cell); kit_refs[fo] += 1; kit_pieces[fo].add(b); cell_kits[cell][fo] += 1
    with open(os.path.join(outdir, 'kits.csv'), 'w', encoding='utf-8') as f:
        f.write('model_folder,interior_cells,placements,pieces\n')
        for fo in sorted(kit_cells, key=lambda fo: -len(kit_cells[fo])):
            f.write('"%s",%d,%d,%d\n' % (fo, len(kit_cells[fo]), kit_refs[fo], len(kit_pieces[fo])))
    with open(os.path.join(outdir, 'cell_kits.csv'), 'w', encoding='utf-8') as f:
        f.write('cell,model_folder,placements\n')
        for cell in sorted(cell_kits):
            for fo, n in cell_kits[cell].most_common(): f.write('%s,"%s",%d\n' % (cell, fo, n))
    # the cells, with the building set their architecture comes from
    with open(os.path.join(outdir, 'cells.csv'), 'w', encoding='utf-8') as f:
        f.write('cell,name,placements,lighting_ambient_directional_fog,main_architecture\n')
        for e in sorted(cells):
            c = cells[e]
            arch = [fo for fo, n in cell_kits[e].most_common() if fo.startswith(('architecture', 'dungeons'))]
            f.write('%s,"%s",%d,%s,"%s"\n' % (e, c['name'], c['refs'], c['light'], arch[0] if arch else ''))
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
