#include "game.h"
#include <stdio.h>
#include <stdlib.h>
const ItemDef& GameItem(int aId);
// skeletons in the catacombs: they notice you, close in and strike; you cut them down; they leave bone meal and rise again
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 g->EnterCell(CELL_CATACOMBS, 100*METER, 100*METER - 30*METER/100, 0); g->iFadeUs = 0;
 int hp0 = g->iS.hp;
 for (int t = 0; t < 300; t++) g->Tick(16000, 0);
 for (int i = 0; i < g->iS.ne; i++) { Ent& e = g->iS.e[i]; if (e.type != E_SKELETON) continue;
   printf("skeleton %d: alive %d aggro %d at (%d,%d) cm floor %d cm, %d cm from you\n", e.var, e.alive, e.aggro, (e.x-100*METER)*100/METER, (e.z-100*METER)*100/METER, (e.y-100*METER)*100/METER,
          r3d_isqrt(((e.x-g->iS.px)>>4)*((e.x-g->iS.px)>>4)+((e.z-g->iS.pz)>>4)*((e.z-g->iS.pz)>>4))*16*100/METER); }
 printf("player hp %d -> %d after 5 s standing\n", hp0 >> 8, g->iS.hp >> 8);
 g->iS.hp = g->iS.hpMax << 8;
 // fight: face the nearest living skeleton and swing
 int kills = 0;
 for (int t = 0; t < 3000 && kills < 2; t++)
     {
     int best = -1, bd = 1 << 30;
     for (int i = 0; i < g->iS.ne; i++) { Ent& e = g->iS.e[i]; if (e.type != E_SKELETON || !e.alive || e.cell != g->iS.cell) continue;
        int dx = (e.x - g->iS.px) >> 4, dz = (e.z - g->iS.pz) >> 4; if (dx*dx+dz*dz < bd) { bd = dx*dx+dz*dz; best = i; } }
     if (best < 0) break;
     Ent& e = g->iS.e[best];
     g->iS.yaw = r3d_atan2(e.x - g->iS.px, e.z - g->iS.pz);
     g->iS.hp = g->iS.hpMax << 8; g->iS.sp = g->iS.spMax << 8; g->iS.drawn = 1;   // keep the tester standing, sword out
     g->Tick(16000, (t & 8) ? KEY_FIRE : 0);
     if (!e.alive) { kills++; printf("skeleton %d down after %d ticks; dead frame pile; looting\n", e.var, t); g->GiveLoot(e); }
     }
 for (int i = 0; i < g->iS.ninv; i++) if (g->iS.inv[i].id == IT_BONE_MEAL || g->iS.inv[i].id == IT_BARROW_BLADE) printf("inventory: %s x%d\n", GameItem(g->iS.inv[i].id).name, g->iS.inv[i].count);
 g->iS.day += 10; g->ReviveSkeletons();
 int alive = 0; for (int i = 0; i < g->iS.ne; i++) if (g->iS.e[i].type == E_SKELETON && g->iS.e[i].alive) alive++;
 printf("10 days later: %d skeletons standing\n", alive);
}
