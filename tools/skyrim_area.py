#!/usr/bin/env python3
# One area of Skyrim for Hrimfell, whole: a city's own worldspace, the land around it, every interior reached
# through its doors, and the models and textures all of that uses. Any area: the game's world is built from these,
# one file per area.
#
# Usage (Windows, Python 3.8+ from python.org, nothing else needed; skyrim_meshes.py must be in the same folder):
#   python skyrim_area.py riften
#   python skyrim_area.py riften 12          (the land round it: 12 cells out instead of 8)
#   python skyrim_area.py 3,7,-13,-9         (Tamriel cells x 3..7, y -13..-9, no city space)
# Skyrim is looked for in C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\Data; elsewhere, give
# that folder first:  python skyrim_area.py "D:\Games\Skyrim Special Edition\Data" riften
# Areas by name: riften, whiterun, solitude, windhelm, markarth, riverwood (cells 3..7, -13..-9),
# bleakfalls (Bleak Falls Barrow's ruin on its mountain, cells -4..2, -13..-8, and the barrow's halls through its doors),
# forelhost (Forelhost's ruin on its mountain east of Riften, cells 42..46, -30..-26, and its three halls).
# Takes some minutes and about 3 GB of memory. Only reads; nothing in the game folder is changed.
#
# Which things: everything placed in the city's worldspace (e.g. RiftenWorld), everything placed in Tamriel in the
# cells round it (8 cells = 470 m out by default; a city's worldspace lies in Tamriel's own coordinates), and every
# interior whose door is there - and the interiors those lead to, up to 6 doors deep (the Ratway, the Ragged Flagon).
#
# Writes a folder "skyrim_<area>" next to this script, and packs it into skyrim_<area>.zip - send that file:
#   area.txt              what was taken: the worldspaces, the Tamriel cells, the interiors, the counts
#   refs.csv.gz           everything placed: what (type, editor id, name), where (x, y, z, rotation, scale), in which
#                         world and cell, its flags (initially disabled ...), its enable parent (things that come and
#                         go with a quest), the door's destination
#   bases.csv             every placed thing's base: type, editor id, name, model, size
#   doors.csv             the doors: where, and where they lead
#   cells.csv             the interior cells taken: editor id, name, things placed
#   worldspaces.csv       every worldspace in Skyrim.esm: editor id, name, parent, whether it uses its parent's land
#   regions.csv           every region (Skyrim's biomes: the Rift's autumn forest, the Reach, the pine forest, ...):
#                         editor id, colour, worldspace, what it brings (weather, objects, sounds ...), its outline
#   cell_regions.csv      every exterior cell of every worldspace: its regions
#   ext_cells.csv         the exterior cells with a name of their own (worldspace, grid, editor id, name)
#   land_<world>.bin      the area's own land in each worldspace taken (heights and ground textures per cell), and
#   land_<world>.txt        the names of the ground textures (Tamriel's land itself is in the heightmap export)
#   npcs.csv              the people placed: race, female, class, outfit (and its items), skin tone, height, packages,
#                         voice type, factions (and rank)
#   packages.csv          their packages: when (hour, minutes long, day ...) and where (a reference's place, a cell)
#   navmesh.txt.gz        Skyrim's walkable floors (navmeshes) of the interiors taken: corners, triangles, neighbours
#   dialogue.csv          what they say: the lines whose conditions name one of them or their voice, with the topic
#                         (its kind: HELO a greeting, IDLE a remark ...) and the conditions as written
#   models.csv, meshes\*.obj, textures.csv, textures\*.ppm (and *_a.pgm: the transparency of cut-out leaves,
#   branches, fences), errors.txt   as skyrim_meshes.py writes them
import os, sys, struct, zlib, math, re, gzip, collections, traceback

DEFAULT_DATA = r'C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\Data'

def data_arg(args):
    # the first argument when it is a path (the Data folder or Skyrim.esm in it); otherwise the usual Steam folder.
    # -> (the Data folder, the remaining arguments)
    if args and (args[0].lower().endswith('.esm') or os.path.isdir(args[0]) or '\\' in args[0] or '/' in args[0]):
        p, args = args[0], args[1:]
    else: p = DEFAULT_DATA
    if p.lower().endswith('.esm'): p = os.path.dirname(p)
    return p, args
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import skyrim_meshes as sm
if not hasattr(sm, 'subrecords') or not hasattr(sm, 'BSA'):
    sys.exit('skyrim_meshes.py beside this script is an older one (%s): put the newest one there (the one sent with this script)' % sm.__file__)

AREAS = {'riften': (['RiftenWorld'], None), 'whiterun': (['WhiterunWorld'], None), 'solitude': (['SolitudeWorld'], None),
         'windhelm': (['WindhelmWorld'], None), 'markarth': (['MarkarthWorld'], None), 'riverwood': ([], (3, 7, -13, -9)),
         'bleakfalls': ([], (-4, 2, -13, -8)), 'forelhost': ([], (42, 46, -30, -26))}
DEPTH = 6               # doors deep into interiors
MAXINT = 400            # interiors at most

def load_strings(data_dir, kind='STRINGS'):
    # Data\Strings\Skyrim_English.STRINGS, or (Skyrim SE) the same file inside "Skyrim - Interface.bsa"; kind
    # 'ILSTRINGS': the dialogue's lines (each with its length first)
    p = os.path.join(data_dir, 'Strings', 'Skyrim_English.' + kind); b = None
    if os.path.exists(p):
        with open(p, 'rb') as f: b = f.read()
    else:
        for fn in sorted(os.listdir(data_dir)):
            if fn.lower().endswith('.bsa') and fn.lower().startswith('skyrim'):
                try:
                    a = sm.BSA(os.path.join(data_dir, fn))
                    k_ = 'strings\\skyrim_english.' + kind.lower()
                    if k_ in a.files: b = a.read(k_); break
                except Exception: pass
    if b is None: return {}
    n, size = struct.unpack_from('<II', b, 0)
    base = 8 + n * 8; out = {}
    for i in range(n):
        sid, off = struct.unpack_from('<II', b, 8 + i * 8)
        if kind == 'STRINGS': o_ = base + off; e = b.find(b'\0', o_)
        else: o_ = base + off + 4; e = o_ + max(0, struct.unpack_from('<I', b, base + off)[0] - 1)
        out[sid] = b[o_:e].decode('cp1252', 'replace')
    return out

