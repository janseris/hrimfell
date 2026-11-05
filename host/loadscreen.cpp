#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// (v0.4.74) the loading screen: with loads put off (as the phone's app has it), a door into another area's file, the
// map's teleport, the travel list: a frame of the loading screen, then the place -> shots/loadscreen.ppm
static u16 out[640*200];
static FILE* f; static int fails = 0;
static void Check(int ok, const char* w) { printf("%s %s\n", ok ? "  ok  " : "  FAIL", w); if (!ok) fails++; }
static void Shot(Game* g) { g->Render(out, 640); for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); } }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY; g->iHideView=0;
 g->iS.dun[FH_GREETED] = 1;
 f=fopen("shots/loadscreen.ppm","wb"); fprintf(f,"P6 640 600 255\n");
 g->iDeferLoads = 1;
 // 1. a door into Forelhost (another area's file)
 const DoorLink& d = g->Door(26);
 g->EnterCell(0, d.ox, d.oz, d.oyaw); for (int t = 0; t < 4; t++) { g->iUi = UI_PLAY; g->Tick(50000, 0); }
 int area0 = g->iAreaCur;
 g->UseDoor(26);
 Check(g->iLoadPhase == 1 && g->iS.cell == 0, "the door: put off, still outside");
 g->Tick(50000, 0);
 Check(g->iS.cell == 0, "nothing moves till the loading screen is shown");
 Shot(g);
 Check(g->iLoadPhase == 2, "the loading screen drawn");
 g->Tick(50000, 0);
 Check(g->iS.cell == CELL_FHSTRONG && g->iAreaCur == 3 && g->iLoadPhase == 0, "then inside, Forelhost's file in");
 Shot(g);
 // 2. the map: T to Riften's forest (no file of its own outside here: straight there)
 g->iUi = UI_MAP; g->iMapCX = (int)(((long long)170000 + 233472) * 36576 / 10000); g->iMapCZ = (int)(((long long)-90000 + 176128) * 36576 / 10000);
 g->KeyChar('t');
 printf("    map T: phase %d cell %d area %d\n", g->iLoadPhase, g->iS.cell, g->iAreaCur);
 if (g->iLoadPhase) { g->Render(out, 640); g->Tick(50000, 0); }
 Check(g->iS.cell == 0 && g->iUi == UI_PLAY, "the map's T: out on the land");
 int dx = (g->iS.px - g->iMapCX) / METER, dz = (g->iS.pz - g->iMapCZ) / METER;
 printf("    arrived %d %d m from the cross, feet %d\n", dx, dz, (g->iS.py - 410 - g->iR->GroundAt(g->iS.px, g->iS.pz)) * 100 / METER);
 Check(dx * dx + dz * dz < 25, "where the cross was");
 for (int t = 0; t < 10; t++) g->Tick(50000, 0);
 Shot(g);
 fclose(f);
 printf("%s (%d failed)\n", fails ? "FAILED" : "all passed", fails); return fails != 0; }
