// v0.4.62: the barrow's ambushers - asleep in their alcoves and sarcophagi, woken by the player near, out to their place
#include <stdio.h>
#include <stdlib.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
static Game* g; static u8* m; static int fails = 0;
static void Check(int ok, const char* w) { printf("%s %s\n", ok ? "  ok  " : "  FAIL", w); if (!ok) fails++; }
static int CmX(int cm) { return 100 * METER + cm * METER / 100; }
static void Run(int ms) { for (int t = 0; t < ms; t += 50) { emu_tick(m, 50000, 0); if (g->iUi != UI_PLAY) g->iUi = UI_PLAY; } }
int main()
 {
 int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64); m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
 emu_init(m, 0); g = (Game*)m; emu_mode(m, 0); g->NewGame(); g->iUi = UI_PLAY;
 const DoorLink& d0 = g->Door(7); g->EnterCell(d0.ocell, d0.ox, d0.oz, d0.oyaw); g->UseDoor(7); Run(200);
 g->iS.hp = 30000 << 8;
 int alc = 0, cof = 0;
 for (int i = 0; i < g->iS.ne; i++) { const Ent& e = g->iS.e[i]; if (e.cell == CELL_BFTEMPLE && e.type == E_WIGHT && (e.pad2 & 32)) { if (e.pad2 & 8) cof++; else alc++; } }
 printf("temple: %d draugr asleep in alcoves, %d in sarcophagi\n", alc, cof);
 Check(alc >= 8 && cof >= 1, "Skyrim's ambushers in their places");
 for (int pass = 0; pass < 2; pass++)
  {
  int k = -1;
  for (int i = 0; i < g->iS.ne; i++) { const Ent& e = g->iS.e[i]; if (e.cell == CELL_BFTEMPLE && e.type == E_WIGHT && e.alive && (e.pad2 & 32) && (!!(e.pad2 & 8)) == (pass == 1) && (e.hx != e.x || e.hz != e.z)) { k = i; break; } }
  if (k < 0) { Check(0, pass ? "a sarcophagus sleeper" : "an alcove sleeper"); continue; }
  Ent& e = g->iS.e[k];
  int x0 = e.x, z0 = e.z;
  // far: still asleep
  g->iS.px = e.x + 12 * METER; g->iS.pz = e.z; g->iS.py = e.y + 410; Run(1000);
  Check((e.pad2 & 32) && e.x == x0, pass ? "in its sarcophagus while you are far" : "standing in its alcove while you are far");
  // near: wakes, steps out to its place, fights
  g->iS.px = e.hx + (e.hx - e.x) / 2; g->iS.pz = e.hz + (e.hz - e.z) / 2; g->iS.py = g->FloorAt(CELL_BFTEMPLE, g->iS.px, g->iS.pz, e.y + METER) + 410;
  g->iS.px = e.x + 3 * METER; g->iS.pz = e.z; Run(300);
  Check(!(e.pad2 & 40), "woken as you come near");
  Run(4000);
  printf("    from %d %d to %d %d (its place %d %d), aggro %d\n", (x0 - CmX(0)) * 100 / METER, (z0 - CmX(0)) * 100 / METER, (e.x - CmX(0)) * 100 / METER, (e.z - CmX(0)) * 100 / METER, (e.hx - CmX(0)) * 100 / METER, (e.hz - CmX(0)) * 100 / METER, e.aggro);
  Check(e.aggro && !(e.pad2 & 128), "out and fighting");
  }
 printf("%s (%d failed)\n", fails ? "FAILED" : "all passed", fails);
 return fails;
 }
