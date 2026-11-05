# Turns the plan (pixels of docs/maps/ref_whiterun_city_map.png, 1500x1935, north up; the outside from the online map's
# approach detail) into game tables: src/city_plan.inc. Coordinates out are decimetres in the city frame (x east,
# z north; the Elder Ash stays at (-2, 23) m), heights are decimetres above the lower city (h0), yaws 1024 a turn.
import math
S = 0.085                                   # metres per map pixel
def X(px): return round(((px - 725) * S - 2) * 10)
def Z(py): return round((23 - (py - 1060) * S) * 10)
def P(px, py): return (X(px), Z(py))
# the online approach detail (its pixels) -> metres, lined up at the city gate
def OX(x): return round((-61.9 + (x - 575) * 0.2) * 10)
def OZ(y): return round((-11.85 - (y - 450) * 0.2) * 10)
def O(x, y): return (OX(x), OZ(y))
def yaw_dir(fx, fz):                        # yaw whose local -z (a building's front) points along (fx, fz)
    return round(math.degrees(math.atan2(-fx, -fz)) * 1024 / 360) & 1023
def yaw_rot(rot): return round(rot * 1024 / 360) & 1023
def front(yaw):                             # world direction of local -z
    a = yaw * 2 * math.pi / 1024; return (-math.sin(a), -math.cos(a))
def minus_x(yaw):
    a = yaw * 2 * math.pi / 1024; return (-math.cos(a), math.sin(a))
out = []
def table(name, rows, per):
    flat = [v for r in rows for v in r]
    out.append("static const int %s[%d] = {" % (name, len(flat)))
    for r in rows: out.append("    " + ", ".join(str(v) for v in r) + ",")
    out.append("    };")
    out.append("enum { %s_N = %d };" % (name, len(rows)))

# ---- walls: polylines (inside to the right of the direction of travel), then scale
walls = [
 (256, [(40,1420),(40,1240),(15,1020),(60,905),(110,710),(245,655),(600,660),(720,545),(815,490),(775,355),(830,235),(940,125),(1165,40),(1330,150),(1430,280),(1430,420),(1330,520)]),
 (256, [(1355,850),(1365,1085),(1305,1170),(1155,1245),(1030,1255),(990,1380),(955,1530),(930,1680),(850,1815),(800,1860),(645,1868),(490,1860),(370,1812),(235,1745),(75,1660),(40,1560),(40,1520)]),
 (230, [(40,1215),(205,1223)]), (230, [(265,1226),(650,1245)]), (230, [(750,1245),(1030,1255)]),   # the tier wall: gaps for the stairs
 (230, [(905,578),(990,660),(1135,665),(1220,615),(1330,520)]),   # round Highhall's terrace (open where the causeway crosses)
 (230, [(855,735),(1000,745),(1000,1010)]), (230, [(1000,1070),(1000,1250)]),          # Jorrvaskr's terrace
]
pts = []; runs = []
for sc, pl in walls:
    runs.append((len(pts), len(pl), sc)); pts += [P(*p) for p in pl]
table("KPlanWallPt", pts, 2); table("KPlanWallRun", runs, 3)
# single wall pieces: x, z, yaw, scale (the pool's rim over the Wind District, either side of the first flight)
segs = [(-115, 495, 0, 120), (26, 495, 0, 180), (61, 495, 0, 180)]
table("KPlanWallSeg", segs, 4)
# towers (bastions): x, z, scale, brazier
towers = [(15,1020,190,0),(110,710,200,1),(245,655,190,0),(600,660,190,1),(815,490,200,0),(830,235,190,0),(1165,40,200,1),
          (1430,280,190,0),(1330,520,200,1),(1365,1085,190,0),(1305,1170,200,1),(1030,1255,190,0),(955,1530,200,1),(850,1815,190,0),
          (645,1868,200,1),(370,1812,190,0),(235,1745,200,1),(75,1660,190,0),(400,1225,170,0),(650,1245,170,0),(750,1245,170,0),
          (990,660,170,0),(1135,665,170,0)]
table("KPlanTower", [P(x, y) + (s, b) for (x, y, s, b) in towers], 4)

