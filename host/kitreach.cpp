#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// where in each house you can walk to from its door: a flood over a 10-unit grid with the game's own floor and wall
// tests (a step up as the floor test allows, any step down); the floor levels reached and their area
static Game* g;
enum { N = 320, H = N / 2 };
static short feetAt[N][N][4];   // up to four levels per spot
int main(int argc, char** argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame();
 static const int CELLS[] = { CELL_RWALVOR, CELL_RWGERDUR, CELL_RWFAENDAL, CELL_RWSVEN, CELL_RWTRADER, CELL_RWINN };
 FILE* dump = fopen("shots/reach.txt", "w");
 const int R = 30 * METER / 100;
 for (int h = 0; h < 6; h++)
  {
  int cell = CELLS[h], d = -1; for (int i = 0; i < g->NumDoors(); i++) if (g->Door(i).cell == cell) d = i;
  const DoorLink& a = g->Door(d);
  g->EnterCell(cell, a.ix + ((r3d_sin(a.iyaw) * METER) >> 14), a.iz + ((r3d_cos(a.iyaw) * METER) >> 14), a.iyaw);
  memset(feetAt, 0x7f, sizeof(feetAt));
  static int qx[N * N * 4], qz[N * N * 4], qf[N * N * 4]; int qh = 0, qt = 0;
  int sx = ((g->iS.px - 100 * METER) * 70 / METER) / 10 + H, sz = ((g->iS.pz - 100 * METER) * 70 / METER) / 10 + H;
  qx[qt] = sx; qz[qt] = sz; qf[qt] = g->iS.py - 410; qt++; feetAt[sx][sz][0] = (g->iS.py - 410 - 100 * METER) * 70 / METER;
  int levels[64] = {0};
  while (qh < qt)
   {
   int x = qx[qh], z = qz[qh], f = qf[qh]; qh++;
   int lv = ((f - 100 * METER) * 70 / METER + 40) / 80; if (lv >= 0 && lv < 64) levels[lv]++;
   static const int DX[] = { 1, -1, 0, 0 }, DZ[] = { 0, 0, 1, -1 };
   for (int k = 0; k < 4; k++)
    {
    int nx = x + DX[k], nz = z + DZ[k]; if (nx < 0 || nz < 0 || nx >= N || nz >= N) continue;
    int ex = 100 * METER + (nx - H) * 10 * METER / 70, ez = 100 * METER + (nz - H) * 10 * METER / 70;
    int nf = g->FloorAt(cell, ex, ez, f);
    if (nf > f + 28 * METER / 100) continue;
    if (g->BlockedAt(cell, ex, ez, R, nf)) continue;
    int fu = (nf - 100 * METER) * 70 / METER, s, free = -1;
    for (s = 0; s < 4; s++) { if (feetAt[nx][nz][s] == 0x7f7f) { free = s; break; } if (abs(feetAt[nx][nz][s] - fu) < 12) break; }
    if (s < 4 && free < 0) continue;
    if (free < 0) continue;
    feetAt[nx][nz][free] = (short)fu; fprintf(dump, "%d %d %d %d\n", cell, (nx - H) * 10, (nz - H) * 10, fu); qx[qt] = nx; qz[qt] = nz; qf[qt] = nf; qt++;
    }
   }
  { R3D& R = *g->iR; for (int i = 0; i < R.iINum; i++) { const Obj& o = R.iIObj[i]; if (o.flag != cell || o.type < ST_CUSTOM) continue; const StaticDef& d = R.iDefs[o.type - ST_CUSTOM]; if (!d.hx) continue;
     fprintf(dump, "B %d %d %d %d %d %d %d\n", cell, (o.x - 100*METER)*70/METER, (o.z - 100*METER)*70/METER, (d.hx*o.scale>>8)*70/METER, (d.hz*o.scale>>8)*70/METER, o.yaw, (o.y-100*METER)*70/METER); }
    for (int i = 0; i < g->iS.nprop; i++) { const Prop& p = g->iS.prop[i]; if (p.cell == cell) fprintf(dump, "P %d %d %d %d\n", cell, (p.x - 100*METER)*70/METER, (p.z - 100*METER)*70/METER, p.type); } }
  printf("%-28s reachable %d spots; by level (80-unit bands):", a.name, qt);
  for (int l = -2; l < 12; l++) if (l >= 0 && levels[l]) printf(" %d:%d", l * 80, levels[l]);
  printf("\n");
  }
}
