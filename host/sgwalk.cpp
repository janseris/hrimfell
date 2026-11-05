#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// the Sleeping Giant Inn: in by the porch door, round the hall, behind the bar, into the room for rent (its door),
// out again; Delphine, Orgnar, Embry; renting the bed
static Game* g;
static void upath(const char* what, const int* p, int n, int maxTicks)
    {
    int k = 0, t;
    for (t = 0; t < maxTicks && k < n; t++)
        {
        int dx = 100 * METER + p[k * 2] * 143 / 100 * METER / 100 - g->iS.px, dz = 100 * METER + p[k * 2 + 1] * 143 / 100 * METER / 100 - g->iS.pz;
        if ((dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4) < (METER / 3 >> 4) * (METER / 3 >> 4)) { k++; continue; }
        g->iS.yaw = r3d_atan2(dx, dz);
        g->Tick(16000, KEY_FWD);
        }
    printf("%-40s reached %d/%d at (%d,%d) units %s\n", what, k, n, (g->iS.px - 100*METER)*100/METER*100/143, (g->iS.pz - 100*METER)*100/METER*100/143, k >= n ? "OK" : "STUCK");
    }
static const char* said() { static char b[300]; b[0] = 0; for (int i = MAXMSG - 1; i >= 0; i--) if (g->iMsg[i][0]) { strcat(b, g->iMsg[i]); strcat(b, " / "); } for (int i = 0; i < MAXMSG; i++) g->iMsg[i][0] = 0; return b; }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 int d = -1; for (int i = 0; i < g->NumDoors(); i++) if (g->Door(i).cell == CELL_RWINN) d = i;
 const DoorLink& a = g->Door(d);
 g->EnterCell(0, a.ox + ((r3d_sin(a.oyaw) * 2 * METER) >> 14), a.oz + ((r3d_cos(a.oyaw) * 2 * METER) >> 14), (a.oyaw + 512) & 1023); g->iFadeUs = 0;
 for (int k = 0; k < 40; k++) g->Tick(16000, KEY_FWD);
 g->FindDoor(); printf("porch door: focus %d (want %d)\n", g->iDoorFocus, d);
 if (g->iDoorFocus == d) g->UseDoor(d);
 printf("in cell %d at (%d,%d) units\n", g->iS.cell, (g->iS.px - 100*METER)*100/METER*100/143, (g->iS.pz - 100*METER)*100/METER*100/143);
 static const int KA[] = { -256, -150,  0, 0,  200, 0,  200, -200,  -200, -180,  -800, -185,  -1050, -185,  -1080, 0,  -1080, 200,  -800, 195,  -300, 200,  -100, 200 };
 upath("round the hall", KA, 12, 6000);
 static const int KB[] = { -800, -190,  -1020, -190,  -1020, -340,  -1300, -340,  -1300, -100,  -1300, 100 };   upath("behind the bar", KB, 6, 3000);
 static const int KC[] = { -1300, -340,  -1020, -340,  -1020, -190,  -768, -190,  -768, -330 };   upath("to the room for rent", KC, 5, 3000);
 // closed, the door stops you
 { int x0 = g->iS.px, z0 = g->iS.pz; static const int KX[] = { -768, -520 }; upath("through the shut door?", KX, 1, 300);
   printf(" -> %s\n", (g->iS.pz - 100*METER)*100/METER*100/143 < -440 ? "WENT THROUGH (wrong)" : "stopped by it"); g->iS.px = x0; g->iS.pz = z0; }
 // its door: open it
 int door = -1, best = 1 << 30;
 for (int i = 0; i < g->iS.nprop; i++) { const Prop& p = g->iS.prop[i]; if (p.cell == CELL_RWINN && p.type == PT_DOOR) { int dd = abs(p.x - g->iS.px) + abs(p.z - g->iS.pz); if (dd < best) { best = dd; door = i; } } }
 g->UseProp(door); for (int k = 0; k < 60; k++) g->Tick(16000, 0);
 printf("its door anim %d\n", g->iS.prop[door].anim);
 static const int KD[] = { -768, -500,  -720, -650 };   upath("into the room", KD, 2, 2000);
 int bed = -1; for (int i = 0; i < g->iS.nprop; i++) { const Prop& p = g->iS.prop[i]; if (p.cell == CELL_RWINN && p.type == PT_BED && p.count == 1) bed = i; }
 g->iS.gold = 5; g->UseProp(bed); printf("rent with 5 gold: %s\n", said());
 g->iS.gold = 50; g->UseProp(bed); printf("rent with 50 gold: %s gold now %d, ui %d (wait %d)\n", said(), g->iS.gold, g->iUi, UI_WAIT);
 g->iUi = UI_PLAY; g->iSleeping = 0;
 static const int KE[] = { -768, -500,  -768, -190,  -256, -190,  -256, -360 };   upath("back to the front door", KE, 4, 3000);
 g->FindDoor(); printf("front door: focus %d\n", g->iDoorFocus);
 if (g->iDoorFocus == d) g->UseDoor(d);
 printf("out in cell %d, %d m from the porch door\n", g->iS.cell, (abs(g->iS.px - a.ox) + abs(g->iS.pz - a.oz)) / METER);
 int del = -1, org = -1, emb = -1;
 for (int i = 0; i < g->iS.ne; i++) { const Ent& e = g->iS.e[i]; if (e.cell != CELL_RWINN) continue; if (e.role == ROLE_NAMED && e.pad1 == 19) del = i; if (e.role == ROLE_NAMED && e.pad1 == 20) emb = i; if (e.role == ROLE_MERCHANT && e.pad1 == MER_RWINN) org = i; }
 printf("Delphine %d, Orgnar %d (purse %d), Embry %d\n", del, org, g->iS.mgold[MER_RWINN], emb);
 // the secret room
 g->Teleport(25); printf("secret room: cell %d floor %d cm\n", g->iS.cell, (g->iS.py - 410 - 100 * METER) * 100 / METER);
 static const int KF[] = { 100, 1000,  0, 800,  -200, 800,  -560, 800 };   upath("up the passage to the wardrobe", KF, 4, 3000);
 printf(" floor %d cm\n", (g->iS.py - 410 - 100 * METER) * 100 / METER);
}
