#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// a view in a kit cell: from (x, y) to (tx, ty) (Skyrim units in the cell), feet at f units, pitch p; open = 1 opens the
// chests and barrels there (their open frames) -> the ppm named last
static u16 out[640*200];
int main(int argc, char** argv)
 { int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY; g->iHideView = 0;
 g->iR->SetOutputMode(2); g->iS.timeOfDay = 13 * 60000;
 int cell = atoi(argv[1]), x = atoi(argv[2]), y = atoi(argv[3]), tx = atoi(argv[4]), ty = atoi(argv[5]), f = atoi(argv[6]), p = atoi(argv[7]), open = atoi(argv[8]);
 int wx = 100*METER + x*METER/70, wz = 100*METER + y*METER/70, twx = 100*METER + tx*METER/70, twz = 100*METER + ty*METER/70;
 g->EnterCell(cell, wx, wz, r3d_atan2(twx - wx, twz - wz)); g->iFadeUs = 0; g->iUi = UI_PLAY;
 g->iS.py = g->FloorAt(cell, wx, wz, 100*METER + f*METER/70) + 410; g->iS.pitch = p;
 for (int t = 0; t < 4; t++) g->Tick(16000, 0);
 g->iS.px = wx; g->iS.pz = wz; g->iS.yaw = r3d_atan2(twx - wx, twz - wz); g->iS.pitch = p;
 if (getenv("NOPEOPLE")) for (int i = 0; i < g->iS.ne; i++) g->iS.e[i].cell = -1;
 if (open) for (int i = 0; i < g->iS.nprop; i++) if (g->iS.prop[i].cell == cell) g->iS.prop[i].anim = 4;
 g->iFadeUs = 0; g->Render(out, 640);
 FILE* fo = fopen(argv[9], "wb"); fprintf(fo, "P6 640 200 255\n");
 for (int i = 0; i < 640 * 200; i++) { u16 q = out[i]; unsigned char c[3] = { (unsigned char)((q >> 8) & 0xF8), (unsigned char)((q >> 3) & 0xFC), (unsigned char)((q << 3) & 0xF8) }; fwrite(c, 1, 3, fo); }
 fclose(fo); const char* nm = g->LookedAtModel(); printf("looking at: %s\n", nm ? nm : "(nothing)"); return 0; }
