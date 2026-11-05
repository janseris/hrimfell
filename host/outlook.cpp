#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// a view outdoors from Skyrim world (x, y) towards (tx, ty), the eye `up` units over the floor there (Skyrim units),
// pitch p, hour h -> the ppm named last.  outlook x y tx ty up pitch hour out.ppm
static u16 out[640*200];
static int WorldX(double aX) { return (int)((aX + 233472) * 3.6576); }
static int WorldZ(double aY) { return (int)((aY + 176128) * 3.6576); }
int main(int argc, char** argv)
 { int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY; g->iHideView = 1;
 g->iR->SetOutputMode(2); g->iS.timeOfDay = atoi(argv[7]) * 60000;
 int wx = WorldX(atof(argv[1])), wz = WorldZ(atof(argv[2])), twx = WorldX(atof(argv[3])), twz = WorldZ(atof(argv[4]));
 int yaw = r3d_atan2(twx - wx, twz - wz);
 g->EnterCell(0, wx, wz, yaw); g->iFadeUs = 0; g->iUi = UI_PLAY;
 for (int t = 0; t < 4; t++) g->Tick(16000, 0);
 g->iS.px = wx; g->iS.pz = wz; g->iS.yaw = yaw; g->iS.pitch = atoi(argv[6]);
 int fl = g->FloorAt(0, wx, wz, g->iR->GroundAt(wx, wz) + 40 * METER);
 if (argc > 9) fl = g->FloorAt(0, wx, wz, g->iR->GroundAt(wx, wz) + atoi(argv[9]) * METER);
 g->iS.py = fl + atoi(argv[5]) * METER / 70; g->iS.vy = 0;
 g->iFadeUs = 0; g->Render(out, 640);
 FILE* fo = fopen(argv[8], "wb"); fprintf(fo, "P6 640 200 255\n");
 for (int i = 0; i < 640 * 200; i++) { u16 q = out[i]; unsigned char c[3] = { (unsigned char)((q >> 8) & 0xF8), (unsigned char)((q >> 3) & 0xFC), (unsigned char)((q << 3) & 0xF8) }; fwrite(c, 1, 3, fo); }
 fclose(fo); printf("floor %d ground %d (m over ground %.1f), dropped %d\n", fl, g->iR->GroundAt(wx, wz), (fl - g->iR->GroundAt(wx, wz)) / 256.0, g->iR->iMeshDropped); return 0; }
