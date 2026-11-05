#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// what each of Bleak Falls' outside bandits gives when searched
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame();
 static const char* const KK[4] = { "one-handed", "two-handed", "archer", "heavy" };
 for (int i = 0; i < g->iS.ne; i++)
  {
  Ent& e = g->iS.e[i]; if (e.type != E_BANDIT || e.cell) continue;
  int gold = g->iS.gold, inv = g->iS.ninv;
  g->KillEnt(e); g->GiveLoot(e);
  printf("bandit %d (%s): gold +%d, items %d -> %d\n", i, KK[BanditKind(e)], g->iS.gold - gold, inv, g->iS.ninv);
  for (int k = 0; k < MAXMSG; k++) printf("   %s\n", g->iMsg[k]);
  for (int k = 0; k < MAXMSG; k++) g->iMsg[k][0] = 0;
  }
 return 0; }
