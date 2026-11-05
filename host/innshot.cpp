#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// views of the Bitterroot (cm in the interior frame; up = on the upper floor) -> shots/inn.ppm
static u16 out[640*200];
static Game* g; static FILE* f;
static void shot(int x, int z, int up, int tx, int tz, int pitch)
    {
    g->EnterCell(CELL_INN, 100 * METER + x * METER / 100, 100 * METER + z * METER / 100, r3d_atan2(tx - x, tz - z));
    if (up) g->iS.py = 100 * METER + 320 * METER / 100 + 410;
    g->iS.pitch = pitch; g->iS.third = 0; g->iFadeUs = 0;
    for (int k = 0; k < 20; k++) g->Tick(16000, 0);
    g->iFadeUs = 0;
    g->Render(out, 640);
    for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
    }
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY; g->iHideView = 0;
 g->iR->SetOutputMode(2);
 f=fopen("shots/inn.ppm","wb"); fprintf(f,"P6 640 1400 255\n");
 shot(120, 870, 0, 760, 870, 0);       // from the front door: the fire pit, the benches, the bard
 shot(760, 700, 0, 760, 100, -20);     // the bar: Gunnhild, the counter, the shelves, the hanging game
 shot(300, 700, 0, 1000, 1150, 25);    // the common room towards the stairs and the balcony
 shot(950, 930, 1, 300, 800, -45);     // from the balcony over the common room
 shot(1600, 1050, 1, 1100, 650, -30);  // the rented room: the double bed
 shot(800, 1350, 0, 600, 1650, -20);   // the kitchen: the fire with its spit
 shot(1100, 870, 0, 1600, 700, -25);   // the back room
 fclose(f);}
