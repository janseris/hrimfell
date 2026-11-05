#include <stdio.h>
#include <stdlib.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
int main(int argc, char** argv)
 {
 int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64); u8* m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
 emu_init(m, 0); Game* g = (Game*)m; emu_mode(m, 0);
 emu_scene(m, argc > 1 ? atoi(argv[1]) : 37);
 R3D& r = *g->iR;
 int tf = 0, tv = 0, to = 0;
 for (int k = 0; k < r.iNFG; k++) { R3D::FarGroup& fg = r.iFG[k]; tf += fg.m.nf; tv += fg.m.nv; to += fg.nobj; printf("group %2d at %4d %4d m: %3d pieces -> %4d faces %4d corners, %d m across\n", k, fg.x / METER, fg.z / METER, fg.nobj, fg.m.nf, fg.m.nv, (fg.x1 - fg.x0) / METER); }
 printf("%d groups: %d pieces, %d faces, %d corners (%d KB); arena free %d KB, area %d\n", r.iNFG, to, tf, tv, (tf * 14 + tv * 6) / 1024, (int)(r.iArenaEnd - r.iArena) / 1024, g->iAreaCur);
 }
