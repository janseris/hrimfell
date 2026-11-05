#!/usr/bin/env python3
# Skyrim's creatures and their gear for Hrimfell: the draugr (bodies, heads, armour, weapons), with what moves each
# vertex - its bones and their weights - so the game can pose Skyrim's own bodies in its walk, attack and death
# frames (instead of the shapes it builds itself). Also the skeletons, skeevers, frostbite spiders, trolls and
# wolves, for later.
#
# Usage (Windows, Python 3.8+ from python.org, nothing else needed), next to skyrim_meshes.py:
#   python skyrim_actors.py                  (or: python skyrim_actors.py path\to\Skyrim\Data)
# Only reads; takes a few minutes. Writes a folder "skyrim_actors" next to this script and packs it into
# skyrim_actors.zip - send that file:
#   models.csv        every model taken: its file, shapes, vertices, triangles, bones, how it went
#   meshes\*.obj      each model's geometry as it is drawn standing in its rest pose (Skyrim units, x east, y north,
#                     z up), one object per shape, its diffuse texture as the material name, its texture coordinates
#   meshes\*.skin.txt each skinned shape's bones and weights (in the .obj's order):
#                       shape <k> <name> verts <n>
#                       xf <13 numbers>            the shape's place in the model (rotation 3x3 by rows, x y z, scale):
#                                                  the .obj's points are xf applied to the shape's own
#                       skin <13 numbers>          NiSkinData's overall transform
#                       bone <i> <name> <13 numbers>   each bone: from the shape's space into the bone's (its rest)
#                       part <p> bones <i i ...>   each partition's bones (by the bone numbers above)
#                       part <p> verts <v v ...>   the shape's vertices it holds
#                       w <v> <b0> <b1> <b2> <b3> <w0> <w1> <w2> <w3>   a vertex's four bones (as stored) and weights
#   skeletons\*.txt   each skeleton.nif's bones: "node <name> <parent> <13 numbers>" (its place in its parent)
#   records.csv       the races, armour, armour addons, outfits, leveled lists and NPCs of these creatures, with
#                     the models and records they name (editor ids)
#   textures\         their textures (64, 128, and 256 where Skyrim made them big), as skyrim_meshes.py writes them
#   errors.txt        anything that could not be read
import os, sys, struct, zlib, re, zipfile, collections, traceback
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import skyrim_meshes as sm

# the folders taken whole (meshes\...): the draugr first
FOLDERS = ('actors\\draugr\\', 'armor\\draugr\\', 'weapons\\draugr\\', 'actors\\skeleton\\', 'actors\\skeever\\',
           'actors\\frostbitespider\\', 'actors\\troll\\', 'actors\\canine\\')
RECORD_WORDS = ('draugr', 'skeleton', 'skeever', 'spider', 'troll', 'wolf', 'canine', 'dragonpriest')

# ---------------------------------------------------------------- the skin of a shape
def skin_info(nif, skin, nverts):
    d = nif.d; q = nif.off[skin]
    sdata, spart, sroot, nb = struct.unpack_from('<iiiI', d, q); q += 16
    brefs = struct.unpack_from('<%di' % nb, d, q)
    names = []
    for b in brefs:
        try: names.append(nif.avobject(nif.off[b])[1] if 0 <= b < nif.n else '?')
        except Exception: names.append('?')
    out = {'names': names, 'bind': [None] * nb, 'skin': None, 'parts': [], 'w': {}}
    if 0 <= sdata < nif.n and nif.tname(sdata) == 'NiSkinData':
        p = nif.off[sdata]
        out['skin'] = struct.unpack_from('<13f', d, p); p += 52
        n2 = struct.unpack_from('<I', d, p)[0]; p += 4
        hasw = d[p]; p += 1
        for i in range(n2):
            t = struct.unpack_from('<13f', d, p); p += 52
            p += 16                                                    # bounding sphere
            nvw = struct.unpack_from('<H', d, p)[0]; p += 2
            if i < nb: out['bind'][i] = t
            if hasw:
                for k in range(nvw):
                    vi, w = struct.unpack_from('<Hf', d, p); p += 6
                    out['w'].setdefault(vi, []).append((i, w))
    if 0 <= spart < nif.n and nif.tname(spart) == 'NiSkinPartition':
        p = nif.off[spart]
        nparts = struct.unpack_from('<I', d, p)[0]; p += 4
        dsize, vsize = struct.unpack_from('<II', d, p); p += 8
        desc = struct.unpack_from('<Q', d, p)[0]; p += 8
        vstart = p; p += dsize
        so = ((desc >> 28) & 15) * 4
        attr = (desc >> 44) & 0xFFF
        nv = dsize // vsize if vsize else 0
        raw = {}
        if attr & 0x40:                                                # skinned: four half weights, four bone bytes
            for k in range(nv):
                r = vstart + k * vsize + so
                ws = [sm.half(h) for h in struct.unpack_from('<4H', d, r)]
                bs = list(struct.unpack_from('<4B', d, r + 8))
                raw[k] = (bs, ws)
        for k in range(nparts):
            pnv, pnt, pnb, pns, pnw = struct.unpack_from('<5H', d, p); p += 10
            bones = list(struct.unpack_from('<%dH' % pnb, d, p)); p += 2 * pnb
            hasmap = d[p]; p += 1
            vmap = list(struct.unpack_from('<%dH' % pnv, d, p)) if hasmap else list(range(pnv)); p += 2 * pnv if hasmap else 0
            hasw = d[p]; p += 1
            pw = None
            if hasw: pw = struct.unpack_from('<%df' % (pnv * pnw), d, p); p += 4 * pnv * pnw
            slen = struct.unpack_from('<%dH' % pns, d, p) if pns else (); p += 2 * pns
            hasf = d[p]; p += 1
            if hasf:
                if pns: p += 2 * sum(slen)
                else: p += 6 * pnt
            hasb = d[p]; p += 1
            pb = None
            if hasb: pb = d[p:p + pnv * pnw]; p += pnv * pnw
            p += 1 + 1 + 8
            p += 6 * pnt
            out['parts'].append((bones, vmap))
            # (the older layout: each partition's own weights and bone numbers, those of its bone list)
            if not raw and pw is not None and pb is not None:
                for j, v in enumerate(vmap):
                    bs = [pb[j * pnw + m] for m in range(pnw)][:4]; ws = [pw[j * pnw + m] for m in range(pnw)][:4]
                    while len(bs) < 4: bs.append(0); ws.append(0.0)
                    raw[v] = (bs, ws)
        out['raw'] = raw
    return out

