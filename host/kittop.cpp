#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// a house from above (camera high over the middle, looking straight down) and from a corner -> shots/kittop.ppm
static u16 out[640*200];
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY; g->iHideView = 0;
 g->iR->SetOutputMode(2); int k = argc > 1 ? atoi(argv[1]) : 3;
 FILE* f=fopen("shots/kittop.ppm","wb"); fprintf(f,"P6 640 600 255\n");
 int cx = argc > 2 ? atoi(argv[2]) : -350, cz = argc > 3 ? atoi(argv[3]) : 0;   // Skyrim units
 struct V { int dx, dz, up, yaw, pitch; } vs[] = { { 0, -1, 1400, 0, -240 }, { -500, -350, 300, 128, -60 }, { 400, 300, 300, 640, -60 } };
 for (int v = 0; v < 3; v++)
   { g->Teleport(26 + k); g->iUi = UI_PLAY; g->iFadeUs = 0;
     g->iS.px = 100 * METER + (cx + vs[v].dx) * 3657 / 1000; g->iS.pz = 100 * METER + (cz + vs[v].dz) * 3657 / 1000;
     g->iS.py = 100 * METER + vs[v].up * METER / 100 + 410; g->iS.yaw = vs[v].yaw; g->iS.pitch = vs[v].pitch;
     g->iFadeUs = 0; g->Render(out, 640);
     for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); } }
 fclose(f);}
