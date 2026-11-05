#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame();
 printf("defs %d / %d, objs %d, iobj %d, kit meshes %d, mem %d\n", g->iR->iNumDefs, MAXDEFS, g->iR->iObjCount, g->iR->iINum, g->iKitMeshN, n); return 0; }
