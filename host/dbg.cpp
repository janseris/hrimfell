#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 for (int t=0;t<3;t++){
 int cs=r3d_cos(g->iTentYaw[t]), sn=r3d_sin(g->iTentYaw[t]);
 for (int lz=-200; lz<=200; lz+=40){ int lx=0; int x=g->iTentX[t]+((lx*cs+lz*sn)>>14), z=g->iTentZ[t]+((-lx*sn+lz*cs)>>14); int gy=g->iR->GroundAt(x,z);
   printf("%d", g->BlockedAt(0,x,z,100,gy+7*METER/25,7*METER/4-7*METER/25)); }
 printf("  tent %d\n", t); }
 R3D& r=*g->iR; for (int i=0;i<r.iObjCount;i++){ Obj&o=r.iObj[i]; if (o.x==g->iTentX[0] && o.z==g->iTentZ[0]) printf("tent obj type %d y %d ground %d nparts %d\n", o.type, o.y, r.GroundAt(o.x,o.z), r.iDefs[o.type-ST_CUSTOM].nparts); }
}
