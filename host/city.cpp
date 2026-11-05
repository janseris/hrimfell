#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static u16 out[640*200];
static Game* g; static FILE* f;
static void shot(int mx, int mz, int tx, int tz, int dy, int pitch, int mode)
    {
    int cx = CITY_CX * CELL + CELL / 2, cz = CITY_CZ * CELL + CELL / 2;
    g->iR->SetOutputMode(mode);
    g->iS.px = cx + mx * METER; g->iS.pz = cz + mz * METER; g->iS.py = g->iR->GroundAt(g->iS.px, g->iS.pz) + 410 + dy * METER;
    g->iS.yaw = r3d_atan2((tx - mx) * METER, (tz - mz) * METER); g->iS.pitch = pitch;
    g->Tick(16000, 0); g->Render(out, 640);
    printf("tris %d\n", g->iR->iTris);
    for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
    }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->iUi=UI_PLAY; g->iHideView = getenv("NOVIEW") != 0;
 printf("statics %d defs %d arena used %d KB\n", g->iR->iObjCount, g->iR->iNumDefs, (int)(g->iR->iArena - ((u8*)g->iR->iRecip + 65536*4)) / 1024);
 f=fopen("shots/city.ppm","wb"); fprintf(f,"P6 640 1400 255\n");
 shot(0, -110, 0, 0, 0, 30, 2);      // approach from the south
 shot(0, -58, 0, 0, 0, 40, 2);       // at the gate
 shot(0, -34, 0, -20, 0, 0, 2);      // market
 shot(-4, -14, 0, 10, 0, 30, 2);     // tree rise
 shot(0, 6, 0, 30, 0, 50, 2);        // looking up at highhall
 shot(0, 26, 0, -60, 0, -40, 2);     // view from the crown over the city
 shot(-100, -40, -68, 2, 0, 20, 2);  // farms + windmill
 fclose(f);}
