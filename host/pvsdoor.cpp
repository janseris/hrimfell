// (v0.4.74) a ruin's door hides what is behind it while shut, and shows it open: Forelhost's Stronghold (emulator scene
// 34), its nearest hinged door shut then open - the triangles drawn each way -> shots/pvsdoor.ppm (shut over open)
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
static u16 out[640 * 200];
static void Put(FILE* f) { for (int q = 0; q < 640 * 200; q++) { u16 c = out[q]; fputc((c >> 8) & 0xF8, f); fputc((c >> 3) & 0xFC, f); fputc((c << 3) & 0xF8, f); } }
int main()
 {
 int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64); u8* m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
 emu_init(m, 0); Game* g = (Game*)m; emu_mode(m, 0); emu_view(m, 64); emu_scene(m, 34);
 R3D& r = *g->iR; int fails = 0;
 int best = -1, bd = 1 << 30;
 for (int i = 0; i < g->iS.nprop; i++)
  {
  const Prop& p = g->iS.prop[i];
  if (p.type != PT_DOOR || p.count <= 0 || p.cell != g->iS.cell) continue;
  int d = abs(p.x - g->iS.px) + abs(p.z - g->iS.pz);
  if (d < bd) { bd = d; best = i; }
  }
 if (best < 0) { printf("no door\nFAILED\n"); return 1; }
 FILE* f = fopen("shots/pvsdoor.ppm", "wb"); fprintf(f, "P6 640 400 255\n");
 emu_render(m, out); emu_render(m, out); int t0 = r.iTris; const u8* b0 = r.iPvsBits; Put(f);
 Prop& p = g->iS.prop[best]; p.state = 0; p.anim = 4;
 emu_render(m, out); emu_render(m, out); int t1 = r.iTris; const u8* b1 = r.iPvsBits; Put(f); fclose(f);
 printf("door %d %.1f m away: shut %d triangles (bits %s), open %d (bits %s)\n", best, bd / (double)METER, t0, b0 ? "region" : "none", t1, b1 == r.iPvsMix ? "joined" : b1 ? "region" : "none");
 if (!b0) { printf("  FAIL no table for the region\n"); fails++; }
 if (b1 != r.iPvsMix) { printf("  FAIL the open door's far side not joined\n"); fails++; }
 if (t1 <= t0) { printf("  FAIL nothing more drawn through the open door\n"); fails++; }
 printf(fails ? "FAILED (%d failed)\n" : "all passed (0 failed)\n", fails);
 return fails != 0;
 }
