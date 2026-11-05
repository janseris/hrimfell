# Hall of the Dead (Vindgard) - plan draft for review: outside, the hall with the priest's room, the catacombs.
# Metres; x east, y north. Run: python3 docs/maps/draw_halldead.py -> docs/maps/halldead_plan_draft.png
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, Circle, Polygon, FancyArrow

STONE, FLOOR, WOOD, GREEN, IRON, WATER = "#6b6258", "#e9e1cf", "#9a6b3c", "#7c9a5a", "#4a4f57", "#9fb7c9"
fig, axs = plt.subplots(1, 3, figsize=(24, 13.5), gridspec_kw={"width_ratios": [1.0, 1.25, 1.45]})

def setup(ax, title, xr, yr):
    ax.set_title(title, fontsize=14, weight="bold"); ax.set_aspect("equal")
    ax.set_xlim(*xr); ax.set_ylim(*yr); ax.grid(True, color="#ccc", lw=0.5)
    ax.set_xticks(range(xr[0], xr[1] + 1, 2)); ax.set_yticks(range(yr[0], yr[1] + 1, 2))

def rect(ax, x0, y0, x1, y1, fc, ec="k", lw=1, z=1, a=1.0, hatch=None):
    ax.add_patch(Rectangle((x0, y0), x1 - x0, y1 - y0, fc=fc, ec=ec, lw=lw, zorder=z, alpha=a, hatch=hatch))

def wall(ax, x0, y0, x1, y1): rect(ax, x0, y0, x1, y1, STONE, STONE, 0, 3)
def label(ax, x, y, n, fs=10): ax.text(x, y, str(n), ha="center", va="center", fontsize=fs, weight="bold", zorder=9,
                                       bbox=dict(boxstyle="circle,pad=0.15", fc="#fff8c0", ec="k", lw=0.6))
def note(ax, x, y, s, fs=8, c="k", rot=0): ax.text(x, y, s, ha="center", va="center", fontsize=fs, color=c, zorder=8, rotation=rot)
def door(ax, x, y, horiz, c=WOOD, w=1.4):
    if horiz: rect(ax, x - w / 2, y - 0.25, x + w / 2, y + 0.25, c, "k", 0.8, 5)
    else: rect(ax, x - 0.25, y - w / 2, x + 0.25, y + w / 2, c, "k", 0.8, 5)
def pot(ax, x, y): ax.add_patch(Circle((x, y), 0.32, fc=GREEN, ec="k", lw=0.6, zorder=5))
def urns(ax, x0, y0, x1, y1, n):
    for i in range(n):
        t = (i + 0.5) / n; ax.add_patch(Circle((x0 + (x1 - x0) * t, y0 + (y1 - y0) * t), 0.22, fc="#b89a6a", ec="k", lw=0.5, zorder=6))
def bench(ax, x, y, horiz=True, l=2.2): rect(ax, x - (l / 2 if horiz else 0.25), y - (0.25 if horiz else l / 2),
                                            x + (l / 2 if horiz else 0.25), y + (0.25 if horiz else l / 2), WOOD, "k", 0.6, 5)
def skel(ax, x, y): ax.plot([x], [y], marker="X", ms=13, color="#c03030", zorder=9)
def stairs(ax, x0, y0, x1, y1, n, down_dir):
    rect(ax, x0, y0, x1, y1, "#d4c9b0", "k", 0.8, 2)
    for i in range(1, n):
        if down_dir in "NS": y = y0 + (y1 - y0) * i / n; ax.plot([x0, x1], [y, y], color="k", lw=0.5, zorder=3)
        else: x = x0 + (x1 - x0) * i / n; ax.plot([x, x], [y0, y1], color="k", lw=0.5, zorder=3)
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    d = {"N": (0, 1), "S": (0, -1), "E": (1, 0), "W": (-1, 0)}[down_dir]
    ax.add_patch(FancyArrow(cx - d[0] * 1.2, cy - d[1] * 1.2, d[0] * 2.2, d[1] * 2.2, width=0.12, head_width=0.5, color="#333", zorder=4))
    note(ax, cx + (0.9 if down_dir in "NS" else 0), cy + (0 if down_dir in "NS" else 0.9), "down", 7)

