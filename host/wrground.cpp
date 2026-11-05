#include "game.h"
#include <stdio.h>
#include <stdlib.h>
static int WX(int a){return (int)(((long long)a+233472)*36576/10000);} static int WZ(int a){return (int)(((long long)a+176128)*36576/10000);}
int main(int c,char**v){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game*g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame();
 for(int i=1;i+1<c;i+=2){int X=atoi(v[i]),Y=atoi(v[i+1]); int x=WX(X),z=WZ(Y); g->iR->FocusLand(x,z); int gr=g->iR->GroundAt(x,z); int fl=g->FloorAt(0,x,z,gr+3*METER/2); printf("%d %d ground %d floor %d\n",X,Y,gr,fl);} }
