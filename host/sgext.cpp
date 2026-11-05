#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int GameRiverwoodX(); int GameRiverwoodZ();
static u16 out[640*200];
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame();
 g->iR->SetOutputMode(2); int ox=GameRiverwoodX(), oz=GameRiverwoodZ();
 FILE* f=fopen("shots/sgext.ppm","wb"); fprintf(f,"P6 640 800 255\n");
 int d = 12; const DoorLink& dl = g->Door(d);
 // from the door: out along its facing (m), to the side (m), up (m), turn to look (1024ths), pitch
 struct V { int out, side, up, look, pitch; } vs[] = { { 14, 0, 0, 512, 20 }, { 12, 10, 0, 440, 15 }, { 12, -12, 0, 600, 15 }, { 4, 0, 30, 512, -150 } };
 for (int k = 0; k < 4; k++) { g->iUi = UI_PLAY;
   int fx = r3d_sin(dl.oyaw), fz = r3d_cos(dl.oyaw), rx = r3d_cos(dl.oyaw), rz = -r3d_sin(dl.oyaw);
   int x = dl.ox + ((fx * vs[k].out * METER) >> 14) + ((rx * vs[k].side * METER) >> 14), z = dl.oz + ((fz * vs[k].out * METER) >> 14) + ((rz * vs[k].side * METER) >> 14);
   g->EnterCell(0, x, z, 0); g->iFadeUs = 0;
   g->iS.py = g->iR->GroundAt(x, z) + 410; g->iS.yaw = r3d_atan2(dl.ox - x, dl.oz - z); if (k == 3) g->iS.yaw = (dl.oyaw + 512) & 1023; g->iS.pitch = vs[k].pitch; g->iS.timeOfDay = 13 * 60000;
   for (int t = 0; t < 5; t++) g->Tick(16000, 0); g->iS.py += vs[k].up * METER; g->Render(out, 640);
   for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); } }
 fclose(f);
}
