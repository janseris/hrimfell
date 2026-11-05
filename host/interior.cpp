#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static u16 out[640*200];
static Game* g; static FILE* f;
// local cm coords inside the cell
static void shot(int x, int z, int tx, int tz, int pitch, int third)
    {
    int ox = 100 * METER, oz = 100 * METER;
    g->EnterCell(CELL_STABLES, ox + x * METER / 100, oz + z * METER / 100, r3d_atan2(tx - x, tz - z));
    g->iS.pitch = pitch; g->iS.third = third; g->iFadeUs = 0;
    for (int k = 0; k < 30; k++) g->Tick(16000, 0);
    g->iFadeUs = 0;
    g->Render(out, 640);
    printf("tris %d\n", g->iR->iTris);
    for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
    }
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->iUi=UI_PLAY; g->iHideView = 0;
 g->iR->SetOutputMode(2);
 printf("arena used %d KB of MemNeeded %d KB\n", (int)(g->iR->iArena - mem)/1024, n/1024);
 f=fopen("shots/int.ppm","wb"); fprintf(f,"P6 640 800 255\n");
 shot(-200, -330, -200, 100, 0, 0);
 shot(-200, -330, -500, 0, -20, 1);
 shot(0, 0, 450, 300, -30, 0);
 shot(-100, 250, -300, -300, -10, 0);
 fclose(f);}
