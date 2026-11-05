#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static u16 out[640*200];
static Game* g; static FILE* f;
static void snap(){ g->Render(out, 640); for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); } }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->iUi=UI_PLAY;
 g->iR->SetOutputMode(0);
 f=fopen("shots/ui.ppm","wb"); fprintf(f,"P6 640 800 255\n");
 g->Tick(16000,0);
 printf("time %d day %d w %d\n", g->iS.timeOfDay/60000, g->iS.day, g->iS.weather);
 g->KeyChar('t'); printf("ui %d (wait=%d)\n", g->iUi, UI_WAIT);
 g->Tick(16000, KEY_LOOK_R); g->Tick(16000, 0); g->Tick(16000, KEY_LOOK_R); g->Tick(16000, 0);
 snap();
 g->KeyChar('e');
 for (int k=0;k<100;k++) g->Tick(16000,0);
 snap();
 for (int k=0;k<300;k++) g->Tick(16000,0);
 printf("after: ui %d time %d day %d w %d hours %d\n", g->iUi, g->iS.timeOfDay/60000, g->iS.day, g->iS.weather, g->iS.weatherHours);
 g->KeyChar(27); g->iMenuSel = 5; g->Tick(16000,0); g->Tick(16000,KEY_FIRE); printf("ui %d tele=%d\n", g->iUi, UI_TELE);
 g->Tick(16000,0); g->Tick(16000,KEY_LOOK_D); g->Tick(16000,0); g->Tick(16000,KEY_LOOK_D);g->Tick(16000,0); g->Tick(16000,KEY_LOOK_D);g->Tick(16000,0); g->Tick(16000,KEY_LOOK_D);g->Tick(16000,0); g->Tick(16000,KEY_LOOK_D);
 snap();
 g->KeyChar('e'); printf("ui %d cell %d\n", g->iUi, g->iS.cell);
 g->iFadeUs=0; g->Tick(16000,0); g->Tick(16000,0); snap();
 // walk to the door and leave
 printf("door focus %d\n", g->iDoorFocus);
 g->iS.yaw = 512; for (int k=0;k<40;k++) g->Tick(16000,KEY_FWD);
 printf("pos local %d %d door focus %d\n", (g->iS.px-100*METER)*100/METER, (g->iS.pz-100*METER)*100/METER, g->iDoorFocus);
 g->KeyChar('e'); printf("cell after E %d\n", g->iS.cell);
 fclose(f);}
