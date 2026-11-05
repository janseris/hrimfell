#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Riverwood outside, built from Skyrim's own models: from the street to each house's door (up the steps onto the
// raised decks and porches, along the boardwalks) with the game's own movement, then in through the door; and up
// onto Alvor's forge deck. The way is found by a flood over a 10-unit grid with the game's own floor and wall tests.
static Game* g;
int GameRwHousePos(int aKind, int* aX, int* aZ);
static int fails;
enum { GN = 1400, GH = GN / 2, STEP = 37 };                  // 10 Skyrim units = 36.6 engine units
static int gPrev[GN * GN], gFeet[GN * GN];
static int ox, oz;                                          // the grid's middle (engine units)
static int Feet() { return g->iS.py - 410; }
static int Path(const int* p, int n, int maxTicks)
    {
    int k = 0, t, still = 0;
    for (t = 0; t < maxTicks && k < n; t++)
        {
        int dx = p[k * 2] - g->iS.px, dz = p[k * 2 + 1] - g->iS.pz;
        if ((dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4) < (METER / 4 >> 4) * (METER / 4 >> 4)) { k++; still = 0; continue; }
        g->iS.yaw = r3d_atan2(dx, dz);
        int x0 = g->iS.px, z0 = g->iS.pz;
        g->Tick(16000, KEY_FWD);
        if (x0 == g->iS.px && z0 == g->iS.pz)
            {
            static const int KT[] = { 85, -85, 170, -170 };
            int y0 = r3d_atan2(dx, dz);
            for (int j = 0; j < 4 && x0 == g->iS.px && z0 == g->iS.pz; j++) { g->iS.yaw = (y0 + KT[j]) & 1023; for (int q = 0; q < 3; q++) g->Tick(16000, KEY_FWD); }
            if (x0 == g->iS.px && z0 == g->iS.pz && ++still > 40) break;
            }
        }
    return k;
    }
