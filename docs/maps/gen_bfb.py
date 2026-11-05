# Bleak Falls Barrow - the three interior cells, generated from the tabletop maps (mirhayasu, docs/maps/ref/bfb):
#   entry   (bfb_barrow_entry_grid.png, 62 x 25 squares): the entry hall where the bandits camp (2), the passages,
#           the pillar puzzle room (3) and the room with the spiral stairs down        -> CELL_BFTEMPLE
#   temple  (bfb_temple_grid.png, 60 x 50): the skeever cave, the spider's lair (4), the south burial passage (5),
#           the north burial passage (6), the waterfall cavern (7), the sunlit cavern (8), the sanctum access (9)
#                                                                                    -> CELL_BFCRYPT
#   sanctum (bfb_sanctum_grid.png, 106 x 30): the first chamber (10), the claw door, the main chamber with the
#           word wall (11), the way out (12)                                          -> CELL_BFSANCTUM
# A map square is 1.5 m (5 ft). The walkable area is sampled from the map on a 0.75 m grid (bfb_mask.py) and
# then shaped by hand below: levels, stairs, walls that are not on the map, doorways. Positions here are in map
# squares (column from the left, row from the top, as on the maps); the game's frame is x east, z north, cm.
# Output: src/bfb.inc
import os, sys
sys.path.insert(0, os.path.dirname(__file__))
from bfb_mask import mask, MAPS, SUB

SQ = 150                         # cm per map square
S = SQ // SUB                    # cm per grid sample (75)

# zone styles
RUIN, CAVE, WATER, STONE = 0, 1, 2, 3
# ramps rise towards: E, W, N, S
FLAT, RE, RW, RN, RS = 0, 1, 2, 3, 4

class Cell:
    def __init__(self, name, cname, default):
        self.name = name; self.cname = cname
        self.m = mask(name)
        self.H = len(self.m); self.W = len(self.m[0])
        self.ci = self.W // 2; self.cj = self.H // 2      # grid centre = the cell's origin
        self.zones = []                                    # (kind, y0, y1, ceil, style, x0, z0, x1, z1)
        self.zid = [[0] * self.W for _ in range(self.H)]
        self.default = self.zone(*default)
        for j in range(self.H):
            for i in range(self.W):
                if self.m[j][i]: self.zid[j][i] = self.default
        self.put = []; self.props = []; self.ents = []; self.lights = []; self.fix = []
    # map squares -> cm in the cell frame
    def X(self, c): return int(round((c * SUB - self.ci) * S))
    def Z(self, r): return int(round((self.cj - r * SUB) * S))
    def zone(self, kind, y0, y1, ceil, style, rect=None):
        x0 = z0 = x1 = z1 = 0
        if rect:
            c0, r0, c1, r1 = rect
            x0, x1 = self.X(c0), self.X(c1); z0, z1 = self.Z(r1), self.Z(r0)
        self.zones.append((kind, y0, y1, ceil, style, x0, z0, x1, z1))
        return len(self.zones)
    def subs(self, c0, r0, c1, r1):
        for j in range(max(0, int(round(r0 * SUB))), min(self.H, int(round(r1 * SUB)))):
            for i in range(max(0, int(round(c0 * SUB))), min(self.W, int(round(c1 * SUB)))):
                yield i, j
    # a flat area (walkable from now on), or only where the map has floor (mapped=True)
    def flat(self, c0, r0, c1, r1, y, ceil, style=RUIN, mapped=False):
        z = self.zone(FLAT, y, y, ceil, style)
        for i, j in self.subs(c0, r0, c1, r1):
            if not mapped or self.m[j][i]: self.zid[j][i] = z
        return z
    def ramp(self, c0, r0, c1, r1, kind, ylo, yhi, ceil, style=RUIN):
        # the low end is the side opposite the direction it rises towards; the slope runs over exactly the
        # samples it owns, so it meets the floors either side without a lip
        q = lambda v: round(v * SUB) / SUB
        z = self.zone(kind, ylo, yhi, ceil, style, (q(c0), q(r0), q(c1), q(r1)))
        for i, j in self.subs(c0, r0, c1, r1): self.zid[j][i] = z
        return z
    def solid(self, c0, r0, c1, r1):
        for i, j in self.subs(c0, r0, c1, r1): self.zid[j][i] = 0
    # placements: decor (static models), props, creatures and people, lights, fixtures (gates, traps, puzzles)
    def place(self, what, c, r, y, yaw=0, sc=256): self.put.append((what, self.X(c), y, self.Z(r), yaw, sc))
    def prop(self, t, c, r, y, yaw=0, item=0): self.props.append((t, self.X(c), y, self.Z(r), yaw, item))
    def ent(self, t, c, r, y, yaw, var=0, pad1=0, pad2=0): self.ents.append((t, self.X(c), y, self.Z(r), yaw, var, pad1, pad2))
    def light(self, c, r, y, rad, s): self.lights.append((self.X(c), y, self.Z(r), rad, s))
    def fixture(self, kind, c, r, y, yaw=0, a=0, b=0): self.fix.append((kind, self.X(c), y, self.Z(r), yaw, a, b))
    def point(self, c, r): return (self.X(c), self.Z(r))

