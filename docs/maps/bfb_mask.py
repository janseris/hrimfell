# Bleak Falls Barrow: the walkable area of each map, sampled on a 0.75 m grid (two samples per map square,
# a map square = 1.5 m). A sample is floor when most of its pixels have the maps' cream floor colour.
from PIL import Image
import os
REF = os.path.join(os.path.dirname(__file__), 'ref', 'bfb')
MAPS = {
    # name: (image, squares across, squares down)
    'entry':   ('bfb_barrow_entry_grid.png', 62, 25),
    'temple':  ('bfb_temple_grid.png', 60, 50),
    'sanctum': ('bfb_sanctum_grid.png', 106, 30),
}
SUB = 2          # samples per map square

def is_floor(p):
    r, g, b = p
    return r > 228 and g > 222 and b > 205

def mask(name):
    fn, cols, rows = MAPS[name]
    im = Image.open(os.path.join(REF, fn)).convert('RGB')
    W, H = im.size
    sx = W / (cols * SUB); sy = H / (rows * SUB)
    px = im.load()
    out = []
    for j in range(rows * SUB):
        row = []
        for i in range(cols * SUB):
            x0, y0 = int(i * sx), int(j * sy)
            x1, y1 = max(x0 + 1, int((i + 1) * sx)), max(y0 + 1, int((j + 1) * sy))
            n = f = 0
            for y in range(y0, y1):
                for x in range(x0, x1):
                    n += 1
                    if is_floor(px[x, y]): f += 1
            row.append(1 if f * 100 >= n * 55 else 0)
        out.append(row)
    return out

if __name__ == '__main__':
    import sys
    for name in MAPS:
        m = mask(name)
        H, W = len(m), len(m[0])
        im = Image.new('RGB', (W * 6, H * 6), (60, 50, 40))
        p = im.load()
        for j in range(H):
            for i in range(W):
                if m[j][i]:
                    for y in range(6):
                        for x in range(6): p[i * 6 + x, j * 6 + y] = (230, 220, 200)
        out = sys.argv[1] if len(sys.argv) > 1 else '.'
        im.save(os.path.join(out, 'mask_%s.png' % name))
        print(name, W, H, sum(map(sum, m)))
