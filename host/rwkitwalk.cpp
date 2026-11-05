#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// the Riverwood Trader and the Sleeping Giant built from Skyrim's own pieces: walked with the game's own movement
// (Skyrim units, x east, y north): in from the street, to the counter, up the log stairs and round the upper floor,
// down and out; the inn's hall, behind the bar, the room for rent through its door, the bed rented, the secret room
// and its passage up to Delphine's wardrobe, out again
static Game* g;
static int fails;
static int UX(int e) { return (e - 100 * METER) * 70 / METER; }
static int FeetU() { return (g->iS.py - 410 - 100 * METER) * 70 / METER; }
static void Path(const char* what, const int* p, int n, int maxTicks)
    {
    int k = 0, t, still = 0;
    for (t = 0; t < maxTicks && k < n; t++)
        {
        int dx = 100 * METER + p[k * 2] * METER / 70 - g->iS.px, dz = 100 * METER + p[k * 2 + 1] * METER / 70 - g->iS.pz;
        if ((dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4) < (METER / 4 >> 4) * (METER / 4 >> 4)) { k++; still = 0; continue; }
        g->iS.yaw = r3d_atan2(dx, dz);
        int x0 = g->iS.px, z0 = g->iS.pz;
        g->Tick(16000, KEY_FWD);
        if (x0 == g->iS.px && z0 == g->iS.pz)
            {
            // snagged on a corner: as a player would, turn a little either way for a moment
            static const int KT[] = { 85, -85, 170, -170 };
            int y0 = r3d_atan2(dx, dz);
            for (int j = 0; j < 4 && x0 == g->iS.px && z0 == g->iS.pz; j++) { g->iS.yaw = (y0 + KT[j]) & 1023; for (int q = 0; q < 3; q++) g->Tick(16000, KEY_FWD); }
            if (x0 == g->iS.px && z0 == g->iS.pz && ++still > 40) break;
            }
        }
    int ok = k >= n;
    if (!ok && k < n) printf("    (heading for (%d,%d); then (%d,%d))\n", p[k * 2], p[k * 2 + 1], k + 1 < n ? p[k * 2 + 2] : 0, k + 1 < n ? p[k * 2 + 3] : 0);
    printf("%-44s reached %d/%d at (%d,%d) feet %d units %s\n", what, k, n, UX(g->iS.px), UX(g->iS.pz), FeetU(), ok ? "OK" : "STUCK");
    if (!ok) fails++;
    }
// a way from where the player stands to (x, y): a flood over a 10-unit grid with the game's own floor and wall tests
// (as host/kitreach), then every third square of it walked with the game's movement
enum { GN = 320, GH = GN / 2 };
static int gPrev[GN * GN], gFeet[GN * GN];
static void Go(const char* what, int tx, int ty)
    {
    int cell = g->iS.cell, R = 30 * METER / 100;
    static int q[GN * GN];
    for (int i = 0; i < GN * GN; i++) gPrev[i] = -2;
    int sx = UX(g->iS.px) / 10 + GH, sy = UX(g->iS.pz) / 10 + GH, s0 = sy * GN + sx, goal = (ty / 10 + GH) * GN + tx / 10 + GH;
    int qh = 0, qt = 0; q[qt++] = s0; gPrev[s0] = -1; gFeet[s0] = g->iS.py - 410;
    while (qh < qt && gPrev[goal] == -2)
        {
        int c = q[qh++], x = c % GN, y = c / GN;
        static const int DX[] = { 1, -1, 0, 0 }, DY[] = { 0, 0, 1, -1 };
        for (int k = 0; k < 4; k++)
            {
            int nx = x + DX[k], ny = y + DY[k]; if (nx < 0 || ny < 0 || nx >= GN || ny >= GN) continue;
            int nc = ny * GN + nx; if (gPrev[nc] != -2) continue;
            int ex = 100 * METER + (nx - GH) * 10 * METER / 70, ez = 100 * METER + (ny - GH) * 10 * METER / 70;
            int nf = g->FloorAt(cell, ex, ez, gFeet[c]);
            if (nf > gFeet[c] + 28 * METER / 100 || g->BlockedAt(cell, ex, ez, 104, nf + 7 * METER / 25, 7 * METER / 4 - 7 * METER / 25)) continue;
            gPrev[nc] = c; gFeet[nc] = nf; q[qt++] = nc;
            }
        }
    if (gPrev[goal] == -2) { printf("%-44s NO WAY from (%d,%d) to (%d,%d)\n", what, UX(g->iS.px), UX(g->iS.pz), tx, ty); fails++; return; }
    static int way[GN * 4]; int n = 0, c = goal, k = 0;
    static int rev[GN * GN]; int m = 0;
    for (; c >= 0; c = gPrev[c]) rev[m++] = c;
    for (int i = m - 1; i >= 0; i--) if (1) { if (n < GN * 2) { way[n * 2] = (rev[i] % GN - GH) * 10; way[n * 2 + 1] = (rev[i] / GN - GH) * 10; n++; } }
    (void)k;
    Path(what, way, n, 20000);
    }

static void In(int aCell)
    {
    int d = -1;
    for (int i = 0; i < g->NumDoors(); i++) if (g->Door(i).cell == aCell && d < 0) d = i;
    const DoorLink& a = g->Door(d);
    g->EnterCell(0, a.ox + ((r3d_sin(a.oyaw) * 2 * METER) >> 14), a.oz + ((r3d_cos(a.oyaw) * 2 * METER) >> 14), (a.oyaw + 512) & 1023); g->iFadeUs = 0;
    for (int k = 0; k < 80 && g->iS.cell == 0; k++) { g->FindDoor(); if (g->iDoorFocus == d) { g->UseDoor(d); break; } g->Tick(16000, KEY_FWD); }
    g->iUi = UI_PLAY; g->iFadeUs = 0;
    printf("%s: in cell %d at (%d,%d) feet %d units\n", a.name, g->iS.cell, UX(g->iS.px), UX(g->iS.pz), FeetU());
    if (g->iS.cell != aCell) fails++;
    }