# decor models (the C side maps these to IN_* / its own models)
D_BRAZIER, D_TORCH, D_FIRE, D_BEDROLL, D_URNS, D_COFFIN, D_PILLAR, D_TABLE, D_WEB, D_EGGS, D_PLAQUE, \
D_DOOR, D_WALLCOFFIN, D_BONES, D_WATERFALL, D_RUBBLE, D_SPIRAL, D_LOGS, D_BARREL, D_SACKS, D_ARCH, D_MURAL = range(22)
# prop types (game.h PT_*)
PT_ITEM, PT_BARREL, PT_SACK, PT_CHEST, PT_LEVER, PT_PILLAR, PT_PULLCHAIN, PT_CLAWDOOR, PT_WORDWALL = \
    'PT_ITEM', 'PT_BARREL', 'PT_SACK', 'PT_CHEST', 'PT_LEVER', 'PT_PILLAR', 'PT_PULLCHAIN', 'PT_CLAWDOOR', 'PT_WORDWALL'
# fixtures: things that move or hurt
F_GATE, F_PLATE, F_SPIKEWALL, F_BLADE, F_OIL, F_LANTERN, F_DARTS, F_DOORPT, F_COFFINLID, F_TRIGGER = range(10)

cells = []

# ======================================================================== the entry: Bleak Falls Temple, upper
e = Cell('entry', 'BFTEMPLE', (FLAT, -300, -300, 450, RUIN)); cells.append(e)
e.solid(0, 0, 14, 15.5)                                   # the pillar room complex is laid out by hand below
e.solid(13, 0, 45, 4.6)                                   # the edge of the cave below (another cell)
e.flat(43.5, 12, 60, 22.6, 0, 900, RUIN, mapped=True)    # (2) the entry hall
e.flat(43, 13, 60.2, 22, 0, 900, RUIN, mapped=True)
e.ramp(39.4, 16, 43.6, 18.1, RE, -150, 0, 360)           # down from the hall
e.flat(30, 16, 39.4, 18.1, -150, 340, RUIN, mapped=True)
e.flat(30, 18, 33.1, 22.1, -150, 340, RUIN, mapped=True)
e.ramp(27.8, 20, 30, 22.1, RE, -300, -150, 360)
e.flat(23, 16, 27.8, 22.1, -300, 340, RUIN, mapped=True)
e.solid(24.5, 22.1, 28, 25)                               # the side stairs down are fallen in
e.solid(16.5, 14, 19.5, 16); e.solid(16.5, 18.1, 19.5, 20)    # the side doors (closed, set in the walls)
e.flat(8, 16, 25, 18.1, -300, 340, RUIN, mapped=True)
e.flat(8, 16, 12.2, 21.2, -300, 340, RUIN, mapped=True)
e.ramp(8, 13.6, 10.1, 16.6, RN, -300, -100, 420)       # up into the pillar room
# (3) the pillar room: main floor, the gate's passage, the balcony with its stairs, the niches of the pillars
e.flat(4, 7.5, 11.1, 13.6, -100, 700)
e.flat(6, 5.4, 8, 7.5, -100, 450)
e.flat(8.5, 4.6, 11.1, 7.5, 200, 400)
e.ramp(9.6, 7.5, 11.1, 11.4, RN, -100, 200, 420)
e.flat(3.5, 8.5, 4, 9.5, -100, 300); e.flat(3.5, 10, 4, 11, -100, 300); e.flat(3.5, 11.5, 4, 12.5, -100, 300)
# the room of the spiral stairs (the way down to the rest of the temple)
e.flat(2, 1, 8.5, 5.4, -100, 450)
e.solid(8, 4.6, 8.5, 7.5)                                 # between the gate's passage and the balcony
# --- the bandits' camp in the hall
e.place(D_FIRE, 50.5, 17, 0)
e.light(50.5, 17, 120, 1100, 220)
e.place(D_BEDROLL, 47.5, 14.2, 0, 256); e.place(D_BEDROLL, 53.5, 20.3, 0, 768)
e.place(D_LOGS, 49, 19.8, 0, 100); e.place(D_URNS, 45, 21.2, 0); e.place(D_URNS, 58.8, 12.8, 0, 300)
e.place(D_WALLCOFFIN, 58.2, 20.6, 0, 768); e.place(D_RUBBLE, 46, 13, 0, 200); e.place(D_RUBBLE, 57, 21.5, 0, 700)
e.place(D_PILLAR, 55, 14, 0, 300); e.place(D_PILLAR, 48, 20.5, 0, 40)
for c, r in ((44.5, 13), (44.5, 21.6), (58.6, 13), (58.6, 21.6)):
    e.place(D_BRAZIER, c, r, 0); e.light(c, r, 140, 800, 150)
