#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// views of the Bitterroot (cm in the interior frame; up = on the upper floor) -> shots/shop.ppm
static u16 out[640*200];
static Game* g; static FILE* f;
static void shot(int x, int z, int up, int tx, int tz, int pitch)
    {
    g->EnterCell(CELL_SHOP, 100 * METER + x * METER / 100, 100 * METER + z * METER / 100, r3d_atan2(tx - x, tz - z));
    if (up) g->iS.py = 100 * METER + 320 * METER / 100 + 410;
    g->iS.pitch = pitch; g->iS.third = 0; g->iFadeUs = 0;
    for (int k = 0; k < 20; k++) g->Tick(16000, 0);
    g->iFadeUs = 0;
    g->Render(out, 640);
    for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
    }
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY; g->iHideView = 0;
 g->iR->SetOutputMode(2);
 f=fopen("shots/shop.ppm","wb"); fprintf(f,"P6 640 1200 255\n");
 shot(300, 820, 0, 600, 300, -20);      // from the front door: the lab, the desk, the gallery
 shot(500, 200, 0, 900, 450, -20);      // the apothecary at her desk, shelves behind
 shot(400, 300, 0, 670, 900, 10);       // the fireplace, herbs overhead
 shot(700, 600, 0, 50, 300, 20);        // the west side: gallery, railing, the doors below
 shot(150, 300, 1, 900, 500, -50);      // from the gallery over the shop
 shot(250, -40, 1, 100, -300, -40);     // her room
 fclose(f);}