# ------------------------------------------------------------------ 1. outside, in the Wind District
ax = axs[0]; setup(ax, "1. Outside (Wind District, building 25)", (-12, 12), (-14, 12))
rect(ax, -12, -14, 12, -6.5, "#d9cfb8", None, 0, 0); note(ax, 0, -10, "street / Wind District ring road\n(towards the Gildergreen: south-east)", 9)
rect(ax, -6.5, -5, 6.5, 7.5, "#a49a8c", "k", 1.2, 2)                              # stone hall
ax.add_patch(Polygon([(-6.5, -5), (0, -1.2), (6.5, -5)], closed=False, fill=False, ec="#5a3a1a", lw=1.5, zorder=4))
ax.plot([0, 0], [-1.2, 3.8], color="#5a3a1a", lw=2, zorder=4)
ax.add_patch(Polygon([(-6.5, 7.5), (0, 3.8), (6.5, 7.5)], closed=False, fill=False, ec="#5a3a1a", lw=1.5, zorder=4))
note(ax, 0, 1.6, "stone hall, 13 x 12.5 m\nsteep shingled roof,\nridge north-south,\ncarved crossed gable ends", 9)
rect(ax, -2.2, -7.2, 2.2, -5, "#c9bfa9", "k", 0.8, 3); note(ax, 0, -6.1, "porch: 3 steps up,\ntwo pillars", 7)
door(ax, 0, -5, True, IRON, 1.8); label(ax, 0, -4.3, "A")
ax.plot([-1.6], [-5.4], marker="$!$", ms=10, color="#a00", zorder=9); label(ax, -3, -5.7, "B")
for bx in (-2.9, 2.9): ax.add_patch(Circle((bx, -6.4), 0.4, fc="#e08a2a", ec="k", zorder=5))
label(ax, 3.8, -6.6, "C")
rect(ax, 6.6, -1, 8, 5, "#cfc5b0", "k", 0.5, 1); note(ax, 7.3, 2, "temple side", 7, rot=90)
rect(ax, -8, -1, -6.6, 5, "#cfc5b0", "k", 0.5, 1); note(ax, -7.3, 2, "Hardrede side", 7, rot=90)
note(ax, 0, 9.8, "behind: the tier wall / nothing to enter", 8, "#555")
ax.text(0.0, -0.07, "A  iron-bound double door (opens -> the hall)\nB  scratched 'danger' mark left of the door\nC  two braziers on the porch steps",
        fontsize=10, va="top", transform=ax.transAxes, zorder=10, bbox=dict(fc="white", ec="#888"))

# ------------------------------------------------------------------ 2. the hall + the priest's room
ax = axs[1]; setup(ax, "2. The Hall (and the priest's room)", (-12, 7), (-7, 9))
rect(ax, -6, -5, 6, 7, FLOOR, None, 0, 0); rect(ax, -11, 1.5, -6, 7, "#efe5cc", None, 0, 0)
wall(ax, -6.4, -5.4, 6.4, -5); wall(ax, -6.4, 7, 6.4, 7.4); wall(ax, 6, -5.4, 6.4, 7.4); wall(ax, -6.4, -5.4, -6, 1.5)
wall(ax, -6.4, 1.5, -6, 3.6); wall(ax, -6.4, 5.4, -6, 7.4)
wall(ax, -11.4, 1.1, -6, 1.5); wall(ax, -11.4, 7, -6, 7.4); wall(ax, -11.4, 1.1, -11, 7.4)
for px in (-2, 2):
    for py in (-2.5, 0.8, 4.1): rect(ax, px - 0.35, py - 0.35, px + 0.35, py + 0.35, STONE, "k", 0.6, 4)
