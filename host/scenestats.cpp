// the emulator's scenes on the host, with what each model puts on the screen (built with R3D_STATS):
//   scenestats scene [view]
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
 int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64);
 u8* m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
 emu_init(m, 0);
 Game* g = (Game*)m;
 emu_mode(m, 0);
 if (argc > 2) emu_view(m, atoi(argv[2]));
 emu_scene(m, atoi(argv[1]));
 if (getenv("SKIP")) g->iR->iSkip = atoi(getenv("SKIP"));
 extern int g_r3dVC; for (int s = 0; s < 12; s++) { g_r3dVC = 0; emu_tick(m, 66000, 0); if (s == 11) { g_r3dStatN = 0; g_r3dPixFlat = 0; g_r3dRows[0] = g_r3dRows[1] = g_r3dRows[2] = 0; g_r3dStage[8] = 0; } { extern int g_triId, g_triVis, g_triOccN, g_triVisOcc; if (s == 11) { g_triId = 0; g_triVis = 0; g_triOccN = 0; g_triVisOcc = 0; } } emu_render(m, out); if (s == 11) { extern int g_triId, g_triVis; printf("triangles set up %d, with a pixel shown %d, wholly hidden %d (%d%%)\n", g_triId, g_triVis, g_triId - g_triVis, g_triId ? 100 * (g_triId - g_triVis) / g_triId : 0); extern int g_triOccN, g_triVisOcc; printf("  set up with no occlusion map yet %d (hidden %d); with one %d (hidden %d)\n", g_triId - g_triOccN, (g_triId - g_triOccN) - (g_triVis - g_triVisOcc), g_triOccN, g_triOccN - g_triVisOcc); } printf("frame %d: VertexColour %d, sun key %x far key %x row %d\n", s, g_r3dVC, g->iR->iBakedSun, g->iR->iFarColKey, g->iR->iFarColRow); }
 int tot = 0, faces = 0; for (int i = 0; i < g_r3dStatN; i++) { tot += g_r3dStat[i].tris; faces += g_r3dStat[i].faces; }
 printf("scene %s: %d triangles drawn (of %d faces sent), %d models, %d hidden; pixels %d (flat %d), screen %d x %d\n", argv[1], tot, faces, g_r3dStatN, g->iR->iOccSkipped, g->iR->iPixels, g_r3dPixFlat, g->iR->iW, g->iR->iH);
 printf("stages (pixels, triangles): buildings %d %d, land %d %d, far land %d %d, the rest %d %d; after the frame %d\n", g_r3dStage[0], g_r3dStage[4], g_r3dStage[1]-g_r3dStage[0], g_r3dStage[5]-g_r3dStage[4], g_r3dStage[2]-g_r3dStage[1], g_r3dStage[6]-g_r3dStage[5], g_r3dStage[3]-g_r3dStage[2], g_r3dStage[7]-g_r3dStage[6], g->iR->iPixels - g_r3dStage[3]);
 printf("far land: vertices worked out %d, fs %d step... farvis %d m, grid %d\n", g_r3dStage[8], 0, g->iR->iFarVis, 0); { extern int g_arenaOver1, g_arenaFree1; printf("arena free %d KB, over %d (first %d with %d free)\n", (int)(g->iR->iArenaEnd - g->iR->iArena) / 1024, g->iR->iArenaOver, g_arenaOver1, g_arenaFree1); } { extern int g_clTot; printf("clusters left out (12 frames): %d of %d; band rows left out %d\n", g->iR->iClSkipped, g_clTot, g->iR->iBandSkip); } printf("spans: flat %d, gouraud %d, textured %d\n", g_r3dRows[0], g_r3dRows[1], g_r3dRows[2]);
 // the biggest first
 for (int k = 0; k < 30; k++)
  {
  int b = -1; for (int i = 0; i < g_r3dStatN; i++) if (g_r3dStat[i].calls >= 0 && (b < 0 || g_r3dStat[i].pix > g_r3dStat[b].pix)) b = i;
  if (b < 0 || g_r3dStat[b].pix == 0) break;
  R3DStat& s = g_r3dStat[b];
  const char* name = "(engine)";
  for (int i = 0; i < g->iKitMeshN && i < Game::MAXKIT; i++) if (&g->iMKit[i] == s.m) name = g->KitM(i).name;
  static char nb[64];
  for (int i = 0; i < 136; i++) if (&g->iMIn[i] == s.m) { sprintf(nb, "interior furniture IN %d", i); name = nb; }
  for (int i = 0; i < 7; i++) if (&g->iMHuman[i] == s.m) { sprintf(nb, "human %d", i); name = nb; }
  for (int i = 0; i < 7; i++) if (&g->iMHumanLod[i] == s.m) { sprintf(nb, "human lod %d", i); name = nb; }
  for (int i = 0; i < IM_COUNT; i++) if (&g->iMItem[i] == s.m) { sprintf(nb, "item mesh %d", i); name = nb; }
  for (int i = 0; i < 72; i++) if (&g->iMCity[i] == s.m) { sprintf(nb, "city %d", i); name = nb; }
  if (s.m == &g->iMChest) name = "chest"; if (s.m == &g->iMFlame) name = "flame"; if (s.m == &g->iMHand) name = "hand"; if (s.m == &g->iMPlayer[0] || s.m == &g->iMPlayer[1]) name = "player body";
  printf("  %5d tris %6d px  %3d times  %4d faces/model %4d verts lazy %d  %s\n", s.tris, s.pix, s.calls, s.calls ? s.faces / s.calls : 0, s.m ? s.m->nv : 0, s.m ? s.m->lazy : 0, name ? name : "?");
  s.calls = -1;
  }
 if (argc > 3) { FILE* f = fopen(argv[3], "wb"); fprintf(f, "P6 640 200 255\n"); for (int i = 0; i < 640 * 200; i++) { u16 q = out[i]; unsigned char c[3] = { (unsigned char)((q >> 8) & 0xF8), (unsigned char)((q >> 3) & 0xFC), (unsigned char)((q << 3) & 0xF8) }; fwrite(c, 1, 3, f); } fclose(f); }
 return 0;
 }
