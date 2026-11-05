#!/usr/bin/env python3
"""Draws a dragon icon for Series 80 (Nokia 9300 / 9500), the view from below and in front: an
original crimson dragon flying over and toward the viewer; the small head at the top with the
snout toward us and golden horns, the slim body with its belly plates toward us, no forelegs (the wings are its front limbs, the
claw at each wing's wrist), hind legs folded with talons spread, the thick tail sweeping away to one side, banking with
the wings raised (the near one wide, the far one higher), lit through from above. It's 3D-shaded from a height field built from the shapes (blurred
masks give rounded forms, their slopes the normals), lit from the upper left with highlights, in
the style of the Browser 9300 globe. Original artwork, generated here (no game's emblem or
characters). 8x supersampled.

Writes icon64.bmp + icon64_mask.bmp (64x50), icon25.bmp + icon25_mask.bmp (25x20) and
preview.png. Masks are black where the icon is drawn (the 9300's convention), 1-bit, with edge
pixels blended toward the Desk's light background."""
import math, os, sys
import numpy as np
from PIL import Image, ImageDraw

BG = np.array([235, 235, 235], float)
SS = 8


def lin(c0, c1, t):
    return np.array(c0, float) + (np.array(c1, float) - np.array(c0, float)) * np.clip(t, 0, 1)[..., None]


def blur(a, r):
    """three box blurs of radius r (about a gaussian)"""
    r = max(1, int(r))
    for _ in range(3):
        for axis in (0, 1):
            c = np.cumsum(np.pad(a, [(r + 1, r) if k == axis else (0, 0) for k in range(2)], mode="edge"), axis=axis)
            hi = np.take(c, np.arange(2 * r + 1, c.shape[axis]), axis=axis)
            lo = np.take(c, np.arange(0, c.shape[axis] - 2 * r - 1), axis=axis)
            a = (hi - lo) / (2 * r + 1)
    return a


