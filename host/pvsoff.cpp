// (v0.4.74) a scene drawn with and without "what can be seen from where": the triangles each way -> shots/pvsoff_<s>.ppm
#include <stdio.h>
#include <stdlib.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
static u16 out[640 * 200];
static void Put(FILE* f) { for (int q = 0; q < 640 * 200; q++) { u16 c = out[q]; fputc((c >> 8) & 0xF8, f); fputc((c >> 3) & 0xFC, f); fputc((c << 3) & 0xF8, f); } }
int main(int argc, char** argv)
 {
 int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64); u8* m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
 emu_init(m, 0); Game* g = (Game*)m; emu_mode(m, 0); emu_view(m, 64); int s = atoi(argv[1]); emu_scene(m, s);
 char fn[64]; sprintf(fn, "shots/pvsoff_%d.ppm", s); FILE* f = fopen(fn, "wb"); fprintf(f, "P6 640 400 255\n");
 emu_render(m, out); emu_render(m, out); int t0 = g->iR->iTris; Put(f);
 KitCell* kc = (KitCell*)g->iKitCells; const u8* save[32]; for (int k = 0; k < g->iKitCellN && k < 32; k++) { save[k] = kc[k].pvs; kc[k].pvs = 0; }
 emu_render(m, out); emu_render(m, out); int t1 = g->iR->iTris; Put(f); fclose(f);
 printf("scene %d cell %d: with the tables %d triangles, without %d\n", s, g->iS.cell, t0, t1);
 for (int k = 0; k < g->iKitCellN && k < 32; k++) kc[k].pvs = save[k];
 static const int KB[] = { 512, 2048, 4096, 16384, 32768, 65536 };
 for (int b = 0; b < 6; b++) { g->iR->iSkip = KB[b]; emu_render(m, out); emu_render(m, out); printf("  skip %d: %d triangles\n", KB[b], g->iR->iTris); }
 g->iR->iSkip = 0;
 }