e.prop(PT_CHEST, 56.5, 13.3, 0, 512, 1)
e.ent('E_BANDIT', 51.6, 15.8, 0, 512, 0, 0, 1)            # the two who talk about Arvel
e.ent('E_BANDIT', 51.4, 18.4, 0, 0, 1, 0, 1)
e.ent('E_SKEEVER', 48, 19, 0, 300, 0, 0, 2)               # dead skeevers: a warning
e.ent('E_SKEEVER', 54.5, 15.2, 0, 700, 1, 0, 2)
# --- the passages
for c, r, yaw in ((36, 16.1, 0), (31, 21.9, 512), (24, 16.1, 0), (14, 16.1, 0), (10, 21.1, 512)):
    e.place(D_TORCH, c, r, -300 if c < 28 else -150, yaw); e.light(c, r + (0.6 if yaw == 0 else -0.6), (-300 if c < 28 else -150) + 200, 650, 130)
e.place(D_DOOR, 18, 16.05, -300, 0); e.place(D_DOOR, 18, 18.05, -300, 512)
for c, y in ((34.5, -150), (37.5, -150), (11, -300), (14.5, -300), (22, -300)):
    e.place(D_ARCH, c, 17.05, y, 256, 268)                # heavy round arches over the passage
e.place(D_URNS, 11.6, 20.6, -300); e.place(D_BONES, 26.5, 21.5, -300, 300)
# --- the pillar room
e.prop(PT_PILLAR, 3.85, 9, -100, 256, 0)                  # three pillars in their niches (west wall), turned with E
e.prop(PT_PILLAR, 3.85, 10.5, -100, 256, 1)
e.prop(PT_PILLAR, 3.85, 12, -100, 256, 2)
e.place(D_PLAQUE, 5.3, 7.55, 230, 512, 0)                 # the answer above the gate: snake, (snake, fallen), whale
e.place(D_PLAQUE, 6.2, 8.4, -100, 420, 1)                # the middle one has fallen to the floor
e.place(D_PLAQUE, 8.7, 7.55, 230, 512, 2)
e.prop(PT_LEVER, 7.0, 8.5, -100, 512, 1)
e.fixture(F_GATE, 7.0, 7.45, -100, 0, 0)                  # gate 0: the iron gate of the pillar room
e.fixture(F_DARTS, 7.0, 9.3, -100, 0, 0)                  # the dart trap's kill zone before the gate
e.ent('E_BANDIT', 7.4, 9.6, -100, 0, 2, 0, 4)             # the bandit who pulls the wrong lever
for c, r in ((4.6, 13.2), (10.5, 12.9)):
    e.place(D_BRAZIER, c, r, -100); e.light(c, r, 40, 800, 150)
e.place(D_TABLE, 9.3, 5.2, 200, 0); e.prop(PT_ITEM, 9.3, 5.2, 280, 120, 'IT_BOOK_THIEF')
e.prop(PT_CHEST, 10.5, 5.1, 200, 512, 2)
e.light(9.8, 6, 400, 600, 110)
e.place(D_SPIRAL, 3.8, 2.8, -100, 0)
e.place(D_BRAZIER, 7.5, 1.6, -100); e.light(7.5, 1.6, 40, 700, 140)
e.place(D_RUBBLE, 2.6, 4.8, -100, 500)
e.ent('E_SKEEVER', 6.8, 3.2, -100, 300, 0, 0, 0)
# doors: the way in (from the terrace) and the spiral stairs down
e.door_in = (e.X(59.6), e.Z(17.1), 768)
e.door_down = (e.X(4.6), e.Z(3.0), 256)

