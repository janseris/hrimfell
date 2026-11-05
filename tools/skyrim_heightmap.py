#!/usr/bin/env python3
# Export Skyrim's real landscape (the Tamriel worldspace) from Skyrim.esm, for Hrimfell.
#
# Usage (Windows, Python 3.8+ from python.org, nothing else needed):
#   python skyrim_heightmap.py
# Skyrim is looked for in C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\Data; elsewhere,
# give its path first (the Data folder, or Skyrim.esm in it).
# (Oldrim / Legendary Edition works too: ...\common\Skyrim\Data\Skyrim.esm)
#
# Writes into a folder "tamriel_export" next to this script:
#   tamriel_height.raw   heights, 16-bit signed little-endian, one per vertex (every 128 game units = 1.83 m),
#                        rows from NORTH to SOUTH, columns WEST to EAST; value * 8 = game units
#   tamriel_height.pgm   the same as a 16-bit greyscale picture you can look at (GIMP, IrfanView)
#   tamriel_height.png   a small 8-bit preview (1 pixel per 4 vertices)
#   tamriel_water.csv    water level of every cell that has one (cell x, cell y, height in game units)
#   tamriel_ground.pgm   the main ground texture of every quarter cell (index into tamriel_ground.txt)
#   tamriel_ground.txt   the ground texture names (LTEX editor ids: LSnow01, LGrass01, LRoad01 ...)
#   tamriel_cells.csv    every cell with a name (cell x, cell y, editor id, full name) - towns, forts ...
#   tamriel_info.txt     sizes, the cell range and the height range
# At the end the folder is packed into a .zip next to it (e.g. tamriel_export.zip): send that file.
#
# Only Skyrim.esm is read; nothing is written next to your game.
import os, sys, struct, zlib

DEFAULT_DATA = r'C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\Data'

def esm_arg(args):
    # the first argument when it is a path (Skyrim.esm or the Data folder); otherwise Skyrim.esm in the usual Steam
    # folder. -> (path to Skyrim.esm, the remaining arguments)
    if args and (args[0].lower().endswith('.esm') or os.path.isdir(args[0]) or '\\' in args[0] or '/' in args[0]):
        p, args = args[0], args[1:]
    else: p = DEFAULT_DATA
    if os.path.isdir(p): p = os.path.join(p, 'Skyrim.esm')
    return p, args

GRID = 32                     # quads per cell side (33 x 33 vertices)

def records(buf, pos, end, out):
    """walk records and groups between pos and end; call out(kind, ...)"""
    while pos < end:
        typ = buf[pos:pos + 4]
        size = struct.unpack_from('<I', buf, pos + 4)[0]
        if typ == b'GRUP':
            label = buf[pos + 8:pos + 12]
            gtype = struct.unpack_from('<i', buf, pos + 12)[0]
            out('grup_start', label, gtype, pos)
            records(buf, pos + 24, pos + size, out)
            out('grup_end', label, gtype, pos)
            pos += size
        else:
            flags, fid = struct.unpack_from('<II', buf, pos + 8)
            data = buf[pos + 24:pos + 24 + size]
            if flags & 0x00040000:              # compressed: uint32 size, then zlib
                try: data = zlib.decompress(data[4:])
                except zlib.error: data = b''
            out('record', typ, fid, data)
            pos += 24 + size

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