// walk from where the player stands to (tx, tz) engine units; reports the highest the feet went over the start
static int Go(const char* what, int tx, int tz)
    {
    ox = g->iS.px; oz = g->iS.pz;
    static int q[GN * GN];
    for (int i = 0; i < GN * GN; i++) gPrev[i] = -2;
    int s0 = GH * GN + GH, gx = (tx - ox) / STEP + GH, gz = (tz - oz) / STEP + GH;
    if (gx < 1 || gz < 1 || gx >= GN - 1 || gz >= GN - 1) { printf("%-40s too far\n", what); fails++; return 0; }
    int goal = gz * GN + gx, qh = 0, qt = 0;
    q[qt++] = s0; gPrev[s0] = -1; gFeet[s0] = Feet();
    while (qh < qt && gPrev[goal] == -2)
        {
        int c = q[qh++], x = c % GN, z = c / GN;
        static const int DX[] = { 1, -1, 0, 0 }, DZ[] = { 0, 0, 1, -1 };
        for (int k = 0; k < 4; k++)
            {
            int nx = x + DX[k], nz = z + DZ[k]; if (nx < 0 || nz < 0 || nx >= GN || nz >= GN) continue;
            int nc = nz * GN + nx; if (gPrev[nc] != -2) continue;
            int ex = ox + (nx - GH) * STEP, ez = oz + (nz - GH) * STEP;
            int nf = g->FloorAt(0, ex, ez, gFeet[c]);
            if (nf > gFeet[c] + 44 * METER / 100 || g->BlockedAt(0, ex, ez, 104, nf + 7 * METER / 25, 7 * METER / 4 - 7 * METER / 25)) continue;
            gPrev[nc] = c; gFeet[nc] = nf; q[qt++] = nc;
            }
        }
    if (gPrev[goal] == -2)
        {
        printf("%-40s NO WAY\n", what); fails++;
        FILE* f = fopen("shots/outreach.txt", "w");
        fprintf(f, "G %d %d\n", (int)((long long)tx * 10000 / 36576 - 233472 - 21996), (int)((long long)tz * 10000 / 36576 - 176128 + 45244));
        for (int i = 0; i < GN * GN; i++) if (gPrev[i] != -2)
            {
            int ex = ox + (i % GN - GH) * STEP, ez = oz + (i / GN - GH) * STEP;
            if (abs(ex - tx) > 20 * METER || abs(ez - tz) > 20 * METER) continue;
            fprintf(f, "%d %d %d\n", (int)((long long)ex * 10000 / 36576 - 233472 - 21996), (int)((long long)ez * 10000 / 36576 - 176128 + 45244), (gFeet[i] - 52736) * 70 / METER);
            }
        fclose(f);
        return 0;
        }
    static int rev[GN * GN], way[GN * 8]; int m = 0, n = 0;
    for (int c = goal; c >= 0; c = gPrev[c]) rev[m++] = c;
    for (int i = m - 1; i >= 0 && n < GN * 4; i--) { way[n * 2] = ox + (rev[i] % GN - GH) * STEP; way[n * 2 + 1] = oz + (rev[i] / GN - GH) * STEP; n++; }
    { FILE* pf = fopen("shots/route.txt", "a"); fprintf(pf, "R %s\n", what);
      for (int i = 0; i < n; i++) fprintf(pf, "%d %d\n", (int)((long long)way[i * 2] * 10000 / 36576 - 233472 - 21996), (int)((long long)way[i * 2 + 1] * 10000 / 36576 - 176128 + 45244));
      fclose(pf); }
    int f0 = Feet(), hi = f0;
    int k = 0, t;
    // walk it, watching the feet
    for (t = 0; t < 400 && k < n; t++) { int kk = Path(way + k * 2, n - k > 25 ? 25 : n - k, 4000); k += kk; if (Feet() > hi) hi = Feet(); if (kk == 0) break; }
    int ok = k >= n;
    if (!ok) printf("    ui %d sit %d vy %d; stuck at (%d,%d) heading (%d,%d), land %d feet %d cm\n", g->iUi, g->iSitting, g->iS.vy, (int)((long long)g->iS.px * 10000 / 36576 - 233472 - 21996), (int)((long long)g->iS.pz * 10000 / 36576 - 176128 + 45244),
                    (int)((long long)way[k * 2] * 10000 / 36576 - 233472 - 21996), (int)((long long)way[k * 2 + 1] * 10000 / 36576 - 176128 + 45244), (g->iR->GroundAt(g->iS.px, g->iS.pz) - 52736) * 100 / METER, (Feet() - 52736) * 100 / METER);
    if (!ok) { int fe = Feet(), gr0 = g->FloorAt(0, g->iS.px, g->iS.pz, fe);
        for (int d = 10; d <= 60; d += 10) { int nx = g->iS.px + d, nz = g->iS.pz; int g2 = g->FloorAt(0, nx, nz, fe);
          printf("    +%d: ground %d g2 %d (rise %d cm) blocked %d kit %d statics %d props %d at (%d,%d)\n", d, (gr0 - 52736) * 100 / METER, (g2 - 52736) * 100 / METER, (g2 - gr0) * 100 / METER, g->BlockedAt(0, nx, nz, 100, fe + 7 * METER / 25, 7 * METER / 4 - 7 * METER / 25),
          GameKitOutBlocked(*g, nx, nz, 100, fe + 7 * METER / 25, 52736), g->iR->Blocked(nx, nz, 100, fe + 7 * METER / 25, fe + 7 * METER / 4), g->PropBlocked(0, nx, nz, 100, fe + 7 * METER / 25), nx, nz); } }
    if (!ok) for (int i = 0; i < g->iS.ne; i++) { const Ent& e = g->iS.e[i]; if (e.cell == 0 && abs(e.x - g->iS.px) < 4 * METER && abs(e.z - g->iS.pz) < 4 * METER) printf("    near: ent %d type %d role %d pad1 %d alive %d at %d,%d cm off\n", i, e.type, e.role, e.pad1, e.alive, (e.x - g->iS.px) * 100 / METER, (e.z - g->iS.pz) * 100 / METER); }
    printf("%-40s %s, %d/%d, feet rose %d cm on the way, ends %d cm over the start\n", what, ok ? "OK" : "STUCK", k, n,
           (hi - f0) * 100 / METER, (Feet() - f0) * 100 / METER);
    if (!ok) fails++;
    return ok;
    }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 g->iS.bfb[BF_LUCAN] = 1;
 static const int KD[] = { CELL_RWTRADER, CELL_RWINN, CELL_RWALVOR, CELL_RWGERDUR, CELL_RWFAENDAL, CELL_RWSVEN };
 for (int h = 0; h < 6; h++)
  {
  int d = -1; for (int i = 0; i < g->NumDoors(); i++) if (g->Door(i).cell == KD[h] && d < 0) d = i;
  const DoorLink& a = g->Door(d);
  // from the middle of the village street, before the Trader (12 m off its door)
  const DoorLink& tr = g->Door(11);
  int x = tr.ox + ((r3d_sin(tr.oyaw) * 12 * METER) >> 14), z = tr.oz + ((r3d_cos(tr.oyaw) * 12 * METER) >> 14);
  g->EnterCell(0, x, z, (a.oyaw + 512) & 1023); g->iFadeUs = 0; g->iUi = UI_PLAY;
  g->iS.py = g->FloorAt(0, x, z, g->iR->GroundAt(x, z)) + 410; g->iS.vy = 0;
  for (int k = 0; k < 10; k++) g->Tick(16000, 0);
  char what[80]; sprintf(what, "street -> %s", a.name);
  Go(what, a.ox + ((r3d_sin(a.oyaw) * METER) >> 14), a.oz + ((r3d_cos(a.oyaw) * METER) >> 14));   // a step before the door
  g->iS.yaw = (a.oyaw + 512) & 1023; g->FindDoor();
  int in = 0; if (g->iDoorFocus == d) { g->UseDoor(d); in = g->iS.cell == KD[h]; }
  printf("  its door: focus %d (want %d) %s\n", g->iDoorFocus, d, in ? "in OK" : "NOT IN");
  if (!in) fails++;
  g->iUi = UI_PLAY;
  if (in) { g->UseDoor(d); g->iUi = UI_PLAY; g->iFadeUs = 0;
            printf("  out again: feet %d cm over the land\n", (Feet() - g->iR->GroundAt(g->iS.px, g->iS.pz)) * 100 / METER); }
  // Alvor's: on to his forge on the raised deck
  if (KD[h] == CELL_RWALVOR)
   {
   int fx = 0, fz = 0;
   for (int i = 0; i < g->iS.ne; i++) if (g->iS.e[i].role == ROLE_NAMED && g->iS.e[i].pad1 == 11) { fx = g->iS.e[i].x; fz = g->iS.e[i].z; g->iS.e[i].x += 50 * METER; }
   int f0 = Feet(), gr = g->iR->GroundAt(g->iS.px, g->iS.pz);
   Go("Alvor's door -> his forge", fx, fz);
   printf("  at the forge: feet %d cm over the land there (the deck)\n", (Feet() - g->iR->GroundAt(g->iS.px, g->iS.pz)) * 100 / METER);
   (void)f0; (void)gr;
   }
  }
 // over the bridge, straight along it, both ways (from the road 22 m off its middle to the other end)
 { int bx, bz, y = GameRwHousePos(9, &bx, &bz) & 1023, off = g->iR->iLandOffset;
   for (int side = 0; side < 2; side++)
    {
    int sg = side ? -1 : 1;
    int ax = bx + sg * ((r3d_sin(y) * 22 * METER) >> 14), az = bz + sg * ((r3d_cos(y) * 22 * METER) >> 14);
    int tx = bx - sg * ((r3d_sin(y) * 22 * METER) >> 14), tz = bz - sg * ((r3d_cos(y) * 22 * METER) >> 14);
    g->EnterCell(0, ax, az, 0); g->iFadeUs = 0; g->iUi = UI_PLAY; g->iS.py = g->FloorAt(0, ax, az, g->iR->GroundAt(ax, az) + METER) + 410;
    int hi = -1 << 30, still = 0;
    for (int t = 0; t < 3000; t++)
     {
     int dx = tx - g->iS.px, dz = tz - g->iS.pz; if (abs(dx) + abs(dz) < METER / 2) break;
     g->iS.yaw = r3d_atan2(dx, dz); int px = g->iS.px, pz = g->iS.pz; g->Tick(16000, KEY_FWD);
     if (g->iS.py > hi) hi = g->iS.py;
     if (px == g->iS.px && pz == g->iS.pz) { if (++still > 20) break; } else still = 0;
     }
    int left = (abs(tx - g->iS.px) + abs(tz - g->iS.pz)) / METER;
    printf("over the bridge (%s)                     %s, deck reached %d cm over the land offset\n", side ? "back" : "there", left == 0 ? "OK" : "STUCK", (hi - 410 - off) * 100 / METER);
    if (left) fails++;
    }
 }
 printf("FAILS %d\n", fails);
}
