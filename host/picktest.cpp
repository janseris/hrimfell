// plants picked come back as Skyrim's cells do: 10 days after the player last had their cell loaded
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main()
 {
 int n = Game::MemNeeded(); unsigned char* mem = (unsigned char*)calloc(1, n); Game* g = (Game*)calloc(1, sizeof(Game));
 g->Init(mem); g->NewGame(); g->iUi = UI_PLAY; g->iFadeUs = 0;
 int fails = 0, i, best = -1, bd = 1 << 30;
 for (i = 0; i < g->iFloraN; i++) { int x, z; g->FloraPos(i, x, z); int d = abs(x - g->iS.px) + abs(z - g->iS.pz); if (g->FloraItem(g->iFloraEnt[i * 6 + 4]) && d < bd) { bd = d; best = i; } }
 printf("flora %d, nearest pickable %d at %d m\n", g->iFloraN, best, bd / METER);
 g->TakeFlora(best);
 int taken = (g->iFloraTaken[best >> 3] >> (best & 7)) & 1;
 printf("picked: %d, list %d\n", taken, g->iS.npick); if (!taken) fails++;
 // 11 days nearby: still gone (its cell loaded all the while)
 g->iS.day += 11; g->UpdatePicks();
 taken = (g->iFloraTaken[best >> 3] >> (best & 7)) & 1; printf("11 days staying near: taken %d (want 1)\n", taken); if (!taken) fails++;
 // away (indoors) 9 days: still gone; 11: back
 g->iS.cell = CELL_RWTRADER; g->iS.day += 9; g->UpdatePicks();
 taken = (g->iFloraTaken[best >> 3] >> (best & 7)) & 1; printf("9 days away: taken %d (want 1)\n", taken); if (!taken) fails++;
 g->iS.day += 2; g->UpdatePicks();
 taken = (g->iFloraTaken[best >> 3] >> (best & 7)) & 1; printf("11 days away: taken %d (want 0), list %d\n", taken, g->iS.npick); if (taken) fails++;
 printf("FAILS %d\n", fails);
 return fails;
 }