def deg(r): return '%.1f' % (math.degrees(r) % 360.0)

def read_esm(path, strings):
    with open(path, 'rb') as f: buf = f.read()
    def name(d):
        if len(d) == 4 and strings: return strings.get(struct.unpack('<I', d)[0], '')
        return sm.zstr(d) if len(d) != 4 else ''
    base = {}           # fid -> [type, edid, name, model, bounds]
    worlds = {}         # fid -> [edid, name, parent fid, parent flags]
    regions = []        # (edid, colour, world fid, [(type, flags, priority)], [[(x, y) ...] ...])
    ltex = {}           # fid -> edid
    cells = {}          # interior cell edid -> [name, refs]
    cellreg = {}        # (world, x, y) -> [region fids]
    extnames = {}       # (v0.4.74) (world, x, y) -> (editor id, name): the exterior cells Skyrim names
    refs = []           # (fid, kind, world, cell edid, cx, cy, base, pos, rot, scale, flags, xesp, tel)
    refcell = {}        # ref fid -> (world, cell edid)
    land = collections.defaultdict(dict)   # world -> (x, y) -> (heights[1089], ground[1089])
    npcs = {}; outfits = {}; packs = {}    # (the people's details, their outfits' items, their packages)
    dials = {}; infos = {}                 # (v0.4.51: the topics, and their lines - who says them: their conditions)
    quests = {}                            # (v0.4.68: the quests - their stages, log, objectives, aliases)
    books = {}                             # (v0.4.71: the books' texts: fid -> DLSTRINGS id)
    navs = []                              # (v0.4.55: the navmeshes - Skyrim's walkable floors: cell, world, where)
    st = {'world': '', 'cell': None}
    tex_idx = {}; tex_names = []
    def tex_id(fid):
        if fid not in tex_idx: tex_idx[fid] = len(tex_names); tex_names.append(fid)
        return tex_idx[fid]

    def walk(pos, end):
        while pos < end:
            typ = buf[pos:pos + 4]; size = struct.unpack_from('<I', buf, pos + 4)[0]
            if typ == b'GRUP':
                label = buf[pos + 8:pos + 12]; gtype = struct.unpack_from('<i', buf, pos + 12)[0]
                saved = (st['world'], st['cell'])
                saved_d = st.get('dial', 0)
                if gtype == 7: st['dial'] = struct.unpack('<I', label)[0]          # (a topic's children: its lines)
                if gtype == 1: st['world'] = worlds.get(struct.unpack('<I', label)[0], ['#%08X' % struct.unpack('<I', label)[0]])[0]
                elif gtype == 0 and label == b'CELL': st['world'] = ''
                walk(pos + 24, pos + size)
                st['dial'] = saved_d
                if gtype in (0, 1): st['world'], st['cell'] = saved[0], None
                pos += size; continue
            flags, fid = struct.unpack_from('<II', buf, pos + 8)
            data = buf[pos + 24:pos + 24 + size]; pos += 24 + size
            if typ == b'NAVM':
                c_ = st['cell']
                navs.append((st['world'], c_['edid'] if c_ else '', c_['x'] if c_ else None, c_['y'] if c_ else None, fid, pos - 24 - size, size, flags))
                continue
            if typ in (b'NAVI', b'SCEN', b'IDLE'): continue
            if flags & 0x00040000:
                try: data = zlib.decompress(data[4:])
                except zlib.error: continue
            subs = sm.subrecords(data)
            if typ == b'DIAL':
                # (a topic: its editor id, its text - what the player says - and its kind: SNAM, e.g. HELO a greeting)
                dl = {'edid': '', 'text': '', 'snam': ''}
                for t, d in subs:
                    if t == b'EDID': dl['edid'] = sm.zstr(d)
                    elif t == b'FULL': dl['text'] = name(d)
                    elif t == b'SNAM' and len(d) >= 4: dl['snam'] = d[:4].decode('latin-1')
                    elif t == b'QNAM' and len(d) >= 4: dl['quest'] = struct.unpack_from('<I', d)[0]     # (v0.4.68: its quest)
                dials[fid] = dl
                continue
            if typ == b'QUST':
                # (v0.4.68) a quest: its name, its stages (each one's log entries: CNAM, in DLSTRINGS), its objectives
                # (QOBJ, their text NNAM), its aliases (ALST/ALLS, the name ALID, a forced reference ALFR, a unique
                # actor ALUA) - so a place's quest comes with the place
                q = {'edid': '', 'name': '', 'stages': [], 'objs': [], 'aliases': []}
                cur = None; ali = None; obj = None
                for t, d in subs:
                    if t == b'EDID': q['edid'] = sm.zstr(d)
                    elif t == b'FULL' and not q['aliases'] and obj is None: q['name'] = name(d)
                    elif t == b'INDX' and len(d) >= 2: cur = {'i': struct.unpack_from('<H', d)[0], 'log': []}; q['stages'].append(cur)
                    elif t == b'CNAM' and cur is not None and len(d) == 4: cur['log'].append(struct.unpack_from('<I', d)[0])
                    elif t == b'QOBJ' and len(d) >= 2: obj = {'i': struct.unpack_from('<H', d)[0], 'text': 0}; q['objs'].append(obj)
                    elif t == b'NNAM' and obj is not None and len(d) == 4 and not obj['text']: obj['text'] = struct.unpack_from('<I', d)[0]
                    elif t in (b'ALST', b'ALLS') and len(d) >= 4: ali = {'id': struct.unpack_from('<I', d)[0], 'loc': t == b'ALLS', 'name': '', 'ref': 0, 'actor': 0}; q['aliases'].append(ali)
                    elif t == b'ALID' and ali is not None: ali['name'] = sm.zstr(d)
                    elif t == b'ALFR' and ali is not None and len(d) >= 4: ali['ref'] = struct.unpack_from('<I', d)[0]
                    elif t == b'ALUA' and ali is not None and len(d) >= 4: ali['actor'] = struct.unpack_from('<I', d)[0]
                    elif t == b'ALCO' and ali is not None and len(d) >= 4: ali['create'] = struct.unpack_from('<I', d)[0]     # (v0.4.71: made by the quest - the well's key)
                quests[fid] = q
                continue
            if typ == b'INFO':
                # (a line: its responses - NAM1, in ILSTRINGS - their order, emotion; the conditions on who says it,
                #  CTDA: function, its first parameter; a shared line's: DNAM)
                inf = {'dial': st.get('dial', 0), 'resp': [], 'cond': [], 'shared': 0, 'prompt': ''}
                cur = None
                for t, d in subs:
                    if t == b'TRDT' and len(d) >= 13: cur = {'n': d[12], 'emo': struct.unpack_from('<I', d)[0], 'id': 0}; inf['resp'].append(cur)
                    elif t == b'NAM1' and len(d) >= 4:
                        if cur is None: cur = {'n': len(inf['resp']), 'emo': 0, 'id': 0}; inf['resp'].append(cur)
                        cur['id'] = struct.unpack_from('<I', d)[0]; cur = None
                    elif t == b'CTDA' and len(d) >= 16:
                        op = d[0]; fn = struct.unpack_from('<H', d, 8)[0]; p1 = struct.unpack_from('<I', d, 12)[0]
                        cv = struct.unpack_from('<f', d, 4)[0]
                        inf['cond'].append((fn, p1, op >> 5, cv, op & 1))
                    elif t == b'DNAM' and len(d) >= 4: inf['shared'] = struct.unpack_from('<I', d)[0]
                    elif t == b'RNAM' and len(d) >= 4: inf['prompt'] = name(d)
                infos[fid] = inf
                continue
            if typ == b'NPC_':
                # (the people: race, female, outfit, class, their day's packages, skin tone, height)
                n_ = {'race': 0, 'female': 0, 'outfit': 0, 'class': 0, 'packs': [], 'skin': None, 'height': 1.0}
                for t, d in subs:
                    if t == b'RNAM' and len(d) >= 4: n_['race'] = struct.unpack_from('<I', d)[0]
                    elif t == b'ACBS' and len(d) >= 4: n_['female'] = struct.unpack_from('<I', d)[0] & 1
                    elif t == b'DOFT' and len(d) >= 4: n_['outfit'] = struct.unpack_from('<I', d)[0]
                    elif t == b'CNAM' and len(d) >= 4: n_['class'] = struct.unpack_from('<I', d)[0]
                    elif t == b'PKID' and len(d) >= 4: n_['packs'].append(struct.unpack_from('<I', d)[0])
                    elif t == b'QNAM' and len(d) >= 12: n_['skin'] = struct.unpack_from('<3f', d)
                    elif t == b'NAM6' and len(d) >= 4: n_['height'] = struct.unpack_from('<f', d)[0]
                    elif t == b'VTCK' and len(d) >= 4: n_['voice'] = struct.unpack_from('<I', d)[0]
                    elif t == b'SNAM' and len(d) >= 5: n_.setdefault('factions', []).append(struct.unpack_from('<Ib', d))
                npcs[fid] = n_
            elif typ == b'OTFT':
                outfits[fid] = [struct.unpack_from('<I', d, k)[0] for t, d in subs if t == b'INAM' for k in range(0, len(d) - 3, 4)]
            elif typ == b'PACK':
                # (a package: when - PSDT: month, day of the week, date, hour, minute, duration in minutes - and
                #  where - PLDT: type, what, radius; PTDA: the target)
                pk = {'edid': '', 'sched': None, 'locs': [], 'targets': [], 'type': -1, 'template': 0}
                for t, d in subs:
                    if t == b'EDID': pk['edid'] = sm.zstr(d)
                    elif t == b'PKDT' and len(d) >= 5: pk['type'] = d[4]
                    elif t == b'PSDT' and len(d) >= 12: pk['sched'] = struct.unpack_from('<bbBbb3xi', d)
                    elif t == b'PLDT' and len(d) >= 12: pk['locs'].append(struct.unpack_from('<iIi', d))
                    elif t == b'PTDA' and len(d) >= 12: pk['targets'].append(struct.unpack_from('<iIi', d))
                    elif t == b'PKCU' and len(d) >= 8: pk['template'] = struct.unpack_from('<II', d)[1]
                packs[fid] = pk
            if typ == b'PACK': continue
            if typ == b'WRLD':
                w = ['', '', 0, 0]
                for t, d in subs:
                    if t == b'EDID': w[0] = sm.zstr(d)
                    elif t == b'FULL': w[1] = name(d)
                    elif t == b'WNAM' and len(d) >= 4: w[2] = struct.unpack_from('<I', d)[0]
                    elif t == b'PNAM' and len(d) >= 2: w[3] = struct.unpack_from('<H', d)[0]
                worlds[fid] = w
            elif typ == b'REGN':
                r = ['', '', 0, [], [], fid]
                for t, d in subs:
                    if t == b'EDID': r[0] = sm.zstr(d)
                    elif t == b'RCLR' and len(d) >= 3: r[1] = '%02X%02X%02X' % (d[0], d[1], d[2])
                    elif t == b'WNAM' and len(d) >= 4: r[2] = struct.unpack_from('<I', d)[0]
                    elif t == b'RPLD': r[4].append([struct.unpack_from('<2f', d, k) for k in range(0, len(d) - 7, 8)])
                    elif t == b'RDAT' and len(d) >= 8:
                        ty, fl, pr = struct.unpack_from('<IBB', d); r[3].append((ty, fl, pr))
                regions.append(r)
            elif typ == b'LTEX':
                for t, d in subs:
                    if t == b'EDID': ltex[fid] = sm.zstr(d)
            elif typ == b'CELL':
                c = {'edid': '', 'x': None, 'y': None, 'name': '', 'reg': []}
                for t, d in subs:
                    if t == b'EDID': c['edid'] = sm.zstr(d)
                    elif t == b'XCLC' and len(d) >= 8: c['x'], c['y'] = struct.unpack_from('<ii', d)
                    elif t == b'FULL': c['name'] = name(d)
                    elif t == b'XCLR': c['reg'] = list(struct.unpack_from('<%dI' % (len(d) // 4), d))
                st['cell'] = c
                if st['world'] == '': cells[c['edid']] = [c['name'], 0]
                elif c['x'] is not None and c['reg']: cellreg[(st['world'], c['x'], c['y'])] = c['reg']
                if st['world'] != '' and c['x'] is not None and c['name']: extnames[(st['world'], c['x'], c['y'])] = (c['edid'], c['name'])
            elif typ == b'LAND':
                c = st['cell']
                if c is None or c['x'] is None or st['world'] in ('', 'Tamriel'): continue
                h = None; grid = [0] * 1089; cur = None
                for t, d in subs:
                    if t == b'VHGT' and len(d) >= 4 + 1089:
                        off = struct.unpack_from('<f', d, 0)[0] * 8
                        g = struct.unpack_from('<1089b', d, 4)
                        h = [0.0] * 1089; ro = 0.0
                        for i in range(1089):
                            r_, col = divmod(i, 33); v = g[i] * 8
                            if col == 0: off += v; ro = 0.0
                            else: ro += v
                            h[i] = off + ro
                    elif t == b'BTXT' and len(d) >= 8:
                        tid, q = struct.unpack_from('<IB', d); k = tex_id(tid); qx, qy = (q & 1) * 16, (q >> 1) * 16
                        for r_ in range(17):
                            for col in range(17): grid[(qy + r_) * 33 + qx + col] = k
                    elif t == b'ATXT' and len(d) >= 8:
                        tid, q = struct.unpack_from('<IB', d); cur = (tex_id(tid), q)
                    elif t == b'VTXT' and cur is not None:
                        k, q = cur; qx, qy = (q & 1) * 16, (q >> 1) * 16
                        for j in range(0, len(d) - 7, 8):
                            p_, _, _, op = struct.unpack_from('<HBBf', d, j)
                            if op > 0.5 and p_ < 289:
                                r_, col = divmod(p_, 17); grid[(qy + r_) * 33 + qx + col] = k
                        cur = None
                if h is not None: land[st['world']][(c['x'], c['y'])] = (h, grid)
            elif typ in (b'REFR', b'ACHR'):
                c = st['cell'] or {'edid': '', 'x': None, 'y': None}
                b = 0; p3 = r3 = None; sc = 1.0; tel = None; xesp = None
                for t, d in subs:
                    if t == b'NAME' and len(d) >= 4: b = struct.unpack_from('<I', d)[0]
                    elif t == b'DATA' and len(d) >= 24: v = struct.unpack_from('<6f', d); p3, r3 = v[:3], v[3:]
                    elif t == b'XSCL' and len(d) >= 4: sc = struct.unpack_from('<f', d)[0]
                    elif t == b'XTEL' and len(d) >= 28: tel = struct.unpack_from('<I6f', d)
                    elif t == b'XESP' and len(d) >= 5: xesp = struct.unpack_from('<IB', d)
                w = st['world']
                refcell[fid] = (w, c['edid'])
                if p3 is None: continue
                cx, cy = c['x'], c['y']
                if w != '':
                    # the cell by its grid, or (a world's persistent things) by the thing's own position
                    cx, cy = int(math.floor(p3[0] / 4096.0)), int(math.floor(p3[1] / 4096.0))
                else: cells[c['edid']][1] += 1
                refs.append((fid, typ.decode(), w, c['edid'], cx, cy, b, p3, r3, sc, flags, xesp, tel))
            else:
                edid = ''; model = ''; nm = ''; ob = None
                for t, d in subs:
                    if t == b'DESC' and typ == b'BOOK' and len(d) == 4: books.setdefault(fid, [0, 0, 0.0])[0] = struct.unpack_from('<I', d)[0]     # (v0.4.71: a book's text, DLSTRINGS)
                    if t == b'DATA' and typ == b'BOOK' and len(d) >= 16:                                          # (v0.4.72: its value and weight)
                        b_ = books.setdefault(fid, [0, 0, 0.0]); b_[1] = struct.unpack_from('<I', d, 8)[0]; b_[2] = struct.unpack_from('<f', d, 12)[0]
                    if t == b'EDID': edid = sm.zstr(d)
                    elif t == b'MODL' and not model: model = sm.zstr(d)
                    elif t == b'FULL' and not nm: nm = name(d)
                    elif t == b'OBND' and len(d) >= 12: ob = struct.unpack_from('<6h', d)
                base[fid] = (typ.decode('latin-1'), edid, nm, model, ob)
    sys.setrecursionlimit(10000)
    walk(0, len(buf))
    return base, worlds, regions, ltex, cells, cellreg, refs, refcell, land, tex_names, npcs, outfits, packs, dials, infos, navs, quests, books, extnames

def main():
    data_dir, rest = data_arg(sys.argv[1:])
    if not os.path.exists(os.path.join(data_dir, 'Skyrim.esm')):
        print('Skyrim.esm not found in', data_dir, '- give the Data folder first: skyrim_area.py path\\to\\Data riften'); return
    if not rest:
        print('which area? e.g.  python skyrim_area.py bleakfalls   (areas: %s, or cells x0,x1,y0,y1)' % ', '.join(sorted(AREAS))); return
    spec = rest[0].lower(); margin = int(rest[1]) if len(rest) > 1 else 8
    if spec not in AREAS and not re.match(r'^-?\d+,-?\d+,-?\d+,-?\d+$', spec):
        print('unknown area %r - areas: %s, or cells x0,x1,y0,y1' % (spec, ', '.join(sorted(AREAS)))); return
    print('exporting', spec)
    if spec in AREAS: wnames, box = AREAS[spec]
    else:
        wnames, box = [], tuple(int(v) for v in spec.split(','))
        spec = 'cells_' + spec.replace(',', '_').replace('-', 'm')
    outdir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'skyrim_' + spec)
    for sub in ('meshes', 'textures'): os.makedirs(os.path.join(outdir, sub), exist_ok=True)
    errors = []
    strings = load_strings(data_dir)
    print('strings: %d' % len(strings))
    print('reading Skyrim.esm (a few minutes)')
    base, worlds, regions, ltex, cells, cellreg, refs, refcell, land, tex_names, npcs, outfits, packs, dials, infos, navs, quests, books, extnames = read_esm(os.path.join(data_dir, 'Skyrim.esm'), strings)
    print('base records %d, worldspaces %d, regions %d, refs %d' % (len(base), len(worlds), len(regions), len(refs)))
    wid = {w[0]: f for f, w in worlds.items()}

    # ---- the area: the city's worldspaces, the Tamriel cells round them, the interiors behind their doors
    wset = set(w for w in wnames if w in wid)
    for w in wnames:
        if w not in wid: errors.append('worldspace %s not found' % w)
    if box is None:
        xs = [r[4] for r in refs if r[2] in wset]; ys = [r[5] for r in refs if r[2] in wset]
        if not xs: print('nothing placed in', wnames); return
        box = (min(xs) - margin, max(xs) + margin, min(ys) - margin, max(ys) + margin)
    def outside(r): return r[2] in wset or (r[2] == 'Tamriel' and box[0] <= r[4] <= box[1] and box[2] <= r[5] <= box[3])
    take = [r for r in refs if outside(r)]
    inter = set(); front = set()
    for r in take:
        if r[12] is not None:
            dw, dc = refcell.get(r[12][0], ('?', '?'))
            if dw == '' and dc not in inter: inter.add(dc); front.add(dc)
    for depth in range(DEPTH - 1):
        nxt = set()
        for r in refs:
            if r[2] == '' and r[3] in front and r[12] is not None:
                dw, dc = refcell.get(r[12][0], ('?', '?'))
                if dw == '' and dc not in inter and len(inter) < MAXINT: inter.add(dc); nxt.add(dc)
        front = nxt
        if not front: break
    take += [r for r in refs if r[2] == '' and r[3] in inter]
    print('area: worldspaces %s, Tamriel cells x %d..%d y %d..%d, interiors %d, things placed %d' %
          (sorted(wset), box[0], box[1], box[2], box[3], len(inter), len(take)))
    with open(os.path.join(outdir, 'area.txt'), 'w', encoding='utf-8') as f:
        f.write('area %s\nworldspaces %s\ntamriel cells x %d..%d, y %d..%d (margin %d)\ninteriors %d\nthings placed %d\n' %
                (spec, ' '.join(sorted(wset)), box[0], box[1], box[2], box[3], margin, len(inter), len(take)))
        for c in sorted(inter): f.write('  %s\n' % c)

    # ---- the placements
    used = set(r[6] for r in take)
    with gzip.open(os.path.join(outdir, 'refs.csv.gz'), 'wt', encoding='utf-8') as f:
        f.write('ref_id,kind,base_id,base_type,base_editor_id,x,y,z,rot_x,rot_y,rot_z,scale,world,cell,cell_x,cell_y,'
                'flags,enable_parent,enable_opposite,dest_ref\n')
        for fid, kind, w, ce, cx, cy, b, p, r, sc, fl, xesp, tel in take:
            bt, be = base.get(b, ('?', '#%08X' % b))[:2]
            f.write('%08X,%s,%08X,%s,%s,%.1f,%.1f,%.1f,%s,%s,%s,%.3f,%s,%s,%s,%s,%X,%s,%d,%s\n' % (
                fid, kind, b, bt, be, p[0], p[1], p[2], deg(r[0]), deg(r[1]), deg(r[2]), sc, w, ce,
                '' if cx is None else cx, '' if cy is None else cy, fl,
                '%08X' % xesp[0] if xesp else '', xesp[1] & 1 if xesp else 0, '%08X' % tel[0] if tel else ''))
    with open(os.path.join(outdir, 'bases.csv'), 'w', encoding='utf-8') as f:
        f.write('base_id,type,editor_id,name,model,bounds\n')
        for b in sorted(used):
            bt, be, nm, model, ob = base.get(b, ('?', '#%08X' % b, '', '', None))
            f.write('%08X,%s,%s,"%s","%s",%s\n' % (b, bt, be, nm.replace('"', "'"), model, ' '.join(str(v) for v in ob) if ob else ''))
    # ---- the people placed: who they are, what they wear, what they do through the day (their packages)
    def ed(f_): return base.get(f_, ('?', '#%08X' % f_))[1] if f_ else ''
    refpos = {r[0]: r for r in refs}
    pset = set()
    with open(os.path.join(outdir, 'npcs.csv'), 'w', encoding='utf-8') as f:
        f.write('base_id,editor_id,name,race,female,class,outfit,outfit_items,skin_rgb,height,packages,voice,factions\n')
        for b in sorted(used):
            n_ = npcs.get(b)
            if not n_: continue
            bt, be, nm = base.get(b, ('?', '', ''))[:3]
            items = ';'.join('%s:%s' % (ed(i), base.get(i, ('', '', ''))[2].replace('"', "'").replace(';', ',')) for i in outfits.get(n_['outfit'], []))
            sk = '%d %d %d' % tuple(int(max(0, min(1, c)) * 255) for c in n_['skin']) if n_['skin'] else ''
            f.write('%08X,%s,"%s",%s,%d,%s,%s,"%s",%s,%.2f,%s,%s,%s\n' % (b, be, nm.replace('"', "'"), ed(n_['race']), n_['female'], ed(n_['class']),
                    ed(n_['outfit']), items, sk, n_['height'], ' '.join(packs.get(k, {'edid': '#%08X' % k})['edid'] for k in n_['packs']),
                    ed(n_.get('voice', 0)), ' '.join('%s:%d' % (ed(fa) or '%08X' % fa, rk) for fa, rk in n_.get('factions', []))))
            pset.update(n_['packs'])
    with open(os.path.join(outdir, 'packages.csv'), 'w', encoding='utf-8') as f:
        f.write('package_id,editor_id,type,template,month,day_of_week,date,hour,minute,duration_min,locations,targets\n')
        def loc(l):
            ty, v, rad = l
            if ty in (0, 6) and v in refpos:            # near a reference: where it is
                r = refpos[v]; return '%d:%08X@%s/%s/%.0f/%.0f/%.0f:r%d' % (ty, v, r[2], r[3], r[7][0], r[7][1], r[7][2], rad)
            if ty == 1: return '%d:%s:r%d' % (ty, refcell.get(v, ('', ''))[1] or ('#%08X' % v), rad)
            return '%d:%s:r%d' % (ty, ed(v) or '%08X' % v, rad)
        for k in sorted(pset):
            pk = packs.get(k)
            if not pk: continue
            sc = pk['sched'] or (-1, -1, 0, -1, -1, 0)
            f.write('%08X,%s,%d,%s,%d,%d,%d,%d,%d,%d,"%s","%s"\n' % (k, pk['edid'], pk['type'], packs.get(pk['template'], {'edid': '#%08X' % pk['template']})['edid'] if pk['template'] else '',
                    sc[0], sc[1], sc[2], sc[3], sc[4], sc[5], ' '.join(loc(l) for l in pk['locs']), ' '.join(loc(l) for l in pk['targets'])))
    print('people %d, their packages %d' % (sum(1 for b in used if b in npcs), len(pset)))
    # ---- (v0.4.51) what they say: every line whose conditions name one of them (GetIsID: their own lines) or their
    # voice (GetIsVoiceType: the lines everyone with that voice may say - their greetings, their remarks), with the
    # topic it answers (its kind: HELO a greeting, IDLE a remark in passing, ...) and all its conditions as written
    # (function number:its first parameter's editor id or number, the comparison, the value) for the game to judge
    ilstr = load_strings(data_dir, 'ILSTRINGS')
    print('dialogue strings: %d' % len(ilstr))
    who = {}
    for b in used:
        n_ = npcs.get(b)
        if not n_: continue
        who[b] = b
        if n_.get('voice'): who.setdefault(n_['voice'], n_['voice'])
        for fa, rk in n_.get('factions', []): who.setdefault(fa, fa)       # (v0.4.53: the lines of their factions too - a carriage driver's)
    OPS = ('==', '!=', '>', '>=', '<', '<=', '?', '?')
    # (v0.4.68) the area's quests: those with an alias forced onto something placed here, or onto one of its people -
    # every line of their topics is taken (whoever says it: an alias's actor, named in its conditions), their stages'
    # log entries, their objectives and their aliases written to quests.csv
    taken_ids = set(r[0] for r in take)
    aq = set(qf for qf, q in quests.items() if any(a['ref'] in taken_ids or (a['actor'] and a['actor'] in used) for a in q['aliases']))
    dlstr = load_strings(data_dir, 'DLSTRINGS')
    with open(os.path.join(outdir, 'quests.csv'), 'w', encoding='utf-8') as f:
        f.write('quest_id,quest,quest_name,kind,index,text,ref,actor,created,created_name\n')
        for qf in sorted(aq):
            q = quests[qf]
            def w_(kind, i, txt, ref='', actor='', cr=0):
                cb = base.get(cr, ('', '', ''))
                f.write('%08X,%s,"%s",%s,%s,"%s",%s,%s,%s,"%s"\n' % (qf, q['edid'], q['name'].replace('"', "'"), kind, i, txt.replace('"', "'").replace('\n', ' '), ref, actor,
                        cb[1] if cr else '', (cb[2] or '').replace('"', "'") if cr else ''))
            for st_ in q['stages']:
                if not st_['log']: w_('stage', st_['i'], '')
                for l_ in st_['log']: w_('stage', st_['i'], dlstr.get(l_, ''))
            for o_ in q['objs']: w_('objective', o_['i'], strings.get(o_['text'], '') if o_['text'] else '')
            for a_ in q['aliases']:
                w_('location' if a_['loc'] else 'alias', a_['id'], a_['name'], '%08X' % a_['ref'] if a_['ref'] else '', ed(a_['actor']) if a_['actor'] else '', a_.get('create', 0))
    print('the area\'s quests: %s' % ', '.join(quests[qf]['edid'] for qf in sorted(aq)))
    # (v0.4.71) the books placed here: their texts (Skyrim's markup kept as written)
    with open(os.path.join(outdir, 'books.csv'), 'w', encoding='utf-8') as f:
        f.write('base_id,editor_id,name,value,weight,text\n')
        for b_ in sorted(set(r[6] for r in take)):
            if b_ in books:
                f.write('%08X,%s,"%s",%d,%g,"%s"\n' % (b_, ed(b_), (base[b_][2] or '').replace('"', "'"), books[b_][1], books[b_][2], dlstr.get(books[b_][0], '').replace('"', "'").replace('\r', '').replace('\n', '\\n')))
    nline = 0
    with open(os.path.join(outdir, 'dialogue.csv'), 'w', encoding='utf-8') as f:
        f.write('info_id,topic,topic_kind,topic_text,speaker,response,emotion,text,prompt,conditions\n')
        for k, inf in infos.items():
            names_ = [c_[1] for c_ in inf['cond'] if c_[1] in who]
            dl = dials.get(inf['dial'], {'edid': '#%08X' % inf['dial'], 'text': '', 'snam': ''})
            inq = dl.get('quest') in aq
            if not names_ and not inq: continue
            src = inf
            if not inf['resp'] and inf['shared'] in infos: src = infos[inf['shared']]
            conds = ' '.join('%d:%s%s%g%s' % (fn, ed(p1) or '%X' % p1, OPS[op], cv, '|' if orr else '') for fn, p1, op, cv, orr in inf['cond'])
            for r_ in sorted(src['resp'], key=lambda q: q['n']):
                txt = ilstr.get(r_['id'], '')
                if not txt: continue
                f.write('%08X,%s,%s,"%s",%s,%d,%d,"%s","%s","%s"\n' % (k, dl['edid'], dl['snam'], dl['text'].replace('"', "'"),
                        ' '.join(ed(n) for n in names_) or ('quest:' + quests[dl['quest']]['edid'] if inq else ''), r_['n'], r_['emo'], txt.replace('"', "'").replace('\n', ' '), inf['prompt'].replace('"', "'"), conds))
                nline += 1
    print('their lines: %d' % nline)
    with open(os.path.join(outdir, 'doors.csv'), 'w', encoding='utf-8') as f:
        f.write('ref_id,world,cell,base_editor_id,x,y,z,rot_z,dest_ref,dest_world,dest_cell,dest_x,dest_y,dest_z,dest_rot_z\n')
        for fid, kind, w, ce, cx, cy, b, p, r, sc, fl, xesp, tel in take:
            if tel is None: continue
            dw, dc = refcell.get(tel[0], ('?', '?'))
            f.write('%08X,%s,%s,%s,%.1f,%.1f,%.1f,%s,%08X,%s,%s,%.1f,%.1f,%.1f,%s\n' % (fid, w, ce, base.get(b, ('?', '?'))[1],
                    p[0], p[1], p[2], deg(r[2]), tel[0], dw, dc, tel[1], tel[2], tel[3], deg(tel[6])))
    # ---- (v0.4.55) the navmeshes of the interiors taken (and of the area's own cells outside): Skyrim's walkable
    # floors - their corners, their triangles with the neighbour across each edge (-1: none, an edge one cannot walk
    # past, unless its flag links it to another navmesh: bits 1, 2, 4 for edges 0-1, 1-2, 2-0) and their flags.
    # navmesh.txt.gz: "N world cell gridx gridy navmesh_id vertices triangles", then "v x y z" per corner, then
    # "t a b c e0 e1 e2 flags" per triangle
    try:
        nn = 0
        with open(os.path.join(data_dir, 'Skyrim.esm'), 'rb') as ef, gzip.open(os.path.join(outdir, 'navmesh.txt.gz'), 'wt', encoding='utf-8') as nf:
            for w, ce, gx, gy, fid, at, size, fl in navs:
                if not ((w == '' and ce in inter) or (w != '' and w in wset)): continue      # (the interiors, the city's own worldspace)
                ef.seek(at); hdr = ef.read(24); data = ef.read(size)
                if fl & 0x00040000:
                    try: data = zlib.decompress(data[4:])
                    except zlib.error: errors.append('navmesh %08X: not decompressed' % fid); continue
                for t, d in sm.subrecords(data):
                    if t != b'NVNM': continue
                    try:
                        p = 8; wsp = struct.unpack_from('<I', d, p)[0]; p += 4
                        p += 4                                              # (the cell, or its grid y and x)
                        nv = struct.unpack_from('<I', d, p)[0]; p += 4
                        V = [struct.unpack_from('<3f', d, p + 12 * i) for i in range(nv)]; p += 12 * nv
                        nt = struct.unpack_from('<I', d, p)[0]; p += 4
                        Tr = [struct.unpack_from('<3H3hH', d, p + 16 * i) for i in range(nt)]
                        if any(max(q[0], q[1], q[2]) >= nv for q in Tr): errors.append('navmesh %08X: a corner past the corners (%d, %d)' % (fid, nv, nt)); continue
                    except struct.error as e_:
                        errors.append('navmesh %08X: %s' % (fid, e_)); continue
                    nf.write('N %s %s %s %s %08X %d %d\n' % (w or '-', ce or '-', gx if gx is not None else '-', gy if gy is not None else '-', fid, nv, nt))
                    for v in V: nf.write('v %.1f %.1f %.1f\n' % v)
                    for q in Tr: nf.write('t %d %d %d %d %d %d %d\n' % q)
                    nn += 1
        print('navmeshes: %d' % nn)
    except Exception as e_:
        errors.append('navmeshes: ' + traceback.format_exc()); print('navmeshes: not written (see errors.txt)')
    with open(os.path.join(outdir, 'cells.csv'), 'w', encoding='utf-8') as f:
        f.write('cell,name,things\n')
        for c in sorted(inter): f.write('%s,"%s",%d\n' % (c, cells.get(c, ['', 0])[0].replace('"', "'"), cells.get(c, ['', 0])[1]))
    # ---- the whole world's plan: worldspaces, regions (biomes), which cells are in which
    with open(os.path.join(outdir, 'worldspaces.csv'), 'w', encoding='utf-8') as f:
        f.write('world_id,editor_id,name,parent,parent_flags\n')
        for fid, (e, nm, par, pf) in sorted(worlds.items()):
            f.write('%08X,%s,"%s",%s,%X\n' % (fid, e, nm.replace('"', "'"), worlds.get(par, [''])[0] if par else '', pf))
    with open(os.path.join(outdir, 'regions.csv'), 'w', encoding='utf-8') as f:
        f.write('editor_id,colour,world,data,outline\n')
        for e, col, w, rd, areas, _ in regions:
            f.write('%s,%s,%s,"%s","%s"\n' % (e, col, worlds.get(w, [''])[0], ' '.join('%d/%d/%d' % x for x in rd),
                    ' | '.join(';'.join('%.0f %.0f' % q for q in a) for a in areas)))
    rname = {r[5]: r[0] for r in regions}
    # (v0.4.74) the exterior cells Skyrim names itself (a load door out onto one shows its name; onto any other, its
    # worldspace's: "Skyrim" for Tamriel)
    with open(os.path.join(outdir, 'ext_cells.csv'), 'w', encoding='utf-8') as f:
        f.write('world,cell_x,cell_y,editor_id,name\n')
        for (w, x, y), (e, n) in sorted(extnames.items()):
            f.write('%s,%d,%d,%s,"%s"\n' % (w, x, y, e, n.replace('"', "'")))
    with open(os.path.join(outdir, 'cell_regions.csv'), 'w', encoding='utf-8') as f:
        f.write('world,cell_x,cell_y,regions\n')
        for (w, x, y), rl in sorted(cellreg.items()):
            f.write('%s,%d,%d,%s\n' % (w, x, y, ' '.join(rname.get(r, '#%08X' % r) for r in rl)))
    # ---- the land of the city's own worldspaces (Tamriel's is in the heightmap export)
    for w in sorted(wset):
        L = land.get(w)
        if not L: continue
        with open(os.path.join(outdir, 'land_%s.bin' % w), 'wb') as f:
            f.write(struct.pack('<4sII', b'LND1', len(L), len(tex_names)))
            for (x, y), (h, g) in sorted(L.items()):
                f.write(struct.pack('<ii', x, y)); f.write(struct.pack('<1089f', *h)); f.write(struct.pack('<1089H', *g))
        with open(os.path.join(outdir, 'land_%s.txt' % w), 'w', encoding='utf-8') as f:
            f.write('# land_%s.bin: "LND1", u32 cells, u32 textures; per cell: i32 x, i32 y, 1089 float heights (33 x 33 from\n'
                    '# the south-west, Skyrim units), 1089 u16 ground texture numbers (the names below, in order)\n' % w)
            for k, fid in enumerate(tex_names): f.write('%d %s\n' % (k, ltex.get(fid, '#%08X' % fid)))
    # ---- the models and their textures (as skyrim_meshes.py)
    want = collections.defaultdict(set)
    for b in used:
        bt, be, nm, model, ob = base.get(b, ('?', '', '', '', None))
        if model and model.lower().endswith('.nif') and bt not in ('NPC_',) and ('marker' not in model.lower() or model.lower().replace('/', '\\').startswith(('furniture', 'meshes\\furniture'))):
            want[model].add(be)
    print('models wanted %d' % len(want))
    meshes, textures = [], []
    for fn in sorted(os.listdir(data_dir)):
        if fn.lower().endswith('.bsa') and fn.lower().startswith('skyrim'):
            try:
                a = sm.BSA(os.path.join(data_dir, fn))
                (meshes if 'mesh' in fn.lower() else textures if 'texture' in fn.lower() else []).append(a)
                print('  %s: %d files' % (fn, len(a.files)))
            except Exception as ex: errors.append('%s: %s' % (fn, ex))
    def find(arcs, nm):
        nm = nm.replace('/', '\\').lower()
        for a in arcs:
            if nm in a.files: return a
        return None
    rows = []; texused = collections.Counter()
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
        try: nif, shapes = sm.nif_shapes(data)
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
        if k % 100 == 0: print('  models %d / %d' % (k, len(want)))
    with open(os.path.join(outdir, 'models.csv'), 'w', encoding='utf-8') as f:
        f.write('objects,model,status,shapes,vertices,triangles,bounds\n')
        for r in rows: f.write('"%s","%s",%s,%d,%d,%d,%s\n' % r)
    print('textures:', len(texused))
    with open(os.path.join(outdir, 'textures.csv'), 'w', encoding='utf-8') as f:
        f.write('texture,used_by_shapes,format,width,height,average_rgb,darker_half_rgb,lighter_half_rgb\n')
        for t, n in texused.most_common():
            if not t: continue
            p = sm.texture_path(t)
            a = find(textures, p); res = None
            try:
                if a is not None: res = sm.save_texture(outdir, p, a.read(p), errors)
            except Exception as ex: errors.append('%s: %s' % (p, ex))
            if res is None: f.write('%s,%d,missing,,,,,\n' % (t, n)); continue
            fmt, w, h, cols = res
            cs = ['%02X%02X%02X' % c for c in cols] if cols else ['', '', '']
            f.write('%s,%d,%s,%d,%d,%s,%s,%s\n' % (t, n, fmt, w, h, cs[0], cs[1], cs[2]))
    with open(os.path.join(outdir, 'errors.txt'), 'w') as f: f.write('\n'.join(errors) if errors else 'none\n')
    print('done ->', outdir, '(%d problems, see errors.txt)' % len(errors))
    sm.make_zip(outdir)

if __name__ == '__main__':
    main()
