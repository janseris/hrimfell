#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// the Riverwood Trader: in by the street door (the argument scene), round the shop and behind the counter, up the
// stairs, round the upper floor, down again and out; then the Golden Claw with Lucan
static Game* g;
const char* GameDunDlgLine(int aId, int aPage);
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
static const char* said() { static char b[300]; b[0] = 0; for (int i = MAXMSG - 1; i >= 0; i--) { strcat(b, g->iMsg[i]); strcat(b, " / "); } for (int i = 0; i < MAXMSG; i++) g->iMsg[i][0] = 0; return b; }
static void talkall()
    {
    while (g->iUi == UI_DIALOG) { if (g->iDlgId >= 100) printf("   %s\n", GameDunDlgLine(g->iDlgId - 100, g->iDlgPage)); g->DialogNext(); }
    }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 int d = -1; for (int i = 0; i < g->NumDoors(); i++) if (g->Door(i).cell == CELL_RWTRADER) d = i;
 const DoorLink& a = g->Door(d);
 g->EnterCell(0, a.ox + ((r3d_sin(a.oyaw) * 2 * METER) >> 14), a.oz + ((r3d_cos(a.oyaw) * 2 * METER) >> 14), (a.oyaw + 512) & 1023); g->iFadeUs = 0;
 for (int k = 0; k < 40; k++) g->Tick(16000, KEY_FWD);
 g->FindDoor(); printf("street door: focus %d (want %d)\n", g->iDoorFocus, d);
 if (g->iDoorFocus == d) g->UseDoor(d);
 printf("in cell %d at (%d,%d) cm, ui %s\n", g->iS.cell, (g->iS.px - 100*METER)*100/METER, (g->iS.pz - 100*METER)*100/METER, g->iUi == UI_DIALOG ? "dialogue (the argument)" : "play");
 talkall();
 static const int KA[] = { -273, -500,  -150, -430,  150, -420,  200, -300,  200, -150,  150, -60,  100, -60 };  ipath("to the counter, round behind it", KA, 7, 3000);
 static const int KB[] = { 150, -60,  200, -150,  200, -300,  -200, -450,  -700, -480,  -857, -480,  -857, -400,  -857, -20,  -857, 50,  -700, 40 };
 ipath("across the shop and up the stairs", KB, 10, 4000);
 static const int KC[] = { -600, 0,  -430, -200,  -430, -420,  -420, -560,  -160, -560,  -150, -430,  -20, -300,  100, -280 };   ipath("round the upper floor", KC, 8, 4000);
 static const int KD[] = { -20, -300,  -150, -430,  -160, -560,  -420, -560,  -430, -420,  -430, -200,  -600, 0,  -700, 30,  -857, 50,  -857, -20,  -857, -440,  -857, -520,  -273, -560 };
 ipath("back down to the shop", KD, 13, 6000);
 static const int KE[] = { -350, -350,  -560, -200 };   ipath("under the stairs' end (ground)", KE, 2, 1500);
 static const int KF[] = { -273, -560,  -273, -640 };   ipath("to the door", KF, 2, 1500);
 g->FindDoor(); printf("inner door: focus %d\n", g->iDoorFocus);
 if (g->iDoorFocus == d) g->UseDoor(d);
 printf("out in cell %d, %d m from the street door\n", g->iS.cell, (abs(g->iS.px - a.ox) + abs(g->iS.pz - a.oz)) / METER);
 // the Golden Claw
 int lucan = -1, cam = -1;
 for (int i = 0; i < g->iS.ne; i++) { const Ent& e = g->iS.e[i]; if (e.cell != CELL_RWTRADER) continue; if (e.role == ROLE_MERCHANT && e.pad1 == MER_RWTRADER) lucan = i; if (e.role == ROLE_NAMED && e.pad1 == 18) cam = i; }
 printf("Lucan %s, Camilla %s\n", lucan >= 0 ? "in the shop" : "MISSING", cam >= 0 ? "in the shop" : "MISSING");
 printf("talk to Lucan:\n"); int r = g->LucanTalk(lucan); talkall(); printf(" quest stage %d (r %d)\n", g->iS.bfb[BF_QCLAW], r);
 printf("talk again (nag, then his wares):\n"); r = g->LucanTalk(lucan); printf(" r %d: %s\n", r, said());
 printf("Camilla:\n"); g->CamillaTalk(); talkall();
 g->AddItem(IT_GOLDEN_CLAW, 1); g->iS.bfb[BF_QCLAW] = 50;
 r = g->LucanTalk(lucan); printf("with the claw, door not opened yet: r %d: %s\n", r, said());
 g->iS.bfb[BF_CLAWDOOR] = 1; int gold = g->iS.gold;
 printf("with the claw, the barrow cleared:\n"); g->LucanTalk(lucan); talkall();
 int claw = 0; for (int i = 0; i < g->iS.nprop; i++) if (g->iS.prop[i].type == PT_ITEM && g->iS.prop[i].item == IT_GOLDEN_CLAW && g->iS.prop[i].cell == CELL_RWTRADER) claw = 1;
 printf(" stage %d, gold %+d, claw kept %d, claw on the counter %d\n", g->iS.bfb[BF_QCLAW], g->iS.gold - gold, g->CountItem(IT_GOLDEN_CLAW), claw);
 r = g->LucanTalk(lucan); printf("after: r %d: %s\n", r, said());
}
