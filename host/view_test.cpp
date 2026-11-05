#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static u16 out[640*200];
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->iUi=UI_PLAY;
 FILE* f=fopen("shots/view.ppm","wb"); fprintf(f,"P6 640 1000 255\n");
 int shots[5]={0,100000,200000,300000,400000};
 for(int s=0;s<5;s++){ g->iS.px=62*CELL; g->iS.pz=36*CELL; g->iS.py=g->iR->GroundAt(g->iS.px,g->iS.pz)+410; g->iS.yaw=0; g->iS.pitch=0; g->iS.drawn=1; g->iDrawUs=0; g->iS.third=0;
  g->Tick(10000,0); g->iSwingUs = s? 450000-shots[s] : 0; g->Render(out,640);
  for(int i=0;i<640*200;i++){u16 p=out[i]; unsigned char c[3]={(unsigned char)((p>>8)&0xF8),(unsigned char)((p>>3)&0xFC),(unsigned char)((p<<3)&0xF8)}; fwrite(c,1,3,f);} }
 fclose(f);
 // inventory weapons + apparel
 g->KeyChar('i'); g->iInvCat=2; g->iInvCol=1; g->iInvSel=0; g->Tick(10000,0); g->Render(out,640);
 f=fopen("shots/inv.ppm","wb"); fprintf(f,"P6 640 400 255\n");
 for(int i=0;i<640*200;i++){u16 p=out[i]; unsigned char c[3]={(unsigned char)((p>>8)&0xF8),(unsigned char)((p>>3)&0xFC),(unsigned char)((p<<3)&0xF8)}; fwrite(c,1,3,f);}
 g->iInvCat=4; g->iInvSel=0; g->iR->SetOutputMode(1); g->Tick(10000,0); g->Render(out,640);
 for(int i=0;i<640*200;i++){u16 p=out[i]; unsigned char c[3]={(unsigned char)((p>>8)&0xF8),(unsigned char)((p>>3)&0xFC),(unsigned char)((p<<3)&0xF8)}; fwrite(c,1,3,f);}
 fclose(f);
}
