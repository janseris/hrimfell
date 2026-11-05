#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int GameBarrowOutX(int a, int b); int GameBarrowOutZ(int a, int b);
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 R3D& r=*g->iR;
 printf("loaded %d gx %d gz %d off %d\n", r.iLandLoaded, r.iGX, r.iGZ, r.iLandOffset/METER);
 int cx=CITY_CX*CELL+CELL/2, cz=CITY_CZ*CELL+CELL/2;
 printf("city centre ground %d m water %d m\n", r.GroundAt(cx,cz)/METER, r.WaterAt(cx,cz)/METER);
 printf("player %d,%d m ground %d m\n", g->iS.px/METER, g->iS.pz/METER, r.GroundAt(g->iS.px,g->iS.pz)/METER);
 printf("terrace %d,%d m ground %d m, bfht %d m\n", GameBarrowOutX(0,0)/METER, GameBarrowOutZ(0,0)/METER, r.GroundAt(GameBarrowOutX(0,0),GameBarrowOutZ(0,0))/METER, g->iBfHt/METER);
 printf("arena over %d free %d KB\n", r.iArenaOver, (int)(r.iArenaEnd-r.iArena)/1024);
}
