# The Gilded Stag (inn) and The Bitterroot (apothecary) - interior plan drafts for review, after UESP's descriptions of
# the Bannered Mare and Arcadia's Cauldron. Metres, x east, y north; upper floors at +3.2 m.
# Run: python3 docs/maps/draw_inn_shop.py -> docs/maps/inn_shop_plan_draft.png
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, Circle, FancyArrow

STONE, FLOOR, UPF, WOOD, GREEN, IRON, FIRE = "#6b6258", "#e6d6b8", "#d9c49c", "#9a6b3c", "#7c9a5a", "#4a4f57", "#e08a2a"
fig, axs = plt.subplots(2, 2, figsize=(22, 21))

def setup(ax, title, xr, yr):
    ax.set_title(title, fontsize=14, weight="bold"); ax.set_aspect("equal")
    ax.set_xlim(*xr); ax.set_ylim(*yr); ax.grid(True, color="#ccc", lw=0.5)
    ax.set_xticks(range(xr[0], xr[1] + 1, 1)); ax.set_yticks(range(yr[0], yr[1] + 1, 1)); ax.tick_params(labelsize=7)
def rect(ax, x0, y0, x1, y1, fc, ec="k", lw=0.8, z=1, hatch=None, a=1.0):
    ax.add_patch(Rectangle((x0, y0), x1 - x0, y1 - y0, fc=fc, ec=ec, lw=lw, zorder=z, hatch=hatch, alpha=a))
def wall(ax, x0, y0, x1, y1): rect(ax, x0, y0, x1, y1, STONE, STONE, 0, 3)
def label(ax, x, y, n): ax.text(x, y, str(n), ha="center", va="center", fontsize=9, weight="bold", zorder=9,
                                bbox=dict(boxstyle="circle,pad=0.15", fc="#fff8c0", ec="k", lw=0.6))
def note(ax, x, y, s, fs=8, c="#333", rot=0): ax.text(x, y, s, ha="center", va="center", fontsize=fs, color=c, zorder=8, rotation=rot)
def door(ax, x, y, horiz, w=1.1, c=WOOD): rect(ax, x - (w / 2 if horiz else 0.12), y - (0.12 if horiz else w / 2), x + (w / 2 if horiz else 0.12), y + (0.12 if horiz else w / 2), c, "k", 0.8, 5)
def table(ax, x, y, w=1.0, d=0.7): rect(ax, x - w / 2, y - d / 2, x + w / 2, y + d / 2, WOOD, "k", 0.6, 5)
def chair(ax, x, y): ax.add_patch(Circle((x, y), 0.2, fc="#b88a58", ec="k", lw=0.5, zorder=5))
def bench(ax, x, y, l=2.0, horiz=True): rect(ax, x - (l / 2 if horiz else 0.2), y - (0.2 if horiz else l / 2), x + (l / 2 if horiz else 0.2), y + (0.2 if horiz else l / 2), "#b88a58", "k", 0.6, 5)
def barrel(ax, x, y): ax.add_patch(Circle((x, y), 0.32, fc="#8a5a30", ec="k", lw=0.6, zorder=5))
def person(ax, x, y, name): ax.plot([x], [y], marker="o", ms=11, color="#2a6fd0", zorder=9); note(ax, x, y - 0.42, name, 7, "#1a4fa0")
def stairs(ax, x0, y0, x1, y1, n, up):
    rect(ax, x0, y0, x1, y1, "#cdb994", "k", 0.8, 2)
    for i in range(1, n):
        if up in "NS": y = y0 + (y1 - y0) * i / n; ax.plot([x0, x1], [y, y], color="k", lw=0.4, zorder=3)
        else: x = x0 + (x1 - x0) * i / n; ax.plot([x, x], [y0, y1], color="k", lw=0.4, zorder=3)
    d = {"N": (0, 1), "S": (0, -1), "E": (1, 0), "W": (-1, 0)}[up]; cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    ax.add_patch(FancyArrow(cx - d[0] * 1.0, cy - d[1] * 1.0, d[0] * 1.8, d[1] * 1.8, width=0.06, head_width=0.3, color="#333", zorder=4))
    note(ax, cx + 0.35 * (1 if up in "NS" else 0), cy + 0.35 * (0 if up in "NS" else 1), "up", 7)
