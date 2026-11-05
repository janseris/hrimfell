#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 R3D& r=*g->iR; int x=(int)(atof(argv[1])*256), z=(int)(atof(argv[2])*256);
 printf("ST_CUSTOM %d\n", ST_CUSTOM);
 int cx=x/(CELL*CHUNK), cz=z/(CELL*CHUNK);
 for (int a=-1;a<=1;a++) for (int b=-1;b<=1;b++){ R3D::TChunk* t=r.Cached(cx+a,cz+b,0); if(!t) continue; for(int k=0;k<t->ntree;k++){ Obj& o=t->tree[k]; if (abs(o.x-x)<8*256 && abs(o.z-z)<8*256) printf("cached %s at (%.1f,%.1f) scale %d\n", o.type==ST_ROCK?"rock":"tree", o.x/256.0,o.z/256.0,o.scale);} }
 for (int i=0;i<r.iObjCount;i++){ Obj& o=r.iObj[i]; if (abs(o.x-x)<30*256 && abs(o.z-z)<30*256) {
   const char* nm="-"; if (o.type>=ST_CUSTOM){ const Mesh* m=r.iDefs[o.type-ST_CUSTOM].mesh; nm = m==&g->iMBfWall?"wall":m==&g->iMBfPillar?"pillar":m==&g->iMRib?"rib":m==&g->iMBfStairs?"stairs":m==&g->iMFacade?"facade":"other"; }
   printf("obj %d type %d %s y %.2f at (%.1f,%.1f) yaw %d scale %d\n", i,o.type,nm,o.y/256.0,o.x/256.0,o.z/256.0,o.yaw,o.scale); } }
}
