# Draws our reading of the Whiterun layout (coordinates = pixels of the user's reference map, 1500x1935,
# north up; the outside, west of the gate, at negative x). Output: docs/maps/vindgard_plan_draft.png
import math
from PIL import Image, ImageDraw, ImageFont
OX, OY = 1300, 30                      # canvas offset of map (0,0)
W, H = 2860, 2660
M_PER_PX = 0.085                        # assumed scale: 10 m = ~118 px
img = Image.new("RGB", (W, H), (201, 191, 143))
d = ImageDraw.Draw(img)
F = lambda s, b=False: ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans%s.ttf" % ("-Bold" if b else ""), s)
def P(x, y): return (x + OX, y + OY)
def poly(pts, fill, outline=None, w=1): d.polygon([P(*p) for p in pts], fill=fill, outline=outline, width=w)
def line(pts, col, w): d.line([P(*p) for p in pts], fill=col, width=w, joint="curve")
def circ(x, y, r, fill, outline=(60, 60, 60), w=2):
    X, Y = P(x, y); d.ellipse([X - r, Y - r, X + r, Y + r], fill=fill, outline=outline, width=w)
def rect(cx, cy, w, h, rot, fill, outline=(40, 40, 40)):
    a = math.radians(rot); c, s = math.cos(a), math.sin(a)
    pts = [(cx + dx * c - dy * s, cy + dx * s + dy * c) for dx, dy in ((-w/2, -h/2), (w/2, -h/2), (w/2, h/2), (-w/2, h/2))]
    poly(pts, fill, outline, 3)
def text(x, y, s, size=22, col=(20, 20, 20), bold=True, anchor="mm"):
    d.text(P(x, y), s, fill=col, font=F(size, bold), anchor=anchor, stroke_width=3, stroke_fill=(255, 255, 255))
def stairs(x0, y0, x1, y1, wid, n=8):
    # a flight from (x0,y0) up to (x1,y1), wid pixels wide, drawn as treads
    dx, dy = x1 - x0, y1 - y0; L = math.hypot(dx, dy); ux, uy = dx / L, dy / L; px, py = -uy, ux
    poly([(x0 + px*wid/2, y0 + py*wid/2), (x1 + px*wid/2, y1 + py*wid/2), (x1 - px*wid/2, y1 - py*wid/2), (x0 - px*wid/2, y0 - py*wid/2)], (205, 200, 190), (60, 60, 60), 2)
    for k in range(1, n):
        t = k / n; cx, cy = x0 + dx * t, y0 + dy * t
        line([(cx + px*wid/2, cy + py*wid/2), (cx - px*wid/2, cy - py*wid/2)], (90, 90, 90), 2)
    # arrow tip shows "up"
    line([(x1 - ux*14 + px*10, y1 - uy*14 + py*10), (x1, y1), (x1 - ux*14 - px*10, y1 - uy*14 - py*10)], (150, 20, 20), 4)

