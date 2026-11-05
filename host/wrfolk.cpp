// (v0.4.65) Whiterun's people by the hour: where each one is at 6, 10, 14, 17 and 22 o'clock
#include <stdio.h>
#include <stdlib.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
const char* GameNamedName(int i);
int main()
    {
    int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64);
    u8* m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
    emu_init(m, 0); Game* g = (Game*)m; emu_mode(m, 0);
    g->NewGame(); g->iUi = UI_PLAY;
    printf("%d of %d people and creatures, groups out %x\n", g->iS.ne, MAXE, g->iS.grpIn);
    { int c[64] = {0}, cc[256] = {0}; for (int i = 0; i < g->iS.ne; i++) { c[g->iS.e[i].type & 63]++; cc[g->iS.e[i].cell]++; } for (int t = 0; t < 64; t++) if (c[t]) printf("type %d: %d  ", t, c[t]); printf("\n"); for (int t = 0; t < 256; t++) if (cc[t]) printf("cell %d: %d  ", t, cc[t]); printf("\n"); }
    static const int H[5] = { 6, 10, 14, 17, 22 };
    for (int h = 0; h < 5; h++)
        {
        g->iS.timeOfDay = H[h] * 60000; g->iS.cell = 5; g->WhiterunSchedule(1);
        int out = 0;
        printf("%2d h:", H[h]);
        for (int i = 0; i < g->iS.ne; i++)
            {
            const Ent& e = g->iS.e[i];
            if (e.type != E_VILLAGER || e.role != ROLE_NAMED || e.pad1 < 29) continue;
            if (e.cell == 0) { out++; printf(" %s(%d)", GameNamedName(e.pad1), (e.y - g->iR->GroundAt(e.x, e.z)) * 100 / METER); }
            }
        printf("  = %d out\n", out);
        }
    for (int i = 0; i < g->iS.ne; i++) { const Ent& e = g->iS.e[i]; if (e.type == E_SMITH || e.type == E_ELDER) printf("%s cell %d at %d %d y %d ground %d\n", e.type == E_SMITH ? "smith" : "steward", e.cell, e.x, e.z, e.y, g->iR->GroundAt(e.x, e.z)); }
    return 0;
    }