def legend(ax, text): ax.text(0.0, -0.05, text, transform=ax.transAxes, fontsize=9.5, va="top", zorder=10, bbox=dict(fc="white", ec="#888"))

# ================================================================== the inn, ground floor
ax = axs[0][0]; setup(ax, "The Gilded Stag (inn) - ground floor", (-1, 17), (-1, 13))
rect(ax, 0, 0, 11, 12, FLOOR, None, 0, 0); rect(ax, 11, 6, 16, 12, "#e2d0ae", None, 0, 0); rect(ax, 11, 0, 16, 6, "#eadcc0", None, 0, 0)
for (a, b, c, d) in ((-0.3, -0.3, 16.3, 0), (-0.3, 12, 16.3, 12.3), (-0.3, -0.3, 0, 5.4), (-0.3, 6.6, 0, 12.3), (16, -0.3, 16.3, 12.3),
                     (11, 0, 11.2, 2.4), (11, 3.6, 11.2, 7.4), (11, 8.6, 11.2, 12), (11.2, 5.9, 16, 6.1)):
    wall(ax, a, b, c, d)
door(ax, -0.15, 6, False, 1.2); label(ax, 0.6, 6, 1); note(ax, -0.8, 6, "from the\nmarket", 7, rot=90)
# common room
rect(ax, 4.0, 5.3, 7.0, 6.7, "#7a6a5a", "k", 1, 4); ax.add_patch(Rectangle((4.4, 5.6), 2.2, 0.8, fc=FIRE, ec="k", lw=0.6, zorder=5)); label(ax, 5.5, 6, 2)
bench(ax, 5.5, 7.3); bench(ax, 5.5, 4.7); chair(ax, 7.5, 6); person(ax, 7.9, 6.5, "bard"); label(ax, 3.4, 7.3, 3)
rect(ax, 3.2, 2.0, 8.2, 2.6, WOOD, "k", 1, 5); chair(ax, 4.8, 3.0); chair(ax, 6.6, 3.0); person(ax, 5.7, 1.3, "innkeeper"); label(ax, 8.6, 2.3, 4)
rect(ax, 2.8, 0.05, 8.6, 0.5, "#7a5230", "k", 0.6, 4); note(ax, 5.7, 0.28, "shelves, barrels, hanging herbs and game", 6, "white")
table(ax, 1.4, 3.0); chair(ax, 1.4, 3.6); chair(ax, 0.6, 3.0); chair(ax, 2.2, 3.0); label(ax, 1.4, 2.2, 5)
table(ax, 1.4, 9.0); chair(ax, 1.4, 8.4); chair(ax, 0.6, 9.0); chair(ax, 2.2, 9.0); label(ax, 1.4, 9.8, 5)
table(ax, 5.5, 11.2, 1.2); chair(ax, 4.6, 11.2); chair(ax, 6.4, 11.2); label(ax, 5.5, 10.4, 6)
table(ax, 10.0, 0.9, 0.9); chair(ax, 9.3, 0.9); chair(ax, 10.6, 1.6); person(ax, 9.3, 1.5, "warrior-woman"); label(ax, 10.3, 0.2, 7)
barrel(ax, 0.45, 10.4); barrel(ax, 0.45, 11.4); rect(ax, 0.1, 10.75, 0.8, 11.05, "#6e4a2a", "k", 0.5, 5); label(ax, 1.2, 10.9, 8)
rect(ax, 10.55, 4.0, 10.95, 5.2, WOOD, "k", 0.6, 5); label(ax, 10.1, 4.6, 9)
stairs(ax, 9.6, 8.6, 10.9, 11.9, 8, "N"); label(ax, 9.0, 10.2, 10)
chair(ax, 10.3, 7.6); chair(ax, 10.3, 7.0)
person(ax, 3.0, 7.8, "serving maid"); person(ax, 7.6, 9.3, "townsman")
# kitchen
door(ax, 11.1, 8.0, False, 1.2)
rect(ax, 11.25, 9.6, 12.0, 11.0, "#7a6a5a", "k", 0.8, 4); ax.add_patch(Circle((11.6, 10.3), 0.3, fc=FIRE, zorder=5)); label(ax, 12.4, 10.3, 11)
door(ax, 12.9, 12.15, True, 1.0); note(ax, 12.9, 12.7, "back door, chopping block outside", 7)
table(ax, 14.2, 11.2); chair(ax, 13.4, 11.2); chair(ax, 15.0, 11.2); label(ax, 14.2, 10.4, 12)
rect(ax, 11.4, 6.2, 14.2, 6.6, "#7a5230", "k", 0.6, 5); barrel(ax, 15.55, 6.55); label(ax, 12.8, 7.0, 13)
rect(ax, 15.55, 7.4, 15.95, 8.6, WOOD, "k", 0.6, 5); label(ax, 15.1, 8.0, 14)
table(ax, 13.2, 8.4, 1.0, 0.6); label(ax, 13.2, 9.0, 15)
stairs(ax, 14.9, 9.0, 16.0, 10.6, 6, "N"); label(ax, 15.45, 8.6, 16)
# the east bedroom
door(ax, 11.1, 3.0, False, 1.1)
rect(ax, 14.4, 0.1, 15.9, 2.2, "#b38a5a", "k", 0.8, 5); label(ax, 15.1, 1.2, 17)
rect(ax, 13.6, 0.1, 14.2, 0.6, WOOD, "k", 0.5, 5); rect(ax, 15.5, 3.0, 15.9, 4.0, WOOD, "k", 0.5, 5)
table(ax, 14.8, 5.2, 0.8, 0.6); chair(ax, 14.0, 5.2); rect(ax, 11.3, 1.4, 11.8, 2.4, WOOD, "k", 0.5, 5); label(ax, 12.6, 3.6, 18)
legend(ax, "1 front door (from the market)   2 fire pit   3 two benches + the bard's chair\n"
           "4 the bar: counter, two stools; innkeeper behind it   5 tables with three chairs by the door\n"
           "6 table, two chairs (north wall)   7 corner table (a warrior-woman sits here)   8 food barrels, a chest\n"
           "9 narrow table on the east wall (an iron sword, goods)   10 stairs up to the room for rent\n"
           "KITCHEN 11 fire pit with a spit, back door to its right   12 table, two chairs, plate shelves above\n"
           "13 shelves, food sacks; barrel in the corner   14 cupboard (special ale)   15 table: ledger, strongbox\n"
           "16 stairs up to the staff rooms   EAST BEDROOM 17 bed, end table   18 cupboard, low table + chair, wardrobe")

