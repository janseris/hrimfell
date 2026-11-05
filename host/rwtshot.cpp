#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// views of the Riverwood Trader (cm in the interior frame; up = on the upper floor) -> shots/rwt.ppm
static u16 out[640*200];
static Game* g; static FILE* f;
static void shot(int x, int z, int up, int tx, int tz, int pitch)
    {
    g->EnterCell(CELL_RWTRADER, 100 * METER + x * METER / 100, 100 * METER + z * METER / 100, r3d_atan2(tx - x, tz - z));
    g->iUi = UI_PLAY;
    if (up) g->iS.py = 100 * METER + 343 * METER / 100 + 410;
    g->iS.pitch = pitch; g->iS.third = 0; g->iFadeUs = 0;
    for (int k = 0; k < 20; k++) g->Tick(16000, 0);
    g->iFadeUs = 0;
    g->Render(out, 640);
    for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
    }
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY; g->iHideView = 0;
 g->iR->SetOutputMode(2); g->iS.bfb[BF_LUCAN] = 1; g->iS.timeOfDay = 13 * 60000;
 f=fopen("shots/rwt.ppm","wb"); fprintf(f,"P6 640 %d 255\n", argc > 1 ? 400 : 1400);
 if (argc > 1) { fprintf(f,""); shot(-560, -182, 1, -208, -182, -60); shot(-208, 120, 1, -208, -400, -50); fclose(f); return 0; }
 shot(-600, -480, 0, -150, -40, -10);    // like Skyrim's view: the hearth on the left, the counter, Lucan
 shot(-273, -600, 0, -273, 60, 0);       // from the door: the round hearth opposite
 shot(-350, 0, 0, 200, -450, -15);       // across the shop: the counter, the east shelves, the dresser
 shot(-100, -450, 0, -880, -150, 10);    // the stairs on the west wall, barrels and firewood under them
 shot(-200, 40, 1, 60, -620, -25);       // upstairs: the long table, the stag heads, the balcony door
 shot(-600, -350, 1, 280, -250, -5);     // upstairs east: the lattice windows, pelts, round table, beds
 shot(-150, -330, 0, 46, -176, 0);       // Lucan at his counter
 fclose(f);}
