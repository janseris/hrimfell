#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// screens of the map and the HUD near Riverwood: uishot mode out.ppm  (mode: world0..3 local hud0..hud4 (banner time s))
static u16 out[640*200];
int GameRiverwoodX(); int GameRiverwoodZ();
int main(int argc, char** argv)
 { int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 g->iR->SetOutputMode(2); g->iS.timeOfDay = 13 * 60000;
 int x = GameRiverwoodX() - 120 * METER, z = GameRiverwoodZ() - 20 * METER;
 g->EnterCell(0, x, z, 256); g->iFadeUs = 0;
 for (int t = 0; t < 4; t++) g->Tick(16000, 0);
 g->iS.spare[1] = 1;                                       // Whiterun found
 const char* m = argv[1];
 if (!strncmp(m, "world", 5)) { g->iUi = UI_MAP; g->iMapLocal = 0; g->iMapWZ = atoi(m + 5); g->iMapCX = g->iS.px; g->iMapCZ = g->iS.pz; }
 else if (!strcmp(m, "localbfb")) { int GameBarrowX(); int GameBarrowZ(); g->iUi = UI_MAP; g->iMapLocal = 1; g->iMapCX = GameBarrowX() + 40 * METER; g->iMapCZ = GameBarrowZ(); g->EnterCell(0, g->iMapCX, g->iMapCZ, 0); g->iFadeUs = 0; g->iUi = UI_MAP; g->iMapLocal = 1; }
 else if (!strncmp(m, "hurt", 4)) { g->iHurtUs = 500000; g->iHurtFrom = (g->iS.yaw + atoi(m + 4)) & 1023; g->Msg("Hit by an arrow"); g->iS.hp = 60 << 8; g->iBarUs[0] = 1; }
 else if (!strcmp(m, "local")) { g->iUi = UI_MAP; g->iMapLocal = 1; g->iMapCX = GameRiverwoodX(); g->iMapCZ = GameRiverwoodZ(); }
 else if (!strncmp(m, "hud", 3))
  {
  g->iS.px = GameRiverwoodX() - 60 * METER; g->iS.pz = GameRiverwoodZ(); g->iS.yaw = 256; g->iR->FocusLand(g->iS.px, g->iS.pz);
  g->iS.py = g->FloorAt(0, g->iS.px, g->iS.pz, g->iR->GroundAt(g->iS.px, g->iS.pz) + 2 * METER) + 410;
  g->Tick(16000, 0); printf("found %d banner %d loc %d\n", g->iS.spare[1], g->iDiscUs, g->iDiscLoc);
  g->iDiscUs = DISC_US - atoi(m + 3) * 1000000 / 10;           // hudN: N tenths of a second in
  }
 g->Render(out, 640);
 FILE* fo = fopen(argv[2], "wb"); fprintf(fo, "P6 640 200 255\n");
 for (int i = 0; i < 640 * 200; i++) { u16 q = out[i]; unsigned char c[3] = { (unsigned char)((q >> 8) & 0xF8), (unsigned char)((q >> 3) & 0xFC), (unsigned char)((q << 3) & 0xF8) }; fwrite(c, 1, 3, fo); }
 fclose(fo); return 0; }
