#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// Highhall: in by the great doors, up the stairs, round the hall, onto the dais, up to the galleries and the war room,
// the kitchen and down to the storeroom, the court wizard's rooms
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
    printf("%-44s reached %d/%d at (%d,%d) cm, floor %+d cm %s\n", what, k, n, (g->iS.px - 100*METER)*100/METER, (g->iS.pz - 100*METER)*100/METER,
           (g->iS.py - 410 - 100*METER)*100/METER, k >= n ? "OK" : "STUCK");
    }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 // the people stand aside for the test
 for (int i = 0; i < g->iS.ne; i++) if (g->iS.e[i].cell == CELL_HIGHHALL) g->iS.e[i].cell = 99;
 int d = -1; for (int i = 0; i < g->NumDoors(); i++) if (g->Door(i).cell == CELL_HIGHHALL) d = i;
 const DoorLink& a = g->Door(d);
 g->EnterCell(0, a.ox + ((r3d_sin(a.oyaw) * 2 * METER) >> 14), a.oz + ((r3d_cos(a.oyaw) * 2 * METER) >> 14), (a.oyaw + 512) & 1023); g->iFadeUs = 0;
 for (int k = 0; k < 60; k++) g->Tick(16000, KEY_FWD);
 g->FindDoor(); printf("great doors: focus %d (want %d)\n", g->iDoorFocus, d);
 if (g->iDoorFocus == d) g->UseDoor(d);
 printf("in cell %d at (%d,%d) cm\n", g->iS.cell, (g->iS.px - 100*METER)*100/METER, (g->iS.pz - 100*METER)*100/METER);
 static const int KA[] = { 0, 400,  0, 1400,  0, 2800,  -680, 2900,  -680, 4150,  -250, 4350,  0, 4650 };
 ipath("entrance, great stairs, west aisle, dais", KA, 7, 4000);
 static const int KB[] = { -250, 4350,  -680, 4270,  -680, 3990,  -1350, 3990,  -1350, 5000,  -1000, 5000,  0, 5000,  0, 5200,  -500, 5450 };
 ipath("west stairs, balcony, war room", KB, 9, 4000);
 static const int KC[] = { 0, 5200,  0, 5000,  -1000, 5000,  -1000, 4700,  -1000, 2100,  -1000, 4700,  -1000, 5000,  -1350, 5000,  -1350, 3990,  -680, 3990 };
 ipath("the west gallery and back down", KC, 10, 6000);
 static const int KD[] = { -950, 3900,  -950, 3020,  -1700, 3020,  -2000, 2850,  -2600, 2850,  -2900, 3300,  -2900, 4300,  -2900, 4800 };
 ipath("kitchen, down to the storeroom", KD, 8, 4000);
 static const int KE[] = { -2900, 4300,  -2900, 3300,  -2600, 2850,  -2000, 2850,  -1700, 3020,  -950, 3020,  -680, 2900,  680, 2900,  1700, 2880,  2200, 2850,  2500, 2850,  2650, 3110,  2950, 3110 };
 ipath("back, across the hall, the wizard's bedroom", KE, 13, 6000);
 static const int KF[] = { 2650, 3110,  2550, 3500,  2650, 3660,  2950, 3900 };
 ipath("the wizard's study", KF, 4, 2000);
}
