#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame();
 int x=atoi(argv[1]), z=atoi(argv[2]), f=atoi(argv[3]);
 int X=100*METER+x*METER/100, Z=100*METER+z*METER/100, F=100*METER+f*METER/100;
 printf("floor %d prop %d all %d\n", (g->FloorAt(CELL_RWTRADER,X,Z,F)-100*METER)*100/METER, g->PropBlocked(CELL_RWTRADER,X,Z,100,F+7*METER/25), g->BlockedAt(CELL_RWTRADER,X,Z,100,F+7*METER/25,7*METER/4-7*METER/25));
 g->iR->iCell=CELL_RWTRADER;
 for (int i=0;i<g->iR->iINum;i++){ const Obj& o=g->iR->iIObj[i]; if(o.flag!=CELL_RWTRADER) continue; int dx=(o.x-X)*100/METER, dz=(o.z-Z)*100/METER; if (abs(dx)<150&&abs(dz)<150){ const StaticDef& d=g->iR->iDefs[o.type-ST_CUSTOM]; printf(" obj type %d at %d,%d y %d hx %d hz %d rad %d hy %d\n", o.type, dx, dz, (o.y-100*METER)*100/METER, d.hx, d.hz, d.rad, d.hy);} }
 for (int i=0;i<g->iS.nprop;i++){ const Prop& p=g->iS.prop[i]; if(p.cell!=CELL_RWTRADER) continue; int dx=(p.x-X)*100/METER, dz=(p.z-Z)*100/METER; if (abs(dx)<150&&abs(dz)<150) printf(" prop %d at %d,%d y %d\n", p.type, dx, dz, (p.y-100*METER)*100/METER); }
}
