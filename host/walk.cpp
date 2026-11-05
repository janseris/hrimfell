#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// walks the Vindgard plan: every flight of stairs and the approach road, steering through waypoints (metres)
static int Wx(int m){ return CITY_CX*CELL+CELL/2+m*METER; } static int Wz(int m){ return CITY_CZ*CELL+CELL/2+m*METER; }
static Game* g;
static int h0;
// walk from the first point through the others; report the last waypoint reached and the height there
static void path(const char* what, const int* p, int n, int maxTicks)
    {
    g->EnterCell(0, Wx(p[0]), Wz(p[1]), 0); g->iFadeUs = 0;
    int k = 1, t;
    for (t = 0; t < maxTicks && k < n; t++)
        {
        int dx = Wx(p[k * 2]) - g->iS.px, dz = Wz(p[k * 2 + 1]) - g->iS.pz;
        if ((dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4) < (METER >> 4) * (METER >> 4)) { k++; continue; }
        g->iS.yaw = r3d_atan2(dx, dz);
        g->Tick(16000, KEY_FWD);
        }
    printf("%-30s reached %d/%d at (%d,%d) m, height %+d dm %s\n", what, k, n - 1, (g->iS.px - Wx(0)) / METER, (g->iS.pz - Wz(0)) / METER,
           (g->iS.py - 410 - h0) * 10 / METER, k >= n ? "OK" : "STUCK");
    }
