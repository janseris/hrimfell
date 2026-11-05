# Highhall (after Dragonsreach) - plan draft for review: the main hall level and the upper level (war room, galleries).
# Metres, x east, y north (the front doors face south, towards the city). Heights above the entrance floor.
# Solid outlines: known from the game / UESP. Dashed red: placed by guess (proportions, exact positions).
# Run: python3 docs/maps/draw_highhall.py -> docs/maps/highhall_plan_draft.png
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, Circle, FancyArrow

STONE, FLOOR, RAISED, WOOD, FIRE = "#6b6258", "#e6d6b8", "#d8c49c", "#9a6b3c", "#e08a2a"
fig, axs = plt.subplots(1, 2, figsize=(26, 17))

def setup(ax, title):
    ax.set_title(title, fontsize=15, weight="bold"); ax.set_aspect("equal")
    ax.set_xlim(-24, 24); ax.set_ylim(-3, 56); ax.grid(True, color="#ccc", lw=0.5)
    ax.set_xticks(range(-24, 25, 2)); ax.set_yticks(range(-2, 57, 2)); ax.tick_params(labelsize=7)
def rect(ax, x0, y0, x1, y1, fc, ec="k", lw=1.0, z=1, ls="-", a=1.0):
    ax.add_patch(Rectangle((x0, y0), x1 - x0, y1 - y0, fc=fc, ec=ec, lw=lw, zorder=z, ls=ls, alpha=a))
def guess(ax, x0, y0, x1, y1): rect(ax, x0, y0, x1, y1, "none", "#c03030", 1.6, 6, "--")
def label(ax, x, y, n): ax.text(x, y, str(n), ha="center", va="center", fontsize=10, weight="bold", zorder=9,
                                bbox=dict(boxstyle="circle,pad=0.15", fc="#fff8c0", ec="k", lw=0.6))
def note(ax, x, y, s, fs=8, c="#333", rot=0): ax.text(x, y, s, ha="center", va="center", fontsize=fs, color=c, zorder=8, rotation=rot)
def person(ax, x, y, name): ax.plot([x], [y], marker="o", ms=10, color="#2a6fd0", zorder=9); note(ax, x, y - 0.8, name, 7, "#1a4fa0")
def stairs(ax, x0, y0, x1, y1, n, up, txt="up"):
    rect(ax, x0, y0, x1, y1, "#cdb994", "k", 0.8, 3)
    for i in range(1, n):
        if up in "NS": y = y0 + (y1 - y0) * i / n; ax.plot([x0, x1], [y, y], color="k", lw=0.4, zorder=4)
        else: x = x0 + (x1 - x0) * i / n; ax.plot([x, x], [y0, y1], color="k", lw=0.4, zorder=4)
    d = {"N": (0, 1), "S": (0, -1), "E": (1, 0), "W": (-1, 0)}[up]; cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    L = min(abs(x1 - x0), abs(y1 - y0)) if False else (abs(y1 - y0) if up in "NS" else abs(x1 - x0)) * 0.6
    ax.add_patch(FancyArrow(cx - d[0] * L / 2, cy - d[1] * L / 2, d[0] * L, d[1] * L, width=0.12, head_width=0.5, color="#333", zorder=5))
    note(ax, cx + (0.9 if up in "NS" else 0), cy + (0 if up in "NS" else 0.7), txt, 7)
def door(ax, x, y, horiz, w=2.0, c=WOOD): rect(ax, x - (w / 2 if horiz else 0.25), y - (0.25 if horiz else w / 2), x + (w / 2 if horiz else 0.25), y + (0.25 if horiz else w / 2), c, "k", 0.8, 6)
def wall(ax, x0, y0, x1, y1): rect(ax, x0, y0, x1, y1, STONE, STONE, 0, 4)
def legend(ax, text): ax.text(0.0, -0.035, text, transform=ax.transAxes, fontsize=10, va="top", zorder=10, bbox=dict(fc="white", ec="#888"))