# ================================================================== the inn, upper floor
ax = axs[0][1]; setup(ax, "The Gilded Stag - upper floor (+3.2 m)", (-1, 17), (-1, 13))
rect(ax, 0, 0, 11, 12, "#f4efe4", None, 0, 0, hatch="..")
note(ax, 4.5, 4, "open to the hall below\n(rafters, the fire's smoke)", 9, "#777")
rect(ax, 2.6, 8.4, 11, 12, UPF, None, 0, 1); rect(ax, 2.6, 7.0, 9.6, 8.4, "#e9d7ae", None, 0, 1); rect(ax, 9.6, 8.6, 11, 12, "#e9d7ae", None, 0, 1)
for (a, b, c, d) in ((2.4, 8.4, 2.6, 12.3), (2.4, 12, 11.2, 12.3), (2.6, 8.4, 8.0, 8.55), (9.0, 8.4, 9.6, 8.55), (9.5, 8.6, 9.65, 10.4), (9.5, 11.4, 9.65, 12)):
    wall(ax, a, b, c, d)
ax.plot([2.6, 9.6], [7.0, 7.0], color="#5a3a1a", lw=3, zorder=4); note(ax, 6.0, 6.7, "railing over the hall", 7)
label(ax, 10.3, 9.0, 1); rect(ax, 10.55, 10.6, 10.95, 11.6, WOOD, "k", 0.5, 5)
door(ax, 9.57, 10.9, False, 1.0)
rect(ax, 2.7, 9.6, 4.6, 11.9, "#b38a5a", "k", 0.8, 5); rect(ax, 2.7, 11.3, 4.6, 11.9, "#ddd", "k", 0.5, 6); label(ax, 3.6, 10.4, 2)
rect(ax, 2.7, 9.0, 3.2, 9.5, WOOD, "k", 0.5, 5); rect(ax, 2.7, 8.6, 3.0, 8.9, WOOD, "k", 0.5, 5)
rect(ax, 8.9, 10.0, 9.4, 11.4, WOOD, "k", 0.5, 5); label(ax, 8.4, 10.7, 3)
door(ax, 8.5, 8.45, True, 1.0); label(ax, 8.5, 9.0, 4)
rect(ax, 2.8, 7.1, 3.3, 7.6, WOOD, "k", 0.5, 5); table(ax, 6.0, 7.7, 0.9, 0.5); chair(ax, 5.2, 7.7); chair(ax, 6.8, 7.7); label(ax, 4.2, 7.7, 5)
# staff floor over the kitchen and the bedroom
rect(ax, 11, 0, 16, 12, UPF, None, 0, 0)
for (a, b, c, d) in ((10.8, -0.3, 16.3, 0), (10.8, 12, 16.3, 12.3), (10.8, -0.3, 11.0, 12.3), (16, -0.3, 16.3, 12.3),
                     (11.0, 8.9, 13.4, 9.1), (14.4, 8.9, 14.9, 9.1), (11.0, 5.9, 12.4, 6.1), (13.4, 5.9, 16, 6.1)):
    wall(ax, a, b, c, d)
