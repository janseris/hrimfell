#include <stdio.h>
#include <stdlib.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
static u16 out[640 * 200];
int main(int argc, char** argv)
 {
 int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64);
 u8* m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
 emu_init(m, 0); Game* g = (Game*)m; emu_mode(m, 0);
 emu_scene(m, atoi(argv[1]));
 emu_tick(m, 66000, 0); emu_render(m, out);
 for (int k = 0; k < g->iKitCellN; k++) printf("kc %d cell %d obj0 %d pvsN %d nstat %d\n", k, g->iKitCells[k].cell, g->iKitObj0[k], g->iKitCells[k].pvsN, g->iKitCells[k].nstat);
 { const KitCell& c = g->iKitCells[0]; int ux = (g->iS.px - 100*METER)*70/METER, uy = (g->iS.pz - 100*METER)*70/METER, f = (g->iS.py - 410 - 100*METER)*70/METER; printf("u %d %d feet %d\n", ux, uy, f); for (int i = 0; i < c.pvsN; i++) { const short* e = c.pvsReg + i*4; if (abs(e[0]*256 - ux) < 800 && abs(e[1]*256-uy) < 800) printf("  reg %d %d %d\n", e[0], e[1], e[2]); } }
 printf("cell %d bits %p first %d n %d\n", g->iS.cell, g->iR->iPvsBits, g->iR->iPvsFirst, g->iR->iPvsN);
 }
// (dump)