# =========================================================== main hall level
ax = axs[0]; setup(ax, "1. Highhall - the main hall level")
rect(ax, -8, 0, 8, 8, FLOOR, None, 0, 0)                                   # entrance room (0 m)
rect(ax, -10, 11, 10, 42, RAISED, None, 0, 0)                              # great hall (+1.5 m)
rect(ax, -7, 36, 7, 42, "#c8b088", None, 0, 0)                             # the dais (+2.2 m)
rect(ax, -22, 16, -10, 34, FLOOR, None, 0, 0)                              # kitchen
rect(ax, 10, 16, 22, 34, FLOOR, None, 0, 0)                                # court wizard
for (a, b, c, d) in ((-8.4, -0.4, 8.4, 0), (-8.4, -0.4, -8, 8), (8, -0.4, 8.4, 8), (-10.4, 8, -4, 8.4), (4, 8, 10.4, 8.4),
                     (-10.4, 8, -10, 16), (10, 8, 10.4, 16), (-10.4, 34, -10, 42.4), (10, 34, 10.4, 42.4), (-10.4, 42, -3, 42.4), (3, 42, 10.4, 42.4),
                     (-22.4, 15.6, -10, 16), (-22.4, 34, -10, 34.4), (-22.4, 15.6, -22, 34.4), (10, 15.6, 22.4, 16), (10, 34, 22.4, 34.4), (22, 15.6, 22.4, 34.4),
                     (-10.4, 16, -10, 23.5), (-10.4, 26.5, -10, 34), (10, 16, 10.4, 23.5), (10, 26.5, 10.4, 34)):
    wall(ax, a, b, c, d)
door(ax, 0, -0.2, True, 3.0); label(ax, 0, 1.3, 1); note(ax, 0, -1.6, "the great doors (from the top of the stairs out in the city)", 8)
for (x, y) in ((-5, 2.5), (5, 2.5), (-5, 5.5), (5, 5.5)): rect(ax, x - 1.2, y - 0.25, x + 1.2, y + 0.25, WOOD, "k", 0.6, 5)
label(ax, -6.8, 4, 2)
stairs(ax, -4, 8, 4, 11, 6, "N"); label(ax, 5, 9.5, 3)
for y in (14, 20, 26, 32, 38):
    for x in (-7, 7): rect(ax, x - 0.5, y - 0.5, x + 0.5, y + 0.5, "#6e4a2a", "k", 0.6, 5)
note(ax, -8.5, 14, "pillars", 7)
rect(ax, -1.0, 18, 1.0, 32, "#7a6a5a", "k", 1, 5); rect(ax, -0.6, 18.5, 0.6, 31.5, FIRE, "k", 0.5, 6); label(ax, 0, 25, 4)
for x in (-3.5, 3.5):
    rect(ax, x - 0.6, 19, x + 0.6, 31, WOOD, "k", 0.8, 5)
    for i in range(5): ax.add_patch(Circle((x + (-1.1 if x < 0 else 1.1), 20 + i * 2.6), 0.3, fc="#b88a58", ec="k", lw=0.5, zorder=5))
