#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// Skyrim world (x, y) per line on stdin -> the land's height there in Skyrim units (the game's land and its offset)
static int WorldX(int aX) { return (int)(((long long)aX + 233472) * 36576 / 10000); }
static int WorldZ(int aY) { return (int)(((long long)aY + 176128) * 36576 / 10000); }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 R3D& r = *g->iR; double x, y;
 while (scanf("%lf %lf", &x, &y) == 2)
  { int ex = WorldX((int)x), ez = WorldZ((int)y); r.FocusLand(ex, ez); int h = r.GroundAt(ex, ez) - r.iLandOffset;
    printf("%.1f\n", h / 256.0 * 100.0 / 1.42875); }
 return 0; }
