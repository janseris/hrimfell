#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->iUi=UI_PLAY;
 for (int i=0;i<g->iS.nprop;i++){ Prop& p=g->iS.prop[i]; if (p.type!=PT_CHAIR && p.type!=PT_BENCH) continue;
   g->EnterCell(p.cell, p.x - 150, p.z, 256); g->UseProp(i); int sx=g->iS.px, sz=g->iS.pz;
   g->StandUp(); int b = g->BlockedAt(g->iS.cell, g->iS.px, g->iS.pz, 100, g->iS.py-410);
   printf("%s %d: seat (%d,%d) -> stood at (%d,%d) cm, blocked %d\n", p.type==PT_BENCH?"bench":"chair", i, (sx-100*METER)*100/METER, (sz-100*METER)*100/METER, (g->iS.px-100*METER)*100/METER, (g->iS.pz-100*METER)*100/METER, b); }
}
