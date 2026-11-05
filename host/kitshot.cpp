#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// the houses built from Skyrim's pieces: from just inside the door, looking in; and turned round -> shots/kit.ppm
static u16 out[640*200];
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY; g->iHideView = 0;
 g->iR->SetOutputMode(2); g->iS.timeOfDay = 13 * 60000;
 FILE* f=fopen("shots/kit.ppm","wb"); fprintf(f,"P6 640 1600 255\n");
 for (int k = 0; k < 4; k++) for (int v = 0; v < 2; v++)
   { g->Teleport(26 + k); g->iUi = UI_PLAY; g->iFadeUs = 0;
     if (v) { g->iS.yaw = (g->iS.yaw + 300) & 1023; int fx = r3d_sin(g->iS.yaw), fz = r3d_cos(g->iS.yaw); g->iS.px -= (fx * 3 * METER) >> 14; g->iS.pz -= (fz * 3 * METER) >> 14; }
     g->iS.pitch = -15;
     for (int t = 0; t < 10; t++) g->Tick(16000, 0);
     g->iFadeUs = 0; g->Render(out, 640);
     printf("%s: cell %d at (%d,%d) floor %d cm\n", g->Door(13 + k).name, g->iS.cell, (g->iS.px - 100*METER)*100/METER, (g->iS.pz - 100*METER)*100/METER, (g->iS.py - 410 - 100*METER)*100/METER);
     for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); } }
 fclose(f);}
