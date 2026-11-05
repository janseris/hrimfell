#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 R3D& r=*g->iR;
 for (double z=1930.0; z<=1934.01; z+=0.25){ int x=(int)(atof(argv[1])*256), zz=(int)(z*256); int gr=r.GroundAt(x,zz), fl=g->FloorAt(0,x,zz,gr);
   printf("z %.2f ground %.2f floor %.2f blocked %d\n", z, gr/256.0, fl/256.0, g->BlockedAt(0,x,zz,100,fl+70,448-70)); }
}
