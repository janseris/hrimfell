#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static u16 out[640*200];
static Game* g; static FILE* f;
static void shot(int w, int third, int hour, int drawn)
    {
    int cx = CITY_CX * CELL + CELL / 2, cz = CITY_CZ * CELL + CELL / 2;
    g->EnterCell(0, cx, cz - 40 * METER, 0);
    g->iS.weather = w; g->iPrevWeather = w; g->iWBlend = 256; g->iS.third = third; g->iS.timeOfDay = hour * 60000; g->iS.drawn = drawn;
    for (int k = 0; k < 40; k++) g->Tick(16000, k < 20 ? KEY_FWD : 0);
    g->iFadeUs = 0; g->iS.weather = w; g->iPrevWeather = w; g->iWBlend = 256;
    g->UpdateAtmosphere();
    g->Render(out, 640);
    printf("w%d tris %d\n", w, g->iR->iTris);
    for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
    }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->iUi=UI_PLAY;
 g->iR->SetOutputMode(2);
 f=fopen("shots/w.ppm","wb"); fprintf(f,"P6 640 1000 255\n");
 shot(W_RAIN, 1, 11, 1); shot(W_FOG, 0, 9, 1); shot(W_SNOW, 1, 15, 0); shot(W_CLOUDY, 0, 18, 0); shot(W_CLEAR, 1, 22, 0);
 fclose(f);}
