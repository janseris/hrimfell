#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// (v0.4.74) where the player gets stuck: dropped at many spots, walked 8 ways with the game's own movement; what
// stopped each step that failed. host/stuck fh | wr | fhin
int GameKitWhy(const Game& g, int aCell, int aX, int aZ, int aR, int aFeetY, int aOff, int* aWall);
static Game* g;
static int Rnd(int n) { static unsigned s = 12345; s = s * 1103515245 + 12345; return (s >> 8) % n; }
static int nstuck = 0, nspots = 0, nblock = 0, why[8];
static void Probe(int cell, int x, int z, int verbose)
    {
    if (cell) { g->iS.cell = cell; g->iR->iCell = cell; }
    g->iS.px = x; g->iS.pz = z;
    int f0 = g->FloorAt(cell, x, z, cell ? 100 * METER + 40 * METER : 1000 * METER);
    g->iS.py = f0 + 410; g->iS.vy = 0;
    for (int t = 0; t < 4; t++) { g->iUi = UI_PLAY; g->Tick(50000, 0); }
    int sx = g->iS.px, sz = g->iS.pz, sy = g->iS.py, bad = 0;
    char line[600]; int lp = 0; line[0] = 0;
    for (int d = 0; d < 8; d++)
        {
        g->iS.px = sx; g->iS.pz = sz; g->iS.py = sy; g->iS.vy = 0; g->iS.yaw = d * 128; g->iUi = UI_PLAY;
        for (int t = 0; t < 5; t++) g->Tick(66000, KEY_FWD);
        int mv = (abs(g->iS.px - sx) + abs(g->iS.pz - sz)) * 100 / METER;
        if (mv >= 40) continue;
        bad++; nblock++;
        // what is ahead (40 cm)
        int ax = sx + ((r3d_sin(d * 128) * 40 / 100 * METER) >> 14), az = sz + ((r3d_cos(d * 128) * 40 / 100 * METER) >> 14);
        int feet = sy - 410, sh = cell ? 7 * METER / 25 : 45 * METER / 100;
        int g2 = g->FloorAt(cell, ax, az, feet);
        int w[8]; memset(w, 0, sizeof w);
        int prop = g->PropBlocked(cell, ax, az, 100, feet + sh);
        int land = !cell && g->iR->Blocked(ax, az, 100, feet + sh, feet + 7 * METER / 4);
        int kit = GameKitWhy(*g, cell, ax, az, 100, feet + sh, g->iR->iLandLoaded ? g->iR->iLandOffset : 0, w);
        int dun = cell && g->IsDungeon(cell) && g->DunBlocked(cell, ax, az, 100, feet + sh);
        int all = g->BlockedAt(cell, ax, az, 100, feet + sh, 7 * METER / 4 - sh);
        int k = prop ? 0 : land ? 1 : kit == 1 ? 2 : kit == 2 ? 3 : dun ? 4 : all ? 5 : (g2 - feet > 30 * METER / 100) ? 6 : 7;
        why[k]++;
        static const char* KN[8] = { "prop", "static", "kitwall", "navedge", "dungeon", "other", "slope", "?" };
        lp += sprintf(line + lp, " %d:%s", d, KN[k]);
        if (k == 6) lp += sprintf(line + lp, "(+%dcm)", (g2 - feet) * 100 / METER);
        if ((k == 2 || k == 3) && verbose) lp += sprintf(line + lp, "[%d,%d-%d,%d h%d..%d feet %d]", w[0], w[1], w[2], w[3], w[4], w[5], w[7]);
        }
    nspots++;
    if (bad >= 5) nstuck++;
    if (bad && verbose) printf("  spot %d %d (cell %d): %d of 8 blocked:%s\n", x, z, cell, bad, line);
    }
