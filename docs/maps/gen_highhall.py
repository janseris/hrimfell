# Highhall (after Dragonsreach) - the main hall level, generated from the reference floor plan
# docs/maps/ref/dragonsreach_dnd_small.png (1500 x 1290 px, north up, the great doors at the bottom).
# Built at about three quarters of the real size: 22 px of that image = 1 m.
# Output: src/highhall.inc (cm in the interior frame; x east, z north, y up; the great doors at x 0, z 0).
#
# Levels (cm above the entrance floor): the entrance hall 0, the great hall 300, the dais 390, the throne 435,
# the galleries / north balcony / war room 700, the storeroom (down from the kitchen) 100.
import os

def X(px): return int(round((px - 780) * 100 / 22.0))
def Z(py): return int(round((1205 - py) * 100 / 22.0))

out = []
def table(name, rows, n):
    out.append("static const int %s[%d] = {" % (name, len(rows) * n))
    for r in rows: out.append("    " + ", ".join(str(v) for v in r) + ",")
    out.append("    };")
    out.append("enum { %s_N = %d };" % (name, len(rows)))

# ---------------------------------------------------------------- floors: x0, z0, x1, z1, level (cm)
# the floor under a point is the highest of these at or below the feet (+60 cm): galleries above, rooms below
F = []
def floor(px0, py0, px1, py1, lvl):
    F.append((min(X(px0), X(px1)), min(Z(py0), Z(py1)), max(X(px0), X(px1)), max(Z(py0), Z(py1)), lvl))
floor(510, 780, 1060, 1200, 0)          # the entrance hall and its north corners
floor(420, 610, 620, 780, 300)          # the great hall, round the stairwell
floor(950, 610, 1145, 780, 300)
floor(420, 270, 1145, 610, 300)
floor(430, 85, 1145, 230, 390)          # the dais
floor(690, 90, 880, 155, 435)           # the throne's platform
floor(85, 265, 420, 605, 300)           # kitchen
floor(1145, 265, 1480, 605, 300)        # the court wizard's rooms
floor(0, 0, 270, 265, 100)              # the storeroom and servants' corner, down from the kitchen
floor(420, 280, 610, 780, 700)          # west gallery, south of the west stairs
floor(520, 85, 610, 280, 700)           # its strip beside the stairs, over the dais
floor(445, 85, 520, 125, 700)           # the landing at the top of the west stairs
floor(955, 85, 1145, 780, 700)          # east gallery
floor(520, 85, 1145, 125, 700)          # the balcony along the north wall, over the throne (the stairs arrive at its west end)
floor(600, -70, 960, 85, 700)           # the war room
table("KHHFloor", F, 5)

# ---------------------------------------------------------------- ramps: x0, z0, x1, z1, axis (0: along z, 1: along x), y at the low end, y at the high end
# the low end is at the lower coordinate along the axis when lowFirst = 1
R = []
def ramp(px0, py0, px1, py1, axis, ylo, yhi, low_is_min):
    x0, x1 = sorted((X(px0), X(px1))); z0, z1 = sorted((Z(py0), Z(py1)))
    R.append((x0, z0, x1, z1, axis, ylo if low_is_min else yhi, yhi if low_is_min else ylo))
ramp(697, 610, 870, 870, 0, 0, 300, 1)        # the great stairs: up northwards from the entrance hall
ramp(520, 230, 1145, 270, 0, 300, 390, 1)     # the step up onto the dais
ramp(690, 155, 880, 175, 0, 390, 435, 1)      # onto the throne's platform
ramp(445, 125, 520, 280, 0, 300, 700, 1)      # the west stairs: up northwards to the galleries
ramp(105, 265, 180, 450, 0, 100, 300, 0)      # the kitchen stairs: down northwards to the storeroom
table("KHHRamp", R, 7)

# ---------------------------------------------------------------- walls: x0, z0, x1, z1, y0, y1 (collision) - and the
# boxes that draw them: slot, x0, y0, z0, x1, y1, z1
W = []; B = []
T = 30
def wall(px0, py0, px1, py1, cy0, cy1, vy0=None, vy1=None, slot=1, thick=T, draw=True):
    x0, x1 = sorted((X(px0), X(px1))); z0, z1 = sorted((Z(py0), Z(py1)))
    if x0 == x1: x0 -= thick // 2; x1 += thick // 2
    if z0 == z1: z0 -= thick // 2; z1 += thick // 2
    W.append((x0, z0, x1, z1, cy0, cy1))
    if draw: B.append((slot, x0, cy0 if vy0 is None else vy0, z0, x1, cy1 if vy1 is None else vy1, z1))
