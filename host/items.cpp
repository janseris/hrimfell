#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static u16 out[640*200];
static FILE* f;
const u8* GameItemCol(int aId);
const ItemDef& GameItem(int aId);
static void dump(){ for(int i=0;i<640*200;i++){u16 p=out[i]; unsigned char c[3]={(unsigned char)((p>>8)&0xF8),(unsigned char)((p>>3)&0xFC),(unsigned char)((p<<3)&0xF8)}; fwrite(c,1,3,f);} }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 R3D& r=*g->iR; r.SetOutputMode(2);
 f=fopen("shots/items.ppm","wb"); fprintf(f,"P6 640 800 255\n");
 int ids[3][6] = { {IT_BREAD, IT_WOOD_BOWL, IT_TANKARD, IT_SILVER_RING, IT_GOLD_NECKLACE, IT_POTATO},
                   {IT_APPLE, IT_CLAY_POT, IT_BERRY_WINE, IT_JUG, IT_PLATE, IT_CHARRED_MEAT},
                   {IT_GOLD_RING, IT_SILVER_NECKLACE, IT_CHEESE, IT_GARLIC_BREAD, IT_VEG_SOUP, IT_CABBAGE} };
 g->EnterCell(CELL_STABLES, 100*METER, 100*METER - 2*METER, 0);
 r.iCell = CELL_STABLES;
 for (int row = 0; row < 3; row++) {
  int cy = 100*METER + 60*METER/100;   // floor + 60 cm
  r.SetCamera(100*METER, 100*METER + 45*METER/100, 100*METER - 45*METER/100, 0, -110);
  r.BeginFrame(out,640);
  for (int k = 0; k < 6; k++) {
   int id = ids[row][k]; const ItemDef& d = GameItem(id); const u8* c = GameItemCol(id);
   u8 sl[12]; for (int j=0;j<3;j++){ sl[j*4]=c[j*3]; sl[j*4+1]=c[j*3+1]; sl[j*4+2]=c[j*3+2]; sl[j*4+3]=1; }
   const short* rest = g->iItemRest + d.mesh*3;
   int x = 100*METER + (k-2)*METER*12/100 - METER*6/100;
   r.DrawMeshRot(g->iMItem[d.mesh], 0, x, 100*METER + rest[1], 100*METER, 100 + row*200, rest[0], rest[2], sl, 0);
  }
  dump(); (void)cy;
 }
 // a barrel of potatoes, its menu
 for (int i = 0; i < g->iS.nprop; i++) if (g->iS.prop[i].cell && g->iS.prop[i].type == PT_BARREL) { g->UseProp(i); }
 g->iUi = UI_CONT; g->iInvCol = 1; g->iInvSel = 0;
 for (int i = 0; i < g->iS.nprop; i++) if (g->iS.prop[i].cell && g->iS.prop[i].type == PT_SACK) { g->iOpenCont = g->iS.prop[i].item; g->iOpenProp = i; }
 g->Tick(16000,0); g->Render(out,640); dump();
 fclose(f);
}
