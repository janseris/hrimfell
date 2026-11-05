// top-down plan of Vindgard: terrain height shading, collision footprints of statics, props and people
// usage: topmap [x0 z0 x1 z1] (city metres) -> shots/topmap.ppm, 2 px per metre
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static int W, H, X0, Z0, PPM = 2;
static unsigned char* img;
static void put(int px, int pz, int r, int g, int b)
    {
    if (px < 0 || pz < 0 || px >= W || pz >= H) return;
    unsigned char* p = img + (pz * W + px) * 3; p[0] = r; p[1] = g; p[2] = b;
    }
static int cx0, cz0;
// world -> pixel
static void wp(int x, int z, int& px, int& pz)
    {
    px = (int)((long long)(x - cx0) * PPM / METER) - X0 * PPM;
    pz = H - 1 - ((int)((long long)(z - cz0) * PPM / METER) - Z0 * PPM);
    }
static void line(int ax, int az, int bx, int bz, int r, int g, int b)
    {
    int x0, z0, x1, z1; wp(ax, az, x0, z0); wp(bx, bz, x1, z1);
    int n = abs(x1 - x0) > abs(z1 - z0) ? abs(x1 - x0) : abs(z1 - z0);
    if (n == 0) n = 1;
    for (int i = 0; i <= n; i++) put(x0 + (x1 - x0) * i / n, z0 + (z1 - z0) * i / n, r, g, b);
    }
static void box(int x, int z, int yaw, int lx0, int lz0, int lx1, int lz1, int r, int g, int b)
    {
    int cs = r3d_cos(yaw), sn = r3d_sin(yaw);
    int lx[4] = { lx0, lx1, lx1, lx0 }, lz[4] = { lz0, lz0, lz1, lz1 }, wx[4], wz[4];
    for (int k = 0; k < 4; k++)
        {
        wx[k] = x + ((lx[k] * cs + lz[k] * sn) >> 14);
        wz[k] = z + ((-lx[k] * sn + lz[k] * cs) >> 14);
        }
    for (int k = 0; k < 4; k++) line(wx[k], wz[k], wx[(k + 1) & 3], wz[(k + 1) & 3], r, g, b);
    }
