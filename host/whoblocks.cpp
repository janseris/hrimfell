#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 R3D& r=*g->iR; int x=(int)(atof(argv[1])*256), z=(int)(atof(argv[2])*256); int feet=r.GroundAt(x,z)+70;
 printf("blocked %d\n", r.Blocked(x,z,100,feet,feet+378));
 for (int i=0;i<r.iObjCount;i++){ Obj& o=r.iObj[i]; int sx=o.x; o.x+= 100000; int b=r.Blocked(x,z,100,feet,feet+378); o.x=sx; if(!b) printf("obj %d type %d at (%.1f,%.1f) y %.2f yaw %d scale %d\n",i,o.type,o.x/256.0,o.z/256.0,o.y/256.0,o.yaw,o.scale); }
}
