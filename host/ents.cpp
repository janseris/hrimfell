#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame();
 R3D& r=*g->iR; int off=r.iLandOffset;
 for (int i=0;i<g->iS.ne;i++){ Ent& e=g->iS.e[i]; if (e.type!=E_BANDIT || e.cell) continue; r.FocusLand(e.x,e.z);
   int fl=g->FloorAt(0,e.x,e.z,e.y+METER/2); printf("bandit %d at sky (%.0f,%.0f) y %.0f floor %.0f ground %.0f\n", i, e.x/3.6576-233472, e.z/3.6576-176128, (e.y-off)/256.0*100/1.42875, (fl-off)/256.0*100/1.42875, (r.GroundAt(e.x,e.z)-off)/256.0*100/1.42875); }
 for (int t=0;t<120;t++) g->Tick(33000,0);
 for (int i=0;i<g->iS.ne;i++){ Ent& e=g->iS.e[i]; if (e.type!=E_BANDIT || e.cell) continue; printf("  later %d y %.0f\n", i, (e.y-off)/256.0*100/1.42875); }
 return 0; }
