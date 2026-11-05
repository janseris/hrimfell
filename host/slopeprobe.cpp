#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 R3D& r=*g->iR; int x=(int)(atof(argv[1])*256), z=(int)(atof(argv[2])*256);
 int feet=r.GroundAt(x,z);
 for (int d=0; d<=8; d++){ int xx=x-d*64; printf("dx -%.2f ground %.2f floor %.2f blocked %d\n", d/4.0, r.GroundAt(xx,z)/256.0, g->FloorAt(0,xx,z,feet)/256.0, g->BlockedAt(0,xx,z,100,feet+70,448-70)); }
}