# ======================================================================== the temple below
t = Cell('temple', 'BFCRYPT', (FLAT, 0, 0, 600, CAVE)); cells.append(t)
t.solid(5, 44, 15, 50)                                   # the spiral room (it belongs to the entry)
t.flat(18, 41, 48, 48.6, 0, 520, CAVE, mapped=True)     # the skeever cave
t.ramp(18, 44, 21, 46.6, RW, 0, 150, 450, CAVE)
t.flat(42, 41.4, 44, 42.8, 0, 420)                       # doorway into the spider's lair
t.flat(31, 34, 45, 42, 0, 620, RUIN, mapped=True)       # (4) the spider's lair
t.flat(27, 36, 31.1, 39.1, 0, 450, RUIN, mapped=True)
t.flat(20, 33, 27, 40.2, 0, 520, RUIN, mapped=True)
t.flat(14, 32, 20.1, 39.2, 0, 520, RUIN, mapped=True)
t.flat(9, 31.6, 14.1, 36, 0, 450, CAVE, mapped=True)
t.flat(2, 19, 9.1, 35.6, 0, 480, RUIN, mapped=True)     # (5) the south burial passage
t.ramp(5.9, 20, 8.3, 22.1, RE, 0, 150, 420)             # (6) the north burial passage: up and back
t.flat(8.3, 20, 15.6, 22.1, 150, 380)
t.ramp(15.6, 20, 19.6, 22.1, RE, 150, 400, 400)
t.flat(19.6, 18, 21.1, 22.1, 400, 360)
t.flat(9, 18, 19.6, 20, 400, 340)                        # the gallery with the swinging blades
t.solid(11, 17.8, 18.4, 18.2)                             # its wall to the passage behind
t.flat(9, 16, 11.1, 20, 400, 340)
t.flat(9, 16, 21.1, 17.8, 400, 340, RUIN, mapped=True)
t.flat(12, 15.6, 14.1, 16.4, 400, 340)
t.flat(11, 11, 16.1, 16, 400, 520, RUIN, mapped=True)
t.flat(16, 12.4, 33.1, 16.6, 400, 460, CAVE, mapped=True)
t.flat(33, 9, 39.2, 15.6, 400, 760, CAVE, mapped=True)   # (7) the waterfall cavern
t.ramp(34, 15.6, 37.6, 20, RN, 100, 400, 520, CAVE)      # down to the stream (the gate at its top)
t.flat(31, 18, 59, 38, 100, 720, CAVE, mapped=True)     # (8) the sunlit cavern
t.ramp(34, 15.6, 37.6, 20, RN, 100, 400, 520, CAVE)
for c0, r0, c1, r1 in ((33.6, 20, 37.2, 27.4), (37, 27, 48.4, 29.8), (48, 27, 56.6, 31), (52.8, 31, 56.6, 36.6), (48, 34.4, 53, 36.6)):
    t.flat(c0, r0, c1, r1, 100, 760, WATER, mapped=True)
t.flat(34, 20, 37.6, 21, 100, 720, WATER)                 # the stream at the foot of the stairs (the map draws a line)
t.flat(47, 16, 55.6, 19.6, 100, 520, CAVE, mapped=True)  # the bend where the path from the stream turns in
t.ramp(47.6, 13, 52.2, 16, RN, 100, 300, 520, CAVE)      # up to the sanctum access
t.flat(45, 1, 53.2, 13, 300, 620, RUIN, mapped=True)    # (9)
# --- skeevers in the cave, the spider's lair
for c, r, yaw in ((26, 44.6, 256), (33, 44.4, 768), (38.5, 44.2, 512)):
    t.ent('E_SKEEVER', c, r, 0, yaw, 0, 0, 0)
for c, r in ((22, 43.6), (30, 46.5), (40, 42.6), (46.5, 44.5)):
    t.place(D_TORCH if c > 35 else D_BRAZIER, c, r, 0, 512 if c > 35 else 0); t.light(c, r, 160, 700, 130)
for c, r, yaw in ((33, 35, 0), (36, 41.2, 300), (41, 35.5, 700), (44, 39, 200), (32, 39.8, 500), (38.5, 36.4, 900), (34.5, 40, 100)):
    t.place(D_WEB, c, r, 0, yaw)
t.place(D_EGGS, 43.5, 35, 0, 0); t.place(D_EGGS, 32.5, 41, 0, 300)
t.place(D_TABLE, 37.5, 39.5, 0, 256); t.prop(PT_ITEM, 37.3, 39.6, 80, 200, 'IT_POT_MHEAL'); t.prop(PT_ITEM, 37.8, 39.3, 80, 600, 'IT_POT_MHEAL')
t.light(37.5, 39.5, 150, 800, 110); t.light(32, 37.5, 200, 700, 80)
t.ent('E_SPIDER', 38, 37.5, 0, 768, 0, 0, 8)             # hangs in the dark until you come in
t.ent('E_BANDIT', 31.8, 37.6, 0, 256, 3, 1, 16)           # Arvel the Swift, stuck in the web
t.place(D_WEB, 31.4, 37.6, 0, 256, 300)
t.arvel_path = [t.point(*p) for p in ((30, 37.5), (25, 36.5), (21, 36.2), (17, 35.6), (13.6, 33.2), (11.6, 33.1), (8, 33.4))]
# --- the south burial passage: wall coffins, draugr asleep in three of them, the plate and the spike wall
for c, r, yaw in ((2.3, 23, 256), (2.3, 26.5, 256), (2.3, 30, 256), (2.3, 33.4, 256), (8.8, 24.8, 768), (8.8, 28.2, 768), (8.8, 31.6, 768)):
    t.place(D_WALLCOFFIN, c, r, 0, yaw)
