#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 int wx=g->iWellX, wz=g->iWellZ; int gy=g->iR->GroundAt(wx+3*METER, wz);
 int it = g->AddProp(PT_ITEM, 0, wx, gy + 10, wz + 2*METER + METER/2, 0, IT_APPLE, 1, 0);
 g->EnterCell(0, wx, wz + 3*METER + METER/2, 512); g->iFadeUs=0;
 g->iS.pitch = -150;
 for(int k=0;k<3;k++) g->Tick(16000,0);
 printf("focus %d %d\n", g->iFocusKind, g->iFocus==it);
 g->KeyChar('e'); for(int k=0;k<30;k++) g->Tick(16000, KEY_USE);
 printf("grab %d\n", g->iGrab);
 for(int k=0;k<5;k++) g->Tick(16000,0);
 g->iS.pitch = -40;
 for(int k=0;k<10;k++) g->Tick(16000,0);
 g->Tick(16000, KEY_FIRE); g->Tick(16000, KEY_FIRE);
 printf("thrown grab %d vx %d vy %d drawn %d\n", g->iGrab, g->iS.prop[it].vx, g->iS.prop[it].vy, g->iS.drawn);
 for(int k=0;k<120;k++) { g->Tick(16000,0); }
 Prop&p=g->iS.prop[it]; printf("apple at %d,%d (well %d,%d) y %d ground %d gone %d\n", (p.x-wx)*100/METER, (p.z-wz)*100/METER, 0,0, p.y, g->iR->GroundAt(p.x,p.z), (p.flags&PF_GONE)!=0);
 // player cannot walk into the well, can pass between stall posts? stall counter blocks
 printf("player blocked at well centre %d, at ring edge %d\n", g->BlockedAt(0, wx, wz, 100, gy), g->BlockedAt(0, wx + METER*14/10, wz, 100, gy));
 printf("small item over the shaft blocked %d\n", g->BlockedAt(0, wx, wz, 12, gy + METER/2, 51));
}
