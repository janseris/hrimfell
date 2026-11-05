#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// views of Bleak Falls Barrow -> shots/bfb.ppm (map squares as on the maps; see docs/maps/gen_bfb.py)
static u16 out[640*200];
static Game* g; static FILE* f;
static int CI(int c) { return c == CELL_BFTEMPLE ? 62 : (c == CELL_BFCRYPT ? 60 : 106); }
static int CJ(int c) { return c == CELL_BFTEMPLE ? 25 : (c == CELL_BFCRYPT ? 50 : 30); }
static int X(int cell, double c) { return (int)((c * 2 - CI(cell)) * 75); }
static int Z(int cell, double r) { return (int)((CJ(cell) - r * 2) * 75); }
static void emit()
    {
    g->iFadeUs = 0;
    g->Render(out, 640);
    for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
    }
static void shot(int cell, double c, double r, int y, double tc, double tr, int pitch)
    {
    int x = X(cell, c), z = Z(cell, r), tx = X(cell, tc), tz = Z(cell, tr);
    g->EnterCell(cell, 100 * METER + x * METER / 100, 100 * METER + z * METER / 100, r3d_atan2(tx - x, tz - z));
    g->iS.py = 100 * METER + y * METER / 100 + 410;
    g->iS.pitch = pitch; g->iS.third = 0;
    for (int k = 0; k < 10; k++) g->Tick(16000, 0);
    g->iS.py = 100 * METER + y * METER / 100 + 410;
    emit();
    }
static void outside(int xm, int zm, int txm, int tzm, int up, int pitch)
    {
    g->EnterCell(0, xm * METER, zm * METER, r3d_atan2(txm - xm, tzm - zm));
    for (int k = 0; k < 10; k++) g->Tick(16000, 0);
    g->iS.py = g->iR->GroundAt(xm * METER, zm * METER) + 410 + up * METER;
    g->iS.pitch = pitch;
    emit();
    }
int main(int argc,char**argv){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY; g->iHideView = 1;
 g->iR->SetOutputMode(2);
 int which = argc > 1 ? atoi(argv[1]) : 0;
 for (int i = 0; i < g->iS.ne; i++) if (g->iS.e[i].type >= E_WOLF) g->iS.e[i].aggro = 0;
 f=fopen("shots/bfb.ppm","wb"); fprintf(f,"P6 640 %d 255\n", 200 * 6);
 if (which == 0)
     {
     shot(CELL_BFTEMPLE, 58, 17.1, 0, 46, 17, 5);          // the entry hall from the doors
     shot(CELL_BFTEMPLE, 41, 17.05, -150, 30, 17.05, 0);   // the passage under its arches
     shot(CELL_BFTEMPLE, 9, 13, -100, 7, 7, 20);           // the pillar room: the gate, the heads
     shot(CELL_BFTEMPLE, 6.3, 10.5, -100, 3.8, 10.5, -10); // the pillars in their niches
     shot(CELL_BFTEMPLE, 24, 17.05, -300, 9, 17.05, 0);    // the lower passage
     shot(CELL_BFTEMPLE, 6, 3, -100, 9, 6, 10);            // the spiral room
     }
 else if (which == 1)
     {
     shot(CELL_BFCRYPT, 43, 41, 0, 33, 37.5, 0);           // the spider's lair from its door
     shot(CELL_BFCRYPT, 7, 34, 0, 7, 21, 0);               // the south burial passage
     shot(CELL_BFCRYPT, 13, 34, 0, 9, 33.8, 0);            // the cave with the plate
     shot(CELL_BFCRYPT, 20.3, 19, 400, 9, 19, 0);          // the blades' gallery
     shot(CELL_BFCRYPT, 7, 21, 0, 16, 21, 5);              // up the north passage
     shot(CELL_BFCRYPT, 42, 25, 100, 54, 30, 0);           // the sunlit cavern
     }
 else if (which == 2)
     {
     shot(CELL_BFSANCTUM, 101, 6.6, 0, 90, 4, 0);         // in from the temple
     shot(CELL_BFSANCTUM, 85, 13.5, 0, 94, 19, -15);        // the first chamber from the walkway
     shot(CELL_BFSANCTUM, 70, 17.65, 0, 66, 17.65, 0);     // the claw door
     shot(CELL_BFSANCTUM, 65.5, 17.65, 0, 52, 17.65, 0);   // the Hall of Stories
     shot(CELL_BFSANCTUM, 47, 18, 0, 30, 18.5, 5);          // into the main chamber
     shot(CELL_BFSANCTUM, 36.5, 18.6, -60, 29, 18.6, 5);    // the word wall
     }
 else
     {
     outside(84, 72, 84, 36, 0, 25);                        // up the slope to the terrace
     outside(84, 61, 84, 34, 0, 25);                        // from the landing
     outside(84, 50, 84, 30, 0, 20);                        // on the terrace before the doors
     outside(125, 100, 90, 43, 0, 15);                      // from the road below the tower
     outside(84, 36, 84, 78, 0, -15);                       // from the doors down over the stairs
     outside(160, 120, 84, 38, 0, 10);                      // from afar
     }
 fclose(f);}
