// (v0.4.74) which of the area's pictures (or the library's) a spot on the screen is drawn with: each painted over in
// turn, the frame drawn again, the spot compared.   whichtex door yaw x y [view]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
static u16 out[640 * 200];
int main(int argc, char** argv)
 {
 int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64); u8* m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
 emu_init(m, 0); Game* g = (Game*)m; emu_mode(m, 0);
 int door = atoi(argv[1]), yaw = atoi(argv[2]), sx = atoi(argv[3]), sy = atoi(argv[4]);
 emu_view(m, argc > 5 ? atoi(argv[5]) : 64);
 const DoorLink& d = g->Door(door);
 g->EnterCell(d.ocell, d.ox + ((r3d_sin(d.oyaw) * METER) >> 14), d.oz + ((r3d_cos(d.oyaw) * METER) >> 14), d.oyaw);
 g->iS.timeOfDay = 12 * 60000; g->UseDoor(door); g->iS.yaw = (g->iS.yaw + yaw) & 1023; g->iUi = UI_PLAY;
 emu_tick(m, 33000, 0); g->iUi = UI_PLAY; g->iFadeUs = 0;
 static u16 b0[640 * 200], b1[640 * 200];
 emu_render(m, b0); emu_render(m, b1);
 int stable[121], ns = 0;
 for (int dy = -5; dy <= 5; dy++) for (int dx = -5; dx <= 5; dx++) { int q = (sy + dy) * 640 + sx + dx; if (b0[q] == b1[q]) stable[ns++] = q; }
 u16 base = b1[sy * 640 + sx];
 printf("%d stable pixels round the spot\n", ns);
 R3D& r = *g->iR;
 printf("spot %d,%d colour %04x; area pictures %d, library %d\n", sx, sy, base, r.iSkyN, r.iLibN);
 for (int lib = 0; lib < 3; lib++)
  {
  int N = lib == 2 ? 32 : lib ? r.iLibN : r.iSkyN;
  u8* t = (u8*)(lib == 2 ? r.iTex : lib ? r.iLibTex : r.iSkyTex);
  for (int k = 0; k < N; k++)
   {
   static u8 save[64 * 64]; memcpy(save, t + k * 4096, 4096); memset(t + k * 4096, 1, 4096);
   emu_render(m, out);
   int ch = 0; for (int j = 0; j < ns; j++) if (out[stable[j]] != b1[stable[j]]) ch++;
   if (ch * 3 > ns) printf("  %s picture %d (slot byte %02x) draws %d of them\n", lib == 2 ? "the engine's" : lib ? "library" : "area", k, lib == 2 ? (0x80 | k) : (lib ? 0x40 : 0xC0) | k, ch);
   memcpy(t + k * 4096, save, 4096);
   }
  }
 }