stairs(ax, 14.9, 9.0, 16.0, 10.6, 6, "N"); note(ax, 15.45, 11.2, "from the\nkitchen", 7)
rect(ax, 11.1, 7.0, 11.5, 8.0, WOOD, "k", 0.5, 5); label(ax, 12.2, 7.5, 6)
door(ax, 13.9, 9.0, True, 1.0); door(ax, 12.9, 6.0, True, 1.0)
rect(ax, 14.9, 10.8, 15.9, 11.9, "#b38a5a", "k", 0.8, 5); rect(ax, 14.3, 11.5, 14.7, 11.9, WOOD, "k", 0.5, 5)
rect(ax, 14.9, 10.6, 15.9, 10.75, WOOD, "k", 0.5, 6); table(ax, 12.0, 10.0, 0.7, 0.7); chair(ax, 11.4, 10.0); chair(ax, 12.6, 10.0); label(ax, 13.2, 11.2, 7)
rect(ax, 14.9, 0.1, 15.9, 2.2, "#b38a5a", "k", 0.8, 5); chair(ax, 14.3, 2.6); rect(ax, 14.1, 0.1, 14.6, 0.6, WOOD, "k", 0.5, 5)
table(ax, 15.4, 3.2, 1.0, 0.5); rect(ax, 15.5, 4.0, 15.9, 5.4, WOOD, "k", 0.5, 5); label(ax, 13.2, 3.0, 8)
legend(ax, "ROOM FOR RENT (up the hall's stairs, 10 gold a night)   1 landing with a cupboard\n"
           "2 double bed on the west wall, end tables   3 dresser with a washbasin (east wall)   4 doorway to the balcony\n"
           "5 balcony over the hall: cupboard, low table, two low chairs\n"
           "STAFF ROOMS (up the kitchen stairs)   6 landing with a small wardrobe\n"
           "7 north: the serving maid's room - bed, end table, washbasin, wardrobe with a chest on it, table + 2 chairs\n"
           "8 south: the innkeeper's room - bed (a book under it), chair, end table, table with a lute and cups,\n"
           "   dresser with a drum and pottery")

