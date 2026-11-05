#include "game.h"
#include <stdio.h>
#include <stdlib.h>
// (v0.4.74) a Nordic ruin's hinged doors: each blocks shut, opens when used, lets the player through open -> shots/fhdoors.ppm
static u16 out[640*200];
int GameKitWhy(const Game& g, int aCell, int aX, int aZ, int aR, int aFeetY, int aOff, int* aWall);
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY; g->iHideView = 0;
 FILE* f=fopen("shots/fhdoors.ppm","wb"); fprintf(f,"P6 640 400 255\n");
 int cells[3] = { CELL_FHSTRONG, CELL_FHCRYPT, CELL_FHREFECT }, tot = 0, okb = 0, oko = 0, shots = 0;
 for (int c = 0; c < 3; c++)
    {
    g->EnterCell(cells[c], 100*METER, 100*METER, 0);
    for (int t = 0; t < 4; t++) { g->iUi = UI_PLAY; g->Tick(50000, 0); }
    for (int i = 0; i < g->iS.nprop; i++)
        {
        Prop& p = g->iS.prop[i];
        if (p.cell != cells[c] || p.type != PT_DOOR || p.count <= 0) continue;
        tot++;
        // the middle of the leaf, and a point either side of it (1 m off)
        int cs = r3d_cos(p.yaw), sn = r3d_sin(p.yaw), half = p.count * METER / 200;
        int mx = p.x + ((sn * half) >> 14), mz = p.z + ((cs * half) >> 14);
        int ax = mx + ((cs * METER) >> 14), az = mz - ((sn * METER) >> 14);       // 1 m to one side (local +x)
        int feet = g->FloorAt(cells[c], mx, mz, p.y + METER / 2);
        int b0 = g->BlockedAt(cells[c], mx, mz, 100, feet + 7 * METER / 25, 7 * METER / 4 - 7 * METER / 25);
        // stand on one side, face the door, use it
        g->iS.px = ax; g->iS.pz = az; g->iS.py = g->FloorAt(cells[c], ax, az, p.y + METER / 2) + 410; g->iS.vy = 0;
        g->iS.yaw = r3d_atan2(mx - ax, mz - az); g->iS.pitch = 0;
        if (shots < 1) { g->Render(out, 640); fwrite(out, 0, 0, f); for (int k = 0; k < 640 * 200; k++) { u16 q = out[k]; unsigned char cc[3] = { (unsigned char)((q >> 8) & 0xF8), (unsigned char)((q >> 3) & 0xFC), (unsigned char)((q << 3) & 0xF8) }; fwrite(cc, 1, 3, f); } }
        g->UseProp(i);
        for (int t = 0; t < 12; t++) { g->iUi = UI_PLAY; g->Tick(66000, 0); }
        int b1 = g->BlockedAt(cells[c], mx, mz, 100, feet + 7 * METER / 25, 7 * METER / 4 - 7 * METER / 25);
        if (shots < 1) { g->Render(out, 640); for (int k = 0; k < 640 * 200; k++) { u16 q = out[k]; unsigned char cc[3] = { (unsigned char)((q >> 8) & 0xF8), (unsigned char)((q >> 3) & 0xFC), (unsigned char)((q << 3) & 0xF8) }; fwrite(cc, 1, 3, f); } shots++; }
        // walk through: 2 m on
        int sx = g->iS.px, sz = g->iS.pz;
        for (int t = 0; t < 12; t++) { g->iUi = UI_PLAY; g->Tick(66000, KEY_FWD); }
        int across = (((g->iS.px - mx) >> 4) * (cs >> 4) - ((g->iS.pz - mz) >> 4) * (sn >> 4)) < 0;       // on the other side?
        printf("  cell %d door %d (%d cm wide, anim %d): shut blocks %d, open blocks %d, walked through %d (%d cm)\n", cells[c], i, p.count, p.anim, b0, b1, across, (abs(g->iS.px - sx) + abs(g->iS.pz - sz)) * 100 / METER);
        okb += b0; oko += !b1 && across;
        if (!across)
            {
            // what stops the walk: each 25 cm along the way through
            int bx = g->iS.px, bz = g->iS.pz, bf = g->iS.py - 410;
            for (int st = 1; st <= 10; st++)
                {
                int qx = ax + (mx - ax) * st * 25 / 100, qz = az + (mz - az) * st * 25 / 100;
                int fl = g->FloorAt(cells[c], qx, qz, bf);
                int pb = g->PropBlocked(cells[c], qx, qz, 100, bf + 7 * METER / 25);
                int db = g->DunBlocked(cells[c], qx, qz, 100, bf + 7 * METER / 25);
                int wb[8]; int kb = GameKitWhy(*g, cells[c], qx, qz, 100, bf + 7 * METER / 25, 0, wb);
                printf("      %3d cm: floor %+d cm, prop %d, dungeon %d, kit %d", st * 25, (fl - bf) * 100 / METER, pb, db, kb);
                if (kb) printf(" [%d,%d-%d,%d h%d..%d feet %d]", wb[0], wb[1], wb[2], wb[3], wb[4], wb[5], wb[7]);
                printf("\n");
                }
            printf("      stopped at %d %d; door at %d %d yaw %d\n", bx, bz, p.x, p.z, p.yaw);
            }
        }
    }
 fclose(f);
 printf("%d doors: %d block when shut, %d open and passable; props %d of %d, containers %d of %d\n", tot, okb, oko, g->iS.nprop, MAXPROP, g->iS.ncont, MAXCONT);
 return 0; }
