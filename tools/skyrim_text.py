#!/usr/bin/env python3
# Export Skyrim's words for Hrimfell: names, people, dialogue, quest journal lines and book texts.
# The companion of skyrim_heightmap.py and skyrim_places.py.
#
# Usage (Windows, Python 3.8+ from python.org, nothing else needed):
#   python skyrim_text.py [language]
# Skyrim is looked for in C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\Data; elsewhere,
# give its path first (the Data folder, or Skyrim.esm in it).
# Takes a few minutes. Reads Skyrim.esm and the strings files: loose ones in Data\Strings if they are there,
# otherwise from inside "Skyrim - Interface.bsa" (Special Edition) or "Skyrim - Interface.bsa"/"Skyrim - Misc.bsa"
# (Legendary Edition). English is used if present, else the first language found (or add a language name as a
# second argument, e.g. german).
#
# Writes into a folder "skyrim_text" next to this script:
#   names.csv        every named record: type, form id, editor id, name (locations, cells, people, items, ...)
#   npcs.csv         people: editor id, name, race, female, class, outfit, voice, height, weight, factions
#   dialogue.csv     every line a person speaks: quest, topic (and what you say to start it), the speaker's
#                    editor id (from the line's conditions), the response number, the text, the emotion
#   quests.csv       quests: editor id, name, the journal entry and the objective text of every stage
#   books.csv        books: editor id, name, the whole text
#   markers.csv      map markers with their names (x, y, z, kind, name)
# At the end the folder is packed into a .zip next to it (e.g. skyrim_text.zip): send that file.
import os, sys, struct, zlib, csv

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
    out = []; p = 0; big = None
    while p + 6 <= len(data):
        t = data[p:p + 4]; n = struct.unpack_from('<H', data, p + 4)[0]; p += 6
        if big is not None: n = big; big = None
        if t == b'XXXX': big = struct.unpack_from('<I', data, p)[0]; p += n; continue
        out.append((t, data[p:p + n])); p += n
    return out

def zstr(d): return d.split(b'\0')[0].decode('cp1252', 'replace')
def u32(d, o=0): return struct.unpack_from('<I', d, o)[0]

# ------------------------------------------------------------------ LZ4 (the Special Edition's .bsa compression)
def lz4_block(src, out):
    p = 0; n = len(src)
    while p < n:
        tok = src[p]; p += 1
        lit = tok >> 4
        if lit == 15:
            while True:
                b = src[p]; p += 1; lit += b
                if b != 255: break
        out += src[p:p + lit]; p += lit
        if p >= n: break
        off = src[p] | (src[p + 1] << 8); p += 2
        ml = tok & 15
        if ml == 15:
            while True:
                b = src[p]; p += 1; ml += b
                if b != 255: break
        ml += 4
        start = len(out) - off
        if off >= ml: out += out[start:start + ml]
        else:
            for i in range(ml): out.append(out[start + i])
    return out

def lz4_frame(d):
    if u32(d) != 0x184D2204: raise ValueError('not an LZ4 frame')
    flg = d[4]; p = 6
    if flg & 0x08: p += 8                  # content size
    if flg & 0x01: p += 4                  # dictionary id
    p += 1                                 # header checksum
    out = bytearray()
    while True:
        bs = u32(d, p); p += 4
        if bs == 0: break
        size = bs & 0x7FFFFFFF
        blk = d[p:p + size]; p += size
        if bs & 0x80000000: out += blk
        else: lz4_block(blk, out)
        if flg & 0x10: p += 4              # block checksum
    return bytes(out)

# ------------------------------------------------------------------ .bsa archives (version 104: Legendary, 105: Special)
def bsa_files(path, want):
    """{lowercase 'folder\\name': bytes} for the files whose name passes want(name)"""
    with open(path, 'rb') as f: b = f.read()
    if b[:4] != b'BSA\0': return {}
    ver, off, aflags, nfold, nfile, lfold, lfile = struct.unpack_from('<7I', b, 4)
    p = off
    folders = []
    for i in range(nfold):
        if ver == 105: h, cnt, _, foff = struct.unpack_from('<QIIQ', b, p); p += 24
        else: h, cnt, foff = struct.unpack_from('<QII', b, p); p += 16
        folders.append(cnt)
    recs = []
    for cnt in folders:
        name = ''
        if aflags & 1:
            ln = b[p]; name = b[p + 1:p + ln].split(b'\0')[0].decode('cp1252'); p += 1 + ln
        for j in range(cnt):
            h, size, foff = struct.unpack_from('<QII', b, p); p += 16
            recs.append([name, size, foff])
    names = []
    if aflags & 2:
        blob = b[p:p + lfile]; names = [n.decode('cp1252') for n in blob.split(b'\0')[:len(recs)]]
    out = {}
    for i, (folder, size, foff) in enumerate(recs):
        nm = (folder + '\\' + (names[i] if i < len(names) else '')).lower()
        if not want(nm): continue
        comp = bool(aflags & 4) != bool(size & 0x40000000)
        size &= 0x3FFFFFFF
        q = foff
        if aflags & 0x100:                 # the full name before the data
            ln = b[q]; q += 1 + ln; size -= 1 + ln
        data = b[q:q + size]
        if comp:
            raw = data[4:]
            data = lz4_frame(raw) if ver == 105 else zlib.decompress(raw)
        out[nm] = data
    return out

