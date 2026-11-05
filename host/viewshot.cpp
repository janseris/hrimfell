#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
// renders outdoor views with the draw distance settings, the world map and the menu: shots/view_*.ppm
static u16 out[640*200];
static void Save(const char* n){ FILE* f=fopen(n,"wb"); fprintf(f,"P6 640 200 255\n");
 for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
 fclose(f); }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game));
 clock_t t0=clock(); g->Init(mem); printf("init %.2f s host\n", (double)(clock()-t0)/CLOCKS_PER_SEC);
 g->iR->SetOutputMode(2); g->iUi = UI_PLAY;
 int cx = CITY_CX * CELL + CELL / 2, cz = CITY_CZ * CELL + CELL / 2;
 struct V { int x, z, yaw, view; const char* n; int pitch; } vs[] = {
   { cx - 150 * METER, cz - 130 * METER, 128, 0, "shots/view_def.ppm", 0 },                 // the plain below Whiterun, looking at the city
   { cx - 150 * METER, cz - 130 * METER, 128, 8, "shots/view_nofar.ppm", 0 },
   { cx - 150 * METER, cz - 130 * METER, 640, 2|4, "shots/view_max.ppm", 0 },               // south-west: the mountains of the barrow
   { GameBarrowOutX(-10, 70), GameBarrowOutZ(-10, 70), (298 + 512 + 40) & 1023, 0, "shots/view_north.ppm", 40 },  // the platforms from below, east
   { GameBarrowOutX(0, 3), GameBarrowOutZ(0, 3), 298, 2|4, "shots/view_mid.ppm", -30 } };   // from the terrace, east over the valley
 for (int k = 0; k < 5; k++) {
   g->iS.px = vs[k].x; g->iS.pz = vs[k].z; g->iS.py = g->iR->GroundAt(g->iS.px, g->iS.pz) + 410; g->iS.yaw = vs[k].yaw; g->iS.pitch = vs[k].pitch;
   g->iS.view = vs[k].view; g->ApplyViewSetting(); g->iS.timeOfDay = 12 * 3600 * 1000 / 60;
   for (int t = 0; t < 3; t++) g->Tick(16000, 0);
   g->Render(out, 640); printf("%s tris %d\n", vs[k].n, g->iR->iTris); Save(vs[k].n); }
 g->iUi = UI_MAP; g->iMapLocal = 0; g->Render(out, 640); Save("shots/view_map.ppm");
 g->iUi = UI_MENU; g->iMenuSel = 6; g->Render(out, 640); Save("shots/view_menu.ppm");
}
