#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// Bleak Falls Barrow outside: out of its door onto the terrace, walk the given Skyrim points in turn; where it stops
static int WorldX(double aX) { return (int)((aX + 233472) * 3.6576); }
static int WorldZ(double aY) { return (int)((aY + 176128) * 3.6576); }
static double SkyZ(Game* g, int y) { return (y - g->iR->iLandOffset) / 256.0 * 100.0 / 1.42875; }
int main(int argc, char** argv)
 { int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 int d = -1; for (int i = 0; i < g->NumDoors(); i++) if (g->Door(i).cell == CELL_BFTEMPLE && g->Door(i).ocell == 0) d = i;
 g->EnterCell(CELL_BFTEMPLE, g->Door(d).ix, g->Door(d).iz, 0); g->iFadeUs = 0;
 g->UseDoor(d); g->iFadeUs = 0;
 printf("out at sky (%.0f, %.0f) feet z %.0f (Skyrim's: 6141 by the door)\n", g->iS.px / 3.6576 - 233472, g->iS.pz / 3.6576 - 176128, SkyZ(g, g->iS.py - 410));
 int fails = 0;
 for (int a = 1; a + 1 < argc; a += 2)
  {
  int tx = WorldX(atof(argv[a])), tz = WorldZ(atof(argv[a + 1])), still = 0, t;
  for (t = 0; t < 4000; t++)
   {
   int dx = tx - g->iS.px, dz = tz - g->iS.pz;
   if (abs(dx) + abs(dz) < METER / 2) break;
   g->iS.yaw = r3d_atan2(dx, dz); int px = g->iS.px, pz = g->iS.pz; g->Tick(33000, KEY_FWD);
   if (px == g->iS.px && pz == g->iS.pz) { if (++still > 30) break; } else still = 0;
   }
  int ok = abs(tx - g->iS.px) + abs(tz - g->iS.pz) < METER / 2;
  if (!ok) fails++;
  printf("%s %s %s: at (%.0f, %.0f) feet z %.0f, ground z %.0f\n", ok ? "reached" : "STOPPED", argv[a], argv[a + 1], g->iS.px / 3.6576 - 233472, g->iS.pz / 3.6576 - 176128, SkyZ(g, g->iS.py - 410), SkyZ(g, g->iR->GroundAt(g->iS.px, g->iS.pz)));
  }
 printf("FAILS %d\n", fails); return fails; }