def parse_strings(data, kind):
    n, size = struct.unpack_from('<II', data, 0)
    base = 8 + n * 8; out = {}
    for i in range(n):
        sid, off = struct.unpack_from('<II', data, 8 + i * 8)
        if kind == 'strings':
            e = data.find(b'\0', base + off); out[sid] = data[base + off:e].decode('cp1252', 'replace')
        else:
            ln = u32(data, base + off); out[sid] = data[base + off + 4:base + off + 4 + ln].split(b'\0')[0].decode('cp1252', 'replace')
    return out

def load_strings(esm, lang):
    dd = os.path.dirname(os.path.abspath(esm))
    files = {}
    sd = os.path.join(dd, 'Strings')
    if os.path.isdir(sd):
        for fn in os.listdir(sd):
            if fn.lower().startswith('skyrim_'): files['strings\\' + fn.lower()] = open(os.path.join(sd, fn), 'rb').read()
    if not files:
        for bn in sorted(os.listdir(dd)):
            if bn.lower().endswith('.bsa') and bn.lower().startswith('skyrim'):
                try: files.update(bsa_files(os.path.join(dd, bn), lambda n: n.startswith('strings\\skyrim_')))
                except Exception as ex: print('  (could not read %s: %s)' % (bn, ex))
                if files: print('strings from', bn); break
    langs = sorted(set(n.split('_', 1)[1].rsplit('.', 1)[0] for n in files))
    if not langs: print('no strings files found - names will be empty'); return {}, {}, {}
    use = lang if lang in langs else ('english' if 'english' in langs else langs[0])
    print('languages:', ', '.join(langs), '- using', use)
    get = lambda ext: parse_strings(files.get('strings\\skyrim_%s.%s' % (use, ext), b'\0' * 8), ext)
    return get('strings'), get('dlstrings'), get('ilstrings')

