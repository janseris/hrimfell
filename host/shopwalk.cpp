#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// the Bitterroot: in by the street door, round the shop, up the stairs to the gallery and her room, down again, out
static Game* g;
int GameMerchantBuys(int m, int aId);
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
 int d = -1; for (int i = 0; i < g->NumDoors(); i++) if (g->Door(i).cell == CELL_SHOP) d = i;
 const DoorLink& a = g->Door(d);
 g->EnterCell(0, a.ox + ((r3d_sin(a.oyaw) * 2 * METER) >> 14), a.oz + ((r3d_cos(a.oyaw) * 2 * METER) >> 14), (a.oyaw + 512) & 1023); g->iFadeUs = 0;
 for (int k = 0; k < 50; k++) g->Tick(16000, KEY_FWD);
 g->FindDoor(); printf("street door: focus %d (want %d), ground at the door %d dm above the market\n", g->iDoorFocus, d, 0);
 if (g->iDoorFocus == d) g->UseDoor(d);
 printf("in cell %d at (%d,%d) cm\n", g->iS.cell, (g->iS.px - 100*METER)*100/METER, (g->iS.pz - 100*METER)*100/METER);
 static const int KA[] = { 300, 700,  250, 400,  420, 260,  440, 140,  590, 110,  590, -60,  700, -120 };   ipath("shop floor to the study", KA, 7, 2000);
 static const int KB[] = { 580, 120,  300, 300,  150, 500,  -70, 510,  -70, 478,  -220, 478,  -220, 845,  -70, 845,  -70, 600,  150, 600,  150, 100,  110, -60,  220, -150 };
 ipath("stairs up, gallery, into her room", KB, 13, 4000);
 static const int KC[] = { 110, 20,  150, 300,  150, 600,  -70, 600,  -70, 845,  -220, 845,  -220, 478,  -70, 478,  150, 500,  360, 480,  400, 760 };
 ipath("back down to the shop", KC, 11, 4000);
 static const int KD[] = { 360, 480,  150, 450,  150, 250 };   ipath("under the gallery (ground floor)", KD, 3, 1500);
 // brew at the lab: two blue mountain flowers + two wheat -> potions of minor healing
 g->AddItem(IT_BLUE_FLOWER, 2); g->AddItem(IT_WHEAT, 2);
 int pots0 = g->CountItem(IT_POT_MHEAL), lab = -1;
 for (int i = 0; i < g->iS.nprop; i++) if (g->iS.prop[i].type == PT_ALCHEMY) lab = i;
 g->UseProp(lab); printf("lab: ui %d (cook=%d) mode %d\n", g->iUi, UI_COOK, g->iCookMode);
 g->iListSel = 0;
 for (int k = 0; k < 6; k++) { g->Tick(16000, 0); g->Tick(16000, KEY_FIRE); }
 printf("brewed: potions of minor healing %d -> %d, flowers left %d, wheat left %d\n", pots0, g->CountItem(IT_POT_MHEAL), g->CountItem(IT_BLUE_FLOWER), g->CountItem(IT_WHEAT));
 g->iUi = UI_PLAY;
 int eira = -1; for (int i = 0; i < g->iS.ne; i++) if (g->iS.e[i].cell == CELL_SHOP) eira = i;
 if (eira >= 0) printf("her purse %d gold, buys potions %d, ingredients %d, swords %d\n", g->iS.mgold[MER_APOTHECARY], GameMerchantBuys(MER_APOTHECARY, IT_POT_HEAL), GameMerchantBuys(MER_APOTHECARY, IT_LAVENDER), GameMerchantBuys(MER_APOTHECARY, IT_IRON_SWORD));
 printf("the apothecary: %s at (%d,%d)\n", eira >= 0 ? "present" : "MISSING", eira >= 0 ? (g->iS.e[eira].x - 100*METER)*100/METER : 0, eira >= 0 ? (g->iS.e[eira].z - 100*METER)*100/METER : 0);
}