def shapes(W, H):
    """the dragon's parts as masks, drawn at 8x in 64x50 design units: flying over and toward the
    viewer, seen from below and in front: the head at the top with the snout toward us, the belly
    facing us, the legs and the tail hanging, the wings raised in a V"""
    w, h = W * SS, H * SS
    k, ky = w / 64.0, h / 50.0

    def P(pts):
        return [(x * k, y * ky) for x, y in pts]

    def new():
        return Image.new("L", (w, h), 0)

    def done(im):
        return np.asarray(im, float) / 255.0

    def curve(d, pts_fn, n, width_fn):
        """a tapering band along a curve"""
        a_, b_ = [], []
        prev = None
        for i in range(n + 1):
            t = i / float(n)
            x, y = pts_fn(t)
            if prev is None:
                x2, y2 = pts_fn(min(1.0, t + 0.01))
                dx, dy = x2 - x, y2 - y
            else:
                dx, dy = x - prev[0], y - prev[1]
            ln = math.hypot(dx, dy) or 1.0
            nx_, ny_ = -dy / ln, dx / ln
            wd = width_fn(t) / 2
            a_.append((x + nx_ * wd, y + ny_ * wd)); b_.append((x - nx_ * wd, y - ny_ * wd))
            prev = (x, y)
        d.polygon(P(a_ + b_[::-1]), fill=255)

    def bez(p0, p1, p2, p3):
        def f(t):
            u_ = 1 - t
            return (u_ ** 3 * p0[0] + 3 * u_ * u_ * t * p1[0] + 3 * u_ * t * t * p2[0] + t ** 3 * p3[0],
                    u_ ** 3 * p0[1] + 3 * u_ * u_ * t * p1[1] + 3 * u_ * t * t * p2[1] + t ** 3 * p3[1])
        return f

    wing = new(); dw = ImageDraw.Draw(wing)
    bones = new(); db = ImageDraw.Draw(bones)
    body = new(); dbo = ImageDraw.Draw(body)
    belly = new(); dbe = ImageDraw.Draw(belly)
    teeth = new(); dt = ImageDraw.Draw(teeth)      # ivory: the claws and talons
    limbs = new(); dli = ImageDraw.Draw(limbs)     # the thighs: shaded on their own, outlined
    shins = new(); dsh = ImageDraw.Draw(shins)     # below the knees and the feet, drawn over the thighs

    def claws(x, y, ang0, spread, ln, n=3):
        for i in range(n):
            a_ = math.radians(ang0 + (i - (n - 1) / 2.0) * spread)
            cx_, cy_ = math.cos(a_), math.sin(a_)
            px_, py_ = -cy_ * 0.45, cx_ * 0.45
            tri = [(x + px_, y + py_), (x - px_, y - py_), (x + cx_ * ln, y + cy_ * ln)]
            dli.polygon(P(tri), fill=255); dt.polygon(P(tri), fill=255)

    # the wings, banking: the near (left) wing spread wide, the far one raised higher and
    # foreshortened; deep scallops between the finger bones, a thumb claw at each wrist
    for side, (sx, sy), elbow, tip, fingers in [
            (-1, (29.0, 21.0), (17.0, 11.5), (0.8, 4.0), [(1.5, 17.0), (6.0, 24.5), (14.0, 28.5), (22.5, 27.5)]),
            (+1, (35.0, 20.0), (44.5, 8.0), (61.0, 0.8), [(62.8, 10.0), (58.0, 17.5), (50.5, 21.5), (42.0, 24.0)])]:
        trail = [tip]
        prev = tip
        for f in fingers + [(32.6 + side * 2.4, 29.0)]:
            mx, my = (prev[0] + f[0]) / 2, (prev[1] + f[1]) / 2
            bx, by = elbow
            trail.append((mx + (bx - mx) * 0.33, my + (by - my) * 0.33))
            trail.append(f)
            prev = f
        dw.polygon(P([(sx, sy), elbow, tip] + trail[1:]), fill=255)
        db.line(P([(sx, sy), elbow]), fill=255, width=max(1, int(2.0 * k)))
        db.line(P([elbow, tip]), fill=255, width=max(1, int(1.4 * k)))
        for f in fingers:
            db.line(P([elbow, f]), fill=255, width=max(1, int(0.7 * k)))
        claws(elbow[0], elbow[1], -90 - side * 30, 0, 2.2, n=1)
    # the tail: as thick as the hips where it starts, sweeping down and away to the left in one
    # long curve, tapering to a small spade
    tail = bez((32.6, 29.0), (32.5, 41.0), (22.0, 47.5), (11.0, 43.0))
    curve(dbo, tail, 30, lambda t: 5.4 * (1 - t) ** 0.9 + 0.5)
    curve(dbe, lambda t: tail(t * 0.85), 26, lambda t: 1.6 * (1 - t) + 0.2)
    tx, ty = tail(1.0); tx0, ty0 = tail(0.97)
    dx_, dy_ = tx - tx0, ty - ty0; ln_ = math.hypot(dx_, dy_); dx_, dy_ = dx_ / ln_, dy_ / ln_
    dbo.polygon(P([(tx - dy_ * 1.8, ty + dx_ * 1.8), (tx + dx_ * 3.2, ty + dy_ * 3.2), (tx + dy_ * 1.8, ty - dx_ * 1.8),
                   (tx + dx_ * 0.6, ty + dy_ * 0.6)]), fill=255)
    # the body: a deep chest narrowing to the hips, the belly scutes toward us
    curve(dbo, lambda t: (32.0 + 0.6 * t, 18.5 + 12.5 * t), 20,
          lambda t: 6.4 + 3.4 * math.sin(math.pi * min(1.0, t * 1.1)) - 2.6 * t)
    curve(dbe, lambda t: (32.0 + 0.6 * t, 19.0 + 12.0 * t), 20,
          lambda t: 2.0 + 1.6 * math.sin(math.pi * min(1.0, t * 1.25)) - 0.6 * t)
    def seg(d, a, b, w0, w1):
        curve(d, lambda t: (a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t), 6, lambda t: w0 + (w1 - w0) * t)
    # hind legs: heavy thighs at the hips, the shins folded down and out, feet with spread talons
    for side in (-1, 1):
        hp = (32.6 + side * 2.2, 28.4)                  # the hip joint, inside the body's side
        kn = (32.6 + side * 7.0, 31.4)                  # crouched: the knee out to the side
        ft = (32.6 + side * 5.0, 35.8)                  # the shin folds back down under it
        dli.ellipse(P([(hp[0] - 2.6, hp[1] - 2.6), (hp[0] + 2.6, hp[1] + 2.6)]), fill=255)
        seg(dli, hp, kn, 4.6, 3.0)                      # the thigh: heavy, tapering to the knee
        dli.ellipse(P([(kn[0] - 1.6, kn[1] - 1.6), (kn[0] + 1.6, kn[1] + 1.6)]), fill=255)
        seg(dsh, kn, ft, 2.6, 1.9)                      # the shin: its own part, outlined at the knee
        # a foot: a small pad, three long separate toes spreading and curling down, each ending in a hooked claw
        dsh.ellipse(P([(ft[0] - 1.4, ft[1] - 1.1), (ft[0] + 1.4, ft[1] + 1.1)]), fill=255)
        for j, ang in enumerate((-50, 0, 50)):
            a0 = math.radians(90 - side * 22 + ang)
            curl = math.radians(18 if ang < 0 else (-18 if ang > 0 else 10 * side))
            ln = 3.0 if abs(ang) < 30 else 2.6
            def toe(t, a0=a0, curl=curl, ln=ln):
                a_ = a0 + curl * t
                return (ft[0] + math.cos(a0) * 0.8 + math.cos(a_) * ln * t, ft[1] + math.sin(a0) * 0.8 + math.sin(a_) * ln * t)
            curve(dsh, toe, 8, lambda t: 1.2 - 0.45 * t)
            tx_, ty_ = toe(1.0)
            ae = a0 + curl * 1.6                          # the claw hooks on the way the toe curls
            cx_, cy_ = math.cos(ae), math.sin(ae)
            px_, py_ = -cy_ * 0.5, cx_ * 0.5
            hook = [(tx_ + px_, ty_ + py_), (tx_ - px_, ty_ - py_),
                    (tx_ + cx_ * 2.7 + math.cos(ae + curl * 3) * 0.5, ty_ + cy_ * 2.7 + math.sin(ae + curl * 3) * 0.5)]
            dsh.polygon(P(hook), fill=255); dt.polygon(P(hook), fill=255)
    # the neck: a slight S up to the head, thick at the chest
    neck = bez((32.2, 20.0), (32.0, 16.5), (32.6, 14.0), (32.4, 10.5))
    curve(dbo, neck, 14, lambda t: 5.8 - 2.2 * t)
    curve(dbe, neck, 14, lambda t: 2.4 - 0.9 * t)
    spikes = new()
    head = new(); dh = ImageDraw.Draw(head)
    # the head: small, turned a little, the snout toward us and down, the jaw's underside showing
    ox = 0.4
    dh.ellipse(P([(29.0 + ox, 5.0), (35.0 + ox, 11.0)]), fill=255)
    dh.polygon(P([(29.9 + ox, 8.5), (34.1 + ox, 8.5), (33.5 + ox, 14.0), (32.0 + ox, 15.2), (30.5 + ox, 14.0)]), fill=255)
    dh.polygon(P([(29.2 + ox, 7.0), (27.0 + ox, 9.4), (29.8 + ox, 9.4)]), fill=255)         # cheek spikes
    dh.polygon(P([(34.8 + ox, 7.0), (37.0 + ox, 9.4), (34.2 + ox, 9.4)]), fill=255)
    dbe.polygon(P([(30.9 + ox, 10.0), (33.1 + ox, 10.0), (32.8 + ox, 13.8), (32.0 + ox, 14.6), (31.2 + ox, 13.8)]), fill=255)
    mouth = new(); dm = ImageDraw.Draw(mouth)
    dm.line(P([(30.7 + ox, 12.6), (32.0 + ox, 13.6), (33.3 + ox, 12.6)]), fill=255, width=max(1, int(0.5 * k)))
    for tx_ in (31.0, 33.0):
        dt.polygon(P([(tx_ + ox - 0.3, 12.8), (tx_ + ox + 0.3, 12.8), (tx_ + ox, 13.7)]), fill=255)
    horns = new(); dho = ImageDraw.Draw(horns)
    for side in (-1, 1):
        curve(dho, lambda t, s_=side: (32.0 + ox + s_ * (2.2 + 2.6 * t + 1.2 * t * t), 6.6 - 5.2 * t + 1.0 * t * t), 12,
              lambda t: 2.3 * (1 - t) + 0.3)
    eye = new(); de = ImageDraw.Draw(eye)
    for side in (-1, 1):
        ex = 32.0 + ox + side * 2.1
        de.polygon(P([(ex - side * 0.9, 7.9), (ex + side * 0.9, 7.2), (ex + side * 0.8, 8.1), (ex - side * 0.7, 8.5)]), fill=255)
    brow = new(); dbr = ImageDraw.Draw(brow)
    dbr.polygon(P([(29.4 + ox, 7.0), (31.4 + ox, 6.3), (32.0 + ox, 7.0), (32.6 + ox, 6.3), (34.6 + ox, 7.0), (34.3 + ox, 7.4), (29.7 + ox, 7.4)]), fill=255)
    nostril = new(); dn = ImageDraw.Draw(nostril)
    for nx_ in (31.4, 32.6):
        dn.ellipse(P([(nx_ + ox - 0.35, 13.6), (nx_ + ox + 0.35, 14.3)]), fill=255)
    fire = new()
    return {n: done(i) for n, i in [("wing", wing), ("bones", bones), ("limbs", limbs), ("shins", shins), ("body", body), ("belly", belly),
                                    ("spikes", spikes), ("head", head), ("mouth", mouth), ("teeth", teeth),
                                    ("horns", horns), ("eye", eye), ("brow", brow), ("nostril", nostril),
                                    ("fire", fire)]}


