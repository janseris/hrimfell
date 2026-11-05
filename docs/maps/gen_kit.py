#!/usr/bin/env python3
# Interiors from Skyrim's own pieces: every model a cell uses, simplified from Skyrim's real meshes
# (tools/skyrim_meshes.py) to what the Nokia 9300 can draw, and each cell's placements (tools/skyrim_kits.py), its
# doors (tools/skyrim_places.py), its collision taken from the building pieces themselves. Writes src/kit_ext.inc.
#   python3 docs/maps/gen_kit.py MESHES_DIR KITS_DIR PLACES_DIR [BLEAKFALLS_DIR [MORE_MESH_DIRS ...]]
# (the further export folders - Riften's - lend their models to the inventory's items)
# (BLEAKFALLS_DIR: tools/skyrim_area.py bleakfalls's folder: the barrow's outside from its refs and meshes)
#
# Engine units: 256 a metre; Skyrim units x 3.6576. Model space: x east, y up, z north (Skyrim's x, z, y; the
# triangles' order reversed, the swap being a mirror). Cell tables stay in Skyrim units (x east, y north, z up).
import csv, gzip, math, os, re, sys, collections, struct

MESHES, KITS, PLACES = sys.argv[1], sys.argv[2], sys.argv[3]
BFB = sys.argv[4] if len(sys.argv) > 4 else None
MESH_DIRS = [MESHES] + ([BFB] if BFB else []) + sys.argv[5:]
# (v0.4.74) whether a ref is there when the game starts: not deleted, not initially disabled, and its enable parent (if
# any, followed up its own) in the state that shows it - Skyrim's "enable opposite" shows a ref while its parent is off.
# Whiterun's destroyed houses, rubble, smoke and stockades are the Battle for Whiterun's (children of disabled markers)
_ENABLE = None
_ENMEMO = {}
def _enable_load():
    global _ENABLE
    _ENABLE = {}
    for d_ in sys.argv[1:]:
        f_ = os.path.join(d_, 'refs.csv.gz')
        if not os.path.exists(f_): continue
        for q in csv.DictReader(gzip.open(f_, 'rt', encoding='utf-8')):
            _ENABLE[q['ref_id']] = (int(q['flags'] or '0', 16), q.get('enable_parent') or '', (q.get('enable_opposite') or '0') == '1')
def _enabled(rid, depth=0):
    if rid in _ENMEMO: return _ENMEMO[rid]
    v = _ENABLE.get(rid)
    if v is None or depth > 16: on = True
    elif v[0] & 0x820: on = False
    elif v[1]: on = _enabled(v[1], depth + 1) != v[2]
    else: on = True
    _ENMEMO[rid] = on
    return on
def ref_on(r):
    if int(r['flags'] or '0', 16) & 0x820: return False
    if not r.get('enable_parent'): return True
    if _ENABLE is None: _enable_load()
    return _enabled(r['ref_id'])
# (v0.4.74) Skyrim's water effects (drawn by its water shader; here they would be solid slabs over the stairs and roads)
WATERFX = re.compile(r'^\w*(River\d*ACT|Waterfall\d*|RapidRocks\d*)$')
# (v0.4.55) AREA=bfb: Bleak Falls Barrow's own area file (data/bleakfalls.har: its two interiors, loaded as you go in -
# the main one, data/riverwood.har, the rest): written to src/area_bfb/
AREA = os.environ.get('AREA', 'main')
SRCDIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'src')
OUT = os.path.join(SRCDIR, 'kit_ext.inc') if AREA == 'main' else os.path.join(SRCDIR, 'area_' + AREA, 'kit_ext.inc')
if AREA != 'main': os.makedirs(os.path.dirname(OUT), exist_ok=True)
K = 3.6576
# the cells built this way: the game's cell id, Skyrim's cell, the exterior door's house in KRwDoor
CELLS = [('CELL_RWALVOR', 'RiverwoodAlvorsHouse'), ('CELL_RWGERDUR', 'RiverwoodGerdursHouse'),
         ('CELL_RWFAENDAL', 'RiverwoodFaendalsHouse'), ('CELL_RWSVEN', 'RiverwoodSvensHouse'),
         ('CELL_RWTRADER', 'RiverwoodRiverwoodTrader'), ('CELL_RWINN', 'RiverwoodSleepingGiantInn'),
         # (v0.4.48) Whiterun's outskirts: the Stables (the game's own residence gives way), the farms' houses
         ('CELL_STABLES', 'WhiterunStables'), ('CELL_PELAGIA', 'PelagiaFarm'), ('CELL_CHILLFURROW', 'ChillfurrowFarm'),
         ('CELL_BATTLEBORN', 'BattleBornFarm')]
BFB_DYN = r'^(NorDefaultPuzzlePillar01|NorLever01|NorPortcullis|TrapBladeSwinging|NorPullChain|NorPullBar|PuzzleDoor(Base|Key|Small|Medium|Large)|TrapWallWood|TrapFallingOilLamp|TrapOilPool|NorSarcophagusTopAnim|TrapStonePressurePlate|TrapDart|CaveGSecretDoor|MS13FXspiderWeb)'
if AREA == 'bfb': CELLS = [('CELL_BFTEMPLE', 'BleakFallsBarrow01'), ('CELL_BFSANCTUM', 'BleakFallsBarrow02')]
# (v0.4.68) Forelhost's own area (data/forelhost.har): its three halls (the Stronghold, the Crypt, the Refectory)
if AREA == 'fh': CELLS = [('CELL_FHSTRONG', 'Forelhost01'), ('CELL_FHCRYPT', 'Forelhost02'), ('CELL_FHREFECT', 'Forelhost03')]
DUN = AREA in ('bfb', 'fh')                     # a Nordic ruin's area: its halls as the buildings outside, their working parts the game's
# Forelhost's working parts (docs/maps/gen_dun_kit.py): the levers, the portcullises, the claw door and its rings, the
# sarcophagi's moving lids, the chests, the Glass Claw - the game's; its traps drawn where they rest
FH_DYN = r'^(NorLever01|NorPortcullis|PuzzleDoor(Base|Key|Small|Medium|Large)|NorSarcophagusTopAnim|dunForelhostGlassClaw)'
# (v0.4.57) Whiterun's own area (data/whiterun.har): the city (WhiterunWorld, outside), the outskirts and the farms'
# and the stables' houses (moved from the main area: they are in sight of the walls)
WR_CELLS = [('CELL_STABLES', 'WhiterunStables'), ('CELL_PELAGIA', 'PelagiaFarm'), ('CELL_CHILLFURROW', 'ChillfurrowFarm'), ('CELL_BATTLEBORN', 'BattleBornFarm')]
if AREA == 'wr': CELLS = WR_CELLS
elif AREA == 'main' and os.environ.get('WRAREA'): CELLS = [c_ for c_ in CELLS if c_ not in WR_CELLS]
# the building only (statics, fires, lights, floors, walls): their furniture, goods, doors and people are placed by
# game_cells.cpp (SpawnRwTrader, SpawnSgInn: the shop's stock, the claw, the room for rent, Delphine's wardrobe)
GAME_DOORS = {'RiverwoodRiverwoodTrader', 'RiverwoodSleepingGiantInn'}      # (their doors out: the game's own table)
GEOMONLY = set()       # (v0.4.40: none - the Trader and the Sleeping Giant had their furniture and goods typed in by hand
                       #  or from gen_sginn.py, and lost what those missed: the Trader's upstairs table)
NAMES = {'RiverwoodAlvorsHouse': "Alvor and Sigrid's House", 'RiverwoodGerdursHouse': "Gerdur's House",
         'RiverwoodFaendalsHouse': "Faendal's House", 'RiverwoodSvensHouse': "Sven and Hilde's House",
         'WhiterunStables': 'Whiterun Stables', 'PelagiaFarm': 'Pelagia Farm', 'ChillfurrowFarm': 'Chillfurrow Farm',
         'BattleBornFarm': 'Battle-Born Farm', 'BleakFallsBarrow01': 'Bleak Falls Temple', 'BleakFallsBarrow02': 'Bleak Falls Sanctum'}
PEOPLE = {'Hilde': 17, 'Dorthe': 21, 'Frodnar': 22, 'SkulvarSableHilt': 23, 'Jervar': 24, 'Nimriel': 26, 'Wilmuth': 27}                                    # who stays inside (the game's named index)

# ---------------------------------------------------------------- sources
models = {}                                               # editor id -> model path
mstat = {}
mbounds = {}                                              # model -> its bounds (x0 y0 z0 x1 y1 z1, Skyrim units)
for r in csv.DictReader(open(os.path.join(MESHES, 'models.csv'), encoding='utf-8')):
    for e in r['objects'].split(';'): models[e] = r['model']
    mstat[r['model']] = r['status']
    if r.get('bounds'): mbounds[r['model'].lower()] = [float(v) for v in r['bounds'].split()]
texcol = {}; texsrc = {}       # (v0.4.46: and the size Skyrim made each, its longer side)
for r in csv.DictReader(open(os.path.join(MESHES, 'textures.csv'), encoding='utf-8')):
    if r['average_rgb']: texcol[r['texture'].lower()] = r['average_rgb']
    if r.get('width') and r.get('height'): texsrc[r['texture'].lower()] = max(int(r['width']), int(r['height']))
# (v0.4.48) the Whiterun export (tools/skyrim_area.py whiterun): among the further folders, the one whose area.txt says so
WR = None
for d_ in sys.argv[5:]:
    a_ = os.path.join(d_, 'area.txt')
    if os.path.exists(a_) and open(a_, encoding='utf-8').readline().strip() == 'area whiterun': WR = d_
for EXTRA in ([BFB] if BFB else []) + sys.argv[5:]:
    # the further exports' models and textures beside Riverwood's (Riverwood's kept where both have one; v0.4.48: all of
    # them - the barrow's, Riften's, Whiterun's)
    for r in csv.DictReader(open(os.path.join(EXTRA, 'models.csv'), encoding='utf-8')):
        for e in r['objects'].split(';'): models.setdefault(e, r['model'])
        mstat.setdefault(r['model'], r['status'])
        try:
            if r.get('bounds'): mbounds.setdefault(r['model'].lower(), [float(v) for v in r['bounds'].split()])
        except ValueError: pass                  # (a model not read: its status's commas spill into the columns)
    for r in csv.DictReader(open(os.path.join(EXTRA, 'textures.csv'), encoding='utf-8')):
        if r.get('average_rgb'): texcol.setdefault(r['texture'].lower(), r['average_rgb'])
        if r.get('width') and r.get('height'): texsrc.setdefault(r['texture'].lower(), max(int(r['width']), int(r['height'])))
bases = {b['base_editor_id']: b for b in csv.DictReader(open(os.path.join(KITS, 'bases.csv'), encoding='utf-8'))}
want = set(c for _, c in CELLS)
refs = [r for r in csv.DictReader(gzip.open(os.path.join(KITS, 'interiors_all.csv.gz'), 'rt', encoding='utf-8')) if r['cell'] in want]
doors = list(csv.DictReader(open(os.path.join(PLACES, 'doors.csv'), encoding='utf-8')))

def obj_path(model):
    stem = re.sub(r'[^A-Za-z0-9_.-]', '_', model.replace('\\', '_').replace('/', '_'))
    for d in MESH_DIRS:
        fn = os.path.join(d, 'meshes', stem[:-4] + '.obj')
        if os.path.exists(fn): return fn
    return os.path.join(MESHES, 'meshes', stem[:-4] + '.obj')

class Tri(tuple):
    # a triangle (A, B, C, texture) that also knows its texture coordinates (.uv: three (u, v), 1 = the texture's
    # width; None when it has none)
    pass
def mk_tri(A, B, C, tx, uv):
    t = Tri((A, B, C, tx)); t.uv = uv; return t
def uv_of(t): return getattr(t, 'uv', None)
# (v0.4.40) the shapes' alpha (the exporter's "# alpha test N" / "# alpha blend" after usemtl): a texture is cut out
# only where a shape using it says so; drawn whole where it is used without (a sign's paint in the alpha channel)
TEX_ALPHA = collections.defaultdict(set)
# (v0.4.74) each texture's vertex colours as Skyrim multiplies them in: [r, g, b, faces] summed over the faces that use
# it (a face without them counts white); tex_tint() - their average, where it is strong enough to see
TEX_VC = collections.defaultdict(lambda: [0, 0, 0, 0])
TEX_VC_SEEN = set()
# (and "# double sided": both its sides drawn - the thatch under the eaves; an older export without the lines: the
#  thatch Skyrim draws so, by name)
TEX_DOUBLE = set()
def tex_double(tx):
    t = tx.lower()
    return t in TEX_DOUBLE or (not TEX_DOUBLE and 'thatch02' in t)
def load_obj(fn):
    V, VT, T, cur = [], [], [], ''
    vc = None; count_vc = fn not in TEX_VC_SEEN; TEX_VC_SEEN.add(fn)
    for l in open(fn, encoding='latin-1'):
        if l.startswith('v '): V.append(tuple(float(x) for x in l.split()[1:4]))
        elif l.startswith('vt '): VT.append(tuple(float(x) for x in l.split()[1:3]))
        elif l.startswith('usemtl'): cur = l.split(None, 1)[1].strip().lower(); TEX_ALPHA[cur].add('none'); vc = None
        elif l.startswith('# vertex colour average'):
            # (v0.4.50) a shape Skyrim darkens with its vertex colours (a door's black backing in WoodPost02's light
            # wood: drawn near-white over the door): its own material, flat in the texture's colour times them
            c_ = [int(x) for x in l.split()[4:7]]
            vc = c_
            if min(c_) < 120 and '#vc' not in cur:
                b_ = cur; cur = '%s#vc%02x%02x%02x' % (b_, c_[0], c_[1], c_[2]); TEX_ALPHA[cur] |= TEX_ALPHA[b_]
        elif l.startswith('# double sided'): TEX_DOUBLE.add(cur)
        elif l.startswith('# alpha ') and l[8:].strip() != 'test 0': TEX_ALPHA[cur].discard('none'); TEX_ALPHA[cur].add(l[8:].strip())   # (test 0: nothing cut away)
        elif l.startswith('f '):
            if count_vc:
                q_ = TEX_VC[cur.split('#vc')[0]]; w_ = vc or (255, 255, 255)
                q_[0] += w_[0]; q_[1] += w_[1]; q_[2] += w_[2]; q_[3] += 1
            parts = [x.split('/') for x in l.split()[1:4]]
            ix = [int(q[0]) - 1 for q in parts]
            uv = None
            if VT and all(len(q) > 1 and q[1] for q in parts):
                vi = [int(q[1]) - 1 for q in parts]
                if all(-len(VT) <= k < len(VT) for k in vi): uv = tuple(VT[k] for k in vi)
            T.append(mk_tri(V[ix[0]], V[ix[1]], V[ix[2]], cur, uv))
    return T

# ---------------------------------------------------------------- the look of a texture
TEXMAP = [('thatch', 'TXI_THATCH'), ('woodpost', 'TXI_LOGS'), ('log', 'TXI_LOGS'), ('beam', 'TXI_LOGS'),
          ('woodwall', 'TXI_PLANKS'), ('woodwalkway', 'TXI_PLANKS'), ('woodplank', 'TXI_PLANKS'), ('plank', 'TXI_PLANKS'),
          ('farmhouse01', 'TXI_PLANKS'), ('stonewall', 'TXI_ROCK'), ('stonefloor', 'TXI_SLATE'), ('roof', 'TXI_SHINGLE'),
          ('hay', 'TXI_THATCH'), ('dirt', 'TXI_DIRT')]
SKIPTEX = ('alpha', 'effects\\', 'fx', 'rope', 'cobweb', 'dust', 'mushroom', 'ivy', 'blood\\')     # (blood: the weapons' edges shown bloodied, hidden until then)
def tex_tint(t):
    # (v0.4.74) the average vertex colour Skyrim multiplies a texture by (over all its faces), when strong - some channel
    # under 200 (Whiterun's interior walls: their pale blue-grey boards ~0.7 and warmer); the mild ones (ambient
    # shading) left out, as the engine's own lighting stands for them
    q = TEX_VC.get(t.lower())
    if not q or not q[3]: return None
    c = [q[k] / q[3] for k in range(3)]
    if min(c) >= 200: return None
    # its hue only: Skyrim's dark vertex colours are mostly baked shading (a cave's rocks ~50 grey - multiplied in they
    # drew near black), which the engine's own light stands for; the brightest channel kept at full
    m_ = max(c)
    if m_ <= 0: return None
    c = [255.0 * v / m_ for v in c]
    return c if min(c) < 235 else None
