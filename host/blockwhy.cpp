#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// why a spot is blocked: kit walls, props, statics (cell, x, y in Skyrim units, feet height in units)
int GameKitBlocked(const Game& gg, int aCell, int aX, int aZ, int aR, int aFeetY);
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame();
 int cell = atoi(argv[1]); g->EnterCell(cell, 100*METER, 100*METER, 0);
 for (int a = 2; a + 2 < argc + 1; a += 3)
  {
  int x = 100*METER + atoi(argv[a]) * METER / 70, z = 100*METER + atoi(argv[a+1]) * METER / 70, f = 100*METER + atoi(argv[a+2]) * METER / 70, r = 100;
  int fl = g->FloorAt(cell, x, z, f);
  printf("(%s,%s) floor %d u: all %d, kit walls %d, props %d, statics %d\n", argv[a], argv[a+1], (fl - 100*METER)*70/METER, g->BlockedAt(cell, x, z, r, fl + 7 * METER / 25, 7 * METER / 4 - 7 * METER / 25), GameKitBlocked(*g, cell, x, z, r, fl + 7 * METER / 25), g->PropBlocked(cell, x, z, r, fl + 7 * METER / 25), (g->iR->iCell = cell, g->iR->BlockedIn(x, z, r, fl + 7 * METER / 25)));
  }
}