LOW, MID, POOL, JOR, ROCK, TOP = (232, 220, 192), (214, 230, 192), (205, 225, 232), (196, 220, 172), (200, 195, 182), (242, 216, 160)
WATER = (95, 160, 214)
# ---- levels (ground inside the walls)
poly([(40,1215),(1030,1255),(990,1380),(955,1530),(930,1680),(850,1815),(800,1860),(490,1860),(370,1812),(235,1745),(75,1660),(40,1560)], LOW)
poly([(40,1240),(15,1020),(60,905),(110,710),(245,655),(600,660),(650,700),(855,735),(1000,745),(1000,1250),(40,1215)], MID)
poly([(600,660),(720,545),(815,490),(990,660),(855,735),(650,700)], POOL)
poly([(1000,745),(1150,760),(1200,900),(1355,850),(1365,1085),(1305,1170),(1155,1245),(1000,1250)], JOR)
poly([(1135,665),(1220,615),(1330,520),(1450,600),(1450,880),(1355,850),(1200,900),(1150,760)], ROCK)
poly([(815,490),(775,355),(830,235),(940,125),(1165,40),(1330,150),(1430,280),(1430,420),(1330,520),(1220,615),(1135,665),(990,660)], TOP)
# ---- water
line([(125,700),(125,1215),(90,1240),(90,1560)], WATER, 26)    # the west canal, as on the map: down past the gate's bridge
for x in (655, 745): line([(x,715),(x,1480)], WATER, 16)
poly([(640,640),(860,640),(860,740),(640,740)], WATER)
poly([(760,560),(880,470),(1070,600),(990,660),(860,700)], WATER)            # the falls and the moat under Highhall
circ(230, 950, 30, WATER); circ(420, 960, 22, WATER); circ(700, 1490, 30, WATER)
# ---- outside, after the online map's detail of the approach (its pixels -> ours: 2.35x, its gate (575,450) = our (20,1470))
def Q(x, y): return (20 + (x - 575) * 2.35, 1470 + (y - 450) * 2.35)
def qline(pts, col, w): line([Q(*p) for p in pts], col, w)
# the stream: falls from under the city's west side, runs west past the drawbridge and down to the pond
qline([(575,300),(520,300),(400,295),(330,330),(230,355),(160,380),(120,450),(95,520)], WATER, 20)
X, Y = Q(80, 545); circ(X, Y, 95, WATER, WATER)
qline([(60,600),(30,700),(0,760)], WATER, 16)
# the road (the red line): up from the river road past the carriage and the stables, north along the west side,
# over the drawbridge, east along the top, down through the inner gatehouse and east into the city gate
roadpts = [(455,940),(455,900),(440,790),(400,640),(330,520),(315,420),(305,330),(320,230),(370,200),(450,205),(470,260),(475,380),(480,440),(575,450)]
qline(roadpts, (150, 130, 100), 46); qline(roadpts, (190, 175, 140), 36)
qline([(0,880),(200,860),(455,870),(681,880)], (190, 175, 140), 40)          # the river road east-west
# the approach's walls
for w in ([(240,300),(240,500),(420,500)], [(470,500),(575,500)], [(290,250),(290,140),(520,140),(560,180),(575,180)],
          [(420,300),(420,500)], [(240,300),(290,300)], [(420,275),(455,275)], [(505,275),(575,275)]):
    qline(w, (90, 90, 90), 12)
X, Y = Q(310, 335); rect(X, Y, 70, 110, 0, (150, 105, 60)); text(X + 110, Y, "drawbridge", 20)
X, Y = Q(322, 500); rect(X, Y, 110, 34, 0, (110, 110, 110)); text(X, Y + 40, "outer gate", 20)
X, Y = Q(480, 275); rect(X, Y, 110, 34, 0, (110, 110, 110)); text(X + 150, Y, "inner gate", 20)
# the stables east of the road, the carriage, the caravan west of it, the farm fields
X, Y = Q(485, 655); rect(X, Y, 190, 190, 40, (120, 140, 160)); text(X, Y, "S1", 26)
X, Y = Q(535, 715); rect(X, Y, 90, 190, 0, (150, 115, 80)); text(X, Y, "S2", 26)
X, Y = Q(455, 790); circ(X, Y, 26, (200, 150, 60)); text(X + 80, Y, "S4 carriage", 20)
X, Y = Q(255, 590)
for (dx, dy, r) in ((-60, 0, 34), (10, 60, 34), (60, -10, 48)): circ(X + dx, Y + dy, r, (196, 160, 110))
circ(X + 5, Y + 15, 10, (230, 110, 30), (120, 40, 10)); text(X, Y - 70, "S3 caravan", 22)
for (x, y) in ((265, 725), (345, 850)):
    X, Y = Q(x, y); rect(X, Y, 150, 200, 0, (95, 110, 70))
