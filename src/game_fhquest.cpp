// game_fhquest.cpp - (v0.4.69) Siege on the Dragon Cult (Skyrim's dunForelhostQST): Captain Valmir in Forelhost's
// courtyard wants the dragon priest's mask "for the war effort"; Skorm Snow-Strider's journal, the well and its key;
// Rahgot; Valmir found out on the battlements, recruiting a soldier in the other side's armour. His words and the
// journal's entries are Skyrim's own (docs/maps/gen_fh_quest.py -> fh_quest.inc), the places where Skyrim has them
// (the forelhost export's refs: Valmir 0007E5B7, the soldier 00062244, the journal 0007E5B9, the crypt's chest
// 0003A497, the well 00082E1C, the Refectory's marker 00082E1A).
#include "game.h"
#include "fh_quest.inc"
#include "fh_books.inc"            // (v0.4.72) Forelhost's books and notes

static const int M = METER;
static inline int Abs(int v) { return v < 0 ? -v : v; }
static int Cm(int cm) { return cm * M / 100; }
static int U(int aUnits) { return aUnits * 143 / 100; }
enum { IN_OX = 100 * METER, IN_OZ = 100 * METER, IN_FLOOR = 100 * METER };
static int Ix(int u) { return IN_OX + Cm(U(u)); }          // (Skyrim units in a hall's frame -> engine)
static int Iz(int u) { return IN_OZ + Cm(U(u)); }
static int Iy(int u) { return IN_FLOOR + Cm(U(u)); }
static int WX(int aX) { return (int)(((long long)aX + 233472) * 36576 / 10000); }
static int WZ(int aY) { return (int)(((long long)aY + 176128) * 36576 / 10000); }
static int SY(int aDeg) { return (aDeg * 1024 / 360) & 1023; }
enum { NM_VALMIR = 53, NM_SOLDIER = 54 };          // (the named people's numbers: game_ui.cpp's looks, game.cpp's names)
enum { DDF0 = 100 + 14 };                          // (dialogue ids: after the barrow's DD_COUNT - game_dungeon.cpp)
const ItemDef& GameItem(int aId);

// (v0.4.71) Skyrim's own stage numbers (UESP's table of dunForelhostQST): 10 started (objectives 10 the journal, 50 the
// mask), 20 the journal found (objective 20: show it to Valmir), 26 shown (objective 30: the key to the well), 40 through
// the well, 50 Rahgot fallen, 60 his mask taken (objective 60: back to Valmir), 70 found out (objective 70: kill him),
// 200 done. Each stage's objectives (KFhObj by their own numbers):
static const short KStageObj[] = { 10, 10,  10, 50,  20, 20,  26, 30,  60, 60,  70, 70,  -1 };
static int Stage(const Game& g) { return g.iS.dun[FH_QUEST]; }
static void SetStage(Game& g, int aStage)
    {
    if (g.iS.dun[FH_QUEST] >= aStage) return;
    g.iS.dun[FH_QUEST] = (u8)aStage;
    int k, j;
    for (j = 0; KStageObj[j] >= 0; j += 2)
        if (KStageObj[j] == aStage)
            for (k = 0; k < (int)(sizeof(KFhObjN) / sizeof(KFhObjN[0])); k++) if (KFhObjN[k] == KStageObj[j + 1]) g.Msg(KFhObj[k]);
    // (the mask taken: Valmir is on the battlements now, with his soldier - out again as the stage has them)
    if (aStage == 60 && ((g.iS.grpIn >> G_FHOUT) & 1)) { g.StageOut(G_FHOUT); g.StageIn(G_FHOUT); }
    }

// the conversations' pages (dialogue ids DDF0 + DDF_*)
const char* GameFhDlgLine(int aId, int aPage)
    {
    if (aId < 0 || aId >= DDF_COUNT) aId = 0;
    int n = KFhDlg[aId * 2 + 1];
    return KFhLines[KFhDlg[aId * 2] + (aPage < 0 ? 0 : (aPage >= n ? n - 1 : aPage))];
    }
int GameFhDlgCount(int aId) { return aId >= 0 && aId < DDF_COUNT ? KFhDlg[aId * 2 + 1] : 1; }

