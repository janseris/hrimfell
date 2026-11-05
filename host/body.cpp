#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static u16 out[640*200];
static void save(const char* fn){ FILE* f=fopen(fn,"wb"); fprintf(f,"P6 640 200 255\n");
 for(int i=0;i<640*200;i++){u16 p=out[i]; unsigned char cc[3]={(unsigned char)((p>>8)&0xF8),(unsigned char)((p>>3)&0xFC),(unsigned char)((p<<3)&0xF8)}; fwrite(cc,1,3,f);} fclose(f);}
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 R3D& r=*g->iR; r.SetOutputMode(2);
 printf("arena used %d of %d\n", (int)(r.iArena-mem), n);
 int lod=argc>1?atoi(argv[1]):0; int fr=argc>2?atoi(argv[2]):0; int dist=argc>3?atoi(argv[3]):3;
 int cx = 62 * CELL, cz = 30 * CELL; int gy = r.GroundAt(cx, cz);
 static const u8 c[32] = { 86,70,52,6, 120,40,36,6, 222,180,146,7, 120,124,128,1, 80,56,36,4, 50,36,28,4, 110,80,50,4, 0,0,0,0 };
 u8 s[32]; for(int i=0;i<32;i++) s[i]=c[i];
 r.SetCamera(cx, gy + 300, cz - dist * METER, 0, 0); r.BeginFrame(out, 640);
 const Mesh* ms[6]={ lod? &g->iMHumanLod[0]:&g->iMHuman[0], lod?&g->iMHumanLod[4]:&g->iMHuman[4], lod?&g->iMHumanLod[1]:&g->iMHuman[1], &g->iMPlayer[1], lod?&g->iMHumanLod[3]:&g->iMHuman[3], &g->iMPlayer[0]};
 int yaws[6]={512,512+256,512-180,512+90,512,0};
 for (int i = 0; i < 6; i++){ int x = cx + (i*2 - 5) * METER*5/10 * dist/3; int z=cz;
   r.DrawMesh(*ms[i], fr, x, r.GroundAt(x, z), z, yaws[i], i==4?180:256, s, 0); }
 save("shots/body.ppm");}
