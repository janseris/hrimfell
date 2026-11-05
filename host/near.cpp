// (v0.4.74) the big pieces near a spot (Skyrim world X Y): their models, how far, which of their models is drawn
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
 emu_init(m, 0); Game* g = (Game*)m; emu_mode(m, 0);
 double X = atof(argv[1]), Y = atof(argv[2]); int R = argc > 3 ? atoi(argv[3]) : 30;
 int ex = (int)((X + 233472) * 0.0142875 * METER), ez = (int)((Y + 176128) * 0.0142875 * METER);
 g->EnterCell(0, ex, ez, 0);
 R3D& r = *g->iR;
 for (int i = 0; i < r.iObjCount; i++)
  {
  const Obj& o = r.iObj[i];
  if (o.type < ST_CUSTOM) continue;
  int d = (int)sqrt((double)(o.x - ex) * (o.x - ex) + (double)(o.z - ez) * (o.z - ez)) / METER;
  if (d > R) continue;
  const StaticDef& sd = r.iDefs[o.type - ST_CUSTOM];
  const char* nm = "?"; for (int k = 0; k < g->iKitMeshN && k < Game::MAXKIT; k++) if (&g->iMKit[k] == sd.mesh) nm = g->KitM(k).name;
  int rad = (sd.mesh->radius * o.scale >> 8) / METER;
  if (rad < 4) continue;
  printf("%3d m  r %3d m  y %+5.1f m  yaw %4d  scale %3d  faces %4d  %s\n", d, rad, (o.y - g->iR->GroundAt(ex, ez)) / (double)METER, o.yaw, o.scale, sd.mesh->nf, nm);
  }
 }
