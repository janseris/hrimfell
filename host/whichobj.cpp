// (v0.4.74) which object draws a spot of an emulator scene: each moved away in turn, the frame drawn again
//   whichobj scene x y      (x in 0..639 of the 640-wide frame)
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
const ItemDef& GameItem(int aId);
static u16 out[640 * 200], b0[640 * 200];
int main(int argc, char** argv)
 {
 int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64); u8* m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
 emu_init(m, 0); Game* g = (Game*)m; emu_mode(m, 0); emu_view(m, 64); emu_scene(m, atoi(argv[1]));
 int sx = atoi(argv[2]), sy = atoi(argv[3]);
 emu_render(m, b0); emu_render(m, b0);
 R3D& r = *g->iR;
 { int w = g->EquippedId(EQ_WEAPON), sh = g->EquippedId(EQ_SHIELD); printf("weapon %d mesh %d shield %d up %d third %d\n", w, w >= 0 ? GameItem(w).mesh : -1, sh, g->DrawAmount(), g->iS.third);
   if (w >= 0) { const Mesh& mm = g->iMItem[GameItem(w).mesh]; printf("  mesh nv %d nf %d radius %d\n", mm.nv, mm.nf, mm.radius); } }
{ int f = g->iDoorFocus; if (f >= 0) { const DoorLink& d = g->Door(f); printf("door focus %d cell %d ocell %d name %s\n", f, d.cell, d.ocell, d.name); } }
 printf("spot colour %04x\n", b0[sy * 640 + sx]);
 { FILE* f = fopen("/tmp/claude-0/wo.ppm", "wb"); fprintf(f, "P6 640 200 255\n"); for (int q = 0; q < 640 * 200; q++) { u16 c = b0[q]; fputc((c >> 8) & 0xF8, f); fputc((c >> 3) & 0xFC, f); fputc((c << 3) & 0xF8, f); } fclose(f); }
 for (int b = 1; b <= 65536; b <<= 1) { r.iSkip = b; emu_render(m, out); r.iSkip = 0; if (out[sy * 640 + sx] != b0[sy * 640 + sx]) printf("skip %d changes it (%04x)\n", b, out[sy * 640 + sx]); }
 for (int k = 0; k < g->iNDun; k++) { if (g->iDunCell[k] != g->iS.cell) continue; int y0 = g->iDunY[k]; g->iDunY[k] += 400 * METER; emu_render(m, out); g->iDunY[k] = y0; if (out[sy * 640 + sx] != b0[sy * 640 + sx]) printf("dun tile %d at %+.1f %+.1f m nv %d nf %d radius %d\n", k, (g->iDunX[k]-g->iS.px)/(double)METER, (g->iDunZ[k]-g->iS.pz)/(double)METER, g->iMDun[k].nv, g->iMDun[k].nf, g->iMDun[k].radius); }
 for (int i = 0; i < g->iS.nprop; i++) { Prop& p = g->iS.prop[i]; int y0 = p.y; p.y += 400 * METER; emu_render(m, out); p.y = y0; if (out[sy * 640 + sx] != b0[sy * 640 + sx]) printf("prop %d type %d model %d count %d\n", i, p.type, p.model, p.count); }
 for (int i = 0; i < MAXE; i++) { Ent& e = g->iS.e[i]; if (!e.type) continue; int y0 = e.y; e.y += 400 * METER; emu_render(m, out); e.y = y0; if (out[sy * 640 + sx] != b0[sy * 640 + sx]) printf("ent %d type %d\n", i, e.type); }
 for (int i = 0; i < r.iObjCount; i++)
  {
  Obj& o = r.iObj[i]; int y0 = o.y; o.y += 400 * METER;
  emu_render(m, out); o.y = y0;
  if (out[sy * 640 + sx] != b0[sy * 640 + sx])
   {
   const char* nm = "?";
   if (o.type >= ST_CUSTOM) { const StaticDef& sd = r.iDefs[o.type - ST_CUSTOM]; for (int k = 0; k < g->iKitMeshN && k < Game::MAXKIT; k++) if (&g->iMKit[k] == sd.mesh) nm = g->KitM(k).name; }
   printf("obj %d type %d at %+.1f %+.1f %+.1f m: %s\n", i, o.type, (o.x - g->iS.px) / (double)METER, (o.y - g->iS.py) / (double)METER, (o.z - g->iS.pz) / (double)METER, nm);
   }
  }
 }