// the journal's line for the quest (QuestText), while it is open
const char* GameFhObjective(const Game& g)
    {
    int s = Stage(g), k, want;
    if (s < 10 || s >= 200) return 0;
    // (the one shown: the journal's while it is to find or show, the key's, then the mask, Valmir, the kill)
    want = s < 20 ? 10 : (s < 26 ? 20 : (s < 40 ? 30 : (s < 60 ? 50 : (s < 70 ? 60 : 70))));
    for (k = 0; k < (int)(sizeof(KFhObjN) / sizeof(KFhObjN[0])); k++) if (KFhObjN[k] == want) return KFhObj[k];
    return 0;
    }

// Forelhost's people outside, by the quest's stage: Valmir in the courtyard (an Imperial officer, as he says); at the
// end on the battlements in Stormcloak armour with the soldier he is luring in; then the impostor to kill
void Game::SpawnFhOut()
    {
    if (!iFhKit) return;
    int s = Stage(*this);
    if (s >= 200) return;
    int i;
    if (s < 60)
        {
        i = SpawnEnt(E_VILLAGER, WX(184169), WZ(-113796), SY(0));
        if (i < 0) return;
        Ent& e = iS.e[i];
        e.role = ROLE_NAMED; e.pad1 = NM_VALMIR; e.pad2 = 0; e.var = 0;
        e.y = FloorAt(0, e.x, e.z, iR->GroundAt(e.x, e.z) + 2 * M);
        return;
        }
    if (s < 70)
        {
        i = SpawnEnt(E_VILLAGER, WX(184084), WZ(-113775), SY(26));
        if (i >= 0) { Ent& e = iS.e[i]; e.role = ROLE_NAMED; e.pad1 = NM_VALMIR; e.var = 1; e.y = FloorAt(0, e.x, e.z, iR->GroundAt(e.x, e.z) + 2 * M); }
        i = SpawnEnt(E_VILLAGER, WX(183960), WZ(-113946), SY(36));
        if (i >= 0) { Ent& e = iS.e[i]; e.role = ROLE_NAMED; e.pad1 = NM_SOLDIER; e.var = 0; e.y = FloorAt(0, e.x, e.z, iR->GroundAt(e.x, e.z) + 2 * M); }
        return;
        }
    // found out: Valmir fights (the soldier is gone)
    i = SpawnEnt(E_BANDIT, WX(184084), WZ(-113775), SY(26));
    if (i >= 0)
        {
        Ent& e = iS.e[i];
        e.pad1 = 2; e.var = 3; e.aggro = 1; e.hp = e.maxhp = 60;
        e.y = FloorAt(0, e.x, e.z, iR->GroundAt(e.x, e.z) + 2 * M);
        }
    // and the soldier he lured (UESP: both to be defeated)
    i = SpawnEnt(E_BANDIT, WX(183960), WZ(-113946), SY(36));
    if (i >= 0)
        {
        Ent& e = iS.e[i];
        e.pad1 = 3; e.var = 4; e.aggro = 1; e.hp = e.maxhp = 50;
        SetBanditKind(e, BK_1H);
        e.y = FloorAt(0, e.x, e.z, iR->GroundAt(e.x, e.z) + 2 * M);
        }
    }

// talking to Valmir
int Game::FhValmirTalk()
    {
    int s = Stage(*this);
    if (s < 10) { StartDialog(DDF0 + DDF_INTRO); return 1; }
    if (s < 60)
        {
        if (CountItem(IT_FH_JOURNAL) > 0 && !iS.dun[FH_JOURNAL]) StartDialog(DDF0 + DDF_JOURNAL);
        else StartDialog(DDF0 + DDF_MID);
        return 1;
        }
    if (s < 70) { StartDialog(DDF0 + DDF_END); return 1; }
    return 0;
    }

void Game::FhDialogDone(int aId)
    {
    switch (aId)
        {
        case DDF_INTRO:
            if (Stage(*this) < 10) { Msg("Quest started: Siege on the Dragon Cult"); SetStage(*this, 10); }
            if (CountItem(IT_FH_JOURNAL) > 0) SetStage(*this, 20);
            break;
        case DDF_JOURNAL:
            // he reads it, keeps it: the key to the well is in the crypt
            TakeItems(IT_FH_JOURNAL, 1); Msg("Skorm Snow-Strider's Journal removed");
            iS.dun[FH_JOURNAL] = 1;
            SetStage(*this, 26);
            break;
        case DDF_END:
            // found out: he must die
            SetStage(*this, 70);
            if ((iS.grpIn >> G_FHOUT) & 1) { StageOut(G_FHOUT); StageIn(G_FHOUT); }
            break;
        }
    }