// a random walk from a start: strides of 0.3 s in a random way, a new way when one fails; each spot reached checked for
// a way out (any of 8): none = trapped where walking took the player
static void Walk(int cell, int x, int z, int steps, int verbose)
    {
    if (cell) { g->iS.cell = cell; g->iR->iCell = cell; }
    g->iS.px = x; g->iS.pz = z; g->iS.py = g->FloorAt(cell, x, z, cell ? 100 * METER + 40 * METER : 1000 * METER) + 410; g->iS.vy = 0;
    int trapped = 0, dir = Rnd(8), far = 0;
    for (int s = 0; s < steps; s++)
        {
        int sx = g->iS.px, sz = g->iS.pz, sy = g->iS.py;
        g->iUi = UI_PLAY; g->iS.yaw = dir * 128;
        for (int t = 0; t < 5; t++) g->Tick(60000, KEY_FWD);
        if (g->iS.cell != cell) { g->iS.cell = cell; g->iR->iCell = cell; g->iS.px = sx; g->iS.pz = sz; g->iS.py = sy; dir = Rnd(8); continue; }
        int mv = (abs(g->iS.px - sx) + abs(g->iS.pz - sz)) * 100 / METER;
        int d = (abs(g->iS.px - x) + abs(g->iS.pz - z)) / METER; if (d > far) far = d;
        if (mv < 20)
            {
            // any way out?
            int ok = 0;
            for (int d2 = 0; d2 < 8 && !ok; d2++)
                {
                g->iS.px = sx; g->iS.pz = sz; g->iS.py = sy; g->iS.vy = 0; g->iS.yaw = d2 * 128; g->iUi = UI_PLAY;
                for (int t = 0; t < 4; t++) g->Tick(60000, KEY_FWD);
                if ((abs(g->iS.px - sx) + abs(g->iS.pz - sz)) * 100 / METER >= 25) ok = 1;
                }
            if (!ok)
                {
                trapped++;
                if (verbose && trapped <= 6) Probe(cell, sx, sz, 1);
                // put back at the start
                g->iS.px = x; g->iS.pz = z; g->iS.py = g->FloorAt(cell, x, z, cell ? 100 * METER + 40 * METER : 1000 * METER) + 410;
                }
            dir = Rnd(8);
            }
        else if (Rnd(6) == 0) dir = Rnd(8);
        }
    printf("walk from %d %d (cell %d): %d strides, farthest %d m, trapped %d times\n", x, z, cell, steps, far, trapped);
    }