int main(int argc, char** argv)
    {
    int x0 = -175, z0 = -125, x1 = 100, z1 = 120;
    if (argc >= 5) { x0 = atoi(argv[1]); z0 = atoi(argv[2]); x1 = atoi(argv[3]); z1 = atoi(argv[4]); }
    if (argc >= 6) PPM = atoi(argv[5]);
    X0 = x0; Z0 = z0; W = (x1 - x0) * PPM; H = (z1 - z0) * PPM;
    img = (unsigned char*)calloc(W * H, 3);
    int n = Game::MemNeeded(); unsigned char* mem = (unsigned char*)calloc(1, n);
    Game* g = (Game*)calloc(1, sizeof(Game)); g->Init(mem);
    R3D& r = *g->iR;
    cx0 = CITY_CX * CELL + CELL / 2; cz0 = CITY_CZ * CELL + CELL / 2;
    int h0 = r.GroundAt(cx0 - 3 * METER, cz0 - 13 * METER);
    for (int pz = 0; pz < H; pz++)
        for (int px = 0; px < W; px++)
            {
            int x = cx0 + (X0 * PPM + px) * METER / PPM, z = cz0 + (Z0 * PPM + (H - 1 - pz)) * METER / PPM;
            int h = r.GroundAt(x, z) - h0;                 // world units
            int v = 90 + h * 8 / METER;                     // 8 grey levels per metre
            if (v < 0) v = 0;
            if (v > 255) v = 255;
            int hx = r.GroundAt(x + METER / 2, z) - r.GroundAt(x - METER / 2, z), hz = r.GroundAt(x, z + METER / 2) - r.GroundAt(x, z - METER / 2);
            int sh = (hz - hx) / 8; v += sh; if (v < 0) v = 0; if (v > 255) v = 255;
            // tiers tinted: lower blue-ish, wind green, terrace yellow, cloud red
            int hm = (h + METER / 2) / METER;
            int R = v, G = v, B = v;
            if (hm == 0) B = v + 20 > 255 ? 255 : v + 20;
            else if (hm == 4) G = v + 25 > 255 ? 255 : v + 25;
            else if (hm == 9) { R = v + 25 > 255 ? 255 : v + 25; G = R; }
            else if (hm == 14) R = v + 30 > 255 ? 255 : v + 30;
            put(px, pz, R, G, B);
            }
    // grid every 10 m
    for (int m = (x0 / 10) * 10; m <= x1; m += 10) for (int pz = 0; pz < H; pz += 3) { int px, q; wp(cx0 + m * METER, cz0, px, q); put(px, pz, m == 0 ? 255 : 60, 60, 60); }
    for (int m = (z0 / 10) * 10; m <= z1; m += 10) for (int px = 0; px < W; px += 3) { int q, pz; wp(cx0, cz0 + m * METER, q, pz); put(px, pz, 60, 60, m == 0 ? 255 : 60); }
    for (int i = 0; i < r.iObjCount; i++)
        {
        const Obj& o = r.iObj[i];
        if (o.type < ST_CUSTOM) { int px, pz; wp(o.x, o.z, px, pz); put(px, pz, 0, 120, 0); continue; }
        const StaticDef& d = r.iDefs[o.type - ST_CUSTOM];
        int s = o.scale;
        if (d.hx) box(o.x, o.z, o.yaw, -d.hx * s / 256, -d.hz * s / 256, d.hx * s / 256, d.hz * s / 256, 255, 40, 40);
        if (d.rad)
            {
            int rr = d.rad * s / 256;
            for (int a = 0; a < 1024; a += 16)
                line(o.x + ((r3d_sin(a) * rr) >> 14), o.z + ((r3d_cos(a) * rr) >> 14), o.x + ((r3d_sin(a + 16) * rr) >> 14), o.z + ((r3d_cos(a + 16) * rr) >> 14), 255, 140, 0);
            }
        for (int k = 0; k < d.nparts; k++)
            {
            const int* p = d.parts + k * 7;
            if (p[0] == PART_BOX) box(o.x, o.z, o.yaw, p[1] * s / 256, p[2] * s / 256, p[3] * s / 256, p[4] * s / 256, 255, 0, 255);
            }
        // front marker: local -z
        int fx = o.x - ((r3d_sin(o.yaw) * METER) >> 14), fz = o.z - ((r3d_cos(o.yaw) * METER) >> 14);
        line(o.x, o.z, fx, fz, 255, 255, 0);
        }
    for (int i = 0; i < g->iS.nprop; i++) { int px, pz; wp(g->iS.prop[i].x, g->iS.prop[i].z, px, pz); if (g->iS.prop[i].cell == 0) put(px, pz, 0, 255, 255); }
    for (int i = 0; i < g->iS.ne; i++)
        {
        const Ent& e = g->iS.e[i];
        if (e.cell) continue;
        int px, pz; wp(e.x, e.z, px, pz);
        for (int a = -2; a <= 2; a++) for (int b = -2; b <= 2; b++) put(px + a, pz + b, 0, 255, 0);
        }
    for (int i = 0; i < g->iNumRamps; i++)
        {
        const Ramp& rp = g->iRamps[i];
        box(rp.x, rp.z0 + rp.len / 2, 0, -rp.hw, -abs(rp.len) / 2, rp.hw, abs(rp.len) / 2, 0, 200, 255);
        }
    { int px, pz; wp(g->iS.px, g->iS.pz, px, pz); for (int a = -4; a <= 4; a++) { put(px + a, pz, 255, 255, 255); put(px, pz + a, 255, 255, 255); } }
    FILE* f = fopen("shots/topmap.ppm", "wb"); fprintf(f, "P6 %d %d 255\n", W, H); fwrite(img, 3, W * H, f); fclose(f);
    printf("h0 %d, objs %d, props %d, ents %d\n", h0, r.iObjCount, g->iS.nprop, g->iS.ne);
    return 0;
    }
