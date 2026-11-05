#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// the kit models' normals and texels as the generator wrote them, against the engine's own (MeshFromData)
#include "../src/kit_ext.inc"
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 long bad = 0, badn = 0, tot = 0;
 for (int i = 0; i < KKitMesh_N; i++)
  {
  const KitMesh& km = KKitMesh[i]; Mesh m; u8* save = g->iR->iArena;
  g->iR->MeshFromData(m, km.v, km.nv, km.f, km.nf);
  for (int k = 0; k < km.nf * 3; k++) if (m.n[k] != km.n[k]) { if (abs(m.n[k] - km.n[k]) > 2) badn++; }
  for (int k = 0; k < km.nf * 6; k++) if (m.uv[k] != km.uv[k]) bad++;
  tot += km.nf; g->iR->iArena = save;
  }
 printf("faces %ld: normals off by more than 2: %ld, texels different: %ld\n", tot, badn, bad);
}
