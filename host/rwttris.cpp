#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame();
 int sum[400]={0}, cnt[400]={0}, tot=0;
 for (int i=0;i<g->iR->iINum;i++){ const Obj& o=g->iR->iIObj[i]; if(o.flag!=CELL_RWINN) continue; const StaticDef& d=g->iR->iDefs[o.type-ST_CUSTOM]; int nf=d.mesh?d.mesh->nf:0; sum[o.type-ST_CUSTOM]+=nf; cnt[o.type-ST_CUSTOM]++; tot+=nf; }
 for (int t=0;t<400;t++) if(sum[t]) { int in=-1; for(int k=0;k<IN_COUNT;k++) if(g->iInType[k]-ST_CUSTOM==t) in=k; printf("IN %3d x%2d  %5d faces\n", in, cnt[t], sum[t]); }
 printf("statics total %d\n", tot);
 int pt[32]={0}; for(int i=0;i<g->iS.nprop;i++) if(g->iS.prop[i].cell==CELL_RWINN) pt[g->iS.prop[i].type]++;
 for(int t=0;t<32;t++) if(pt[t]) printf("prop type %d x%d\n",t,pt[t]);
}
