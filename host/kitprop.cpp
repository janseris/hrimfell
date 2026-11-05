#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// the props near a point in a kit house (Skyrim units)
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame();
 int cell = atoi(argv[1]), x = atoi(argv[2]), z = atoi(argv[3]), r = argc > 4 ? atoi(argv[4]) : 150;
 { R3D& R = *g->iR; int ex = 100*METER + x*METER/70, ez = 100*METER + z*METER/70;
   for (int i = 0; i < R.iINum; i++) { const Obj& o = R.iIObj[i]; if (o.flag != cell || o.type < ST_CUSTOM) continue; const StaticDef& d = R.iDefs[o.type - ST_CUSTOM];
    if (!d.hx) continue; int dx = ex - o.x, dz = ez - o.z; int cs = r3d_cos(o.yaw), sn = r3d_sin(o.yaw);
    int lx = ((dx * cs - dz * sn) >> 14) - (d.bx * o.scale >> 8), lz = ((dx * sn + dz * cs) >> 14) - (d.bz * o.scale >> 8); int hx = (d.hx * o.scale >> 8) + 75, hz = (d.hz * o.scale >> 8) + 75;
    if (abs(lx) < hx && abs(lz) < hz) { int k; for (k = 0; k < 128; k++) if (g->iKitType[k] == o.type) break; printf("type %d ", o.type);
      printf("static kit model %d at (%d,%d) y %d cm, box %dx%d hy %d (units)\n", k, (o.x-100*METER)*70/METER, (o.z-100*METER)*70/METER, (o.y-100*METER)*100/METER, d.hx*70/METER, d.hz*70/METER, d.hy*70/METER); } } }
 for (int i = 0; i < g->iS.nprop; i++) { const Prop& p = g->iS.prop[i]; if (p.cell != cell) continue;
  int px = (p.x - 100*METER) * 70 / METER, pz = (p.z - 100*METER) * 70 / METER;
  if (abs(px - x) < r && abs(pz - z) < r) printf("prop %d type %d at (%d,%d) y %d cm state %d yaw %d\n", i, p.type, px, pz, (p.y - 100*METER)*100/METER, p.state, p.yaw); }
}
// and the statics whose box blocks there (compile with -DSTATS)
