#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Riverwood's houses built from Skyrim's pieces: in by the door from the street, out in eight directions (never off the
// floor, never through a wall), back to the door and out again
static Game* g;
int GameKitFloor(const Game& gg, int aCell, int aX, int aZ, int aFeetY, int* aFound);
int GameKitBlocked(const Game& gg, int aCell, int aX, int aZ, int aR, int aFeetY);
static int UX(int e) { return (e - 100 * METER) * 70 / METER; }
static int Feet() { return (g->iS.py - 410 - 100 * METER) * 100 / METER; }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 static const int CELLS[] = { CELL_RWALVOR, CELL_RWGERDUR, CELL_RWFAENDAL, CELL_RWSVEN };
 int fails = 0;
 for (int h = 0; h < 4; h++)
  {
  int d = -1; for (int i = 0; i < g->NumDoors(); i++) if (g->Door(i).cell == CELLS[h]) d = i;
  if (d < 0) { printf("cell %d: NO DOOR\n", CELLS[h]); fails++; continue; }
  const DoorLink& a = g->Door(d);
  g->EnterCell(0, a.ox + ((r3d_sin(a.oyaw) * 2 * METER) >> 14), a.oz + ((r3d_cos(a.oyaw) * 2 * METER) >> 14), (a.oyaw + 512) & 1023); g->iFadeUs = 0;
  for (int k = 0; k < 60 && g->iS.cell == 0; k++) { g->FindDoor(); if (g->iDoorFocus == d) { g->UseDoor(d); break; } g->Tick(16000, KEY_FWD); }
  printf("%-28s in cell %d at (%d,%d) units, feet %d cm\n", a.name, g->iS.cell, UX(g->iS.px), UX(g->iS.pz), Feet());
  if (g->iS.cell != CELLS[h]) { printf("  DID NOT ENTER\n"); fails++; continue; }
  g->iFadeUs = 0; for (int k = 0; k < 5; k++) g->Tick(16000, 0);
  int x0 = g->iS.px, z0 = g->iS.pz, y0 = g->iS.py;
  printf("  blocked here: %d, kit walls %d (r 30cm), door at (%d,%d)\n", g->BlockedAt(g->iS.cell, x0, z0, 30 * METER / 100, y0 - 410), GameKitBlocked(*g, g->iS.cell, x0, z0, 30 * METER / 100, y0 - 410), UX(a.ix), UX(a.iz));
  for (int dir = 0; dir < 8; dir++)
   {
   g->iS.px = x0; g->iS.pz = z0; g->iS.py = y0; g->iS.yaw = dir * 128;
   int off = 0, lo = 1 << 30, hi = -(1 << 30);
   for (int t = 0; t < 250; t++)
    {
    g->Tick(16000, KEY_FWD);
    int f; GameKitFloor(*g, g->iS.cell, g->iS.px, g->iS.pz, g->iS.py - 410, &f); if (!f) off++;
    int ft = Feet(); if (ft < lo) lo = ft; if (ft > hi) hi = ft;
    }
   int dist = (abs(g->iS.px - x0) + abs(g->iS.pz - z0)) * 70 / METER;
   printf("  dir %d: to (%d,%d) %4d units, feet %d..%d cm%s\n", dir, UX(g->iS.px), UX(g->iS.pz), dist, lo, hi, off ? "  OFF THE FLOOR" : "");
   if (off) fails++;
   }
  // back to the door, and out
  g->iS.px = x0; g->iS.pz = z0; g->iS.py = y0;
  for (int t = 0; t < 400; t++)
   {
   int dx = a.ix - g->iS.px, dz = a.iz - g->iS.pz;
   if ((dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4) < (METER / 2 >> 4) * (METER / 2 >> 4)) break;
   g->iS.yaw = r3d_atan2(dx, dz); g->Tick(16000, KEY_FWD);
   }
  g->FindDoor(); if (g->iDoorFocus == d) g->UseDoor(d);
  int far = (abs(g->iS.px - a.ox) + abs(g->iS.pz - a.oz)) / METER;
  printf("  out: cell %d, %d m from the door %s\n", g->iS.cell, far, g->iS.cell == 0 && far < 4 ? "OK" : "FAIL");
  if (g->iS.cell != 0 || far >= 4) fails++;
  }
 printf("FAILS %d\n", fails);
}