def look(tex, big):
    """(r, g, b, tex byte) for a slot"""
    t = tex.lower()
    if t.startswith('gen:'): return GENLOOK[t]
    if '#vc' in t:
        b_, v_ = t.split('#vc'); r0, g0, b0, tb_ = look(b_, big)
        m_ = [int(v_[k:k + 2], 16) for k in (0, 2, 4)]
        return (max(2, r0 * m_[0] // 255), max(2, g0 * m_[1] // 255), max(2, b0 * m_[2] // 255), tb_)   # (the texture kept: only its flat colour darker)
    if not t.startswith('textures\\'): t = 'textures\\' + t
    c = texcol.get(t, '605848')
    r, g, b = int(c[0:2], 16), int(c[2:4], 16), int(c[4:6], 16)
    if tex_file(t) and sky_ok(t) and alpha_info(t): r, g, b = alpha_info(t)[1]      # (the cut-away part not counted)
    tt_ = tex_tint(t)
    if tt_: r, g, b = int(r * tt_[0] / 255), int(g * tt_[1] / 255), int(b * tt_[2] / 255)
    if tt_: c = '%02x%02x%02x' % (r, g, b)
    r, g, b = [min(250, int(v * 1.55 + 8)) for v in (r, g, b)]          # the engine's lighting is darker than Skyrim's
    if 'window' in t: return (r, g, b, '1')                              # the measured colour (dark frames)
    if 'candle' in t or 'flame' in t: return (240, 224, 180, '1')
    if t.startswith('textures\\dungeons\\') or t.startswith('textures\\landscape\\rocks') or t.startswith('textures\\landscape\\mountains'):
        # the Nordic ruins' stone and the mountains' rock: Skyrim shows them lighter than their textures' average
        r, g, b = [min(250, int(int(c[k:k + 2], 16) * 2.0 + 16)) for k in (0, 2, 4)]
        return (r, g, b, 'SL_TEX | TXI_ROCK') if big else (r, g, b, '1')
    if big:
        for k, x in TEXMAP:
            if k in t: return (r, g, b, 'SL_TEX | ' + x)
    if True:
        # the furniture and the hides: the engine's own stained wood and fur (a textured face takes its colours from
        # the texture, as the dresser and the wardrobe do; the averages of Skyrim's textures come out a flat grey-brown)
        for k, x in SMALLMAP:
            if k in t: return (r, g, b, 'SL_TEX | ' + x)
    return (r, g, b, '1')
# models drawn the engine's way instead of from Skyrim's mesh: the static barrel (Barrel02Static, the one that cannot
# be searched) as the working barrel (staves bulging to the middle, a lid) but in darker wood, bound with rope
GENLOOK = {'gen:dstave': (60, 44, 32, 'SL_TEX | TXI_DSTAVE'), 'gen:cord': (104, 82, 56, 'SL_TEX | TXI_ROPE'),
           'gen:dlid': (60, 44, 32, 'SL_TEX | TXI_DSTAVE'), 'gen:snow': (232, 236, 242, 'SL_TEX | TXI_SNOW')}
def gen_static_barrel():
    """Skyrim's size (80 high, 27 at the middle; things stand on it at 80), 8 staves round, in Skyrim units"""
    N = 8
    prof = [(0, 21.5), (20, 25.5), (60, 25.5), (80, 21.5)]
    def ring(z, r): return [(r * math.cos(2 * math.pi * (k + 0.5) / N), r * math.sin(2 * math.pi * (k + 0.5) / N), z) for k in range(N)]
    out = []
    def quad(A, B, C, D, tx):
        # outward: away from the barrel's axis
        for T in ((A, B, C), (A, C, D)):
            n = _cross(_sub(T[1], T[0]), _sub(T[2], T[0])); m = sum(p[0] for p in T) / 3, sum(p[1] for p in T) / 3
            if n[0] * m[0] + n[1] * m[1] < 0: T = (T[0], T[2], T[1])
            out.append(T + (tx,))
    for (z0, r0), (z1, r1) in zip(prof, prof[1:]):
        a, b = ring(z0, r0), ring(z1, r1)
        for k in range(N): quad(a[k], a[(k + 1) % N], b[(k + 1) % N], b[k], 'gen:dstave')
    def rad(z):
        for (z0, r0), (z1, r1) in zip(prof, prof[1:]):
            if z0 <= z <= z1: return r0 + (r1 - r0) * (z - z0) / (z1 - z0)
    for z0, z1 in ((9, 11.5), (64, 66.5), (70.5, 73)):                 # the rope: one round the foot, two below the rim
        a, b = ring(z0, rad(z0) + 0.6), ring(z1, rad(z1) + 0.6)
        for k in range(N): quad(a[k], a[(k + 1) % N], b[(k + 1) % N], b[k], 'gen:cord')
    top = ring(79, 20.5)                                                # the lid, a little sunk in the rim
    for k in range(1, N - 1):
        T = (top[0], top[k], top[k + 1])
        if _cross(_sub(T[1], T[0]), _sub(T[2], T[0]))[2] < 0: T = (T[0], T[2], T[1])
        out.append(T + ('gen:dlid',))
    return out
GENMODEL = {'clutter\\barrel02.nif': lambda model, far: gen_static_barrel()}
# Skyrim's forest pines are a trunk and cards of branches cut out by their texture's alpha, which the engine cannot
# draw (as solid triangles they are a tangle): each is drawn the engine's way instead, at its size - a trunk and tiers
# of drooping branches, wide at the bottom of the crown, narrowing to the top, each tier closed underneath; the far
# model four tiers. Dead ones: the same, thin and grey. (Snowy ones: snow on their tops, as every ...Snow piece.)
GENLOOK.update({'gen:needle': (44, 70, 46, 'SL_TEX | TXI_NEEDLE'), 'gen:needle2': (54, 82, 52, 'SL_TEX | TXI_NEEDLE'),
                'gen:bark': (84, 62, 44, 'SL_TEX | TXI_LOGS'), 'gen:deadwood': (104, 94, 82, 'SL_TEX | TXI_LOGS')})
def gen_pine(model, far):
    b = mbounds.get(model.lower(), [-300, -300, -80, 300, 300, 2000])
    H = b[5]; zlow = min(b[2], 0) - 10
    R = 0.8 * (b[3] - b[0] + b[4] - b[1]) / 4
    dead = 'dead' in model.lower()
    if dead: R *= 0.5
    out = []
    def tri(A, B, C, tx, want):
        # facing `want` (a direction): its triangle turned so
        n = _cross(_sub(B, A), _sub(C, A))
        if _dot(n, want) < 0: B, C = C, B
        out.append((A, B, C, tx))
    def ring(z, r, k, rot): return [(r * math.cos(2 * math.pi * (i + rot) / k), r * math.sin(2 * math.pi * (i + rot) / k), z) for i in range(k)]
    # the trunk: a tapering prism into the ground
    k = 3 if far == 2 else (4 if far else 6)
    rb = max(10.0, H * 0.012); rt = rb * 0.35
    lo, hi = ring(zlow, rb, k, 0), ring(H * 0.86, rt, k, 0)
    for i in range(k):
        j = (i + 1) % k
        mid = ((lo[i][0] + lo[j][0]) / 2, (lo[i][1] + lo[j][1]) / 2, 0)
        tri(lo[i], lo[j], hi[j], 'gen:bark', mid); tri(lo[i], hi[j], hi[i], 'gen:bark', mid)
    # the crown
    N = 3 if far == 2 else (4 if far else 8)
    sides = 4 if far == 2 else (5 if far else 7)
    z0 = H * (0.30 if dead else 0.24)
    span = H - z0
    for t in range(N):
        t0, t1 = t / N, (t + 1) / N
        r = R * (1 - 0.82 * t0) * (1.0 if t < N - 1 else 0.7)
        za = z0 + span * t1 + (span / N * 0.15 if t == N - 1 else 0)          # the tier's top (at the trunk)
        zr = z0 + span * t0 - span / N * 0.35                                  # its rim, drooping below the tier under it
        rim = ring(zr, r, sides, 0.5 * (t & 1))
        apex = (0.0, 0.0, za)
        tx = 'gen:deadwood' if dead else ('gen:needle' if t & 1 else 'gen:needle2')
        for i in range(sides):
            j = (i + 1) % sides
            mid = ((rim[i][0] + rim[j][0]) / 2, (rim[i][1] + rim[j][1]) / 2, abs(r) * 0.6)
            tri(apex, rim[i], rim[j], tx, mid)
            if not far:
                # underneath: up to the trunk a little above the rim
                under = (0.0, 0.0, zr + (za - zr) * 0.3)
                tri(under, rim[i], rim[j], tx, (mid[0], mid[1], -abs(r)))
    return out
# Skyrim's woven fences are a hundred twigs each (a thousand triangles for Riverwood's fences in one view): drawn as a
# panel of wattle between two stakes instead, at their size (14 triangles)
GENLOOK.update({'gen:wattle': (104, 80, 54, 'SL_TEX | TXI_THATCH'), 'gen:stake': (82, 62, 44, 'SL_TEX | TXI_LOGS')})
def gen_fence(model, far):
    b = mbounds.get(model.lower(), [-80, -6, 0, 80, 6, 104])
    x0, x1, H = b[0], b[3], b[5]
    t = 3.0
    out = []
    def quad(A, B, C, D, tx, want):
        for T in ((A, B, C), (A, C, D)):
            n = _cross(_sub(T[1], T[0]), _sub(T[2], T[0]))
            if _dot(n, want) < 0: T = (T[0], T[2], T[1])
            out.append(T + (tx,))
    z0, z1 = H * 0.12, H * 0.9
    for y, w in ((-t, (0, -1, 0)), (t, (0, 1, 0))):                      # the wattle, both faces
        quad((x0, y, z0), (x1, y, z0), (x1, y, z1), (x0, y, z1), 'gen:wattle', w)
    if far: return out                                                   # (far: only the panel, both faces)
    quad((x0, -t, z1), (x1, -t, z1), (x1, t, z1), (x0, t, z1), 'gen:wattle', (0, 0, 1))
    for x in (x0 + 4, x1 - 4):                                         # the stakes: three sides each
        r = 4.0
        for (ax, ay), (bx, by), w in (((-r, -r), (r, -r), (0, -1, 0)), ((r, -r), (r, r), (1, 0, 0)), ((r, r), (-r, r), (0, 1, 0))):
            quad((x + ax, ay, 0), (x + bx, by, 0), (x + bx, by, H), (x + ax, ay, H), 'gen:stake', w)
    return out
for _fm in ('fencewoven01', 'fencewoven02'):
    GENMODEL['architecture\\farmhouse\\' + _fm + '.nif'] = gen_fence
for _pm in ('treepineforest01', 'treepineforest02', 'treepineforest03', 'treepineforest04', 'treepineforest05',
            'treepineforestsnow02', 'treepineforestsnow03', 'treepineforestsnow04', 'treepineforestsnow05',
            'treepineforestsnowl01', 'treepineforestsnowl02', 'treepineforestsnowl03', 'treepineforestsnowl04', 'treepineforestsnowl05',
            'treepineforestdead02', 'treepineforestdead03', 'treepineforestdead04', 'treepineforestdead05',
            'treepineforestdeadsnowl03', 'treepineforestdeadsnowl04'):
    GENMODEL['landscape\\trees\\' + _pm + '.nif'] = gen_pine
SMALLMAP = [('pelt', 'TXI_FUR'), ('dresser01', 'TXI_FURN'), ('barset\\bar01', 'TXI_FURN'), ('noblefurn', 'TXI_FURN'),
            ('blacksmithworkbench', 'TXI_FURN'), ('woodmount', 'TXI_FURN'), ('mountedheadbase', 'TXI_FURN'),
            ('handcart', 'TXI_FURN'), ('chest01', 'TXI_FURN'), ('woodpost', 'TXI_LOGS'), ('firewood', 'TXI_LOGS'),
            ('choppingblock01', 'TXI_LOGS'), ('basket', 'TXI_THATCH'), ('bucket01', 'TXI_FURN'), ('woodenladle', 'TXI_FURN'), ('woodwall', 'TXI_PLANKS'), ('woodwalkway', 'TXI_PLANKS'), ('hay01', 'TXI_THATCH')]

# ---------------------------------------------------------------- simplifying a model
import heapq
def _sub(a, b): return (a[0] - b[0], a[1] - b[1], a[2] - b[2])
def _cross(a, b): return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
def _dot(a, b): return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]
def _plane_q(n, d, w):
    a, b, c = n
    return [w * v for v in (a * a, a * b, a * c, a * d, b * b, b * c, b * d, c * c, c * d, d * d)]
def _qadd(q, r): return [x + y for x, y in zip(q, r)]
def _qerr(q, p):
    x, y, z = p
    return (q[0] * x * x + 2 * q[1] * x * y + 2 * q[2] * x * z + 2 * q[3] * x + q[4] * y * y + 2 * q[5] * y * z + 2 * q[6] * y
            + q[7] * z * z + 2 * q[8] * z + q[9])

def simplify(tris, target, maxv=480):
    """quadric edge collapse (Garland-Heckbert): merge the edge whose merging changes the shape least, again and again,
    keeping open edges (doorways, the ends of walls) in place and never turning a face over"""
    # weld the corners (per slot: a texture seam stays a seam)
    vid = {}; P = []; F = []
    for A, B, C, slot in tris:
        ids = []
        for X in (A, B, C):
            k = (round(X[0] * 2), round(X[1] * 2), round(X[2] * 2))
            if k not in vid: vid[k] = len(P); P.append(list(X))
            ids.append(vid[k])
        if len(set(ids)) == 3: F.append([ids[0], ids[1], ids[2], slot])
    nv = len(P)
    Q = [[0.0] * 10 for _ in range(nv)]
    vf = [set() for _ in range(nv)]
    alive = [True] * len(F)
    for fi, (a, b, c, s) in enumerate(F):
        n = _cross(_sub(P[b], P[a]), _sub(P[c], P[a])); L = math.sqrt(_dot(n, n))
        if L < 1e-9: alive[fi] = False; continue
        u = (n[0] / L, n[1] / L, n[2] / L); d = -_dot(u, P[a])
        # (what faces up - floors, table tops - kept in its plane far more firmly: things lie on it, the pelts and
        #  rugs just over it, and a floor raised by the simplifying hides them)
        q = _plane_q(u, d, L * (60.0 if u[2] > 0.9 else 1.0))
        for v in (a, b, c): Q[v] = _qadd(Q[v], q); vf[v].add(fi)
    # open edges: a plane through the edge, upright to the face, heavily weighted
    ecount = collections.Counter()
    for fi, f in enumerate(F):
        if not alive[fi]: continue
        for i in range(3):
            e = tuple(sorted((f[i], f[(i + 1) % 3]))); ecount[e] += 1
    for fi, f in enumerate(F):
        if not alive[fi]: continue
        n = _cross(_sub(P[f[1]], P[f[0]]), _sub(P[f[2]], P[f[0]])); L = math.sqrt(_dot(n, n)) or 1
        for i in range(3):
            a, b = f[i], f[(i + 1) % 3]
            if ecount[tuple(sorted((a, b)))] != 1: continue
            e = _sub(P[b], P[a]); m = _cross(e, n); Lm = math.sqrt(_dot(m, m))
            if Lm < 1e-9: continue
            u = (m[0] / Lm, m[1] / Lm, m[2] / Lm); d = -_dot(u, P[a])
            q = _plane_q(u, d, 400.0 * math.sqrt(_dot(e, e)))
            Q[a] = _qadd(Q[a], q); Q[b] = _qadd(Q[b], q)
    ver = [0] * nv
    def cost(a, b):
        q = _qadd(Q[a], Q[b])
        best = None
        for c in (P[a], P[b], [(P[a][i] + P[b][i]) / 2 for i in range(3)]):
            e = _qerr(q, c)
            if best is None or e < best[0]: best = (e, c)
        return best
    heap = []
    edges = set()
    for fi, f in enumerate(F):
        if not alive[fi]: continue
        for i in range(3): edges.add(tuple(sorted((f[i], f[(i + 1) % 3]))))
    for a, b in edges:
        c, pos = cost(a, b); heapq.heappush(heap, (c, a, b, ver[a], ver[b], pos))
    nf = sum(alive)
    gone = [False] * nv
    while nf > target and heap:
        c, a, b, va, vb, pos = heapq.heappop(heap)
        if gone[a] or gone[b] or ver[a] != va or ver[b] != vb: continue
        # would a face turn over?
        flip = False
        for v, other in ((a, b), (b, a)):
            for fi in vf[v]:
                if not alive[fi]: continue
                f = F[fi]
                if other in f[:3]: continue
                pts = [pos if x == v else P[x] for x in f[:3]]
                n0 = _cross(_sub(P[f[1]], P[f[0]]), _sub(P[f[2]], P[f[0]]))
                n1 = _cross(_sub(pts[1], pts[0]), _sub(pts[2], pts[0]))
                if _dot(n0, n1) <= 0.2 * math.sqrt(_dot(n0, n0) * _dot(n1, n1)): flip = True; break
            if flip: break
        if flip: continue
        # merge b into a
        P[a] = list(pos); Q[a] = _qadd(Q[a], Q[b]); gone[b] = True; ver[a] += 1
        for fi in list(vf[b]):
            if not alive[fi]: continue
            f = F[fi]
            if a in f[:3]:
                alive[fi] = False; nf -= 1
                for x in f[:3]: vf[x].discard(fi)
                continue
            f[f.index(b)] = a; vf[a].add(fi)
        vf[b] = set()
        nb = set()
        for fi in vf[a]:
            if alive[fi]: nb.update(F[fi][:3])
        nb.discard(a)
        for x in nb:
            cc, pp = cost(a, x); heapq.heappush(heap, (cc, min(a, x), max(a, x), ver[min(a, x)], ver[max(a, x)], pp))
    faces = [F[fi] for fi in range(len(F)) if alive[fi]]
    used = sorted(set(i for f in faces for i in f[:3]))
    remap = {o: n for n, o in enumerate(used)}
    pts = [tuple(P[i]) for i in used]
    out = [(remap[f[0]], remap[f[1]], remap[f[2]], f[3]) for f in faces]
    if len(pts) > maxv: return simplify([(pts[a], pts[b], pts[c], s) for a, b, c, s in out], int(len(out) * 0.8), maxv)
    return pts, out

# ---------------------------------------------------------------- the log steps of a staircase
# Skyrim's stairs (FarmInt2End01 and the like) are separate small logs lying level, about 110 x 11 units and 10 high,
# each 21 units above and 25 on from the last, between two sloping stringers. Simplified with the rest of a 4800-face
# piece they vanish (two long logs are left), and as walls their sides stop a body at the foot. So: found here, drawn
# as a box each, walked as a ramp (their tops flat, chained by stair_ramps), and left out of the walls.
step_cache = {}
# where a candle holder's flames burn (its own frame, Skyrim units): the top of each candle (the small pieces of wax,
# about 10 x 10 units, standing in the horns), or where its candles are part of the horns themselves (the
# chandelier), the top of each horn; none found: the top of the whole
tip_cache = {}
def candle_tips(model):
    if model in tip_cache: return tip_cache[model]
    if not os.path.exists(obj_path(model)): tip_cache[model] = []; return []
    raw = []; V = []; grp = ''
    for l in open(obj_path(model), encoding='latin-1'):
        if l.startswith('v '): V.append(tuple(float(x) for x in l.split()[1:4]))
        elif l.startswith('o '): grp = l[2:].strip().lower()
        elif l.startswith('f '):
            ix = [int(x.split('/')[0]) - 1 for x in l.split()[1:4]]
            if 'rope' not in grp and 'chain' not in grp: raw.append((V[ix[0]], V[ix[1]], V[ix[2]]))
    key = {}; par = list(range(len(raw)))
    def fnd(i):
        while par[i] != i: par[i] = par[par[i]]; i = par[i]
        return i
    for ti, t in enumerate(raw):
        for X in t:
            k = (round(X[0], 1), round(X[1], 1), round(X[2], 1))
            if k in key: par[fnd(ti)] = fnd(key[k])
            else: key[k] = ti
    comps = collections.defaultdict(list)
    for ti in range(len(raw)): comps[fnd(ti)].append(ti)
    boxes = []
    for m in comps.values():
        P = [X for ti in m for X in raw[ti]]
        boxes.append(([min(p[k] for p in P) for k in range(3)] + [max(p[k] for p in P) for k in range(3)], len(m)))
    wax = [b for b, n in boxes if n >= 20 and b[3] - b[0] <= 13 and b[4] - b[1] <= 13 and b[5] - b[2] <= 8]
    horns = [b for b, n in boxes if n >= 100 and max(b[3] - b[0], b[4] - b[1]) <= 25]
    use = wax or horns
    tips = [((b[0] + b[3]) / 2, (b[1] + b[4]) / 2, b[5] + 1) for b in use]
    tip_cache[model] = tips
    return tips

def step_logs(model, raw):
    """(set of the triangle indices that are log steps, [(x0, y0, z0, x1, y1, z1, texture)] a box each)"""
    if model in step_cache: return step_cache[model]
    if not model.lower().startswith(('architecture', 'dungeons\\nordic\\exterior\\towers')): step_cache[model] = (set(), []); return step_cache[model]
    key = {}; par = list(range(len(raw)))
    def fnd(i):
        while par[i] != i: par[i] = par[par[i]]; i = par[i]
        return i
    for ti, t in enumerate(raw):
        for X in t[:3]:
            k = (round(X[0], 1), round(X[1], 1), round(X[2], 1))
            if k in key: par[fnd(ti)] = fnd(key[k])
            else: key[k] = ti
    comps = collections.defaultdict(list)
    for ti in range(len(raw)): comps[fnd(ti)].append(ti)
    cand = []
    wmax = 30 if model.lower().startswith('dungeons') else 18       # (the tower's boards are wider than the logs)
    for m in comps.values():
        P = [X for ti in m for X in raw[ti][:3]]
        bb = [min(p[k] for p in P) for k in range(3)] + [max(p[k] for p in P) for k in range(3)]
        dx, dy, dz = bb[3] - bb[0], bb[4] - bb[1], bb[5] - bb[2]
        tex = collections.Counter(raw[ti][3] for ti in m).most_common(1)[0][0]
        if len(m) >= 8 and dz <= 16 and min(dx, dy) <= wmax and 50 <= max(dx, dy) <= 200 and 'thatch' not in tex:
            cand.append((bb, m, tex))
    # a step has a neighbour 10..35 above or below it and 10..45 along
    def near(a, b):
        dz = abs(a[5] - b[5]); dh = math.hypot((a[0] + a[3] - b[0] - b[3]) / 2, (a[1] + a[4] - b[1] - b[4]) / 2)
        return 10 <= dz <= 35 and 10 <= dh <= 45
    steps = [s for s in cand if sum(1 for o in cand if o is not s and near(s[0], o[0])) >= 1]
    if len(steps) < 3: steps = []
    r = (set(ti for s in steps for ti in s[1]), [tuple(s[0]) + (s[2],) for s in steps])
    step_cache[model] = r
    return r

# (v0.4.40) the flights of steps built into a piece (the Bleak Falls tower's stone stairs, in its floor): their treads
# (narrow level patches, each a step - 10..35 units - above the last and 10..45 along) found, and every face within
# the flight's box kept as made, not simplified (simplified, the steps became a slope)
_skeep = {}
def stair_keep(model, raw):
    k_ = (model, len(raw))
    if k_ not in _skeep: _skeep[k_] = _stair_keep(model, raw)
    return _skeep[k_]
def _stair_keep(model, raw):
    m = model.lower()
    if not m.startswith(('architecture', 'dungeons')): return set(), []
    flat = []
    for i, t in enumerate(raw):
        n = _cross(_sub(t[1], t[0]), _sub(t[2], t[0])); L = math.sqrt(_dot(n, n)) or 1
        if n[2] / L > 0.95: flat.append(i)
    if len(flat) < 6: return set(), []
    # level patches: flat faces at one height that touch
    by = collections.defaultdict(list)
    for i in flat: by[round(sum(p[2] for p in raw[i][:3]) / 3)].append(i)
    patches = []
    for z, idx in by.items():
        par = {i: i for i in idx}
        def fnd(i):
            while par[i] != i: par[i] = par[par[i]]; i = par[i]
            return i
        key = {}
        for i in idx:
            for X in raw[i][:3]:
                k = (round(X[0]), round(X[1]))
                if k in key: par[fnd(i)] = fnd(key[k])
                else: key[k] = i
        comp = collections.defaultdict(list)
        for i in idx: comp[fnd(i)].append(i)
        for mem in comp.values():
            P = [X for i in mem for X in raw[i][:3]]
            x0, y0, x1, y1 = min(q[0] for q in P), min(q[1] for q in P), max(q[0] for q in P), max(q[1] for q in P)
            if min(x1 - x0, y1 - y0) < 50 and 40 < max(x1 - x0, y1 - y0) < 400:
                patches.append((z, x0, y0, x1, y1))
    patches.sort()
    used = set(); boxes = []
    for k, a in enumerate(patches):
        if k in used: continue
        chain = [k]
        while True:
            c = patches[chain[-1]]; nxt = None
            for j in range(len(patches)):
                if j in used or j in chain: continue
                b = patches[j]; dz = b[0] - c[0]
                dh = math.hypot((b[1] + b[3] - c[1] - c[3]) / 2, (b[2] + b[4] - c[2] - c[4]) / 2)
                if 10 <= dz <= 35 and 10 <= dh <= 45 and (nxt is None or dh < nxt[1]): nxt = (j, dh)
            if nxt is None: break
            chain.append(nxt[0])
        if len(chain) < 4: continue
        used.update(chain)
        P = [patches[j] for j in chain]
        boxes.append((min(q[1] for q in P) - 2, min(q[2] for q in P) - 2, P[0][0] - 40, max(q[3] for q in P) + 2, max(q[4] for q in P) + 2, P[-1][0] + 1, P))
    if not boxes or len(boxes) > 6: return set(), []          # (more: not stairs - a pattern of ledges)
    # each step a box: its tread, down to the tread below (the first: a rise below it), in the tread's texture
    steps = []
    for b in boxes:
        P = b[6]
        for j, q in enumerate(P):
            zlo = P[j - 1][0] if j else q[0] - (P[1][0] - P[0][0])
            txc = collections.Counter(raw[i][3] for z_, idx in by.items() if z_ == q[0] for i in idx
                                      if q[1] - 1 <= min(p[0] for p in raw[i][:3]) and max(p[0] for p in raw[i][:3]) <= q[3] + 1
                                      and q[2] - 1 <= min(p[1] for p in raw[i][:3]) and max(p[1] for p in raw[i][:3]) <= q[4] + 1)
            steps.append((q[1], q[2], zlo, q[3], q[4], q[0], txc.most_common(1)[0][0] if txc else raw[0][3]))
    keep = set()
    for i, t in enumerate(raw):
        cx = sum(p[0] for p in t[:3]) / 3; cy = sum(p[1] for p in t[:3]) / 3; cz = sum(p[2] for p in t[:3]) / 3
        for b in boxes:
            if b[0] <= cx <= b[3] and b[1] <= cy <= b[4] and b[2] <= cz <= b[5]: keep.add(i); break
    if keep: print('   %s: %d flights, %d steps drawn as boxes (for %d triangles)' % (model, len(boxes), len(steps), len(keep)))
    return keep, steps

def box_tris(b, slot):
    """a box's top and four sides (no bottom), each face turned outward (cross(B - A, C - A) points out)"""
    x0, y0, z0, x1, y1, z1 = b[:6]
    c = ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2)
    quads = [((x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)),
             ((x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1)), ((x1, y1, z0), (x0, y1, z0), (x0, y1, z1), (x1, y1, z1)),
             ((x0, y1, z0), (x0, y0, z0), (x0, y0, z1), (x0, y1, z1)), ((x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1))]
    out = []
    for A, B, C, D in quads:
        for T in ((A, B, C), (A, C, D)):
            n = _cross(_sub(T[1], T[0]), _sub(T[2], T[0]))
            m = [sum(p[k] for p in T) / 3 - c[k] for k in range(3)]
            if _dot(n, m) < 0: T = (T[0], T[2], T[1])
            out.append(T + (slot,))
    return out

# ---------------------------------------------------------------- what each placed thing becomes
ITEM_EDID = {'IT_IRON_SWORD': 'IronSword', 'IT_STEEL_SWORD': 'SteelSword', 'IT_IRON_AXE': 'IronWarAxe',
    'IT_IRON_DAGGER': 'IronDagger', 'IT_BARROW_BLADE': 'DraugrSword', 'IT_FUR_ARMOR': 'ArmorBanditCuirass',
    'IT_IRON_ARMOR': 'ArmorIronCuirass', 'IT_FUR_HELMET': 'ArmorBanditHelmet', 'IT_IRON_HELMET': 'ArmorIronHelmet',
    'IT_FUR_BOOTS': 'ArmorStormcloakBoots', 'IT_IRON_BOOTS': 'ArmorIronBoots', 'IT_FUR_GAUNTLETS': 'ArmorStormcloakGauntlets',
    'IT_IRON_GAUNTLETS': 'ArmorIronGauntlets', 'IT_HIDE_SHIELD': 'ArmorHideShield', 'IT_IRON_SHIELD': 'ArmorIronShield',
    'IT_POT_MHEAL': 'RestoreHealth01', 'IT_POT_HEAL': 'RestoreHealth02', 'IT_POT_MMAG': 'RestoreMagicka01',
    'IT_POT_MSTAM': 'RestoreStamina01', 'IT_BREAD': 'FoodBread01A', 'IT_APPLE': 'FoodApple', 'IT_LICHEN': 'FrostMirriam',
    'IT_WOLF_PELT': 'WolfPelt', 'IT_SILVER_RING': 'JewelryRingSilver', 'IT_CABBAGE': 'FoodCabbage', 'IT_LEEK': 'FoodLeek',
    'IT_POTATO': 'FoodPotato', 'IT_CARROT': 'FoodCarrot', 'IT_GARLIC': 'Garlic', 'IT_SALT': 'SaltPile',
    'IT_RAW_VENISON': 'FoodVenison', 'IT_SALMON': 'FoodSalmon', 'IT_RAW_BEEF': 'FoodBeef', 'IT_SALMON_STEAK': 'FoodSalmonCooked',
    'IT_COOKED_BEEF': 'FoodBeefCooked', 'IT_VEG_SOUP': 'FoodVegetableSoup', 'IT_CABBAGE_SOUP': 'FoodCabbagePotatoSoup',
    'IT_APPLE_STEW': 'FoodAppleCabbageStew', 'IT_VENISON_STEW': 'FoodVenisonStew', 'IT_GOLD_RING': 'JewelryRingGold',
    'IT_SILVER_NECKLACE': 'JewelryNecklaceSilver', 'IT_GOLD_NECKLACE': 'JewelryNecklaceGold', 'IT_GARNET': 'GemGarnet',
    'IT_AMETHYST': 'GemAmethyst', 'IT_TANKARD': 'BasicTankard01', 'IT_WOOD_BOWL': 'BasicWoodenBowl01', 'IT_TORCH': 'Torch01',
    'IT_BONE_MEAL': 'BoneMeal', 'IT_BLUE_FLOWER': 'MountainFlower01Blue', 'IT_POT_CURE': 'CureDisease',
    'IT_POISON_WEAK': 'DamageHealth01', 'IT_BOOK_HERBS': 'SkillAlchemy5', 'IT_JUNIPER': 'JuniperBerries',
    'IT_EMBER_SALTS': 'FireSalts', 'IT_SHADOW_SALTS': 'VoidSalts', 'IT_WISP_DUST': 'glowDust', 'IT_CRONE_FEATHERS': 'HagravenFeathers',
    'IT_TROLL_FAT': 'TrollFat', 'IT_RAT_TAIL': 'SkeeverTail', 'IT_SPIDER_EGG': 'SpiderEgg', 'IT_EMBER_FLY': 'DragonflyOrange',
    'IT_BLUE_FLY': 'DragonflyBlue', 'IT_TAPROOT': 'Taproot', 'IT_PIKE_SCALES': 'SlaughterfishScales',
    'IT_WRAITH_TEETH': 'IceWraithTeeth', 'IT_GHOST_ESSENCE': 'Ectoplasm', 'IT_TREANT_SAP': 'SprigganSap',
    'IT_POT_RESFIRE': 'ResistFire25', 'IT_FROST_VENOM': 'FrostbiteVenom', 'IT_GOLDEN_CLAW': 'MS13GoldenClaw',
    'IT_ARVEL_JOURNAL': 'MS13ArvelsJournal', 'IT_DRAGONSTONE': 'MQ103Dragonstone', 'IT_BOOK_DRAGONBORN': 'Book3ValuableDragonborn',
    'IT_BOOK_NIGHTTEARS': 'MG03Book01', 'IT_RAW_RABBIT': 'FoodRabbit', 'IT_PHEASANT': 'FoodPheasant', 'IT_IRON_MACE': 'IronMace',
    'IT_IRON_GREATSWORD': 'IronGreatsword', 'IT_IRON_BATTLEAXE': 'IronBattleaxe', 'IT_IRON_WARHAMMER': 'IronWarhammer',
    'IT_HUNTING_BOW': 'HuntingBow', 'IT_LONG_BOW': 'LongBow', 'IT_IRON_ARROW': 'IronArrow', 'IT_LOCKPICK': 'Lockpick',
    'IT_IRON_INGOT': 'IngotIron', 'IT_STEEL_DAGGER': 'SteelDagger', 'IT_PURPLE_FLOWER': 'MountainFlower01Purple',
    'IT_RED_FLOWER': 'MountainFlower01Red', 'IT_THISTLE': 'Thistle01', 'IT_SNOWBERRIES': 'Snowberry', 'IT_DEER_HIDE': 'DeerHide',
    'IT_SMALL_ANTLERS': 'AntlersSmall', 'IT_LARGE_ANTLERS': 'AntlersLarge', 'IT_FOX_PELT': 'FoxPelt', 'IT_GOAT_HIDE': 'GoatHide',
    'IT_GOAT_LEG': 'FoodGoatMeat', 'IT_MUDCRAB_CHITIN': 'MudcrabChitin', 'IT_NORD_FROST': 'DraugrWarAxe'}
EDID_ITEM = {v: k for k, v in ITEM_EDID.items() if k != 'IT_NORD_FROST'}     # (its model is the plain axe's)
ITEM_TYPES = ('MISC', 'ALCH', 'INGR', 'WEAP', 'AMMO', 'ARMO', 'BOOK', 'KEYM', 'SLGM', 'SCRL', 'APPA')
SKIPBASE = re.compile(r'(Marker|^FX|^Default|Trigger|BlackPlane|^Idle|^Sweep|^COC|^Lean|^AmbSound|^Collision|Navmesh)', re.I)
ITEMS = [('BasicTankard', 'IT_TANKARD'), ('FoodBread', 'IT_BREAD'), ('BasicWoodenPlate', 'IT_PLATE'),
         ('FoodSalmonCooked', 'IT_SALMON_STEAK'), ('FoodSalmon', 'IT_SALMON'), ('FoodApple', 'IT_APPLE'),
         ('FoodCheese', 'IT_CHEESE'), ('BasicWoodenBowl', 'IT_WOOD_BOWL'), ('FoodPotato', 'IT_POTATO'),
         ('FoodCabbage', 'IT_CABBAGE'), ('FoodLeek', 'IT_LEEK'), ('FoodCarrot', 'IT_CARROT'), ('Garlic', 'IT_GARLIC'),
         ('IronDagger', 'IT_IRON_DAGGER'), ('IronSword', 'IT_IRON_SWORD'), ('SteelSword', 'IT_STEEL_SWORD'),
         ('RestoreHealth01', 'IT_POT_MHEAL'), ('RestoreMagicka01', 'IT_POT_MMAG'), ('RestoreStamina01', 'IT_POT_MSTAM'),
         ('SaltPile', 'IT_SALT'), ('FoodVenison', 'IT_RAW_VENISON'), ('FoodBeef', 'IT_RAW_BEEF'), ('Wheat', 'IT_WHEAT'),
         ('Lavender', 'IT_LAVENDER'), ('BlisterWort', 'IT_BLISTERWORT'), ('TrollFat', 'IT_TROLL_FAT')]
def prop_of(e, t):
    if t == 'CONT':
        if e.startswith('Barrel'): return ('PT_BARREL', 0)
        if 'Chest' in e: return ('PT_CHEST', 0)
        if e.startswith(('Dresser', 'EndTable')): return ('PT_DRESSER', 0)
        if e.startswith(('Cupboard', 'CommonWardrobe', 'Wardrobe')): return ('PT_CUPBOARD', 0)
        if 'Sack' in e: return ('PT_SACK', 0)
    if t == 'FURN':
        if 'BedDouble' in e or 'DoubleBed' in e: return ('PT_BED', 0)
        if 'Bed' in e and 'Bedroll' not in e: return ('PT_BED', 1)
        if 'Chair' in e: return ('PT_CHAIR', 0)
        if e.startswith(('FarmBench', 'CommonBench', 'FarmTableBench')): return ('PT_BENCH', 0)
        if e.startswith('CraftingCookingPot'): return ('PT_POT', 0)
        if e.startswith('CraftingAlchemy'): return ('PT_ALCHEMY', 0)
    if t == 'FLOR' and e.startswith('HangingGarlic'): return ('PT_GARLIC', 0)
    # the flora that are not plants: taken whole (what they give: 1 rabbit, 2 pheasant, 3..5 a small, medium, large purse)
    if t == 'FLOR':
        for k, what in (('HangingRabbit', 1), ('HangingPheasant', 2), ('CoinPurseSmall', 3), ('CoinPurseMedium', 4), ('CoinPurseLarge', 5)):
            if e.startswith(k): return ('PT_HARVEST', what)
    return None

mesh_ix = {}; mesh_list = []                               # model -> index
def has_mesh(e):
    m = models.get(e)
    return bool(m) and mstat.get(m) == 'ok' and os.path.exists(obj_path(m))
def mesh_of(e):
    m = models.get(e)
    if not has_mesh(e): return -1
    if m not in mesh_ix: mesh_ix[m] = len(mesh_list); mesh_list.append((m, e))
    return mesh_ix[m]
# a model placed tilted (Skyrim turns a placed thing about z, then y, then x, each clockwise: rocks and cliffs lean
# with the slope): the engine turns models about the vertical only, so such a one is its own model, turned as placed,
# and placed unturned
tilt_of = {}; tilt_ix = {}
def mesh_tilted(e, rot):
    if not has_mesh(e): return -1
    key = (models[e], tuple(int(round(v)) for v in rot))
    if key not in tilt_ix:
        tilt_ix[key] = len(mesh_list); mesh_list.append((models[e], e + ' tilted %d %d %d' % key[1])); tilt_of[tilt_ix[key]] = key[1]
    return tilt_ix[key]
def rotate(raw, rot):
    rx, ry, rz = [math.radians(v) for v in rot]
    cz, sz = math.cos(-rz), math.sin(-rz); cy, sy = math.cos(-ry), math.sin(-ry); cx, sx = math.cos(-rx), math.sin(-rx)
    def R(p):
        x, y, z = p
        x, y = cz * x - sz * y, sz * x + cz * y
        x, z = cy * x + sy * z, -sy * x + cy * z
        y, z = cx * y - sx * z, sx * y + cx * z
        return (x, y, z)
    return [mk_tri(R(t[0]), R(t[1]), R(t[2]), t[3], uv_of(t)) for t in raw]
def stair_skirt(model, raw):
    # the barrow's flights of steps (NorTmpExtPlatStair*): their sides drop only to the foot of the flight (512 under
    # its top), where Skyrim's mountain meshes - left out here - meet them; the land under them is metres lower, a
    # hollow under the steps. Their sides and their foot carried on down to 1024 under the top (in the stone of the
    # flight's own sides; below the land it is not seen)
    m = model.lower()
    if 'norTmpExtPlatStair'.lower() not in m: return raw
    side = collections.Counter(t[3] for t in raw if abs(_cross(_sub(t[1], t[0]), _sub(t[2], t[0]))[0]) > 1).most_common(1)
    tx = side[0][0] if side else raw[0][3]
    out = list(raw)
    def quad(A, B, C, D, want):
        for T in ((A, B, C), (A, C, D)):
            n = _cross(_sub(T[1], T[0]), _sub(T[2], T[0]))
            if _dot(n, want) < 0: T = (T[0], T[2], T[1])
            out.append(T + (tx,))
    xs = [p[0] for t in raw for p in t[:3]]; x0, x1 = min(xs), max(xs)
    ytop, ybot, zb = 54, -704, -1024
    def zf(y): return -5 - 507 * (ytop - y) / (ytop - ybot)            # (just under the steps)
    if 'capl' in m: quad((x0, ytop, zf(ytop)), (x0, ybot, zf(ybot)), (x0, ybot, zb), (x0, ytop, zb), (-1, 0, 0))
    if 'capr' in m: quad((x1, ytop, zf(ytop)), (x1, ybot, zf(ybot)), (x1, ybot, zb), (x1, ytop, zb), (1, 0, 0))
    quad((x0, ybot, -512), (x1, ybot, -512), (x1, ybot, zb), (x0, ybot, zb), (0, -1, 0))   # the foot
    return out
def grid_polys(g):
    """the filled cells of a grid as simple polygons (corner coordinates, counter-clockwise): each 4-connected part,
    its small holes (under 40 cells: posts through the boards) filled, its outline straightened (Douglas-Peucker, a
    cell); a part with a big hole (a stairwell) as rectangles instead"""
    import numpy as np
    H, W = g.shape
    lab = -np.ones((H, W), int); parts = []
    for j0 in range(H):
        for i0 in range(W):
            if not g[j0, i0] or lab[j0, i0] >= 0: continue
            st = [(j0, i0)]; lab[j0, i0] = len(parts); cells = []
            while st:
                j, i = st.pop(); cells.append((j, i))
                for dj, di in ((0, 1), (0, -1), (1, 0), (-1, 0)):
                    jj, ii = j + dj, i + di
                    if 0 <= jj < H and 0 <= ii < W and g[jj, ii] and lab[jj, ii] < 0: lab[jj, ii] = len(parts); st.append((jj, ii))
            parts.append(cells)
    out = []
    for k, cells in enumerate(parts):
        m = np.zeros((H + 2, W + 2), bool)
        for j, i in cells: m[j + 1, i + 1] = True
        # the outside: flood from the border; what is neither part nor outside is a hole
        outside = np.zeros_like(m); st = [(0, 0)]; outside[0, 0] = True
        while st:
            j, i = st.pop()
            for dj, di in ((0, 1), (0, -1), (1, 0), (-1, 0)):
                jj, ii = j + dj, i + di
                if 0 <= jj < H + 2 and 0 <= ii < W + 2 and not m[jj, ii] and not outside[jj, ii]: outside[jj, ii] = True; st.append((jj, ii))
        holes = ~m & ~outside
        if holes.sum() >= 40:
            # rectangles (greedy)
            used = ~m
            for j in range(H + 2):
                i = 0
                while i < W + 2:
                    if used[j, i]: i += 1; continue
                    i1 = i
                    while i1 + 1 < W + 2 and not used[j, i1 + 1]: i1 += 1
                    j1 = j
                    while j1 + 1 < H + 2 and not used[j1 + 1, i:i1 + 1].any(): j1 += 1
                    used[j:j1 + 1, i:i1 + 1] = True
                    out.append([(i - 1, j - 1), (i1, j - 1), (i1, j1), (i - 1, j1)])
                    i = i1 + 1
            continue
        m |= holes
        # the outline: walk the boundary edges (cell corners), the part on the left
        E = {}
        for j in range(1, H + 1):
            for i in range(1, W + 1):
                if not m[j, i]: continue
                x, y = i - 1, j - 1
                if not m[j - 1, i]: E[(x + 1, y)] = (x, y)          # bottom edge, going -x
                if not m[j + 1, i]: E[(x, y + 1)] = (x + 1, y + 1)  # top edge, going +x
                if not m[j, i - 1]: E[(x, y)] = (x, y + 1)          # left edge, going +y
                if not m[j, i + 1]: E[(x + 1, y + 1)] = (x + 1, y)  # right edge, going -y
        if not E: continue
        start = min(E); loop = [start]; cur = E[start]
        while cur != start and len(loop) < 100000:
            loop.append(cur); cur = E.get(cur)
            if cur is None: break
        if cur != start: continue
        # it goes clockwise as walked (bottom edges -x): make it counter-clockwise
        loop = loop[::-1]
        # straighten: drop the corners within a cell of the line between kept ones
        def dp(pts):
            if len(pts) < 3: return pts
            a, b = pts[0], pts[-1]; L = math.hypot(b[0] - a[0], b[1] - a[1]) or 1
            dmax, k = -1, 0
            for q in range(1, len(pts) - 1):
                d = abs((b[0] - a[0]) * (a[1] - pts[q][1]) - (a[0] - pts[q][0]) * (b[1] - a[1])) / L
                if d > dmax: dmax, k = d, q
            if dmax <= 1.0: return [a, b]
            return dp(pts[:k + 1])[:-1] + dp(pts[k:])
        n = len(loop); far = max(range(n), key=lambda q: (loop[q][0] - loop[0][0]) ** 2 + (loop[q][1] - loop[0][1]) ** 2)
        poly = dp(loop[:far + 1])[:-1] + dp(loop[far:] + [loop[0]])[:-1]
        if len(poly) >= 3: out.append(poly)
    return out
