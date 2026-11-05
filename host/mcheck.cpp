#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static void chk(const char* nm, int i, const Mesh& m){ if (m.vstride && (m.dropped || m.nf >= m.fstride)) printf("FULL %s[%d]: nv %d/%d nf %d/%d\n", nm, i, m.nv, m.vstride, m.nf, m.fstride); }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 for(int i=0;i<IN_COUNT;i++) chk("iMIn",i,g->iMIn[i]);
 for(int i=0;i<69;i++) chk("iMCity",i,g->iMCity[i]);
 for(int i=0;i<IM_COUNT;i++) chk("iMItem",i,g->iMItem[i]);
 for(int i=0;i<7;i++){ chk("human",i,g->iMHuman[i]); chk("lod",i,g->iMHumanLod[i]); }
 chk("player",0,g->iMPlayer[0]); chk("player",1,g->iMPlayer[1]); chk("hand",0,g->iMHand); chk("handopen",0,g->iMHandOpen);
 chk("flame",0,g->iMFlame); chk("chest",0,g->iMChest); chk("wolf",0,g->iMWolf); chk("dragon",0,g->iMDragon); chk("horse",0,g->iMHorse);
 for(int i=0;i<ST_COUNT+3;i++) chk("st",i,g->iR->iMSt[i]);
 printf("parts dropped: %d\n", g->iR->iMeshDropped); }