// the same inside an interior (cm in the interior frame)
static void ipath(const char* what, const int* p, int n, int maxTicks)
    {
    int k = 0, t;
    for (t = 0; t < maxTicks && k < n; t++)
        {
        int dx = 100 * METER + p[k * 2] * METER / 100 - g->iS.px, dz = 100 * METER + p[k * 2 + 1] * METER / 100 - g->iS.pz;
        if ((dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4) < (METER / 2 >> 4) * (METER / 2 >> 4)) { k++; continue; }
        g->iS.yaw = r3d_atan2(dx, dz);
        g->Tick(16000, KEY_FWD);
        }
    printf("%-40s reached %d/%d at (%d,%d) cm, floor %+d cm %s\n", what, k, n, (g->iS.px - 100*METER)*100/METER, (g->iS.pz - 100*METER)*100/METER,
           (g->iS.py - 410 - 100*METER)*100/METER, k >= n ? "OK" : "STUCK");
    }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 h0 = g->iR->GroundAt(Wx(-4), Wz(-14));
 static const int KWide[] = { -4, -8,  -4, -2,  -4, 12 };                    path("wide stairs to the Wind District", KWide, 3, 900);
 static const int KWest[] = { -43, -4,  -43, 2,  -43, 12 };                  path("small west stairs", KWest, 3, 900);
 static const int KInn[] = { -6, -9,  0, -9,  3, -9 };                        path("market up to the inn", KInn, 3, 600);
 static const int KHigh[22] = { -4, 30,  -4, 36,  -4, 51,  0, 52,  0, 58,  1, 61,  8, 65,  12, 69,  14, 70,  18, 73,  20, 76 };
 path("four flights up to Highhall", KHigh, 11, 3000);
 static const int KJor[] = { 10, 25,  15, 25,  24, 25 };                      path("bridge-stair to Jorrvaskr", KJor, 3, 600);
 static const int KForge[] = { 43, 30,  43, 34,  43, 42 };                    path("stairs up to the Skyforge", KForge, 3, 600);
 static const int KApproach[] = { -86, -100,  -86, -80,  -89, -68,  -97, -50,  -109, -32,  -112, -26,  -113, -16,  -114, -6,  -115, 5,
                                  -115, 18,  -114, 30,  -110, 36,  -103, 38,  -87, 37,  -83, 27,  -82, 16,  -82, 2,  -81, -10,  -70, -12,  -55, -12 };
 path("the approach road into the city", KApproach, 20, 8000);
 static const int KStreet[] = { -55, -12,  -40, -12,  -20, -13,  -8, -12 };  path("main street to the market", KStreet, 4, 1500);

 {
 // the Hall of the Dead and its catacombs (skeletons removed so the walk is not a fight)
 for (int i = 0; i < g->iS.ne; i++) if (g->iS.e[i].type == E_SKELETON) g->iS.e[i].cell = 99;
 int d1 = -1, d2 = -1;
 for (int i = 0; i < g->NumDoors(); i++) { if (g->Door(i).cell == CELL_HALLDEAD) d1 = i; if (g->Door(i).cell == CELL_CATACOMBS) d2 = i; }
 const DoorLink& a = g->Door(d1);
 g->EnterCell(0, a.ox + ((r3d_sin(a.oyaw) * 2 * METER) >> 14), a.oz + ((r3d_cos(a.oyaw) * 2 * METER) >> 14), (a.oyaw + 512) & 1023); g->iFadeUs = 0;
 for (int k = 0; k < 60; k++) g->Tick(16000, KEY_FWD);
 g->FindDoor(); printf("street door: focus %d (want %d)\n", g->iDoorFocus, d1);
 if (g->iDoorFocus == d1) g->UseDoor(d1);
 printf("in cell %d at (%d,%d) cm\n", g->iS.cell, (g->iS.px - 100*METER)*100/METER, (g->iS.pz - 100*METER)*100/METER);
 static const int KHall[] = { 0, -380,  -100, -300,  -100, 600,  0, 640 };
 ipath("hall: up the middle aisle", KHall, 4, 1500);
 g->FindDoor(); printf("iron door: focus %d (want %d)\n", g->iDoorFocus, d2);
 if (g->iDoorFocus == d2) g->UseDoor(d2);
 printf("in cell %d at (%d,%d) cm floor %d\n", g->iS.cell, (g->iS.px - 100*METER)*100/METER, (g->iS.pz - 100*METER)*100/METER, (g->iS.py - 410 - 100*METER)*100/METER);
 static const int KCat[] = { 0, 0,  680, 100,  680, 300,  680, 900,  680, 1100,  860, 1200,  1300, 1200 };
 ipath("catacombs: east stairs to the amulet room", KCat, 7, 3000);
 for (int i = 0; i < g->iS.nprop; i++) if (g->iS.prop[i].type == PT_DOOR && g->iS.prop[i].cell == CELL_CATACOMBS) { g->iS.prop[i].state = 0; g->iS.prop[i].anim = 4; }
 static const int KCat2[] = { 1300, 1200,  860, 1200,  400, 980,  -260, 980,  -260, 850,  -260, 300,  -260, 150,  -300, 100,  -600, 110,  -800, 110 };
 ipath("catacombs: west stairs up, the side room", KCat2, 9, 4000);
 static const int KCat3[] = { -300, 100,  -260, 250,  -260, 1000,  -640, 1000,  -640, 600 };
 ipath("catacombs: the gem alcove", KCat3, 5, 3000);
 for (int i = 0; i < g->iS.ne; i++) if (g->iS.e[i].type == E_SKELETON) g->iS.e[i].cell = CELL_CATACOMBS;
 }
 for (int t = 0; t < 3; t += 2)
     {
     g->EnterCell(0, g->iTentX[t] + ((r3d_sin(g->iTentYaw[t])*4*METER)>>14), g->iTentZ[t] + ((r3d_cos(g->iTentYaw[t])*4*METER)>>14), (g->iTentYaw[t]+512)&1023); g->iFadeUs=0;
     for(int k=0;k<200;k++) g->Tick(16000, KEY_FWD);
     int dx=(g->iS.px-g->iTentX[t])*100/METER, dz=(g->iS.pz-g->iTentZ[t])*100/METER; printf("in tent %d: %d,%d cm from its middle\n", t, dx, dz);
     }
}
