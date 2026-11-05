// pictures from anywhere (the host renders as the emulator does):
//   shot out.ppm door N [yaw+ pitch view]        - through door N (as the player comes out of it), turned by yaw+
//   shot out.ppm at CELL X Z [yaw pitch view]     - X, Z Skyrim units of the cell's own frame (exteriors: Riverwood's)
//   shot out.ppm sky X Y [yaw pitch view]         - Skyrim world units (Tamriel), feet on the ground
//   shot out.ppm cam X Y Z yaw pitch [view]       - the eye at Skyrim world X, Y, Z (Tamriel), free
// yaw 0..1023 (0 north), pitch -256..256
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
const char* GameEntName(const Ent& e);
#undef memcpy
#undef memset
static u16 out[640 * 200];
int main(int argc, char** argv)
    {
    if (argc < 4) { printf("usage: see the top of host/shot.cpp\n"); return 1; }
    int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64);
    u8* m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
    emu_init(m, 0);
    Game* g = (Game*)m;
    emu_mode(m, 0);
    g->iUi = UI_PLAY;
    int a = 3, yaw = 0, pitch = 0, view = 0;
    if (!strcmp(argv[2], "door"))
        {
        const DoorLink& d = g->Door(atoi(argv[3]));
        // come out of it: the outside end
        g->EnterCell(d.ocell, d.ox + ((r3d_sin(d.oyaw) * METER) >> 14), d.oz + ((r3d_cos(d.oyaw) * METER) >> 14), d.oyaw);
        if (getenv("ENTER")) { if (getenv("HOUR")) g->iS.timeOfDay = atoi(getenv("HOUR")) * 60000; g->UseDoor(atoi(argv[3])); }   // through it, inside
        a = 4;
        if (argc > a) yaw = atoi(argv[a]);
        g->iS.yaw = (g->iS.yaw + yaw) & 1023;
        }
    else if (!strcmp(argv[2], "sky"))
        {
        double X = atof(argv[3]), Y = atof(argv[4]);
        int ex = (int)((X + 233472) * 0.0142875 * METER), ez = (int)((Y + 176128) * 0.0142875 * METER);
        g->EnterCell(0, ex, ez, 0);
        if (getenv("UP")) g->iS.py = g->FloorAt(0, ex, ez, g->iR->GroundAt(ex, ez) + atoi(getenv("UP")) * METER) + 410;   // (on what is built, up to UP m over the land)
        a = 5;
        if (argc > a) g->iS.yaw = atoi(argv[a]) & 1023;
        }
    else if (!strcmp(argv[2], "in"))
        {
        // shot out.ppm in CELL X Y FLOOR yaw pitch [view]: inside a cell, Skyrim units of its own frame, feet on the
        // floor nearest FLOOR (units)
        int cell = atoi(argv[3]); int x = atoi(argv[4]), y = atoi(argv[5]), f0 = atoi(argv[6]);
        g->EnterCell(cell, 100 * METER + x * METER / 70, 100 * METER + y * METER / 70, 0);
        g->iS.py = g->FloorAt(cell, g->iS.px, g->iS.pz, 100 * METER + f0 * METER / 70) + 410;
        a = 7; g->iS.yaw = atoi(argv[a]) & 1023;
        }
    int cam = 0, cx = 0, cy = 0, cz = 0;
    if (!strcmp(argv[2], "cam"))
        {
        double X = atof(argv[3]), Y = atof(argv[4]), Z = atof(argv[5]);
        cx = (int)((X + 233472) * 0.0142875 * METER); cz = (int)((Y + 176128) * 0.0142875 * METER);
        cy = (int)((Z * 0.0142875 + 206) * METER);
        g->EnterCell(0, cx, cz, 0); cam = 1;
        a = 6; g->iS.yaw = atoi(argv[a]) & 1023;
        }
    a++;
    if (argc > a) { pitch = atoi(argv[a]); g->iS.pitch = pitch; }
    a++;
    if (argc > a) { view = atoi(argv[a]); emu_view(m, view); }
    g->iFadeUs = 0;
    for (int s = 0; s < 20; s++) { emu_tick(m, 33000, 0); }
    if (pitch) g->iS.pitch = pitch;
    if (getenv("FACEANIM"))
        {
        // look at the nearest of the land's animals, from 6 m
        int best = -1, bd = 1 << 30;
        for (int i = 0; i < g->iS.ne; i++) { Ent& e = g->iS.e[i]; if (e.type < E_DEER && e.type != E_WOLF) continue; if (!(e.pad1 | e.pad2)) continue; int d = abs(e.x - g->iS.px) + abs(e.z - g->iS.pz); if (d < bd) { bd = d; best = i; } }
        if (best >= 0)
            {
            Ent& e = g->iS.e[best];
            int a = atoi(getenv("FACEANIM")) & 1023;
            g->iS.px = e.x - ((r3d_sin(a) * 6 * METER) >> 14); g->iS.pz = e.z - ((r3d_cos(a) * 6 * METER) >> 14);
            g->iS.py = g->iR->GroundAt(g->iS.px, g->iS.pz) + 410; g->iS.yaw = a; g->iS.pitch = -60;
            e.state = S_IDLE; e.anim = 3 * 64;
            printf("animal %s at %d m\n", GameEntName(e), bd / METER);
            }
        }
    g->iUi = UI_PLAY;      // (no greeting over the picture)
    if (getenv("NOPVS")) for (int k = 0; k < 32; k++) g->iKitObj0[k] = -1;
    if (getenv("SKIP")) g->iR->iSkip = atoi(getenv("SKIP"));
    if (!getenv("FACEOBJ")) { emu_tick(m, 33000, 0); g->iUi = UI_PLAY; }
    if (getenv("TICKS")) for (int t = atoi(getenv("TICKS")); t > 0; t--) { emu_tick(m, 50000, 0); g->iUi = UI_PLAY; }
    if (getenv("FACEFLORA"))
        {
        // look at the nearest plant of kind N (FACEFLORA=N) from 4 m, a little from above
        int kd = atoi(getenv("FACEFLORA")), best = -1, bd = 1 << 30, i;
        for (i = 0; i < g->iFloraN; i++)
            {
            if (g->iFloraEnt[i * 6 + 4] != kd) continue;
            int x, z; g->FloraPos(i, x, z);
            int d = abs(x - g->iS.px) + abs(z - g->iS.pz);
            if (d < bd) { bd = d; best = i; }
            }
        if (best >= 0)
            {
            int x, z; g->FloraPos(best, x, z);
            int a = getenv("FACEYAW") ? atoi(getenv("FACEYAW")) & 1023 : 0, dist = (getenv("DIST") ? atoi(getenv("DIST")) : 4) * METER;
            cx = x - ((r3d_sin(a) * dist) >> 14); cz = z - ((r3d_cos(a) * dist) >> 14);
            g->iR->FocusLand(cx, cz);
            cy = g->iR->GroundAt(cx, cz) + 300; g->iS.yaw = a; g->iS.pitch = -60; cam = 1;
            printf("plant %d kind %d at %d m\n", best, kd, bd / METER);
            }
        }
    if (getenv("FACEOBJ"))
        {
        // look at the nearest placed copy of kit model N (FACEOBJ=N), from DIST metres (default 3) along FACEYAW
        int mi = atoi(getenv("FACEOBJ")), best = -1, bd = 1 << 30, i;
        R3D& r = *g->iR;
        for (i = 0; i < r.iObjCount; i++)
            {
            const Obj& o = r.iObj[i];
            if (o.type < ST_CUSTOM || r.iDefs[o.type - ST_CUSTOM].mesh != &g->iMKit[mi]) continue;
            int d = abs(o.x - g->iS.px) + abs(o.z - g->iS.pz);
            if (d < bd) { bd = d; best = i; }
            }
        int inside = 0;
        if (best < 0)
            for (i = 0; i < r.iINum; i++)
                {
                const Obj& o = r.iIObj[i];
                if (o.type < ST_CUSTOM || r.iDefs[o.type - ST_CUSTOM].mesh != &g->iMKit[mi]) continue;
                int d = abs(o.x - g->iS.px) + abs(o.z - g->iS.pz);
                if (d < bd) { bd = d; best = i; inside = 1; }
                }
        if (best >= 0 && getenv("NEARLIST"))
            {
            const Obj& b = inside ? r.iIObj[best] : r.iObj[best];
            for (i = 0; i < r.iObjCount; i++)
                {
                const Obj& o = r.iObj[i];
                if (o.type < ST_CUSTOM || abs(o.x - b.x) > 8 * METER || abs(o.z - b.z) > 8 * METER) continue;
                const StaticDef& d = r.iDefs[o.type - ST_CUSTOM];
                int k = (int)(d.mesh - g->iMKit);
                printf("near: kit %d at %+d %+d %+d yaw %d scale %d lod %d mid %d\n", k, (o.x - b.x) * 100 / METER, (o.y - b.y) * 100 / METER, (o.z - b.z) * 100 / METER, o.yaw, o.scale, d.lod ? (int)(d.lod - g->iMKit) : -1, d.mid ? (int)(d.mid - g->iMKit) : -1);
                }
            }
        if (best >= 0)
            {
            const Obj& o = inside ? r.iIObj[best] : r.iObj[best];
            int a = getenv("FACEYAW") ? atoi(getenv("FACEYAW")) & 1023 : 0, dist = (getenv("DIST") ? atoi(getenv("DIST")) : 3) * METER;
            const Mesh& m = g->iMKit[mi];
            g->iS.px = o.x - ((r3d_sin(a) * dist) >> 14); g->iS.pz = o.z - ((r3d_cos(a) * dist) >> 14);
            g->iS.py = o.y + r.iDefs[o.type - ST_CUSTOM].yOff + (getenv("FACEY") ? atoi(getenv("FACEY")) * METER / 70 : m.cy); g->iS.yaw = a; g->iS.pitch = getenv("FACEPITCH") ? atoi(getenv("FACEPITCH")) : 0; cam = 1;
            cx = g->iS.px; cz = g->iS.pz; cy = g->iS.py; g->iR->FocusLand(cx, cz);
            printf("object %d at %d m\n", mi, bd / METER);
            }
        else printf("no object %d\n", mi);
        }
    if (getenv("DUMPI"))
        {
        for (int i = 0; i < g->iS.ne; i++) { const Ent& e = g->iS.e[i]; if (e.cell == g->iS.cell && e.alive && e.type == E_VILLAGER) printf("ent %d role %d pad1 %d at %+d %+d (yaw %d)\n", i, e.role, e.pad1, (e.x - g->iS.px) * 100 / METER, (e.z - g->iS.pz) * 100 / METER, g->iS.yaw); }
        R3D& r = *g->iR; int kit = 0, near = 0, mnx = 1 << 30, mxx = -(1 << 30), mny = 1 << 30, mxy = -(1 << 30);
        for (int i = 0; i < r.iINum; i++) { const Obj& o = r.iIObj[i]; if (o.type >= ST_CUSTOM) kit++; if (abs(o.x - g->iS.px) < 20 * METER && abs(o.z - g->iS.pz) < 20 * METER) near++;
            if (o.x < mnx) mnx = o.x; if (o.x > mxx) mxx = o.x; if (o.y < mny) mny = o.y; if (o.y > mxy) mxy = o.y; }
        { int cnt[256] = {0}; for (int i = 0; i < r.iINum; i++) cnt[r.iIObj[i].flag]++; for (int k = 0; k < 256; k++) if (cnt[k]) printf("  cell %d: %d\n", k, cnt[k]); }
        printf("interior objects %d (kit %d), within 20 m %d, x %d..%d y %d..%d player %d %d %d\n", r.iINum, kit, near, mnx, mxx, mny, mxy, g->iS.px, g->iS.py, g->iS.pz);
        }
    if (cam) { g->iS.px = cx; g->iS.pz = cz; g->iS.py = cy; g->iS.vy = 0; }
    emu_render(m, out);
    FILE* f = fopen(argv[1], "wb"); fprintf(f, "P6 640 200 255\n");
    for (int i = 0; i < 640 * 200; i++) { u16 q = out[i]; unsigned char c[3] = { (unsigned char)((q >> 8) & 0xF8), (unsigned char)((q >> 3) & 0xFC), (unsigned char)((q << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
    fclose(f);
    { int hist[16] = {0}; for (int dz = -15; dz <= 15; dz += 3) for (int dx = -15; dx <= 15; dx += 3) hist[g->iR->MaterialAt(g->iS.px + dx * METER, g->iS.pz + dz * METER) & 15]++;
      printf("materials:"); for (int k = 0; k < 16; k++) if (hist[k]) printf(" %d:%d", k, hist[k]); printf("  flora %d ui %d view %d\n", g->iFloraN, g->iUi, g->iS.view); }
    printf("area %d skyN %d hiN %d kitmeshes %d kitcells %d\n", g->iAreaCur, g->iR->iSkyN, g->iR->iSkyHiN, g->iKitMeshN, g->iKitCellN);
    printf("cell %d at %d %d feet %d yaw %d (skyrim %.0f %.0f)\n", g->iS.cell, g->iS.px, g->iS.pz, g->iS.py - 410, g->iS.yaw,
           g->iS.px / (0.0142875 * METER) - 233472, g->iS.pz / (0.0142875 * METER) - 176128);
    return 0;
    }
