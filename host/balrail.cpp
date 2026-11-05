#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// the Riverwood Trader's balcony: out of the upstairs door, then walk 6 m each way; the rail must keep you on it
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 int fails = 0;
 for (int dir = 0; dir < 8; dir++)
  {
  g->EnterCell(CELL_RWTRADER, 100*METER - 191*METER/70, 100*METER - 400*METER/70, 512); g->iFadeUs = 0;
  g->iS.py = g->FloorAt(CELL_RWTRADER, g->iS.px, g->iS.pz, 100*METER + 260*METER/70) + 410;
  g->FindDoor(); if (g->iDoorFocus >= 0) g->UseDoor(g->iDoorFocus);
  for (int t = 0; t < 20; t++) g->Tick(16000, 0);
  int f0 = g->iS.py - 410, x0 = g->iS.px, z0 = g->iS.pz;
  g->iS.yaw = (g->iS.yaw + dir * 128) & 1023;
  g->iFadeUs = 0; g->iUi = UI_PLAY; for (int t = 0; t < 400; t++) g->Tick(16000, KEY_FWD);
  int f1 = g->iS.py - 410;
  int moved = (abs(g->iS.px - x0) + abs(g->iS.pz - z0)) * 100 / METER;
  int drop = (f0 - f1) * 100 / METER;
  printf("cell %d ui %d ", g->iS.cell, g->iUi); printf("dir %d: moved %d cm, dropped %d cm %s\n", dir, moved, drop, drop > 100 ? "FELL" : "ok");
  if (drop > 100) fails++;
  }
 printf("FAILS %d\n", fails); return fails; }
