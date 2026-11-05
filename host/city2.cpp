#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static u16 out[640*200];
static Game* g; static FILE* f;
static void shot(int mx, int mz, int tx, int tz, int dy, int pitch)
    {
    int cx = CITY_CX * CELL + CELL / 2, cz = CITY_CZ * CELL + CELL / 2;
    g->iS.px = cx + mx * METER / 2; g->iS.pz = cz + mz * METER / 2; g->iS.py = g->iR->GroundAt(g->iS.px, g->iS.pz) + 410 + dy * METER/2;
    g->iS.yaw = r3d_atan2((tx - mx) * METER, (tz - mz) * METER); g->iS.pitch = pitch;
    g->Tick(16000, 0); g->Render(out, 640);
    for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
    }
// args: groups of 6 numbers (half-metres): mx mz tx tz dy pitch
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->iUi=UI_PLAY; g->iHideView = 1;
 g->iR->SetOutputMode(2);
 if (getenv("WX")) { g->iS.weather = atoi(getenv("WX")); g->iPrevWeather = g->iS.weather; }
 if (getenv("TOD")) g->iS.timeOfDay = atoi(getenv("TOD")) * 60000;
 if (getenv("TEX")) g->iR->iMode = MODE_TEX;
 if (getenv("TORCH")) { g->NewGame(); g->iUi = UI_PLAY; g->iHideView = 0; if (getenv("TOD")) g->iS.timeOfDay = atoi(getenv("TOD")) * 60000; g->ToggleTorch(); }
 if (getenv("THIRD")) g->iS.third = 1;
 int k=(argc-1)/6;
 f=fopen("shots/c2.ppm","wb"); fprintf(f,"P6 640 %d 255\n",200*k);
 for(int i=0;i<k;i++){ int*a=new int[6]; for(int j=0;j<6;j++) a[j]=atoi(argv[1+i*6+j]); shot(a[0],a[1],a[2],a[3],a[4],a[5]); }
 fclose(f);}
