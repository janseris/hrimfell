#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static u16 out[640*200];
static Game* g; static FILE* f;
static void shot(int x, int z, int tx, int tz, int pitch, int third)
    {
    int ox = 100 * METER, oz = 100 * METER;
    g->EnterCell(CELL_STABLES, ox + x * METER / 100, oz + z * METER / 100, r3d_atan2(tx - x, tz - z));
    g->iS.pitch = pitch; g->iS.third = third; g->iFadeUs = 0;
    for (int k = 0; k < 30; k++) g->Tick(16000, 0);
    g->iFadeUs = 0;
    g->Render(out, 640);
    for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
    }
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY; g->iHideView = 0;
 g->iR->SetOutputMode(2);
 printf("arena over %d props %d conts %d\n", g->iR->iArenaOver, g->iS.nprop, g->iS.ncont);
 int N = argc > 1 ? atoi(argv[1]) : 0;
 f=fopen("shots/int2.ppm","wb"); fprintf(f,"P6 640 800 255\n");
 if (N == 0) {
 shot(300, 120, 600, 300, -60, 0);     // NE bedroom: bed, rug, wolf hide
 shot(-100, -300, -560, -120, -60, 0);   // bedroll
 shot(-40, 300, -40, 432, -10, 0);    // north shelf
 shot(-300, 160, -600, 160, 30, 0);  // bear hide over the long table
 } else {
 shot(330, -150, 470, -440, -40, 0);   // SE bedroom: bed and wolf hide
 shot(520, 60, 300, 85, -120, 0);      // bear rug
 shot(-420, -60, -545, -140, -90, 0);// pelt bed
 shot(100, -300, 100, -432, -10, 0);  // south shelf
 }
 fclose(f);}