int main(int argc, char** argv)
    {
    int n = Game::MemNeeded(); unsigned char* mem = (unsigned char*)calloc(1, n); g = (Game*)calloc(1, sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi = UI_PLAY;
    const char* mode = argc > 1 ? argv[1] : "fh";
    g->iS.dun[FH_GREETED] = 1;
    int verbose = argc > 2;
    if (!strcmp(mode, "walkfh") || !strcmp(mode, "walkwr"))
        {
        int cx, cz;
        if (!strcmp(mode, "walkfh")) { const DoorLink& d = g->Door(26); cx = d.ox; cz = d.oz; }
        else { cx = (int)(((long long)19900 + 233472) * 36576 / 10000); cz = (int)(((long long)-12200 + 176128) * 36576 / 10000); }
        g->EnterCell(0, cx, cz, 0);
        for (int t = 0; t < 30; t++) g->Tick(50000, 0);
        for (int k = 0; k < 4; k++) Walk(0, g->iS.px, g->iS.pz, 300, verbose);
        return 0;
        }
    if (!strcmp(mode, "start"))
        {
        // the new game's first spot (Forelhost's door), as the player has it: the dialogue run through first
        g->iS.dun[FH_GREETED] = 0;
        for (int t = 0; t < 40; t++) { g->Tick(50000, 0); while (g->iUi == UI_DIALOG) g->DialogNext(); }
        printf("start: cell %d at %d %d feet %d, ui %d\n", g->iS.cell, g->iS.px, g->iS.pz, g->iS.py - 410, g->iUi);
        Probe(g->iS.cell, g->iS.px, g->iS.pz, 1);
        Walk(g->iS.cell, g->iS.px, g->iS.pz, 200, 1);
        return 0;
        }
    if (!strcmp(mode, "doors"))
        {
        // every door: come through it (UseDoor from the other side), then try to walk away
        for (int d = 0; d < 40; d++)
            {
            const DoorLink& L = g->Door(d);
            if (!L.name || !L.cell) continue;
            for (int side = 0; side < 2; side++)
                {
                // side 0: arrive inside (enter from outside); 1: arrive outside
                if (side == 0) g->EnterCell(L.ocell, L.ox, L.oz, L.oyaw); else g->EnterCell(L.cell, L.ix, L.iz, L.iyaw);
                for (int t = 0; t < 6; t++) { g->iUi = UI_PLAY; g->Tick(50000, 0); }
                g->UseDoor(d);
                for (int t = 0; t < 30; t++) { g->iUi = UI_PLAY; g->iFadeUs = 0; g->Tick(50000, 0); }
                int sx = g->iS.px, sz = g->iS.pz, sy = g->iS.py, ok = 0, cell = g->iS.cell;
                char ways[16]; int nw = 0;
                for (int d2 = 0; d2 < 8; d2++)
                    {
                    g->iS.px = sx; g->iS.pz = sz; g->iS.py = sy; g->iS.vy = 0; g->iS.yaw = d2 * 128; g->iUi = UI_PLAY;
                    for (int t = 0; t < 5; t++) g->Tick(60000, KEY_FWD);
                    int m = (abs(g->iS.px - sx) + abs(g->iS.pz - sz)) * 100 / METER;
                    if (g->iS.cell != cell) m = 999;
                    ways[nw++] = m >= 40 ? '+' : '.';
                    if (m >= 40) ok++;
                    g->iS.cell = cell; g->iR->iCell = cell;
                    }
                ways[nw] = 0;
                printf("door %2d %-28s %s: in cell %2d, ways out %d [%s]%s\n", d, L.name, side ? "out" : "in ", cell, ok, ways, ok <= 2 ? "  <-- STUCK" : "");
                if (ok <= 2) Probe(cell, sx, sz, 1);
                }
            }
        return 0;
        }
    if (!strcmp(mode, "walkfhin"))
        {
        int cells[3] = { CELL_FHSTRONG, CELL_FHCRYPT, CELL_FHREFECT };
        for (int c = 0; c < 3; c++)
            {
            g->EnterCell(cells[c], 100 * METER, 100 * METER, 0);
            for (int t = 0; t < 10; t++) g->Tick(50000, 0);
            Walk(cells[c], g->iS.px, g->iS.pz, 300, verbose);
            }
        return 0;
        }
    if (!strcmp(mode, "fh") || !strcmp(mode, "wr"))
        {
        int cx, cz, r0, r1;
        if (!strcmp(mode, "fh")) { const DoorLink& d = g->Door(26); cx = d.ox; cz = d.oz; r0 = 0; r1 = 60; }
        else { cx = (int)(((long long)24800 + 233472) * 36576 / 10000); cz = (int)(((long long)-4300 + 176128) * 36576 / 10000); r0 = 260; r1 = 600; }
        g->EnterCell(0, cx + (r0 + 5) * METER, cz, 0);
        for (int t = 0; t < 30; t++) g->Tick(50000, 0);
        printf("kit wr %d fh %d\n", g->iWrKit, g->iFhKit);
        for (int s = 0; s < 120; s++)
            {
            int a = Rnd(1024), r = r0 + Rnd(r1 - r0);
            int x = cx + ((r3d_sin(a) * r) >> 14) * METER, z = cz + ((r3d_cos(a) * r) >> 14) * METER;
            if (g->iR->WaterAt(x, z) > g->iR->GroundAt(x, z)) continue;
            Probe(0, x, z, verbose);
            }
        }
    else
        {
        // inside Forelhost: the spots its people and props stand on
        int cells[3] = { CELL_FHSTRONG, CELL_FHCRYPT, CELL_FHREFECT };
        for (int c = 0; c < 3; c++)
            {
            g->EnterCell(cells[c], 100 * METER, 100 * METER, 0);
            for (int t = 0; t < 10; t++) g->Tick(50000, 0);
            for (int i = 0; i < g->iS.nprop; i++) { const Prop& p = g->iS.prop[i]; if (p.cell == cells[c] && Rnd(3) == 0) Probe(cells[c], p.x + METER, p.z, verbose); }
            }
        }
    printf("%s: %d spots, %d stuck (5+ of 8 ways blocked), %d blocked steps: prop %d, static %d, kit wall %d, navmesh edge %d, dungeon %d, other %d, slope %d, ? %d\n",
           mode, nspots, nstuck, nblock, why[0], why[1], why[2], why[3], why[4], why[5], why[6], why[7]);
    return 0;
    }