# ---- levels: polygons with their height (dm above h0) and edge (m), applied in order
levels = [
 (40, 2, [(40,1240),(15,1020),(60,905),(110,710),(245,655),(600,660),(650,700),(855,735),(1000,745),(1000,1250),(40,1218)]),   # Wind District
 (60, 2, [(1000,745),(1150,760),(1200,900),(1355,850),(1365,1085),(1305,1170),(1155,1245),(1000,1250)]),                      # Jorrvaskr
 (90, 2, [(1135,665),(1220,615),(1330,520),(1450,600),(1450,880),(1355,850),(1200,900),(1150,760)]),                          # Skyforge rock
 (80, 1, [(600,660),(720,545),(815,490),(850,470),(890,430),(940,470),(1040,565),(1135,665),(990,660),(855,735),(650,700)]),  # pool and moat basin
 (90, 1, [(675,690),(775,690),(775,750),(675,750)]),                                                                          # the pool platform
 (180, 2, [(815,490),(775,355),(830,235),(940,125),(1165,40),(1330,150),(1430,280),(1430,420),(1330,520),(1220,615),(1135,665),(1040,565),(940,470),(890,430),(850,470)]),  # Highhall
 (20, 1, [(790,1310),(915,1310),(915,1550),(790,1550)]),                                                                      # the inn's rise
]
lp = []; lr = []
for h, e, pl in levels:
    lr.append((len(lp), len(pl), h, e)); lp += [P(*p) for p in pl]
table("KPlanLevelPt", lp, 2); table("KPlanLevel", lr, 4)
city_outline = [(40,1560),(40,1240),(15,1020),(60,905),(110,710),(245,655),(600,660),(720,545),(815,490),(775,355),(830,235),(940,125),(1165,40),(1330,150),(1430,280),(1450,600),(1450,880),(1365,1085),(1305,1170),(1155,1245),(1030,1255),(990,1380),(955,1530),(930,1680),(850,1815),(800,1860),(645,1868),(490,1860),(370,1812),(235,1745),(75,1660)]
table("KPlanOutline", [P(*p) for p in city_outline], 2)

# ---- buildings: (label, ct, px, py, w, h, rot, facing): facing 'S'/'N' = front south/north of the rotated footprint,
# ('T', x, y) = whichever of the two fronts faces that point; the long side w goes along local x
T = lambda x, y: ('T', x, y)
TREE = T(725, 1060)
B = [("barracks","CT_HOUSE",160,1310,110,100,0,'S'), ("warmaiden","CT_HOUSE",205,1555,120,110,0,'N'),
     ("huntsman","CT_HOUSE",345,1335,115,90,-25,'S'), ("severio","CT_HOUSE",560,1355,140,100,0,'S'),
     ("breezehome","CT_HOUSE",360,1580,100,140,0,T(360,1470)), ("belethor","CT_HOUSE",590,1580,150,110,15,'N'),
     ("arcadia","CT_HOUSE",800,1640,90,130,0,T(700,1490)), ("olava","CT_HOUSE",380,1730,85,70,0,'N'),
     ("ysolda","CT_HOUSE",690,1760,90,80,-25,'N'), ("mare","CT_HALL",850,1430,100,210,0,T(700,1440)),
     ("eastbarracks","CT_HOUSE",985,1305,70,50,0,T(850,1400)),
     ("heimskr","CT_HOUSE",835,1185,100,90,30,TREE), ("graymane","CT_HOUSE",510,1110,110,190,-20,TREE),
     ("amren","CT_HOUSE",385,1000,120,100,-20,TREE), ("uthgerd","CT_HOUSE",330,1110,110,100,-20,TREE),
     ("carlotta","CT_HOUSE",130,1100,120,110,0,T(300,1100)), ("battleborn","CT_HOUSE",270,820,150,150,30,TREE),
     ("jorrvaskr","CT_HALL",1120,1050,210,300,0,T(960,1050)), ("dragonsreach","CT_GREATHALL",1110,290,320,430,35,T(950,470))]
