#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 R3D& r=*g->iR;
 for (int k=0;k<=8;k++){ int x,z; GameBarrowPathPt(k,x,z); printf("pt %d (%.1f,%.1f) ground %.2f\n",k,x/256.0,z/256.0,r.GroundAt(x,z)/256.0); }
 int x0,z0,x1,z1; GameBarrowPathPt(2,x0,z0); GameBarrowPathPt(3,x1,z1);
 for (int dz=-4; dz<=4; dz++) { int x=x0, z=z0+dz*64; printf("  dz %d blockedAt %d prop %d\n", dz, g->BlockedAt(0,x,z,100,r.GroundAt(x,z)), g->PropBlocked(0,x,z,100,r.GroundAt(x,z))); }
 for (int s=0;s<=20;s++){ int x=x0+(x1-x0)*s/20, z=z0+(z1-z0)*s/20; printf("  s%d ground %.2f blocked %d\n", s, r.GroundAt(x,z)/256.0, r.Blocked(x,z,100, r.GroundAt(x,z)+64, r.GroundAt(x,z)+400)); }
}
