#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static u16 out[640*200];
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->iUi=UI_PLAY; g->iR->SetOutputMode(2); g->iHideView=1;
 FILE* f=fopen("shots/close.ppm","wb"); fprintf(f,"P6 640 400 255\n");
 int types[2]={PT_BARREL, PT_POT}; int cells[2]={0, CELL_STABLES};
 for(int s=0;s<2;s++){ int i; for(i=0;i<g->iS.nprop;i++) if(g->iS.prop[i].type==types[s] && g->iS.prop[i].cell==cells[s]) break;
  Prop& p=g->iS.prop[i]; g->EnterCell(p.cell, p.x - 200, p.z - 150, 0); g->iFadeUs=0; g->iS.yaw = r3d_atan2(200,150); g->iS.pitch=-45; g->Tick(16000,0); g->iFadeUs=0; g->Render(out,640);
  for(int k=0;k<640*200;k++){u16 q=out[k]; unsigned char c[3]={(unsigned char)((q>>8)&0xF8),(unsigned char)((q>>3)&0xFC),(unsigned char)((q<<3)&0xF8)}; fwrite(c,1,3,f);} }
 fclose(f);}
