#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static u16 out[640*200];
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->iUi=UI_PLAY;
 FILE* f=fopen("shots/inv.ppm","wb"); fprintf(f,"P6 640 800 255\n");
 int cats[4]={2,3,4,1}; int modes[4]={0,0,1,2};
 for(int k=0;k<4;k++){ g->iR->SetOutputMode(modes[k]); g->iUi=UI_INV; g->iInvCat=cats[k]; g->iInvCol=1; g->iInvSel=k==3?6:0; g->iPreviewAngle=150;
   g->Tick(10000,0); g->Render(out,640);
   for(int i=0;i<640*200;i++){u16 p=out[i]; unsigned char c[3]={(unsigned char)((p>>8)&0xF8),(unsigned char)((p>>3)&0xFC),(unsigned char)((p<<3)&0xF8)}; fwrite(c,1,3,f);} }
 fclose(f);}
