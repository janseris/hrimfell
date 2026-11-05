// (v0.4.69) Siege on the Dragon Cult through: Valmir calls out by Forelhost's door, the journal, shown to him, the
// well's key in the crypt's chest, down the well to the Refectory, Rahgot's mask, Valmir found out, killed.
#include <stdio.h>
#include <stdlib.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
int GameDunLoot(Game& g, Ent& e);
void GameStaffCast(Game& g);
static Game* g; static u8* m; static int fails = 0;
static void Check(int ok, const char* w) { printf("%s %s\n", ok ? "  ok  " : "  FAIL", w); if (!ok) fails++; }
static int pages = 0;
static void Talkthrough() { while (g->iUi == UI_DIALOG) { g->DialogNext(); pages++; } }
static void Run(int ms) { for (int t = 0; t < ms; t += 50) { emu_tick(m, 50000, 0); if (g->iUi == UI_DIALOG) Talkthrough(); } }
static int Valmir(int type) { for (int i = 0; i < g->iS.ne; i++) { const Ent& e = g->iS.e[i]; if (e.grp == G_FHOUT && e.type == type && (type == E_BANDIT || e.pad1 == 53)) return i; } return -1; }
int main()
    {
    int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64); m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
    emu_init(m, 0); g = (Game*)m; emu_mode(m, 0); g->NewGame(); g->iUi = UI_PLAY;
    printf("    items %d\n", (int)IT_COUNT);
    const DoorLink& d0 = g->Door(26);
    g->EnterCell(0, d0.ox, d0.oz, d0.oyaw); Run(300);
    int v = Valmir(E_VILLAGER);
    Check(v >= 0, "Valmir in Forelhost's courtyard");
    if (v < 0) return 1;
    printf("    Valmir %d m from the door\n", (int)((abs(g->iS.e[v].x - d0.ox) + abs(g->iS.e[v].z - d0.oz)) / METER));
    g->iS.px = g->iS.e[v].x + 3 * METER; g->iS.pz = g->iS.e[v].z; pages = 0;
    Run(300);
    Check(pages >= 16 && g->iS.dun[FH_QUEST] == 10, "he calls out; his mission told: the quest at stage 10");
    printf("    quest line: %s\n", g->QuestText());
    // the journal in the Stronghold
    g->UseDoor(26); Run(300);
    int jp = -1; for (int i = 0; i < g->iS.nprop; i++) if (g->iS.prop[i].type == PT_ITEM && g->iS.prop[i].item == IT_FH_JOURNAL) jp = i;
    Check(jp >= 0 && g->iS.prop[jp].cell == CELL_FHSTRONG, "Skorm Snow-Strider's journal in the Stronghold");
    g->AddItem(IT_FH_JOURNAL, 1); Run(200);
    Check(g->iS.dun[FH_QUEST] == 20, "picked up: stage 20");
    g->UseDoor(26); Run(300);
    v = Valmir(E_VILLAGER);
    g->iS.px = g->iS.e[v].x + 2 * METER; g->iS.pz = g->iS.e[v].z;
    g->FhValmirTalk(); Talkthrough();
    Check(g->iS.dun[FH_QUEST] == 26 && g->CountItem(IT_FH_JOURNAL) == 0, "shown to Valmir: the well's key is in the crypt");
    // the crypt's chest, the well
    g->UseDoor(26); Run(200); g->UseDoor(27); Run(300);
    int key = 0;
    for (int i = 0; i < g->iS.nprop; i++) { const Prop& p = g->iS.prop[i]; if (p.type == PT_CHEST && p.cell == CELL_FHCRYPT && [&]{ for (int k = 0; k < CONTSLOTS; k++) if (g->iS.cont[p.item].id[k] == IT_FH_WELLKEY && g->iS.cont[p.item].n[k]) return 1; return 0; }()) key = 1; }
    Check(key, "the well's key in the crypt's chest");
    int wl = -1; for (int i = 0; i < g->iS.nprop; i++) if (g->iS.prop[i].type == PT_WELL) wl = i;
    Check(wl >= 0, "the well");
    g->UseDunProp(wl); Check(g->iS.cell == CELL_FHCRYPT, "locked without its key");
    g->AddItem(IT_FH_WELLKEY, 1); g->UseDunProp(wl); Run(300);
    Check(g->iS.cell == CELL_FHREFECT && g->iS.dun[FH_QUEST] == 40, "down the well into the Refectory");
    // Rahgot
    int ra = -1; for (int i = 0; i < g->iS.ne; i++) if (g->iS.e[i].cell == CELL_FHREFECT && g->iS.e[i].type == E_WIGHT && g->iS.e[i].pad1 == 4) ra = i;
    if (ra >= 0) { Ent& e = g->iS.e[ra]; g->HurtEnt(e, 9999, 0, e.x, e.z); }
    Run(200);
    Check(g->iS.dun[FH_QUEST] == 50, "Rahgot dead: stage 50");
    if (ra >= 0) GameDunLoot(*g, g->iS.e[ra]);
    Check(g->CountItem(IT_RAHGOT_STAFF) == 1, "his staff taken");
    Run(200);
    Check(g->iS.dun[FH_QUEST] == 60, "his mask taken: back to Valmir");
    g->UseDoor(29); Run(300);
    v = Valmir(E_VILLAGER);
    Check(v >= 0 && g->iS.e[v].var == 1, "Valmir on the battlements, in Stormcloak armour, with his soldier");
    if (v >= 0) { g->iS.px = g->iS.e[v].x + 5 * METER; g->iS.pz = g->iS.e[v].z; pages = 0; Run(300); }
    Check(g->iS.dun[FH_QUEST] == 70, "found out: kill the impostor");
    int nb = 0;
    for (int i = 0; i < g->iS.ne; i++) { const Ent& e = g->iS.e[i]; if (e.grp == G_FHOUT && e.type == E_BANDIT && e.alive) { nb++; printf("    %s pad1 %d aggro %d\n", e.pad1 == 53 ? "Valmir" : "soldier", e.pad1, e.aggro); } }
    Check(nb == 2, "Valmir and his soldier both hostile");
    for (int i = 0; i < g->iS.ne; i++) { Ent& e = g->iS.e[i]; if (e.grp == G_FHOUT && e.type == E_BANDIT && e.alive && e.pad1 == 53) g->HurtEnt(e, 9999, 0, e.x, e.z); }
    Run(200);
    Check(g->iS.dun[FH_QUEST] == 70, "the soldier still up: not done yet");
    // the staff: a wall of flames on the soldier
    int sb = -1; for (int i = 0; i < g->iS.ne; i++) { const Ent& e = g->iS.e[i]; if (e.grp == G_FHOUT && e.type == E_BANDIT && e.alive) sb = i; }
    if (sb >= 0)
        {
        Ent& e = g->iS.e[sb]; int hp0 = e.hp;
        g->iS.px = e.x; g->iS.pz = e.z - 4 * METER; g->iS.yaw = 0; e.x = g->iS.px; e.z = g->iS.pz + 4 * METER;
        GameStaffCast(*g); Run(1000);
        printf("    soldier hp %d -> %d\n", hp0, sb < g->iS.ne ? g->iS.e[sb].hp : -1);
        }
    for (int i = 0; i < g->iS.ne; i++) { Ent& e = g->iS.e[i]; if (e.grp == G_FHOUT && e.type == E_BANDIT && e.alive) g->HurtEnt(e, 9999, 0, e.x, e.z); }
    Run(200);
    Check(g->iS.dun[FH_QUEST] == 200, "Siege on the Dragon Cult completed");
    printf("%s (%d failed)\n", fails ? "FAILED" : "all passed", fails);
    return fails != 0;
    }