NOMLEN = {"CT_HOUSE": 10.0, "CT_HALL": 21.0, "CT_GREATHALL": 26.0, "CT_TEMPLE": 14.0}
LEVEL = {"heimskr":40,"graymane":40,"amren":40,"uthgerd":40,"carlotta":40,"battleborn":40,"jorrvaskr":60,"dragonsreach":180,"mare":20}
rows = []; named = {}; doors = []
for (lab, ct, px, py, w, h, rot, fc) in B:
    if h > w: w, h, rot = h, w, rot + 90
    yaw = yaw_rot(rot)
    if fc == 'N': yaw = (yaw + 512) & 1023
    elif fc != 'S':
        tx, tz = (fc[1] - px) * S, -(fc[2] - py) * S
        if ct == "CT_GREATHALL":      # its door is on the long side: turn it to face the stairs whatever the footprint
            yaw = max(((yaw + k * 256) & 1023 for k in range(4)), key=lambda y: front(y)[0] * tx + front(y)[1] * tz)
        else:
            f = front(yaw)
            if f[0] * tx + f[1] * tz < 0: yaw = (yaw + 512) & 1023
    sc = round(256 * w * S / NOMLEN[ct])
    sc = max(170, min(sc, 300 if ct == "CT_HOUSE" else 340))
    var = len(rows) % 4 + 4
    rows.append((ct, X(px), Z(py), yaw, sc, var, LEVEL.get(lab, 0))); named[lab] = rows[-1]
    hz = {"CT_HOUSE": 3.5, "CT_HALL": 5.0, "CT_GREATHALL": 8.0}[ct] * sc / 256 + 0.6
    f = front(yaw); doors.append((lab, round(X(px) + f[0] * hz * 10), round(Z(py) + f[1] * hz * 10)))
# the temple and the hall of the dead: porch (local -x) towards a point
for (lab, px, py, w, rot, tx_, ty_) in (("temple", 570, 920, 170, 45, 725, 1060), ("halldead", 450, 730, 120, 0, 450, 850)):
    yaw = yaw_rot(rot); tx, tz = (tx_ - px) * S, -(ty_ - py) * S
    best = max(((yaw + k * 256) & 1023 for k in range(4)), key=lambda y: minus_x(y)[0] * tx + minus_x(y)[1] * tz)
    sc = max(170, round(256 * w * S / 14))
    rows.append(("CT_TEMPLE", X(px), Z(py), best, sc, 0, 40))
    mx = minus_x(best); d_ = 7.0 * sc / 256 + 0.6
    doors.append((lab, round(X(px) + mx[0] * d_ * 10), round(Z(py) + mx[1] * d_ * 10)))
out.append("static const int KPlanBuilding[%d] = {" % (len(rows) * 7))
for r in rows: out.append("    %s, %d, %d, %d, %d, %d, %d," % r)
out.append("    };"); out.append("enum { KPlanBuilding_N = %d };" % len(rows))

# ---- the doors of the numbered buildings (not to be entered yet): where, and the name shown there
NAMES = {"barracks": "Guard Barracks", "huntsman": "The Drunken Huntsman", "severio": "Severio Pelagia's House", "mare": "The Bannered Mare",
         "eastbarracks": "Guard Barracks", "arcadia": "Arcadia's Cauldron", "ysolda": "Ysolda's House", "belethor": "Belethor's General Goods",
         "olava": "Olava the Feeble's House", "breezehome": "Breezehome", "warmaiden": "Warmaiden's", "temple": "Temple of Kynareth",
         "heimskr": "Heimskr's House", "jorrvaskr": "Jorrvaskr", "graymane": "House Gray-Mane", "uthgerd": "Uthgerd the Unbroken's House",
         "amren": "Amren's House", "carlotta": "Carlotta Valentia's House", "battleborn": "House of Clan Battle-Born", "halldead": "Hall of the Dead",
         "dragonsreach": "Dragonsreach"}
DOORS_PENDING = True
# ---- market stalls (x, z, yaw: counter front towards the well)
WELL = (700, 1490)
stalls = [(645,1405),(740,1385),(610,1515),(680,1565),(745,1540)]
st = []
for (x, y) in stalls:
    st.append(P(x, y) + (yaw_dir((WELL[0] - x) * S, -(WELL[1] - y) * S),))
pos = []
save = out; out = pos
table("KPlanStall", st, 3)
out = save

# ---- stairs: ct, foot (x, z), yaw (uphill), base (dm above h0), steps, half width (cm), carve (0: stands on piers)
def stair(ct, fx, fy, tx, ty, base, n, hw, carve):
    yaw = round(math.degrees(math.atan2((tx - fx) * S, -(ty - fy) * S)) * 1024 / 360) & 1023
    return (ct, X(fx), Z(fy), yaw, base, n, hw, carve)
