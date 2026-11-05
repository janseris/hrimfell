#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// views of Highhall (cm in the interior frame; y = the floor level there) -> shots/hh.ppm
static u16 out[640*200];
static Game* g; static FILE* f;
static void shot(int x, int z, int y, int tx, int tz, int pitch)
    {
    g->EnterCell(CELL_HIGHHALL, 100 * METER + x * METER / 100, 100 * METER + z * METER / 100, r3d_atan2(tx - x, tz - z));
    g->iS.py = 100 * METER + y * METER / 100 + 410;
    g->iS.pitch = pitch; g->iS.third = 0; g->iFadeUs = 0;
    for (int k = 0; k < 20; k++) g->Tick(16000, 0);
    g->iFadeUs = 0;
    g->Render(out, 640);
    for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
    }
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY; g->iHideView = 0;
 g->iR->SetOutputMode(2);
 f=fopen("shots/hh.ppm","wb"); fprintf(f,"P6 640 1200 255\n");
 shot(0, 4650, 390, 0, 2000, 10);          // from before the throne, down the hall (as the first screenshot)
 shot(0, 2800, 300, 0, 5000, 25);          // from the top of the great stairs towards the throne
 shot(-1000, 2300, 700, 300, 4300, -15);   // from the west gallery over the fire and the tables
 shot(0, 300, 0, 0, 2000, 15);             // the entrance hall, the great stairs
 shot(-1800, 3020, 300, -2700, 3900, -5);  // the kitchen
 shot(1800, 2880, 300, 2700, 3500, -5);    // the court wizard's workshop
 fclose(f);}