def ear_clip(P):
    """triangles (index triples) of a simple counter-clockwise polygon (its x, y)"""
    idx = list(range(len(P))); tris = []
    def cross(o, a, b): return (P[a][0] - P[o][0]) * (P[b][1] - P[o][1]) - (P[a][1] - P[o][1]) * (P[b][0] - P[o][0])
    guard = 0
    while len(idx) > 3 and guard < 100000:
        guard += 1; done = False
        for k in range(len(idx)):
            a, b, c = idx[k - 1], idx[k], idx[(k + 1) % len(idx)]
            if cross(a, b, c) <= 1e-9: continue
            ok = True
            for q in idx:
                if q in (a, b, c): continue
                if cross(a, b, q) >= -1e-9 and cross(b, c, q) >= -1e-9 and cross(c, a, q) >= -1e-9: ok = False; break
            if ok: tris.append((a, b, c)); idx.pop(k); done = True; break
        if not done:
            # (a degenerate leftover: fan it)
            for k in range(1, len(idx) - 1): tris.append((idx[0], idx[k], idx[k + 1]))
            return tris
    if len(idx) == 3: tris.append(tuple(idx))
    return tris
def level_floors(raw, cell=10.0):
    # plank floors (the porches, the balconies, the walkways: boards with gaps, each its own little box) kept as floors:
    # the level tops of each texture and height laid on a grid of 4 units, the gaps between boards closed, and drawn
    # as a few rectangles instead (the simplifying keeps big flat areas, not a hundred boards)
    import numpy as np
    groups = collections.defaultdict(list); rest = []
    for t in raw:
        n = _cross(_sub(t[1], t[0]), _sub(t[2], t[0])); L = math.sqrt(_dot(n, n)) or 1
        if n[2] / L > 0.9 and not t[3].startswith('gen:'): groups[(t[3], sum(p[2] for p in t[:3]) / 3)].append(t)
        else: rest.append(t)
    # only the floors: the walkway boards' heights, and at them the boards of other wood among them (beams' tops, the
    # roofs' flats stay as they were)
    fl = sorted(set(round(k[1]) for k in groups if 'walkway' in k[0].lower()))
    for k in list(groups):
        if not any(abs(k[1] - h) <= 12 for h in fl): rest += groups.pop(k)
    # (the boards' tops within 12 units of each other: one floor)
    merged = collections.defaultdict(list)
    for tx in set(k[0] for k in groups):
        hs = sorted(k[1] for k in groups if k[0] == tx); lead = None
        for h in hs:
            if lead is None or h - last > 12: lead = h
            last = h; merged[(tx, lead)] += groups[(tx, h)]
    groups = merged
    out = list(rest)
    for (tx, hz), ts in groups.items():
        area = sum(math.sqrt(_dot(_cross(_sub(t[1], t[0]), _sub(t[2], t[0])), _cross(_sub(t[1], t[0]), _sub(t[2], t[0])))) / 2 for t in ts)
        if len(ts) < 12 or area < 20000: out += ts; continue
        xs = [p[0] for t in ts for p in t[:3]]; ys = [p[1] for t in ts for p in t[:3]]
        x0, y0 = min(xs), min(ys); W = int((max(xs) - x0) / cell) + 2; H = int((max(ys) - y0) / cell) + 2
        if W * H > 400000: out += ts; continue
        g = np.zeros((H, W), bool)
        gx = (np.arange(W) + 0.5) * cell + x0; gy = (np.arange(H) + 0.5) * cell + y0
        X, Y = np.meshgrid(gx, gy)
        for A, B, C, _ in [t[:4] for t in ts]:
            bx0, bx1 = int((min(A[0], B[0], C[0]) - x0) / cell), int((max(A[0], B[0], C[0]) - x0) / cell) + 1
            by0, by1 = int((min(A[1], B[1], C[1]) - y0) / cell), int((max(A[1], B[1], C[1]) - y0) / cell) + 1
            sx, sy = X[by0:by1 + 1, bx0:bx1 + 1], Y[by0:by1 + 1, bx0:bx1 + 1]
            d = (B[1] - C[1]) * (A[0] - C[0]) + (C[0] - B[0]) * (A[1] - C[1])
            if abs(d) < 1e-9: continue
            l1 = ((B[1] - C[1]) * (sx - C[0]) + (C[0] - B[0]) * (sy - C[1])) / d
            l2 = ((C[1] - A[1]) * (sx - C[0]) + (A[0] - C[0]) * (sy - C[1])) / d
            g[by0:by1 + 1, bx0:bx1 + 1] |= (l1 >= -0.02) & (l2 >= -0.02) & (1 - l1 - l2 >= -0.02)
        # close the gaps between the boards (up to 2 cells), then back
        def dil(a):
            b = a.copy(); b[1:] |= a[:-1]; b[:-1] |= a[1:]; b[:, 1:] |= a[:, :-1]; b[:, :-1] |= a[:, 1:]; return b
        def ero(a):
            b = a.copy(); b[1:] &= a[:-1]; b[:-1] &= a[1:]; b[:, 1:] &= a[:, :-1]; b[:, :-1] &= a[:, 1:]
            b[0, :] = a[0, :] & b[0, :]; return b
        g = ero(dil(g))
        z = max(p[2] for t in ts for p in t[:3])
        for poly in grid_polys(g):
            P = [(x0 + px * cell, y0 + py * cell, z) for px, py in poly]
            for a_, b_, c_ in ear_clip(P):
                t_ = mk_tri(P[a_], P[b_], P[c_], tx, None); t_.floor = 1; out.append(t_)
    return out
def raw_of(mi):
    model = mesh_list[mi][0]
    gm = GENMODEL.get(model.lower())
    raw = gm(model, mesh_list[mi][1].endswith(' (far)')) if gm else stair_skirt(model, load_obj(obj_path(model)))
    if mi in tilt_of: raw = rotate(raw, tilt_of[mi])
    return raw

cells = {}
for cid, cname in CELLS:
    cells[cname] = {'id': cid, 'name': cname, 'stat': [], 'prop': [], 'item': [], 'flame': [], 'light': [], 'people': [], 'arch': [], 'door': None, 'shut': [],
                    'dropped': collections.Counter(), 'prop_mi': []}
dungeon_models = set(); outside_models = set(); water_models = set(); spin_models = set(); tree_models = set(); mesh_wall_models = set()
for r in refs:
    c = cells[r['cell']]
    e, t = r['base_editor_id'], r['base_type']
    geo = r['cell'] in GEOMONLY
    x, y, z, rz = float(r['x']), float(r['y']), float(r['z']), float(r['rot_z'])
    sc = float(r['scale'])
    if SKIPBASE.search(e) or t in ('IDLM', 'SOUN', 'TXST', 'SPEL', 'HAZD'): continue
    if t == 'FURN' and re.search(r'Ambush', e): continue     # (v0.4.55) where a creature drops in (Furniture\\CreatureExit): a marker
    # (v0.4.55) the barrow's working parts are the game's (docs/maps/gen_bfb_kit.py -> src/bfb_kit.inc): not drawn
    # still here - the puzzle pillars, the lever, the gates, the chains, the claw door, the traps, the sarcophagus' lid,
    # its chests; its secret way out left open (Skyrim slides it aside)
    if DUN and (re.match(BFB_DYN if AREA == 'bfb' else FH_DYN, e) or (t == 'CONT' and re.match(r'^Treas(Bandit|Draugr|Explorer).*Chest', e))): continue
    if t == 'DOOR' and 'Secret' in e:
        mi = mesh_of(e)                                  # Delphine's wardrobe, the secret door: drawn, kept shut
        if mi >= 0: c['stat'].append((mi, x, y, z, rz, int(round(sc * 256)), 0))
        b = [int(v) for v in r['bounds'].split()] if r['bounds'] else [-40, -20, 0, 40, -20, 190]
        a = math.radians(float(r['rot_z'])); ca, sa = math.cos(a), math.sin(a); ly = (b[1] + b[4]) / 2
        P = [(x + lx * ca + ly * sa, y - lx * sa + ly * ca) for lx in (b[0] - 6, b[3] + 6)]
        c['shut'].append((int(round(P[0][0])), int(round(P[0][1])), int(round(P[1][0])), int(round(P[1][1])), int(z + b[2]), int(z + b[5])))
        continue
    # (a table with its benches: one Skyrim model - drawn here, the game seats people on its benches)
    if geo and t == 'FURN' and e.startswith('CommonTable') and 'Bench' in e: t = 'STAT'
    if geo and (t in ('NPC_', 'DOOR', 'CONT', 'FURN') or (t == 'FLOR' and e.startswith('HangingGarlic'))):
        if t != 'NPC_': c['dropped']['%s %s (the cell\'s own furniture: gen_sginn.py)' % (t, e)] += 1
        continue
    if t == 'NPC_':
        if e in PEOPLE: c['people'].append((PEOPLE[e], x, y, z, rz))
        continue
    if t == 'LIGH':
        c['light'].append((x, y, z, 1 if 'Fire' in e else 0)); continue
    if e.startswith('FireplaceWood') or e.startswith('Campfire'):
        c['flame'].append((x, y, z + 6, int(480 * sc)))
    # (v0.4.74) a Nordic ruin's hinged doors between its rooms (Skyrim's NorDoorSm, NorWoodDoor, NorDoorMedium - not
    # the load doors, which are the game's door table): the game's hinged door, drawn with Skyrim's model and swung on
    # its end. item: model | width (units) << 11 | (the leaf's middle across, + 256) << 20 (the models run along y)
    if DUN and t == 'DOOR' and re.match(r'^Nor(DoorSm0\d|DoorSm02Flipped|WoodDoor0\d|DoorMedium0\d)$', e) and not r.get('dest_ref'):
        mi = mesh_of(e)
        b = [int(float(v)) for v in r['bounds'].split()] if r.get('bounds') else None
        if mi >= 0 and b:
            w_ = b[4] - b[1]; hx_ = (b[0] + b[3]) // 2
            c['prop'].append(('PT_DOOR', x, y, z, rz, mi | (w_ << 11) | ((hx_ + 256) << 20))); c['prop_mi'].append((mi, x, y, z, rz, sc, e, r['bounds']))
            continue
    if t == 'DOOR' and e.startswith('FarmhouseAnimDoor'):
        c['prop'].append(('PT_DOOR', x, y, z, rz, 0)); c['prop_mi'].append((-1, x, y, z, rz, sc, e, r['bounds'])); continue
    p = prop_of(e, t)
    if p and p[0] == 'PT_HARVEST':
        mi = mesh_of(e)                                  # drawn with its own model: model * 16 + what it gives
        if mi >= 0: c['prop'].append((p[0], x, y, z, rz, mi * 16 + p[1])); c['prop_mi'].append((mi, x, y, z, rz, sc, e, r['bounds']))
        continue
    if p:
        c['prop'].append((p[0], x, y, z, rz, p[1])); c['prop_mi'].append((mesh_of(e), x, y, z, rz, sc, e, r['bounds'])); continue
    # the things one can take: the game's item for it (by its editor id, Skyrim's record; else by the start of it)
    it = [EDID_ITEM[e]] if e in EDID_ITEM else [v for k, v in ITEMS if e.startswith(k)]
    if geo and t in ITEM_TYPES:
        c['dropped']['%s %s (the cell\'s own goods: gen_sginn.py)' % (t, e)] += 1; continue
    if it and t in ITEM_TYPES:
        c['item'].append((it[0], x, y, z)); continue
    if t in ITEM_TYPES or e.startswith('Gold0'):
        # (no game item for it: drawn where it lies, not to be taken; listed, to give the game one)
        mi = mesh_of(e)
        c['dropped']['%s %s (no game item: %s)' % (t, e, 'drawn, not taken' if mi >= 0 else 'not drawn')] += 1
        if mi >= 0: c['stat'].append((mi, x, y, z, rz, int(round(sc * 256)), 0))
        continue
    mi = mesh_of(e)
    if mi < 0:
        if models.get(e): c['dropped']['%s %s (its model not in the export: %s)' % (t, e, models.get(e))] += 1
        continue
    ml = models.get(e, '').lower()
    hall = ml.startswith('architecture\\farmhouse\\interior\\farmint') or ml.startswith('architecture\\whiterun\\wrinteriors\\wrshackinterior')   # a piece of the hall itself: always drawn (v0.4.48: and Whiterun's shacks')
    c['stat'].append((mi, x, y, z, rz, int(round(sc * 256)), 1 if hall else 0))
    if 'Candle' in e:
        tips = candle_tips(models.get(e, ''))
        if tips:
            # a flame on each candle, turned and scaled with the holder
            a = math.radians(rz); ca, sa = math.cos(a), math.sin(a)
            for tx, ty, tz in tips:
                c['flame'].append((x + sc * (tx * ca + ty * sa), y + sc * (-tx * sa + ty * ca), z + sc * tz, 55))
        else:
            b = bases.get(e, {}).get('bounds', '')
            top = int(b.split()[5]) if b else 20
            c['flame'].append((x, y, z + top, 160 if 'Floor' in e or 'Chandelier' in e else 55))
    m = models.get(e, '').lower()
    if m.startswith('architecture\\farmhouse\\interior') or m.startswith('architecture\\farmhouse\\farmint') or m.startswith('architecture\\whiterun\\wrinteriors') or \
       (DUN and m.startswith(('dungeons\\nordic\\', 'dungeons\\caves\\', 'dungeons\\ruins\\')) and
        not re.search(r'(NoCol|Vine|Rubble|RockPileS|Cobweb|Debris|Dust|Roots)', e, re.I)):      # (v0.4.55: the barrow's halls and caves)
        c['arch'].append((mi, x, y, z, rz, sc))
        if AREA == 'bfb' and os.environ.get('BFBHF'): c.setdefault('hf', set()).add(mi)
        # (v0.4.61) a barrow's halls and caves as the buildings outside: up to 480 triangles near (they were cut to
        # 90 - the tunnels' arches and walls in a few flat facets), a middle model from 3 m, a far one from 10 m
        if AREA != 'main' and m.startswith('dungeons\\'): outside_models.add(mi); dungeon_models.add(mi)

# ---------------------------------------------------------------- Riverwood outside: its buildings as Skyrim builds them
# (Inn01, Smith01, Farmhouse02..06, the mill, the boardwalks and their steps): the models, and floors and walls to walk
# on and against (the raised decks, the porches, the steps). Positions from Riverwood's origin (RW_OX, RW_OZ engine
# metres in rw_ext.inc = Skyrim x 21996, y -45244), heights Skyrim's own.
OUT_OX, OUT_OY = 21996, -45244
# what stands in Riverwood, by kind: buildings and boardwalks, walls and fences, the bridge, the cliffs (their own floors
# and walls; a far model each); the river's surfaces and falls (drawn as water, no collision); the water wheel (turns);
# the rest (rocks, logs and stumps, firewood, hay, carts, furniture, signs, racks, the campfire: a box each)
BIG_OUT = re.compile(r'^(Farm|Walkway|Stonewall|FenceWoven|Bridge01|DirtCliffs|ResourceObjectSawmill)', re.I)
WATER_OUT = re.compile(r'^(FXCreek|FXRapids|FXrapids|FxRapids|FXWaterfall|FXSplash)', re.I)
SPIN_OUT = {'FarmLumbermill01WaterWheel01'}
TREE_OUT = re.compile(r'^TreePineForest(0[1-5]|0[1-5]Dead|Dead0[1-5])$')   # Riverwood's pines (their branches sway in Skyrim)
SMALL_OUT = re.compile(r'^(Rock|TreePineForest(Cut)?(Log|Stump)|FirewoodPile|HayMound|HandCart|FishRack|Spit|Campfire01Land|Sign|RoadSign|'
                       r'BeeHive|BarrelFood|CraftingBlacksmith(Armor|Sharp)|CraftingCookingPot|FarmBench|CommonChair|ExteriorWoodenTable|'
                       r'FarmTable|WoodChoppingBlock|WeaponRack(Mid$|EndCap)|Axe01$|HorseTrough|LeverPush|WoodPile$|CraftingBlackSmithForge$|CraftingTanningRackMarker$)', re.I)
U_M = 0.0142875
def in_rw(x, y):
    if AREA != 'main': return False                     # (another area's file: not Riverwood's outside)
    ex, ez = (x + 233472) * U_M, (y + 176128) * U_M
    return 3585 <= ex <= 3712 and 1810 <= ez <= 1965
out = {'id': 'CELL_NONE', 'stat': [], 'prop': [], 'item': [], 'flame': [], 'light': [], 'people': [], 'arch': [], 'door': None, 'shut': [], 'outside': 1}
for r in csv.DictReader(open(os.path.join(PLACES, 'refs_area.csv'), encoding='utf-8')):
    e = r['base_editor_id']
    if r['world'] != 'Tamriel': continue
    door = r['base_type'] == 'DOOR' and e == 'FarmhouseLDoor01'                # the houses' own doors
    big = (r['base_type'] == 'STAT' or e == 'ResourceObjectSawmill') and BIG_OUT.match(e) is not None
    water = WATER_OUT.match(e) is not None
    spin = e in SPIN_OUT
    small = SMALL_OUT.match(e) is not None and not big
    tree = r['base_type'] == 'TREE' and TREE_OUT.match(e) is not None
    if not (door or big or water or spin or small or tree): continue
    x, y = float(r['x']), float(r['y'])
    if not in_rw(x, y): continue
    mi = mesh_of(e)
    if mi < 0: continue
    lx, ly, z, rz, sc = x - OUT_OX, y - OUT_OY, float(r['z']), float(r['rot_z']), float(r['scale'])
    if big:
        outside_models.add(mi)
        if e.startswith(('FenceWoven', 'Stonewall')): out.setdefault('wallonly', set()).add(mi)   # nothing to stand on
        if not e.startswith('DirtCliffs'): out['arch'].append((mi, lx, ly, z, rz, sc))   # the cliffs: the land's own
    if water: water_models.add(mi)
    if spin: spin_models.add(mi); rz += 90                    # its axle turned to the model's z (see the models)
    if door: outside_models.add(mi)
    if tree: tree_models.add(mi)
    if e in ('CraftingBlackSmithForge', 'CraftingTanningRackMarker'):
        # a piece of furniture you work at: its own walls (its box would take in the place you stand at it)
        out.setdefault('nofloor', set()).add(mi); out['arch'].append((mi, lx, ly, z, rz, sc)); mesh_wall_models.add(mi)
    out['stat'].append((mi, lx, ly, z, rz, int(round(sc * 256)), 1))
    if e.startswith('Campfire01Land'): out['flame'].append((lx, ly, z + 6, int(480 * sc)))
out['notree'] = tuple(v / U_M - o for v, o in zip((3585, 1810, 3712, 1965), (233472, 176128, 233472, 176128)))  # the village's land: no grown trees
if AREA == 'main': cells['RiverwoodOutside'] = out
print('outside: %d pieces, %d models' % (len(out['stat']), len(outside_models)))

# ---------------------------------------------------------------- Bleak Falls Barrow outside, as Skyrim builds it
# (tools/skyrim_area.py bleakfalls): the temple's platforms, walls, flights and towers, the great ribs and the dragon
# statues, its doors, Bleak Falls Tower below it, the rocks, cliffs and trees round them; positions from the main
# door, heights Skyrim's own. Their own floors and walls to walk on and against (the platforms' tops, the flights, the
# tower's floors, the rocks).
BF_OX, BF_OY = -3032, -43341
BF_BOX = (-13500, -48500, 9000, -40000)                    # from the hidden way out of the sanctum to past the tower
# (the mountains' own models are left out: the land is the mountain here, and one walks on it; inside Skyrim's
#  mountain meshes, which stand over and under the land, one would see nothing)
BF_SKIP = re.compile(r'^(FX|INV_|Sky|Cloud|MapMarker|XMarker|default|.*Marker|AutoLoadDoor|NorLargeDoorMinUse|Mountain)', re.I)
BF_BIG = re.compile(r'^(Nor|Mountain|RockCliff|RockL)', re.I)                 # a far model each
BF_ARCH = re.compile(r'^(Nor|RockCliff|RockL|RockM\d|RockPileL)', re.I)   # their own floors and walls
BF_TREE = re.compile(r'^Tree(PineForest|DeadShrub|PineShrub)', re.I)
def _area_cells(d_):
    fa = os.path.join(d_, 'area.txt')
    if not os.path.exists(fa): return None
    m_ = re.search(r'tamriel cells x (-?\d+)\.\.(-?\d+), y (-?\d+)\.\.(-?\d+)', open(fa).read())
    return tuple(int(v) for v in m_.groups()) if m_ else None
def ruin_outside(cname, label, refpath, OX, OY, BOX, trees, doors):
    # (v0.4.68: one function for every ruin's outside - Bleak Falls', Forelhost's) the ruin's pieces in Tamriel within BOX
    # (Skyrim units), from its origin (OX, OY: its main door), heights Skyrim's own; trees: the export's own (False: the
    # land's plants' file has them - the Rift's)
    bf = {'id': 'CELL_NONE', 'stat': [], 'prop': [], 'item': [], 'flame': [], 'light': [], 'people': [], 'arch': [], 'door': None,
          'shut': [], 'outside': 1, 'origin': (OX, OY), 'allheights': 1,
          'notree': (BOX[0] - OX, BOX[1] - OY, BOX[2] - OX, BOX[3] - OY)}
    ntilt = 0
    for r in csv.DictReader(gzip.open(refpath, 'rt', encoding='utf-8')):
        e, t = r['base_editor_id'], r['base_type']
        if r['world'] != 'Tamriel' or t not in ('STAT', 'TREE', 'DOOR'): continue
        if not ref_on(r): continue                       # initially disabled, deleted
        if BF_SKIP.match(e): continue
        if t == 'DOOR' and e not in doors: continue                          # the ruin's great doors (drawn shut)
        if t == 'TREE' and not trees: continue
        x, y = float(r['x']), float(r['y'])
        if not (BOX[0] <= x <= BOX[2] and BOX[1] <= y <= BOX[3]): continue
        z, rz, sc = float(r['z']), float(r['rot_z']), float(r['scale'])
        rx, ry = float(r['rot_x']), float(r['rot_y'])
        tree = t == 'TREE' and BF_TREE.match(e) is not None
        if t == 'TREE' and not tree: continue
        if abs(rx) > 1.5 and abs(rx) < 358.5 or abs(ry) > 1.5 and abs(ry) < 358.5:
            mi = mesh_tilted(e, (rx, ry, rz)); rz = 0; ntilt += 1
        else: mi = mesh_of(e)
        if mi < 0: continue
        lx, ly = x - OX, y - OY
        if tree: tree_models.add(mi)
        elif BF_BIG.match(e): outside_models.add(mi)
        if BF_ARCH.match(e) and not tree: bf['arch'].append((mi, lx, ly, z, rz, sc))
        if e.startswith('Nor'): bf.setdefault('hf', set()).add(mi)
        bf['stat'].append((mi, lx, ly, z, rz, int(round(sc * 256)), 1))
    cells[cname] = bf
    print('%s outside: %d pieces (%d tilted), %d with floors and walls' % (label, len(bf['stat']), ntilt, len(bf['arch'])))
if BFB and AREA == 'main':
    ruin_outside('BleakFallsOutside', 'Bleak Falls', os.path.join(BFB, 'refs.csv.gz'), BF_OX, BF_OY, BF_BOX, True, ('NorLargeDoor',))
# (v0.4.68) Forelhost's ruin on its mountain east of Riften (the riften export's Tamriel cells), from its main door
FH_OX, FH_OY = 182930, -113715
FH_BOX = (180300, -117300, 187900, -108200)
for d_ in sys.argv[5:]:
    if AREA == 'main' and _area_cells(d_) and os.path.exists(os.path.join(d_, 'refs.csv.gz')):
        c_ = _area_cells(d_)
        if c_[0] * 4096 <= FH_OX <= (c_[1] + 1) * 4096 and c_[2] * 4096 <= FH_OY <= (c_[3] + 1) * 4096:
            ruin_outside('ForelhostOutside', 'Forelhost', os.path.join(d_, 'refs.csv.gz'), FH_OX, FH_OY, FH_BOX, False,
                         ('NorLargeDoor', 'NorDoorSmLoad02')); break

# ---------------------------------------------------------------- Whiterun's outskirts, as Skyrim builds them (v0.4.48)
# (tools/skyrim_area.py whiterun): the Stables, Pelagia Farm and its windmill, Chillfurrow Farm, Battle-Born Farm,
# Honningbrew Meadery, their fences, hay, carts and barrels, the rocks and the pines round them; positions from the
# city's middle, heights Skyrim's own. Not what lies in or by the game's own city (Vindgard: its hill and its walled
# approach, game_city.cpp) - it stands where Whiterun stands; nor the civil war's camps and siege works, the markers,
# the roads (the land's paint), the plants (the land's own: kit_flora), the water's effects.
WR_OX, WR_OY = 23678, -6609
WR_BOX = (-170, -200, 240, 60)                              # metres from the city's middle (x east, y north)
WR_SKIP = re.compile(r'^(CW|CivilWar|.*Marker|Marker|XMarker|COCMarker|Collision|Critter|critter|FX|Fx|Mountain|Dragon|'
                     r'Road(?!Sign)|ImperialTent|NorTent|WRScafGuardTower|.*Rubble|WRWall|WRSkyForge|WRHouseBanneredMare|'
                     r'TreeFlora|TG03|MS08|C00|Default|Water|Tundra(Stream|Pond)|PlaneMarker|ComplexScene|Bone|BearTrap|Mine|'
                     r'Hanging|PeltFloor|Rug|Glazed|Bedroll|Pickaxe|Sit|Lean|Wall(Lean)?|Soldier|Cart(Furniture|FurnPass)|Patrol|'
                     r'Idle|Shadow|Look|Warm|Flower|Browse|Cow|Chicken|LvlAn|Enc|Map)', re.I)
WR_BIG = re.compile(r'^(WRHouse|WRStable|Farmhouse|SFarmhouse|Bridge01|StockadeLeanTo)', re.I)      # a far model, floors, walls
WR_FENCE = re.compile(r'^(WRFence|FenceWoven)', re.I)                                               # walls only
WR_KEEP_T = ('STAT', 'TREE', 'DOOR', 'MSTT', 'FURN', 'CONT', 'ACTI', 'LIGH')
WR_TREE = re.compile(r'^(TreePineForest0[1-5]|TreeTundraDriftWoodTree01)$')
WR_SMALL = re.compile(r'^(Rock|Hay|Barrel|Firewood|HandCart|CartFurnStatic|HorseTrough|WoodChopping|CommonTable|CommonChair|WoodenChair|'
                      r'Crafting|Brewery|Honningbrew|Sign|RoadSign|Crate|Misc|Basket|Bucket|Campfire|TreePineForestCut|'
                      r'TreePineForestStump|TreePineForestUprooted|TreeReach|CoastDrift|TreeTundraDrift)', re.I)
def _outline():
    src_ = open(os.path.join(SRCDIR, 'city_plan.inc')).read()
    m_ = re.search(r'KPlanOutline\[\d+\] = \{([^}]*)\}', src_); v_ = [int(x) for x in m_.group(1).replace('\n', ' ').split(',') if x.strip()]
    return [(v_[i] / 10.0, v_[i + 1] / 10.0) for i in range(0, len(v_), 2)]
WR_OUTLINE = _outline()
def wr_city(mx, mz, margin=6.0):
    # in or within margin of the game's own city (its hill's outline), or its walled approach to the gate
    if -135 <= mx <= -58 and -28 <= mz <= 56: return True
    P_ = WR_OUTLINE; inside = False
    for i in range(len(P_)):
        (x1, z1), (x2, z2) = P_[i], P_[(i + 1) % len(P_)]
        if (z1 > mz) != (z2 > mz) and mx < x1 + (mz - z1) * (x2 - x1) / (z2 - z1): inside = not inside
        dx, dz = x2 - x1, z2 - z1; L = dx * dx + dz * dz or 1; t = max(0, min(1, ((mx - x1) * dx + (mz - z1) * dz) / L))
        if (x1 + t * dx - mx) ** 2 + (z1 + t * dz - mz) ** 2 < margin * margin: return True
    return inside
wr = {'id': 'CELL_NONE', 'stat': [], 'prop': [], 'item': [], 'flame': [], 'light': [], 'people': [], 'arch': [], 'door': None,
      'shut': [], 'outside': 1, 'origin': (WR_OX, WR_OY),
      'notree': tuple(v / U_M for v in (WR_BOX[0], WR_BOX[1], WR_BOX[2], WR_BOX[3]))}
