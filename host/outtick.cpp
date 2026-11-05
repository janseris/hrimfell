#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// outside: stand at (Skyrim local x, y) with feet at a height (cm over the land offset), walk toward (x2, y2), print
static int E(int lx, int o, int sh) { return (int)(((long long)lx + o + sh) * 36576 / 10000); }
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 int off = g->iR->iLandOffset;
 int x = E(atoi(argv[1]), 21996, 233472), z = E(atoi(argv[2]), -45244, 176128), tx = E(atoi(argv[4]), 21996, 233472), tz = E(atoi(argv[5]), -45244, 176128);
 g->EnterCell(0, x, z, 0); g->iFadeUs = 0; g->iUi = UI_PLAY; g->iS.py = off + atoi(argv[3]) * METER / 100 + 410; g->iS.vy = 0;
 for (int t = 0; t < 30; t++)
  { g->iS.yaw = r3d_atan2(tx - g->iS.px, tz - g->iS.pz); g->Tick(16000, KEY_FWD);
    printf("t%d (%d,%d) feet %d cm\n", t, (int)((long long)g->iS.px * 10000 / 36576 - 233472 - 21996), (int)((long long)g->iS.pz * 10000 / 36576 - 176128 + 45244), (g->iS.py - 410 - off) * 100 / METER); }
}
