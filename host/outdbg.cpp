#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
void GameKitPvs(Game& g);
int main(int argc, char** argv)
    {
    int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64);
    u8* m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
    emu_init(m, 0); Game* g = (Game*)m; emu_mode(m, 0);
    g->NewGame(); g->iUi = UI_PLAY;
    int ux = atoi(argv[1]), uy = atoi(argv[2]), uz = atoi(argv[3]);
    g->iS.cell = 0;
    g->iS.px = (int)(((long long)ux + 233472) * 36576 / 10000); g->iS.pz = (int)(((long long)uy + 176128) * 36576 / 10000);
    int off = g->iR->iLandLoaded ? g->iR->iLandOffset : 0;
    g->iS.py = off + uz * 143 / 100 * 256 / 100 + 410;
    GameKitPvs(*g);
    R3D& r = *g->iR;
    printf("bits %p first %d n %d mark %d\n", r.iOutPvsBits, r.iOutPvsFirst, r.iOutPvsN, g->iOutObjMark);
    for (int k = 0; k < r.iOutPvsN; k++)
        {
        const Obj& o = r.iObj[g->iOutObjMark + k];
        int seen = (r.iOutPvsBits[k >> 3] >> (k & 7)) & 1;
        long long dx = o.x - g->iS.px, dz = o.z - g->iS.pz;
        int d = (int)(sqrt((double)(dx * dx + dz * dz)) / METER);
        if (!seen && d < 25) printf("hidden %d type %d at %d m (y %d vs %d)\n", k, o.type, d, o.y, g->iS.py);
        }
    return 0;
    }