def main():
    path, _ = esm_arg(sys.argv[1:])
    if not os.path.exists(path): print('Skyrim.esm not found at', path, '- give its path: skyrim_heightmap.py path\\to\\Skyrim.esm'); return
    print('reading', path)
    with open(path, 'rb') as f: buf = f.read()
    outdir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'tamriel_export')
    os.makedirs(outdir, exist_ok=True)

    ltex = {}                 # formid -> editor id
    cells = {}                # (x, y) -> dict
    state = {'world': None, 'in_tamriel': False, 'tamriel_id': None, 'cell': None, 'wrld_dnam': None}
    depth = []

    def out(kind, *a):
        if kind == 'grup_start':
            label, gtype, pos = a
            depth.append((label, gtype))
            if gtype == 1:    # world children: label = the WRLD form id
                wid = struct.unpack('<I', label)[0]
                state['in_tamriel'] = (wid == state['tamriel_id'])
            return
        if kind == 'grup_end':
            label, gtype, pos = a
            depth.pop()
            if gtype == 1: state['in_tamriel'] = False
            return
        typ, fid, data = a
        if typ == b'LTEX':
            for t, d in subrecords(data):
                if t == b'EDID': ltex[fid] = d.split(b'\0')[0].decode('latin-1')
        elif typ == b'WRLD':
            edid = ''
            for t, d in subrecords(data):
                if t == b'EDID': edid = d.split(b'\0')[0].decode('latin-1')
                if t == b'DNAM' and len(d) >= 8 and edid == 'Tamriel': state['wrld_dnam'] = struct.unpack_from('<ff', d, 0)
            if edid == 'Tamriel': state['tamriel_id'] = fid
        elif typ == b'CELL' and state['in_tamriel']:
            c = {'x': None, 'y': None, 'water': None, 'edid': '', 'full': ''}
            for t, d in subrecords(data):
                if t == b'XCLC' and len(d) >= 8: c['x'], c['y'] = struct.unpack_from('<ii', d, 0)
                elif t == b'XCLW' and len(d) >= 4: c['water'] = struct.unpack_from('<f', d, 0)[0]
                elif t == b'EDID': c['edid'] = d.split(b'\0')[0].decode('latin-1')
                elif t == b'FULL' and len(d) >= 4: c['full'] = '#%08X' % struct.unpack_from('<I', d, 0)[0] if len(d) == 4 else d.split(b'\0')[0].decode('latin-1')
            state['cell'] = c
            if c['x'] is not None: cells[(c['x'], c['y'])] = c
        elif typ == b'LAND' and state['in_tamriel'] and state['cell'] is not None and state['cell']['x'] is not None:
            c = state['cell']
            for t, d in subrecords(data):
                if t == b'VHGT' and len(d) >= 4 + 33 * 33:
                    off = struct.unpack_from('<f', d, 0)[0] * 8
                    g = struct.unpack_from('<1089b', d, 4)
                    h = [0] * 1089
                    rowoff = 0.0
                    for i in range(1089):
                        r, col = divmod(i, 33)
                        v = g[i] * 8
                        if col == 0: off += v; rowoff = 0.0
                        else: rowoff += v
                        h[i] = off + rowoff
                    c['h'] = h            # row 0 = south edge, col 0 = west edge
                elif t == b'BTXT' and len(d) >= 8:
                    tid, q = struct.unpack_from('<IB', d, 0)
                    c.setdefault('base', {})[q] = tid
                elif t == b'ATXT' and len(d) >= 8:
                    tid, q = struct.unpack_from('<IB', d, 0)
                    c['_atxt'] = (tid, q)
                elif t == b'VTXT' and '_atxt' in c:
                    # the layer's total opacity over its quarter: the strongest layer names the ground
                    tot = 0.0
                    for k in range(0, len(d) - 7, 8): tot += struct.unpack_from('<f', d, k + 4)[0]
                    tid, q = c.pop('_atxt')
                    best = c.setdefault('best', {})
                    if tot > best.get(q, (0, 0))[1]: best[q] = (tid, tot)

    records(buf, 0, len(buf), out)
    land = [k for k, c in cells.items() if 'h' in c]
    if not land:
        print('no landscape found - is this Skyrim.esm?'); return
    x0 = min(k[0] for k in land); x1 = max(k[0] for k in land)
    y0 = min(k[1] for k in land); y1 = max(k[1] for k in land)
    nx, ny = x1 - x0 + 1, y1 - y0 + 1
    W, H = nx * GRID + 1, ny * GRID + 1
    default_h = state['wrld_dnam'][0] if state['wrld_dnam'] else -2048.0
    print('cells x %d..%d, y %d..%d: %d x %d cells, %d x %d vertices' % (x0, x1, y0, y1, nx, ny, W, H))
    hm = [int(default_h / 8)] * (W * H)
    lo, hi = 1e9, -1e9
    for (cx, cy), c in cells.items():
        if 'h' not in c: continue
        bx = (cx - x0) * GRID
        for r in range(33):
            row = (y1 - cy) * GRID + (GRID - r)          # north at the top
            base = row * W + bx
            hr = c['h'][r * 33:(r + 1) * 33]
            for col in range(33):
                v = hr[col]
                if v < lo: lo = v
                if v > hi: hi = v
                hm[base + col] = int(round(v / 8))
    hm = [max(-32768, min(32767, v)) for v in hm]
    with open(os.path.join(outdir, 'tamriel_height.raw'), 'wb') as f:
        f.write(struct.pack('<%dh' % len(hm), *hm))
    # 16-bit PGM (big-endian by the format), scaled to the full range
    mn, mx = min(hm), max(hm)
    sc = 65535.0 / max(1, mx - mn)
    with open(os.path.join(outdir, 'tamriel_height.pgm'), 'wb') as f:
        f.write(b'P5 %d %d 65535\n' % (W, H))
        f.write(struct.pack('>%dH' % len(hm), *[int((v - mn) * sc) for v in hm]))
    # a small preview, 8-bit PNG written by hand (zlib + crc only)
    pw, ph = (W + 3) // 4, (H + 3) // 4
    raw = bytearray()
    for y in range(ph):
        raw.append(0)
        for x in range(pw):
            v = hm[min(H - 1, y * 4) * W + min(W - 1, x * 4)]
            raw.append(int((v - mn) * 255 / max(1, mx - mn)))
    def chunk(t, d): return struct.pack('>I', len(d)) + t + d + struct.pack('>I', zlib.crc32(t + d) & 0xffffffff)
    with open(os.path.join(outdir, 'tamriel_height.png'), 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', pw, ph, 8, 0, 0, 0, 0)) +
                chunk(b'IDAT', zlib.compress(bytes(raw), 9)) + chunk(b'IEND', b''))
    # ground textures per quarter cell (2 x 2 per cell), north at the top
    names = ['(none)']; idx = {0: 0}
    gw, gh = nx * 2, ny * 2
    gr = bytearray(gw * gh)
    for (cx, cy), c in cells.items():
        for q in range(4):
            tid = c.get('best', {}).get(q, (None, 0))[0] or c.get('base', {}).get(q, 0)
            if tid not in idx:
                idx[tid] = len(names); names.append(ltex.get(tid, '#%08X' % tid))
            qx, qy = q & 1, q >> 1            # 0 SW, 1 SE, 2 NW, 3 NE
            gx = (cx - x0) * 2 + qx
            gy = (y1 - cy) * 2 + (1 - qy)
            if 0 <= gx < gw and 0 <= gy < gh: gr[gy * gw + gx] = min(255, idx[tid])
    with open(os.path.join(outdir, 'tamriel_ground.pgm'), 'wb') as f:
        f.write(b'P5 %d %d 255\n' % (gw, gh)); f.write(bytes(gr))
    with open(os.path.join(outdir, 'tamriel_ground.txt'), 'w') as f:
        for i, n in enumerate(names): f.write('%d %s\n' % (i, n))
    with open(os.path.join(outdir, 'tamriel_water.csv'), 'w') as f:
        f.write('cell_x,cell_y,water\n')
        for (cx, cy), c in sorted(cells.items()):
            if c['water'] is not None and abs(c['water']) < 1e7: f.write('%d,%d,%.1f\n' % (cx, cy, c['water']))
    with open(os.path.join(outdir, 'tamriel_cells.csv'), 'w', encoding='utf-8') as f:
        f.write('cell_x,cell_y,editor_id,name\n')
        for (cx, cy), c in sorted(cells.items()):
            if c['edid'] or c['full']: f.write('%d,%d,%s,"%s"\n' % (cx, cy, c['edid'], c['full']))
    with open(os.path.join(outdir, 'tamriel_info.txt'), 'w') as f:
        f.write('cells x %d..%d y %d..%d (%d x %d)\n' % (x0, x1, y0, y1, nx, ny))
        f.write('vertices %d x %d, 128 game units (1.83 m) apart; raw values * 8 = game units\n' % (W, H))
        f.write('heights %.0f .. %.0f game units; world default land %.0f, water %s\n' % (lo, hi, default_h, state['wrld_dnam'][1] if state['wrld_dnam'] else '?'))
        f.write('land cells %d, ground textures %d\n' % (len(land), len(names)))
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
