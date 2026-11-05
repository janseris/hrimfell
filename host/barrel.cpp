#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static u16 out[640*200];
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->iUi=UI_PLAY; g->iR->SetOutputMode(2); g->iHideView=1;
 FILE* f=fopen("shots/barrel.ppm","wb"); fprintf(f,"P6 640 600 255\n");
 int i; for(i=0;i<g->iS.nprop;i++) if(g->iS.prop[i].type==PT_BARREL && g->iS.prop[i].cell==0) break;
 Prop& p=g->iS.prop[i];
 int ang[3]={0,300,700};
 for(int s=0;s<3;s++){ int a=ang[s]; int dx=(r3d_sin(a)*230)>>14, dz=(r3d_cos(a)*230)>>14;
  g->EnterCell(0, p.x - dx, p.z - dz, a); g->iFadeUs=0; g->iS.pitch=-120; g->Tick(16000,0); g->iFadeUs=0; g->Render(out,640);
  for(int k=0;k<640*200;k++){u16 q=out[k]; unsigned char c[3]={(unsigned char)((q>>8)&0xF8),(unsigned char)((q>>3)&0xFC),(unsigned char)((q<<3)&0xF8)}; fwrite(c,1,3,f);} }
 fclose(f);}
