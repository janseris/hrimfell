// (v0.4.74) a scene's doors (props): where, their model, whether it can be drawn
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
int main(int argc, char** argv)
 {
 int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64); u8* m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
 emu_init(m, 0); Game* g = (Game*)m; emu_mode(m, 0); emu_scene(m, atoi(argv[1]));
 printf("cell %d at %d %d yaw %d kitN %d\n", g->iS.cell, g->iS.px, g->iS.pz, g->iS.yaw, g->iKitMeshN);
 for (int i = 0; i < g->iS.nprop; i++)
  {
  const Prop& p = g->iS.prop[i];
  if (p.type != PT_DOOR || p.cell != g->iS.cell) continue;
  int kt = p.model >= 0 && p.model < Game::MAXKIT ? g->iKitType[p.model] : -1;
  printf("%s ", g->KitM(p.model).name); printf("door %d at %+.1f %+.1f m  y %+.1f yaw %d count %d model %d kittype %d state %d anim %d\n", i, (p.x-g->iS.px)/(double)METER, (p.z-g->iS.pz)/(double)METER, (p.y-g->iS.py)/(double)METER, p.yaw, p.count, p.model, kt, p.state, p.anim);
  }
 }
