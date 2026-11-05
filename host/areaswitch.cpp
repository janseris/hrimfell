// v0.4.57: walking between Riverwood's and Whiterun's areas outside; a door into the stables (area 2) and the barrow
#include <stdio.h>
#include <stdlib.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
static u16 out[640 * 200];
int main()
 {
 int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64); u8* m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
 emu_init(m, 0); Game* g = (Game*)m; emu_mode(m, 0);
 g->NewGame(); g->iUi = UI_PLAY;
 printf("new game: area %d objs %d at %d m N\n", g->iAreaCur, g->iR->iObjCount, g->iS.pz / METER);
 int zs[6] = { 2300, 2050, 1900, 2150, 2300, 2000 }, fails = 0;
 for (int k = 0; k < 6; k++)
  {
  g->iS.cell = 0; g->iS.px = 3674 * METER; g->iS.pz = zs[k] * METER; g->iR->FocusLand(g->iS.px, g->iS.pz);
  g->iS.py = g->FloorAt(0, g->iS.px, g->iS.pz, g->iR->GroundAt(g->iS.px, g->iS.pz) + 2 * METER) + 410;
  for (int t = 0; t < 5; t++) { emu_tick(m, 50000, 0); }
  emu_render(m, out);
  int want = zs[k] > 2120 ? 2 : zs[k] < 2080 ? 0 : g->iAreaCur;
  printf("at %d m N: area %d (want %d), objs %d, tris %d\n", zs[k], g->iAreaCur, want, g->iR->iObjCount, g->iR->iTris);
  if (g->iAreaCur != want) fails++;
  }
 // the stables' door (Skyrim's), from area 0
 for (int i = 0; i < 40; i++) { const DoorLink& d = g->Door(i); if (d.cell == CELL_STABLES && d.name) { g->UseDoor(i); break; } }
 emu_tick(m, 50000, 0); emu_render(m, out);
 printf("stables: cell %d area %d tris %d\n", g->iS.cell, g->iAreaCur, g->iR->iTris);
 if (g->iS.cell != CELL_STABLES || g->iAreaCur != 2) fails++;
 // the barrow (area 1) and out again
 g->UseDoor(7); emu_tick(m, 50000, 0); emu_render(m, out);
 printf("barrow: cell %d area %d\n", g->iS.cell, g->iAreaCur);
 if (g->iAreaCur != 1) fails++;
 g->UseDoor(7); emu_tick(m, 50000, 0); emu_render(m, out);
 printf("out of the barrow: cell %d area %d objs %d tris %d\n", g->iS.cell, g->iAreaCur, g->iR->iObjCount, g->iR->iTris);
 printf("%s (%d failed)\n", fails ? "FAILED" : "all passed", fails);
 return fails;
 }
