#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int GameBarrowOutX(int a, int b); int GameBarrowOutZ(int a, int b);
static u16 out[640*200];
static void Save(const char* n){ FILE* f=fopen(n,"wb"); fprintf(f,"P6 640 200 255\n");
 for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); } fclose(f); }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 g->iR->SetOutputMode(2);
 // the local map round the doors, then views from high up
 g->iUi = UI_MAP; g->iMapLocal = 1; g->iMapZoom = 1; g->iMapCX = GameBarrowOutX(0, 30); g->iMapCZ = GameBarrowOutZ(0, 30);
 g->iS.px = GameBarrowOutX(0, 2); g->iS.pz = GameBarrowOutZ(0, 2); g->Tick(16000, 0); g->iUi = UI_MAP; g->Render(out, 640); Save("shots/bfb_map.ppm");
 struct V { int a, b, up, yaw, pitch; const char* n; } vs[] = { { 0, 110, 40, (298 + 512) & 1023, 25, "shots/bfb_far.ppm" }, { 40, 60, 25, (298 + 512 + 100) & 1023, 20, "shots/bfb_side.ppm" }, { 0, -2, 2, 298, -10, "shots/bfb_door.ppm" } };
 for (int k = 0; k < 3; k++) { g->iUi = UI_PLAY; g->iS.px = GameBarrowOutX(vs[k].a, vs[k].b); g->iS.pz = GameBarrowOutZ(vs[k].a, vs[k].b);
   g->iS.py = g->iR->GroundAt(g->iS.px, g->iS.pz) + 410; g->iS.yaw = vs[k].yaw; g->iS.pitch = vs[k].pitch; g->iS.timeOfDay = 12 * 3600 * 1000 / 60;
   for (int t = 0; t < 3; t++) g->Tick(16000, 0); g->iS.py += vs[k].up * METER; g->Render(out, 640); Save(vs[k].n); }
}
