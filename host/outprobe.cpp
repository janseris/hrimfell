#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// outside at a house's door: the land, the kit floor, blocked, along the line out from the door
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame();
 int d = atoi(argv[1]); const DoorLink& a = g->Door(d); int off = g->iR->iLandLoaded ? g->iR->iLandOffset : 0;
 printf("%s: door out (%d,%d), oyaw %d, land off %d\n", a.name, a.ox, a.oz, a.oyaw, off);
 for (int m = -2; m <= 12; m++)
  {
  int x = a.ox + ((r3d_sin(a.oyaw) * m * METER / 2) >> 14), z = a.oz + ((r3d_cos(a.oyaw) * m * METER / 2) >> 14);
  int gr = g->iR->GroundAt(x, z), f, k = GameKitOutFloor(*g, x, z, gr + 3 * METER, off, &f);
  int fl = g->FloorAt(0, x, z, gr + 2 * METER);
  printf(" %4.1f m: land %d cm, kit floor %s %d cm, FloorAt(feet land+2m) %d cm, blocked %d, kit walls %d\n", m / 2.0, (gr - off) * 100 / METER, f ? "yes" : "no ", f ? (k - off) * 100 / METER : 0, (fl - off) * 100 / METER,
         g->BlockedAt(0, x, z, 104, fl + 7 * METER / 25, 7 * METER / 4 - 7 * METER / 25), GameKitOutBlocked(*g, x, z, 104, fl + 7 * METER / 25, off));
  }
}
