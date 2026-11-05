#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 for(int k=0;k<IN_COUNT;k++){ Mesh& m=g->iMIn[k]; if(m.dropped) printf("in %d nv %d/%d nf %d/%d dropped %d\n",k,m.nv,m.vstride,m.nf,m.fstride,m.dropped);}
 printf("defs %d dropped %d iobj %d props %d conts %d ents %d\n", g->iR->iNumDefs, g->iR->iMeshDropped, g->iR->iINum, g->iS.nprop, g->iS.ncont, g->iS.ne);
 printf("arena free %d KB\n", (int)(g->iR->iArenaEnd - g->iR->iArena)/1024);}