stairs = [stair("CT_STAIRSW", 700, 1343, 700, 1260, 0, 32, 300, 1),       # market -> Wind District (wide)
          stair("CT_STAIRS", 235, 1305, 235, 1222, 0, 32, 125, 1),        # small stairs by the barracks
          stair("CT_STAIRS16", 755, 1440, 796, 1440, 0, 16, 250, 1),      # market -> the inn
          stair("CT_STAIRS2W", 700, 872, 700, 769, 40, 40, 300, 1),       # flight 1: the tree -> the pool platform
          stair("CT_STAIRS24", 745, 702, 745, 640, 90, 24, 200, 0),       # flight 2: onto the first pier
          stair("CT_STAIRS", 763, 620, 831, 572, 120, 32, 125, 0),        # flight 3: pier to pier
          stair("CT_STAIRS16", 915, 505, 946, 478, 160, 16, 250, 0),      # flight 4: off the causeway onto Highhall's terrace
          stair("CT_STAIRS16", 958, 1040, 999, 1040, 40, 16, 250, 1),     # up to Jorrvaskr's terrace
          stair("CT_STAIRS24", 1250, 906, 1250, 844, 60, 24, 200, 1)]     # up to the Skyforge
out.append("static const int KPlanStair[%d] = {" % (len(stairs) * 8))
for s_ in stairs: out.append("    %s, %d, %d, %d, %d, %d, %d, %d," % s_)
out.append("    };"); out.append("enum { KPlanStair_N = %d };" % len(stairs))
# piers: x, z, top (dm above h0), yaw
piers = [(745,611,120,0), (855,555,160,round(-40.4*1024/360)&1023), (893,523,160,round(-40.4*1024/360)&1023)]
table("KPlanPier", [P(x, y) + (t, w) for (x, y, t, w) in piers], 4)

# ---- water: channels (polyline, width class 0 narrow / 1 canal), pool and moat (polygon minus the platform)
chan = [(1, [(125,700),(125,1215),(90,1240),(90,1560)]), (0, [(655,752),(655,1352)]), (0, [(745,752),(745,1352)])]
# the drains at the channels' ends, at the foot of the wide stairs
table("KPlanDrain", [P(655, 1362), P(745, 1362)], 2)
cp = []; cr = []
for w, pl in chan:
    cr.append((len(cp), len(pl), w)); cp += [P(*p) for p in pl]
table("KPlanChanPt", cp, 2); table("KPlanChan", cr, 3)
pool = [(600,660),(720,545),(815,490),(850,470),(890,430),(940,470),(1040,565),(1135,665),(990,660),(855,735),(650,700)]
table("KPlanPool", [P(*p) for p in pool], 2)
table("KPlanPlatform", [P(675,690), P(775,750)], 2)
# cliffs: x, z, yaw (outwards), top (dm above h0)
cliffs = [(800,450,640,180),(1010,560,420,180),(1090,640,420,180),(1155,790,512,90),(1330,875,512,90),(1430,640,256,90),(1440,800,256,90)]
table("KPlanCliff", [P(x, y) + (yw, t) for (x, y, yw, t) in cliffs], 4)

# ---- singles (x, z) and a few yaws
def one(name, x, y): pos.append("enum { %s_X = %d, %s_Z = %d };" % (name, X(x), name, Z(y)))
one("PL_GATE", 30, 1470); one("PL_WELL", 700, 1490); one("PL_TREE", 725, 1060); one("PL_TALOS", 840, 860)
one("PL_FORGE", 125, 1525); one("PL_SKYFORGE", 1240, 790); one("PL_HALLDOOR", 955, 470)
# streets (polylines, painted cobble): lower district, middle district
streets = [[(40,1470),(250,1470),(450,1460),(600,1480),(700,1490)], [(700,1490),(700,1343)], [(235,1305),(235,1400),(250,1470)],
           [(300,1480),(330,1650),(470,1700),(640,1820)], [(700,1490),(820,1600),(860,1720)],
           [(700,1260),(700,1160)], [(620,1060),(380,1060),(200,950),(160,820)], [(830,1060),(958,1040)], [(700,960),(700,872)]]
sp = []; sr = []
for pl in streets:
    sr.append((len(sp), len(pl))); sp += [P(*p) for p in pl]
table("KPlanStreetPt", sp, 2); table("KPlanStreet", sr, 2)

