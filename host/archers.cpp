#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// Bleak Falls' bandit archers: from the open they shoot and hit; from the tower's top (an archer below) nothing reaches;
// a bandit on the storey below does not strike through the floor
static int WorldX(double aX) { return (int)((aX + 233472) * 3.6576); }
static int WorldZ(double aY) { return (int)((aY + 176128) * 3.6576); }
static Game* g;
static int Run(const char* what, double x, double y, double z, int secs, int onlyArchers)
 {
 int off = g->iR->iLandOffset;
 g->EnterCell(0, WorldX(x), WorldZ(y), 0); g->iFadeUs = 0; g->iUi = UI_PLAY;
 g->iS.py = g->FloorAt(0, g->iS.px, g->iS.pz, (int)(z * 1.42875 * 256 / 100) + off + METER / 2) + 410;
 for (int i = 0; i < g->iS.ne; i++) { Ent& e = g->iS.e[i]; if (e.type == E_BANDIT && !e.cell) { e.aggro = 0; if (onlyArchers && BanditKind(e) != BK_ARCHER) e.cell = 99; } }
 int hits = 0, shots = 0, hp0 = g->iS.hp;
 for (int t = 0; t < secs * 30; t++)
  {
  int before = 0; for (int k = 0; k < MAXP; k++) before += g->iP[k].active && g->iP[k].pad == 5;
  g->iS.hp = 100 << 8; int h0 = g->iS.hp;
  g->Tick(33000, 0); g->iUi = UI_PLAY;
  int after = 0; for (int k = 0; k < MAXP; k++) after += g->iP[k].active && g->iP[k].pad == 5 && g->iP[k].kind != 2;
  if (after > before) shots += after - before;
  if (g->iS.hp < h0) hits++;
  }
 printf("%-48s feet %.0f: arrows %d, hits %d\n", what, (g->iS.py - 410 - off) / 256.0 * 100 / 1.42875, shots, hits);
 for (int i = 0; i < g->iS.ne; i++) { Ent& e = g->iS.e[i]; if (e.cell == 99) e.cell = 0; }
 (void)hp0; return hits;
 }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame();
 int fails = 0;
 if (Run("in the open below the tower (archers only)", 8200, -43300, 3300, 10, 1) == 0) fails++;
 if (Run("at the tower's chest (archers only)", 7580, -42445, 4892, 10, 1) != 0) fails++;
 if (Run("on the terrace by the doors (all)", -2700, -43400, 6141, 10, 0) == 0) fails++;
 printf("FAILS %d\n", fails); return fails; }
