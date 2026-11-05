#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int GameKitOutBlocked(const Game& g, int aX, int aZ, int aR, int aFeetY, int aOff);
static int WorldX(double aX) { return (int)((aX + 233472) * 3.6576); }
static int WorldZ(double aY) { return (int)((aY + 176128) * 3.6576); }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame();
 R3D& r=*g->iR; int off=r.iLandOffset; double CX=atof(getenv("CX")), CY=atof(getenv("CY")), CZ=atof(getenv("CZ")); int feet=(int)(CZ*1.42875*256/100)+off;
 for (int dy=100; dy>=-300; dy-=50){ for (int dx=-300; dx<=300; dx+=50){ int x=WorldX(CX+dx), z=WorldZ(CY+dy); r.FocusLand(x,z);
   int f=g->FloorAt(0,x,z,feet+METER/4); int b=r.Blocked(x,z,100,f+7*METER/25,f+7*METER/4); int k=GameKitOutBlocked(*g,x,z,100,f+7*METER/25,off);
   printf("%s", b ? " R" : k ? " K" : (abs(f-feet)<METER/2 ? " ." : " o")); } printf("\n"); }
 return 0; }
