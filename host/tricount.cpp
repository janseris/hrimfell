#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// rough per-type triangle budget of statics in front of a camera (city metres): x z tx tz
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 R3D& r=*g->iR; int cx0=CITY_CX*CELL+CELL/2, cz0=CITY_CZ*CELL+CELL/2;
 int cx=cx0+atoi(argv[1])*METER, cz=cz0+atoi(argv[2])*METER, tx=cx0+atoi(argv[3])*METER, tz=cz0+atoi(argv[4])*METER;
 int fx=(tx-cx)>>8, fz=(tz-cz)>>8; int fl=(int)__builtin_sqrt((double)(fx*fx+fz*fz));
 static int sum[256], cnt[256]; int tot=0;
 for (int i=0;i<r.iObjCount;i++){ Obj&o=r.iObj[i]; if(o.type<ST_CUSTOM) continue; const StaticDef&d=r.iDefs[o.type-ST_CUSTOM];
  int dx=(o.x-cx)>>8, dz=(o.z-cz)>>8; int d2=dx*dx+dz*dz; if (d2>d.drawDist*d.drawDist) continue;
  int along=(dx*fx+dz*fz)/fl, side=abs(dx*fz-dz*fx)/fl; int rad=d.mesh->brad*o.scale/256/256+2;
  if (along < -rad) continue; if (side > along*1.0+rad) continue;   // ~90 deg fov
  const Mesh* m=(d.lod && d2>d.lodDist*d.lodDist)?d.lod:d.mesh; sum[o.type]+=m->nf; cnt[o.type]++; tot+=m->nf; }
 printf("total %d\n", tot);
 for (int t=0;t<256;t++) if (sum[t]>150) printf("type %d: %d objs, %d faces\n", t, cnt[t], sum[t]);
 for (int k=0;k<60;k++) if (g->iCityType[k]) printf("CT %d -> type %d\n", k, g->iCityType[k]);
}
