#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static u16 out[640*200];
static void sl(u8* s,int i,int r,int g,int b){s[i*4]=r;s[i*4+1]=g;s[i*4+2]=b;s[i*4+3]=1;}
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 R3D& r=*g->iR;
 printf("verts: man %d woman %d elder %d wight %d troll %d | faces man %d\n", g->iMHuman[0].nv, g->iMHuman[4].nv, g->iMHuman[1].nv, g->iMHuman[2].nv, g->iMHuman[3].nv, g->iMHuman[0].nf);
 int cx=62*CELL, cz=30*CELL; int gy=r.GroundAt(cx,cz);
 u8 s[32]; sl(s,0,86,70,52); sl(s,1,120,40,36); sl(s,2,222,180,146); sl(s,3,120,124,128); sl(s,4,80,56,36); sl(s,5,50,36,28); sl(s,6,110,80,50);
 FILE* f=fopen("shots/heads.ppm","wb"); fprintf(f,"P6 640 600 255\n");
 int dist[3]={3,6,10}; int mode[3]={2,0,0};
 for(int k=0;k<3;k++){ r.SetOutputMode(mode[k]); for(int i=0;i<640*200;i++) out[i]=0;
  r.SetCamera(cx, gy+420, cz-dist[k]*METER, 0, -4); r.BeginFrame(out,640);
  for(int j=0;j<3;j++){ int x=cx+(j-1)*METER*(k?2:1); int yaw = j==0? 512 : (j==1? 512+120 : 512-150); r.DrawMesh(g->iMHuman[j==2?4:(j==1?1:0)], 0, x, r.GroundAt(x,cz), cz, yaw, 256, s, 0);} 
  for(int i=0;i<640*200;i++){u16 p=out[i]; unsigned char c[3]={(unsigned char)((p>>8)&0xF8),(unsigned char)((p>>3)&0xFC),(unsigned char)((p<<3)&0xF8)}; fwrite(c,1,3,f);} }
 fclose(f);}
