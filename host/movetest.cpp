#include "game.h"
#include <stdio.h>
#include <stdlib.h>
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem);
 g->iUi = UI_PLAY;
 int x=(int)(3316.1*256), z=(int)(1900.1*256);
 for (int yaw=0; yaw<1024; yaw+=128) {
   g->EnterCell(0, x, z, yaw); g->iFadeUs=0; g->iUi=UI_PLAY; g->iS.yaw=yaw;
   for (int t=0;t<30;t++){ g->iS.yaw=yaw; g->iUi=UI_PLAY; g->Tick(16000, KEY_FWD);} 
   printf("yaw %d moved %.2f %.2f\n", yaw, (g->iS.px-x)/256.0, (g->iS.pz-z)/256.0);
 }
}
