#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// walk a straight line in a cell with the game's own movement (cell, x0, y0, x1, y1 in Skyrim units): where it stops
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 int cell = atoi(argv[1]); int x0 = atoi(argv[2]), y0 = atoi(argv[3]), x1 = atoi(argv[4]), y1 = atoi(argv[5]), f0 = argc > 6 ? atoi(argv[6]) : 0, dt = argc > 7 ? atoi(argv[7]) : 16000;
 g->iS.bfb[BF_LUCAN] = 1; g->EnterCell(cell, 100*METER + x0*METER/70, 100*METER + y0*METER/70, 0); g->iFadeUs = 0; g->iUi = UI_PLAY;
 g->iS.py = g->FloorAt(cell, g->iS.px, g->iS.pz, 100*METER + f0*METER/70) + 410;
 int tx = 100*METER + x1*METER/70, tz = 100*METER + y1*METER/70, last = -1, still = 0;
 for (int t = 0; t < 3000; t++)
  {
  int dx = tx - g->iS.px, dz = tz - g->iS.pz;
  if (abs(dx) + abs(dz) < METER / 5) { printf("arrived, feet %d u\n", (g->iS.py - 410 - 100*METER)*70/METER); return 0; }
  g->iS.yaw = r3d_atan2(dx, dz); int px = g->iS.px, pz = g->iS.pz; g->Tick(dt, KEY_FWD);
  if (t % 20 == 0) printf("  (%d,%d) feet %d u\n", (g->iS.px-100*METER)*70/METER, (g->iS.pz-100*METER)*70/METER, (g->iS.py - 410 - 100*METER)*70/METER);
  if (px == g->iS.px && pz == g->iS.pz) { if (++still > 30) { printf("STUCK at (%d,%d) feet %d u\n", (px-100*METER)*70/METER, (pz-100*METER)*70/METER, (g->iS.py - 410 - 100*METER)*70/METER); return 1; } } else still = 0;
  }
 printf("timeout\n"); return 1;
}
