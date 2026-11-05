#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// views of the Hall of the Dead and its catacombs (cm in the interior frame) -> shots/crypt.ppm
static u16 out[640*200];
static Game* g; static FILE* f;
static void shot(int cell, int x, int z, int tx, int tz, int pitch)
    {
    int ox = 100 * METER, oz = 100 * METER;
    g->EnterCell(cell, ox + x * METER / 100, oz + z * METER / 100, r3d_atan2(tx - x, tz - z));
    g->iS.pitch = pitch; g->iS.third = 0; g->iFadeUs = 0;
    for (int k = 0; k < 20; k++) g->Tick(16000, 0);
    g->iFadeUs = 0;
    g->Render(out, 640);
    for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
    }
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY; g->iHideView = 0;
 g->iR->SetOutputMode(2);
 // freeze the skeletons where they stand (a pose to look at), one fallen
 for (int i = 0; i < g->iS.ne; i++) if (g->iS.e[i].type == E_SKELETON) { g->iS.e[i].aggro = 0; }
 f=fopen("shots/crypt.ppm","wb"); fprintf(f,"P6 640 1800 255\n");
 shot(CELL_HALLDEAD, 0, -440, 0, 700, -20);          // the hall from the door: middle aisle, pillars, the iron door
 shot(CELL_HALLDEAD, 150, -100, 400, 640, -40);      // the chapel and the shrine
 shot(CELL_HALLDEAD, -700, 350, -1000, 550, -40);    // the priest's room
 shot(CELL_CATACOMBS, 0, -40, 600, 100, -30);        // the corridor: the first skeleton, urn niches
 shot(CELL_CATACOMBS, 680, 150, 680, 1100, -80);     // down the east stairs to the second skeleton
 shot(CELL_CATACOMBS, 700, 1100, -400, 1500, -60);   // the lower crypt: coffins, braziers
 shot(CELL_CATACOMBS, 1000, 1200, 1500, 1150, -40);  // the amulet room
 // close: a skeleton standing (attacking) and one fallen
 for (int i = 0; i < g->iS.ne; i++) { Ent& e = g->iS.e[i]; if (e.type != E_SKELETON) continue;
   if (e.var == 0) { e.x = 100*METER + 300*METER/100; e.z = 100*METER + 100*METER/100; e.yaw = 768; e.atk = 400000; }
   if (e.var == 1) { e.x = 100*METER + 300*METER/100; e.z = 100*METER + 240*METER/100; g->KillEnt(e); } }
 shot(CELL_CATACOMBS, 50, 120, 300, 170, -30);
 shot(CELL_CATACOMBS, 150, 170, 300, 170, -40);
 fclose(f);}
