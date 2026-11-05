#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 for(int i=0;i<23;i++){ Mesh& m=g->iMCity[i]; printf("city %2d nv %3d/%3d nf %3d/%3d frames %d brad %d\n", i, m.nv, m.vstride, m.nf, m.fstride, m.nfr, m.brad); }
 for(int i=0;i<g->iR->iNumDefs;i++){ const StaticDef& d=g->iR->iDefs[i]; printf("def %d mesh nv %d\n", i, d.mesh? d.mesh->nv:-1); }
}
