#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static u16 out[640*200];
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 R3D& r=*g->iR; r.SetOutputMode(2); g->iUi = UI_PLAY;
 g->iS.px = 300 * METER; g->iS.pz = 200 * METER; g->iS.py = r.GroundAt(g->iS.px, g->iS.pz) + 410; g->iS.yaw = 128;
 g->iS.view = 2|4; g->ApplyViewSetting(); g->iS.timeOfDay = 12 * 3600 * 1000 / 60;
 for (int t = 0; t < 3; t++) g->Tick(16000, 0);
 g->Render(out, 640);
 printf("farOn %d farVis %d near %d far %d fogEnd %d fog0 %d fogK %d vis %d\n", r.iFarOn, r.iFarVis, r.iNearM, r.iFarM, r.iFogEndM, r.iFog0, r.iFogK, r.iVis);
 int mx=600*CELL, mz=420*CELL; printf("peak h %d m ground at cam %d m\n", r.GroundAt(mx,mz)/METER, r.GroundAt(g->iS.px,g->iS.pz)/METER);
 int sx, sy; int ok=r.ProjectWorld(mx, r.GroundAt(mx,mz), mz, sx, sy); printf("peak on screen %d at %d,%d\n", ok, sx, sy);
}