# ---- walls and towers
WALL = (85, 85, 85)
outer = [(40,1420),(40,1240),(15,1020),(60,905),(110,710),(245,655),(600,660),(720,545),(815,490),(775,355),(830,235),(940,125),(1165,40),
         (1330,150),(1430,280),(1430,420),(1330,520)]
outer2 = [(1355,850),(1365,1085),(1305,1170),(1155,1245),(1030,1255),(990,1380),(955,1530),(930,1680),(850,1815),(800,1860),(645,1868),
          (490,1860),(370,1812),(235,1745),(75,1660),(40,1560),(40,1520)]
line(outer, WALL, 14); line(outer2, WALL, 14)
line([(40,1215),(650,1245)], WALL, 12); line([(750,1245),(1030,1255)], WALL, 12)                  # the tier wall
line([(815,490),(990,660),(1135,665),(1220,615),(1330,520)], WALL, 12)                             # round Highhall's terrace
line([(855,735),(1000,745),(1000,1010)], WALL, 10); line([(1000,1070),(1000,1250)], WALL, 10)      # Skjoldhall's terrace
for (x, y) in outer[1:] + outer2[:-2] + [(185,1215),(400,1225),(530,1240),(650,1245),(750,1245),(990,660),(1135,665)]:
    circ(x, y, 14, (225, 225, 220))
for (x, y) in ((1305,1170),(955,1530),(645,1868),(235,1745)): rect(x, y, 46, 70, 0, (95, 125, 155))
rect(30, 1470, 60, 120, 0, (130, 130, 130)); text(30, 1470, "1", 26, (255, 255, 255))
rect(122, 1470, 84, 40, 0, (215, 205, 185))      # the bridge over the canal just inside the gate
# ---- stairs (arrow = up)
stairs(700, 1390, 700, 1252, 80, 12)        # wide stairs, market -> Wind District
stairs(790, 1440, 835, 1440, 34, 4)         # market -> the inn
stairs(235, 1300, 235, 1225, 34, 6)         # small west stairs
# Highhall's stairs: four flights from the Wind District, the upper three on a ridge of arched masonry that the water
# runs through (falls out of the arches) and round (falls down the rocks either side) into the pool below
line([(745,712),(745,628),(865,552),(1000,438)], (128, 122, 114), 92)
for (x, y) in ((712,672),(778,672),(800,632),(830,612),(845,575),(905,540),(935,520),(960,560),(1040,520)):
    X, Y = P(x, y); d.rectangle([X - 7, Y - 14, X + 7, Y + 14], fill=(225, 240, 250), outline=(120, 170, 210))
stairs(700, 885, 700, 765, 80, 10)          # flight 1: the tree plaza -> the pool platform
stairs(745, 705, 745, 638, 46, 6)           # flight 2: up the masonry
stairs(770, 620, 858, 557, 48, 7)           # flight 3: turning north-east along it
stairs(882, 538, 990, 446, 56, 9)           # flight 4: over the moat to Highhall's doors
for (k, x, y) in ((1, 640, 830), (2, 690, 670), (3, 790, 560), (4, 905, 470)): text(x, y, "F%d" % k, 20, (120, 20, 20))
stairs(960, 1040, 1020, 1040, 60, 5)        # bridge-stair up to Skjoldhall's terrace
stairs(450, 840, 450, 800, 40, 4)
# ---- buildings: (label, cx, cy, w, h, rot, colour)
HOUSE, HALL, SPECIAL = (110, 140, 165), (150, 115, 80), (190, 110, 70)
B = [("2",160,1310,110,100,0,HOUSE),("13",205,1555,120,110,0,HOUSE),("3",345,1335,115,90,-25,SPECIAL),("4",560,1355,140,100,0,HOUSE),
     ("12",360,1580,100,140,0,SPECIAL),("10",590,1580,150,110,15,HOUSE),("8",800,1640,90,130,0,HOUSE),("11",380,1730,85,70,0,HALL),
     ("9",690,1760,90,80,-25,HOUSE),("6",850,1430,100,210,0,HALL),("7",955,1530,40,60,-10,HOUSE),
     ("15",570,920,170,130,45,HOUSE),("16",835,1185,100,90,30,HOUSE),("20",510,1110,110,190,-20,HALL),("22",385,1000,120,100,-20,HOUSE),
     ("21",330,1110,110,100,-20,HOUSE),("23",130,1100,120,110,0,HALL),("24",270,820,150,150,30,HOUSE),("25",450,730,120,100,0,HOUSE),
     ("18",1120,1050,210,300,0,HOUSE),("26",1110,290,320,430,35,HOUSE)]