void GameFhQuestProps(Game& g)
    {
    // Skorm Snow-Strider's journal where Skyrim leaves it in the Stronghold; the well's key in the crypt's chest by the
    // Draugr Wight (Skyrim puts it there: alias CryptChest); the well
    g.AddProp(PT_ITEM, CELL_FHSTRONG, Ix(1786), Iy(448) + g.iItemRest[GameItem(IT_FH_JOURNAL).mesh * 3 + 1], Iz(1795), SY(171), IT_FH_JOURNAL, 1, 0);
    int i, best = -1, bd = 0x7fffffff;
    for (i = 0; i < g.iS.nprop; i++)
        {
        const Prop& p = g.iS.prop[i];
        if (p.type != PT_CHEST || p.cell != CELL_FHCRYPT) continue;
        int d = Abs(p.x - Ix(2656)) + Abs(p.z - Iz(1056));
        if (d < bd) { bd = d; best = i; }
        }
    if (best >= 0) g.ContAdd(g.iS.prop[best].item, IT_FH_WELLKEY, 1);
    g.AddProp(PT_WELL, CELL_FHCRYPT, Ix(240), Iy(-784), Iz(4752), 0, 0, 0, 0);
    // (v0.4.72) the cult's notes and the books where Skyrim leaves them (fh_books.inc)
    for (i = 0; KFhBookPlace[i] >= 0; i += 6)
        {
        const int* b = KFhBookPlace + i;
        g.AddProp(PT_ITEM, b[1], Ix(b[2]), Iy(b[4]) + g.iItemRest[GameItem(b[0]).mesh * 3 + 1], Iz(b[3]), SY(b[5]), b[0], 1, 0);
        }
    }
const char* GameFhBook(int aId) { return KFhBooks[aId]; }

// climbing into the well (with its key): down it to the Refectory's marker (Skyrim's RefectoryMarker)
int GameFhUseWell(Game& g)
    {
    if (g.CountItem(IT_FH_WELLKEY) <= 0) { g.Msg("The well's grate is locked."); return 1; }
    g.EnterCell(CELL_FHREFECT, Ix(2446), Iz(8065), 0, Iy(240));
    if (Stage(g) >= 10) SetStage(g, 40);
    return 1;
    }

void GameFireWallUpdate(Game& g, int dt);
void Game::FhQuestUpdate(int dt)
    {
    GameFireWallUpdate(*this, dt);                 // (v0.4.71) the staff's flames burn on
    if (!iFhKit) return;
    int s = Stage(*this), i;
    // the journal picked up
    if (s >= 10 && s < 20 && CountItem(IT_FH_JOURNAL) > 0) { Msg("Skorm Snow-Strider's journal found"); SetStage(*this, 20); }
    // Rahgot's mask taken
    if (s >= 10 && s < 50)
        {
        // stage 50 as Skyrim has it: when Rahgot falls (the mask may be taken later)
        int dead = iS.dun[FH_RAHGOT];
        for (i = 0; i < iS.ne && !dead; i++) { const Ent& e = iS.e[i]; if (e.grp == G_FHREFECT && e.type == E_WIGHT && e.pad1 == 4 && !e.alive) dead = 1; }
        if (dead) SetStage(*this, 50);
        }
    if (s >= 10 && s < 60 && CountItem(IT_RAHGOT) > 0) { SetStage(*this, 50); SetStage(*this, 60); }
    if (iS.cell || iUi != UI_PLAY) return;
    for (i = 0; i < iS.ne; i++)
        {
        Ent& e = iS.e[i];
        if (e.grp != G_FHOUT) continue;
        int dx = Abs(e.x - iS.px), dz = Abs(e.z - iS.pz);
        // he calls out as you come near (dunForelhostForceGreetIntro), once; at the end the scene plays as you come
        if (e.type == E_VILLAGER && e.pad1 == NM_VALMIR && s < 10 && !iS.dun[FH_GREETED] && dx < 6 * M && dz < 6 * M)
            { iS.dun[FH_GREETED] = 1; StartDialog(DDF0 + DDF_INTRO); return; }
        if (e.type == E_VILLAGER && e.pad1 == NM_VALMIR && s >= 60 && s < 70 && dx < 12 * M && dz < 12 * M)
            { StartDialog(DDF0 + DDF_END); return; }
        }
    // the impostor dead: done
    if (s == 70 && ((iS.grpIn >> G_FHOUT) & 1))
        {
        int alive = 0;
        for (i = 0; i < iS.ne; i++) if (iS.e[i].grp == G_FHOUT && iS.e[i].type == E_BANDIT && iS.e[i].alive) alive = 1;
        if (!alive) { iS.dun[FH_QUEST] = 200; Msg("Quest completed: Siege on the Dragon Cult"); }
        }
    }

