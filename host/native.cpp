#include "r3d.h"
#include <stdio.h>
#include <stdlib.h>
int main(){ int n=R3D::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); R3D* r=(R3D*)calloc(1,sizeof(R3D)); r->Init(mem);
 static u16 out[640*200];
 FILE* f=fopen("shots/modes.ppm","wb"); fprintf(f,"P6 640 800 255\n");
 for(int m=0;m<4;m++){ for(int i=0;i<640*200;i++) out[i]=0xF81F;
 r->SetFreeMode(0,0); r->SetNative(m); r->iCamX=64*1024; r->iCamZ=48*1024; r->iCamY=r->GroundAt(r->iCamX,r->iCamZ)+448; r->iYaw=0;
 for(int k=0;k<4;k++){ r->Tick(50000,0); r->Render(out,640);}
 for(int i=0;i<640*200;i++){u16 p=out[i]; unsigned char c[3]={(unsigned char)((p>>8)&0xF8),(unsigned char)((p>>3)&0xFC),(unsigned char)((p<<3)&0xF8)}; fwrite(c,1,3,f);} }
 fclose(f);}
