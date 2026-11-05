// (v0.4.68) Forelhost through: in by its door on the mountain, the Stronghold (its lever, its portcullis), down to the
// Crypt (its draugr asleep, its boss), up to the Refectory (the Glass Claw, the claw door's rings, Rahgot and his
// mask), out by the great door onto the top, the word wall (STRUN), and back to the main area.
#include <stdio.h>
#include <stdlib.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
int GameDunLoot(Game& g, Ent& e);
static Game* g; static u8* m; static int fails = 0;
static u16 scr[640 * 200];
static void Check(int ok, const char* w) { printf("%s %s\n", ok ? "  ok  " : "  FAIL", w); if (!ok) fails++; }
static void Run(int ms) { for (int t = 0; t < ms; t += 50) { emu_tick(m, 50000, 0); if (t % 1000 == 0) emu_render(m, scr); if (g->iUi == UI_DIALOG) g->iUi = UI_PLAY; } }
static int Count(int cell, int type, int alive) { int n = 0; for (int i = 0; i < g->iS.ne; i++) { const Ent& e = g->iS.e[i]; if (e.cell == cell && (type < 0 || e.type == type) && e.alive == alive) n++; } return n; }
static int Prop1(int type, int cell) { for (int i = 0; i < g->iS.nprop; i++) if (g->iS.prop[i].type == type && g->iS.prop[i].cell == cell) return i; return -1; }
int main()
    {
    int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64); m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
    emu_init(m, 0); g = (Game*)m; emu_mode(m, 0); g->NewGame(); g->iUi = UI_PLAY;
    Check(g->iFhKit, "Forelhost's file is there");
    const DoorLink& d0 = g->Door(26);
    g->EnterCell(0, d0.ox, d0.oz, d0.oyaw);
    Run(300);
    int near = 0;
    for (int i = 0; i < g->iR->iObjCount; i++) { const Obj& o = g->iR->iObj[i]; if (abs(o.x - d0.ox) < 60 * METER && abs(o.z - d0.oz) < 60 * METER && o.type >= ST_CUSTOM) near++; }
    printf("    by the door: area %d, %d pieces within 60 m, feet %d, door sill %d\n", g->iAreaCur, near, g->iS.py - 410, d0.oy + g->iR->iLandOffset);
    Check(near > 40, "the ruin stands round its door");
    g->UseDoor(26); Run(500);
    Check(g->iS.cell == CELL_FHSTRONG && g->iAreaCur == 3, "the door leads into the Stronghold (Forelhost's file in)");
    printf("    in: %d alive, %d dead, table %d, pvs %p\n", Count(CELL_FHSTRONG, -1, 1), Count(CELL_FHSTRONG, -1, 0), g->iS.ne, g->iR->iPvsBits);
    Check(Count(CELL_FHSTRONG, E_WIGHT, 1) >= 8, "its ghosts and draugr are out");
    // the lever and the portcullis it raises
    int lv = Prop1(PT_LEVER, CELL_FHSTRONG);
    Check(lv >= 0, "a lever");
    if (lv >= 0) { int gi = g->iS.prop[lv].item - 100; g->UseDunProp(lv); Run(3000); Check(g->iS.dun[FH_GATE0 + gi] == 1 && g->iFhGate[gi] >= 200, "it raises its portcullis"); }
    // down to the Crypt
    g->UseDoor(27); Run(500);
    Check(g->iS.cell == CELL_FHCRYPT, "the door down leads into the Crypt");
    printf("    crypt: %d alive, %d dead\n", Count(CELL_FHCRYPT, -1, 1), Count(CELL_FHCRYPT, -1, 0));
    Check(Count(CELL_FHSTRONG, -1, 1) == 0, "the Stronghold's in again");
    int asleep = 0, boss = -1;
    for (int i = 0; i < g->iS.ne; i++) { const Ent& e = g->iS.e[i]; if (e.cell == CELL_FHCRYPT && e.type == E_WIGHT && (e.pad2 & 32)) asleep++; if (e.cell == CELL_FHCRYPT && e.pad1 == 5) boss = i; }
    Check(asleep >= 10, "draugr asleep in their alcoves and sarcophagi");
    Check(boss >= 0, "the Crypt's boss");
    // up to the Refectory
    g->UseDoor(28); Run(500);
    Check(g->iS.cell == CELL_FHREFECT, "the door up leads into the Refectory");
    int cl = -1; for (int i = 0; i < g->iS.nprop; i++) if (g->iS.prop[i].type == PT_ITEM && g->iS.prop[i].item == IT_GLASS_CLAW) cl = i;
    Check(cl >= 0 && g->iS.prop[cl].cell == CELL_FHREFECT, "the Glass Claw on its pedestal");
    int cd = Prop1(PT_CLAWDOOR, CELL_FHREFECT);
    Check(cd >= 0, "the claw door");
    if (cd >= 0)
        {
        const Prop& p = g->iS.prop[cd];
        Check(g->DunBlocked(CELL_FHREFECT, p.x, p.z, 40, p.y), "it is shut");
        g->UseDunProp(cd); Check(g->iUi != UI_PUZZLE, "without the claw it does not turn");
        g->AddItem(IT_GLASS_CLAW, 1);
        g->UseDunProp(cd); Check(g->iUi == UI_PUZZLE && g->iPuzDoor == 1, "with the claw: its rings");
        g->iS.dun[FH_RING0] = 0; g->iS.dun[FH_RING1] = 1; g->iS.dun[FH_RING2] = 2;     // Fox, Owl, Snake
        g->PuzzleInput(); g->iUi = UI_PLAY; g->iS.dun[FH_CLAWDOOR] = 1;
        Run(4000);
        Check(!g->DunBlocked(CELL_FHREFECT, p.x, p.z, 40, p.y), "Fox, Owl, Snake: the door sinks");
        }
    int ra = -1; for (int i = 0; i < g->iS.ne; i++) if (g->iS.e[i].cell == CELL_FHREFECT && g->iS.e[i].type == E_WIGHT && g->iS.e[i].pad1 == 4) ra = i;
    Check(ra >= 0, "Rahgot is there");
    if (ra >= 0) { Ent& e = g->iS.e[ra]; g->HurtEnt(e, 9999, 0, e.x, e.z); GameDunLoot(*g, e); }
    Check(g->CountItem(IT_RAHGOT) > 0, "his mask from his body");
    // out onto the top, the word wall
    g->UseDoor(29); Run(500);
    Check(g->iS.cell == 0 && g->iAreaCur == 0, "the great door leads out onto the ruin's top (the main area)");
    int ww = -1; for (int i = 0; i < g->iS.nprop; i++) if (g->iS.prop[i].type == PT_WORDWALL && g->iS.prop[i].item == 1) ww = i;
    Check(ww >= 0, "the word wall");
    if (ww >= 0)
        {
        const Prop& p = g->iS.prop[ww];
        g->iS.px = p.x + 2 * METER; g->iS.pz = p.z; g->iS.py = g->FloorAt(0, g->iS.px, g->iS.pz, p.y + 2 * METER) + 410;
        printf("    word wall at feet %d, player feet %d\n", p.y, g->iS.py - 410);
        Run(500);
        Check(g->iS.dun[FH_WORD] == 1, "walking up to it: STRUN learned");
        }
    printf("%s (%d failed)\n", fails ? "FAILED" : "all passed", fails);
    return fails != 0;
    }
