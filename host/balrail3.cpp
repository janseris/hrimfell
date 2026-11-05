#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
int GameKitOutBlocked(const Game& g, int aX, int aZ, int aR, int aFeetY, int aOff);
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 g->EnterCell(CELL_RWTRADER, 100*METER - 191*METER/70, 100*METER - 400*METER/70, 512); g->iFadeUs = 0;
 g->iS.py = g->FloorAt(CELL_RWTRADER, g->iS.px, g->iS.pz, 100*METER + 260*METER/70) + 410;
 g->FindDoor(); if (g->iDoorFocus >= 0) g->UseDoor(g->iDoorFocus);
 for (int t = 0; t < 20; t++) g->Tick(16000, 0);
 int feet = g->iS.py - 410;
 for (int ly = -440; ly >= -520; ly -= 10)
  { double lx = 15, a = 135 * M_PI / 180; double X = 22176 + lx * cos(a) + ly * sin(a), Y = -45866 - lx * sin(a) + ly * cos(a);
    int ex = (int)((X + 233472) * 0.0142875 * METER), ez = (int)((Y + 176128) * 0.0142875 * METER);
    printf("ly %d: blocked %d kit %d (feet %d off %d loaded %d)\n", ly, g->BlockedAt(0, ex, ez, 100, feet + 45 * METER / 100, 7 * METER / 4), GameKitOutBlocked(*g, ex, ez, 100, feet + 45 * METER / 100, g->iR->iLandLoaded ? g->iR->iLandOffset : 0), feet, g->iR->iLandOffset, g->iR->iLandLoaded); }
 return 0; }
