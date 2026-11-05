#include <math.h>
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// (v0.4.72) Forelhost's books and notes: each where Skyrim leaves it, seen, opened, its pages read -> shots/fhbooks.ppm
const ItemDef& GameItem(int aId);
static u16 out[640*200];
static Game* g; static FILE* f; static int fails = 0, nshots = 0;
static void Check(int ok, const char* w) { printf("%s %s\n", ok ? "  ok  " : "  FAIL", w); if (!ok) fails++; }
static void Shot() { g->Render(out, 640); nshots++; for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); } }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY; g->iHideView = 0;
 f=fopen("shots/fhbooks.ppm","wb"); fprintf(f,"P6 640 %d 255\n", 200 * 18);
 static const int K[] = { IT_FH_JOURNAL, IT_FH_NOTE_FRODA, IT_FH_NOTE_RAHGOT, IT_FH_NOTE_ORDERS, IT_FH_NOTE_POISON, IT_FH_NOTE_DEATH, IT_BK_EXODUS, IT_TOME_FLAMES };
 for (int k = 0; k < 8; k++)
    {
    int b = -1; for (int i = 0; i < g->iS.nprop; i++) if (g->iS.prop[i].type == PT_ITEM && g->iS.prop[i].item == K[k]) b = i;
    char w[120]; sprintf(w, "%s lies in cell %d", GameItem(K[k]).name, b >= 0 ? g->iS.prop[b].cell : -1); Check(b >= 0, w);
    if (b < 0) continue;
    Prop p = g->iS.prop[b];
    g->EnterCell(p.cell, p.x - 9 * METER / 10, p.z, 256); g->iFadeUs = 0; g->iS.py = p.y + METER / 2; g->iS.vy = 0;
    for (int t = 0; t < 10; t++) g->Tick(16000, 0);
    g->iS.yaw = r3d_atan2(p.x - g->iS.px, p.z - g->iS.pz);
    int dy = p.y - (g->iS.py - 30); int hd = (int)sqrt((double)(p.x - g->iS.px) * (p.x - g->iS.px) + (double)(p.z - g->iS.pz) * (p.z - g->iS.pz)); g->iS.pitch = r3d_atan2(dy, hd); if (g->iS.pitch > 512) g->iS.pitch -= 1024;
    g->iUi = UI_PLAY; g->Tick(16000, 0); g->FindFocus(); g->FindPropFocus();
    const char* verb = ""; const char* name = ""; char info[80]; int red = 0; g->PropText(b, verb, name, info, red);
    printf("    prop at feet %+d cm, prompt: %s %s (focus %d) player %+.2f %+.2f m from the spot\n", (p.y - g->iS.py) * 100 / METER, verb, name, g->iFocus, (g->iS.px - (p.x - 9 * METER / 10)) / (double)METER, (g->iS.pz - p.z) / (double)METER);
    if (g->iFocus >= 0 && g->iFocus < g->iS.nprop) { const Prop& q = g->iS.prop[g->iFocus]; printf("    focus prop type %d at %+.2f %+.2f m y %+.2f count %d model %d\n", q.type, (q.x - p.x) / (double)METER, (q.z - p.z) / (double)METER, (q.y - p.y) / (double)METER, q.count, q.model); }
    if (k == 1 || k == 6) Shot();
    g->KeyChar('e'); for (int t = 0; t < 3; t++) g->Tick(16000, 0);
    sprintf(w, "%s opens to read", GameItem(K[k]).name); Check(g->iUi == UI_BOOK, w);
    int pages = 0;
    while (g->iUi == UI_BOOK && pages < 40) { if (pages < 2 && (k == 0 || k == 1 || k == 6) && nshots < 18) Shot(); else g->Render(out, 640); pages++; g->KeyChar(' '); g->Tick(16000, 0); g->Render(out, 640); }
    printf("    %d pages\n", pages);
    g->iUi = UI_PLAY;
    }
 while (nshots < 18) { for (int i = 0; i < 640 * 200; i++) { unsigned char c[3] = { 0, 0, 0 }; fwrite(c, 1, 3, f); } nshots++; }
 fclose(f);
 printf("%s (%d failed)\n", fails ? "FAILED" : "all passed", fails); return fails != 0; }
