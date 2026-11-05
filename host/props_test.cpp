#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static u16 out[640*200];
static Game* g; static FILE* f; static int shots=0;
static void snap(){ g->Render(out, 640); shots++; for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); } }
// aim at a prop index: set yaw/pitch toward it from a point 1.2 m away
static void aimAt(int i, int dist){ Prop& p=g->iS.prop[i]; int cy = p.y + (p.type==PT_GARLIC||p.type==PT_FISH ? -160 : (p.type==PT_ITEM? 10 : 120));
  g->EnterCell(p.cell, p.x - dist, p.z - 0, 256); g->iFadeUs=0;
  g->iS.yaw = 256; int dy = cy - g->iS.py; g->iS.pitch = r3d_atan2(dy, dist); if (g->iS.pitch > 512) g->iS.pitch -= 1024; g->Tick(16000,0); }
static int findProp(int type, int cell){ for(int i=0;i<g->iS.nprop;i++) if(g->iS.prop[i].type==type && g->iS.prop[i].cell==cell && !(g->iS.prop[i].flags&PF_GONE)) return i; return -1; }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->iUi=UI_PLAY; g->iR->SetOutputMode(2);
 f=fopen("shots/props.ppm","wb"); fprintf(f,"P6 640 1200 255\n");
 printf("props %d conts %d ents %d\n", g->iS.nprop, g->iS.ncont, g->iS.ne);
 int gi = findProp(PT_GARLIC, CELL_STABLES); aimAt(gi, 300);
 printf("garlic focus kind %d idx %d (want 3 %d)\n", g->iFocusKind, g->iFocus, gi);
 g->KeyChar('e'); g->KeyChar('e'); g->Tick(16000,0); printf("garlic state %d, garlic count %d\n", g->iS.prop[gi].state, g->CountItem(IT_GARLIC));
 snap();
 int bi = findProp(PT_BARREL, CELL_STABLES); aimAt(bi, 260); printf("barrel focus %d %d\n", g->iFocusKind, g->iFocus==bi);
 g->KeyChar('e'); for(int k=0;k<40;k++) g->Tick(16000,0); printf("ui %d (cont %d) lid %d\n", g->iUi, UI_CONT, g->iS.prop[bi].anim);
 snap();
 g->KeyChar('r'); printf("after take all: ui %d salt %d\n", g->iUi, g->CountItem(IT_SALT));
 int ci = findProp(PT_CHAIR, CELL_STABLES); aimAt(ci, 200); printf("chair focus %d %d\n", g->iFocusKind, g->iFocus==ci);
 g->KeyChar('e'); g->Tick(16000,0); printf("sitting %d\n", g->iSitting); g->iS.third=1; for(int k=0;k<20;k++) g->Tick(16000,0); snap(); g->iS.third=0;
 g->KeyChar('e'); printf("sitting after E %d\n", g->iSitting);
 int pi = findProp(PT_POT, CELL_STABLES); aimAt(pi, 220); printf("pot focus %d %d\n", g->iFocusKind, g->iFocus==pi);
 g->AddItem(IT_SALMON, 1);
 g->KeyChar('e'); printf("ui %d (cook %d)\n", g->iUi, UI_COOK); g->iListSel = 2; g->KeyChar('e'); printf("salmon steak %d\n", g->CountItem(IT_SALMON_STEAK)); snap(); g->KeyChar(27);
 // market: take an item, trade
 int ii = findProp(PT_ITEM, 0); aimAt(ii, 70);
 // (the stalls face the well: from here the crosshair may rest on the next item on the counter; take that one)
 if (g->iFocusKind == 3 && g->iFocus >= 0 && g->iS.prop[g->iFocus].type == PT_ITEM) ii = g->iFocus;
 printf("item focus %d %d item %d\n", g->iFocusKind, g->iFocus==ii, g->iS.prop[ii].item);
 snap();
 g->KeyChar('e'); g->Tick(16000,0); printf("item gone %d\n", (g->iS.prop[ii].flags & PF_GONE)!=0);
 // grab: hold KEY_USE
 int i2 = findProp(PT_ITEM, 0); aimAt(i2, 70); if (g->iFocusKind == 3 && g->iFocus >= 0 && g->iS.prop[g->iFocus].type == PT_ITEM) i2 = g->iFocus; int y0=g->iS.prop[i2].y; g->KeyChar('e'); for(int k=0;k<30;k++) g->Tick(16000,KEY_USE); printf("grab %d y %d->%d\n", g->iGrab, y0, g->iS.prop[i2].y);
 g->iS.yaw = 0; for(int k=0;k<5;k++) g->Tick(16000,KEY_USE); for(int k=0;k<10;k++) g->Tick(16000,0); printf("still held after release: %d\n", g->iGrab); g->KeyChar('e'); for(int k=0;k<40;k++) g->Tick(16000,0); printf("dropped grab %d y %d floor %d\n", g->iGrab, g->iS.prop[i2].y, g->iR->GroundAt(g->iS.prop[i2].x,g->iS.prop[i2].z));
 // merchant
 for (int e=0;e<g->iS.ne;e++) if (g->iS.e[e].role==ROLE_MERCHANT){ Ent& m=g->iS.e[e]; g->EnterCell(0, m.x, m.z - 200, 0); g->iFadeUs=0; for(int k=0;k<3;k++) g->Tick(16000,0); printf("merchant focus %d %d\n", g->iFocusKind, g->iFocus==e); g->KeyChar('e'); printf("ui %d (trade %d)\n", g->iUi, UI_TRADE); int gold=g->iS.gold; g->KeyChar('e'); printf("gold %d -> %d\n", gold, g->iS.gold); snap(); g->KeyChar(27); break; }
 fclose(f); printf("shots %d\n", shots); }
