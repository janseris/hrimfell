#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// outside: a view from (Skyrim local x, y of Riverwood) at standing height, looking at (x2, y2) -> shots/outshot.ppm
static u16 out[640*200];
static int E(int lx, int o, int sh) { return (int)(((long long)lx + o + sh) * 36576 / 10000); }
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY; g->iHideView = 0;
 g->iR->SetOutputMode(2);
 int x = E(atoi(argv[1]), 21996, 233472), z = E(atoi(argv[2]), -45244, 176128), tx = E(atoi(argv[3]), 21996, 233472), tz = E(atoi(argv[4]), -45244, 176128);
 g->EnterCell(0, x, z, r3d_atan2(tx - x, tz - z)); g->iFadeUs = 0; g->iUi = UI_PLAY;
 g->iS.py = g->FloorAt(0, x, z, g->iR->GroundAt(x, z) + 2 * METER) + 410; g->iS.pitch = argc > 5 ? atoi(argv[5]) : 0;
 for (int k = 0; k < 3; k++) { g->iFadeUs = 0; g->Render(out, 640); }
 FILE* f=fopen("shots/outshot.ppm","wb"); fprintf(f,"P6 640 200 255\n");
 for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
 fclose(f); }