# ================================================================== the apothecary, ground floor
ax = axs[1][0]; setup(ax, "The Bitterroot (apothecary) - ground floor", (-4, 11), (-4, 10))
rect(ax, 0, 0, 10, 9, FLOOR, None, 0, 0); rect(ax, -3, 0, 0, 4, "#e2d0ae", None, 0, 0); rect(ax, -3, 4.2, 0, 9, "#eadcc0", None, 0, 0)
rect(ax, 4, -3, 8, 0, "#eadcc0", None, 0, 0)
for (a, b, c, d) in ((-3.3, -0.3, 4, 0), (8, -0.3, 10.3, 0), (4, -0.3, 5.3, 0), (6.3, -0.3, 8, 0), (3.8, -3.3, 8.2, -3), (3.8, -3.3, 4, 0), (8, -3.3, 8.2, 0),
                     (-3.3, 9, 2.4, 9.3), (3.6, 9, 10.3, 9.3), (10, -0.3, 10.3, 6.4), (10, 7.6, 10.3, 9.3), (-3.3, -0.3, -3, 9.3),
                     (-0.15, -0.3, 0.05, 1.6), (-0.15, 2.7, 0.05, 6.0), (-0.15, 7.1, 0.05, 9.3), (-3, 4.0, 0, 4.2)):
    wall(ax, a, b, c, d)
door(ax, 3.0, 9.15, True, 1.2); label(ax, 3.0, 8.4, 1); note(ax, 3.0, 9.7, "front door, from the market", 7)
door(ax, 10.15, 7.0, False, 1.2); label(ax, 9.4, 7.0, 2); note(ax, 10.7, 7.0, "back\ndoor", 7)
rect(ax, 5.6, 8.4, 7.8, 9.0, "#7a6a5a", "k", 1, 4); ax.add_patch(Rectangle((6.0, 8.5), 1.4, 0.4, fc=FIRE, zorder=5)); label(ax, 6.7, 7.8, 3)
rect(ax, 4.2, 3.8, 5.8, 5.0, "#6a7a5a", "k", 1, 5); ax.add_patch(Circle((5.0, 4.4), 0.3, fc="#a0c0e0", ec="k", zorder=6)); label(ax, 5.0, 3.2, 4)
rect(ax, 8.2, 2.6, 8.8, 6.4, WOOD, "k", 1, 5); person(ax, 9.4, 4.5, "apothecary"); label(ax, 7.6, 4.5, 5)
for (x, y) in ((9.5, 0.5), (8.8, 0.5)): ax.add_patch(Circle((x, y), 0.3, fc="#c8b080", ec="k", zorder=5))
rect(ax, 6.6, 0.05, 7.9, 0.5, WOOD, "k", 0.6, 5); label(ax, 8.6, 1.2, 6)
door(ax, 5.8, 0.0, True, 1.0); label(ax, 5.8, 0.7, 7)
table(ax, 2.4, 6.0, 1.2, 0.8); label(ax, 2.4, 6.8, 8)
rect(ax, 0.1, 0.4, 0.5, 2.2, "#7a5230", "k", 0.6, 5); label(ax, 1.0, 1.3, 9)
door(ax, -0.05, 2.15, False, 1.0, IRON); label(ax, 0.6, 2.15, 10); door(ax, -0.05, 6.55, False, 1.0); label(ax, 0.6, 6.55, 11)
rect(ax, -2.8, 0.2, -1.8, 0.8, "#6e4a2a", "k", 0.6, 5); barrel(ax, -2.5, 3.4); barrel(ax, -1.6, 3.4); barrel(ax, -0.7, 3.5); note(ax, -1.5, 2.0, "storage\n(locked)", 8)
stairs(ax, -2.9, 4.4, -0.2, 8.9, 9, "N"); note(ax, -1.5, 3.9, "", 7)
rect(ax, 4.2, -2.9, 5.4, -2.3, WOOD, "k", 0.6, 5); chair(ax, 4.8, -1.9); rect(ax, 7.4, -2.9, 7.9, -1.0, "#7a5230", "k", 0.6, 5); label(ax, 6.0, -1.5, 12)
for (x, y) in ((1.5, 3.0), (3.2, 2.0), (7.0, 7.2), (1.0, 8.2)): ax.plot([x], [y], marker="s", ms=6, color="#7a9a50", zorder=6)
legend(ax, "1 front door (market side)   2 back door (east)   3 large fireplace (north wall)\n"
           "4 the alchemy lab in the middle of the room   5 the long desk on the east side, the apothecary behind it (8am-8pm)\n"
           "6 south-east corner: two food sacks, common books, a cupboard   7 door to her private study (12: desk, chair, shelves)\n"
           "8 table: a bowl of green apples, a cream treat, the herbalist's guide (alchemy book)\n"
           "9 south-west shelves: books, goat cheese, bread, a sack   10 locked door to the storage room (a chest, three barrels)\n"
           "11 door to the stairs up   green squares: potions and ingredients on display (to buy or steal)")