wr_people = []                    # (editor id, x, y, z, rot) of the people placed outside (game_whiterun: the outskirts' folk)
if WR and (AREA == 'wr' or (AREA == 'main' and not os.environ.get('WRAREA'))):
    nwr = collections.Counter()
    for r in csv.DictReader(gzip.open(os.path.join(WR, 'refs.csv.gz'), 'rt', encoding='utf-8')):
        e, t = r['base_editor_id'], r['base_type']
        if r['world'] != 'Tamriel': continue
        if not ref_on(r): continue                       # initially disabled, deleted
        x, y = float(r['x']), float(r['y'])
        mx, mz = (x - WR_OX) * U_M, (y - WR_OY) * U_M
        if not (WR_BOX[0] <= mx <= WR_BOX[2] and WR_BOX[1] <= mz <= WR_BOX[3]) or wr_city(mx, mz): continue
        if t == 'NPC_': wr_people.append((e, x, y, float(r['z']), float(r['rot_z']))); continue
        if t not in WR_KEEP_T or WR_SKIP.match(e) or WATERFX.match(e): continue
        big, fence, tree, small = WR_BIG.match(e), WR_FENCE.match(e), t == 'TREE' and WR_TREE.match(e), WR_SMALL.match(e)
        if e == 'FarmhouseWindMillFan':                            # the windmill's sails: turning
            mi = mesh_of(e)
            if mi >= 0:
                spin_models.add(mi); wr['stat'].append((mi, x - WR_OX, y - WR_OY, float(r['z']), float(r['rot_z']), int(round(float(r['scale']) * 256)), 1))
            continue
        door = t == 'DOOR' and e.startswith(('WRShackDoor', 'WRDragonDoor', 'WRFenceStrGate', 'WRStableGate'))
        if t == 'TREE' and not tree: continue
        if not (big or fence or tree or small or door): continue
        z, rz, sc = float(r['z']), float(r['rot_z']), float(r['scale'])
        rx, ry = float(r['rot_x']), float(r['rot_y'])
        # (tilted: a model of its own, turned as placed - only where it leans more than 6 degrees; a little less is
        #  stood upright: each tilted one is another model to hold)
        def _lean(a): return min(a % 360, 360 - a % 360)
        if _lean(rx) > 6 or _lean(ry) > 6:
            mi = mesh_tilted(e, (round(rx / 3) * 3, round(ry / 3) * 3, round(rz / 3) * 3)); rz = 0
        else: mi = mesh_of(e)
        if mi < 0: nwr['no model: ' + e] += 1; continue
        lx, ly = x - WR_OX, y - WR_OY
        if big:
            outside_models.add(mi); wr['arch'].append((mi, lx, ly, z, rz, sc))
        elif fence:
            wr.setdefault('wallonly', set()).add(mi); wr['arch'].append((mi, lx, ly, z, rz, sc))
        elif door: outside_models.add(mi)
        if tree: tree_models.add(mi)
        wr['stat'].append((mi, lx, ly, z, rz, int(round(sc * 256)), 1))
        nwr['big' if big else 'fence' if fence else 'tree' if tree else 'door' if door else 'small'] += 1
        if e.startswith('Campfire01Land'): wr['flame'].append((lx, ly, z + 6, int(480 * sc)))
    cells['WhiterunOutskirts'] = wr
    print('Whiterun outskirts: %d pieces (%s), %d people placed' % (len(wr['stat']), ', '.join('%s %d' % kv for kv in sorted(nwr.items()) if not kv[0].startswith('no ')), len(wr_people)))
    for k_, v_ in sorted(nwr.items()):
        if k_.startswith('no '): print('   ', k_, v_)

# ---------------------------------------------------------------- Whiterun itself, as Skyrim builds it (v0.4.57)
# WhiterunWorld (the city's own world, in Tamriel's coordinates): every piece inside the walls - the walls and gates,
# the houses, Dragonsreach, Jorrvaskr, the temple, the Gildergreen, the Skyforge, the stalls, the clutter - as an
# outside kit cell with the outskirts' origin; walked on Skyrim's navmesh of the city (cell_collision). The main
# gate's doors are left out: the city is open. Not the markers, the effects, the civil war's siege, the items lying
# about (later), the people (later: their schedules).
WRC_SKIP = re.compile(r'^(CW|CivilWar|.*Marker|Marker|XMarker|COCMarker|Collision|Critter|critter|FX|Fx|Default|'
                      r'PlaneMarker|ComplexScene|Patrol|Idle|Shadow|LvlAn|Enc|Map|TG03|MS08|C00|AmbSound|Sound|'
                      r'WRDoorMainGate|Mountain|Water|Lean|Sit|Bedroll|Browse|Warm|Look)', re.I)
WRC_BIG = re.compile(r'^(WRTer|WRMainRoad|WR.*(Base|Platform)|WRCompanion|WRHouse|WRWall|WRCastle|WRTemple|WRJorvaskr|WRDragonsreach|WRTower|WRBridge|WRStair|WRPlatform|'
                     r'WRGate|WRScaf|WRSkyForge|WRUnderforge|WRFort|WRBarracks|WRStable|WRGildergreen|WRArch|WRRamp|WRCliff|WRRock)', re.I)
WRC_KEEP_T = ('STAT', 'TREE', 'DOOR', 'MSTT', 'FURN', 'CONT', 'ACTI', 'LIGH')
if WR and AREA == 'wr':
    city = {'id': 'CELL_NONE', 'name': 'WhiterunWorld', 'stat': [], 'prop': [], 'item': [], 'flame': [], 'light': [], 'people': [],
            'arch': [], 'door': None, 'shut': [], 'outside': 1, 'origin': (WR_OX, WR_OY), 'allheights': 1}
    ncity = collections.Counter(); xs_ = []
    for r in csv.DictReader(gzip.open(os.path.join(WR, 'refs.csv.gz'), 'rt', encoding='utf-8')):
        e, t = r['base_editor_id'], r['base_type']
        if r['world'] != 'WhiterunWorld': continue
        if not ref_on(r): continue                       # initially disabled, deleted
        x, y, z = float(r['x']), float(r['y']), float(r['z'])
        if not (15000 <= x <= 32000 and -14000 <= y <= 6000): continue      # (the city's world reaches far: only the city)
        if t == 'NPC_': ncity['people (later)'] += 1; continue
        if t not in WRC_KEEP_T or WRC_SKIP.match(e) or SKIPBASE.search(e) or WATERFX.match(e): continue
        rz, sc, rx, ry = float(r['rot_z']), float(r['scale']), float(r['rot_x']), float(r['rot_y'])
        def _lean(a): return min(a % 360, 360 - a % 360)
        if _lean(rx) > 6 or _lean(ry) > 6:
            mi = mesh_tilted(e, (round(rx / 3) * 3, round(ry / 3) * 3, round(rz / 3) * 3)); rz = 0
        else: mi = mesh_of(e)
        if mi < 0: ncity['no model: ' + e] += 1; continue
        lx, ly = x - WR_OX, y - WR_OY
        if WRC_BIG.match(e) or (t == 'DOOR'): outside_models.add(mi); ncity['big' if t != 'DOOR' else 'door'] += 1
        elif t == 'TREE': tree_models.add(mi); ncity['tree'] += 1
        else: ncity['small'] += 1
        city['stat'].append((mi, lx, ly, z, rz, int(round(sc * 256)), 1))
        xs_.append((lx, ly))
        if e.startswith(('WRBrazier', 'Campfire01Land')): city['flame'].append((lx, ly, z + 40, int(420 * sc)))
    if xs_:
        city['notree'] = tuple(v / U_M for v in (min(q[0] for q in xs_) * U_M - 10, min(q[1] for q in xs_) * U_M - 10, max(q[0] for q in xs_) * U_M + 10, max(q[1] for q in xs_) * U_M + 10))
    cells['WhiterunCity'] = city
    print('Whiterun (the city): %d pieces (%s)' % (len(city['stat']), ', '.join('%s %d' % kv for kv in sorted(ncity.items()) if not kv[0].startswith('no '))))
    for k_, v_ in sorted(ncity.items()):
        if k_.startswith('no '): print('   ', k_, v_)

# the doors out: the interior's door that leads to Tamriel, and the exterior door leading here
for cname, c in cells.items():
    inner = [d for d in doors if d['cell'] == cname and d['dest_world'] == 'Tamriel']
    outer = [d for d in doors if d['world'] == 'Tamriel' and d['dest_cell'] == cname]
    if cname in GAME_DOORS or c.get('outside'): continue
    if inner and outer:
        i, o = inner[0], outer[0]
        c['door'] = (float(i['x']), float(i['y']), float(i['dest_x']), float(i['dest_y']),     # inside: door, where you come out
                     float(o['x']), float(o['y']), float(o['dest_x']), float(o['dest_y']),     # outside: door, where you come in
                     float(i['z']))                                                             # the floor at the door inside
    else: print('no door for', cname, len(inner), len(outer))

# ---------------------------------------------------------------- the models
def budget(model, tris, ext):
    # (v0.4.38: about two thirds of before - what the 9300 draws at 8 frames a second)
    m = model.lower()
    if m.startswith('architecture') and ext > 200:
        # building pieces (v0.4.38: the rooms' pieces tried at 240 at most - their floors' raised boards spread and
        # the counters' feet showed through them as dashed lines - so 330 as before)
        return max(120, min(330, int(tris ** 0.74)))
    if ext > 60: return max(20, min(90, int(tris ** 0.55)))
    return max(8, min(24, int(tris ** 0.45)))
lod_of = {}
for mi in sorted(outside_models | tree_models):              # a far model for each building and tree seen from afar
    lod_of[mi] = len(mesh_list); mesh_list.append((mesh_list[mi][0], mesh_list[mi][1] + ' (far)'))
    if mi in tilt_of: tilt_of[lod_of[mi]] = tilt_of[mi]
# (v0.4.46) and a middle one for the buildings (architecture: their near ones the biggest), drawn from 3 m off their
# nearest side to 10 m (the game: x the Model detail setting) - the near one only close, the far one not so soon
mid_of = {}
for mi in sorted(outside_models):
    if not (mesh_list[mi][0].lower().startswith('architecture') or mi in dungeon_models) or GENMODEL.get(mesh_list[mi][0].lower()): continue
    mid_of[mi] = len(mesh_list); mesh_list.append((mesh_list[mi][0], mesh_list[mi][1] + ' (mid)'))
    if mi in tilt_of: tilt_of[mid_of[mi]] = tilt_of[mi]
print('buildings with a middle model: %d' % len(mid_of))
# and a plainer one (a third of the triangles) for the furniture and the bigger clutter, drawn past 7 m
nmodels = len(mesh_list)
for mi in range(nmodels):
    if mi in lod_of or mi in outside_models or mi in tree_models or mi in water_models or mi in spin_models: continue
    model = mesh_list[mi][0]; ml = model.lower()
    if ml in GENMODEL or ml.startswith('architecture') or mesh_list[mi][1].endswith(')'): continue
    b = mbounds.get(ml)
    if not b: continue
    ext = max(b[3] - b[0], b[4] - b[1], b[5] - b[2])
    if not 60 < ext <= 200: continue
    lod_of[mi] = len(mesh_list); mesh_list.append((model, mesh_list[mi][1] + ' (lod)'))
    if mi in tilt_of: tilt_of[lod_of[mi]] = tilt_of[mi]
print('furniture with a plainer model: %d' % (len(mesh_list) - nmodels))
# the items' own models, shown turning in the inventory (Skyrim's, simplified; the game's made-up ones where none)
ITEM_MODELS = [
    ('IT_IRON_SWORD', 'Weapons\\Iron\\LongSword.nif'), ('IT_STEEL_SWORD', 'Weapons\\Steel\\SteelSword.nif'),
    ('IT_IRON_AXE', 'Weapons\\Iron\\WarAxe.nif'), ('IT_IRON_DAGGER', 'Weapons\\Iron\\IronDagger.nif'),
    ('IT_BARROW_BLADE', 'Weapons\\Draugr\\DraugrSword.nif'), ('IT_IRON_MACE', 'Weapons\\Iron\\IronMace.nif'),
    ('IT_IRON_GREATSWORD', 'Weapons\\Iron\\IronClaymore.nif'), ('IT_IRON_BATTLEAXE', 'Weapons\\Iron\\IronBattleAxe.nif'),
    ('IT_IRON_WARHAMMER', 'Weapons\\Iron\\WarHammer.nif'), ('IT_HUNTING_BOW', 'Weapons\\Iron\\IronBow.nif'),
    ('IT_LONG_BOW', 'Weapons\\Wooden\\Woodenbow.nif'), ('IT_IRON_ARROW', 'Weapons\\Iron\\IronArrow.nif'),
    ('IT_STEEL_DAGGER', 'Weapons\\Steel\\SteelDagger.nif'), ('IT_NORD_FROST', 'Weapons\\Draugr\\DraugrHandAxe.nif'),
    ('IT_TORCH', 'Weapons\\Torch\\torch.nif'),
    ('IT_POT_MHEAL', 'Clutter\\Potions\\PotionHealthLesser.nif'), ('IT_POT_HEAL', 'Clutter\\Potions\\PotionHealthExtra.nif'),
    ('IT_POT_MMAG', 'Clutter\\Potions\\PotionMagickaLesser.nif'), ('IT_POT_MSTAM', 'Clutter\\Potions\\PotionStaminaLesser.nif'),
    ('IT_POT_RESFIRE', 'Clutter\\Potions\\PotionResistFire50.nif'), ('IT_POT_RESFROST', 'Clutter\\Potions\\PotionResistFrost50.nif'),
    ('IT_POISON_WEAK', 'Clutter\\Potions\\PoisonDamageHealth01.nif'),
    ('IT_BERRY_WINE', 'Clutter\\Potions\\MeadBlackBriar.nif'), ('IT_CREAM_MEAD', 'Clutter\\Potions\\MeadHonningBrew01.nif'),
    ('IT_BREAD', 'Clutter\\Food\\Bread01A.nif'), ('IT_CHEESE', 'Clutter\\Food\\CheeseWedge01.nif'),
    ('IT_APPLE', 'Clutter\\Food\\Apple01.nif'), ('IT_VENISON', 'Clutter\\Ingredients\\VenisonCooked.nif'),
    ('IT_RAW_VENISON', 'Clutter\\Ingredients\\Venison.nif'), ('IT_RAW_BEEF', 'Clutter\\Ingredients\\BeefMeat.nif'),
    ('IT_COOKED_BEEF', 'Clutter\\Ingredients\\BeefMeatCooked.nif'), ('IT_SALMON_STEAK', 'Clutter\\Food\\CookedSalmonSteak01.nif'),
    ('IT_SALMON', 'Clutter\\DeadAnimals\\SalmonMeat01.nif'), ('IT_RAW_RABBIT', 'Clutter\\DeadAnimals\\RabbitMeat01.nif'),
    ('IT_PHEASANT', 'Clutter\\DeadAnimals\\PheasantMeat01.nif'), ('IT_CARROT', 'Clutter\\Ingredients\\Carrot01.nif'),
    ('IT_SALT', 'Clutter\\Ingredients\\SaltPile.nif'), ('IT_BONE_MEAL', 'Clutter\\Ingredients\\Bonemeal01.nif'),
    ('IT_GOAT_LEG', 'Clutter\\Ingredients\\GoatMeat.nif'), ('IT_TROLL_FAT', 'Clutter\\Ingredients\\TrollFat01.nif'),
    ('IT_SPIDER_EGG', 'Clutter\\Ingredients\\SpiderEgg01.nif'), ('IT_RAT_TAIL', 'Clutter\\Ingredients\\SkeeverTail01.nif'),
    ('IT_FROST_VENOM', 'Clutter\\Ingredients\\FrostbiteVenom01.nif'), ('IT_MUDCRAB_CHITIN', 'Clutter\\Ingredients\\MudcrabChitin.nif'),
    ('IT_DEER_HIDE', 'Clutter\\Ingredients\\DeerPelt.nif'), ('IT_EMBER_SALTS', 'Clutter\\Ingredients\\FireSaltPile.nif'),
    ('IT_SHADOW_SALTS', 'Clutter\\Ingredients\\VoidSalts.nif'), ('IT_WISP_DUST', 'Clutter\\Ingredients\\Glowdust01.nif'),
    ('IT_CRONE_FEATHERS', 'Clutter\\Ingredients\\HagFeathers.nif'), ('IT_TAPROOT', 'Clutter\\Ingredients\\SprigganTaproot.nif'),
    ('IT_TREANT_SAP', 'Clutter\\Ingredients\\Sap.nif'), ('IT_GHOST_ESSENCE', 'Clutter\\Ingredients\\Ectoplasm.nif'),
    ('IT_WRAITH_TEETH', 'Clutter\\Ingredients\\IceWraithTeeth.nif'), ('IT_PIKE_SCALES', 'Clutter\\Ingredients\\SlaughterFishScale.nif'),
    ('IT_GARNET', 'Clutter\\Gemstones\\Garnet.nif'), ('IT_AMETHYST', 'Clutter\\Gemstones\\Amethyst.nif'),
    ('IT_TANKARD', 'Clutter\\Dining Set\\BasicTankard01.nif'), ('IT_PLATE', 'Clutter\\Dining Set\\BasicWoodenPlate01.nif'),
    ('IT_WOOD_BOWL', 'Clutter\\Dining Set\\BasicWoodenBowl01.nif'), ('IT_JUG', 'Clutter\\GlazedJugSmall01.nif'),
    ('IT_CLAY_POT', 'Clutter\\GlazedPot01.nif'), ('IT_LOCKPICK', 'Clutter\\Common\\Lockpick.nif'),
    ('IT_RUSTY_KEY', 'Clutter\\Key01.nif'), ('IT_CHARRED_MEAT', 'Clutter\\Food\\CharredSkeeverMeat01.nif'),
    ('IT_BOOK_DRAGONBORN', 'Clutter\\Books\\BasicBook01.nif'), ('IT_BOOK_DRAGONLANG', 'Clutter\\Books\\BasicBook03.nif'),
    ('IT_BOOK_NIGHTTEARS', 'Clutter\\Books\\BasicBook04.nif'), ('IT_BOOK_THIEF', 'Clutter\\Books\\BasicBook05.nif'),
    ('IT_BOOK_HERBS', 'Clutter\\Books\\BasicBook06.nif'), ('IT_ARVEL_JOURNAL', 'Clutter\\Books\\JournalLowPoly01.nif'),
    ('IT_VEG_SOUP', 'Clutter\\Ingredients\\Stew.nif'), ('IT_VENISON_STEW', 'Clutter\\Ingredients\\Stew.nif'),
    ('IT_CABBAGE_SOUP', 'Clutter\\Ingredients\\Stew.nif'), ('IT_APPLE_STEW', 'Clutter\\Ingredients\\Stew.nif'),
    ]
# (v0.4.40: not in the area's file - their own, data/items.hit, read one at a time as the inventory shows it: see the
#  end of this file)
# (v0.4.40) the items by Skyrim's own records: an item's editor id -> its model, from the export's items.csv
# (tools/skyrim_meshes.py; armour: its model lying on the ground) - over the paths above, which stay for an older
# export
edid_model = {}
for d_ in MESH_DIRS:
    fi_ = os.path.join(d_, 'items.csv')
    if os.path.exists(fi_):
        for r_ in csv.DictReader(open(fi_, encoding='utf-8')):
            if r_['model']: edid_model.setdefault(r_['editor_id'], r_['model'])
item_mi = {}
_im = dict(ITEM_MODELS)
for it, ed in ITEM_EDID.items():
    m_ = edid_model.get(ed)
    if m_ and os.path.exists(obj_path(m_)): _im[it] = m_
item_models = [(it, mdl) for it, mdl in sorted(_im.items()) if os.path.exists(obj_path(mdl))]
print('items by their Skyrim records: %d (items.csv: %d records)' % (sum(1 for it in ITEM_EDID if edid_model.get(ITEM_EDID[it])), len(edid_model)))
print('items with their own model: %d of %d' % (len(item_models), len(ITEM_MODELS)))
# ---------------------------------------------------------------- Skyrim's own textures (the Textures setting)
# Every texture the models use that the export has as a picture (64 x 64; not the cut-out ones, which have
# transparency the engine cannot draw yet) gets a number; its faces get Skyrim's texture coordinates (taken from the
# full model for the simplified one: each face from the original triangle nearest it) and a slot (number low, number
# high, 0, 0xC0). The pictures, on one palette of 256 colours, go to src/kit_tex.bin (tools/kit_pack.py adds it to
# the area's file).
import numpy as np
SKYTEX = []; sky_ix = {}
CARD_TEX_L = set()           # (the plants' card pictures, lower case: drawn cut out - filled before the numbering)
def tex_file(tx, suffix='.ppm'):
    tx = tx.split('#vc')[0]                             # (v0.4.50: a darkened shape's material: its texture's file)
    i_ = tx.lower().find('textures\\')                  # (some paths carry a build folder first: skyrimhd\build\pc\data\...)
    if i_ < 0: return None
    tx = tx[i_:]
    stem = re.sub(r'[^A-Za-z0-9_.-]', '_', tx[len('textures\\'):].replace('\\', '_'))[:-4]
    for d in MESH_DIRS:
        fn = os.path.join(d, 'textures', stem + suffix)
        if os.path.exists(fn): return fn
    return None
# textures with transparency: the cut-out leaves, grass, flowers, effects stay out; the rest (pelts with ragged
# edges, rugs, banners: mostly opaque) are drawn whole, their transparent texels in the opaque ones' average colour
FOLIAGE = ('plants\\', 'landscape\\trees', 'landscape\\grass', 'fieldgrass', 'effects\\', 'blood', 'candles',
           'woodfires', 'hay', 'shrub', 'thicket', 'cave', 'skeevernest', 'dirt02', 'snowrocks', 'riverbededge', 'rocks01')