def shape_list(nif):
    # each shape (in the order skyrim_meshes.nif_shapes visits them) with its block number and its place in the model
    out = []
    def visit(i, chain, depth):
        if i < 0 or i >= nif.n or depth > 60: return
        tn = nif.tname(i)
        if tn in sm.SHAPES:
            if tn == 'BSDynamicTriShape': return
            out.append((i, chain))
        elif 'Node' in tn or tn in ('BSFadeNode', 'BSLeafAnimNode', 'BSTreeNode', 'BSOrderedNode', 'BSMultiBoundNode'):
            try: xf, kids = nif.children(i)
            except Exception: return
            for k in kids: visit(k, [xf] + chain, depth + 1)
    visit(0, [], 0)
    return out

def xf_mul(a, b):
    # the transform doing b, then a (each: t, r by rows, s)
    ta, ra, sa = a; tb, rb, sb = b
    r = [sum(ra[i * 3 + k] * rb[k * 3 + j] for k in range(3)) for i in range(3) for j in range(3)]
    t = sm.xf_apply(a, tb)
    return (t, r, sa * sb)

def flat(xf):
    t, r, s = xf
    return list(r) + list(t) + [s]
def fmt(v): return ' '.join('%.6g' % x for x in v)

def skeleton_text(nif):
    lines = []
    def visit(i, parent, depth):
        if i < 0 or i >= nif.n or depth > 80: return
        tn = nif.tname(i)
        if 'Node' not in tn: return
        try: xf, kids = nif.children(i)
        except Exception: return
        nm = nif.avobject(nif.off[i])[1]
        lines.append('node %s|%s|%s' % (nm, parent, fmt(flat(xf))))
        for k in kids: visit(k, nm, depth + 1)
    visit(0, '-', 0)
    return '\n'.join(lines) + '\n'