# ================================================================== the apothecary, upper floor
ax = axs[1][1]; setup(ax, "The Bitterroot - upper floor (+3.2 m)", (-4, 11), (-4, 10))
rect(ax, 0, 0, 10, 9, "#f4efe4", None, 0, 0, hatch=".."); note(ax, 6.5, 4.5, "open to the shop below", 9, "#777")
rect(ax, -3, 4.2, 0, 9, UPF, None, 0, 0); rect(ax, 0, 0, 2.6, 9, UPF, None, 0, 1); rect(ax, 0, -3.5, 5, 0, "#eadcc0", None, 0, 0)
for (a, b, c, d) in ((-3.3, 9, 2.8, 9.3), (-3.3, 4.0, -3, 9.3), (-3, 4.0, 0, 4.2), (-0.1, 0.0, 0.05, 4.2), (-0.2, -3.8, 5.2, -3.5), (-0.2, -3.8, 0, 0.2),
                     (5, -3.8, 5.2, 0.2), (0, -0.15, 0.6, 0.05), (1.6, -0.15, 5.2, 0.05)):
    wall(ax, a, b, c, d)
ax.plot([2.6, 2.6], [0.1, 9.0], color="#5a3a1a", lw=3, zorder=4); note(ax, 2.95, 6.0, "railing", 7, rot=90)
stairs(ax, -2.9, 4.4, -0.2, 8.9, 9, "N"); note(ax, -1.5, 4.0, "from below", 7)
rect(ax, 0.1, 2.2, 0.5, 3.2, WOOD, "k", 0.5, 5); rect(ax, 0.1, 3.6, 0.5, 4.6, WOOD, "k", 0.5, 5); label(ax, 1.0, 3.4, 1)
table(ax, 0.8, 8.2, 1.0, 0.7); ax.add_patch(Circle((1.9, 8.4), 0.32, fc="#555", ec="k", zorder=5)); label(ax, 1.4, 7.3, 2)
door(ax, 1.1, -0.05, True, 1.0); label(ax, 1.1, 0.6, 3)
rect(ax, 0.2, -3.3, 1.8, -1.1, "#b38a5a", "k", 0.8, 5); rect(ax, 3.8, -3.4, 4.9, -2.9, WOOD, "k", 0.5, 5)
table(ax, 3.6, -1.0, 0.8, 0.5); rect(ax, 4.4, -2.2, 4.9, -1.8, "#555", "k", 0.5, 5); label(ax, 2.6, -2.2, 4)
legend(ax, "the stairs come up behind the west wall to a gallery that overlooks the shop\n"
           "1 two wardrobes (clothes)   2 north-west: a table with bread, wine and grilled chicken, the cooking pot\n"
           "   (with a spit), a food sack under the table   3 door (south) to her own room\n"
           "4 her room: bed, wardrobe, a table with the business ledger, a strongbox")
fig.suptitle("Vindgard - The Gilded Stag (inn) and The Bitterroot (apothecary): plan drafts for correction (metres, north up)", fontsize=16, weight="bold")
plt.subplots_adjust(left=0.03, right=0.99, top=0.95, bottom=0.08, wspace=0.08, hspace=0.32)
plt.savefig("docs/maps/inn_shop_plan_draft.png", dpi=80)
