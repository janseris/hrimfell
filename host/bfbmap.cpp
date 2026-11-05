#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>
int GameBarrowOutX(int a, int b); int GameBarrowOutZ(int a, int b); int GameTowerX(); int GameTowerZ();
// dumps floor heights, blocking and what can be reached from the tower / the doors over the barrow's area (0.5 m)
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 R3D& r=*g->iR; int S=METER/2;
 int ox=GameBarrowOutX(0,0)-60*METER, oz=GameBarrowOutZ(0,0)-60*METER; int W=440, H=240;
 std::vector<int> fl(W*H), bl(W*H), rt(W*H,0), rd(W*H,0);
 for(int j=0;j<H;j++) for(int i=0;i<W;i++){ int x=ox+i*S,z=oz+j*S; int gr=r.GroundAt(x,z); fl[j*W+i]=g->FloorAt(0,x,z,gr); bl[j*W+i]=g->BlockedAt(0,x,z,100,fl[j*W+i]+70,448-70); }
 auto flood=[&](int x0,int z0,std::vector<int>& m){ std::vector<int> q; int s=((z0-oz)/S)*W+(x0-ox)/S; if(s<0||s>=W*H) return; q.push_back(s); m[s]=1;
   for(size_t h=0;h<q.size();h++){ int c=q[h],ci=c%W,cj=c/W; for(int d=0;d<4;d++){ int ni=ci+(d==0)-(d==1), nj=cj+(d==2)-(d==3); if(ni<0||nj<0||ni>=W||nj>=H) continue; int nn=nj*W+ni; if(m[nn]||bl[nn]) continue;
     int up=fl[nn]-fl[c]; if (&m==&rd) up=-up; if(up>S*11/10||up<-2*METER) continue; m[nn]=1; q.push_back(nn);} } };
 flood(GameTowerX()-14*METER, GameTowerZ()+6*METER, rt); flood(GameBarrowOutX(0,3), GameBarrowOutZ(0,3), rd);
 FILE* f=fopen("/tmp/claude-0/bfbmap.bin","wb"); int hd[4]={W,H,ox,oz}; fwrite(hd,4,4,f); fwrite(fl.data(),4,W*H,f); fwrite(bl.data(),4,W*H,f); fwrite(rt.data(),4,W*H,f); fwrite(rd.data(),4,W*H,f); fclose(f);
 printf("tower (%d,%d) door (%d,%d) cells\n", (GameTowerX()-14*METER-ox)/S,(GameTowerZ()+6*METER-oz)/S,(GameBarrowOutX(0,3)-ox)/S,(GameBarrowOutZ(0,3)-oz)/S);
}