_alpha = {}
def alpha_info(tx):
    # (fraction of opaque texels, their average r, g, b) of a texture with transparency; None without - and None
    # when its alpha is no transparency: the shapes say so (an export with "# alpha" lines), or (an older export)
    # all but a twentieth of it would be cut away (the Sleeping Giant's sign: paint, its alpha the shine's mask)
    modes = TEX_ALPHA.get(tx.lower())
    if modes and modes == {'none'} and any('test' in m or 'blend' in m for v in TEX_ALPHA.values() for m in v): return None
    if tx not in _alpha:
        fa = tex_file(tx, '_a.pgm'); r = None
        if fa:
            from PIL import Image
            a = list(Image.open(fa).convert('L').getdata())
            c = list(Image.open(tex_file(tx)).convert('RGB').resize(Image.open(fa).size).getdata())
            op = [c[i] for i, v in enumerate(a) if v >= 128]
            r = (len(op) / len(a), tuple(sum(q[k] for q in op) // max(1, len(op)) for k in range(3)))
            if r[0] < 0.05: r = None
        _alpha[tx] = r
    return _alpha[tx]
def sky_key(tx):
    # (v0.4.40) a cut-out picture the game draws cut out (its texel 0 the key): the signs' rings, lettering and hooks
    t = tx.lower()
    ai = alpha_info(t)
    if t in CARD_TEX_L: return True
    return ai is not None and ('signage' in t or ai[0] < 0.5) and not any(k in t for k in FOLIAGE)
def sky_ok(tx):
    t = tx.lower()
    if any(k in t for k in SKIPTEX) or 'window' in t or 'candle' in t or 'flame' in t: return False
    if tex_file(t) is None: return False
    ai = alpha_info(t)
    return t in CARD_TEX_L or ai is None or (ai[0] >= 0.5 and not any(k in t for k in FOLIAGE)) or sky_key(t)
def sky_id(tx):
    if tx not in sky_ix: sky_ix[tx] = len(SKYTEX); SKYTEX.append(tx)
    return sky_ix[tx]
def uv_transfer(pts, faces, raw, slot_tex):
    # per face (whose slot is a Skyrim texture): (u, v) at its three corners, from the nearest original triangle with
    # that texture (facing the same way), its own mapping carried over the face's plane
    out = {}
    by = collections.defaultdict(list)
    for t in raw:
        if uv_of(t): by[t[3].split('#vc')[0]].append(t)
    cache = {}
    for fi, (a, b, c, sl) in enumerate(faces):
        tx = slot_tex.get(sl)
        if not tx or not by.get(tx): continue
        if tx not in cache:
            T = by[tx]
            A = np.array([t[0] for t in T]); B = np.array([t[1] for t in T]); C = np.array([t[2] for t in T])
            N = np.cross(B - A, C - A); L = np.linalg.norm(N, axis=1); L[L == 0] = 1
            cache[tx] = (T, A, B, C, (A + B + C) / 3, N / L[:, None])
        T, A, B, C, CC, NN = cache[tx]
        P = [np.array(pts[a]), np.array(pts[b]), np.array(pts[c])]
        fc = (P[0] + P[1] + P[2]) / 3
        fn = np.cross(P[1] - P[0], P[2] - P[0]); fl = np.linalg.norm(fn); fn = fn / fl if fl else fn
        d2 = ((CC - fc) ** 2).sum(axis=1) + (NN @ fn < 0.3) * 1e9
        i = int(np.argmin(d2))
        a0, b0, c0 = A[i], B[i], C[i]; uv = T[i].uv
        e1, e2 = b0 - a0, c0 - a0
        g11, g12, g22 = e1 @ e1, e1 @ e2, e2 @ e2; det = g11 * g22 - g12 * g12
        if abs(det) < 1e-9: continue
        r = []
        for X in P:
            q = X - a0; p1, p2 = q @ e1, q @ e2
            s_ = (g22 * p1 - g12 * p2) / det; t_ = (g11 * p2 - g12 * p1) / det
            r.append((uv[0][0] + s_ * (uv[1][0] - uv[0][0]) + t_ * (uv[2][0] - uv[0][0]),
                      uv[0][1] + s_ * (uv[1][1] - uv[0][1]) + t_ * (uv[2][1] - uv[0][1])))
        out[fi] = r
    return out
def sky_texels(r):
    # three (u, v) -> engine texel coordinates (v0.4.46: a quarter of a 64 picture's texel - 256 the picture, as
    # planar_uv; were whole texels, too coarse for the 128 and 256 pictures), moved by whole pictures to start near 0
    # (it repeats), kept within 16383 (64 pictures)
    bu = math.floor(min(q[0] for q in r)); bv = math.floor(min(q[1] for q in r))
    return [max(0, min(16383, int(round((q[k] - (bu if k == 0 else bv)) * 256)))) for q in r for k in (0, 1)]

rope_slot = {}; rope_full = {}
# (v0.4.50) how often each model stands in the world: a texture's use weighed by it (square root) - the 64 numbered
# pictures were the most used by the models' own faces, so an interior's one-off pieces pushed out the houses' doors'
# planks (Riverwood's doors flat and pale, like holes to the sky)
_place = collections.Counter()
for c_ in cells.values():
    for q_ in c_['stat']: _place[mesh_list[q_[0]][0]] += 1
mesh_data = []; face_uv = []; mesh_stex = []; mesh_order = []; tex_use = collections.Counter(); tex_wa = collections.Counter(); tex_ua = collections.Counter(); tex_waw = collections.Counter()
for mi, (model, e) in enumerate(mesh_list):
    gm = GENMODEL.get(model.lower())
    raw = raw_of(mi)
    base_mi = mi if mi in tilt_of else mesh_ix.get(model, mi)
    # the rope handrails of the houses' balconies (Rope01: a tube round the posts): kept, made on their own
    rope_raw = [t for t in raw if t[3].lower().endswith('\\rope01.dds')] if base_mi in outside_models and not gm and not e.endswith(')') else []
    uv_raw = raw
    floor_raw = []
    if base_mi in outside_models and not gm and model.lower().startswith('architecture') and not e.endswith(')'):
        raw = level_floors(raw)
        floor_raw = [t for t in raw if getattr(t, 'floor', 0)]; raw = [t for t in raw if not getattr(t, 'floor', 0)]
    if base_mi in water_models: raw = [(A, B, C, 'water') for A, B, C, tx in raw]          # the river: water and foam
    else:
        raw = [t for t in raw if not any(k in t[3] for k in SKIPTEX)] or raw
        raw = [t for t in raw if t[3] not in ('-', '')] or raw     # untextured editor previews (the forge's mannequins)
        # (v0.4.55) inside, a plant's card picture (the barrow's dead shrubs, its roots): drawn cut out - flat, its
        # cards were dark sheets across the halls
        # (v0.4.57: outside too - Whiterun's ground pieces' grass, laid over their dirt, cut out)
        if base_mi not in tree_models:
            for t_ in raw:
                tl_ = t_[3].lower()
                if tl_ not in CARD_TEX_L and any(k in tl_ for k in FOLIAGE) and not any(k in tl_ for k in SKIPTEX) and tex_file(tl_):
                    ai_ = alpha_info(tl_)
                    if ai_ is not None: CARD_TEX_L.add(tl_)          # (v0.4.57: mostly opaque ones too - Whiterun's WRFieldGrass01)
    # Skyrim's snowy pieces (...Snow, ..._SN: the snow is the material's, laid on what faces up): those faces snow
    if re.search(r'Snow|SN\b|SN ', e):
        def up(A, B, C):
            n = _cross(_sub(B, A), _sub(C, A)); L = math.sqrt(_dot(n, n)) or 1
            return n[2] / L > 0.55
        # (not on the wood: the tower's stairs and floors are boards, under its roof)
        raw = [(t[0], t[1], t[2], 'gen:snow') if up(t[0], t[1], t[2]) and t[3] != 'gen:needle' and 'wood' not in t[3].lower() else t for t in raw]   # (a pine: every other tier)
    spin_info = (0, 0)
    if base_mi in spin_models and raw and 'windmillfan' in model.lower():
        # (v0.4.48) a windmill's sails: they turn about Skyrim's y - already the model's (the engine's z) - round the
        # hub at the origin
        spin_info = (0, 0)
    elif base_mi in spin_models and raw:
        # the water wheel: its axle (Skyrim's x) laid along the model's y (the engine's z, about which it turns), the
        # hub at the origin; spin_info: where the hub is from the placed origin (along the axle, and up)
        hx_ = (min(p[0] for t in raw for p in t[:3]) + max(p[0] for t in raw for p in t[:3])) / 2
        hz_ = (min(p[2] for t in raw for p in t[:3]) + max(p[2] for t in raw for p in t[:3])) / 2
        raw = [tuple((-p[1], p[0] - hx_, p[2] - hz_) for p in (A, B, C)) + (tx,) for A, B, C, tx in raw]
        spin_info = (int(hx_ * K), int(hz_ * K))
    xs = [p[i] for t in raw for p in t[:3] for i in (0,)]; ys = [p[1] for t in raw for p in t[:3]]; zs = [p[2] for t in raw for p in t[:3]]
    if raw and max(zs) - min(zs) < 3 and max(xs) - min(xs) > 40 and max(ys) - min(ys) > 40:
        # a pelt or a rug laid on the floor (lifted over it, see lift()): only its top (its thin edges, lifted, showed
        # as a dark dashed line along the floor)
        def top(t):
            n = _cross(_sub(t[1], t[0]), _sub(t[2], t[0])); L = math.sqrt(_dot(n, n)) or 1
            return n[2] / L > 0.5
        raw = [t for t in raw if top(t)] or raw
    ext = max(max(xs) - min(xs), max(ys) - min(ys), max(zs) - min(zs)) if raw else 0
    big = model.lower().startswith('architecture') or ext > 90
    # textures -> slots (eight at most; the rest join the nearest colour)
    texs = collections.Counter(t[3] for t in raw)
    slots = [look(tx, big) if tx != 'water' else ((132, 158, 168, '1') if 'creek' in model.lower() else (176, 192, 198, '1')) for tx, _ in texs.most_common()]
    order = [tx for tx, _ in texs.most_common()]
    for t in floor_raw:
        if t[3] not in order: order.append(t[3]); slots.append(look(t[3], big))
    if rope_raw:
        # (its own slot, among the first eight: its walls are found by it)
        k_ = min(len(order), 7); order.insert(k_, 'rope'); slots.insert(k_, (92, 74, 50, '1'))
    slotcol = list(slots)                         # (the colours, for joining a ninth texture to the nearest slot)
    slot_tex = {}
    if not gm and base_mi not in water_models and base_mi not in spin_models:
        for j, tx in enumerate(order[:8]):
            if sky_ok(tx):
                slot_tex[j] = tx.split('#vc')[0]               # (v0.4.50: a darkened shape: its texture's own number)
    if os.environ.get('DBGM') and os.environ['DBGM'] in model: print('DBG', e, order[:8], slots[:8], slot_tex, [sky_ok(t) for t in order[:8]])
    def slot_of(tx):
        i = order.index(tx)
        if i < 8: return i
        c = slotcol[i]
        return min([j for j in range(min(8, len(order))) if order[j] != 'rope'], key=lambda j: sum((slotcol[j][k] - c[k]) ** 2 for k in range(3)))
    tris = [(A, B, C, slot_of(tx)) for A, B, C, tx in raw]
    if mi in tree_models: want = 56
    elif base_mi in tree_models: want = 20
    elif mi in outside_models and model.lower().startswith('architecture\\farmhouse'): want = int(os.environ.get("HOUSEMAX", "0")) or max(100, min(680, int(len(tris) ** 0.76)))
    elif mi in outside_models: want = max(100, min(480, int(len(tris) ** 0.72)))      # (the barrow's stone: as before)     # (v0.4.42: 480 at most left the gables and the eaves' undersides open - compared with Skyrim's full models, 640 closes them; drawn within 12 m of its nearest side, the far model past)
    elif e.endswith(' (mid)'): want = max(48, min(320, int(len(tris) ** 0.66)))
    elif e.endswith(' (far)'): want = max(24, min(160, int(len(tris) ** 0.75) // 4))
    elif e.endswith(' (lod)'): want = max(8, budget(model, len(tris), ext) // 3)
    elif e.startswith('item '): want = max(24, min(140, int(len(tris) ** 0.65)))     # (seen big, turning, in the inventory)
    elif 'signage' in model.lower(): want = min(len(tris), 400)        # (v0.4.40: the signs as made - simplified, the inn's board went)
    else: want = budget(model, len(tris), ext)
    if gm: want = len(tris)                                           # drawn as made
    sidx, sboxes = step_logs(model, raw) if big and base_mi not in water_models and base_mi not in spin_models else (set(), [])
    if sboxes:
        # the stairs' steps: a box each, the rest simplified as before (keeping room for the boxes' corners)
        rest = [t for i, t in enumerate(tris) if i not in sidx]
        pts, faces = simplify(rest, max(24, want - 10 * len(sboxes)), 480 - 8 * len(sboxes))
        pts = list(pts); faces = list(faces)
        for b in sboxes:
            for A, B, C, s in box_tris(b, slot_of(b[6])):
                ids = []
                for X in (A, B, C):
                    if X not in pts: pts.append(X)
                    ids.append(pts.index(X))
                faces.append((ids[0], ids[1], ids[2], s))
        print('   %s: %d log steps drawn as boxes' % (model, len(sboxes)))
    elif big and not gm and stair_keep(model, raw)[0]:
        # the flights of steps a box a step, the rest simplified (keeping room for them)
        keep, steps_ = stair_keep(model, raw)
        rest = [t for i, t in enumerate(tris) if i not in keep]
        pts, faces = simplify(rest, max(24, want - 10 * len(steps_)), max(120, 480 - 8 * len(steps_)))
        pts = list(pts); faces = list(faces)
        for b in steps_:
            for A, B, C, s_ in box_tris(b, slot_of(b[6])):
                ids = []
                for X in (A, B, C):
                    if X not in pts: pts.append(X)
                    ids.append(pts.index(X))
                faces.append((ids[0], ids[1], ids[2], s_))
    else:
        pts, faces = simplify(tris, want, 900 if 'signage' in model.lower() else (int(os.environ.get('HOUSEV', '1200')) if mi in outside_models and model.lower().startswith('architecture\\farmhouse') else 480))
        if len(faces) < 4 and len(tris) >= 4: pts, faces = simplify(tris, max(24, want * 3))     # (a thin thing collapsed to nothing)
    if floor_raw:
        # the plank floors: as made (not simplified), added to the rest
        # (simplified alone: flat, so only their outlines count; the grid's stair-stepped edges go)
        pts = list(pts); faces = list(faces)
        for A, B, C, tx in [t[:4] for t in floor_raw]:
            faces.append((len(pts), len(pts) + 1, len(pts) + 2, slot_of(tx))); pts += [A, B, C]
        print('   %s: plank floors %d triangles' % (model, len(floor_raw)))
    if rope_raw:
        # the handrail: its tube simplified alone (a few triangles a stretch), added to the house
        rs = slot_of('rope'); rope_slot[mi] = rs; rope_full[mi] = rope_raw
        rp, rf = simplify([(A, B, C, rs) for A, B, C, tx in rope_raw], max(16, len(rope_raw) // 8))
        pts = list(pts); faces = list(faces); base = len(pts)
        pts += list(rp); faces += [(a + base, b + base, c + base, s_) for a, b, c, s_ in rf]
    # (slivers the simplifying leaves - no area, so no true facing: lit at random, a dark dashed line along a seam)
    def _area2(f_):
        n_ = _cross(_sub(pts[f_[1]], pts[f_[0]]), _sub(pts[f_[2]], pts[f_[0]])); return _dot(n_, n_)
    faces = [f_ for f_ in faces if _area2(f_) > 0.25] or faces
    fuv = uv_transfer(pts, faces, uv_raw, slot_tex) if slot_tex else {}
    # the double-sided shapes' faces again, turned the other way (v0.4.41: the roofs' undersides showed the sky)
    dbl = set(j for j, tx in enumerate(order[:8]) if tex_double(tx))
    if dbl:
        n0 = len(faces); faces = list(faces)
        for fi in range(n0):
            a, b, c, s_ = faces[fi]
            if s_ in dbl:
                faces.append((a, c, b, s_))
                if fi in fuv: r_ = fuv[fi]; fuv[len(faces) - 1] = [r_[0], r_[2], r_[1]]
    face_uv.append({fi: sky_texels([r[0], r[2], r[1]]) for fi, r in fuv.items()})        # (written a, c, b)
    mesh_stex.append(slot_tex); mesh_order.append(order[:8] if not gm else [])
    w_ = math.sqrt(max(1, _place[model]))
    for fi in fuv:
        tex_use[slot_tex[faces[fi][3]]] += w_
        # (v0.4.46) how big a picture is in the world: the faces' area against their pictures' (the texels' size)
        P_ = [pts[k_] for k_ in faces[fi][:3]]; r_ = fuv[fi]
        wa_ = math.sqrt(_dot(*(2 * [_cross(_sub(P_[1], P_[0]), _sub(P_[2], P_[0]))])))
        ua_ = abs((r_[1][0] - r_[0][0]) * (r_[2][1] - r_[0][1]) - (r_[2][0] - r_[0][0]) * (r_[1][1] - r_[0][1]))
        tx_ = slot_tex[faces[fi][3]]; tex_wa[tx_] += wa_; tex_ua[tx_] += ua_; tex_waw[tx_] += wa_ * w_
    mesh_data.append((model, e, pts, faces, slots[:8], len(raw), (min(xs), min(ys), min(zs), max(xs), max(ys), max(zs)) if raw else (0,) * 6, spin_info if base_mi in spin_models else None))

# ---------------------------------------------------------------- collision from the building pieces
# (the barrow's pieces are snow-covered and finely made: their floors and walls taken coarser)
coarse_coll = set(a[0] for a in cells['BleakFallsOutside']['arch']) if 'BleakFallsOutside' in cells else set()
if DUN: coarse_coll |= set(a[0] for c_ in cells.values() for a in c_['arch'])     # (v0.4.55: the barrow's halls too)
# the floors from the pieces' full meshes (the simplified ones lose the stairs' log steps), simplified on their own
floor_cache = {}
def floor_mesh(mi):
    if mi in floor_cache: return floor_cache[mi]
    raw = raw_of(mi)
    raw = [t for t in raw if not any(k in t[3] for k in SKIPTEX)] or raw
    raw = [t for t in raw if t[3] not in ('-', '')] or raw
    sidx, sboxes = step_logs(mesh_data[mi][0], raw)
    raw = [t for i, t in enumerate(raw) if i not in sidx]
    up = []
    for A, B, C, tx in raw:
        e = [B[i] - A[i] for i in range(3)]; g = [C[i] - A[i] for i in range(3)]
        n = (e[1] * g[2] - e[2] * g[1], e[2] * g[0] - e[0] * g[2], e[0] * g[1] - e[1] * g[0])
        L = math.sqrt(sum(v * v for v in n)) or 1
        if n[2] / L > 0.6: up.append((A, B, C, 0))
    r = simplify(up, max(24, len(up) // (16 if mi in coarse_coll else 5)), 100000) if up else ([], [])
    if sboxes:                                             # each log step's top, flat, kept whole (stair_ramps: a ramp)
        pts, faces = list(r[0]), list(r[1])
        for b in sboxes:
            x0, y0, z0, x1, y1, z1 = b[:6]; k = len(pts)
            pts += [(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)]
            faces += [(k, k + 1, k + 2, 0), (k, k + 2, k + 3, 0)]
        r = (pts, faces)
    floor_cache[mi] = r
    return r

# Skyrim's stairs are log steps (narrow tops, a rise of about 21 units each) that its physics walks as a slope: each
# flight found (a chain of narrow level patches, each a step up and a step on from the last) becomes one ramp from a
# step below the first to the last, and its steps' own tops are dropped
def stair_ramps(tris):
    n = len(tris)
    par = list(range(n))
    def find(i):
        while par[i] != i: par[i] = par[par[i]]; i = par[i]
        return i
    flat = [i for i, P in enumerate(tris) if max(q[2] for q in P) - min(q[2] for q in P) < 3]
    box = {i: (min(q[0] for q in tris[i]), min(q[1] for q in tris[i]), max(q[0] for q in tris[i]), max(q[1] for q in tris[i]), sum(q[2] for q in tris[i]) / 3) for i in flat}
    for ii, i in enumerate(flat):
        a = box[i]
        for j in flat[ii + 1:]:
            b = box[j]
            if abs(a[4] - b[4]) < 2 and a[0] <= b[2] + 1 and b[0] <= a[2] + 1 and a[1] <= b[3] + 1 and b[1] <= a[3] + 1: par[find(i)] = find(j)
    patches = collections.defaultdict(list)
    for i in flat: patches[find(i)].append(i)
    tread = []
    for r, mem in patches.items():
        x0 = min(box[i][0] for i in mem); y0 = min(box[i][1] for i in mem); x1 = max(box[i][2] for i in mem); y1 = max(box[i][3] for i in mem)
        z = sum(box[i][4] for i in mem) / len(mem)
        if min(x1 - x0, y1 - y0) < 24 and 30 < max(x1 - x0, y1 - y0) < 260:
            tread.append({'m': mem, 'x0': x0, 'y0': y0, 'x1': x1, 'y1': y1, 'z': z, 'cx': (x0 + x1) / 2, 'cy': (y0 + y1) / 2})
    tread.sort(key=lambda t: t['z'])
    used = set(); ramps = []; drop = set()
    for k, t in enumerate(tread):
        if k in used: continue
        chain = [k]
        while True:
            a = tread[chain[-1]]; nxt = None
            for j in range(len(tread)):
                if j in used or j in chain: continue
                b = tread[j]; dz = b['z'] - a['z']; dh = math.hypot(b['cx'] - a['cx'], b['cy'] - a['cy'])
                if 10 <= dz <= 35 and 10 <= dh <= 45 and (nxt is None or dh < nxt[1]): nxt = (j, dh)
            if nxt is None: break
            chain.append(nxt[0])
        if len(chain) < 3: continue
        used.update(chain)
        T = [tread[j] for j in chain]
        dx, dy = T[-1]['cx'] - T[0]['cx'], T[-1]['cy'] - T[0]['cy']; L = math.hypot(dx, dy); ux, uy = dx / L, dy / L; vx, vy = -uy, ux
        rise = (T[-1]['z'] - T[0]['z']) / (len(T) - 1); run = L / (len(T) - 1)
        pts = [(q[0], q[1]) for t in T for i in t['m'] for q in tris[i]]
        w0 = min((p[0] - T[0]['cx']) * vx + (p[1] - T[0]['cy']) * vy for p in pts); w1 = max((p[0] - T[0]['cx']) * vx + (p[1] - T[0]['cy']) * vy for p in pts)
        s0, z0 = -run, T[0]['z'] - rise                                  # a step before the first, a rise lower
        s1, z1 = L + run, T[-1]['z'] + rise                              # up to the landing, a rise above the last
        def P(sv, wv, zv): return (T[0]['cx'] + ux * sv + vx * wv, T[0]['cy'] + uy * sv + vy * wv, zv)
        A, B, C, D = P(s0, w0, z0), P(s0, w1, z0), P(s1, w1, z1), P(s1, w0, z1)
        ramps += [(A, B, C), (A, C, D)]
        for t in T: drop.update(t['m'])
        print('   stairs: %d steps from %.0f to %.0f units high, at (%.0f, %.0f)' % (len(T), z0, z1, T[0]['cx'], T[0]['cy']))
    return [P for i, P in enumerate(tris) if i not in drop] + ramps

# the walls likewise from the full meshes (simplified the pieces lose their doorways), simplified on their own
wall_cache = {}
def wall_mesh(mi):
    if mi in wall_cache: return wall_cache[mi]
    raw = raw_of(mi)
    raw = [t for t in raw if not any(k in t[3] for k in SKIPTEX)] or raw
    raw = [t for t in raw if t[3] not in ('-', '')] or raw
    sidx, _ = step_logs(mesh_data[mi][0], raw)              # the log steps are walked as a ramp, not walls
    raw = [t for i, t in enumerate(raw) if i not in sidx]
    side = []
    for A, B, C, tx in raw:
        e = [B[i] - A[i] for i in range(3)]; g = [C[i] - A[i] for i in range(3)]
        n = (e[1] * g[2] - e[2] * g[1], e[2] * g[0] - e[0] * g[2], e[0] * g[1] - e[1] * g[0])
        L = math.sqrt(sum(v * v for v in n)) or 1
        if abs(n[2] / L) < 0.35: side.append((A, B, C, 0))
    r = simplify(side, max(16, len(side) // (30 if mi in coarse_coll else 10)), 100000) if side else ([], [])
    wall_cache[mi] = r
    return r

# The barrow's stone pieces (platforms, flights, walls, the temple, the towers): their floors and walls from where they
# are solid. Each is sampled on a grid of 16 units from above: under every point, the surfaces facing up (tops) and
# down (undersides), so the stretches of height that are solid; the tops are its floors (neighbouring points at one
# height joined into rectangles: a platform's top is a few), and wherever a point beside it is not solid over a stretch
# this one is, a wall along their common edge over that stretch (the side of a platform, a pillar, a step too high;
# not under an arch: what is solid only overhead does not reach the feet). Walls in a line with the same heights joined.
HF = 24
hf_cache = {}
def hf_collision(mi):
    if mi in hf_cache: return hf_cache[mi]
    global HF
    HF = 12 if mesh_data[mi][1].startswith('NorTower') else int(os.environ.get('HF', '24'))   # (the tower: narrow doorways and stairs)
    raw = raw_of(mi)
    raw = [t for t in raw if not any(k in t[3] for k in SKIPTEX)] or raw
    raw = [t for t in raw if t[3] not in ('-', '')] or raw
    xs = [p[0] for t in raw for p in t[:3]]; ys = [p[1] for t in raw for p in t[:3]]; zs = [p[2] for t in raw for p in t[:3]]
    if not raw: hf_cache[mi] = ([], []); return hf_cache[mi]
    bx0, by0, zmin = math.floor(min(xs) / HF) * HF, math.floor(min(ys) / HF) * HF, min(zs)
    NX, NY = int((max(xs) - bx0) // HF) + 1, int((max(ys) - by0) // HF) + 1
    surf = collections.defaultdict(list)                                  # cell -> [(z, +1 top / -1 underside)]
    side = collections.defaultdict(list)                                  # cell -> [(z low, z high)] of the steep faces there
    for A, B, C, tx in raw:
        e = _sub(B, A); g = _sub(C, A); n = _cross(e, g); L = math.sqrt(_dot(n, n)) or 1
        nz = n[2] / L
        if abs(nz) < 0.25:
            # a steep face: along its edges, the cells it passes and the heights it spans
            zl, zh = min(A[2], B[2], C[2]), max(A[2], B[2], C[2])
            for P, Q in ((A, B), (B, C), (C, A)):
                ln = math.hypot(Q[0] - P[0], Q[1] - P[1]); k = max(1, int(ln / (HF / 2)))
                for q in range(k + 1):
                    x = P[0] + (Q[0] - P[0]) * q / k; y = P[1] + (Q[1] - P[1]) * q / k
                    side[(int((x - bx0) // HF), int((y - by0) // HF))].append((zl, zh))
            continue
        kind = 1 if nz > 0 else -1
        d = (B[1] - C[1]) * (A[0] - C[0]) + (C[0] - B[0]) * (A[1] - C[1])
        if abs(d) < 1e-9: continue
        i0 = max(0, int((min(A[0], B[0], C[0]) - bx0) // HF)); i1 = min(NX - 1, int((max(A[0], B[0], C[0]) - bx0) // HF))
        j0 = max(0, int((min(A[1], B[1], C[1]) - by0) // HF)); j1 = min(NY - 1, int((max(A[1], B[1], C[1]) - by0) // HF))
        for j in range(j0, j1 + 1):
            py = by0 + (j + 0.5) * HF
            for i in range(i0, i1 + 1):
                px = bx0 + (i + 0.5) * HF
                w0 = ((B[1] - C[1]) * (px - C[0]) + (C[0] - B[0]) * (py - C[1])) / d
                w1 = ((C[1] - A[1]) * (px - C[0]) + (A[0] - C[0]) * (py - C[1])) / d
                w2 = 1 - w0 - w1
                if w0 < -1e-6 or w1 < -1e-6 or w2 < -1e-6: continue
                surf[(i, j)].append((w0 * A[2] + w1 * B[2] + w2 * C[2], kind))
    # each point's solid stretches, from below: an underside starts one, a top ends it; the first top with nothing
    # under it is solid as far down as the steep faces round it go; an underside with no top over it, a slab
    zmax = max(zs)
    solid = {}
    for k, lst in surf.items():
        lst.sort()
        iv = []; start = None
        for z, kind in lst:
            if kind < 0:
                if start is None: start = z
            else:
                if start is not None: lo = start
                else:
                    # a top with nothing under it: solid down as far as the steep faces beside it reach from it (a
                    # wall's top, a platform's edge: to their foot; an arch, a lintel: to where they end), else a sheet
                    i, j = k
                    near = [q for di in (-1, 0, 1) for dj in (-1, 0, 1) for q in side.get((i + di, j + dj), ()) if q[1] >= z - 24 and q[0] < z - 12]
                    lo = min(q[0] for q in near) if near else z - 12
                    if iv and iv[-1][1] >= lo: lo = iv[-1][1]
                if iv and lo <= iv[-1][1] + 4: iv[-1] = (iv[-1][0], max(iv[-1][1], z))
                else: iv.append((lo, z))
                start = None
        if start is not None:                                             # (an underside with nothing over it: a slab)
            if iv and start <= iv[-1][1] + 4: iv[-1] = (iv[-1][0], max(iv[-1][1], start + 64))
            else: iv.append((start, start + 64))
        solid[k] = iv
    # the temple's facade and ribs: only their feet are within reach (the terrace runs round and under them)
    if mesh_data[mi][1].startswith('NorTemple'):
        REACH = 300
        solid = {k: [(lo, min(hi, REACH + 300)) for lo, hi in iv if lo <= REACH + 170] for k, iv in solid.items()}
        solid = {k: iv for k, iv in solid.items() if iv}
    # floors: the tops, neighbours on one plane joined into rectangles (flat tops, flights, ramps); tops steeper than one
    # can climb are no floors (the walls round them keep one off them)
    tops = {k: [b for a, b in iv if not (mesh_data[mi][1].startswith('NorTemple') and b > 300)] for k, iv in solid.items()}
    used = set(); floors = []
    def get(i, j, h):
        for t in tops.get((i, j), ()):
            if abs(t - h) <= 12 and (i, j, round(t)) not in used: return t
        return None
    STEEP = 1.15 * HF
    for j in range(NY):
        for i in range(NX):
            for h in tops.get((i, j), ()):
                if (i, j, round(h)) in used: continue
                # the slope along x and along y, from the next points (none: flat)
                gx = 0.0; gy = 0.0
                for t in tops.get((i + 1, j), ()):
                    if abs(t - h) <= STEEP and (i + 1, j, round(t)) not in used: gx = t - h; break
                for t in tops.get((i, j + 1), ()):
                    if abs(t - h) <= STEEP and (i, j + 1, round(t)) not in used: gy = t - h; break
                if abs(gx) < 3: gx = 0.0
                if abs(gy) < 3: gy = 0.0
                def at(q, r): return get(i + q, j + r, h + q * gx + r * gy)
                w = 1
                while i + w < NX and at(w, 0) is not None: w += 1
                hgt = 1
                while j + hgt < NY and all(at(q, hgt) is not None for q in range(w)): hgt += 1
                if w == 1: gx = 0.0
                if hgt == 1: gy = 0.0
                for r in range(hgt):
                    for q in range(w):
                        t = at(q, r)
                        if t is not None: used.add((i + q, j + r, round(t)))
                used.add((i, j, round(h)))
                # a lone point steeper than one climbs (its neighbours far above or below): no floor
                if w == 1 and hgt == 1:
                    nb = [t for (di, dj) in ((1, 0), (-1, 0), (0, 1), (0, -1)) for t in tops.get((i + di, j + dj), ())]
                    if nb and min(abs(t - h) for t in nb) > STEEP: continue
                x0, y0, x1, y1 = bx0 + i * HF, by0 + j * HF, bx0 + (i + w) * HF, by0 + (j + hgt) * HF
                def zc(q, r): return h + (q - 0.5) * gx + (r - 0.5) * gy
                P00, P10, P11, P01 = (x0, y0, zc(0, 0)), (x1, y0, zc(w, 0)), (x1, y1, zc(w, hgt)), (x0, y1, zc(0, hgt))
                floors.append((P00, P10, P11)); floors.append((P00, P11, P01))
    # walls: on the edge between two points, the stretches solid on one side and not on the other
    def minus(a, b):
        out = []
        for lo, hi in a:
            segs = [(lo, hi)]
            for blo, bhi in b:
                nxt = []
                for slo, shi in segs:
                    if bhi <= slo or blo >= shi: nxt.append((slo, shi)); continue
                    if blo > slo: nxt.append((slo, blo))
                    if bhi < shi: nxt.append((bhi, shi))
                segs = nxt
            out += [q for q in segs if q[1] - q[0] >= 24]
        return out
    # (the walls on a grid twice as coarse: each square solid where any of its four points is; carved detail on a face
    # makes no walls of its own)
    def union(ivs):
        ivs = sorted(ivs); out = []
        for lo, hi in ivs:
            if out and lo <= out[-1][1] + 8: out[-1] = (out[-1][0], max(out[-1][1], hi))
            else: out.append((lo, hi))
        return out
    WG = 1                                               # (points to a wall square)
    solid2 = {}
    for (i, j), iv in solid.items(): solid2.setdefault((i // WG, j // WG), []).extend(iv)
    solid = {k: union(v) for k, v in solid2.items()}
    NX2, NY2 = (NX + WG - 1) // WG, (NY + WG - 1) // WG
    runs = collections.defaultdict(list)                 # (direction, line, lo, hi) -> [positions along it]
    for i in range(-1, NX2):
        for j in range(-1, NY2):
            a = solid.get((i, j), [])
            for di, dj, dirn in ((1, 0, 'x'), (0, 1, 'y')):
                b = solid.get((i + di, j + dj), [])
                if not a and not b: continue
                for lo, hi in minus(a, b) + minus(b, a):
                    key = (dirn, i + di if dirn == 'x' else j + dj, int(math.floor(lo / 24)) * 24, int(math.ceil(hi / 24)) * 24)
                    runs[key].append(j if dirn == 'x' else i)
    walls = []
    for (dirn, line, lo, hi), pos in runs.items():
        pos.sort(); k = 0
        while k < len(pos):
            e = k
            while e + 1 < len(pos) and pos[e + 1] == pos[e] + 1: e += 1
            G2 = WG * HF
            if dirn == 'x':
                xw = bx0 + line * G2; walls.append((xw, by0 + pos[k] * G2, xw, by0 + (pos[e] + 1) * G2, lo, hi))
            else:
                yw = by0 + line * G2; walls.append((bx0 + pos[k] * G2, yw, bx0 + (pos[e] + 1) * G2, yw, lo, hi))
            k = e + 1
    hf_cache[mi] = (floors, walls)
    if os.environ.get('COLLSTAT'): print('   hf %s: %d floors, %d walls' % (mesh_data[mi][1], len(floors), len(walls)))
    return hf_cache[mi]

COLLCNT = collections.Counter(); COLLCNTW = collections.Counter()
# (v0.4.55) Skyrim's navmeshes (the export's navmesh.txt.gz, tools/skyrim_area.py): where its people walk - its floors
# exactly, and its edges: an edge with no triangle across it (and no link to another navmesh) is where one cannot walk
# on, a wall from the floor up 2 m. Each interior's, by its editor id
NAVS = collections.defaultdict(list)
for d_ in MESH_DIRS:
    nf_ = os.path.join(d_, 'navmesh.txt.gz')
    if not os.path.exists(nf_): continue
    cur = None
    for l in gzip.open(nf_, 'rt', encoding='utf-8'):
        w_ = l.split()
        if not w_: continue
        if w_[0] == 'N': cur = {'V': [], 'T': []}; NAVS[w_[2] if w_[1] == '-' else w_[1]].append(cur)     # (an interior's by its cell, a world's by the world)
        elif w_[0] == 'v' and cur is not None: cur['V'].append((float(w_[1]), float(w_[2]), float(w_[3])))
        elif w_[0] == 't' and cur is not None: cur['T'].append(tuple(int(v) for v in w_[1:8]))
def nav_collision(cname, origin=(0, 0)):
    floors, walls = [], set()
    for nm in NAVS.get(cname, ()):
        V = [(q[0] - origin[0], q[1] - origin[1], q[2]) for q in nm['V']]
        for a, b, cc, e0, e1, e2, fl in nm['T']:
            floors.append(tuple(int(round(v)) for q in (V[a], V[b], V[cc]) for v in q))
            for (i, j, e, bit) in ((a, b, e0, 1), (b, cc, e1, 2), (cc, a, e2, 4)):
                if e >= 0 or fl & bit: continue
                A, B = V[i], V[j]
                sg = (int(round(A[0])), int(round(A[1])), int(round(B[0])), int(round(B[1])))
                if sg[:2] > sg[2:]: sg = sg[2:] + sg[:2]
                if sg[:2] == sg[2:]: continue
                # (the edge of a floor: in the way only of feet at its height - written top first, so the game knows it
                #  from a wall's: a spiral stair's turn above or below is no wall)
                walls.add(sg + (int(round(max(A[2], B[2]) + 45)), int(round(min(A[2], B[2]) - 45))))
    return floors, sorted(walls)
def cell_collision(c):
    if AREA != 'main' and NAVS.get(c.get('name', '')):
        fl_, wl_ = nav_collision(c['name'], c.get('origin', (0, 0)) if c.get('outside') else (0, 0))
        # (v0.4.74) a hinged door's opening: the navmesh ends there (Skyrim joins the rooms through the door), but the
        # door is the game's now - in the way shut, not open; the edges along its leaf are not walls
        leaves = []
        for (mi_, x_, y_, z_, rz_, sc_, e_, bd_) in c.get('prop_mi', ()):
            if not re.match(r'^Nor(DoorSm|WoodDoor|DoorMedium)', e_) or not bd_: continue
            b_ = [float(v) for v in bd_.split()]
            a_ = math.radians(rz_); ca_, sa_ = math.cos(a_), math.sin(a_); hx_ = (b_[0] + b_[3]) / 2
            P_ = [(x_ + hx_ * ca_ + ly_ * sa_, y_ - hx_ * sa_ + ly_ * ca_) for ly_ in (b_[1] - 20, b_[4] + 20)]
            leaves.append((P_[0], P_[1], z_))
        def on_leaf(w):
            for (A_, B_, z_) in leaves:
                if not (w[5] - 120 <= z_ <= w[4] + 120): continue
                dx_, dy_ = B_[0] - A_[0], B_[1] - A_[1]; L2 = dx_ * dx_ + dy_ * dy_ or 1
                ok = True
                for (px_, py_) in ((w[0], w[1]), (w[2], w[3])):
                    t_ = max(0, min(1, ((px_ - A_[0]) * dx_ + (py_ - A_[1]) * dy_) / L2))
                    if math.hypot(px_ - (A_[0] + dx_ * t_), py_ - (A_[1] + dy_ * t_)) > 40: ok = False
                if ok: return True
                # (v0.4.74) a thick doorway: the navmesh ends on the frame's far side, along the leaf up to ~1.4 m off it
                # (Forelhost03's Refectory door: 66-79 units) - an edge parallel to the leaf, within its span
                ex_, ey_ = w[2] - w[0], w[3] - w[1]; el_ = math.hypot(ex_, ey_) or 1
                if abs(ex_ * dy_ - ey_ * dx_) / (el_ * math.sqrt(L2)) > 0.34: continue       # (more than 20 degrees off)
                ok = True
                for (px_, py_) in ((w[0], w[1]), (w[2], w[3])):
                    t_ = ((px_ - A_[0]) * dx_ + (py_ - A_[1]) * dy_) / L2
                    d_ = abs((px_ - A_[0]) * dy_ - (py_ - A_[1]) * dx_) / math.sqrt(L2)
                    if t_ < -0.1 or t_ > 1.1 or d_ > 100: ok = False
                if ok: return True
            return False
        n0_ = len(wl_)
        wl_ = [w for w in wl_ if not (w[4] > w[5] and on_leaf(w))]
        if n0_ != len(wl_): print('   %s: %d navmesh edges in doors\' openings dropped' % (c['name'], n0_ - len(wl_)))
        return fl_, wl_
    floors, walls = [], set()
    raw = []
    hf_walls = set()
    for mi, x, y, z, rz, sc in c['arch']:
        if mi in c.get('wallonly', ()) or mi in c.get('nofloor', ()): continue
        if mi in c.get('hf', ()):
            # (the barrow's stone: sampled from above, see hf_collision)
            hfl, hwl = hf_collision(mi)
            a = math.radians(rz); ca, sa = math.cos(a), math.sin(a)
            def T(p): return (x + sc * (p[0] * ca + p[1] * sa), y + sc * (-p[0] * sa + p[1] * ca), z + sc * p[2])
            for P in hfl: floors.append(tuple(int(round(v)) for q in map(T, P) for v in q))
            for w in hwl:
                A = T((w[0], w[1], w[4])); B = T((w[2], w[3], w[5]))
                sg = (int(round(A[0])), int(round(A[1])), int(round(B[0])), int(round(B[1])))
                if sg[:2] > sg[2:]: sg = sg[2:] + sg[:2]
                hf_walls.add(sg + (int(round(A[2])), int(round(B[2]))))
            continue
        fp, ff = floor_mesh(mi)
        if os.environ.get('COLLSTAT2'): COLLCNT[mesh_data[mi][1]] += len(ff); COLLCNTW[mesh_data[mi][1]] += len(wall_mesh(mi)[1])
        if os.environ.get('COLLSTAT') and c.get('allheights'): print('   coll floor %4d %4d %s' % (len(ff), len(wall_mesh(mi)[1]), mesh_data[mi][1]))
        a = math.radians(rz); ca, sa = math.cos(a), math.sin(a)
        W = [(x + sc * (p[0] * ca + p[1] * sa), y + sc * (-p[0] * sa + p[1] * ca), z + sc * p[2]) for p in fp]
        for f in ff:
            if c.get('outside') and not c.get('allheights') and max(fp[f[0]][2], fp[f[1]][2], fp[f[2]][2]) > 300: continue    # roofs are not floors
            raw.append((W[f[0]], W[f[1]], W[f[2]]))
    if True:
        for P in stair_ramps(raw):
            # each grown 6 units out from its middle (no cracks between the pieces)
            cx = sum(q[0] for q in P) / 3; cy = sum(q[1] for q in P) / 3
            Q = []
            for q in P:
                dx, dy = q[0] - cx, q[1] - cy; L = math.hypot(dx, dy) or 1
                Q.append((q[0] + dx / L * 6, q[1] + dy / L * 6, q[2]))
            floors.append(tuple(int(round(v)) for q in Q for v in q))
    walls.update(hf_walls)
    for mi, x, y, z, rz, sc in c['arch']:
        if mi in c.get('hf', ()): continue
        if mi in c.get('wallonly', ()):
            # a fence or a dry-stone wall: one line along its length, as high as it stands
            bb = mesh_data[mi][6]; a = math.radians(rz); ca, sa = math.cos(a), math.sin(a)
            if bb[3] - bb[0] >= bb[4] - bb[1]: P0, P1 = (bb[0], (bb[1] + bb[4]) / 2), (bb[3], (bb[1] + bb[4]) / 2)
            else: P0, P1 = ((bb[0] + bb[3]) / 2, bb[1]), ((bb[0] + bb[3]) / 2, bb[4])
            W = [(x + sc * (p[0] * ca + p[1] * sa), y + sc * (-p[0] * sa + p[1] * ca)) for p in (P0, P1)]
            sg = tuple(int(round(v)) for q in W for v in q)
            if sg[:2] > sg[2:]: sg = sg[2:] + sg[:2]
            walls.add(sg + (int(round(z + sc * bb[2])), int(round(z + sc * bb[5]))))
            continue
        if mi in rope_full:
            # a balcony's handrail: walls along the rope (its tube's lengthwise edges, merged into straight runs, each
            # run a little longer at its ends so none leaves a gap), from the floor (70 under the rope) to its top
            a = math.radians(rz); ca, sa = math.cos(a), math.sin(a)
            segs = []
            for t in rope_full[mi]:
                P = t[:3]
                i_, j_ = max(((0, 1), (1, 2), (0, 2)), key=lambda q: (P[q[0]][0] - P[q[1]][0]) ** 2 + (P[q[0]][1] - P[q[1]][1]) ** 2)
                if (P[i_][0] - P[j_][0]) ** 2 + (P[i_][1] - P[j_][1]) ** 2 < 16: continue
                segs.append((P[i_], P[j_], min(p[2] for p in P), max(p[2] for p in P)))
            # runs: segments grouped by direction (5 degrees) and offset (6 units), each run from its extreme ends
            runs = collections.defaultdict(list)
            for A, B, lo, hi in segs:
                dx, dy = B[0] - A[0], B[1] - A[1]; L = math.hypot(dx, dy)
                ang = math.atan2(dy, dx) % math.pi; ux, uy = math.cos(ang), math.sin(ang)
                off = -A[0] * uy + A[1] * ux
                runs[(round(ang / math.radians(5)), round(off / 6))].append((A, B, lo, hi, ux, uy))
            for k_, rs_ in runs.items():
                ux, uy = rs_[0][4], rs_[0][5]
                ts = [p[0] * ux + p[1] * uy for r_ in rs_ for p in r_[:2]]
                # split where the run has a gap of more than 30 units
                pts_ = sorted(set(round(v) for v in ts))
                spans, s0, last = [], pts_[0], pts_[0]
                for v in pts_[1:]:
                    if v - last > 30: spans.append((s0, last)); s0 = v
                    last = v
                spans.append((s0, last))
                P0 = rs_[0][0]; off_x, off_y = P0[0] - (P0[0] * ux + P0[1] * uy) * ux, P0[1] - (P0[0] * ux + P0[1] * uy) * uy
                lo = min(r_[2] for r_ in rs_); hi = max(r_[3] for r_ in rs_)
                for t0, t1 in spans:
                    E = [(off_x + (t0 - 20) * ux, off_y + (t0 - 20) * uy), (off_x + (t1 + 20) * ux, off_y + (t1 + 20) * uy)]
                    Wp = [(x + sc * (p[0] * ca + p[1] * sa), y + sc * (-p[0] * sa + p[1] * ca)) for p in E]
                    sg = tuple(int(round(v)) for q in Wp for v in q)
                    if sg[:2] > sg[2:]: sg = sg[2:] + sg[:2]
                    walls.add(sg + (int(round(z + sc * (lo - 70))), int(round(z + sc * hi))))
        pts, faces = wall_mesh(mi)
        a = math.radians(rz); ca, sa = math.cos(a), math.sin(a)
        W = [(x + sc * (p[0] * ca + p[1] * sa), y + sc * (-p[0] * sa + p[1] * ca), z + sc * p[2]) for p in pts]
        for f in faces:
            if c.get('outside') and not c.get('allheights') and min(pts[f[0]][2], pts[f[1]][2], pts[f[2]][2]) > 250: continue  # the upper walls: out of reach
            A, B, C = W[f[0]], W[f[1]], W[f[2]]
            e = [B[i] - A[i] for i in range(3)]; g = [C[i] - A[i] for i in range(3)]
            n = (e[1] * g[2] - e[2] * g[1], e[2] * g[0] - e[0] * g[2], e[0] * g[1] - e[1] * g[0])
            L = math.sqrt(sum(v * v for v in n)) or 1
            nz = n[2] / L
            if abs(nz) < 0.35:
                P = (A, B, C)
                i, j = max(((0, 1), (1, 2), (0, 2)), key=lambda q: (P[q[0]][0] - P[q[1]][0]) ** 2 + (P[q[0]][1] - P[q[1]][1]) ** 2)
                if (P[i][0] - P[j][0]) ** 2 + (P[i][1] - P[j][1]) ** 2 < 36: continue
                # along its longest level edge, in pieces of at most 40 units, each with the heights the face has there
                # (a sloping stair side is low at its foot and high at its head, not a wall its whole length)
                ax, ay = P[i][0], P[i][1]; ex, ey = P[j][0] - ax, P[j][1] - ay; L = math.hypot(ex, ey); ux, uy = ex / L, ey / L
                npc = max(1, int(math.ceil(L / 64.0)))
                lo = [1e9] * npc; hi = [-1e9] * npc
                for u in range(9):
                    for v in range(9 - u):
                        a3, b3 = u / 8.0, v / 8.0; c3 = 1 - a3 - b3
                        q = [P[0][k] * a3 + P[1][k] * b3 + P[2][k] * c3 for k in range(3)]
                        sproj = (q[0] - ax) * ux + (q[1] - ay) * uy
                        k = min(npc - 1, max(0, int(sproj / L * npc)))
                        lo[k] = min(lo[k], q[2]); hi[k] = max(hi[k], q[2])
                runs = []                                   # neighbouring pieces of much the same heights become one
                for k in range(npc):
                    if lo[k] > hi[k]: continue
                    if runs and runs[-1][1] == k and abs(runs[-1][2] - lo[k]) < 20 and abs(runs[-1][3] - hi[k]) < 20:
                        r0 = runs[-1]; runs[-1] = (r0[0], k + 1, min(r0[2], lo[k]), max(r0[3], hi[k]))
                    else: runs.append((k, k + 1, lo[k], hi[k]))
                for k0, k1, lk, hk in runs:
                    lo_, hi_ = lk, hk
                    s0, s1 = L * k0 / npc, L * k1 / npc
                    sg = (int(round(ax + ux * s0)), int(round(ay + uy * s0)), int(round(ax + ux * s1)), int(round(ay + uy * s1)))
                    if sg[:2] > sg[2:]: sg = sg[2:] + sg[:2]
                    walls.add(sg + (int(round(lo_)), int(round(hi_))))
    walls.update(c['shut'])                              # a shut secret door: a wall across its opening
    return floors, sorted(walls)

# ---------------------------------------------------------------- writing
# the face normals and texel coordinates as the engine works them out (r3d.cpp FaceNormal, MeshPlanarUV with shift 2),
# so a model can be drawn straight from the program's own data with nothing copied into the 9300's memory
def _ctrunc(a, b): q = abs(a) // abs(b); return q if (a >= 0) == (b > 0) else -q
def face_normal(A, B, C):
    ex, ey, ez = B[0] - A[0], B[1] - A[1], B[2] - A[2]; fx, fy, fz = C[0] - A[0], C[1] - A[1], C[2] - A[2]
    sh = 0; mx = abs(ex) | abs(ey) | abs(ez) | abs(fx) | abs(fy) | abs(fz)
    while (mx >> sh) > 16000: sh += 1
    ex >>= sh; ey >>= sh; ez >>= sh; fx >>= sh; fy >>= sh; fz >>= sh
    nx = ey * fz - ez * fy; ny = ez * fx - ex * fz; nz = ex * fy - ey * fx
    while abs(nx) > 23000 or abs(ny) > 23000 or abs(nz) > 23000: nx >>= 1; ny >>= 1; nz >>= 1
    l = math.isqrt(nx * nx + ny * ny + nz * nz)
    if l == 0: return (0, 0, 0)
    return (_ctrunc(nx * 16384, l), _ctrunc(ny * 16384, l), _ctrunc(nz * 16384, l))
def planar_uv(P, n):
    ax, ay, az = abs(n[0]), abs(n[1]), abs(n[2])
    if ay >= ax and ay >= az: cu, cv = 0, 2
    elif ax >= az: cu, cv = 2, 1
    else: cu, cv = 0, 1
    u = [p[cu] >> 2 for p in P]; v = [-(p[cv] >> 2) for p in P]
    bu, bv = min(u) & ~63, min(v) & ~63
    return [x * 4 for k in range(3) for x in (max(0, min(4095, u[k] - bu)), max(0, min(4095, v[k] - bv)))]     # (v0.4.46: in quarter texels)

def arr(f, typ, name, vals, per):
    f.write('static const %s %s[%d] = {\n' % (typ, name, max(1, len(vals))))
    vals = list(vals) or [0]
    for i in range(0, len(vals), per): f.write('    ' + ', '.join(str(v) for v in vals[i:i + per]) + ',\n')
    f.write('    };\n')
# the 64 textures the most faces use get numbers (a slot's look byte: 0xC0 + the number; its colour stays the
# texture's, for far and with the textures off); the others' faces keep their colours and the engine's mapping
# (v0.4.56) LIB=1: for tools/lib_pack.py (the shared library): more numbered (each area's 128 most used), all with
# 128 and 256 pictures where Skyrim has them; the slots name the number as 0x1000 + n (lib_pack.py gives them their
# bank: the library's or the area's) and the pictures go to kit_texpics.pkl
LIBMODE = bool(os.environ.get('LIB'))
SKYMAX = 100000 if LIBMODE else 64; HIMAX = 100000 if LIBMODE else 40; HI2MAX = 100000 if LIBMODE else 14     # (v0.4.51: 14, all three pelts; v0.4.50: 12 - the Whiterun signs took the pelts' places) (v0.4.46: 40 at 128 x 128, was 32; the first 9 of them at 256 x 256 too)
# (those with a 128 x 128 picture first - the game keeps the first 32 at that size, for faces seen close)
# (the signs first, whatever few faces they have: their lettering read close; v0.4.40)
FLORA = [('Snow', None), ('Thicket', 0), ('YellowShrub', 1), ('TundraShrub', 2), ('DeadShrub', 3), ('Fern', 4),
         ('MountainFlower01Purple', 5), ('MountainFlower02Purple', 5), ('MountainFlower01Blue', 6), ('MountainFlower02Blue', 6),
         ('MountainFlower01Red', 7), ('MountainFlower02Red', 7), ('Lavender', 8), ('Thistle', 9), ('TundraCotton', 10),
         ('Clover', 11), ('Snowberry', 12), ('Cabbage', 13), ('Potato', 14), ('Leek', 15), ('Wheat', 16), ('VineMaple', 17),
         ('FallForestShrub', 19), ('FallForestFern', 19), ('DriftWood', 20)]
# (v0.4.66) Skyrim's own trees where an export has the land's every placement (the Rift: tools/skyrim_area.py riften's
# Tamriel cells round the city): its aspens (Skyrim's model: bark, and its branch cards cut out) and pines, as plants
# of their own kinds, drawn out to the object distance; the land grows none in those cells (tools/patch_land.py)
TREE_KINDS = {21: 'aspen', 22: 'aspen', 23: 'aspen', 24: 'pine', 25: 'pine'}
TREE_RE = [(re.compile(r'^TreeAspen0[123]$'), 21), (re.compile(r'^TreeAspen0[56]$'), 22), (re.compile(r'^TreeAspen04$'), 23),
           (re.compile(r'^TreePineForest(0[1-5]|Dead0[1-5]|0[1-5]Dead)$'), 24), (re.compile(r'^TreePineForest(Snow|SnowL|DeadSnowL)0[1-5]$'), 25)]
def tree_kind(e):
    for rx, k in TREE_RE:
        if rx.match(e): return k
    return None
def _area_cells(d_):
    fa = os.path.join(d_, 'area.txt')
    if not os.path.exists(fa): return None
    m_ = re.search(r'tamriel cells x (-?\d+)\.\.(-?\d+), y (-?\d+)\.\.(-?\d+)', open(fa).read())
    return tuple(int(v) for v in m_.groups()) if m_ else None
def extra_land_refs():
    # the TREE references in Tamriel of every export whose cells lie clear of refs_area.csv's (Whiterun's hold: its
    # plants come from there, its trees are the land's own); each reference once (v0.4.68: the exports overlap -
    # Forelhost's lies within Riften's)
    seen_ = set()
    for d_ in MESH_DIRS:
        c_ = _area_cells(d_)
        if not c_ or not os.path.exists(os.path.join(d_, 'refs.csv.gz')): continue
        if not (c_[0] > 14 or c_[1] < -8 or c_[2] > 4 or c_[3] < -22): continue
        for r_ in csv.DictReader(gzip.open(os.path.join(d_, 'refs.csv.gz'), 'rt', encoding='utf-8')):
            if r_['world'] != 'Tamriel' or r_['base_type'] != 'TREE': continue
            if not ref_on(r_): continue
            if r_['ref_id'] in seen_: continue
            seen_.add(r_['ref_id'])
            yield r_
_EXTRA = None
def extra_refs():
    global _EXTRA
    if _EXTRA is None: _EXTRA = list(extra_land_refs())
    return _EXTRA
def flora_kind(e):
    snowy = 'Snow' in e
    for k, v in FLORA[1:]:
        if k in e:
            if snowy and v in (0, 3, 17): return 18                  # the snowy bushes: white
            return v
    return None
_anym = None
def any_model(e):
    # an editor id's model from any export's bases.csv / models.csv (the plants Riverwood's export did not take)
    global _anym
    if _anym is None:
        _anym = {}
        for d_ in MESH_DIRS:
            fb = os.path.join(d_, 'bases.csv')
            if os.path.exists(fb):
                for r_ in csv.reader(open(fb, encoding='utf-8', errors='replace')):
                    if len(r_) >= 5 and r_[4]: _anym.setdefault(r_[2], r_[4])
    return _anym.get(e)
# (v0.4.42) the bushes and ferns drawn as Skyrim draws them: its leaf cards, cut out - their pictures numbered too
CARD_KINDS = {0, 1, 2, 4, 17, 18, 19}
card_model = {}; CARD_TEX = {}
_fe = collections.defaultdict(collections.Counter)
for r_ in csv.DictReader(open(os.path.join(PLACES, 'refs_area.csv'), encoding='utf-8')):
    e_ = r_['base_editor_id']
    if r_['world'] != 'Tamriel' or r_['base_type'] != 'TREE' or e_.startswith('TreePine') or not e_.startswith('Tree'): continue
    k_ = flora_kind(e_)
    if k_ in CARD_KINDS: _fe[k_][e_] += 1
for r_ in extra_refs():
    e_ = r_['base_editor_id']
    k_ = tree_kind(e_) or flora_kind(e_)
    if k_ in CARD_KINDS or (k_ in TREE_KINDS and TREE_KINDS[k_] == 'aspen'): _fe[k_][e_] += 1
for k_, cnt in _fe.items():
    for e_, _ in cnt.most_common():
        m_ = models.get(e_) or any_model(e_)
        if m_ and os.path.exists(obj_path(m_)):
            tc_ = collections.Counter(t[3] for t in load_obj(obj_path(m_)))
            tx_ = tc_.most_common(1)[0][0]
            if k_ in TREE_KINDS: tx_ = next((t for t in tc_ if 'branch' in t.lower()), tx_)     # (a tree: its leaves' picture)
            if tex_file(tx_): card_model[k_] = m_; CARD_TEX[k_] = tx_
            break
_cards = sorted(set(CARD_TEX.values())); CARD_TEX_L.update(t.lower() for t in _cards)
_sign = [tx for tx, _ in tex_use.most_common() if 'signage' in tx][:8] + _cards
def tex_mag(tx): return math.sqrt(tex_wa[tx] / tex_ua[tx]) if tex_ua[tx] > 0 else 0
def hi2_score(tx): return texsrc.get(tx.lower(), 0) / tex_mag(tx) if tex_mag(tx) else 0
# (v0.4.50) those that will be drawn at 256 (the finest against their size in the world: the pelts) kept among the 64
_fine = sorted([tx for tx, _ in tex_use.most_common(SKYMAX + 24) if 'signage' not in tx and tex_file(tx, '_256.ppm') and tex_file(tx, '_128.ppm')], key=lambda t: -hi2_score(t))[:HI2MAX]
_sign = _sign + [tx for tx in _fine if tx not in _sign]
_top = (_sign + [tx for tx, _ in tex_use.most_common() if tx not in _sign])[:SKYMAX]
if os.environ.get('TEXRANK'):
    for i_, (tx_, c_) in enumerate(tex_use.most_common()[:100]): print('rank %3d %6d %s%s' % (i_, c_, tx_, ' *' if tx_ in _top else ''))
# (v0.4.46) the pictures at 128 (and 256): those whose texels are biggest in the world - seen close they are the most
# blocky (a pelt: its whole picture across a metre and a half) - weighed by how often they are used
def hi_score(tx): return tex_mag(tx) * math.sqrt(tex_use[tx])
_signs = [tx for tx in _top if 'signage' in tx]
_hi = (_signs + [tx for tx in _fine if tx not in _signs] + sorted([tx for tx in _top if tx not in _signs and tx not in _fine], key=lambda t: -hi_score(t)))   # (v0.4.50: the finest kept at 128 too, so they can be at 256)
_hi = [tx for tx in _hi if tex_file(tx, '_128.ppm')][:HIMAX]
# (256 x 256: the signs - their lettering - and then the biggest texels; only those Skyrim made big enough, 2048 or
#  more: the exporter writes _256 for them)
_hi2 = [tx for tx in _hi if tex_file(tx, '_256.ppm')]
# (then by how much finer Skyrim's picture is than its place in the world needs: its texels to a unit of size - the
#  pelts, 4096 across a metre and a half, before the walls' 2048 across two and a half)
_hi2 = ([tx for tx in _hi2 if tx in _signs] + sorted([tx for tx in _hi2 if tx not in _signs], key=lambda t: -hi2_score(t)))[:HI2MAX]
if os.environ.get('HISTAT'):
    for tx in sorted(_hi, key=lambda t: -hi2_score(t)): print('   hi2 %-60s src %5d mag %5.0f score %6.1f%s' % (tx, texsrc.get(tx.lower(), 0), tex_mag(tx), hi2_score(tx), ' 256' if tx in _hi2 else ''))
_hi = _hi2 + [tx for tx in _hi if tx not in _hi2]
if os.environ.get('HISTAT'):
    for tx in sorted(_top, key=lambda t: -hi_score(t)): print('   hi %-60s mag %6.0f uses %5d score %7.0f%s' % (tx, tex_mag(tx), tex_use[tx], hi_score(tx), ' 128' if tx in _hi else ''))
for tx in _hi + [tx for tx in _top if tx not in _hi]: sky_id(tx)
NHI = len(_hi); NHI2 = len(_hi2)
print('pictures at 128: %d, at 256: %d (%s)' % (NHI, NHI2, ', '.join(t.split(chr(92))[-1] for t in _hi2)))
for mi, md in enumerate(mesh_data):
    slots = md[4]
    for j, tx in mesh_stex[mi].items():
        if tx in sky_ix: slots[j] = slots[j][:3] + ('0x%X' % ((0x1000 if LIBMODE else 0xC0) | sky_ix[tx]),)
    for j, tx in enumerate(mesh_order[mi]):
        # a Skyrim texture without a picture here: its own colour, flat, rather than the engine's wood or fur
        if j < len(slots) and 'SL_TEX' in str(slots[j][3]) and not tx.startswith('gen:') and tx.split('#vc')[0] not in sky_ix: slots[j] = slots[j][:3] + ('1',)
    fu = face_uv[mi]
    for fi in list(fu):
        if mesh_stex[mi].get(md[3][fi][3]) not in sky_ix: del fu[fi]
# what Skyrim keeps out of sight: a container wholly under a floor (the merchants' stock chests, under the shop's
# boards): not a thing to find (v0.4.40: the Trader's two showed through its floor)
def floor_heights(c, x, y):
    for st in c['stat']:
        mi = st[0]
        if mi < 0 or mi >= len(mesh_data): continue
        pts, faces = mesh_data[mi][2], mesh_data[mi][3]
        a = math.radians(st[4]); ca, sa = math.cos(a), math.sin(a); sc = st[5] / 256.0
        for f_ in faces:
            P = [(st[1] + sc * (pts[k][0] * ca + pts[k][1] * sa), st[2] + sc * (-pts[k][0] * sa + pts[k][1] * ca), st[3] + sc * pts[k][2]) for k in f_[:3]]
            if min(q[0] for q in P) > x or max(q[0] for q in P) < x or min(q[1] for q in P) > y or max(q[1] for q in P) < y: continue
            n_ = _cross(_sub(P[1], P[0]), _sub(P[2], P[0])); L = math.sqrt(_dot(n_, n_)) or 1
            if abs(n_[2]) / L < 0.6: continue
            d = (P[1][1] - P[2][1]) * (P[0][0] - P[2][0]) + (P[2][0] - P[1][0]) * (P[0][1] - P[2][1])
            if abs(d) < 1e-6: continue
            l1 = ((P[1][1] - P[2][1]) * (x - P[2][0]) + (P[2][0] - P[1][0]) * (y - P[2][1])) / d
            l2 = ((P[2][1] - P[0][1]) * (x - P[2][0]) + (P[0][0] - P[2][0]) * (y - P[2][1])) / d
            if min(l1, l2, 1 - l1 - l2) < 0: continue
            yield l1 * P[0][2] + l2 * P[1][2] + (1 - l1 - l2) * P[2][2]
def walk_heights(fl, x, y):
    for f_ in fl:
        P = (f_[0:3], f_[3:6], f_[6:9])
        if min(q[0] for q in P) > x or max(q[0] for q in P) < x or min(q[1] for q in P) > y or max(q[1] for q in P) < y: continue
        d = (P[1][1] - P[2][1]) * (P[0][0] - P[2][0]) + (P[2][0] - P[1][0]) * (P[0][1] - P[2][1])
        if abs(d) < 1e-6: continue
        l1 = ((P[1][1] - P[2][1]) * (x - P[2][0]) + (P[2][0] - P[1][0]) * (y - P[2][1])) / d
        l2 = ((P[2][1] - P[0][1]) * (x - P[2][0]) + (P[0][0] - P[2][0]) * (y - P[2][1])) / d
        if min(l1, l2, 1 - l1 - l2) < -0.05: continue
        yield l1 * P[0][2] + l2 * P[1][2] + (1 - l1 - l2) * P[2][2]
def hidden_under(c, x, y, z, ztop):
    # nothing to stand on at its base (the cell's walkable floors, or the pieces' own), and a floor over it
    if 'walkfl' not in c: c['walkfl'] = cell_collision(c)[0]
    hs = list(floor_heights(c, x, y)) + list(walk_heights(c['walkfl'], x, y))
    return not any(z - 20 <= h <= z + 20 for h in hs) and any(ztop - 5 < h < ztop + 110 for h in hs)
for cname, c in cells.items():
    if c.get('outside') or not c.get('prop_mi'): continue
    keep = []; keepm = []
    for pr, pm in zip(c['prop'], c['prop_mi']):
        if pr[0] in ('PT_CHEST', 'PT_BARREL', 'PT_SACK', 'PT_DRESSER', 'PT_CUPBOARD') and len(pm) > 7:
            b = [float(v) for v in pm[7].split()] if pm[7] else [0, 0, 0, 0, 0, 60]
            if b[5] <= b[2]: b = [0, 0, 0, 0, 0, 60]
            if pm[6].startswith('Merchant') or hidden_under(c, pm[1], pm[2], pm[3] + b[2] * pm[5], pm[3] + b[5] * pm[5]):
                c['dropped']['%s %s (under the floor: out of sight)' % (pr[0], pm[6])] += 1; continue
        keep.append(pr); keepm.append(pm)
    c['prop'], c['prop_mi'] = keep, keepm
print('Skyrim textures: %d used, %d numbered (%d faces of %d)' % (len(tex_use), len(SKYTEX), sum(c for t, c in tex_use.items() if t in sky_ix), sum(tex_use.values())))
open(os.path.join(os.path.dirname(OUT), "kit_tex_names.txt"), "w").write("\n".join(SKYTEX) + "\n")
# (v0.4.43) a big model's faces in clusters of 32 for the game (R3D::MeshClusters): grouped by the way they face (six
# ways), then split in space (halves along the longest side, at a multiple of 32) - the game leaves out a cluster off
# the screen or turned away as a whole. The vertices then numbered as the faces first use them (each cluster's few)
CLS = 16
def cluster_order(EV, faces, NN):
    n = len(faces)
    if n < 2 * CLS: return list(range(n))
    cen = [tuple((EV[a][k] + EV[b][k] + EV[c][k]) / 3.0 for k in range(3)) for a, b, c, s in faces]
    def way(nv):
        k = max(range(3), key=lambda j: abs(nv[j]))
        return k * 2 + (nv[k] < 0)
    buckets = [[] for _ in range(6)]
    for i in range(n): buckets[way(NN[i])].append(i)
    out = []
    def split(ids):
        if len(ids) <= CLS: out.extend(ids); return
        ext = [max(cen[i][k] for i in ids) - min(cen[i][k] for i in ids) for k in range(3)]
        k = max(range(3), key=lambda j: ext[j])
        ids = sorted(ids, key=lambda i: cen[i][k])
        cut = CLS * ((len(ids) + 2 * CLS - 1) // (2 * CLS))
        split(ids[:cut]); split(ids[cut:])
    # (the fullest first; a part-cluster where one way ends joins the next way's start)
    for b in sorted(buckets, key=lambda b: -len(b)): split(b)
    return out
def cluster_mesh(pts, faces, fu, EV, NN):
    perm = cluster_order(EV, faces, NN)
    if perm == list(range(len(faces))): return pts, faces, fu, EV, NN
    faces = [faces[i] for i in perm]; NN = [NN[i] for i in perm]
    inv = {o: i for i, o in enumerate(perm)}
    fu = {inv[o]: v for o, v in fu.items()}
    vmap = {}
    for a, b, c, s_ in faces:
        for x in (a, c, b):
            if x not in vmap: vmap[x] = len(vmap)
    for i in range(len(pts)):
        if i not in vmap: vmap[i] = len(vmap)
    npts = [None] * len(pts); nEV = [None] * len(pts)
    for o, i in vmap.items(): npts[i] = pts[o]; nEV[i] = EV[o]
    faces = [(vmap[a], vmap[b], vmap[c], s_) for a, b, c, s_ in faces]
    return npts, faces, fu, nEV, NN
with open(OUT, 'w') as f:
    f.write('// generated by docs/maps/gen_kit.py from Skyrim.esm and its meshes - do not edit by hand\n')
    f.write('// models: vertices in engine units (x east, y up, z north), faces a, b, c, slot; slots r, g, b, look\n')
    tv = tf = 0
    for mi, (model, e, pts, faces, slots, nraw, bb, spin) in enumerate(mesh_data):
        f.write('// %d: %s (%s): %d triangles from %d\n' % (mi, model, e, len(faces), nraw))
        EV = [tuple(int(round(v * K)) for v in (p[0], p[2], p[1])) for p in pts]
        NN = [face_normal(EV[a], EV[c], EV[b]) for a, b, c, s in faces]
        fu = face_uv[mi]
        if not spin: pts, faces, fu, EV, NN = cluster_mesh(pts, faces, fu, EV, NN)
        arr(f, 'short', 'KKitV%d' % mi, [v for p in EV for v in p], 12)
        arr(f, 'unsigned short', 'KKitF%d' % mi, [v for a, b, c, s in faces for v in (a, c, b, s)], 16)
        arr(f, 'short', 'KKitN%d' % mi, [v for n in NN for v in n], 12)
        arr(f, 'unsigned short', 'KKitT%d' % mi, [v for fi, ((a, b, c, s), n) in enumerate(zip(faces, NN)) for v in (fu[fi] if fi in fu else planar_uv((EV[a], EV[c], EV[b]), n))], 12)
        tv += len(pts); tf += len(faces)
    f.write('static const KitMesh KKitMesh[%d] = {\n' % len(mesh_data))
    for mi, (model, e, pts, faces, slots, nraw, bb, spin) in enumerate(mesh_data):
        sl = ', '.join('%d, %d, %d, %s' % s for s in slots) + ', 0, 0, 0, 0' * (8 - len(slots))
        big = model.lower().startswith('architecture')
        hx = int((bb[3] - bb[0]) / 2 * K) if not big else 0; hz = int((bb[4] - bb[1]) / 2 * K) if not big else 0
        hy = int(bb[5] * K) if not big else 0
        if hy < 20 * K or max(hx, hz) < 18 * K: hx = hz = hy = 0                      # small things do not block
        if 'tabletwobenches' in model.lower() or 'tableonebench' in model.lower(): hz = hz * 55 // 100   # (the table only: the benches are seats)
        bx = int((bb[3] + bb[0]) / 2 * K) if hx else 0; bz = int((bb[4] + bb[1]) / 2 * K) if hx else 0   # the box's middle
        if mi in outside_models or mi in water_models or spin: hx = hz = hy = bx = bz = 0     # their own walls, or none
        if mi in tree_models: hx = hz = int(28 * K); hy = bx = bz = 0                     # a tree: its trunk
        if mi in mesh_wall_models: hx = hz = hy = bx = bz = 0
        # (v0.4.56: kept in navmesh areas too - the game leaves them out there, GameKitNav - so a piece is the same in every area)
        sp = spin or (0, 0)
        f.write('    { KKitV%d, %d, KKitF%d, %d, KKitN%d, KKitT%d, { %s }, %d, %d, %d, %d, %d, %d, %d, %d, %d },\n' % (mi, len(pts), mi, len(faces), mi, mi, sl, hx, hz, hy, bx, bz, (lod_of[mi] | ((mid_of[mi] + 1) << 16 if mi in mid_of else 0)) if mi in lod_of else -1,
                1 if spin else 0, sp[0], sp[1]))
    _mk = int(re.search(r'MAXKIT = (\d+)', open(os.path.join(SRCDIR, '..', 'inc', 'game.h')).read()).group(1))
    print('models: %d (the game holds %d)' % (len(mesh_data), _mk))
    assert len(mesh_data) <= _mk, 'more models than the game holds (MAXKIT in game.h)'
    f.write('    };\nenum { KKitMesh_N = %d };\n' % len(mesh_data))
    for cname, c in cells.items():
        n = cname
        floors, walls = cell_collision(c)
        # flat things laid on a floor (Skyrim's floor pelts and rugs: every corner at the one height, a third of a
        # unit over the boards) lifted 2.5 units: in the plane of the floor its depth and the floor's are the same to
        # the engine's 16 bits, and which of them shows changed as one moved (rugs flickering through the boards)
        def lift(s):
            bb = mesh_data[s[0]][6]
            if bb[5] - bb[2] < 3 and bb[3] - bb[0] > 40 and bb[4] - bb[1] > 40: return s[:3] + (s[3] + 2.5,) + s[4:]
            return s
        arr(f, 'int', 'K%sStat' % n, [int(round(v)) for s in map(lift, c['stat']) for v in s], 7)
        arr(f, 'int', 'K%sProp' % n, [v if isinstance(v, str) else int(round(v)) for s in c['prop'] for v in s], 6)
        arr(f, 'int', 'K%sItem' % n, [v if isinstance(v, str) else int(round(v)) for s in c['item'] for v in s], 4)
        arr(f, 'int', 'K%sFlame' % n, [int(round(v)) for s in c['flame'] for v in s], 4)
        arr(f, 'int', 'K%sLight' % n, [int(round(v)) for s in c['light'] for v in s], 4)
        arr(f, 'int', 'K%sPeople' % n, [int(round(v)) for s in c['people'] for v in s], 5)
        arr(f, 'short', 'K%sFloor' % n, [v for t in floors for v in t], 9)
        arr(f, 'short', 'K%sWall' % n, [v for w in walls for v in w], 6)
        d = c['door'] or ((tuple(c['notree']) + (0,) * 5) if c.get('notree') else (0,) * 9)
        arr(f, 'int', 'K%sDoor' % n, [int(round(v)) for v in d], 9)
        # the 64-unit squares, each with the floors (grown by 12: the feet are looked for a little around) and the walls
        # (grown by 40: a body's radius) that touch it
        G = 256 if c.get('outside') else 128
        xs = [v for t in floors for v in t[0::3]] + [v for w in walls for v in (w[0], w[2])] or [0]
        ys = [v for t in floors for v in t[1::3]] + [v for w in walls for v in (w[1], w[3])] or [0]
        gx0, gy0 = min(xs) - 64, min(ys) - 64
        gw, gh = (max(xs) + 64 - gx0) // G + 1, (max(ys) + 64 - gy0) // G + 1
        fl = [[] for _ in range(gw * gh)]; wl = [[] for _ in range(gw * gh)]
        def put(lst, i, x0, y0, x1, y1):
            for gy in range(max(0, (y0 - gy0) // G), min(gh - 1, (y1 - gy0) // G) + 1):
                for gx in range(max(0, (x0 - gx0) // G), min(gw - 1, (x1 - gx0) // G) + 1): lst[gy * gw + gx].append(i)
        for i, t in enumerate(floors): put(fl, i, min(t[0::3]) - 12, min(t[1::3]) - 12, max(t[0::3]) + 12, max(t[1::3]) + 12)
        for i, w in enumerate(walls): put(wl, i, min(w[0], w[2]) - 40, min(w[1], w[3]) - 40, max(w[0], w[2]) + 40, max(w[1], w[3]) + 40)
        lst, fst, wst = [], [], []
        for k in range(gw * gh): fst.append(len(lst)); lst.extend(fl[k])
        fst.append(len(lst))
        for k in range(gw * gh): wst.append(len(lst)); lst.extend(wl[k])
        wst.append(len(lst))
        print('   %s: floors %d, walls %d' % (n, len(floors), len(walls)))
        if os.environ.get('COLLSTAT2'): print('   by model:', COLLCNT.most_common(25)); print('   walls by model:', COLLCNTW.most_common(25))
        assert len(floors) < 65536 and len(walls) < 65536
        arr(f, 'int', 'K%sGrid' % n, [gx0, gy0, gw, gh, G.bit_length() - 1] + fst + wst, 16)
        arr(f, 'unsigned short', 'K%sGList' % n, lst, 20)
        c['counts'] = (len(c['stat']), len(c['prop']), len(c['item']), len(c['flame']), len(c['light']), len(c['people']), len(floors), len(walls))
    f.write('static const KitCell KKitCell[%d] = {\n' % len(cells))
    for cname, c in cells.items():
        n = cname
        o = c.get('origin', (OUT_OX, OUT_OY)) if c.get('outside') else (0, 0)
        f.write('    { %s, "%s", K%sStat, %d, K%sProp, %d, K%sItem, %d, K%sFlame, %d, K%sLight, %d, K%sPeople, %d, K%sFloor, %d, K%sWall, %d, K%sDoor, K%sGrid, K%sGList, %d, %d },\n'
                % ((c['id'], NAMES.get(n, n).replace('"', '')) + tuple(v for pair in zip([n] * 8, c['counts']) for v in pair) + (n, n, n) + o))
    f.write('    };\nenum { KKitCell_N = %d };\n' % len(cells))
print('models %d: %d vertices, %d triangles' % (len(mesh_data), tv, tf))

# the pictures: one palette of 256 colours for them all (made brighter as the slots' colours are: the engine lights
# more darkly than Skyrim), each picture 64 x 64 bytes on it; and each one's average colour (drawn from afar)
from PIL import Image
TEXOUT = os.path.join(os.path.dirname(OUT), 'kit_tex.bin')
def tex_picture(tx, side):
    suf = '' if side == 64 else '_%d' % side
    im = Image.open(tex_file(tx, suf + '.ppm')).convert('RGB')
    if alpha_info(tx):
        # the transparent texels in the opaque ones' average (the engine draws the face whole)
        af = tex_file(tx, suf + '_a.pgm') or tex_file(tx, '_a.pgm')
        al = Image.open(af).convert('L').resize(im.size)
        im = Image.composite(im, Image.new('RGB', im.size, alpha_info(tx)[1]), al.point(lambda v: 255 if v >= 128 else 0))
    if im.size != (side, side): im = im.resize((side, side), Image.BILINEAR)
    tt_ = tex_tint(tx)
    if tt_:
        # (v0.4.74) Skyrim's vertex colours on it, multiplied in as Skyrim does
        ch = im.split(); im = Image.merge('RGB', [ch[k].point(lambda v, f=tt_[k] / 255.0: int(v * f)) for k in range(3)])
    dark = tx.startswith('textures\\dungeons\\') or tx.startswith('textures\\landscape\\rocks') or tx.startswith('textures\\landscape\\mountains')
    return im.point((lambda v: min(250, int(v * 2.0 + 16))) if dark else (lambda v: min(250, int(v * 1.55 + 8))))
ims = [tex_picture(tx, 64) for tx in SKYTEX]
if ims and LIBMODE:
    # (v0.4.56) every numbered picture as made here (before the palette) for lib_pack.py's banks: all at 64, at 128
    # where Skyrim's is big enough, at 256 the 48 finest (and the signs)
    import pickle
    def keymask_l(tx, side):
        suf = '' if side == 64 else '_%d' % side
        af = tex_file(tx, suf + '_a.pgm') or tex_file(tx, '_a.pgm')
        th = 128
        for m_ in TEX_ALPHA.get(tx.lower(), ()):
            if m_.startswith('test '): th = max(1, int(m_[5:]))
        return Image.open(af).convert('L').resize((side, side), Image.BILINEAR).point(lambda v: 255 if v >= th else 0)
    fine256 = set(sorted([tx for tx in SKYTEX if tex_file(tx, '_256.ppm') and tex_file(tx, '_128.ppm')], key=lambda t: (not 'signage' in t, -hi2_score(t)))[:48])
    pics = {}
    for i, tx in enumerate(SKYTEX):
        k_ = bool(sky_key(tx))
        d_ = {'use': tex_use[tx], 'wa': tex_waw[tx], 'hi': hi_score(tx), 'hi2': hi2_score(tx), 'sign': 'signage' in tx, 'key': k_, '64': ims[i].tobytes()}
        if tex_file(tx, '_128.ppm'): d_['128'] = tex_picture(tx, 128).tobytes()
        if tx in fine256: d_['256'] = tex_picture(tx, 256).tobytes()
        if k_:
            for side in (64, 128, 256):
                if side == 64 or str(side) in d_: d_['mask%d' % side] = keymask_l(tx, side).tobytes()
        pics[tx] = d_
    pickle.dump({'order': list(SKYTEX), 'pics': pics}, open(os.path.join(os.path.dirname(OUT), 'kit_texpics.pkl'), 'wb'))
    print('textures: %d of Skyrim\'s numbered for the library (kit_texpics.pkl)' % len(SKYTEX))
if ims and not LIBMODE:
    sheet = Image.new('RGB', (64, 64 * len(ims)))
    for i, im in enumerate(ims): sheet.paste(im, (0, 64 * i))
    # (255 colours: number 0 is the cut-out pictures' key, where they are cut away)
    q = sheet.quantize(255, method=Image.MEDIANCUT, dither=Image.NONE)
    pal = q.getpalette()[:765]; pal += [0] * (765 - len(pal)); pal = [255, 0, 255] + pal
    def qbytes(im): return bytes(v + 1 for v in im.quantize(palette=q, dither=Image.NONE).tobytes())
    keyed = [sky_key(tx) for tx in SKYTEX]
    def keymask(tx, side):
        suf = '' if side == 64 else '_%d' % side
        af = tex_file(tx, suf + '_a.pgm') or tex_file(tx, '_a.pgm')
        th = 128
        for m_ in TEX_ALPHA.get(tx.lower(), ()):
            if m_.startswith('test '): th = max(1, int(m_[5:]))
        return Image.open(af).convert('L').resize((side, side), Image.BILINEAR).point(lambda v: 255 if v >= th else 0)
    data = bytearray(qbytes(sheet))
    for i, tx in enumerate(SKYTEX):
        if keyed[i]:
            mk = keymask(tx, 64).tobytes()
            for k_ in range(4096):
                if not mk[k_]: data[i * 4096 + k_] = 0
    data = bytes(data)
    # and each blurred (2 x 2 texels averaged, at the same size), for faces seen smaller than their texels
    blur = sheet.resize((32, 32 * len(ims)), Image.BOX).resize((64, 64 * len(ims)), Image.NEAREST)
    data += qbytes(blur)
    out = bytearray(b'HTX3' + struct.pack('<I', len(ims))) + bytes(pal)
    for im in ims:
        px = list(im.getdata()); n_ = len(px)
        out += bytes(sum(p[k] for p in px) // n_ for k in range(3))
    while len(out) % 4: out.append(0)
    out += data
    # (HTX3) the first NHI again at 128 x 128, on the same palette
    out += struct.pack('<I', NHI)
    def hipic(tx, side, i):
        b_ = bytearray(qbytes(tex_picture(tx, side)))
        if keyed[i]:                                      # (v0.4.46: the cut-out ones cut out at this size too)
            mk = keymask(tx, side).tobytes()
            for k_ in range(side * side):
                if not mk[k_]: b_[k_] = 0
        return bytes(b_)
    for i, tx in enumerate(SKYTEX[:NHI]): out += hipic(tx, 128, i)
    # (v0.4.40) "HTXK", u32 n, u8 cut-out (1) each, to 4
    out += b'HTXK' + struct.pack('<I', len(SKYTEX)) + bytes(1 if k_ else 0 for k_ in keyed)
    while len(out) % 4: out.append(0)
    # (v0.4.42) "HTXM", u32 n, then each at 32 x 32 and each at 16 x 16 (averaged), tiled to fill 64 x 64 (the game draws
    # them with the 64's mask, the coordinates halved, quartered): for faces seen small - fewer texels to fetch
    # (measured with a model of the 9300's data cache: 1..4 % fewer misses - the misses are the z buffer's and the
    #  transformed vertices'; not worth their 512 KB: made only with MIPS=1)
    if os.environ.get('MIPS'): out += b'HTXM' + struct.pack('<I', len(SKYTEX))
    for side in ((32, 16) if os.environ.get('MIPS') else ()):
        for im in ims:
            sm = im.resize((side, side), Image.BOX); t_ = Image.new('RGB', (64, 64))
            for yy in range(0, 64, side):
                for xx in range(0, 64, side): t_.paste(sm, (xx, yy))
            out += qbytes(t_)
    # (v0.4.46) "HTXH", u32 n, the first n again at 256 x 256 (the signs, the pelts: seen from a metre)
    while len(out) % 4: out.append(0)
    if NHI2:
        out += b'HTXH' + struct.pack('<I', NHI2)
        for i, tx in enumerate(SKYTEX[:NHI2]): out += hipic(tx, 256, i)
    print('   cut out: %s' % ', '.join(tx.split('\\')[-1] for tx, k_ in zip(SKYTEX, keyed) if k_))
    open(TEXOUT, 'wb').write(out)
open(os.path.join(os.path.dirname(OUT), 'kit_items.txt'), 'w').write(''.join('%s %d\n' % kv for kv in sorted(item_mi.items())))
# ---------------------------------------------------------------- the items' own models (src/kit_items.bin)
# Each item's Skyrim model as made (simplified only past ITEMTRIS triangles), with its own textures (four at most, on
# the area's palette: 64 x 64, blurred, and 128 x 128 when the export has them) - the inventory shows one at a time,
# read from data/items.hit (tools/kit_pack.py: the items' numbers for their names). Each:
#   u8 name length, the name (IT_...), u32 size, then the model:
#   u16 nv, nf; u8 textures, 128s (0/1); u16 0; u8 slots[32] (r, g, b, look: 0xC0 + its texture, or 1);
#   u8 palette[768] (its own 256 colours: the area's are the buildings', poor in an apple's reds);
#   s16 v[nv * 3] (x east, y up, z north; scaled to about 6000 units across - the preview fits it to its box), to 4;
#   u16 f[nf * 4] (a, b, c, slot); s16 n[nf * 3]; u16 uv[nf * 6] (texels: 64 the picture), to 4;
#   the pictures (4096 bytes each), the blurred (4096 each), the 128s (16384 each)
ITEMTRIS = int(os.environ.get('ITEMTRIS', '3000'))       # (v0.4.40: the inventory's frame rate stays over 18 at 2900, the largest)
def item_blob(mdl):
    raw = load_obj(obj_path(mdl))
    raw = [t for t in raw if not any(k in t[3] for k in SKIPTEX)] or raw
    raw = [t for t in raw if t[3] not in ('-', '')] or raw
    if not raw: return None
    use = collections.Counter(t[3] for t in raw)
    order = [tx for tx, _ in use.most_common()]
    texs = [tx for tx in order if sky_ok(tx) and tex_file(tx)][:4]
    slots = []; slot_tex = {}
    for j, tx in enumerate(order[:8]):
        r_, g_, b_ = look(tx, False)[:3]
        if tx in texs: slots.append((r_, g_, b_, 0xC0 | texs.index(tx))); slot_tex[j] = tx
        else: slots.append((r_, g_, b_, 1))
    def slot_of(tx):
        i = order.index(tx)
        if i < 8: return i
        return min(range(min(8, len(order))), key=lambda j: sum((slots[j][k] - look(tx, False)[k]) ** 2 for k in range(3)))
    # stood on its longest side (a shield, a sword, a plate lie flat in Skyrim's model; turning, they showed an edge):
    # the axes turned round (keeping the faces' winding) so the longest is up
    ex3 = [max(p[k] for t in raw for p in t[:3]) - min(p[k] for t in raw for p in t[:3]) for k in range(3)]
    if max(ex3[0], ex3[1]) > 1.3 * ex3[2]:
        rot = (lambda p: (p[1], p[2], p[0])) if ex3[0] >= ex3[1] else (lambda p: (p[2], p[0], p[1]))
        raw = [mk_tri(rot(t[0]), rot(t[1]), rot(t[2]), t[3], uv_of(t)) for t in raw]
    ext = max(max(p[k] for t in raw for p in t[:3]) - min(p[k] for t in raw for p in t[:3]) for k in range(3)) or 1
    S = min(6000.0 / ext, 400.0)
    tris = [(A, B, C, slot_of(tx)) for A, B, C, tx in raw]
    if len(tris) > ITEMTRIS:
        pts, faces = simplify(tris, ITEMTRIS, 1800)
        fuv = uv_transfer(pts, faces, raw, slot_tex) if slot_tex else {}
    else:
        ix = {}; pts = []; faces = []; fuv = {}
        for fi, (t, tr) in enumerate(zip(raw, tris)):
            ids = []
            for P_ in t[:3]:
                if P_ not in ix: ix[P_] = len(pts); pts.append(P_)
                ids.append(ix[P_])
            faces.append((ids[0], ids[1], ids[2], tr[3]))
            if tr[3] in slot_tex and uv_of(t): fuv[fi] = list(uv_of(t))
    if len(pts) > 2000: return None
    EV = [tuple(int(round(v * S)) for v in (p_[0], p_[2], p_[1])) for p_ in pts]
    out = bytearray(struct.pack('<HHBBH', len(EV), len(faces), len(texs), 0, 0))
    for sl in slots + [(0, 0, 0, 0)] * (8 - len(slots)): out += bytes(sl)
    hi = bool(texs) and all(tex_file(tx, '_128.ppm') for tx in texs)
    pics = [tex_picture(tx, 64) for tx in texs]
    pics128 = [tex_picture(tx, 128) for tx in texs] if hi else []
    if pics:
        sheet = Image.new('RGB', (128, 64 * len(pics) + 128 * len(pics128)))
        for i, im in enumerate(pics): sheet.paste(im, (0, 64 * i))
        for i, im in enumerate(pics128): sheet.paste(im, (0, 64 * len(pics) + 128 * i))
        ipal = sheet.quantize(256, method=Image.MEDIANCUT, dither=Image.NONE)
        pl = ipal.getpalette()[:768]; out += bytes(pl + [0] * (768 - len(pl)))
    else: ipal = None; out += bytes(768)
    for v_ in EV: out += struct.pack('<hhh', *v_)
    while len(out) % 4: out.append(0)
    for a, b, c, s_ in faces: out += struct.pack('<HHHH', a, c, b, s_)
    for a, b, c, s_ in faces: out += struct.pack('<hhh', *face_normal(EV[a], EV[c], EV[b]))
    for fi, (a, b, c, s_) in enumerate(faces):
        if fi in fuv: r = fuv[fi]; out += struct.pack('<6H', *sky_texels([r[0], r[2], r[1]]))
        else: out += struct.pack('<6H', *planar_uv((EV[a], EV[c], EV[b]), face_normal(EV[a], EV[c], EV[b])))
    while len(out) % 4: out.append(0)
    if hi: out[5] = 1
    for im in pics: out += im.quantize(palette=ipal, dither=Image.NONE).tobytes()
    for im in pics: out += im.resize((32, 32), Image.BOX).resize((64, 64), Image.NEAREST).quantize(palette=ipal, dither=Image.NONE).tobytes()
    for im in pics128: out += im.quantize(palette=ipal, dither=Image.NONE).tobytes()
    return bytes(out), len(faces), len(texs)
if ims:
    ib = bytearray(); st = []
    for it, mdl in item_models:
        r = item_blob(mdl)
        if not r: continue
        ib += bytes([len(it)]) + it.encode() + struct.pack('<I', len(r[0])) + r[0]
        st.append((it, r[1], r[2], len(r[0])))
    if AREA == 'main': open(os.path.join(os.path.dirname(OUT), 'kit_items.bin'), 'wb').write(ib)
    print('items: %d models, %d triangles at most, %d bytes (src/kit_items.bin)' % (len(st), max(x[1] for x in st), len(ib)))
    for x in st: print('   %-20s %4d triangles, %d textures, %6d bytes' % x)
# ---------------------------------------------------------------- the land's plants (src/kit_flora.bin)
# Every bush, fern, flower and crop Skyrim places in the exported area (its TREE records that are not trees): a kind
# each (the game draws its own small models of them, near; the flowers and crops can be picked), where it stands
# (an eighth of a metre), turned and sized. In 32 m squares, so the game finds those near the player at once.
#   "HFL1", u32 n, i32 ox, oz (engine units: the area's south-west corner), i32 square (engine units), u32 gw, gh,
#   u16 first[gw * gh + 1] (to a multiple of 4), then n * (u16 x, u16 z (1/8 m from ox, oz), u8 kind, u8 turn * 64 + size)
FK_N = 26
BUSH_KINDS = {1, 2, 17, 19}         # leafy bushes: lobes (the vine maple ...)
REED_KINDS = {0, 18}                # (v0.4.42) Skyrim's thickets are clumps of tall dry grass stalks, not bushes: blades
flora_eds = collections.defaultdict(collections.Counter)
def tex_opaque_avg(tx):
    f = tex_file(tx.lower())
    if not f: return None
    from PIL import Image
    im = Image.open(f).convert('RGB'); px = list(im.getdata())
    fa = tex_file(tx.lower(), '_a.pgm')
    if fa:
        al = list(Image.open(fa).convert('L').resize(im.size).getdata())
        px = [c for c, a in zip(px, al) if a >= 128] or px
    return tuple(sum(c[k] for c in px) // len(px) for k in range(3))
def flora_models(fl):
    import random
    out = bytearray(b'HFM2' + struct.pack('<I', FK_N))
    S = 3.6576 / 2.5
    for k in range(FK_N):
        col = (0,) * 6; V = []; F = []; VN = []; V2 = []; F2 = []; UV = []; UV2 = []; tid = 255
        # (the commonest of the kind whose model some export has: its record's model from any export's lists)
        e = m = None
        for e_, _ in flora_eds[k].most_common():
            m_ = models.get(e_) or any_model(e_)
            if m_ and os.path.exists(obj_path(m_)): e, m = e_, m_; break
        if e is None and flora_eds[k]: e = flora_eds[k].most_common(1)[0][0]
        raw = load_obj(obj_path(m)) if m else []
        if raw:
            tx = collections.Counter(t[3] for t in raw).most_common(1)[0][0]
            c = tex_opaque_avg(tx) if k in BUSH_KINDS | REED_KINDS | {3, 4, 11} else None     # (the flowers' heads and the crops: the game's colours)
            if c:
                br = [min(250, int(v * 1.55 + 8)) for v in c]
                if k in BUSH_KINDS:
                    # (a third of the way to grey: brightened for the engine's light, Skyrim's red-brown tundra shrubs
                    #  stood out as red balls; its leaves seen through, they look duller there)
                    mn_ = sum(br) / 3; br = [int(mn_ + 0.66 * (v - mn_)) for v in br]
                col = tuple(br) + tuple(v * 7 // 10 for v in br)
        if k in TREE_KINDS and (raw or TREE_KINDS[k] == 'pine'):
            # (v0.4.66) a tree of the Rift's: Skyrim's aspen - its bark simplified (slot 1, the bark's colour) and its
            # branch cards cut out (slot 0, both sides) - 110 / 14 triangles near, 14 / 3 far; a pine as the land's own
            # pines are drawn (gen_pine: tiers of needles, slot 0, over a trunk, slot 1)
            if TREE_KINDS[k] == 'aspen' and CARD_TEX.get(k) in sky_ix:
                tx = CARD_TEX[k]; tid = sky_ix[tx]
                lv = [t for t in raw if t[3] == tx]; bk = [t for t in raw if t[3] != tx]
                c1 = tex_opaque_avg(tx) or (200, 150, 60); c2 = tex_opaque_avg(bk[0][3]) if bk else (180, 176, 168)
                col = tuple(min(250, int(v * 1.4 + 8)) for v in c1) + tuple(min(250, int(v * 1.3 + 8)) for v in (c2 or (180, 176, 168)))
                fl_ = 2
                parts = []
                for (nl, nb), VV, FF, UU in (((110, 14), V, F, UV), ((5, 2), V2, F2, UV2)):
                    for src_, want_, slot_ in ((lv, nl, 0), (bk, nb, 1)):
                        if not src_: continue
                        tris_ = [(t[0], t[1], t[2], 0) for t in src_]
                        pts_, faces_ = simplify(tris_, min(want_, len(tris_)), 480)
                        fuv_ = uv_transfer(pts_, faces_, src_, {0: tx}) if slot_ == 0 else {}
                        b0 = len(VV); VV += list(pts_)
                        for fi, (a_, b_, c_, s_) in enumerate(faces_):
                            if slot_ == 0:
                                uvs = sky_texels([fuv_[fi][0], fuv_[fi][2], fuv_[fi][1]]) if fi in fuv_ else [0, 0, 256, 0, 0, 256]
                                FF.append((b0 + a_, b0 + b_, b0 + c_, 0)); UU.append(uvs)
                                FF.append((b0 + a_, b0 + c_, b0 + b_, 0)); UU.append([uvs[0], uvs[1], uvs[4], uvs[5], uvs[2], uvs[3]])
                            else:
                                FF.append((b0 + a_, b0 + b_, b0 + c_, 1)); UU.append([0, 0, 0, 0, 0, 0])
            else:
                tid = 255; fl_ = 0
                snow = k == 25
                col = ((196, 202, 210) if snow else (40, 74, 42)) + (92, 64, 40)
                pm = (m or 'landscape\\trees\\treepineforest03.nif').lower()
                for far_, VV, FF in ((0, V, F), (2, V2, F2)):
                    tris_ = gen_pine(pm, far_)
                    for A, B, C, tx_ in tris_:
                        b0 = len(VV); VV += [A, B, C]
                        FF.append((b0, b0 + 1, b0 + 2, 1 if 'bark' in tx_ else 0))
            out += bytes(col) + bytes([tid if tid < 255 else 255, fl_]) + struct.pack('<HH', len(V), len(F))
            for VV, FF, UU in ((V, F, UV), (V2, F2, UV2)):
                EV = [tuple(int(round(v * S)) for v in (p_[0], p_[2], p_[1])) for p_ in VV]
                if VV is V2: out += struct.pack('<HH', len(V2), len(F2))
                for v_ in EV: out += struct.pack('<hhh', *v_)
                while len(out) % 4: out.append(0)
                for a, b, c_, s_ in FF: out += struct.pack('<HHHH', a, c_, b, s_)
                for a, b, c_, s_ in FF:
                    if fl_ == 2 and s_ == 0: out += struct.pack('<hhh', 0, 16384, 0)              # (cards: lit from above)
                    else: out += struct.pack('<hhh', *face_normal(EV[a], EV[c_], EV[b]))
                if fl_ & 2:
                    for u_ in UU: out += struct.pack('<6H', *u_)
                while len(out) % 4: out.append(0)
            print('   plant %2d %-28s %s: a tree, %s, %d / %d triangles' % (k, e or '-', (m or '-').split('\\')[-1], 'picture %d' % tid if tid != 255 else 'plain', len(F), len(F2)))
            continue
        if raw and k in CARD_KINDS and CARD_TEX.get(k) in sky_ix:
            # (v0.4.42) Skyrim's own leaf cards, cut out, simplified (48 triangles near, 14 far), seen from both sides,
            # lit as from above (as Skyrim's foliage is: evenly)
            tx = CARD_TEX[k]; tid = sky_ix[tx]
            cr = [t for t in raw if t[3] == tx] or raw
            tris_ = [(t[0], t[1], t[2], 0) for t in cr]
            for want_, VV, FF, UU in ((48, V, F, UV), (14, V2, F2, UV2)):
                pts_, faces_ = simplify(tris_, min(want_, len(tris_)), 400)
                fuv_ = uv_transfer(pts_, faces_, cr, {0: tx})
                b0 = len(VV); VV += list(pts_)
                for fi, (a_, b_, c_, s_) in enumerate(faces_):
                    uvs = sky_texels([fuv_[fi][0], fuv_[fi][2], fuv_[fi][1]]) if fi in fuv_ else [0, 0, 256, 0, 0, 256]
                    FF.append((b0 + a_, b0 + b_, b0 + c_, 0)); UU.append(uvs)
                    FF.append((b0 + a_, b0 + c_, b0 + b_, 0)); UU.append([uvs[0], uvs[1], uvs[4], uvs[5], uvs[2], uvs[3]])
            out += bytes(col) + bytes([tid, 2]) + struct.pack('<HH', len(V), len(F))
            EV = [tuple(int(round(v * S)) for v in (p_[0], p_[2], p_[1])) for p_ in V]
            for v_ in EV: out += struct.pack('<hhh', *v_)
            while len(out) % 4: out.append(0)
            for a, b, c_, s_ in F: out += struct.pack('<HHHH', a, c_, b, s_)
            for f_ in F: out += struct.pack('<hhh', 0, 16384, 0)            # (lit from above, both sides)
            for u_ in UV: out += struct.pack('<6H', *u_)
            while len(out) % 4: out.append(0)
            EV = [tuple(int(round(v * S)) for v in (p_[0], p_[2], p_[1])) for p_ in V2]
            out += struct.pack('<HH', len(V2), len(F2))
            for v_ in EV: out += struct.pack('<hhh', *v_)
            while len(out) % 4: out.append(0)
            for a, b, c_, s_ in F2: out += struct.pack('<HHHH', a, c_, b, s_)
            for f_ in F2: out += struct.pack('<hhh', 0, 16384, 0)
            for u_ in UV2: out += struct.pack('<6H', *u_)
            while len(out) % 4: out.append(0)
            print('   plant %2d %-28s %s: cut-out cards, picture %d, %d / %d triangles' % (k, e or '-', (m or '-').split('\\')[-1], tid, len(F), len(F2)))
            continue
        if raw and k in REED_KINDS:
            # a clump of stalks: blades from round its middle, leaning out, as tall as Skyrim's (its cards' tops), each
            # a thin triangle seen from both sides (slot 0 the paler tips' colour)
            P = [p_ for t in raw for p_ in t[:3]]
            rnd = random.Random(k * 7919)
            H = max(p_[2] for p_ in P); R = max(math.hypot(p_[0], p_[1]) for p_ in P)
            for j in range(30):
                a = rnd.random() * 2 * math.pi; r0 = R * 0.25 * rnd.random()
                bx, by = r0 * math.cos(a), r0 * math.sin(a)
                lean = R * (0.12 + 0.4 * rnd.random()); h = H * (0.55 + 0.45 * rnd.random())
                tx_, ty_ = bx + lean * math.cos(a), by + lean * math.sin(a)
                w = max(1.5, R * 0.05); pa = a + math.pi / 2
                b0 = len(V)
                V += [(bx - w * math.cos(pa), by - w * math.sin(pa), 0), (bx + w * math.cos(pa), by + w * math.sin(pa), 0), (tx_, ty_, h)]
                for q in range(3): VN.append((math.cos(a) * 0.3, math.sin(a) * 0.3, 0.95))
                F.append((b0, b0 + 1, b0 + 2, 0 if j % 3 else 1)); F.append((b0, b0 + 2, b0 + 1, 0 if j % 3 else 1))
            # (far: a third of them)
            V2 = V[:30]; F2 = [f_ for f_ in F[:20]]
        if raw and k in BUSH_KINDS:
            P = [p_ for t in raw for p_ in t[:3]]
            L = max(6, min(9, int(round(len(raw) / 8))))
            leaves = []
            rnd = random.Random(k * 7919)
            cen = [P[0]]
            while len(cen) < L: cen.append(max(P, key=lambda q: min((q[0] - c_[0]) ** 2 + (q[1] - c_[1]) ** 2 + (q[2] - c_[2]) ** 2 for c_ in cen)))
            for it_ in range(12):
                grp = [[] for _ in cen]
                for q in P: grp[min(range(len(cen)), key=lambda j: (q[0] - cen[j][0]) ** 2 + (q[1] - cen[j][1]) ** 2 + (q[2] - cen[j][2]) ** 2)].append(q)
                cen = [tuple(sum(q[d] for q in g) / len(g) for d in range(3)) if g else cen[j] for j, g in enumerate(grp)]
            zs = sorted(c_[2] for c_ in cen); zmid = zs[len(zs) // 2]
            # each lobe a rounded lump: an icosahedron stretched to the lobe and pushed in and out a little, smooth-shaded
            # (normals from its middle); the lower ones darker
            t_ = (1 + 5 ** 0.5) / 2
            IV = [(-1, t_, 0), (1, t_, 0), (-1, -t_, 0), (1, -t_, 0), (0, -1, t_), (0, 1, t_), (0, -1, -t_), (0, 1, -t_),
                  (t_, 0, -1), (t_, 0, 1), (-t_, 0, -1), (-t_, 0, 1)]
            IV = [tuple(c_ / math.sqrt(1 + t_ * t_) for c_ in v_) for v_ in IV]
            IF = [(0, 11, 5), (0, 5, 1), (0, 1, 7), (0, 7, 10), (0, 10, 11), (1, 5, 9), (5, 11, 4), (11, 10, 2), (10, 7, 6),
                  (7, 1, 8), (3, 9, 4), (3, 4, 2), (3, 2, 6), (3, 6, 8), (3, 8, 9), (4, 9, 5), (2, 4, 11), (6, 2, 10), (8, 6, 7), (9, 8, 1)]
            for j, g in enumerate(grp):
                if len(g) < 3: continue
                cx, cy, cz = cen[j]
                rx = max(10, 1.05 * max(abs(q[0] - cx) for q in g)); ry = max(10, 1.05 * max(abs(q[1] - cy) for q in g))
                rz = max(10, 0.9 * max(abs(q[2] - cz) for q in g))
                if cz - rz * 0.7 < 0: rz = max(10, cz / 0.7 + 4)
                b0 = len(V)
                for v_ in IV:
                    # (pushed in and out a good deal - clumps of leaves, not a ball - and its light speckled as leaves')
                    f_ = 0.65 + 0.6 * rnd.random()
                    V.append((cx + rx * f_ * v_[0], cy + ry * f_ * v_[1], cz + rz * f_ * v_[2] * (0.7 if v_[2] < 0 else 1.0)))
                    n_ = [v_[d] + 0.7 * (rnd.random() - 0.5) for d in range(3)]; ln = math.sqrt(sum(q * q for q in n_)) or 1
                    VN.append(tuple(q / ln for q in n_))
                sl = 0 if cz >= zmid else 1
                # (its far form: an octahedron, flat-shaded)
                b1 = len(V2)
                for v_ in ((1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -0.7)):
                    V2.append((cx + rx * v_[0], cy + ry * v_[1], cz + rz * v_[2]))
                for A_, B_, C_ in ((0, 2, 4), (2, 1, 4), (1, 3, 4), (3, 0, 4), (2, 0, 5), (1, 2, 5), (3, 1, 5), (0, 3, 5)):
                    P_ = (V2[b1 + A_], V2[b1 + B_], V2[b1 + C_])
                    n_ = _cross(_sub(P_[1], P_[0]), _sub(P_[2], P_[0])); m_ = [sum(q[d] for q in P_) / 3 - (cx, cy, cz)[d] for d in range(3)]
                    F2.append((b1 + A_, b1 + B_, b1 + C_, sl) if _dot(n_, m_) > 0 else (b1 + A_, b1 + C_, b1 + B_, sl))
                for A_, B_, C_ in IF:
                    # (winding: outward, as box_tris turns its faces) - the lobe its darker core (slot 1)
                    P_ = (V[b0 + A_], V[b0 + B_], V[b0 + C_])
                    n_ = _cross(_sub(P_[1], P_[0]), _sub(P_[2], P_[0])); m_ = [sum(q[d] for q in P_) / 3 - (cx, cy, cz)[d] for d in range(3)]
                    F.append((b0 + A_, b0 + B_, b0 + C_, 1) if _dot(n_, m_) > 0 else (b0 + A_, b0 + C_, b0 + B_, 1))
                # leaves on it: small triangles standing out from its surface, every way turned, seen from both sides
                for q_ in range(9):
                    u = rnd.uniform(-1, 1); th = rnd.uniform(0, 2 * math.pi); w_ = math.sqrt(1 - u * u)
                    d_ = (w_ * math.cos(th), w_ * math.sin(th), abs(u) * 0.9 + 0.1 if rnd.random() < 0.75 else u)
                    o_ = (cx + rx * 1.02 * d_[0], cy + ry * 1.02 * d_[1], cz + rz * 1.02 * d_[2])
                    sz = 0.55 * min(rx, ry, rz) * (0.7 + 0.6 * rnd.random())
                    t1 = (rnd.uniform(-1, 1), rnd.uniform(-1, 1), rnd.uniform(-1, 1)); t2 = (rnd.uniform(-1, 1), rnd.uniform(-1, 1), rnd.uniform(-1, 1))
                    leaves.append((o_, tuple(o_[k_] + sz * t1[k_] for k_ in range(3)), tuple(o_[k_] + sz * t2[k_] for k_ in range(3)), d_))
            for A_, B_, C_, d_ in leaves:
                b2 = len(V); V += [A_, B_, C_]
                for q in range(3): VN.append(d_)
                F.append((b2, b2 + 1, b2 + 2, 0)); F.append((b2, b2 + 2, b2 + 1, 0))
        out += bytes(col) + bytes([255, 1 if V else 0]) + struct.pack('<HH', len(V), len(F))
        if V:
            EV = [tuple(int(round(v * S)) for v in (p_[0], p_[2], p_[1])) for p_ in V]
            for v_ in EV: out += struct.pack('<hhh', *v_)
            while len(out) % 4: out.append(0)
            for a, b, c_, s_ in F: out += struct.pack('<HHHH', a, c_, b, s_)
            for a, b, c_, s_ in F: out += struct.pack('<hhh', *face_normal(EV[a], EV[c_], EV[b]))
            # (and each vertex's own normal, x east y up z north, Q14: smooth shading)
            for n_ in VN: out += struct.pack('<hhh', int(n_[0] * 16000), int(n_[2] * 16000), int(n_[1] * 16000))
            while len(out) % 4: out.append(0)
            # (then its far form: u16 nv, nf, s16 v, to 4, u16 f, s16 n, to 4)
            EV = [tuple(int(round(v * S)) for v in (p_[0], p_[2], p_[1])) for p_ in V2]
            out += struct.pack('<HH', len(V2), len(F2))
            for v_ in EV: out += struct.pack('<hhh', *v_)
            while len(out) % 4: out.append(0)
            for a, b, c_, s_ in F2: out += struct.pack('<HHHH', a, c_, b, s_)
            for a, b, c_, s_ in F2: out += struct.pack('<hhh', *face_normal(EV[a], EV[c_], EV[b]))
            while len(out) % 4: out.append(0)
        print('   plant %2d %-28s %s: colour %s, %d lobes' % (k, e or '-', (m or '-').split('\\')[-1], col[:3], len(F) // 20))
    return bytes(out)
fl = []; unk = collections.Counter()
for r in csv.DictReader(open(os.path.join(PLACES, 'refs_area.csv'), encoding='utf-8')):
    e = r['base_editor_id']
    if r['world'] != 'Tamriel' or r['base_type'] != 'TREE' or e.startswith('TreePine') or not e.startswith('Tree'): continue
    k = flora_kind(e)
    if k is None: unk[e] += 1; continue
    flora_eds[k][e] += 1
    ex = (float(r['x']) + 233472) * U_M; ez = (float(r['y']) + 176128) * U_M        # metres
    sc = float(r['scale'] or 1)
    fl.append((ex, ez, k, int(float(r['rot_z']) / 360 * 64) & 63, max(0, min(3, int(round((sc - 0.8) / 0.2))))))
# (v0.4.66) the further exports' plants and trees (the Rift's)
for r in extra_refs():
    e = r['base_editor_id']
    if not e.startswith('Tree'): continue
    k = tree_kind(e)
    if k is None:
        if e.startswith('TreePine'): unk[e] += 1; continue
        k = flora_kind(e)
    if k is None: unk[e] += 1; continue
    flora_eds[k][e] += 1
    ex = (float(r['x']) + 233472) * U_M; ez = (float(r['y']) + 176128) * U_M
    sc = float(r['scale'] or 1)
    fl.append((ex, ez, k, int(float(r['rot_z']) / 360 * 64) & 63, max(0, min(3, int(round((sc - 0.8) / 0.2))))))
if fl:
    SQ = 32.0
    ox = math.floor(min(f[0] for f in fl)); oz = math.floor(min(f[1] for f in fl))
    gw = int((max(f[0] for f in fl) - ox) // SQ) + 1; gh = int((max(f[1] for f in fl) - oz) // SQ) + 1
    fl.sort(key=lambda f: (int((f[1] - oz) // SQ) * gw + int((f[0] - ox) // SQ)))
    first = [0] * (gw * gh + 1); k = 0
    for b in range(gw * gh):
        first[b] = k
        while k < len(fl) and int((fl[k][1] - oz) // SQ) * gw + int((fl[k][0] - ox) // SQ) == b: k += 1
    first[gw * gh] = k
    M_ = 256
    out = bytearray(b'HFL1' + struct.pack('<Iiiiii', len(fl), int(ox * M_), int(oz * M_), int(SQ * M_), gw, gh))
    out += struct.pack('<%dH' % len(first), *first)
    while len(out) % 4: out.append(0)
    for f in fl: out += struct.pack('<HHBB', int((f[0] - ox) * 8), int((f[1] - oz) * 8), f[2], f[3] * 4 + f[4])
    # then the animals Skyrim places there (its leveled creatures' markers, at level 1): "HAN1", u32 n, then n *
    # (i32 x, i32 z engine units, u8 kind (0 wolf, 1 deer, 2 elk, 3 hare, 4 fox, 5 goat, 6 mudcrab), 3 pad)
    AN = [('LvlAnimalPlainsPrey', 2), ('LvlAnimalForestPrey', 1), ('LvlAnimalMountainSnowPrey', 5), ('LvlAmbientCreatures', 3),
          ('EncHare', 3), ('EncFox', 4), ('EncDeer', 1), ('EncElk', 2), ('EncGoat', 5), ('LvlMudcrab', 6), ('EncMudcrab', 6),
          ('Predator', 0), ('EncWolf', 0), ('LvlWolf', 0)]
    an = []
    for r in csv.DictReader(open(os.path.join(PLACES, 'refs_area.csv'), encoding='utf-8')):
        e = r['base_editor_id']
        if r['world'] != 'Tamriel' or r['base_type'] != 'NPC_': continue
        k = next((v for key, v in AN if (key in e if key == 'Predator' else e.startswith(key))), None)
        if k is None or (key_ := 'Predator') and k == 0 and not (e.startswith('LvlAnimal') or 'Wolf' in e): continue
        if k == 3 and (int(float(r['x'])) * 7 + int(float(r['y']))) % 3 == 0: k = 4      # (the ambient creatures: a fox now and then)
        an.append((int((float(r['x']) + 233472) * U_M * 256), int((float(r['y']) + 176128) * U_M * 256), k))
    out += b'HAN1' + struct.pack('<I', len(an))
    for x, z, k in an: out += struct.pack('<iiBBBB', x, z, k, 0, 0, 0)
    print('animals: %d (%s)' % (len(an), collections.Counter(a[2] for a in an)))
    # (v0.4.42) then each kind's look from Skyrim's own plant: "HFM1", u32 kinds, then per kind u8 colour[6] (the
    # leaves' and the darker), u16 nv, nf, and (nv > 0) s16 v[nv * 3] (the game's plant units: 1.463 to Skyrim's,
    # x east, y up, z north), to 4, u16 f[nf * 4] (a, b, c, slot 0/1), s16 n[nf * 3], s16 vn[nv * 3], to 4. A bush: its leaf cards
    # gathered into lobes (k-means), each an uneven double pyramid, the lower ones darker - Skyrim's outline, solid
    out += flora_models(fl)
    if AREA in ('main', 'wr'): open(os.path.join(os.path.dirname(OUT), 'kit_flora.bin'), 'wb').write(out)
    print('plants: %d (%d bytes, %d x %d squares); left out: %s' % (len(fl), len(out), gw, gh, unk.most_common(8)))
for cname, c in cells.items(): print(' %-26s statics %d props %d items %d flames %d lights %d people %d floor tris %d walls %d' % ((cname,) + c['counts']))

# ---------------------------------------------------------------- the placements checked (docs/maps/audit.txt)
# Every reference a cell has that is not drawn or not taken as the game's own (by kind and editor id), and every item
# with nothing under it (no surface facing up within its reach below, among the cell's drawn models): what was lost on
# the way (v0.4.39: the Trader's upstairs table, a furniture record, was dropped and its food floated).
def audit_cell(c):
    tops = []
    def add(mi, x, y, z, rz, sc):
        if mi < 0 or mi >= len(mesh_data): return
        pts, faces = mesh_data[mi][2], mesh_data[mi][3]
        a = math.radians(rz); ca, sa = math.cos(a), math.sin(a)
        W = [(x + sc * (p_[0] * ca + p_[1] * sa), y + sc * (-p_[0] * sa + p_[1] * ca), z + sc * p_[2]) for p_ in pts]
        for f_ in faces:
            A, B, C = W[f_[0]], W[f_[1]], W[f_[2]]
            n_ = _cross(_sub(B, A), _sub(C, A)); L = math.sqrt(_dot(n_, n_)) or 1
            if abs(n_[2]) / L > 0.5: tops.append((A, B, C))
    for st in c['stat']: add(st[0], st[1], st[2], st[3], st[4], st[5] / 256.0)
    for pm in c.get('prop_mi', []): add(*pm[:6])
    def under(x, y, z):
        best = None
        for A, B, C in tops:
            if min(A[0], B[0], C[0]) - 2 > x or max(A[0], B[0], C[0]) + 2 < x or min(A[1], B[1], C[1]) - 2 > y or max(A[1], B[1], C[1]) + 2 < y: continue
            d = (B[1] - C[1]) * (A[0] - C[0]) + (C[0] - B[0]) * (A[1] - C[1])
            if abs(d) < 1e-6: continue
            l1 = ((B[1] - C[1]) * (x - C[0]) + (C[0] - B[0]) * (y - C[1])) / d
            l2 = ((C[1] - A[1]) * (x - C[0]) + (A[0] - C[0]) * (y - C[1])) / d
            l3 = 1 - l1 - l2
            if min(l1, l2, l3) < -0.08: continue
            h = l1 * A[2] + l2 * B[2] + l3 * C[2]
            if z - 30 <= h <= z + 8 and (best is None or h > best): best = h
        return best
    floating = [(it, x, y, z) for it, x, y, z in c['item'] if under(x, y, z) is None and z > 8]
    return floating
with open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'audit.txt' if AREA == 'main' else 'audit_%s.txt' % AREA), 'w') as fa:
    nflo = ndrop = 0
    for cname, c in cells.items():
        if c.get('outside'): continue
        fl = audit_cell(c)
        dr = c.get('dropped', {})
        nflo += len(fl); ndrop += sum(dr.values())
        fa.write('%s: %d items, %d with nothing under them; %d references dropped or not taken\n' % (cname, len(c['item']), len(fl), sum(dr.values())))
        for it, x, y, z in fl: fa.write('   floating: %s at %d %d %d\n' % (it, x, y, z))
        for k, v in sorted(dr.items()): fa.write('   %3d  %s\n' % (v, k))
    print('audit (docs/maps/audit.txt): %d items with nothing under them, %d references dropped or not taken' % (nflo, ndrop))

# ---------------------------------------------------------------- (v0.4.53) what the area's named people say
# From the export's dialogue.csv (tools/skyrim_area.py): their greetings (HELO) that are theirs alone (GetIsID), kept
# when every condition is one the game can judge: GetIsID (72) them, GetDead (46) == 0 (alive: the game's people
# are), the player's sex (131: male). The lines a condition holds back (a quest's stage, the player's faction) left
# out. Written as src/kit_dialogue.inc: the lines, and each named person's first line and count.
SAY_WHO = {'SkulvarSableHilt': 23, 'Jervar': 24, 'SeverioPelagia': 25, 'Nimriel': 26, 'Wilmuth': 27, 'CarriageDriverWhiterun': 28}

# (v0.4.65) Whiterun's own people in its streets (the open city): everyone with a place of their own in WhiterunWorld
# by the hour (packages.csv: a package's place and radius there, from its hour for its minutes), named from 29 on.
# Left out: the market's three (they are the merchants at their stalls), Adrianne (the smith, at her forge), Saadia
# (the inn's), the guards (no names) and Proventus (in Dragonsreach). Written as src/wr_folk.inc: their rows (named,
# x, y, z, radius, hour, hours, 0, walks about) and their looks (sex, child, skin, shirt, trousers, hair).
WR_SKIP = {'CarlottaValentia', 'Anoriath', 'FraliaGrayMane', 'AdrianneAvenicci', 'Saadia', 'ProventusAvenicci'}
def _outfit_rgb(o):
    o = o.lower()
    for k_, sh, tr in (('guard', (176, 140, 60), (70, 60, 46)), ('companions', (70, 56, 42), (56, 46, 36)),
                       ('draugr', (84, 76, 62), (60, 50, 40)), ('steel', (128, 130, 136), (70, 60, 46)),
                       ('iron', (112, 112, 116), (70, 60, 46)), ('scaled', (104, 98, 86), (66, 56, 44)),
                       ('hide', (116, 88, 62), (84, 64, 46)), ('leather', (96, 70, 48), (70, 52, 38)),
                       ('monk', (128, 116, 94), (110, 100, 82)), ('beggar', (108, 100, 86), (80, 72, 62)),
                       ('barkeep', (120, 98, 70), (84, 70, 56)), ('blacksmith', (84, 66, 48), (70, 54, 40)),
                       ('fine', (96, 62, 112), (60, 46, 70)), ('merchant', (130, 64, 52), (80, 62, 48)),
                       ('child', (140, 118, 86), (78, 64, 50)), ('hewnon', (96, 86, 104), (62, 56, 66)),
                       ('outfit04', (84, 96, 112), (72, 62, 50)), ('outfit03', (112, 82, 56), (76, 60, 44)),
                       ('outfit02', (104, 106, 84), (80, 70, 54)), ('outfit01', (168, 140, 98), (84, 66, 48))):
        if k_ in o: return sh, tr
    return (140, 118, 86), (80, 66, 50)
_HAIR = {'Nord': (150, 110, 64), 'Imperial': (52, 40, 32), 'Redguard': (30, 24, 20), 'Elder': (206, 206, 200),
         'WoodElf': (92, 64, 40), 'DarkElf': (40, 36, 40), 'Breton': (90, 62, 40), 'HighElf': (190, 160, 100)}
WR_FOLK = []                                           # (editor id, name, named, looks, rows)
for d_ in MESH_DIRS:
    if AREA != 'main' or not os.path.exists(os.path.join(d_, 'packages.csv')) or not os.path.exists(os.path.join(d_, 'land_WhiterunWorld.bin')): continue
    _pk = {r['editor_id']: r for r in csv.DictReader(open(os.path.join(d_, 'packages.csv'), encoding='utf-8'))}
    for r in csv.DictReader(open(os.path.join(d_, 'npcs.csv'), encoding='utf-8')):
        eid = r['editor_id']
        if eid in WR_SKIP or not r['name'] or eid.startswith('Guard') or eid in SAY_WHO: continue
        rows = []
        for pn in r['packages'].split():
            p_ = _pk.get(pn)
            if not p_ or int(p_['hour']) < 0: continue
            m_ = re.search(r'@WhiterunWorld//(-?\d+)/(-?\d+)/(-?\d+):r(\d+)', p_['locations'])
            if not m_: continue
            x_, y_, z_, rad = (int(v) for v in m_.groups())
            hrs = max(1, min(23, int(round(int(p_['duration_min']) / 60.0))))
            rows.append([x_, y_, z_, min(rad, 1200), int(p_['hour']), hrs, 1 if 'Sandbox' in p_['template'] or 'Patrol' in p_['template'] else 0])
        if not rows: continue
        k = 29 + len(WR_FOLK)
        race = r['race'].replace('RaceChild', '').replace('Race', '')
        skin = tuple(int(v) for v in r['skin_rgb'].split()) if r['skin_rgb'].strip() else (200, 160, 130)
        sh, tr = _outfit_rgb(r['outfit'])
        looks = (int(r['female']), 1 if 'Child' in r['race'] else 0) + skin + sh + tr + _HAIR.get(race, (110, 80, 52))
        WR_FOLK.append((eid, r['name'], k, looks, rows))
        SAY_WHO[eid] = k
    break
with open(os.path.join(SRCDIR, 'wr_folk.inc') if AREA == 'main' else os.devnull, 'w') as f:
    f.write('// generated by docs/maps/gen_kit.py (v0.4.65): Whiterun\'s people in its streets, from the export\'s npcs.csv and packages.csv\n')
    rows_ = [[k] + w for _, _, k, _, rws in WR_FOLK for w in rws]
    f.write('enum { KWrFolk0 = 29, KWrFolkN = %d, KWrFolkRowN = %d };\n' % (len(WR_FOLK), len(rows_)))
    f.write('static const char* const KWrFolkName[%d] = { %s };\n' % (max(1, len(WR_FOLK)), ', '.join('"%s"' % n for _, n, _, _, _ in WR_FOLK) or '""'))
    f.write('// named, x, y, z (Skyrim), radius, from hour, hours, home cell (0: away), walks about\n')
    f.write('static const short KWrFolkRow[%d] = {\n' % max(1, len(rows_) * 9))
    for (eid, _, k, _, rws) in WR_FOLK:
        for w in rws: f.write('    %d, %d, %d, %d, %d, %d, %d, 0, %d,     // %s\n' % (k, w[0], w[1], w[2], w[3], w[4], w[5], w[6], eid))
    f.write('    };\n// sex (1 woman), child, skin, shirt, trousers, hair\n')
    f.write('static const unsigned char KWrFolkLook[%d] = {\n' % max(1, len(WR_FOLK) * 14))
    for (eid, _, k, lk, _) in WR_FOLK: f.write('    %s,     // %s\n' % (', '.join(str(v) for v in lk), eid))
    f.write('    };\n')
print('Whiterun\'s people: %d, %d places by the hour' % (len(WR_FOLK), sum(len(q[4]) for q in WR_FOLK)))
def _say_ok(conds, who):
    for c_ in conds.split():
        m_ = re.match(r'^(\d+):(.*?)(==|!=|>=|<=|>|<|\?)([-0-9.e]+)\|?$', c_)
        if not m_: return False
        fn, p1, op, v = int(m_.group(1)), m_.group(2), m_.group(3), float(m_.group(4))
        if fn == 72 and p1 == who and op == '==' and v == 1: continue
        if fn == 46 and op == '==' and v == 0: continue
        if fn == 131 and op == '==' and v == 1 and p1 in ('0', ''): continue
        return False
    return True
# (v0.4.54) who has none of their own: their factions' (a carriage driver's: CarriageSystemFaction - GetInFaction
# 71, nothing else asked), else their voice type's everyday ones (GetIsVoiceType 426 - only lines shared by several
# voices, of the everyday greetings' topic DialogueGenericHello, asking nothing more: the others answer what their
# quests' conditions hold - nakedness, a spell in hand, an old assault remembered)
def _cond_list(conds):
    out = []
    for c_ in conds.split():
        m_ = re.match(r'^(\d+):(.*?)(==|!=|>=|<=|>|<|\?)([-0-9.e]+)\|?$', c_)
        if not m_: return None
        out.append((int(m_.group(1)), m_.group(2), m_.group(3), float(m_.group(4))))
    return out
_said = collections.defaultdict(list); _fsaid = collections.defaultdict(list); _vsaid = collections.defaultdict(list)
for d_ in MESH_DIRS:
    fn_ = os.path.join(d_, 'dialogue.csv')
    if not os.path.exists(fn_): continue
    _said.clear(); _fsaid.clear(); _vsaid.clear()
    _voice = {}; _facs = {}
    if os.path.exists(os.path.join(d_, 'npcs.csv')):
        for r in csv.DictReader(open(os.path.join(d_, 'npcs.csv'), encoding='utf-8')):
            _voice[r['editor_id']] = r.get('voice') or ''
            _facs[r['editor_id']] = set(q.split(':')[0] for q in (r.get('factions') or '').split())
    for r in csv.DictReader(open(fn_, encoding='utf-8')):
        if r['topic_kind'] != 'HELO': continue
        t_ = r['text'].strip()
        for who in r['speaker'].split():
            if who in SAY_WHO and _say_ok(r['conditions'], who) and t_ not in _said[who]: _said[who].append(t_)
        cl = _cond_list(r['conditions'])
        if not cl: continue
        for who in SAY_WHO:
            if all(fn == 71 and p1 in _facs.get(who, ()) and op == '==' and v == 1 for fn, p1, op, v in cl) and t_ not in _fsaid[who]: _fsaid[who].append(t_)
            if r['topic'] == 'DialogueGenericHello' and len(cl) >= 2 and all(fn == 426 and op == '==' and v == 1 for fn, p1, op, v in cl) and any(p1 == _voice.get(who) for fn, p1, op, v in cl) and t_ not in _vsaid[who]: _vsaid[who].append(t_)
    for who in SAY_WHO:
        if not _said[who]: _said[who] = _fsaid[who][:6] or _vsaid[who][:6]
def _cstr(t): return '"' + t.replace('\\', '\\\\').replace('"', '\\"') + '"'
with open(os.path.join(SRCDIR, 'kit_dialogue.inc') if AREA == 'main' else os.devnull, 'w') as f:
    f.write('// generated by docs/maps/gen_kit.py from the export\'s dialogue.csv: the named people\'s own greetings in Skyrim\n')
    lines = []; ix = {}
    for who, k in sorted(SAY_WHO.items(), key=lambda q: q[1]):
        ix[k] = (len(lines), len(_said.get(who, [])))
        lines += _said.get(who, [])
    f.write('static const char* const KKitSay[%d] = {\n' % max(1, len(lines)))
    for t in lines or ['']: f.write('    %s,\n' % _cstr(t))
    f.write('    };\n')
    f.write('static const unsigned short KKitSayIx[%d] = { %s };     // per named person (0..%d): first line, count\n' % (
        2 * (max(SAY_WHO.values()) + 1), ', '.join('%d, %d' % ix.get(k, (0, 0)) for k in range(max(SAY_WHO.values()) + 1)), max(SAY_WHO.values())))
print('their own greetings: ' + ', '.join('%s %d' % (w, len(_said.get(w, []))) for w in SAY_WHO))