t.ent('E_WIGHT', 2.9, 26.5, 0, 256, 0, 0, 32)
t.ent('E_WIGHT', 8.2, 28.2, 0, 768, 0, 0, 32)
t.ent('E_WIGHT', 2.9, 30, 0, 256, 0, 0, 32)
for c, r in ((5.5, 34.7), (3, 20), (8.4, 19.6)):
    t.place(D_BRAZIER, c, r, 0); t.light(c, r, 140, 800, 140)
t.fixture(F_PLATE, 11.6, 33.1, 0, 0, 0)                   # plate 0 -> spike wall 0
t.fixture(F_SPIKEWALL, 11.6, 32.0, 0, 0, 0)
t.place(D_BONES, 6, 33.5, 0, 200); t.place(D_URNS, 3, 34.6, 0)
# --- the north burial passage: blades in the gallery, the chain past them; the oil in the cave beyond
for c in (17.2, 14.8, 12.4):
    t.fixture(F_BLADE, c, 19, 400, 256, 0)               # group 0, swinging until the chain is pulled
t.prop(PT_PULLCHAIN, 9.6, 17.0, 400, 256, 0)              # chain 0: stops blade group 0
t.place(D_TORCH, 20.9, 20, 400, 768); t.light(20.4, 20, 600, 700, 140)
t.place(D_TORCH, 9.1, 18.5, 400, 256); t.light(9.6, 18.5, 600, 600, 120)
t.place(D_BRAZIER, 12, 21.85, 150); t.light(12, 21.6, 290, 700, 120)
for c in (10, 14.4):
    t.place(D_ARCH, c, 21.05, 150, 256, 268)
for c in (13.6, 16.0, 18.6):
    t.place(D_ARCH, c, 19.0, 400, 256, 256)                # the gallery of the blades: a tunnel of arches
t.ent('E_WIGHT', 13.5, 12.6, 400, 0, 1, 0, 0)
t.place(D_COFFIN, 14.5, 14.5, 400, 256); t.place(D_URNS, 11.6, 11.6, 400)
t.place(D_BRAZIER, 15.5, 11.6, 400); t.light(15.5, 11.6, 540, 800, 140)
t.fixture(F_OIL, 22.5, 14.5, 400, 0, 0, 160)              # oil 0 (radius cm)
t.fixture(F_LANTERN, 22.5, 14.5, 400 + 260, 0, 0)         # its lantern
t.ent('E_WIGHT', 23.2, 14.8, 400, 768, 2, 0, 0)
t.light(22.5, 14.5, 640, 500, 90)
# --- the waterfall cavern: a chest, the chain that opens the gate down to the stream
t.prop(PT_CHEST, 37.8, 10.2, 400, 768, 3)
t.prop(PT_PULLCHAIN, 33.25, 13.8, 400, 256, 1)           # chain 1: opens gate 1
t.fixture(F_GATE, 35.8, 16.0, 400, 0, 1)
t.place(D_WATERFALL, 36, 9.4, 400, 0); t.light(36, 11, 600, 1000, 160)
t.light(30, 14.5, 600, 700, 100)
# --- the sunlit cavern: daylight from above, a chest at the end of the water, one draugr
t.light(43, 24, 650, 1500, 200); t.light(54, 25, 650, 1400, 180); t.light(40, 33, 600, 1300, 170)
t.prop(PT_CHEST, 51, 35.6, 100, 0, 4)
t.ent('E_WIGHT', 44, 23, 100, 512, 3, 0, 0)
t.place(D_BONES, 41, 23.5, 100, 400)
# --- the sanctum access: a Restless Draugr, a chest, the door
t.ent('E_WIGHT', 49, 6, 300, 512, 4, 1, 0)
t.prop(PT_CHEST, 46.2, 2.2, 300, 512, 5)
for c, r in ((45.8, 11.6), (52.4, 11.6), (46, 3.6), (52.2, 3.6)):
    t.place(D_BRAZIER, c, r, 300); t.light(c, r, 440, 750, 140)
for c, r, yaw in ((45.3, 6.5, 256), (52.9, 6.5, 768), (45.3, 9, 256), (52.9, 9, 768)):
    t.place(D_WALLCOFFIN, c, r, 300, yaw)
