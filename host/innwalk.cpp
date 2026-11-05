#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// the Gilded Stag: in by the front door, the common room, the bar, the back room, up to the rented room and its
// balcony, the kitchen and up its stairs to the staff rooms, out by the back door; renting the room
static Game* g;
static void ipath(const char* what, const int* p, int n, int maxTicks)
    {
    int k = 0, t;
    for (t = 0; t < maxTicks && k < n; t++)
        {
        int dx = 100 * METER + p[k * 2] * METER / 100 - g->iS.px, dz = 100 * METER + p[k * 2 + 1] * METER / 100 - g->iS.pz;
        if ((dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4) < (METER / 3 >> 4) * (METER / 3 >> 4)) { k++; continue; }
        g->iS.yaw = r3d_atan2(dx, dz);
        g->Tick(16000, KEY_FWD);
        }
    printf("%-46s reached %d/%d at (%d,%d) cm, floor %+d cm %s\n", what, k, n, (g->iS.px - 100*METER)*100/METER, (g->iS.pz - 100*METER)*100/METER,
           (g->iS.py - 410 - 100*METER)*100/METER, k >= n ? "OK" : "STUCK");
    }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 int d = -1, d2 = -1; for (int i = 0; i < g->NumDoors(); i++) if (g->Door(i).cell == CELL_INN) { if (d < 0) d = i; else d2 = i; }
 const DoorLink& a = g->Door(d);
 g->EnterCell(0, a.ox + ((r3d_sin(a.oyaw) * 2 * METER) >> 14), a.oz + ((r3d_cos(a.oyaw) * 2 * METER) >> 14), (a.oyaw + 512) & 1023); g->iFadeUs = 0;
 for (int k = 0; k < 50; k++) g->Tick(16000, KEY_FWD);
 g->FindDoor(); printf("front door: focus %d (want %d)\n", g->iDoorFocus, d);
 if (g->iDoorFocus == d) g->UseDoor(d);
 printf("in cell %d at (%d,%d) cm\n", g->iS.cell, (g->iS.px - 100*METER)*100/METER, (g->iS.pz - 100*METER)*100/METER);
 static const int KA[] = { 300, 870,  560, 870,  560, 420,  760, 420 };              ipath("common room to the bar", KA, 4, 2000);
 static const int KB[] = { 560, 420,  560, 640,  960, 820,  1120, 820,  1400, 850 };  ipath("into the back room", KB, 5, 2000);
 static const int KC[] = { 960, 820,  600, 1060,  600, 1180,  1000, 1180,  1100, 1170,  1100, 1050,  1350, 900,  1080, 930,  960, 925 };
 ipath("stairs up, the rented room, the balcony", KC, 9, 4000);
 // rent it and sleep
 int bed = -1; for (int i = 0; i < g->iS.nprop; i++) if (g->iS.prop[i].type == PT_BED && g->iS.prop[i].cell == CELL_INN && g->iS.prop[i].count == 1) bed = i;
 int gold0 = g->iS.gold; g->UseProp(bed);
 printf("rented bed: gold %d -> %d, ui %d (wait=%d)\n", gold0, g->iS.gold, g->iUi, UI_WAIT); g->iUi = UI_PLAY;
 int other = -1; for (int i = 0; i < g->iS.nprop; i++) if (g->iS.prop[i].type == PT_BED && g->iS.prop[i].cell == CELL_INN && g->iS.prop[i].count == 0) other = i;
 g->UseProp(other); printf("someone else's bed: ui %d (stays %d)\n", g->iUi, UI_PLAY);
 static const int KD[] = { 1080, 935,  1300, 900,  1100, 1050,  1100, 1170,  1000, 1180,  640, 1180,  570, 1150,  570, 1350,  760, 1450,  950, 1500,  950, 1980,  760, 1990,  600, 1990,  600, 1700 };
 ipath("down, the kitchen, its stairs, the innkeeper's room", KD, 14, 6000);
 static const int KE[] = { 600, 1990,  760, 1990,  760, 2200,  820, 2250 };          ipath("the serving maid's room", KE, 4, 2000);
 static const int KF[] = { 760, 2200,  760, 1990,  960, 1990,  960, 1960,  960, 1580,  820, 1560,  790, 1560,  790, 1800,  560, 1860 };  ipath("back down, to the back door", KF, 9, 4000);
 g->FindDoor(); printf("back door: focus %d (want %d)\n", g->iDoorFocus, d2);
 if (g->iDoorFocus == d2) g->UseDoor(d2);
 printf("outside: cell %d at (%d,%d) m from the city centre\n", g->iS.cell, (g->iS.px - (CITY_CX*CELL+CELL/2))/METER, (g->iS.pz - (CITY_CZ*CELL+CELL/2))/METER);
 int people = 0; for (int i = 0; i < g->iS.ne; i++) if (g->iS.e[i].cell == CELL_INN) people++;
 printf("people in the inn: %d\n", people);
}
