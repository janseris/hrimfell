// Skyrim's own Bleak Falls Barrow (v0.4.55), its quest through: in by the main door, the bandits' talk, the man at
// the lever, the pillars and the lever (the gate), the spider and Arvel (his run, the claw), the pull chains, the
// claw door, the word wall, the Overlord, out by the way out. Prints each step; FAIL lines for what did not happen.
#include <stdio.h>
#include <stdlib.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
struct DunProp { int type, x, y, z, yaw, item; };
#include "bfb_kit.inc"
enum { DD_ARVEL_FREE = 5 };
int GameDunLoot(Game& g, Ent& e);
static Game* g; static u8* m;
static int fails = 0;
static void Check(int ok, const char* what) { printf("%s %s\n", ok ? "  ok  " : "  FAIL", what); if (!ok) fails++; }
static int CmX(int cm) { return 100 * METER + cm * METER / 100; }
static u16 scr[640 * 200];
static void Run(int ms) { for (int t = 0; t < ms; t += 50) { emu_tick(m, 50000, 0); if (t % 1000 == 0) emu_render(m, scr); if (g->iUi == UI_DIALOG) g->iUi = UI_PLAY; } }
// the player at (x, z) cm, feet on the floor near y cm
static void At(int x, int y, int z)
    {
    g->iS.px = CmX(x); g->iS.pz = CmX(z);
    g->iS.py = g->FloorAt(g->iS.cell, g->iS.px, g->iS.pz, CmX(y) + METER / 2) + 410; g->iS.vy = 0;
    }
static int Ent1(int type, int pad1, int mask)
    {
    for (int i = 0; i < g->iS.ne; i++) { const Ent& e = g->iS.e[i]; if (e.type == type && e.pad1 == pad1 && (e.pad2 & mask) == mask && e.cell == g->iS.cell) return i; }
    return -1;
    }
static int Prop1(int type, int item)
    {
    for (int i = 0; i < g->iS.nprop; i++) { const Prop& p = g->iS.prop[i]; if (p.type == type && p.cell == g->iS.cell && (item < 0 || p.item == item)) return i; }
    return -1;
    }
static int Near(int i, int ref, int cmR) { const Ent& e = g->iS.e[i]; return abs(e.x - CmX(ref)) < cmR * METER / 100; }

