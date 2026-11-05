# overlay the drawn city map (ref_whiterun_city_map.png) on the official map (ref_official_map.png)
import numpy as np
from PIL import Image, ImageDraw, ImageFont
D = "/home/claude/game/docs/maps/"
off = Image.open(D + "ref_official_map.png").convert("RGB")
city = Image.open(D + "ref_whiterun_city_map.png").convert("RGBA")
# matching landmarks: official px -> city map px
pairs = [((522, 420), (725, 1060)),   # Gildergreen
         ((685, 425), (1120, 1050)),  # Jorrvaskr
         ((700, 150), (1110, 290)),   # Dragonsreach
         ((262, 578), (40, 1470)),    # main gate
         ((275, 487), (40, 1215)),    # tier wall, west end
         ((630, 500), (1030, 1255)),  # tier wall, east end
         ((500, 748), (645, 1868))]   # south wall gatehouse
A = []; b = []
for (ox, oy), (cx, cy) in pairs:
    A.append([cx, cy, 1, 0, 0, 0]); b.append(ox)
    A.append([0, 0, 0, cx, cy, 1]); b.append(oy)
p, *_ = np.linalg.lstsq(np.array(A, float), np.array(b, float), rcond=None)
for (ox, oy), (cx, cy) in pairs:
    print("landmark", (ox, oy), "fit", (round(p[0]*cx + p[1]*cy + p[2]), round(p[3]*cx + p[4]*cy + p[5])))
print("scale official px per city-map px:", np.hypot(p[0], p[3]), np.hypot(p[1], p[4]))
# PIL wants the inverse map: official -> city
M = np.array([[p[0], p[1], p[2]], [p[3], p[4], p[5]], [0, 0, 1]])
inv = np.linalg.inv(M)
S = 2                                  # draw at twice the official map's size
big = off.resize((off.width * S, off.height * S), Image.LANCZOS)
coef = (inv[0][0] / S, inv[0][1] / S, inv[0][2], inv[1][0] / S, inv[1][1] / S, inv[1][2])
warp = city.transform(big.size, Image.AFFINE, coef, resample=Image.BILINEAR)
a = np.array(warp)
mask = (a[..., 3] > 0) & (a[..., :3].sum(axis=2) > 60)        # the drawn map's black background is left out
a[..., 3] = np.where(mask, 150, 0).astype(np.uint8)
out = big.convert("RGBA"); out.alpha_composite(Image.fromarray(a))
d = ImageDraw.Draw(out)
F = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 22)
names = ["Gildergreen", "Jorrvaskr", "Dragonsreach", "main gate", "tier wall W", "tier wall E", "south gatehouse"]
for ((ox, oy), _), n in zip(pairs, names):
    X, Y = ox * S, oy * S
    d.ellipse([X - 9, Y - 9, X + 9, Y + 9], outline=(255, 0, 0), width=4)
    d.text((X + 12, Y - 12), n, fill=(255, 40, 40), font=F, stroke_width=3, stroke_fill=(255, 255, 255))
out.convert("RGB").save(D + "overlay_city_on_official.png")
