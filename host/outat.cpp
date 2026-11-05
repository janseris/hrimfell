#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// outside at a Riverwood spot (cm from RW_OX, RW_OZ): the land and the kit floor
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame();
 int off = g->iR->iLandLoaded ? g->iR->iLandOffset : 0;
 for (int a = 1; a + 1 < argc; a += 2)
  { int x = 3650 * METER + atoi(argv[a]) * METER / 100, z = 1870 * METER + atoi(argv[a + 1]) * METER / 100;
    int gr = g->iR->GroundAt(x, z), f, k = GameKitOutFloor(*g, x, z, gr + 2 * METER, off, &f);
    int fl = f ? k : gr;
    printf("(%s,%s) cm: land %d cm, deck %s %d cm, blocked %d (kit walls %d, statics %d)\n", argv[a], argv[a + 1], (gr - off) * 100 / METER, f ? "yes" : "no", f ? (k - off) * 100 / METER : 0,
           g->BlockedAt(0, x, z, 100, fl + 7 * METER / 25, 7 * METER / 4 - 7 * METER / 25), GameKitOutBlocked(*g, x, z, 100, fl + 7 * METER / 25, off), g->iR->Blocked(x, z, 100, fl + 7 * METER / 25, fl + 7 * METER / 4)); }
}
