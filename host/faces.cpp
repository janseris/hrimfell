#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static u16 out[640*200];
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 R3D& r=*g->iR; r.SetOutputMode(2);
 int cx = 62 * CELL, cz = 30 * CELL; int gy = r.GroundAt(cx, cz);
 static const u8 c[32] = { 86,70,52,6, 120,40,36,6, 222,180,146,7, 120,124,128,1, 90,56,30,4, 50,36,28,4, 110,80,50,4, 0,0,0,0 };
 u8 s[32]; for(int i=0;i<32;i++) s[i]=c[i];
 FILE* f=fopen("shots/faces.ppm","wb"); fprintf(f,"P6 640 400 255\n");
 for (int pass=0; pass<2; pass++){
 r.SetCamera(cx, gy + 440, cz - (pass?3:2) * METER, 0, -2); r.BeginFrame(out, 640);
 int kinds[4]={0,4,1,2};
 for (int i = 0; i < 4; i++){ int x = cx + (i*2 - 3) * METER*45/100; int yaw = pass ? (i*200+300) : 512+ (i&1? 90:-90)*0;
   r.DrawMesh(g->iMHuman[kinds[i]], 0, x, r.GroundAt(x, cz), cz, yaw, 256, s, 0); }
 for(int i=0;i<640*200;i++){u16 p=out[i]; unsigned char cc[3]={(unsigned char)((p>>8)&0xF8),(unsigned char)((p>>3)&0xFC),(unsigned char)((p<<3)&0xF8)}; fwrite(cc,1,3,f);} }
 fclose(f);}
