#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 R3D& r=*g->iR;
 int x=(int)(3327.7*256), z=(int)(1911.9*256);
 int feet=r.GroundAt(x,z);
 for (int dz=-3; dz<=6; dz++){ int xx=x-dz*32, zz=z; printf("dx %d ground %d floor %d blocked %d stat %d prop %d\n", -dz*32, r.GroundAt(xx,zz), g->FloorAt(0,xx,zz,feet), g->BlockedAt(0,xx,zz,100,feet+7*METER/25, 7*METER/4-7*METER/25), r.Blocked(xx,zz,100,feet+70,feet+448), g->PropBlocked(0,xx,zz,100,feet+70)); }
}
