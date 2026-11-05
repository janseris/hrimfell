#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
static u16 out[640 * 200];
const char* GameKitSay(int aNamed, int k);
int main(){ int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64); u8* m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
 emu_init(m, 0); Game* g = (Game*)m; g->iUi = UI_PLAY;
 for (int t = 0; t < 4; t++) for (int i = 0; i < g->iS.ne; i++) { Ent& e = g->iS.e[i]; if (e.role == ROLE_NAMED && (e.pad1 == 23 || e.pad1 == 24 || e.pad1 == 25 || e.pad1 == 27)) {
   g->iS.cell = e.cell; g->iFocus = i; g->iFocusKind = 1; g->iUi = UI_PLAY; g->Activate(); int q = -1000 - g->iDlgPage;
   printf("talk %d to %d: page %d -> %s\n", t, e.pad1, g->iDlgPage, g->iDlgPage <= -1000 ? GameKitSay(q >> 6, q & 63) : "(old line)"); } }
 FILE* f = fopen("/tmp/claude-0/talk.ppm", "wb"); emu_render(m, out); fprintf(f, "P6 640 200 255\n");
 for (int i = 0; i < 640 * 200; i++) { u16 q = out[i]; unsigned char c[3] = { (unsigned char)((q >> 8) & 0xF8), (unsigned char)((q >> 3) & 0xFC), (unsigned char)((q << 3) & 0xF8) }; fwrite(c, 1, 3, f); } fclose(f);
}
