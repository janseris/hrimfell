// (v0.4.74) whether a scene's area fitted its memory: the first allocation that did not, and what was left then
#include <stdio.h>
#include <stdlib.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
int g_arenaOver1, g_arenaFree1;
int main(int argc, char** argv)
 {
 int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64); u8* m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
 emu_init(m, 0); Game* g = (Game*)m; emu_mode(m, 0);
 for (int a = 1; a < argc; a++)
  {
  g_arenaOver1 = 0; g->iR->iArenaOver = 0; emu_scene(m, atoi(argv[a]));
  printf("scene %s: arena over by %d (first refused %d bytes with %d free), free now %d\n", argv[a], g->iR->iArenaOver, g_arenaOver1, g_arenaFree1, (int)(g->iR->iArenaEnd - g->iR->iArena));
  }
 }