for (lab, x, y, w, h, r, col) in B:
    rect(x, y, w, h, r, col); text(x, y, lab, 30)
for (x, y) in ((630,1400),(765,1385),(770,1455),(590,1460)): rect(x, y, 40, 30, 0, (200, 70, 60))
text(560, 1500, "5", 30)
circ(725, 1060, 105, (120, 105, 80)); circ(725, 1060, 40, (80, 130, 70)); text(725, 1060, "14", 30, (255, 255, 255))
circ(840, 860, 34, (170, 170, 165)); text(840, 860, "17", 26)
rect(125, 1525, 50, 34, 0, (60, 60, 60)); text(125, 1490, "forge", 18)
rect(1240, 790, 60, 44, 20, (60, 60, 60)); text(1240, 790, "19", 26, (255, 255, 255))
# ---- grid and scale
for k in range(0, 30):
    x = -1300 + k * 118
    d.line([P(x, -30), P(x, -10)], fill=(90, 90, 90), width=2)
X0, Y0 = P(1150, 1650)
d.rectangle([X0, Y0, X0 + 118, Y0 + 12], fill=(30, 30, 30)); d.text((X0, Y0 - 34), "10 m (assumed)", fill=(20, 20, 20), font=F(22, True))
# ---- legend
L = ["VINDGARD / WHITERUN  -  draft for correction", "",
     "Levels: lower district (sand) < Wind District (green) < pool terrace (blue)",
     "  < Skjoldhall terrace (dark green) < Skyforge rock (grey) < Highhall (gold)", "",
     "1  Main gate (entered from the west)          14 Gildergreen / Elder Ash",
     "2  Guard barracks (left of the entry)        15 Temple of Kynareth",
     "13 Warmaiden's + forge (right of the entry)  16 Heimskr's house",
     "3  The Drunken Huntsman                       17 Shrine of Talos",
     "4  Severio Pelagia's house                    18 Jorrvaskr / Skjoldhall",
     "5  Marketplace (well, stalls)                 19 Skyforge",
     "6  Bannered Mare (stairs up from the market)  20 House Gray-Mane",
     "7  East guard barracks (wall gatehouse)       21 Uthgerd's house",
     "8  Arcadia's Cauldron (alchemist)             22 Amren's house",
     "9  Ysolda's house                             23 Carlotta Valentia's house",
     "10 Belethor's General Goods                   24 House Battle-Born",
     "11 Olava the Feeble's house                   25 Hall of the Dead",
     "12 Breezehome                                 26 Dragonsreach / Highhall", "",
     "S1 Whiterun Stables   S2 stable block   S3 Khajiit caravan   S4 carriage (fast travel)",
     "Stairs: arrow points UP. F1-F4: four flights from the Wind District up to Dragonsreach,",
     "F2-F4 on arched masonry, water falling through the arches and round it; wide stairs market -> Wind District;",
     "small stairs market -> Bannered Mare; bridge-stair up to Jorrvaskr."]
for i, s in enumerate(L):
    d.text((40, 40 + i * 32), s, fill=(20, 20, 20), font=F(26 if i == 0 else 21, i == 0))
img.save("/home/claude/game/docs/maps/vindgard_plan_draft.png")
print("ok")