# ---------------------------------------------------------------- the records
def read_records(path, words):
    with open(path, 'rb') as f: buf = f.read()
    recs = {}                                                            # formid -> (type, edid, [(sub, value)])
    want = (b'RACE', b'ARMO', b'ARMA', b'OTFT', b'LVLN', b'NPC_', b'HDPT', b'WEAP', b'LVLI')
    def walk(pos, end):
        while pos < end:
            typ = buf[pos:pos + 4]; size = struct.unpack_from('<I', buf, pos + 4)[0]
            if typ == b'GRUP':
                label = buf[pos + 8:pos + 12]; gtype = struct.unpack_from('<i', buf, pos + 12)[0]
                if gtype == 0 and label not in want: pos += size; continue
                walk(pos + 24, pos + size); pos += size; continue
            flags, fid = struct.unpack_from('<II', buf, pos + 8)
            data = buf[pos + 24:pos + 24 + size]; pos += 24 + size
            if typ not in want: continue
            if flags & 0x00040000:
                try: data = zlib.decompress(data[4:])
                except zlib.error: continue
            subs = sm.subrecords(data)
            edid = ''; fields = []
            for t, d in subs:
                if t == b'EDID': edid = sm.zstr(d)
                elif t in (b'MODL', b'MOD2', b'MOD3', b'MOD4', b'MOD5') and d[:1] != b'\0' and len(d) > 4 and d[-1:] == b'\0' and all(32 <= c < 127 for c in d[:-1]):
                    fields.append((t.decode(), sm.zstr(d)))
                elif t in (b'RNAM', b'WNAM', b'DOFT', b'TPLT', b'CNAM', b'INAM', b'PNAM', b'MODL', b'SNAM', b'ANAM') and len(d) >= 4:
                    if t == b'INAM' and typ == b'OTFT':
                        fields += [('INAM', '%08X' % v) for v in struct.unpack_from('<%dI' % (len(d) // 4), d)]
                    elif len(d) == 4: fields.append((t.decode(), '%08X' % struct.unpack_from('<I', d)[0]))
                elif t == b'LVLO' and len(d) >= 8:
                    lvl, _, ref = struct.unpack_from('<HHI', d); fields.append(('LVLO', '%d:%08X' % (lvl, ref)))
            recs[fid] = (typ.decode(), edid, fields)
    sys.setrecursionlimit(10000)
    walk(0, len(buf))
    return recs

def main():
    data_dir, _ = sm.data_arg(sys.argv[1:])
    if not os.path.exists(os.path.join(data_dir, 'Skyrim.esm')): print('Skyrim.esm not found in', data_dir, '- give the Data folder: skyrim_actors.py path\\to\\Data'); return
    outdir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'skyrim_actors')
    for sub in ('meshes', 'textures', 'skeletons'): os.makedirs(os.path.join(outdir, sub), exist_ok=True)
    errors = []
    print('reading Skyrim.esm')
    try:
        recs = read_records(os.path.join(data_dir, 'Skyrim.esm'), RECORD_WORDS)
        ed = {fid: r[1] for fid, r in recs.items()}
        with open(os.path.join(outdir, 'records.csv'), 'w', encoding='utf-8') as f:
            f.write('type,formid,editor_id,fields\n')
            for fid, (t, e, fl) in sorted(recs.items()):
                if not any(w in e.lower() for w in RECORD_WORDS) and not any(w in v.lower() for k, v in fl if k.startswith('MOD')): continue
                parts = []
                for k, v in fl:
                    if k.startswith('MOD'): parts.append('%s=%s' % (k, v))
                    elif ':' in v: lv, ref = v.split(':'); parts.append('%s=%s:%s' % (k, lv, ed.get(int(ref, 16), ref)))
                    else: parts.append('%s=%s' % (k, ed.get(int(v, 16), v)))
                f.write('%s,%08X,%s,"%s"\n' % (t, fid, e, ';'.join(parts).replace('"', "'")))
    except Exception: errors.append('records:\n' + traceback.format_exc())
    meshes, textures = [], []
    for fn in sorted(os.listdir(data_dir)):
        if fn.lower().endswith('.bsa') and fn.lower().startswith('skyrim'):
            try:
                a = sm.BSA(os.path.join(data_dir, fn))
                (meshes if 'mesh' in fn.lower() else textures if 'texture' in fn.lower() else []).append(a)
            except Exception as ex: errors.append('%s: %s' % (fn, ex))
    names = sorted(set(n for a in meshes for n in a.files if n.endswith('.nif') and n.startswith(tuple('meshes\\' + f for f in FOLDERS))
                       and not n.endswith('_0.nif') and '\\animations\\' not in n and 'facegen' not in n))
    print('models: %d' % len(names))
    rows = []; texused = collections.Counter()
    for k, path in enumerate(names):
        a = next(x for x in meshes if path in x.files)
        try: data = a.read(path)
        except Exception as ex: errors.append('%s: %s' % (path, ex)); continue
        stem = re.sub(r'[^A-Za-z0-9_.-]', '_', path[7:].replace('\\', '_'))[:-4]
        try:
            nif = sm.NIF(data)
            if path.endswith('skeleton.nif') or path.endswith('skeleton_female.nif') or path.endswith('skeletonbeast.nif'):
                open(os.path.join(outdir, 'skeletons', stem + '.txt'), 'w').write('# %s\n' % path + skeleton_text(nif))
                rows.append((path, 'skeleton', 0, 0, 0, 0)); continue
            shapes = shape_list(nif)
            obj = ['# %s' % path]; skin = ['# %s' % path]; vb = 1; nvt = ntt = nbones = 0
            for sk, (i, chain) in enumerate(shapes):
                nm, xf, verts, uvs, cols, tris, tex = nif.shape(i)
                place = xf
                for c in chain: place = xf_mul(c, place)
                pts = [sm.xf_apply(place, v) for v in verts]
                texused[tex.lower()] += 1
                obj.append('o %s' % (nm or 'shape')); obj.append('usemtl %s' % (tex or '-'))
                if getattr(nif, 'last_alpha', ''): obj.append('# alpha %s' % nif.last_alpha)
                obj += ['v %.2f %.2f %.2f' % v for v in pts]
                obj += ['vt %.4f %.4f' % uv for uv in uvs]
                if uvs: obj += ['f %d/%d %d/%d %d/%d' % (t[0] + vb, t[0] + vb, t[1] + vb, t[1] + vb, t[2] + vb, t[2] + vb) for t in tris]
                else: obj += ['f %d %d %d' % (t[0] + vb, t[1] + vb, t[2] + vb) for t in tris]
                vb += len(pts); nvt += len(pts); ntt += len(tris)
                # its skin
                d = nif.d; p = nif.off[i]
                p2, _, _ = nif.avobject(p); p2 += 16
                sref = struct.unpack_from('<i', d, p2)[0]
                if 0 <= sref < nif.n:
                    try:
                        si = skin_info(nif, sref, len(verts))
                        skin.append('shape %d %s verts %d' % (sk, nm or 'shape', len(verts)))
                        skin.append('xf ' + fmt(flat(place)))
                        if si['skin']: skin.append('skin ' + fmt(si['skin']))
                        for b, (bn, bt) in enumerate(zip(si['names'], si['bind'])):
                            skin.append('bone %d %s|%s' % (b, bn, fmt(bt) if bt else '-'))
                        nbones = max(nbones, len(si['names']))
                        for pi, (bones, vmap) in enumerate(si['parts']):
                            skin.append('part %d bones %s' % (pi, ' '.join(map(str, bones))))
                            skin.append('part %d verts %s' % (pi, ' '.join(map(str, vmap))))
                        raw = si.get('raw') or {}
                        if raw:
                            for v in range(len(verts)):
                                if v in raw: bs, ws = raw[v]; skin.append('w %d %d %d %d %d %.4f %.4f %.4f %.4f' % tuple([v] + bs + ws))
                        else:
                            for v, lst in sorted(si['w'].items()):
                                lst = sorted(lst, key=lambda x: -x[1])[:4]
                                while len(lst) < 4: lst.append((0, 0.0))
                                skin.append('w %d %d %d %d %d %.4f %.4f %.4f %.4f' % tuple([v] + [x[0] for x in lst] + [x[1] for x in lst]))
                            skin.append('# weights from NiSkinData: bone numbers are the shape\'s own')
                    except Exception: errors.append('%s shape %d skin:\n%s' % (path, sk, traceback.format_exc()))
            open(os.path.join(outdir, 'meshes', stem + '.obj'), 'w').write('\n'.join(obj) + '\n')
            if len(skin) > 1: open(os.path.join(outdir, 'meshes', stem + '.skin.txt'), 'w').write('\n'.join(skin) + '\n')
            rows.append((path, 'ok' if nvt else 'no shapes', len(shapes), nvt, ntt, nbones))
        except Exception:
            errors.append('%s:\n%s' % (path, traceback.format_exc())); rows.append((path, 'unreadable', 0, 0, 0, 0))
        if k % 20 == 0: print('  %d / %d' % (k, len(names)))
    with open(os.path.join(outdir, 'models.csv'), 'w', encoding='utf-8') as f:
        f.write('model,status,shapes,vertices,triangles,bones\n')
        for r in rows: f.write('"%s",%s,%d,%d,%d,%d\n' % r)
    print('textures:', len(texused))
    with open(os.path.join(outdir, 'textures.csv'), 'w', encoding='utf-8') as f:
        f.write('texture,used_by_shapes,format,width,height,average_rgb,darker_half_rgb,lighter_half_rgb\n')
        for t, n in texused.most_common():
            if not t: continue
            p = sm.texture_path(t)
            a = next((x for x in textures if p in x.files), None); res = None
            try:
                if a is not None: res = sm.save_texture(outdir, p, a.read(p), errors)
            except Exception as ex: errors.append('%s: %s' % (p, ex))
            if res is None: f.write('%s,%d,missing,,,,,\n' % (t, n)); continue
            fmt_, w, h, cols = res
            cs = ['%02X%02X%02X' % c for c in cols] if cols else ['', '', '']
            f.write('%s,%d,%s,%d,%d,%s,%s,%s\n' % (t, n, fmt_, w, h, cs[0], cs[1], cs[2]))
    with open(os.path.join(outdir, 'errors.txt'), 'w') as f: f.write('\n'.join(errors) if errors else 'none\n')
    print('done ->', outdir, '(%d problems, see errors.txt)' % len(errors))
    sm.make_zip(outdir)

if __name__ == '__main__':
    main()
