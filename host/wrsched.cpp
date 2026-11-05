#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// Whiterun's outskirts' people: out by day, home (or away) by night, on a floor and clear of walls when home
const char* GameNamedName(int i);
static Game* g;
static int fails;
static void Show(const char* when)
    {
    printf("%s (hour %d):\n", when, g->iS.timeOfDay / 60000);
    for (int i = 0; i < g->iS.ne; i++)
        {
        const Ent& e = g->iS.e[i];
        if (e.type != E_VILLAGER || e.role != ROLE_NAMED || e.pad1 < 23 || e.pad1 > 28) continue;
        int bl = (e.cell && e.cell < 250) ? g->BlockedAt(e.cell, e.x, e.z, 25 * METER / 100, e.y) : 0;
        int fl = (e.cell && e.cell < 250) ? g->FloorAt(e.cell, e.x, e.z, e.y) : g->iR->GroundAt(e.x, e.z);
        printf("  %-18s cell %3d  skyrim (%d,%d), feet %d, floor %d%s\n", GameNamedName(e.pad1), e.cell,
               (int)((long long)e.x * 10000 / 36576 - 233472), (int)((long long)e.z * 10000 / 36576 - 176128), e.y, fl, bl ? "  BLOCKED" : "");
        if (bl || (e.cell && e.cell < 250 && abs(fl - e.y) > METER / 4)) fails++;
        }
    }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 Show("new game");
 g->EnterCell(CELL_RWINN, 100 * METER, 100 * METER, 0);
 while (g->iS.timeOfDay / 60000 != 21) g->WaitHour();
 for (int k = 0; k < 30; k++) g->Tick(16000, 0);
 Show("evening");
 while (g->iS.timeOfDay / 60000 != 9) g->WaitHour();
 for (int k = 0; k < 30; k++) g->Tick(16000, 0);
 Show("morning");
 printf("FAILS %d\n", fails);
}