label(ax, -3.5, 33, 5); label(ax, 3.5, 33, 5)
stairs(ax, -5, 34.5, 5, 36, 3, "N", "")
rect(ax, -1.2, 38.6, 1.2, 40.6, "#5a3a1a", "k", 1.2, 6); person(ax, 0, 39.6, "the Jarl"); label(ax, 0, 37.2, 6)
person(ax, -2.5, 38.5, "steward"); person(ax, 2.6, 38.5, "housecarl"); person(ax, -5.5, 37.2, "guard"); person(ax, 5.5, 37.2, "guard")
rect(ax, -5.5, 40.2, -3.5, 41.4, WOOD, "k", 0.6, 5); rect(ax, 3.5, 40.2, 5.5, 41.4, WOOD, "k", 0.6, 5)
rect(ax, 5.8, 39.0, 6.6, 41.6, "#6e4a2a", "k", 0.6, 5); rect(ax, 6.0, 36.6, 6.8, 38.8, "#6e4a2a", "k", 0.6, 5); label(ax, 7.6, 40.4, 7)
stairs(ax, -9.8, 36, -7.6, 42, 6, "N"); stairs(ax, 7.6, 36, 9.8, 42, 6, "N"); label(ax, -8.7, 34.6, 8); label(ax, 8.7, 34.6, 8)
# kitchen
door(ax, -10.2, 25, False, 3.0)
rect(ax, -19, 33.2, -13, 34, "#7a6a5a", "k", 1, 5); rect(ax, -17.5, 33.3, -14.5, 33.9, FIRE, "k", 0.5, 6); label(ax, -16, 32.2, 9)
rect(ax, -16.5, 26, -14.5, 30, WOOD, "k", 0.8, 5); rect(ax, -20, 22, -18, 27, WOOD, "k", 0.8, 5); label(ax, -15.5, 24.8, 10)
for x in (-20.5, -18.5, -15): rect(ax, x - 0.7, 16.1, x + 0.7, 16.8, "#6e4a2a", "k", 0.6, 5)
label(ax, -17, 17.6, 11)
stairs(ax, -21.8, 30, -20, 33.8, 5, "S", "down"); label(ax, -19.2, 31.5, 12)
door(ax, -22.2, 20, False, 1.6); label(ax, -21, 20, 13)
stairs(ax, -13.5, 16.2, -11, 19, 4, "S", "down"); label(ax, -12.2, 20.3, 14)
guess(ax, -22.4, 15.6, -10, 34.4)
# court wizard
door(ax, 10.2, 25, False, 3.0)
rect(ax, 11, 20.5, 14, 21.5, WOOD, "k", 0.8, 5); label(ax, 12.5, 22.3, 15)
rect(ax, 18.5, 25, 20.5, 28, "#6a7a5a", "k", 1, 5); label(ax, 19.5, 24, 16)
rect(ax, 15.5, 29.5, 17.5, 31, "#5a5a8a", "k", 1, 5); label(ax, 16.5, 28.7, 17)
rect(ax, 21.5, 19, 21.95, 24, "#c8b070", "k", 0.6, 5); label(ax, 20.7, 21.5, 18)
person(ax, 17, 24.8, "court wizard")
rect(ax, 10.4, 30, 16, 33.9, "#eadcc0", "k", 0.6, 1); label(ax, 13, 32, 19)
rect(ax, 16.5, 16.1, 21.9, 19.5, "#eadcc0", "k", 0.6, 1); label(ax, 19.2, 17.8, 20)
guess(ax, 10, 15.6, 22.4, 34.4)
guess(ax, -10.4, 11, 10.4, 42.4)
note(ax, 0, 46, "behind the throne, one storey up: the war room\n(see the upper level)", 9)
legend(ax, "1 the great doors   2 entrance room: four benches (servants sweep here)   3 steps up into the hall (+1.5 m)\n"
           "4 the long fire pit   5 two long dining tables, five chairs each   6 the throne on its dais (+2.2 m), the Jarl's court\n"
           "7 a table each side of the throne; bookshelves on the right   8 stairs up both sides behind the throne\n"
           "KITCHEN (west)  9 big fireplace with the cooking spit (north wall)   10 food tables (meat; garlic and greens)\n"
           "11 wardrobes and a cupboard (south wall), a rack with salmon   12 down to the storeroom and a servants' bedroom\n"
           "13 door to the servants' part of the living quarters   14 down some steps: the door to the cells (dungeon)\n"
           "WIZARD (east)  15 dining table, two benches, wardrobe   16 alchemy lab   17 arcane enchanter   18 big map of the land\n"
           "19 his study (desk, bookshelves)   20 his bedroom (bed, end table, dresser)\n"
           "red dashed: placed by guess - sizes and exact positions not known")