note(ax, -2, 6.0, "pillars", 7, "#444")
door(ax, 0, -5.2, True, IRON, 1.8); label(ax, 0, -4.2, 1); note(ax, 0, -6.2, "from the street", 8)
door(ax, 0, 7.2, True, IRON, 1.4); label(ax, 0, 6.2, 2)
for (x, y) in ((-1.3, 4.8), (1.3, 4.8), (-1.3, 2.5), (1.3, 2.5), (-1.3, -3.6), (1.3, -3.6)): pot(ax, x, y)
label(ax, 0, 3.6, 3); bench(ax, 0, 0.9); bench(ax, 0, -1.2); label(ax, 0, -0.15, 4)
rect(ax, 3.2, 5.6, 4.8, 6.9, "#d9c9a0", "k", 1, 5); ax.add_patch(Circle((4, 6.25), 0.35, fc="#f2d36b", ec="k", zorder=6)); label(ax, 4, 4.9, 5)
bench(ax, 4, 2.6); bench(ax, 4, 0.6); label(ax, 4, 1.6, 6)
bench(ax, -4, 0.2); bench(ax, -4, -2.0); label(ax, -4, -0.9, 7)
for (x, y) in ((-5.4, -4.3), (-5.4, -3.3), (-5.4, 6.3)): pot(ax, x, y); label(ax, -4.6, -3.8, 8)
door(ax, -6.2, 4.5, False, WOOD, 1.6); label(ax, -5.2, 4.5, 9)
urns(ax, 6.0 - 0.3, -4.5, 6.0 - 0.3, 4.5, 7); urns(ax, -6.0 + 0.3, -2.2, -6.0 + 0.3, 1.0, 3)
urns(ax, -5.5, 7 - 0.3, -2.5, 7 - 0.3, 4); urns(ax, 2.5, -5 + 0.3, 5.5, -5 + 0.3, 4); label(ax, 5.2, -2.2, 10)
for (x, y) in ((-5.5, 1.2), (5.5, -4.5), (-2.8, 6.6), (2.8, 6.6)): ax.add_patch(Circle((x, y), 0.25, fc="#ffb347", ec="k", zorder=6))
# the priest's room
rect(ax, -10.9, 4.8, -9.1, 6.9, "#b38a5a", "k", 0.8, 5); rect(ax, -10.9, 6.3, -9.1, 6.9, "#ddd", "k", 0.5, 6); label(ax, -10, 5.4, 11)
rect(ax, -8.8, 6.4, -7.8, 6.95, WOOD, "k", 0.8, 5); rect(ax, -7.6, 6.4, -6.6, 6.95, WOOD, "k", 0.8, 5); label(ax, -8.2, 5.9, 12)
rect(ax, -10.6, 3.9, -9.4, 4.5, "#7a5230", "k", 0.8, 5); label(ax, -10, 3.4, 13)
rect(ax, -9.0, 1.6, -7.4, 2.4, WOOD, "k", 0.8, 5); ax.add_patch(Circle((-8.2, 2.9), 0.3, fc=WOOD, ec="k", zorder=5)); label(ax, -8.2, 2.0, 14)
rect(ax, -10.95, 1.6, -9.6, 2.1, "#6e4a2a", "k", 0.8, 5); label(ax, -10.3, 2.6, 15)
ax.text(0.0, -0.07,
        "1  entrance (iron-bound door)\n2  iron door down to the catacombs\n3  six lavender planters (middle aisle)\n4  two benches\n"
        "5  shrine of the Keeper (Arkay) - blessing\n6  chapel: two benches facing it\n7  west aisle: two benches\n8  three lavender planters\n"
        "9  door to the priest's room\n10 burial niches with urns in the walls\n    (candles on stands: orange dots)\n"
        "11 priest's bed   12 two wardrobes\n13 chest (gold)\n14 table + chair: purse, potion, 3 books,\n    shelf above (weak poison)\n15 bookshelf left of the table",
        fontsize=10, va="top", transform=ax.transAxes, zorder=10, bbox=dict(fc="white", ec="#888"))
note(ax, 0, -6.7, "hall 12 x 12 m, three aisles, vaulted, ~6 m high at the ridge", 8, "#333")