// ---------------------------------------------------------------- (v0.4.71) Rahgot's staff: a Wall of Flames
// Skyrim's Dragon Priest Staff (fire): sprayed on the ground it makes a wall of fire, 50 fire damage a second to what
// stands in it, for 30 s, ten walls at most. Here: five flames across the way, four metres ahead, a metre apart.
// (Skyrim's staffs run on charge, refilled with soul gems; there are none in the game yet - this one does not run out.)
void GameStaffCast(Game& g)
    {
    int k, best = 0;
    for (k = 0; k < 10; k++) if (g.iFireWall[k * 5 + 4] < g.iFireWall[best * 5 + 4]) best = k;     // (a free one, or the oldest)
    int fx = r3d_sin(g.iS.yaw), fz = r3d_cos(g.iS.yaw);
    int* w = g.iFireWall + best * 5;
    w[0] = g.iS.px + ((fx * 4 * M) >> 14); w[2] = g.iS.pz + ((fz * 4 * M) >> 14);
    w[1] = g.FloorAt(g.iS.cell, w[0], w[2], g.iS.py - 410 + M);
    w[3] = g.iS.yaw; w[4] = 30000;
    g.iShakeUs = 100000;
    }
// a flame's place: the k-th of a wall (-2..2 m across)
static void FlamePos(const int* w, int k, int& x, int& z)
    {
    int a = (k - 2) * M, c = r3d_cos(w[3]), s = r3d_sin(w[3]);
    x = w[0] + ((a * c) >> 14); z = w[2] - ((a * s) >> 14);
    }
void GameFireWallUpdate(Game& g, int dt)
    {
    int k, j, i;
    for (k = 0; k < 10; k++)
        {
        int* w = g.iFireWall + k * 5;
        if (w[4] <= 0) continue;
        w[4] -= dt / 1000;
        // 50 a second to what stands in it (a tenth of a second at a time)
        int t0 = (w[4] + dt / 1000) / 100, t1 = w[4] / 100;
        if (t0 == t1) continue;
        for (i = 0; i < g.iS.ne; i++)
            {
            Ent& e = g.iS.e[i];
            if (!e.alive || e.type < E_WOLF || e.type == E_CHEST || e.cell != g.iS.cell) continue;
            for (j = 0; j < 5; j++)
                {
                int x, z; FlamePos(w, j, x, z);
                if (Abs(e.x - x) < 3 * M / 4 && Abs(e.z - z) < 3 * M / 4 && Abs(e.y - w[1]) < 2 * M) { g.HurtEnt(e, 5, 0, x, z); e.aggro = 1; break; }
                }
            }
        }
    }
void GameFireWallDraw(Game& g)
    {
    R3D& r = *g.iR;
    int k, j;
    u8 sl[16];
    sl[0] = 70; sl[1] = 46; sl[2] = 28; sl[3] = TXI_FUR;
    sl[4] = 240; sl[5] = 96; sl[6] = 24; sl[7] = TXI_FUR;
    sl[8] = 255; sl[9] = 210; sl[10] = 80; sl[11] = TXI_FUR;
    sl[12] = 255; sl[13] = 246; sl[14] = 200; sl[15] = TXI_FUR;
    for (k = 0; k < 10; k++)
        {
        const int* w = g.iFireWall + k * 5;
        if (w[4] <= 0) continue;
        for (j = 0; j < 5; j++)
            {
            int x, z; FlamePos(w, j, x, z);
            r.DrawMesh(r.iMSt[ST_FIRE], ((g.iTimeUs >> 16) + j * 3) & 7, x, w[1], z, j * 200, 300, sl, MF_UNLIT);
            }
        }
    }
