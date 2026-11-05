#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static u16 out[640*200];
static FILE* f;
static void dump(){ for(int i=0;i<640*200;i++){u16 p=out[i]; unsigned char c[3]={(unsigned char)((p>>8)&0xF8),(unsigned char)((p>>3)&0xFC),(unsigned char)((p<<3)&0xF8)}; fwrite(c,1,3,f);} }
static void sl(u8* s,int i,int r,int g,int b){s[i*4]=r;s[i*4+1]=g;s[i*4+2]=b;s[i*4+3]=1;}
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 R3D& r=*g->iR; r.SetOutputMode(2);  // 640 native for detail
 f=fopen("shots/models.ppm","wb"); fprintf(f,"P6 640 800 255\n");
 int cx=62*CELL, cz=30*CELL; int gy=r.GroundAt(cx,cz);
 u8 s[32];
 sl(s,0,86,70,52); sl(s,1,120,40,36); sl(s,2,222,180,146); sl(s,3,120,124,128); sl(s,4,80,56,36); sl(s,5,70,50,36); sl(s,6,110,80,50);
 int frames[4]={0,4,18,24};
 for(int row=0;row<4;row++){
  r.SetCamera(cx, gy+260, cz-5*METER, 0, -8); r.BeginFrame(out,640);
  for(int k=0;k<5;k++){ int x=cx+(k-2)*METER*3/2; int fr=frames[row]; if(k==4) {} 
    u8 t[32]; for(int i=0;i<32;i++) t[i]=s[i];
    if(k==1){ sl(t,1,96,102,120); sl(t,0,86,92,108); sl(t,4,226,226,222);} 
    if(k==2){ sl(t,1,84,86,80); sl(t,2,150,158,146); sl(t,4,64,64,60);} 
    if(k==3){ sl(t,0,120,120,106); sl(t,1,140,142,126); sl(t,2,132,140,118); sl(t,6,96,70,46);} 
    if(k==4){ sl(t,1,50,80,120); sl(t,4,200,170,100);} 
    int yaw = row==3 ? 0 : 512+100;
    r.DrawMesh(k==3? g->iMHuman[3] : g->iMHuman[k], fr, x, r.GroundAt(x,cz), cz + (k==3?2*METER:0), yaw, k==3?170:256, t, 0); }
  dump(); }
 fclose(f);
 // wolf + dragon
 f=fopen("shots/beasts.ppm","wb"); fprintf(f,"P6 640 400 255\n");
 u8 w[16]; sl(w,0,118,114,106); sl(w,1,74,70,66); sl(w,2,196,188,172);
 r.SetCamera(cx, gy+200, cz-4*METER, 0, -10); r.BeginFrame(out,640);
 for(int k=0;k<3;k++){ int x=cx+(k-1)*METER*2; r.DrawMesh(g->iMWolf, k*5, x, r.GroundAt(x,cz), cz, 256+k*200, 256, w, 0);} dump();
 u8 d[16]; sl(d,0,116,50,36); sl(d,1,176,128,86); sl(d,2,150,76,52); sl(d,3,52,34,28);
 r.SetCamera(cx, gy+6*METER, cz-26*METER, 0, 30); r.BeginFrame(out,640);
 r.DrawMesh(g->iMDragon, 4, cx-6*METER, gy+6*METER, cz, 200, 256, d, 0);
 r.DrawMesh(g->iMDragon, 18, cx+8*METER, r.GroundAt(cx+8*METER,cz), cz+4*METER, 800, 256, d, 0);
 dump(); fclose(f);
}
