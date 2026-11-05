#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 printf("arena free %d KB over %d\n", (int)(g->iR->iArenaEnd - g->iR->iArena)/1024, g->iR->iArenaOver);
 for(int k=0;k<5;k++) printf("human %d nv %d nf %d\n",k,g->iMHuman[k].nv,g->iMHuman[k].nf);
 printf("player nv %d nf %d lod nv %d\n", g->iMPlayer[1].nv, g->iMPlayer[1].nf, g->iMHumanLod[0].nv);}