FULL = (-100, 1400)
# the entrance hall: outer walls, the great doors in the south wall
wall(510, 1200, 740, 1200, *FULL, vy0=0, vy1=900); wall(820, 1200, 1060, 1200, *FULL, vy0=0, vy1=900)
wall(740, 1200, 820, 1200, 260, 1400, vy0=260, vy1=900)
wall(510, 780, 510, 1200, *FULL, vy0=0, vy1=900); wall(1060, 780, 1060, 1200, *FULL, vy0=0, vy1=900)
wall(510, 780, 620, 780, -100, 280, vy0=0, vy1=300, slot=2)       # the stone face under the great hall's floor
wall(950, 780, 1060, 780, -100, 280, vy0=0, vy1=300, slot=2)
wall(620, 780, 620, 870, -100, 280, vy0=0, vy1=300, slot=2); wall(950, 780, 950, 870, -100, 280, vy0=0, vy1=300, slot=2)
wall(620, 870, 697, 870, -100, 280, vy0=0, vy1=110, slot=2); wall(870, 870, 950, 870, -100, 280, vy0=0, vy1=110, slot=2)
# the stairwell: its stepped stone sides (ledges with braziers on them), the hall's railing round its top
for (py0, py1, h) in ((800, 870, 100), (730, 800, 200), (610, 730, 300)):
    for (px0, px1) in ((620, 697), (870, 950)):
        B.append((2, X(px0), 0, Z(py1), X(px1), h, Z(py0)))
wall(620, 610, 697, 870, -100, 600, vy0=0, vy1=300, slot=2, draw=False)
wall(870, 610, 950, 870, -100, 600, vy0=0, vy1=300, slot=2, draw=False)
wall(420, 780, 620, 780, 280, 600, vy0=300, vy1=400, slot=3); wall(950, 780, 1145, 780, 280, 600, vy0=300, vy1=400, slot=3)
wall(620, 610, 697, 610, 280, 600, vy0=300, vy1=400, slot=3); wall(870, 610, 950, 610, 280, 600, vy0=300, vy1=400, slot=3)
# the great hall's outer walls (two storeys and more), with the doorways to the kitchen and the wizard's rooms
wall(420, 780, 420, 560, *FULL, vy0=0, vy1=1400); wall(420, 520, 420, 270, *FULL, vy0=300, vy1=1400)
wall(420, 560, 420, 520, 530, 1400, vy0=530, vy1=1400)
wall(1145, 780, 1145, 590, *FULL, vy0=0, vy1=1400); wall(1145, 550, 1145, 340, *FULL, vy0=300, vy1=1400)
wall(1145, 300, 1145, 85, *FULL, vy0=300, vy1=1400)
wall(1145, 590, 1145, 550, 530, 1400, vy0=530, vy1=1400); wall(1145, 340, 1145, 300, 530, 1400, vy0=530, vy1=1400)
wall(420, 270, 420, 85, *FULL, vy0=300, vy1=1400)
wall(420, 80, 690, 80, *FULL, vy0=300, vy1=1400, slot=2); wall(880, 80, 1145, 80, *FULL, vy0=300, vy1=1400, slot=2)
wall(690, 80, 740, 80, *FULL, vy0=300, vy1=1400, slot=2); wall(820, 80, 880, 80, *FULL, vy0=300, vy1=1400, slot=2)
wall(740, 80, 820, 80, -100, 680, vy0=300, vy1=700, slot=2)        # behind the throne: stone (the war room's door above)
wall(740, 80, 820, 80, 930, 1400, vy0=930, vy1=1400)
# the west stairs' wall towards the dais, the dais' west edge; their outer side (under the gallery's edge)
wall(445, 85, 445, 280, -100, 680, vy0=300, vy1=700, slot=3)
wall(520, 90, 520, 230, 280, 680, vy0=300, vy1=480, slot=3)
# galleries: railings along their open sides; the north balcony's railing
wall(610, 270, 610, 780, 680, 1400, vy0=700, vy1=800, slot=3, draw=False)
wall(955, 270, 955, 780, 680, 1400, vy0=700, vy1=800, slot=3, draw=False)
wall(610, 125, 955, 125, 680, 1400, vy0=700, vy1=800, slot=3, draw=False)
wall(420, 780, 1145, 780, 680, 1400, draw=False)
wall(520, 125, 520, 280, 680, 1400, draw=False)                    # round the west stairs' top
# the kitchen
wall(85, 265, 85, 605, *FULL, vy0=100, vy1=900); wall(85, 605, 420, 605, *FULL, vy0=300, vy1=900)
wall(270, 265, 420, 265, *FULL, vy0=300, vy1=900); wall(180, 265, 270, 265, *FULL, vy0=300, vy1=900)
wall(180, 450, 180, 275, 280, 1400, vy0=100, vy1=900)             # beside the stairs down
wall(105, 450, 105, 275, 280, 1400, vy0=300, vy1=400, slot=3, draw=False)   # the drop beside the stairs
# the storeroom (north-west)
wall(0, 0, 270, 0, *FULL, vy0=100, vy1=600); wall(0, 0, 0, 265, *FULL, vy0=100, vy1=600)
wall(270, 0, 270, 265, *FULL, vy0=100, vy1=600); wall(0, 265, 105, 265, *FULL, vy0=100, vy1=600)
# the court wizard's rooms: outer walls, the wall to his study and bedroom (doorways), between those two
wall(1480, 265, 1480, 605, *FULL, vy0=300, vy1=900); wall(1145, 605, 1480, 605, *FULL, vy0=300, vy1=900)
wall(1145, 265, 1480, 265, *FULL, vy0=300, vy1=900)
wall(1380, 265, 1380, 380, *FULL, vy0=300, vy1=900); wall(1380, 420, 1380, 500, *FULL, vy0=300, vy1=900)
wall(1380, 540, 1380, 605, *FULL, vy0=300, vy1=900)
wall(1380, 380, 1380, 420, 530, 1400, vy0=530, vy1=900); wall(1380, 500, 1380, 540, 530, 1400, vy0=530, vy1=900)
wall(1380, 440, 1480, 440, *FULL, vy0=300, vy1=900)
# the war room
wall(600, -70, 960, -70, *FULL, vy0=700, vy1=1200); wall(600, -70, 600, 80, *FULL, vy0=700, vy1=1200)
wall(960, -70, 960, 80, *FULL, vy0=700, vy1=1200)
table("KHHWall", W, 6)
out.append("static const int KHHBox[%d] = {" % (len(B) * 7))
for b in B: out.append("    " + ", ".join(str(v) for v in b) + ",")
out.append("    };")
out.append("enum { KHHBox_N = %d };" % len(B))