static void Out(int aCell)
    {
    int d = -1;
    for (int i = 0; i < g->NumDoors(); i++) if (g->Door(i).cell == aCell && d < 0) d = i;
    const DoorLink& a = g->Door(d);
    Go("back to the door", UX(a.ix), UX(a.iz));
    g->FindDoor(); if (g->iDoorFocus >= 0) g->UseDoor(g->iDoorFocus);
    int far = (abs(g->iS.px - a.ox) + abs(g->iS.pz - a.oz)) / METER;
    printf("out: cell %d, %d m from the door %s\n", g->iS.cell, far, g->iS.cell == 0 && far < 5 ? "OK" : "FAIL");
    if (g->iS.cell != 0 || far >= 5) fails++;
    }
static int NearProp(int aCell, int aType)
    {
    int best = -1, bd = 1 << 30;
    for (int i = 0; i < g->iS.nprop; i++) { const Prop& p = g->iS.prop[i]; if (p.cell != aCell || p.type != aType) continue;
        int d = abs(p.x - g->iS.px) + abs(p.z - g->iS.pz); if (d < bd) { bd = d; best = i; } }
    return best;
    }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 g->iS.bfb[BF_LUCAN] = 1;
 // ---- the Trader
 In(CELL_RWTRADER);
 // (Camilla and Lucan stand in the counter's gap, where Skyrim puts her: stepped aside for the walk)
 { int cam = -1; for (int i = 0; i < g->iS.ne; i++) if (g->iS.e[i].role == ROLE_NAMED && g->iS.e[i].pad1 == 18) cam = i;
   int luc = -1; for (int i = 0; i < g->iS.ne; i++) if (g->iS.e[i].role == ROLE_MERCHANT && g->iS.e[i].pad1 == MER_RWTRADER) luc = i;
   int lx = luc >= 0 ? g->iS.e[luc].x : 0; if (luc >= 0) g->iS.e[luc].x += 40 * METER;
   int cx = cam >= 0 ? g->iS.e[cam].x : 0; if (cam >= 0) g->iS.e[cam].x += 40 * METER;
   Go("behind the counter (Lucan's place)", 40, -110);
   if (cam >= 0) g->iS.e[cam].x = cx;
   if (luc >= 0) g->iS.e[luc].x = lx;
   Go("back round the counter", 20, -260); }
 Go("to the foot of the stairs", -509, -380);
 Go("up the stairs", -509, -30);
 printf("  upstairs: feet %d units (want 240)\n", FeetU()); if (FeetU() < 220) fails++;
 Go("across the upper floor (south-east)", -100, -330);
 Go("the upper floor's north side", -100, 40);
 Go("down the stairs", -509, -390);
 printf("  down: feet %d units (want 0)\n", FeetU()); if (FeetU() > 20) fails++;
 Out(CELL_RWTRADER);
 // ---- the Sleeping Giant
 In(CELL_RWINN);
 Go("down the hall to the west end", -1000, 0);
 Go("behind the bar (Orgnar's place)", -1280, 0);
 Go("to the room for rent's door", -768, -330);
 int door = NearProp(CELL_RWINN, PT_DOOR);
 { int x0 = g->iS.px, z0 = g->iS.pz; static const int KX[] = { -768, -560 }; Path("through the shut door? (must stop)", KX, 1, 300); fails--;
   int went = UX(g->iS.pz) < -440; printf("  -> %s\n", went ? "WENT THROUGH (wrong)" : "stopped by it"); if (went) fails++; g->iS.px = x0; g->iS.pz = z0; }
 g->UseProp(door); for (int k = 0; k < 60; k++) g->Tick(16000, 0);
 Go("into the room", -720, -650);
 int bed = -1; for (int i = 0; i < g->iS.nprop; i++) { const Prop& p = g->iS.prop[i]; if (p.cell == CELL_RWINN && p.type == PT_BED && p.count == 1) bed = i; }
 g->iS.gold = 50; g->UseProp(bed); printf("rent: ui %d (wait %d), gold %d\n", g->iUi, UI_WAIT, g->iS.gold); if (g->iUi != UI_WAIT) fails++;
 g->iUi = UI_PLAY; g->iSleeping = 0; g->iSitting = -1;
 printf("  in bed at (%d,%d)\n", UX(g->iS.px), UX(g->iS.pz));
 g->iS.px = 100 * METER + (-768) * METER / 70; g->iS.pz = 100 * METER + (-560) * METER / 70;     // up, by the bed
 if (g->iS.prop[door].state) { g->UseProp(door); for (int k = 0; k < 60; k++) g->Tick(16000, 0); }
 printf("  the room's door after the night: %s\n", g->iS.prop[door].state ? "shut" : "open");
 Go("the hall's east end", 200, -100);
 Out(CELL_RWINN);
 // the secret room and its passage
 g->Teleport(25); g->iUi = UI_PLAY; g->iFadeUs = 0;
 printf("secret room: cell %d feet %d units\n", g->iS.cell, FeetU()); if (FeetU() > -200) fails++;
 Go("up the passage to the wardrobe", -560, 800);
 static const int KW[] = { -720, 800 };                                              Path("through the shut wardrobe? (must stop)", KW, 1, 300); fails--;
 printf("  -> %s\n", UX(g->iS.px) < -650 ? "WENT THROUGH (wrong)" : "stopped by it"); if (UX(g->iS.px) < -650) fails++;
 printf("FAILS %d\n", fails);
}