t.door_in = (t.X(19.0), t.Z(45.3), 256)
t.door_up = (t.X(49.2), t.Z(1.6), 512)

# ======================================================================== the sanctum
s = Cell('sanctum', 'BFSANCTUM', (FLAT, 0, 0, 500, CAVE)); cells.append(s)
s.flat(95, 2, 103.6, 11.2, 0, 600, CAVE, mapped=True)
s.flat(84, 1.4, 96, 5.6, 0, 450, CAVE, mapped=True)
s.flat(81, 5.4, 88.6, 12.2, 0, 460, CAVE, mapped=True)
s.flat(84, 12, 98, 22.8, -300, 1100, CAVE, mapped=True)  # (10) the first chamber's pit
s.flat(77, 13.2, 89, 16.2, 0, 800, STONE)                 # the walkway along it
s.flat(81.5, 12, 84.2, 13.6, 0, 460, RUIN)                # the way down from the passage onto the walkway
s.ramp(89, 12.6, 92.2, 15.4, RW, -300, 0, 800, STONE)     # stairs down into the pit
s.flat(76, 15.4, 81.6, 20.6, 0, 600, RUIN, mapped=True)
s.flat(78, 16, 81.6, 20.2, 0, 600, RUIN)
s.flat(66.4, 16, 76.2, 19.3, 0, 400, RUIN, mapped=True) # the blades
s.flat(66.4, 16.1, 76, 19.2, 0, 400, RUIN)
s.flat(51, 16, 66.6, 19.3, 0, 560, RUIN)                  # the Hall of Stories, past the claw door
s.flat(41, 14, 51.2, 21.6, 0, 700, RUIN, mapped=True)
s.flat(41, 15, 51.2, 20.6, 0, 700, RUIN)
s.flat(24, 9.4, 41.2, 28.8, -200, 1300, CAVE, mapped=True)   # (11) the main chamber
s.ramp(40.6, 16, 44, 19.5, RE, -200, 0, 900, RUIN)
s.flat(29.4, 15, 36, 22.2, -60, 1200, STONE)              # the platform of the word wall
s.ramp(36, 16.6, 38.2, 20.4, RW, -200, -60, 1200, STONE)
s.ramp(31, 13.4, 34.4, 15, RS, -200, -60, 1200, STONE)
s.ramp(31, 22.2, 34.4, 23.8, RN, -200, -60, 1200, STONE)
s.solid(28.6, 15.6, 29.4, 21.6)                           # the word wall
s.ramp(14.4, 16, 26.4, 19.4, RW, -200, 250, 420, CAVE)    # (12) the way up and out
s.flat(8.6, 14, 14.4, 20.6, 250, 500, CAVE, mapped=True)
s.flat(0.5, 15, 9, 18.6, 250, 420, CAVE, mapped=True)
s.flat(1, 15.6, 9, 18, 250, 420, CAVE)
# --- the first chamber: draugr below, a Restless Draugr on the walkway, oil under it, a chest, a potion
s.ent('E_WIGHT', 90.5, 18.5, -300, 768, 0, 0, 0)
s.ent('E_WIGHT', 94.5, 20.2, -300, 768, 1, 0, 0)
s.ent('E_WIGHT', 79.5, 14.6, 0, 256, 2, 1, 0)
s.fixture(F_OIL, 87, 18.5, -300, 0, 1, 190)
s.fixture(F_LANTERN, 87, 16.5, -110, 0, 1)
s.prop(PT_CHEST, 96.2, 21, -300, 768, 6)
s.place(D_TABLE, 79.2, 19.5, 0, 0); s.prop(PT_ITEM, 79.2, 19.5, 80, 300, 'IT_POT_RESFROST')
for c, r, y in ((86, 12.6, 0), (78, 13.6, 0), (96, 13, -300), (96, 22, -300), (88, 21.8, -300)):
    s.place(D_BRAZIER, c, r, y); s.light(c, r, y + 140, 900, 150)
s.light(91, 17, 300, 1400, 120)
s.place(D_URNS, 82, 16, 0); s.place(D_BONES, 92, 21, -300, 100)
s.light(99, 6.5, 200, 900, 110); s.light(86.5, 4, 200, 700, 90)
s.place(D_WALLCOFFIN, 95.6, 6, 0, 256); s.place(D_RUBBLE, 101, 9.6, 0, 300)
# --- the blades and the claw door
s.fixture(F_PLATE, 75.4, 17.65, 0, 0, 1)                  # plate 1 -> blade group 1 starts
for c in (74.2, 71.8, 69.4):
    s.fixture(F_BLADE, c, 17.65, 0, 256, 1)