# ---------------------------------------------------------------- slabs (drawn floors): slot, x0, z0, x1, z1, level
S = [(0 if lvl in (0, 300) else 4,) + f[:4] + (lvl,) for f in F for lvl in (f[4],)]
S = [(0,) + f[:5] for f in F[:4]] + [(4,) + F[4][:5], (4,) + F[5][:5], (5,) + F[6][:5], (0,) + F[7][:5], (5,) + F[8][:5]] + \
    [(0,) + f[:5] for f in F[9:]]
table("KHHSlab", S, 6)

# ---------------------------------------------------------------- pillars (x, z, base level), the long tables, rugs
P = []
for py in (900, 1050, 1140):
    for px in (693, 873): P.append((X(px), Z(py), 0))
for py in (300, 460, 610, 750):
    for px in (610, 955): P.append((X(px), Z(py), 300))
table("KHHPillar", P, 3)

# placements: what, x, y, z, yaw, scale  (what: an index the C side maps to its IN_ model)
PL = []
def put(what, px, py, lvl, yaw=0, sc=256): PL.append((what, X(px), lvl, Z(py), yaw, sc))
# 0 carpet 1 feast table 2 fire pit 3 brazier 4 throne 5 table 6 bookcase 7 banner 8 chandelier 9 enchanter
# 10 wall map 11 map table 12 hearth 13 barrel-set 14 hanging food 15 shelf 16 rug(striped)
put(0, 780, 1040, 0, 0, 256)        # the long carpet from the doors
put(0, 605, 1040, 0, 0, 200); put(0, 960, 1040, 0, 0, 200)
put(0, 515, 448, 300, 0, 300); put(0, 1050, 632, 300, 0, 300); put(0, 785, 575, 300, 256, 230)
put(2, 780, 372, 300, 0, 340)
put(1, 684, 422, 300, 0, 256); put(1, 891, 422, 300, 0, 256)
for (px, py, h) in ((660, 680, 300), (660, 765, 200), (660, 835, 100), (905, 680, 300), (905, 765, 200), (905, 835, 100)): put(3, px, py, h)
for (px, py) in ((560, 850), (1005, 850)): put(3, px, py, 0)
put(3, 715, 113, 435); put(3, 855, 113, 435)
put(4, 777, 108, 435, 512)
put(5, 1070, 150, 390); put(6, 1135, 120, 390, 768); put(6, 1135, 205, 390, 768); put(5, 620, 165, 390)
put(5, 510, 690, 300); put(5, 1050, 690, 300, 256); put(5, 1100, 330, 300)
for (px, py) in ((610, 300), (955, 300), (610, 610), (955, 610), (693, 900), (873, 900)): put(7, px, py + 6, 300 if py < 800 else 0, 512)
put(8, 780, 372, 300); put(8, 780, 560, 300); put(8, 780, 1000, 0)
# the kitchen
put(12, 245, 278, 300, 512); put(5, 270, 335, 300, 256); put(5, 265, 430, 300, 256); put(5, 275, 520, 300, 256)
put(15, 95, 520, 300, 256); put(15, 410, 300, 300, 768); put(14, 270, 400, 300); put(14, 300, 480, 300, 256)
put(16, 270, 330, 300, 0, 200)
# the storeroom
put(13, 60, 40, 100); put(13, 200, 40, 100); put(13, 40, 200, 100); put(16, 130, 140, 100, 0, 180)
# the court wizard
put(9, 1300, 330, 300); put(10, 1478, 500, 300, 768); put(5, 1290, 540, 300, 256); put(6, 1470, 320, 300, 768)
put(5, 1420, 380, 300); put(6, 1150, 420, 300, 256); put(16, 1300, 460, 300, 256, 220)
# the war room
put(11, 780, 10, 700); put(10, 780, -66, 700, 0); put(6, 610, 0, 700, 256); put(6, 950, 0, 700, 768)
table("KHHPut", PL, 6)

