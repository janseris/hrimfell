#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// a house from the middle of its room at standing height, four ways -> shots/kitmid.ppm
static u16 out[640*200];
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY; g->iHideView = 0;
 g->iR->SetOutputMode(2); int k = atoi(argv[1]), cx = atoi(argv[2]), cy = atoi(argv[3]), cz = atoi(argv[4]); if (argc > 5) { g->iS.timeOfDay = atoi(argv[5]) * 60000; g->RiverwoodSchedule(1); }   // optional: the hour
 FILE* f=fopen("shots/kitmid.ppm","wb"); fprintf(f,"P6 640 800 255\n");
 for (int v = 0; v < 4; v++)
   { g->Teleport(k == 4 ? 23 : k == 5 ? 24 : 26 + k); g->iUi = UI_PLAY; g->iFadeUs = 0;
     g->iS.px = 100 * METER + cx * 3657 / 1000; g->iS.pz = 100 * METER + cy * 3657 / 1000;
     g->iS.py = 100 * METER + cz * 3657 / 1000 + 410; g->iS.yaw = v * 256; g->iS.pitch = 0;
     g->iFadeUs = 0; g->Render(out, 640);
     for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); } }
 fclose(f);}
