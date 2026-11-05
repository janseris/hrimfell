#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// the Riverwood Trader's upstairs door: to the balcony and back; its street door still to the street
static Game* g;
static void Where(const char* s) { int f = g->iS.py - 410; printf("%s: cell %d feet %d ground %d (cm over ground %d)\n", s, g->iS.cell, f, g->iS.cell ? 0 : g->iR->GroundAt(g->iS.px, g->iS.pz), g->iS.cell ? 0 : (f - g->iR->GroundAt(g->iS.px, g->iS.pz)) * 100 / METER); }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 int fails = 0;
 g->EnterCell(CELL_RWTRADER, 100*METER - 191*METER/70, 100*METER - 400*METER/70, 512); g->iFadeUs = 0;
 g->iS.py = g->FloorAt(CELL_RWTRADER, g->iS.px, g->iS.pz, 100*METER + 260*METER/70) + 410;
 Where("upstairs"); g->FindDoor(); printf("door %d\n", g->iDoorFocus); if (g->iDoorFocus != 13) fails++;
 if (g->iDoorFocus >= 0) g->UseDoor(g->iDoorFocus);
 Where("out"); if (g->iS.cell != 0) fails++;
 for (int t = 0; t < 30; t++) g->Tick(16000, 0);
 Where("out after 0.5 s");
 int bal = g->iS.py - 410;
 g->iS.yaw = (g->iS.yaw + 512) & 1023; g->FindDoor(); printf("door back %d\n", g->iDoorFocus); if (g->iDoorFocus != 13) fails++;
 if (g->iDoorFocus >= 0) g->UseDoor(g->iDoorFocus);
 Where("back in"); printf("  feet %d u (want ~240)\n", (g->iS.py - 410 - 100*METER)*70/METER); if (abs((g->iS.py - 410 - 100*METER)*70/METER - 240) > 30) fails++;
 // the street door from the street
 const DoorLink& s = g->Door(11);
 g->EnterCell(0, s.ox + ((r3d_sin(s.oyaw) * METER) >> 14), s.oz + ((r3d_cos(s.oyaw) * METER) >> 14), (s.oyaw + 512) & 1023);
 for (int t = 0; t < 10; t++) g->Tick(16000, 0);
 Where("street"); printf("  balcony was %d cm over this\n", (bal - (g->iS.py - 410)) * 100 / METER);
 g->FindDoor(); printf("door %d\n", g->iDoorFocus); if (g->iDoorFocus != 11) fails++;
 if (g->iDoorFocus >= 0) g->UseDoor(g->iDoorFocus);
 Where("in"); printf("  feet %d u (want ~0)\n", (g->iS.py - 410 - 100*METER)*70/METER);
 printf("FAILS %d\n", fails); return fails; }