def main():
    path, rest = esm_arg(sys.argv[1:])
    if not os.path.exists(path): print('Skyrim.esm not found at', path, '- give its path: skyrim_text.py path\\to\\Skyrim.esm [language]'); return
    lang = rest[0].lower() if rest else 'english'
    S, DL, IL = load_strings(path, lang)
    print('strings: %d, %d, %d' % (len(S), len(DL), len(IL)))
    def name(d, table=None):
        t = S if table is None else table
        if len(d) == 4: return t.get(u32(d), '')
        return zstr(d)
    print('reading', path)
    with open(path, 'rb') as f: buf = f.read()
    outdir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'skyrim_text')
    os.makedirs(outdir, exist_ok=True)

    edid = {}                    # form id -> editor id
    names, npcs, quests, books, markers = [], [], [], [], []
    infos = []                   # (quest fid, topic fid, info fid, speaker fids, responses)
    topics = {}                  # topic fid -> (edid, player text, quest fid)
    cur_topic = [None]

    def walk(pos, end):
        while pos < end:
            typ = buf[pos:pos + 4]; size = u32(buf, pos + 4)
            if typ == b'GRUP':
                gtype = struct.unpack_from('<i', buf, pos + 12)[0]
                if gtype == 7: cur_topic[0] = u32(buf, pos + 8)      # a topic's children: its INFOs
                walk(pos + 24, pos + size); pos += size; continue
            flags, fid = struct.unpack_from('<II', buf, pos + 8)
            data = buf[pos + 24:pos + 24 + size]; pos += 24 + size
            if typ in (b'NAVM', b'NAVI', b'LAND', b'REFR', b'PGRE', b'PHZD'):
                # (placed things: only map markers are wanted from them)
                if typ != b'REFR': continue
                if flags & 0x00040000:
                    try: data = zlib.decompress(data[4:])
                    except zlib.error: continue
                if b'XMRK' not in data: continue
                subs = subrecords(data); pos3 = None; nm = ''; kind = ''
                for t, d in subs:
                    if t == b'DATA' and len(d) >= 12: pos3 = struct.unpack_from('<3f', d)
                    elif t == b'FULL': nm = name(d)
                    elif t == b'TNAM' and len(d) >= 1: kind = d[0]
                if pos3: markers.append(('%.1f' % pos3[0], '%.1f' % pos3[1], '%.1f' % pos3[2], kind, nm))
                continue
            if flags & 0x00040000:
                try: data = zlib.decompress(data[4:])
                except zlib.error: continue
            subs = subrecords(data)
            e = ''; full = ''
            for t, d in subs:
                if t == b'EDID': e = zstr(d); edid[fid] = e
                elif t == b'FULL' and not full: full = name(d)
            T = typ.decode('latin-1')
            if full and T not in ('INFO', 'DIAL'): names.append((T, '%08X' % fid, e, full))
            if typ == b'NPC_':
                n = {'race': 0, 'female': 0, 'class': 0, 'outfit': 0, 'voice': 0, 'height': 0, 'weight': 0, 'factions': []}
                for t, d in subs:
                    if t == b'ACBS' and len(d) >= 4: n['female'] = u32(d) & 1
                    elif t == b'RNAM': n['race'] = u32(d)
                    elif t == b'CNAM': n['class'] = u32(d)
                    elif t == b'DOFT': n['outfit'] = u32(d)
                    elif t == b'VTCK': n['voice'] = u32(d)
                    elif t == b'NAM6' and len(d) >= 4: n['height'] = struct.unpack_from('<f', d)[0]
                    elif t == b'NAM7' and len(d) >= 4: n['weight'] = struct.unpack_from('<f', d)[0]
                    elif t == b'SNAM' and len(d) >= 4: n['factions'].append(u32(d))
                npcs.append((fid, e, full, n))
            elif typ == b'DIAL':
                q = 0
                for t, d in subs:
                    if t == b'QNAM' and len(d) >= 4: q = u32(d)
                topics[fid] = (e, full, q)
            elif typ == b'INFO':
                spk = []; resp = []; cur = None; prompt = ''
                for t, d in subs:
                    if t == b'CTDA' and len(d) >= 16:
                        func = struct.unpack_from('<H', d, 8)[0]
                        if func == 72: spk.append(u32(d, 12))          # GetIsID
                    elif t == b'TRDT' and len(d) >= 16:
                        cur = {'emotion': u32(d), 'value': u32(d, 4), 'n': d[12], 'text': ''}; resp.append(cur)
                    elif t == b'NAM1' and cur is not None: cur['text'] = name(d, IL)
                    elif t == b'RNAM': prompt = name(d)
                infos.append((cur_topic[0], fid, spk, resp, prompt))
            elif typ == b'QUST':
                stages = []; idx = None
                for t, d in subs:
                    if t == b'INDX' and len(d) >= 2: idx = struct.unpack_from('<H', d)[0]
                    elif t == b'CNAM' and idx is not None: stages.append((idx, 'journal', name(d, DL)))
                    elif t == b'QOBJ' and len(d) >= 2: idx = struct.unpack_from('<H', d)[0]
                    elif t == b'NNAM' and idx is not None: stages.append((idx, 'objective', name(d, DL)))
                quests.append((e, full, stages))
            elif typ == b'BOOK':
                txt = ''
                for t, d in subs:
                    if t == b'DESC': txt = name(d, DL)
                books.append((e, full, txt))

    sys.setrecursionlimit(10000)
    walk(0, len(buf))
    W = lambda fn: csv.writer(open(os.path.join(outdir, fn), 'w', encoding='utf-8', newline=''))
    w = W('names.csv'); w.writerow(['type', 'form_id', 'editor_id', 'name'])
    for r in names: w.writerow(r)
    w = W('npcs.csv'); w.writerow(['editor_id', 'name', 'race', 'female', 'class', 'outfit', 'voice', 'height', 'weight', 'factions', 'form_id'])
    for fid, e, full, n in npcs:
        w.writerow([e, full, edid.get(n['race'], ''), n['female'], edid.get(n['class'], ''), edid.get(n['outfit'], ''),
                    edid.get(n['voice'], ''), '%.2f' % n['height'], '%.0f' % n['weight'], ' '.join(edid.get(f, '') for f in n['factions']), '%08X' % fid])
    w = W('dialogue.csv'); w.writerow(['quest', 'topic', 'you_say', 'speaker', 'info_id', 'response', 'text', 'emotion'])
    emo = ['neutral', 'anger', 'disgust', 'fear', 'sad', 'happy', 'surprise', 'puzzled']
    for tf, fid, spk, resp, prompt in infos:
        te, tfull, q = topics.get(tf, ('', '', 0))
        who = ' '.join(edid.get(s, '%08X' % s) for s in spk)
        for r in resp:
            w.writerow([edid.get(q, ''), te, prompt or tfull, who, '%08X' % fid, r['n'], r['text'], emo[r['emotion']] if r['emotion'] < 8 else r['emotion']])
    w = W('quests.csv'); w.writerow(['quest', 'name', 'stage', 'kind', 'text'])
    for e, full, st in quests:
        for idx, kind, txt in st: w.writerow([e, full, idx, kind, txt])
    w = W('books.csv'); w.writerow(['editor_id', 'name', 'text'])
    for r in books: w.writerow(r)
    w = W('markers.csv'); w.writerow(['x', 'y', 'z', 'kind', 'name'])
    for r in markers: w.writerow(r)
    print('names %d, people %d, dialogue lines %d, quests %d, books %d, markers %d' %
          (len(names), len(npcs), sum(len(i[3]) for i in infos), len(quests), len(books), len(markers)))
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
