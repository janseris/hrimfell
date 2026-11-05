#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static u16 out[640*200];
static Game* g; static FILE* f;
static void shot(int mx, int mz, int tx, int tz, int dy, int pitch)
    {
    int cx = CITY_CX * CELL + CELL / 2, cz = CITY_CZ * CELL + CELL / 2;
    g->iS.px = cx + mx * METER; g->iS.pz = cz + mz * METER; g->iS.py = g->iR->GroundAt(g->iS.px, g->iS.pz) + 410 + dy * METER;
    g->iS.yaw = r3d_atan2((tx - mx) * METER, (tz - mz) * METER); g->iS.pitch = pitch;
    g->Tick(16000, 0); g->Render(out, 640);
    for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
    }
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->iUi=UI_PLAY; g->iHideView = 1;
 g->iR->SetOutputMode(2);
 f=fopen("shots/houses.ppm","wb"); fprintf(f,"P6 640 800 255\n");
 shot(-10, -24, -26, -28, 0, 0);
 shot(10, -20, 30, -20, 0, 0);
 shot(-30, 0, -36, 18, 0, 0);
 shot(0, -2, 34, 10, 0, 0);
 fclose(f);}
