#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 int cx = CITY_CX * CELL + CELL / 2, cz = CITY_CZ * CELL + CELL / 2;
 for (int z=-26; z<=-10; z++) printf("z %d ground %d floor %d  side(x=3) %d\n", z, g->iR->GroundAt(cx, cz+z*METER)*100/METER, g->FloorAt(0,cx,cz+z*METER,0)*100/METER, g->iR->GroundAt(cx+3*METER, cz+z*METER)*100/METER);
 for (int z=6; z<=20; z++) printf("z %d ground %d floor %d side %d\n", z, g->iR->GroundAt(cx, cz+z*METER)*100/METER, g->FloorAt(0,cx,cz+z*METER,0)*100/METER, g->iR->GroundAt(cx+3*METER, cz+z*METER)*100/METER);
}