# =========================================================== upper level
ax = axs[1]; setup(ax, "2. Highhall - the upper level (about +6 m)")
rect(ax, -10, 11, 10, 42, "#f4efe4", None, 0, 0); note(ax, 0, 26, "open to the hall below\n(the fire, the tables)", 10, "#777")
rect(ax, -10, 11, -7, 36, "#e9d7ae", None, 0, 1); rect(ax, 7, 11, 10, 36, "#e9d7ae", None, 0, 1)
ax.plot([-7, -7], [11, 36], color="#5a3a1a", lw=3, zorder=4); ax.plot([7, 7], [11, 36], color="#5a3a1a", lw=3, zorder=4)
label(ax, -8.5, 24, 1); label(ax, 8.5, 24, 1)
stairs(ax, -9.8, 36, -7.6, 42, 6, "S", "down"); stairs(ax, 7.6, 36, 9.8, 42, 6, "S", "down")
rect(ax, -8, 42, 8, 52, "#e2d0ae", None, 0, 0)
for (a, b, c, d) in ((-8.4, 52, 8.4, 52.4), (-8.4, 42, -8, 52.4), (8, 42, 8.4, 46), (8, 49, 8.4, 52.4), (-10.4, 42, -8, 42.4), (8, 42, 10.4, 42.4),
                     (-2, 52, 2, 52.4)):
    wall(ax, a, b, c, d)
rect(ax, -3, 45, 3, 48, WOOD, "k", 1, 5); note(ax, 0, 46.5, "map table", 8, "white"); label(ax, 0, 44, 2)
rect(ax, -2.5, 51.4, 2.5, 51.9, "#c8b070", "k", 0.6, 5); note(ax, 0, 50.9, "maps on the wall", 7)
rect(ax, -7.8, 43, -7.2, 45.5, WOOD, "k", 0.6, 5); rect(ax, -7.8, 46, -7.2, 48.5, WOOD, "k", 0.6, 5); label(ax, -6.4, 45.8, 3)
rect(ax, 6.5, 50.4, 7.8, 51.8, "#6e4a2a", "k", 0.6, 5); rect(ax, -7.6, 50.6, -6, 51.6, WOOD, "k", 0.6, 5); label(ax, 5.6, 51, 4)
door(ax, 0, 52.2, True, 3.0); label(ax, 0, 53.8, 5); note(ax, 0, 55, "to the Great Porch (north)", 8)
door(ax, 8.2, 47.5, False, 2.0); label(ax, 9.6, 47.5, 6); note(ax, 13, 47.5, "to the Jarl's quarters (east)", 8)
guess(ax, -8.4, 42, 8.4, 52.4); guess(ax, -10, 11, -7, 36); guess(ax, 7, 11, 10, 36)
legend(ax, "1 the galleries (balconies) along both sides of the hall, over its side aisles, reached from the war room\n"
           "2 the war room behind and above the throne: the great map table with its flags, two maps on the wall behind\n"
           "3 two wardrobes   4 a chest, an end table with a locked display case (a dagger); bookshelves\n"
           "5 north door: out to the Great Porch (training yard, dining table, the dragon trap)\n"
           "6 east door: the Jarl's quarters (their own building part: entrance hall with display cases, the steward's room,\n"
           "   the servants' rooms, the brother's and the children's rooms, the Jarl's dining room, study, bedroom, balcony)\n"
           "red dashed: placed by guess")
fig.suptitle("Vindgard - Highhall (Dragonsreach): plan draft, main hall level and upper level (metres, north up)", fontsize=17, weight="bold")
plt.subplots_adjust(left=0.03, right=0.99, top=0.95, bottom=0.2, wspace=0.08)
plt.savefig("docs/maps/highhall_plan_draft.png", dpi=72)