# props: type, x, y(level), z, yaw (sitter faces yaw+512 for seats), item/variant
PR = []
def prop(t, px, py, lvl, yaw=0, item=0): PR.append((t, X(px), lvl, Z(py), yaw, item))
# chairs at the two long tables (four a side), facing the table
for tx in (684, 891):
    for k in range(4):
        py = 360 + k * 42
        prop("PT_CHAIR", tx - 32, py, 300, 768)      # west side, sitter faces east
        prop("PT_CHAIR", tx + 32, py, 300, 256)      # east side, sitter faces west
for (px, py, yaw) in ((490, 690, 256), (530, 690, 768), (1075, 330, 768)): prop("PT_CHAIR", px, py, 300, yaw)
prop("PT_BENCH", 1030, 690, 300, 256); prop("PT_BENCH", 1070, 690, 300, 768)
for (px, py) in ((525, 1020), (525, 1110)): prop("PT_BENCH", px, py, 0, 768)
for (px, py) in ((1040, 1020), (1040, 1110)): prop("PT_BENCH", px, py, 0, 256)
# the kitchen: barrels, sacks, a pot at the spit, a cupboard
for (px, py) in ((100, 570), (130, 590), (400, 590)): prop("PT_BARREL", px, py, 300)
prop("PT_POT", 245, 300, 300); prop("PT_CUPBOARD", 220, 595, 300, 0); prop("PT_SACK", 360, 590, 300)
# the storeroom: crates, a bed, a chest
prop("PT_CRATE", 30, 30, 100); prop("PT_CRATE", 120, 25, 100); prop("PT_BED", 220, 210, 100, 0, 1); prop("PT_CHEST", 220, 150, 100, 768)
# the wizard: the alchemy lab, a chest, his bed, a wardrobe
prop("PT_ALCHEMY", 1240, 470, 300); prop("PT_CHEST", 1365, 590, 300, 0); prop("PT_BED", 1440, 470, 300, 768, 1)
prop("PT_CUPBOARD", 1470, 570, 300, 768)
# the war room: a chest, wardrobes, the locked display case with a dagger
prop("PT_CHEST", 640, -50, 700, 0); prop("PT_CUPBOARD", 930, -55, 700, 0); prop("PT_CASE", 700, -55, 700, 0)
out.append("struct HHProp { int type, x, y, z, yaw, item; };")
out.append("static const HHProp KHHProp[] = {")
for p in PR: out.append("    { %s, %d, %d, %d, %d, %d }," % p)
out.append("    };")
out.append("enum { KHHProp_N = %d };" % len(PR))

# people: x, z, level, yaw, role (0 jarl, 1 housecarl, 2 guard, 3 wizard, 4 servant)
PP = [(X(777), Z(150), 435, 512, 0), (X(840), Z(180), 390, 512, 1), (X(700), Z(240), 390, 512, 2), (X(860), Z(240), 390, 512, 2),
      (X(1290), Z(380), 300, 768, 3), (X(780), Z(820), 0, 0, 4), (X(260), Z(470), 300, 256, 4), (X(780), Z(480), 300, 0, 2)]
table("KHHPeople", PP, 5)

# the great doors inside, for the door table
out.append("enum { HH_DOOR_X = %d, HH_DOOR_Z = %d };" % (X(780), Z(1200)))
open(os.path.join(os.path.dirname(__file__), "..", "..", "src", "highhall.inc"), "w").write(
    "// generated by docs/maps/gen_highhall.py from docs/maps/ref/dragonsreach_dnd_small.png - do not edit\n" + "\n".join(out) + "\n")
print("floors", len(F), "ramps", len(R), "walls", len(W), "boxes", len(B), "pillars", len(P), "placements", len(PL), "props", len(PR))
