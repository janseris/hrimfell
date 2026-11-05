#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// a book lying in the world: E opens it to read, R takes it (Thief, on the pillar room's balcony)
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame();
 int b=-1; for (int i=0;i<g->iS.nprop;i++) if (g->iS.prop[i].type==PT_ITEM && g->iS.prop[i].item==IT_BOOK_THIEF) b=i;
 const Prop& p=g->iS.prop[b];
 g->EnterCell(p.cell, p.x - 12*METER/10, p.z, 256); g->iS.py = p.y + 200; g->iFadeUs=0;
 for(int k=0;k<10;k++) g->Tick(16000,0);
 g->iS.yaw = r3d_atan2(p.x-g->iS.px, p.z-g->iS.pz); g->iS.pitch = -100;
 g->iUi=UI_PLAY; g->Tick(16000,0); g->FindFocus(); g->FindPropFocus();
 printf("focus kind %d idx %d (book %d)\n", g->iFocusKind, g->iFocus, b);
 const char* verb=""; const char* name=""; char info[80]; int red=0; g->PropText(b, verb, name, info, red); printf("prompt: %s %s\n", verb, name);
 int before = g->CountItem(IT_BOOK_THIEF);
 g->KeyChar('e'); for(int k=0;k<3;k++) g->Tick(16000,0);
 printf("after E: ui %d (book %d), reading prop %d, page %d, have %d\n", g->iUi, UI_BOOK, g->iBookProp, g->iBookPage, g->CountItem(IT_BOOK_THIEF)-before);
 g->KeyChar(' '); printf("next page: %d\n", g->iBookPage);
 g->KeyChar('e'); g->Tick(16000,0);
 printf("after E: ui %d, have %d, gone %d\n", g->iUi, g->CountItem(IT_BOOK_THIEF)-before, (g->iS.prop[b].flags & PF_GONE) != 0);
}
