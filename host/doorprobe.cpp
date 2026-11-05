#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame();
 for (int i = 0; i < g->iS.nprop; i++) { Prop& p = g->iS.prop[i]; if (p.cell != CELL_RWINN || p.type != PT_DOOR) continue;
   printf("door at (%d,%d) cm yaw %d anim %d\n", (p.x-100*METER)*100/METER, (p.z-100*METER)*100/METER, p.yaw, p.anim);
   for (int a = 0; a <= 4; a += 4) { p.anim = a; for (int dz = -60; dz <= 60; dz += 30) { int X = p.x + ((r3d_sin(p.yaw) * (54 * METER / 100)) >> 14) + ((r3d_cos(p.yaw) * (dz * METER / 100)) >> 14), Z = p.z + ((r3d_cos(p.yaw) * (54 * METER / 100)) >> 14) - ((r3d_sin(p.yaw) * (dz * METER / 100)) >> 14); printf("  anim %d at +54cm along x, %+d cm in z: blocked %d\n", a, dz, g->PropBlocked(CELL_RWINN, X, Z, 100, 100 * METER)); } }
   p.anim = 0; }
}
