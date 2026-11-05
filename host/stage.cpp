// (v0.4.67) people and creatures by place: the barrow's temple in, three killed (one looted), out, back - the dead
// where they fell, the looted still looted, the living back at their places; the table never past MAXE
#include <stdio.h>
#include <stdlib.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
static Game* g; static u8* m; static int fails = 0;
static void Check(int ok, const char* w) { printf("%s %s\n", ok ? "  ok  " : "  FAIL", w); if (!ok) fails++; }
static int Count(int cell, int alive) { int n = 0; for (int i = 0; i < g->iS.ne; i++) if (g->iS.e[i].cell == cell && g->iS.e[i].type != E_VILLAGER && g->iS.e[i].alive == alive) n++; return n; }
int main()
    {
    int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64); m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
    emu_init(m, 0); g = (Game*)m; emu_mode(m, 0); g->NewGame(); g->iUi = UI_PLAY;
    printf("start: %d in the table, groups %x\n", g->iS.ne, g->iS.grpIn);
    Check(Count(CELL_BFTEMPLE, 1) == 0, "the temple's people are not out while the player is elsewhere");
    const DoorLink& d0 = g->Door(7);
    g->EnterCell(d0.ocell, d0.ox, d0.oz, d0.oyaw);
    for (int t = 0; t < 20; t++) emu_tick(m, 50000, 0);
    printf("by the barrow's doors: %d in the table, groups %x\n", g->iS.ne, g->iS.grpIn);
    Check((g->iS.grpIn >> G_BFOUT) & 1, "the barrow's bandits out by its doors");
    g->UseDoor(7);
    for (int t = 0; t < 4; t++) emu_tick(m, 50000, 0);
    int alive0 = Count(CELL_BFTEMPLE, 1), dead0 = Count(CELL_BFTEMPLE, 0);
    printf("in the temple: %d alive, %d dead, %d in the table, groups %x\n", alive0, dead0, g->iS.ne, g->iS.grpIn);
    Check(alive0 > 10, "the temple's people out");
    Check(!((g->iS.grpIn >> G_WRFOLK) & 1), "Whiterun's people in");
    int killed = 0, kx[3], kz[3], lk = -1;
    for (int i = 0; i < g->iS.ne && killed < 3; i++)
        {
        Ent& e = g->iS.e[i];
        if (e.cell != CELL_BFTEMPLE || !e.alive || e.type == E_VILLAGER) continue;
        g->HurtEnt(e, 9999, 0, e.x, e.z);
        e.x += METER; kx[killed] = e.x; kz[killed] = e.z;          // (moved: it fell somewhere else)
        if (killed == 1) { e.looted |= 1; lk = e.sid; }
        killed++;
        }
    g->UseDoor(10);
    for (int t = 0; t < 4; t++) emu_tick(m, 50000, 0);
    printf("out: %d in the table, groups %x, roster %d\n", g->iS.ne, g->iS.grpIn, g->iS.nroster);
    Check(Count(CELL_BFTEMPLE, 1) + Count(CELL_BFTEMPLE, 0) == 0, "the temple's people in again");
    g->EnterCell(d0.ocell, d0.ox, d0.oz, d0.oyaw);
    g->UseDoor(7);
    for (int t = 0; t < 4; t++) emu_tick(m, 50000, 0);
    int alive1 = Count(CELL_BFTEMPLE, 1), dead1 = Count(CELL_BFTEMPLE, 0);
    printf("back: %d alive, %d dead\n", alive1, dead1);
    Check(alive1 == alive0 - 3 && dead1 == dead0 + 3, "three fewer living, three more dead");
    int at = 0, lt = 0;
    for (int i = 0; i < g->iS.ne; i++)
        {
        const Ent& e = g->iS.e[i];
        if (e.cell != CELL_BFTEMPLE || e.alive) continue;
        for (int k = 0; k < 3; k++) if (e.x == kx[k] && e.z == kz[k]) at++;
        if (e.sid == lk && e.grp == G_BFTEMPLE && (e.looted & 1)) lt = 1;
        }
    Check(at == 3, "the dead where they fell");
    Check(lt, "the looted one still looted");
    return fails != 0;
    }
