// (v0.4.74) where FreeSpot puts a scene's player: moved how far, how high, the ways out before and after
#include <stdio.h>
#include <stdlib.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
int main(int argc, char** argv)
 {
 int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64); u8* m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
 emu_init(m, 0); Game* g = (Game*)m; emu_mode(m, 0);
 for (int a = 1; a < argc; a++)
  {
  int s = atoi(argv[a]);
  if (s >= 34 && s <= 36)
   {
   const DoorLink& d = g->Door(26 + (s - 34));
   int fx = (r3d_sin(d.iyaw) * 3 * METER) >> 14, fz = (r3d_cos(d.iyaw) * 3 * METER) >> 14;
   int x = d.ix + fx, z = d.iz + fz, f = g->FloorAt(d.cell, x, z, d.iy);
   emu_scene(m, s);
   printf("scene %d: asked %d %d feet %d (ways out %d) -> %+.2f %+.2f m, feet %+.2f m (ways out %d)\n", s, x, z, f, g->WaysOut(d.cell, x, z, f),
          (g->iS.px - x) / (double)METER, (g->iS.pz - z) / (double)METER, (g->iS.py - 410 - f) / (double)METER, g->WaysOut(d.cell, g->iS.px, g->iS.pz, g->iS.py - 410));
   }
  else { emu_scene(m, s); printf("scene %d: at %d %d feet %d ways out %d\n", s, g->iS.px, g->iS.pz, g->iS.py - 410, g->WaysOut(g->iS.cell, g->iS.px, g->iS.pz, g->iS.py - 410)); }
  }
 }
