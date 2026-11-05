#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int GameRiverwoodX(); int GameRiverwoodZ();
static u16 out[640*200];
static void SaveMap(Game* g){ g->iUi = UI_MAP; g->iMapLocal = 1; g->iMapZoom = 1; g->iMapCX = GameRiverwoodX(); g->iMapCZ = GameRiverwoodZ() - 10 * METER; g->Render(out, 640); g->iUi = UI_PLAY; }
static void Save(const char* n){ FILE* f=fopen(n,"wb"); fprintf(f,"P6 640 200 255\n");
 for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); } fclose(f); }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 g->iR->SetOutputMode(2); int ox=GameRiverwoodX(), oz=GameRiverwoodZ();
 g->iS.px = ox; g->iS.pz = oz; g->Tick(16000, 0); SaveMap(g); Save("shots/rw_map.ppm");
 struct V { int x, z, up, yaw, pitch; const char* n; } vs[] = {
   { 0, -40, 45, 0, -170, "shots/rw_bridge.ppm" },     // from high in the east, west over the town
   { -2, -16, 0, 40, 0, "shots/rw_street.ppm" },          // the street by the Trader, looking north-north-east
   { -14, 22, 0, 768, 0, "shots/rw_mill.ppm" },           // west to the mill
   { 20, -75, 18, 896, -40, "shots/rw_above.ppm" } };       // from high in the south, north over the town
 for (int k = 0; k < 4; k++) { g->iUi = UI_PLAY; g->iS.px = ox + vs[k].x * METER; g->iS.pz = oz + vs[k].z * METER;
   g->iS.py = g->iR->GroundAt(g->iS.px, g->iS.pz) + 410; g->iS.yaw = vs[k].yaw & 1023; g->iS.pitch = vs[k].pitch; g->iS.timeOfDay = 12 * 3600 * 1000 / 60;
   for (int t = 0; t < 3; t++) g->Tick(16000, 0); g->iS.py += vs[k].up * METER; g->Render(out, 640); Save(vs[k].n); }
}