s.prop(PT_PULLCHAIN, 67.9, 16.2, 0, 0, 2)                # chain 2: stops blade group 1
s.prop(PT_CLAWDOOR, 66.9, 17.65, 0, 256, 0)
s.place(D_TORCH, 76.4, 16.4, 0, 0); s.light(76.5, 17, 200, 700, 130)
s.place(D_TORCH, 66.2, 19.2, 0, 512); s.light(66, 18.5, 200, 600, 110)
s.place(D_TORCH, 58, 16.05, 0, 0); s.light(58, 16.8, 200, 700, 120)
for c in (52.5, 55.5, 58.5, 61.5, 64.5):
    s.place(D_ARCH, c, 17.65, 0, 256, 420)                 # the vault's ribs
for c in (54, 57, 60, 63):
    s.place(D_MURAL, c, 16.1, 0, 512); s.place(D_MURAL, c, 19.2, 0, 0)   # the stories, both walls
# --- the main chamber
s.prop(PT_WORDWALL, 29.4, 18.6, -60, 256, 0)
s.fixture(F_COFFINLID, 31.6, 18.6, -60, 256, 0)          # the Overlord's sarcophagus
s.ent('E_WIGHT', 31.6, 18.6, -60, 256, 5, 2, 64)          # Draugr Overlord, asleep in it
s.prop(PT_CHEST, 33.2, 21.4, -60, 0, 7)
for c, r in ((34.4, 16.2), (34.4, 21)):
    s.place(D_BRAZIER, c, r, -60); s.light(c, r, 80, 1200, 210)
s.place(D_WATERFALL, 25.2, 12.6, -200, 300); s.place(D_WATERFALL, 25.2, 25.4, -200, 724)
s.light(31, 12, 600, 1600, 150); s.light(31, 26, 600, 1600, 150); s.light(38, 18.5, 300, 1000, 120)
for c, r in ((42, 14.6), (42, 21), (50.2, 14.6), (50.2, 21)):
    s.place(D_PILLAR, c, r, 0, 0, 300)
s.place(D_BRAZIER, 46, 15, 0); s.light(46, 15, 140, 900, 150); s.place(D_BRAZIER, 46, 20.6, 0); s.light(46, 20.6, 140, 900, 150)
s.place(D_BONES, 27, 20, -200, 300); s.place(D_RUBBLE, 38, 26, -200, 100)
s.light(16, 17.6, 250, 700, 100); s.light(5, 16.8, 450, 700, 110)
s.place(D_TORCH, 11.5, 14.1, 250, 0); s.light(11.5, 14.8, 450, 600, 110)
s.door_in = (s.X(102.4), s.Z(6.6), 768)
s.door_out = (s.X(1.4), s.Z(16.8), 256)

# ======================================================================== output
def table(f, name, rows, n):
    flat = [v for r in rows for v in r]
    if not flat: flat = [0] * n
    f.write('static const int %s[%d] = {\n' % (name, len(flat)))
    for r in rows: f.write('    ' + ', '.join(str(v) for v in r) + ',\n')
    f.write('    };\n')
    f.write('enum { %s_N = %d };\n' % (name, len(rows)))

def esym(v):
    return v if isinstance(v, str) else str(v)

