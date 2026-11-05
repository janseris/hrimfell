#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// views of the Sleeping Giant Inn (Skyrim units in the interior frame; h = eye floor in units) -> shots/sg.ppm
static u16 out[640*200];
static Game* g; static FILE* f;
static void shot(int x, int y, int h, int tx, int ty, int pitch)
    {
    int X = 100 * METER + x * 143 / 100 * METER / 100, Z = 100 * METER + y * 143 / 100 * METER / 100;
    g->EnterCell(CELL_RWINN, X, Z, r3d_atan2(tx - x, ty - y));
    g->iUi = UI_PLAY;
    g->iS.py = 100 * METER + h * 143 / 100 * METER / 100 + 410;
    g->iS.pitch = pitch; g->iS.third = 0; g->iFadeUs = 0;
    for (int k = 0; k < 20; k++) g->Tick(16000, 0);
    g->iFadeUs = 0;
    g->Render(out, 640);
    for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
    }
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY; g->iHideView = 0;
 g->iR->SetOutputMode(2); g->iS.timeOfDay = 20 * 60000;
 f=fopen("shots/sg.ppm","wb"); fprintf(f,"P6 640 1400 255\n");
 shot(200, -150, 0, -1100, 0, 10);       // down the hall to the bar (the fandom view)
 shot(-1000, 150, 0, 200, -100, 10);     // back from the bar: the fire pit, the tables, the door
 shot(-300, 330, 0, -700, 0, -15);       // by the north tables: the pit and its spit
 shot(-1300, 0, 0, -1100, 0, 0);         // Orgnar behind the bar
 shot(-900, -880, 0, -900, -500, 0);     // the south rooms
 shot(-1050, 650, 0, -700, 850, 0);      // Delphine's room, her wardrobe
 shot(450, 1050, -256, 100, 650, -5);    // the secret room
 fclose(f);}
