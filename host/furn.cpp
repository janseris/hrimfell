#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static Game* g;
static int Icm(int cm){ return 100*METER + cm*METER/100; }
// stand at (sx,sz) cm, look at prop i at height hy cm above its base
static void look(int i, int sx, int sz, int hy){ Prop& p=g->iS.prop[i];
  g->EnterCell(CELL_STABLES, Icm(sx), Icm(sz), 0); g->iFadeUs=0;
  int dx=p.x-g->iS.px, dz=p.z-g->iS.pz; g->iS.yaw = r3d_atan2(dx,dz) & 1023;
  int d=(int)__builtin_sqrt((double)dx*dx+(double)dz*dz); int dy=p.y+hy*METER/100-g->iS.py; int pt=r3d_atan2(dy,d); if(pt>512)pt-=1024; g->iS.pitch=pt;
  for(int k=0;k<3;k++) g->Tick(16000,0); }
static int find(int type,int nth){ for(int i=0;i<g->iS.nprop;i++) if(g->iS.prop[i].type==type && g->iS.prop[i].cell==CELL_STABLES && nth--==0) return i; return -1; }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 struct T { int type, nth, sx, sz, hy; const char* nm; } t[] = {
  {PT_CHEST,0, 560,150, 30,"chest"}, {PT_CASE,0, 300,-60, 85,"case"}, {PT_DRESSER,0, 640,120, 70,"dresser1"},
  {PT_DRESSER,1, 300,-280, 70,"dresser2"}, {PT_CUPBOARD,0, -300,250, 110,"wardrobe"} };
 for (int k=0;k<5;k++){ int i=find(t[k].type,t[k].nth); look(i,t[k].sx,t[k].sz,t[k].hy);
   int fk=g->iFocusKind, fi=g->iFocus; g->KeyChar('e'); for(int j=0;j<40;j++) g->Tick(16000,0);
   int ui=g->iUi, an=g->iS.prop[i].anim; int before=g->iS.ninv; g->KeyChar('e'); int after=g->iS.ninv;
   g->KeyChar(9); for(int j=0;j<8;j++) g->Tick(16000,0);
   printf("%-9s focus %d/%d ui %d anim %d took %d after tab ui %d anim %d\n", t[k].nm, fk, fi==i, ui, an, after-before, g->iUi, g->iS.prop[i].anim); }
 // beds
 for (int b=0;b<3;b++){ int i=find(PT_BED,b); int sx = b==0?400:(b==1?520:-420), sz = b==0?300:(b==1?-200:-120); look(i,sx,sz,30);
   int fk=g->iFocusKind, fi=g->iFocus; int day=g->iS.day, tod=g->iS.timeOfDay; g->KeyChar('e'); int ui=g->iUi, sl=g->iSleeping, sel=g->iWaitSel; g->KeyChar('e');
   for(int j=0;j<700;j++) g->Tick(16000,0); printf("bed%d focus %d/%d ui %d sleeping %d hours %d -> ui %d, %d h passed\n", b, fk, fi==i, ui, sl, sel, g->iUi, ((g->iS.day-day)*1440000+g->iS.timeOfDay-tod)/60000); }
 // doors: blocked when closed?
 int d=find(PT_DOOR,0); look(d, 100, 200, 110); printf("door focus %d/%d state %d anim %d\n", g->iFocusKind, g->iFocus==d, g->iS.prop[d].state, g->iS.prop[d].anim);
 int open = g->BlockedAt(CELL_STABLES, Icm(210), Icm(205), 100, 100*METER);
 g->KeyChar('e'); for(int j=0;j<8;j++) g->Tick(16000,0);
 int shut = g->BlockedAt(CELL_STABLES, Icm(210), Icm(205), 100, 100*METER);
 printf("door state %d anim %d: doorway blocked open %d closed %d\n", g->iS.prop[d].state, g->iS.prop[d].anim, open, shut);
 // walk through the closed door: try
 g->EnterCell(CELL_STABLES, Icm(120), Icm(205), 256); g->iFadeUs=0; for(int j=0;j<120;j++) g->Tick(16000, KEY_FWD); printf("walked to x %d cm (door closed)\n", (g->iS.px-100*METER)*100/METER);
 look(d, 100, 200, 110); g->KeyChar('e'); for(int j=0;j<8;j++) g->Tick(16000,0);
 g->EnterCell(CELL_STABLES, Icm(120), Icm(205), 256); g->iFadeUs=0; for(int j=0;j<120;j++) g->Tick(16000, KEY_FWD); printf("walked to x %d cm (door open)\n", (g->iS.px-100*METER)*100/METER);
 // shelf drink and hare
 int nd=0; for(int i=0;i<g->iS.nprop;i++){ Prop&p=g->iS.prop[i]; if(p.cell==CELL_STABLES&&p.type==PT_ITEM&&(p.item==IT_BERRY_WINE||p.item==IT_CHARRED_MEAT)) nd++; } printf("drinks/meat props %d\n", nd);
 // tab out of menus
 g->KeyChar('t'); printf("wait ui %d\n", g->iUi); g->KeyChar(9); printf("after tab %d\n", g->iUi);
 g->KeyChar('i'); g->KeyChar(9); printf("inv tab -> %d\n", g->iUi);
 // reach every part of the north-east bedroom from its doorway: flood the walkable floor on a 20 cm grid
 {
  static unsigned char seen[60][50]; int qx[3000], qz[3000], qh=0, qt=0;
  for(int a=0;a<60;a++) for(int b=0;b<50;b++) seen[a][b]=0;
  // cells cover x 220..760 (27 cols) z 10..450 (22 rows) at 20 cm
  int sx=(240-220)/20, sz=(205-10)/20; seen[sx][sz]=1; qx[qt]=sx; qz[qt++]=sz;
  while(qh<qt){ int x=qx[qh], z=qz[qh++]; int d[8]={1,0,-1,0,0,1,0,-1}; for(int k=0;k<4;k++){ int nx=x+d[k*2], nz=z+d[k*2+1]; if(nx<0||nz<0||nx>=27||nz>=22||seen[nx][nz]) continue;
    if (g->BlockedAt(CELL_STABLES, Icm(220+nx*20), Icm(10+nz*20), 100, 100*METER)) { seen[nx][nz]=2; continue; } seen[nx][nz]=1; qx[qt]=nx; qz[qt++]=nz; } }
  printf("NE bedroom reachable cells %d\n", qt);
  for(int z=21;z>=0;z--){ for(int x=0;x<27;x++) putchar(seen[x][z]==1?'.':(seen[x][z]==2?'#':' ')); putchar('\n'); }
 }
}