out = os.path.join(os.path.dirname(__file__), '..', '..', 'src', 'bfb.inc')
with open(out, 'w') as f:
    f.write('// generated by docs/maps/gen_bfb.py from the maps in docs/maps/ref/bfb - do not edit\n')
    f.write('// grid samples are %d cm; cm in each cell\'s frame (its grid\'s centre at 0, 0), x east, z north\n' % S)
    for c in cells:
        n = c.cname
        # crop to the used part of the grid
        js = [j for j in range(c.H) if any(c.zid[j])]; is_ = [i for i in range(c.W) if any(c.zid[j][i] for j in range(c.H))]
        j0, j1, i0, i1 = min(js), max(js) + 1, min(is_), max(is_) + 1
        j0 = max(0, j0 - 1); i0 = max(0, i0 - 1); j1 = min(c.H, j1 + 1); i1 = min(c.W, i1 + 1)
        W, H = i1 - i0, j1 - j0
        # grid row 0 = the southernmost row (z grows north)
        f.write('// ---- %s\n' % n)
        f.write('static const u8 K%sGrid[%d] = {\n' % (n, W * H))
        for j in range(j1 - 1, j0 - 1, -1):
            f.write('    ' + ','.join(str(c.zid[j][i]) for i in range(i0, i1)) + ',\n')
        f.write('    };\n')
        # the grid's south-west corner (cm)
        gx0 = (i0 - c.ci) * S; gz0 = (c.cj - j1) * S
        f.write('enum { K%sW = %d, K%sH = %d, K%sX0 = %d, K%sZ0 = %d };\n' % (n, W, n, H, n, gx0, n, gz0))
        table(f, 'K%sZone' % n, [list(z) for z in c.zones], 9)
        table(f, 'K%sPut' % n, [list(p) for p in c.put], 6)
        f.write('static const DunProp K%sProp[] = {\n' % n)
        for p in c.props: f.write('    { %s, %d, %d, %d, %d, %s },\n' % (p[0], p[1], p[2], p[3], p[4], esym(p[5])))
        f.write('    { -1, 0, 0, 0, 0, 0 } };\n')
        f.write('static const int K%sEnt[] = {\n' % n)
        for p in c.ents: f.write('    %s, %d, %d, %d, %d, %d, %d, %d,\n' % p)
        f.write('    -1 };\n')
        table(f, 'K%sLight' % n, [list(p) for p in c.lights], 5)
        table(f, 'K%sFix' % n, [list(p) for p in c.fix], 7)
    f.write('// doors (cm, yaw): the way in from the terrace, the spiral stairs (both ends), up to the sanctum (both ends), out\n')
    f.write('enum { BFD_IN_X = %d, BFD_IN_Z = %d, BFD_IN_YAW = %d,\n' % e.door_in)
    f.write('       BFD_DOWN_X = %d, BFD_DOWN_Z = %d, BFD_DOWN_YAW = %d,\n' % e.door_down)
    f.write('       BFD_CRYPT_X = %d, BFD_CRYPT_Z = %d, BFD_CRYPT_YAW = %d,\n' % t.door_in)
    f.write('       BFD_UP_X = %d, BFD_UP_Z = %d, BFD_UP_YAW = %d,\n' % t.door_up)
    f.write('       BFD_SANC_X = %d, BFD_SANC_Z = %d, BFD_SANC_YAW = %d,\n' % s.door_in)
    f.write('       BFD_OUT_X = %d, BFD_OUT_Z = %d, BFD_OUT_YAW = %d };\n' % s.door_out)
    f.write('// the way Arvel runs when he is cut free (cm)\n')
    f.write('static const int KArvelPath[%d] = { %s };\n' % (len(t.arvel_path) * 2, ', '.join('%d, %d' % p for p in t.arvel_path)))
    f.write('enum { KArvelPath_N = %d };\n' % len(t.arvel_path))
print('wrote', out)
for c in cells:
    print(c.name, 'grid', c.W, 'x', c.H, 'zones', len(c.zones), 'decor', len(c.put), 'props', len(c.props), 'ents', len(c.ents), 'lights', len(c.lights), 'fix', len(c.fix))

# ---- a picture of each cell's levels, for checking (python3 gen_bfb.py <dir>)
if len(sys.argv) > 1:
    from PIL import Image, ImageDraw
    for c in cells:
        K = 6
        im = Image.new('RGB', (c.W * K, c.H * K), (40, 34, 30))
        d = ImageDraw.Draw(im)
        for j in range(c.H):
            for i in range(c.W):
                z = c.zid[j][i]
                if not z: continue
                kind, y0, y1, ceil, style = c.zones[z - 1][:5]
                y = (y0 + y1) // 2
                v = max(40, min(250, 140 + y // 4))
                col = (v, v, v)
                if style == CAVE: col = (v, int(v * 0.9), int(v * 0.75))
                if style == WATER: col = (60, 110, v)
                if style == STONE: col = (int(v * 0.8), int(v * 0.85), v)
                if kind != FLAT: col = (v, int(v * 0.6), int(v * 0.4))
                d.rectangle([i * K, j * K, i * K + K - 1, j * K + K - 1], fill=col)
        def P(x, z): return ((x / S + c.ci) * K, (c.cj - z / S) * K)
        for p in c.put:
            x, y = P(p[1], p[3]); d.ellipse([x - 3, y - 3, x + 3, y + 3], outline=(0, 160, 0))
        for p in c.props:
            x, y = P(p[1], p[3]); d.rectangle([x - 3, y - 3, x + 3, y + 3], outline=(0, 120, 255))
        for p in c.ents:
            x, y = P(p[1], p[3]); d.ellipse([x - 4, y - 4, x + 4, y + 4], fill=(220, 30, 30))
        for p in c.lights:
            x, y = P(p[0], p[2]); d.point((x, y), fill=(255, 255, 0))
        for p in c.fix:
            x, y = P(p[1], p[3]); d.rectangle([x - 4, y - 2, x + 4, y + 2], fill=(255, 0, 255))
        im.save(os.path.join(sys.argv[1], 'bfb_%s.png' % c.name))