# ------------------------------------------------------------------ 3. catacombs (two levels)
ax = axs[2]; setup(ax, "3. Catacombs (upper level 0 m, lower level -3 m)", (-12, 16), (-4, 17))
UP, LOW = "#e6dcc6", "#cfc2a6"
rect(ax, -4, -1, 8, 3, UP, None, 0, 0); rect(ax, -10, -2.5, -5, 4, UP, None, 0, 0)          # upper corridor + side room
rect(ax, -8, 9, 9, 16, LOW, None, 0, 0); rect(ax, 9.4, 9, 15, 15, LOW, None, 0, 0)          # lower crypt + amulet room
rect(ax, -8, 4, -5.4, 9, LOW, None, 0, 0)                                                   # gem alcove passage
wall(ax, -4.4, -1.4, 8.4, -1); wall(ax, -1.2, 3, 5.6, 3.4); wall(ax, 8, -1.4, 8.4, 9); wall(ax, -10.4, -2.9, -4.6, -2.5)
wall(ax, -10.4, -2.9, -10, 4.4); wall(ax, -10.4, 4, -4.6, 4.4); wall(ax, -5, -2.9, -4.6, 0.4); wall(ax, -5, 2.0, -4.6, 4.4)
wall(ax, -8.4, 8.6, -5.4, 9.0); wall(ax, -8.4, 4.4, -8, 16.4); wall(ax, -8.4, 16, 9.4, 16.4); wall(ax, 9.0, 8.6, 9.4, 11.2)
wall(ax, 9.0, 12.8, 9.4, 16.4); wall(ax, 9.0, 8.6, 15.4, 9.0); wall(ax, 15, 8.6, 15.4, 15.4); wall(ax, 9.0, 15, 15.4, 15.4)
wall(ax, -1.2, 8.6, 5.6, 9.0)
note(ax, 2.2, 6, "solid rock", 9, "#555")
door(ax, 0, -1.2, True, IRON, 1.4); label(ax, 0, -2.2, 1); note(ax, 1.5, -3, "from the hall", 8)
door(ax, -4.8, 1.2, False, WOOD, 1.6); label(ax, -3.6, 1.8, 2)
rect(ax, -9.7, 2.6, -8.1, 3.6, WOOD, "k", 0.6, 5); rect(ax, -9.7, -2.1, -8.1, -1.1, WOOD, "k", 0.6, 5); label(ax, -7, 3.1, 3)
skel(ax, -7.2, 0.6); urns(ax, -9.7, -0.6, -9.7, 2.0, 3)
skel(ax, 6.6, 1.0); label(ax, 6.6, 2.2, 4); urns(ax, 2, -0.7, 7.6, -0.7, 6)
stairs(ax, 5.6, 3, 8, 9, 10, "N"); label(ax, 4.9, 6, 5)
stairs(ax, -4, 3, -1.2, 9, 10, "N"); label(ax, -0.6, 6, 6)
skel(ax, 6.8, 10.2); label(ax, 5.7, 10.3, 7)
door(ax, 9.2, 12, False, WOOD, 1.6); label(ax, 8.2, 12, 8)
ax.add_patch(Circle((15 - 0.8, 12), 0.55, fc="#e08a2a", ec="k", zorder=6)); rect(ax, 13.6, 13.0, 14.8, 13.8, WOOD, "k", 0.6, 5)
for (x, y) in ((12, 14.6), (11, 14.6), (12.5, 9.4)): ax.add_patch(Circle((x, y), 0.32, fc="none", ec=GREEN, lw=2.5, zorder=6))
label(ax, 12.2, 12, 9)
rect(ax, -7.9, 5.6, -6.6, 7.2, "#b5a68a", "k", 0.6, 4); ax.plot([-7.25], [6.4], marker="D", ms=8, color="#3aa0d0", zorder=7); label(ax, -6.0, 6.4, 10)
for i in range(6):                                                                          # coffins
    x = -6.8 + i * 2.3; rect(ax, x, 14.4, x + 1.0, 15.9, "#8a6a4a", "k", 0.6, 5); rect(ax, x, 9.1, x + 1.0, 10.6, "#8a6a4a", "k", 0.6, 5) if i in (1, 2) else None
label(ax, 1.5, 13.6, 11); urns(ax, -7.7, 10.5, -7.7, 15.5, 5); urns(ax, 0, 9.3, 4.8, 9.3, 4)
for (x, y) in ((-2.5, 12.5), (3.5, 12.5), (-6, -0.5), (3, 1.5)): ax.add_patch(Circle((x, y), 0.25, fc="#ffb347", ec="k", zorder=6))
ax.text(0.0, -0.07,
        "1  iron door from the hall (upper level)\n2  closed side room left of the entrance: two tables,\n"
        "   Block skill book + 2 coin purses, skeleton 3\n3  tables   4  turn right: skeleton 1, urn niches\n"
        "5  stairs down (east), skeleton 2 at the foot (7)\n6  second stairs down (west)\n"
        "8  door to the amulet room\n9  amulet room: brazier on the east wall,\n   small table right of it (the priest's amulet),\n   funeral wreaths\n"
        "10 alcove at the foot of the west stairs: rare gem\n11 lower crypt: coffins (named ones for dead\n   townsfolk later), urn niches\n"
        "X  skeletons (come back after some days)",
        fontsize=10, va="top", transform=ax.transAxes, zorder=10, bbox=dict(fc="white", ec="#888"))
fig.suptitle("Vindgard - Hall of the Dead: plan draft for correction (metres, north up)", fontsize=16, weight="bold")
plt.subplots_adjust(left=0.03, right=0.99, top=0.93, bottom=0.30, wspace=0.12); plt.savefig("docs/maps/halldead_plan_draft.png", dpi=90)
