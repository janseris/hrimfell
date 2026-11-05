#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
int main(int argc, char** argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 int dir = atoi(argv[1]);
 g->EnterCell(CELL_RWTRADER, 100*METER - 191*METER/70, 100*METER - 400*METER/70, 512); g->iFadeUs = 0;
 g->iS.py = g->FloorAt(CELL_RWTRADER, g->iS.px, g->iS.pz, 100*METER + 260*METER/70) + 410;
 g->FindDoor(); if (g->iDoorFocus >= 0) g->UseDoor(g->iDoorFocus);
 for (int t = 0; t < 20; t++) g->Tick(16000, 0);
 g->iS.yaw = (g->iS.yaw + dir * 128) & 1023; g->iFadeUs = 0; g->iUi = UI_PLAY;
 for (int t = 0; t < 400; t++) { g->Tick(16000, KEY_FWD);
   { int f2 = g->iS.py - 410; int b = g->BlockedAt(0, g->iS.px, g->iS.pz, 100, f2 + 45 * METER / 100, 7 * METER / 4); (void)b; }
   if (t % 10 == 0) { double X = g->iS.px / (0.0142875 * METER) - 233472, Y = g->iS.pz / (0.0142875 * METER) - 176128;
     double dx = X - 22176, dy = Y + 45866, a = 135 * M_PI / 180; double lx = dx * cos(a) - dy * sin(a), ly = dx * sin(a) + dy * cos(a);
     printf("t %d local %.0f %.0f feet %d\n", t, lx, ly, (g->iS.py - 410)); } }
 return 0; }