# ---- the outside (the online map's approach detail)
road = [(455,940,-100),(455,900,-100),(440,790,-95),(400,640,-80),(330,520,-65),(320,470,-56),(315,420,-50),(310,375,-45),
        (308,318,-45),(305,300,-42),(320,230,-30),(370,200,-25),(450,205,-20),(470,260,-15),(475,380,-10),(480,440,-5),(575,450,0)]
table("KPlanRoad", [O(x, y) + (h,) for (x, y, h) in road], 3)
riverroad = [(0,880),(200,860),(455,870),(681,880)]
# the plain round the city's south-west, a little below the lower city (dm, height dm above h0)
out.append('static const int KPlanPlain[16] = { -2000, -1350,  -450, -1350,  -450, -520,  -640, -470,  -640, -280,  -1350, -280,  -1350, 220,  -2000, 220 };')
out.append('enum { KPlanPlain_N = 8, PLAIN_H = -100 };')
table("KPlanRiverRoad", [O(x, y) for (x, y) in riverroad], 2)
stream = [(575,300),(520,310),(460,312),(420,315),(380,322),(330,335),(240,360),(200,365),(160,380),(120,450),(95,520),(80,545),(60,600),(30,700),(0,760)]
table("KPlanStream", [O(x, y) for (x, y) in stream], 2)
ap_walls = [[(420,300),(330,300)], [(290,300),(240,300),(240,345)], [(240,375),(240,500),(305,500)], [(340,500),(420,500)], [(575,500),(470,500)],
            [(290,250),(290,140),(520,140),(560,180),(575,180)], [(420,500),(420,330)], [(455,275),(420,275)], [(575,275),(505,275)]]
ap = []; ar = []
for pl in ap_walls:
    ar.append((len(ap), len(pl))); ap += [O(*p) for p in pl]
table("KPlanApWallPt", ap, 2); table("KPlanApWall", ar, 2)
# the approach's ground: west part (low), east part, north part (dm above h0)
apl = [(-50, [(240,300),(420,300),(420,500),(240,500)]), (-12, [(420,275),(575,275),(575,500),(420,500)]),
       (-35, [(290,140),(520,140),(575,180),(575,275),(290,275)])]
alp = []; alr = []
for h, pl in apl:
    alr.append((len(alp), len(pl), h)); alp += [O(*p) for p in pl]
table("KPlanApLevelPt", alp, 2); table("KPlanApLevel", alr, 3)
def oo(name, x, y): pos.append("enum { %s_X = %d, %s_Z = %d };" % (name, OX(x), name, OZ(y)))
oo("PL_BRIDGE", 310, 318); oo("PL_LEVER", 335, 312); oo("PL_OUTERGATE", 322, 500); oo("PL_INNERGATE", 475, 275); oo("PL_WATERGATE", 240, 360)
oo("PL_STABLES", 485, 655); oo("PL_STABLEBLOCK", 535, 715); oo("PL_CARRIAGE", 478, 790); oo("PL_CARAVAN", 255, 590)
oo("PL_FIELD1", 265, 725); oo("PL_FIELD2", 345, 850); oo("PL_POND", 80, 545); oo("PL_START", 455, 900)
pos.append("static const int KPlanDoor[%d] = {" % (len(doors) * 2))
for (lab, x, z) in doors: pos.append("    %d, %d,     // %s" % (x, z, lab))
pos.append("    };")
pos.append("static const char* const KPlanDoorName[%d] = {" % len(doors))
for (lab, x, z) in doors: pos.append('    "%s",' % NAMES[lab])
pos.append("    };")
pos.append("enum { KPlanDoor_N = %d };" % len(doors))
open("/home/claude/game/src/plan_pos.inc", "w").write("// generated by docs/maps/gen_layout.py: plan positions shared by the game files (dm, city frame)\n" + "\n".join(pos) + "\n")
open("/home/claude/game/src/city_plan.inc", "w").write("// generated by docs/maps/gen_layout.py from the plan - do not edit by hand\n" + "\n".join(out) + "\n")
print("buildings", len(rows), "stairs", len(stairs), "lines", len(out))
for k in ("PL_GATE","PL_WELL","PL_TREE"): pass
print("gate", X(30), Z(1470), "well", X(700), Z(1490), "dragonsreach", named["dragonsreach"][1:5], "jorrvaskr", named["jorrvaskr"][1:5])
print("stables", OX(485), OZ(655), "caravan", OX(255), OZ(590), "bridge", OX(305), OZ(315), "start", OX(455), OZ(900))