int main()
    {
    int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64);
    m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
    emu_init(m, 0); g = (Game*)m; emu_mode(m, 0);
    g->NewGame(); g->iUi = UI_PLAY;
    Check(g->iBfbKit, "Skyrim's barrow file is there");
    // in by the main door (from the terrace)
    const DoorLink& d0 = g->Door(7);
    g->EnterCell(d0.ocell, d0.ox, d0.oz, d0.oyaw);
    g->UseDoor(7);
    Run(500);
    Check(g->iS.cell == CELL_BFTEMPLE, "the main door leads into the temple");
    Check(g->iAreaCur == 1, "the barrow's own area is in");
    Check(g->iR->iPvsBits != 0, "what can be seen from the door is known");
    printf("    at %d %d feet %d\n", (g->iS.px - CmX(0)) * 100 / METER, (g->iS.pz - CmX(0)) * 100 / METER, (g->iS.py - 410 - CmX(0)) * 100 / METER);
    // the bandits by the fire talk of Arvel (the quest starts)
    At(-7000, 833, 1300);
    Run(17000);
    Check(g->iS.bfb[BF_QCLAW] == 12, "listening to the bandits starts The Golden Claw");
    // the bandits there killed
    for (int i = 0; i < g->iS.ne; i++) { Ent& e = g->iS.e[i]; if (e.cell == CELL_BFTEMPLE && e.type == E_BANDIT && e.pad1 == 0 && e.alive && (e.pad2 & 1)) g->HurtEnt(e, 999, 0, e.x, e.z); }
    // the man at the lever pulls it: the darts
    int lm = Ent1(E_BANDIT, 0, 4);
    Check(lm >= 0, "the bandit at the lever is there");
    if (lm >= 0)
        {
        const Ent& e = g->iS.e[lm];
        At((e.x - CmX(0)) * 100 / METER - 500, (e.y - CmX(0)) * 100 / METER, (e.z - CmX(0)) * 100 / METER);
        Run(2500);
        Check(g->iS.bfb[BF_LEVERMAN] == 1, "he pulls the lever (the darts)");
        g->HurtEnt(g->iS.e[lm], 999, 0, e.x, e.z);
        }
    // the gate: shut, then the pillars to snake, snake, whale and the lever
    const int* gf = KBFKTEMPLEFix;
    int gx = CmX(gf[1]), gz = CmX(gf[3]), gy = CmX(gf[2]);
    Check(g->DunBlocked(CELL_BFTEMPLE, gx, gz, 40, gy), "the gate is shut");
    int lever = Prop1(PT_LEVER, -1);
    Check(lever >= 0, "the lever is there");
    for (int k = 0; k < 3; k++)
        {
        int p = Prop1(PT_PILLAR, k);
        int want = k == 0 ? 2 : 0;
        for (int t = 0; t < 3 && g->iS.bfb[BF_PILLAR0 + k] != want; t++) { g->UseDunProp(p); Run(800); }
        }
    g->UseDunProp(lever);
    Check(g->iS.bfb[BF_GATE0] == 1, "the pillars right, the lever raises the gate");
    Run(3000);
    Check(!g->DunBlocked(CELL_BFTEMPLE, gx, gz, 40, gy), "the gate is open to walk through");
    // the spider drops as you come in; Arvel cries out
    int sp = Ent1(E_SPIDER, 0, 8);
    Check(sp >= 0, "the spider waits in the dark above");
    At(4642 + 300, -2568, -4799);
    Run(400);
    Check(g->iS.bfb[BF_SPIDER] == 1, "it drops");
    Run(1500);
    if (sp >= 0) { Ent& e = g->iS.e[sp]; g->HurtEnt(e, 9999, 0, e.x, e.z); }
    Check(g->iS.bfb[BF_SPIDER] == 2, "killed");
    int ar = Ent1(E_BANDIT, 1, 16);
    Check(ar >= 0, "Arvel is in the web");
    if (ar >= 0)
        {
        g->DunDialogDone(DD_ARVEL_FREE);
        Check(g->iS.bfb[BF_ARVEL] == 1, "cut free, he runs");
        Ent& e = g->iS.e[ar];
        int x0 = e.x, z0 = e.z;
        Run(25000);
        printf("    Arvel ran from %d %d to %d %d (cm), step %d of %d\n", (x0 - CmX(0)) * 100 / METER, (z0 - CmX(0)) * 100 / METER, (e.x - CmX(0)) * 100 / METER, (e.z - CmX(0)) * 100 / METER, e.cool, KBFKArvelPath_N);
        Check(abs(e.x - CmX(KBFKArvelPath[KBFKArvelPath_N * 2 - 2])) < 2 * METER && abs(e.z - CmX(KBFKArvelPath[KBFKArvelPath_N * 2 - 1])) < 2 * METER, "he reaches the end of his run");
        int had = g->CountItem(IT_GOLDEN_CLAW);
        g->HurtEnt(e, 9999, 0, e.x, e.z);
        Check(g->iS.bfb[BF_ARVEL] == 2, "killed");
        // the claw from his body
        int got = 0;
        for (int i = 0; i < g->iS.ne && !got; i++) if (&g->iS.e[i] == &e) { if (GameDunLoot(*g, e)) got = 1; }
        Check(g->CountItem(IT_GOLDEN_CLAW) > had, "the Golden Claw is on him");
        }
    // the pull chains: the blades, the small gate
    int ch0 = Prop1(PT_PULLCHAIN, 0), ch1 = Prop1(PT_PULLCHAIN, 1);
    Check(ch0 >= 0 && ch1 >= 0, "the pull chains are there");
    if (ch0 >= 0) g->UseDunProp(ch0);
    if (ch1 >= 0) g->UseDunProp(ch1);
    Check(g->iS.bfb[BF_BLADES0] == 1 && g->iS.bfb[BF_GATE1] == 1, "they stop the blades and raise the small gate");
    // through to the sanctum
    g->UseDoor(9);
    Run(500);
    Check(g->iS.cell == CELL_BFSANCTUM, "the door down leads into the sanctum");
    Check(g->iR->iPvsBits != 0, "what can be seen there is known");
    // the claw door
    const DunProp* cd = KBFKSANCTUMProp;
    int cx = CmX(cd[0].x), cz = CmX(cd[0].z), cy = CmX(cd[0].y);
    int claw = Prop1(PT_CLAWDOOR, -1);
    Check(claw >= 0, "the claw door is there");
    Check(g->DunBlocked(CELL_BFSANCTUM, cx, cz, 40, cy), "it is shut");
    if (claw >= 0) { g->UseDunProp(claw); Check(g->iUi == UI_PUZZLE, "with the claw: its rings to turn"); }
    g->iS.bfb[BF_CLAWDOOR] = 1; g->iUi = UI_PLAY;
    Run(4000);
    Check(!g->DunBlocked(CELL_BFSANCTUM, cx, cz, 40, cy), "the door sinks: the way is open");
    // the word wall; the Overlord wakes
    const DunProp& ww = KBFKSANCTUMProp[1];
    At(ww.x + 250, ww.y, ww.z);
    Run(500);
    { int wp = Prop1(PT_WORDWALL, -1); if (wp >= 0) printf("    word wall at %d %d %d, the player %d %d feet %d\n", (g->iS.prop[wp].x - CmX(0)) * 100 / METER, (g->iS.prop[wp].z - CmX(0)) * 100 / METER, (g->iS.prop[wp].y - CmX(0)) * 100 / METER, (g->iS.px - CmX(0)) * 100 / METER, (g->iS.pz - CmX(0)) * 100 / METER, (g->iS.py - 410 - CmX(0)) * 100 / METER); else printf("    no word wall prop\n"); }
    Check(g->iS.bfb[BF_WORD] == 1, "the word wall teaches FUS");
    Run(2500);
    Check(g->iS.bfb[BF_LORD] == 1, "the Overlord rises");
    int lord = Ent1(E_WIGHT, 2, 0);
    if (lord >= 0) { Ent& e = g->iS.e[lord]; Check(!(e.pad2 & 64) && e.aggro, "out of his sarcophagus, fighting"); g->HurtEnt(e, 9999, 0, e.x, e.z); GameDunLoot(*g, e); }
    Check(g->CountItem(IT_DRAGONSTONE) > 0, "the Dragonstone from his chest or body");
    // out by the way out
    g->UseDoor(10);
    Run(500);
    Check(g->iS.cell == 0, "the way out leads onto the mountain");
    Check(g->iAreaCur == 0, "the main area is back");
    printf("%s (%d failed)\n", fails ? "FAILED" : "all passed", fails);
    return fails != 0;
    }
