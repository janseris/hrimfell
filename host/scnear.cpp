// (v0.4.74) what lies within R m of an emulator scene's camera
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
 int R = argc > 2 ? atoi(argv[2]) : 10;
 R3D& r = *g->iR; int ex = g->iS.px, ez = g->iS.pz;
 printf("cell %d at %d %d yaw %d objs %d\n", g->iS.cell, ex, ez, g->iS.yaw, r.iObjCount);
 for (int i = 0; i < r.iObjCount; i++)
  {
  const Obj& o = r.iObj[i];
  if (o.type < ST_CUSTOM) { if (argc > 3) printf("type %d at %d %d\n", o.type, (o.x-ex)/METER, (o.z-ez)/METER); continue; }
  int d = (int)sqrt((double)(o.x - ex) * (o.x - ex) + (double)(o.z - ez) * (o.z - ez)) / METER;
  if (d > R) continue;
  const StaticDef& sd = r.iDefs[o.type - ST_CUSTOM];
  const char* nm = "?"; for (int k = 0; k < g->iKitMeshN && k < Game::MAXKIT; k++) if (&g->iMKit[k] == sd.mesh) nm = g->KitM(k).name;
  printf("%3d m  dx %+6.1f dz %+6.1f y %+5.1f yaw %4d faces %4d %s\n", d, (o.x-ex)/(double)METER, (o.z-ez)/(double)METER, (o.y - g->iS.py) / (double)METER, o.yaw, sd.mesh->nf, nm);
  }
 }
