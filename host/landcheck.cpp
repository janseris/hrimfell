#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// the land as the game reads it (a window round the player, the rest coarse) against the whole land the baker
// built (host/land_samples.txt): exact in the window; how far off the coarse land is outside it
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 R3D& r=*g->iR; FILE* f=fopen("host/land_samples.txt","r"); int x,z,h,m,cnt=0,bad=0,badm=0; long far=0,farmax=0,farn=0,mfar=0;
 printf("land baked %d, loaded %d, window %d x %d, tiles read so far %d\n", r.iLandBaked, r.iLandLoaded, r.iWW, r.iWH, r.iLandLoads);
 while(fscanf(f,"%d %d %d %d",&x,&z,&h,&m)==4){
   // far: away from the window first
   if (!r.InWin(x/CELL,z/CELL)) { int d=abs(r.GroundAt(x,z)-h); far+=d; if(d>farmax)farmax=d; farn++; if (r.MaterialAt(x,z)!=m) mfar++; }
   r.FocusLand(x,z); cnt++;
   if (r.GroundAt(x,z)!=h){ if(bad<5)printf("  height differs at %d,%d: %d vs %d\n",x,z,r.GroundAt(x,z),h); bad++; }
   if (r.MaterialAt(x,z)!=m) badm++; }
 printf("points %d: in the window, heights wrong %d, ground wrong %d; far (coarse) %ld points: mean error %.2f m, max %.2f m, ground differs %ld\n",
   cnt,bad,badm,farn,farn?far/(double)farn/METER:0.0,farmax/(double)METER,mfar);
 printf("tiles read %d\n", r.iLandLoads); return bad||badm; }