def render(W, H):
    small = W < 40
    w, h = W * SS, H * SS
    m = shapes(W, H)
    ys, xs = np.mgrid[0:h, 0:w].astype(float)
    u = w / 64.0
    L = np.array([-0.55, -0.65, 0.52]); L /= np.linalg.norm(L)
    Hv = L + np.array([0, 0, 1.0]); Hv /= np.linalg.norm(Hv)

    def shade(height, strength):
        gy, gx = np.gradient(height)
        nx, ny, nz = -gx * strength, -gy * strength, np.ones_like(height)
        n = np.sqrt(nx * nx + ny * ny + nz * nz)
        nx, ny, nz = nx / n, ny / n, nz / n
        return np.clip(nx * L[0] + ny * L[1] + nz * L[2], 0, 1), np.clip(nx * Hv[0] + ny * Hv[1] + nz * Hv[2], 0, 1)

    col = np.zeros((h, w, 3)) + BG
    # ---- the wings: a membrane with raised bones
    Hw = blur(m["wing"], u * 1.2) * 0.25 + blur(m["bones"], u * 0.5) * 0.9
    dw_, sw_ = shade(Hw, u * 7.0)
    # (from below the light comes through: brighter toward the thin outer parts and the top)
    thin = blur(m["wing"], u * 3.0)
    glow = np.clip(1.05 - thin * 1.1 + 0.30 * (1 - ys / h), 0, 1)
    veins = blur(m["bones"], u * 1.6)
    glow = glow * (1 - 0.35 * np.clip(veins * 3, 0, 1))
    mem = lin([70, 10, 12], [215, 75, 40], glow)
    bonec = lin([30, 6, 8], [95, 22, 18], dw_)
    wc = np.where((m["bones"] > 0.5)[..., None], bonec, mem)
    col = np.where((m["wing"] > 0.5)[..., None], wc, col)
    # ---- the body: muscular, scales, lighter belly plates, spikes along the neck
    Hb = blur(m["body"], u * 1.2) + blur(m["belly"], u * 0.6) * 0.2
    if not small:
        sc = (np.sin(xs / (u * 1.1) * math.pi + (np.floor(ys / (u * 0.9)) % 2) * 1.57) * np.sin(ys / (u * 0.9) * math.pi)) ** 2
        Hb = Hb + 0.020 * blur(sc, 1) * m["body"] * (1 - m["belly"])
    db_, sb_ = shade(Hb, u * 9.0)
    bc = lin([45, 6, 9], [185, 45, 32], db_ * 1.1 - 0.05)
    bel = m["belly"] > 0.5
    bc = np.where(bel[..., None], lin([105, 55, 28], [215, 150, 85], db_ * 0.7 + 0.3), bc)
    plates = bel & ((np.floor(ys / (u * 1.4)) % 2) == 0)
    bc = np.where(plates[..., None], bc * 0.78, bc)
    bc = bc + (np.array([255, 210, 170.]) - bc) * (sb_ ** 30 * 0.35)[..., None]
    col = np.where((m["body"] > 0.5)[..., None], bc, col)
    Hl = blur(m["limbs"], u * 0.7)
    dl_, sl_ = shade(Hl, u * 7.0)
    lc = lin([50, 7, 9], [215, 62, 42], dl_ * 1.15 - 0.05)
    lc = lc + (np.array([255, 205, 165.]) - lc) * (sl_ ** 30 * 0.35)[..., None]
    edge = (m["limbs"] > 0.5) & (blur(m["limbs"], u * 0.35) < 0.72)
    lc = np.where(edge[..., None], np.array([28, 4, 6.]), lc)
    behind = (m["belly"] > 0.5) | (blur(m["belly"], u * 0.6) > 0.25)   # the thighs sit behind the belly
    col = np.where(((m["limbs"] > 0.5) & ~behind)[..., None], lc, col)
    Hk = blur(m["shins"], u * 0.6)
    dk_, sk_ = shade(Hk, u * 7.0)
    kc = lin([50, 7, 9], [215, 62, 42], dk_ * 1.15 - 0.05)
    kc = kc + (np.array([255, 205, 165.]) - kc) * (sk_ ** 30 * 0.35)[..., None]
    kedge = (m["shins"] > 0.5) & (blur(m["shins"], u * 0.35) < 0.72)
    kc = np.where(kedge[..., None], np.array([28, 4, 6.]), kc)
    col = np.where((m["shins"] > 0.5)[..., None], kc, col)
    Hs = blur(m["spikes"], u * 0.4)
    ds_, ss_ = shade(Hs, u * 4.0)
    col = np.where((m["spikes"] > 0.5)[..., None], lin([90, 50, 15], [255, 200, 100], ds_), col)
    # ---- the head: a ridge for the brow, the dark mouth, teeth, a glowing eye, a nostril
    Hh = blur(m["head"], u * 1.1) + blur(m["brow"], u * 0.35) * 0.4
    dh_, sh_ = shade(Hh, u * 9.0)
    hc = lin([50, 6, 9], [200, 55, 38], dh_ * 1.1 - 0.05)
    hc = hc + (np.array([255, 215, 175.]) - hc) * (sh_ ** 28 * 0.4)[..., None]
    col = np.where((m["head"] > 0.5)[..., None], hc, col)
    col = np.where((m["brow"] > 0.5)[..., None], col * 0.72, col)
    col = np.where((m["mouth"] > 0.5)[..., None], np.array([60, 10, 5.]), col)
    col = np.where((m["teeth"] > 0.5)[..., None], np.array([215, 170, 80.]), col)
    col = np.where((m["eye"] > 0.5)[..., None], lin([255, 140, 0], [255, 250, 140], 1 - np.abs(ys / u - 7.8) / 0.8), col)
    col = np.where((m["nostril"] > 0.5)[..., None], np.array([35, 3, 5.]), col)
    # ---- the horns: gold
    Hn = blur(m["horns"], u * 0.5)
    dn_, sn_ = shade(Hn, u * 5.0)
    hn = lin([110, 70, 15], [255, 215, 110], dn_)
    hn = hn + (255 - hn) * (sn_ ** 18 * 0.8)[..., None]
    col = np.where((m["horns"] > 0.5)[..., None], hn, col)
    # ---- the fire: white-yellow in the middle, orange, red at the edges, flickering
    fx, fy = 9.5 * u, 26.0 * u
    dist = np.hypot(xs - fx, ys - fy) / (u * 22.0)
    axis = np.array([-0.55, 0.83])
    along = ((xs - fx) * axis[0] + (ys - fy) * axis[1]) / (u * 22.0)
    across = np.abs(-(xs - fx) * axis[1] + (ys - fy) * axis[0]) / (u * 22.0)
    core = np.clip(1 - across / np.maximum(0.06 + along * 0.32, 1e-3), 0, 1)
    flick = 0.85 + 0.15 * np.sin(along * 40 + across * 25)
    firec = lin([200, 30, 0], [255, 150, 20], core * flick)
    firec = firec + (np.array([255, 250, 200.]) - firec) * (np.clip(core * 1.6 - 0.8, 0, 1) * (1 - along))[..., None]
    fire_m = (m["fire"] > 0.5) & ~((m["head"] > 0.5))
    col = np.where(fire_m[..., None], firec, col)
    alpha = np.maximum.reduce([m["wing"], m["body"], m["limbs"], m["shins"], m["head"], m["horns"], m["spikes"], fire_m.astype(float)])

    def down(arr):
        return arr.reshape(H, SS, W, SS, *arr.shape[2:]).mean(axis=(1, 3))
    cov = down((alpha > 0.5).astype(float))
    premul = down(col * (alpha > 0.5)[..., None])
    rgb = np.where(cov[..., None] > 0, premul / np.maximum(cov[..., None], 1e-6), BG)
    rgb = rgb * cov[..., None] + BG * (1 - cov[..., None])
    return np.clip(rgb, 0, 255).astype(np.uint8), cov >= 0.5


def save(W, H, name, outdir):
    rgb, mask = render(W, H)
    Image.fromarray(rgb, "RGB").save(os.path.join(outdir, name + ".bmp"), "BMP")
    mm = np.where(mask, 0, 255).astype(np.uint8)
    Image.fromarray(np.stack([mm, mm, mm], -1), "RGB").save(os.path.join(outdir, name + "_mask.bmp"), "BMP")


def preview(outdir):
    tiles = []
    for (W, H, k) in [(64, 50, 5), (25, 20, 12)]:
        rgb, mask = render(W, H)
        for bg in [(235, 235, 235), (30, 40, 60)]:
            b = rgb.copy(); b[~mask] = bg
            tiles.append(Image.fromarray(b).resize((W * k, H * k), Image.NEAREST))
    out = Image.new("RGB", (650, 510), (255, 0, 255))
    out.paste(tiles[0], (0, 0)); out.paste(tiles[1], (330, 0)); out.paste(tiles[2], (10, 260)); out.paste(tiles[3], (340, 260))
    out.save(os.path.join(outdir, "preview.png"))


if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))
    for (W, H, n) in [(64, 50, "icon64"), (25, 20, "icon25")]:
        save(W, H, n, out)
    preview(out)
    print("written to", out)
