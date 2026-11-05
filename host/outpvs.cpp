// (v0.4.65) the outside's "what can be seen from where" checked: from points on Whiterun's streets, each way round,
// the picture with it and without it; prints the worst differences (pieces it hid that should show)
#include <stdio.h>
#include <stdlib.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
static u16 a[640 * 200], b[640 * 200];
int main(int argc, char** argv)
    {
    int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64);
    u8* m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
    emu_init(m, 0); Game* g = (Game*)m; emu_mode(m, 0);
    g->NewGame(); g->iUi = UI_PLAY;
    emu_view(m, argc > 1 ? atoi(argv[1]) : 64);
    printf("outside pvs: %p, %d regions, objs %d/%d\n", g->iOutPvs, g->iOutPvsRegN, g->iOutObjN, g->iOutPvsObjN);
    const u8* keep = g->iOutPvs;
    int tested = 0, worst = 0, bad = 0, hit = 0;
    srand(7);
    for (int k = 0; k < g->iKitCellN; k++)
        {
        const KitCell& c = g->iKitCells[k];
        if (c.cell != CELL_NONE || !c.nwall || c.wall[4] <= c.wall[5]) continue;
        for (int t = 0; t < 60; t++)
            {
            int f = rand() % c.nfloor;
            const short* v = c.floor + f * 9;
            int ux = c.ox + (v[0] + v[3] + v[6]) / 3, uy = c.oy + (v[1] + v[4] + v[7]) / 3, uz = (v[2] + v[5] + v[8]) / 3;
            g->iS.cell = 0;
            g->iS.px = (int)(((long long)ux + 233472) * 36576 / 10000); g->iS.pz = (int)(((long long)uy + 176128) * 36576 / 10000);
            int off = g->iR->iLandLoaded ? g->iR->iLandOffset : 0;
            g->iS.py = off + uz * 143 / 100 * 256 / 100 + 410; g->iS.vy = 0; g->iS.pitch = 0;
            for (int yaw = 0; yaw < 1024; yaw += 256)
                {
                g->iS.yaw = yaw;
                g->iOutPvs = keep; emu_render(m, a); hit += g->iR->iOutPvsBits != 0;
                g->iOutPvs = 0; emu_render(m, b);
                if (getenv("BASE")) { g->iOutPvs = 0; emu_render(m, a); }
                int d = 0;
                for (int i = 0; i < 640 * 200; i++) d += a[i] != b[i];
                tested++;
                if (d > worst && getenv("DUMP")) { FILE* q = fopen("/tmp/claude-0/oa.raw", "wb"); fwrite(a, 2, 640 * 200, q); fclose(q); q = fopen("/tmp/claude-0/ob.raw", "wb"); fwrite(b, 2, 640 * 200, q); fclose(q); }
                if (d > worst) { worst = d; printf("  %d px differ at %d %d %d yaw %d\n", d, ux, uy, uz, yaw); fflush(stdout); }
                if (d > 200) bad++;
                }
            }
        }
    printf("%d views, %d with the table, %d with over 200 px different, worst %d\n", tested, hit, bad, worst);
    return 0;
    }
