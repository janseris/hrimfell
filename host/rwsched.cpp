#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// Riverwood's people: out by day, home by night (and standing on a floor, clear of walls, when home)
const char* GameNamedName(int i);
static Game* g;
static int fails;
static void Show(const char* when)
    {
    printf("%s (hour %d):\n", when, g->iS.timeOfDay / 60000);
    for (int i = 0; i < g->iS.ne; i++)
        {
        const Ent& e = g->iS.e[i];
        if (e.type != E_VILLAGER || e.role != ROLE_NAMED || !((e.pad1 >= 11 && e.pad1 <= 17) || e.pad1 == 21 || e.pad1 == 22)) continue;
        int bl = e.cell ? g->BlockedAt(e.cell, e.x, e.z, 25 * METER / 100, e.y) : 0;
        int fl = e.cell ? g->FloorAt(e.cell, e.x, e.z, e.y) : e.y;
        printf("  %-10s cell %2d  (%d,%d) units, feet %d cm, floor there %d cm%s\n", GameNamedName(e.pad1), e.cell,
               (e.x - 100 * METER) * 70 / METER, (e.z - 100 * METER) * 70 / METER, (e.y - 100 * METER) * 100 / METER, (fl - 100 * METER) * 100 / METER, bl ? "  BLOCKED" : "");
        if (bl || (e.cell && abs(fl - e.y) > METER / 4)) fails++;
        }
    }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 Show("new game");
 g->EnterCell(CELL_RWINN, 100 * METER, 100 * METER, 0);           // the player away in the inn
 while (g->iS.timeOfDay / 60000 != 21) g->WaitHour();
 for (int k = 0; k < 30; k++) g->Tick(16000, 0);
 Show("evening");
 while (g->iS.timeOfDay / 60000 != 9) g->WaitHour();
 Show("morning");
 // standing by the forge at 19:55, the smith stays till the player turns away; through a door, he is home
 { int x = 0, z = 0; for (int i = 0; i < g->iS.ne; i++) if (g->iS.e[i].role == ROLE_NAMED && g->iS.e[i].pad1 == 11) { x = g->iS.e[i].x; z = g->iS.e[i].z; }
   g->EnterCell(0, x + 3 * METER, z, 0); g->iS.timeOfDay = 19 * 60000 + 55000; for (int k = 0; k < 400; k++) g->Tick(16000, 0);
   Show("20:00 by the forge"); int d = -1; for (int i = 0; i < g->NumDoors(); i++) if (g->Door(i).cell == CELL_RWALVOR) d = i;
   g->UseDoor(d); Show("through Alvor's door"); }
 printf("FAILS %d\n", fails);
}
