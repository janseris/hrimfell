// game_ui.cpp - Hrimfell rendering: creatures, first-person view model, HUD, inventory, map, menus
#include "game.h"

static const int M = METER;

const ItemDef& GameItem(int aId);
const u8* GameItemCol(int aId);
const char* GameCatName(int aCat);
const char* GameBook(int aId);
const char* GameDlgLine(int aDlg, int aPage);
const char* GameKitSay(int aNamed, int k);
const char* GameWord(int i);
u32 GameZoneMask(Game& g);
int GameZoneOf(int aCell, int aX, int aZ);
const char* GameMerchantName(int m);
const char* GameNamedName(int i);
const u8* GameWrFolkLook(int aNamed);
int GameVillageX();
int GameVillageZ();
#include "plan_pos.inc"

// the city's houses can't be entered yet: near one of their doors and facing it, its name is shown
static int ShutDoorNear(const GameState& s)
    {
    if (s.cell) return -1;
    int fx = r3d_sin(s.yaw), fz = r3d_cos(s.yaw), i, best = -1, bd = (5 * METER / 2) >> 4;
    for (i = 0; i < KPlanDoor_N; i++)
        {
        int x = (CITY_CX * CELL + CELL / 2) + KPlanDoor[i * 2] * METER / 10, z = (CITY_CZ * CELL + CELL / 2) + KPlanDoor[i * 2 + 1] * METER / 10;
        int dx = (x - s.px) >> 4, dz = (z - s.pz) >> 4;
        int d = r3d_isqrt(dx * dx + dz * dz);
        if (d >= bd) continue;
        if (d > 8 && ((dx * fx + dz * fz) >> 14) < d / 2) continue;           // not facing it
        bd = d; best = i;
        }
    return best;
    }

int GameFullDist(int v, int aBase);
static inline int Abs(int v) { return v < 0 ? -v : v; }
static int Clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static void Slot(u8* s, int i, int r, int g, int b, int t) { s[i * 4] = (u8)r; s[i * 4 + 1] = (u8)g; s[i * 4 + 2] = (u8)b; s[i * 4 + 3] = (u8)t; }

static const u8 KTunic[5 * 3] = { 120, 40, 36,  50, 80, 120,  70, 100, 60,  130, 110, 60,  100, 70, 110 };
static const u8 KHair[4 * 3] = { 80, 56, 36,  200, 170, 100,  150, 80, 40,  60, 50, 44 };

// colours
#define C_WHITE 0xFFFF
#define C_GOLD  0xE6AE       // ~ (230,212,112)
#define C_GREY  0xA514
#define C_RED   0xD8C3
#define C_BLUE  0x3A7D
#define C_GREEN 0x4E09
#define C_DARK  0x18E3

const char* GameEntName(const Ent& e);
static int AtkDur(int aType)
    {
    switch (aType)
        {
        case E_WOLF: return 600000; case E_WIGHT: return 900000; case E_SKELETON: return 800000; case E_TROLL: return 1200000;
        case E_BANDIT: return 800000; case E_SKEEVER: return 600000; case E_SPIDER: return 900000;
        default: return 1000000;
        }
    }

// ---------------------------------------------------------------- world: creatures, projectiles
void Game::DrawEntities()
    {
    R3D& r = *iR;
    int i;
    for (i = 0; i < iS.ne; i++)
        {
        Ent& e = iS.e[i];
        if (e.cell != iS.cell) continue;
        int dx = (e.x - iS.px) >> 8, dz = (e.z - iS.pz) >> 8;
        int lim = e.type == E_DRAGON ? 200 : 110;
        if (dx * dx + dz * dz > lim * lim) continue;
        u8 sl[32];
        int fl = e.flash ? MF_FLASH : 0;
        int frame = 0;
        const Mesh* m = 0;
        if (e.type == E_CHEST)
            {
            Slot(sl, 0, 92, 52, 34, SL_TEX | TXI_FURN); Slot(sl, 1, 58, 56, 56, 1); Slot(sl, 2, 104, 60, 38, SL_TEX | TXI_FURN); Slot(sl, 3, 24, 16, 12, 1);
            r.DrawMesh(iMChest, e.looted ? 4 : 0, e.x, e.y, e.z, e.yaw, 256, sl, 0);
            continue;
            }
        int nearR = GameFullDist(iS.view, iS.cell ? 4 : 7);   // full models only close by (people fill the city and the inn; v0.4.45: 7 m and 4 m, from 10 and 5; v0.4.46: with the Full models setting, -1: never)
        int near = nearR >= 0 && dx * dx + dz * dz < nearR * nearR;
        // the barrow's hidden ones: the spider in the dark above, the Overlord in his sarcophagus
        if ((e.type == E_SPIDER || e.type == E_WIGHT) && (e.pad2 & 72)) continue;
        if (e.type == E_SKEEVER)
            {
            // a skeever: a big rat, grey-brown, a long pink tail (the wolf's frame, small)
            Slot(sl, 0, 84, 74, 64, 1); Slot(sl, 1, 54, 46, 40, 1); Slot(sl, 2, 196, 140, 130, 1);
            frame = !e.alive ? 16 : ((e.anim >> 5) & 15);
            r.DrawMesh(near ? iMWolf : iMWolfLod, frame, e.x, e.y, e.z, e.yaw, 118, sl, fl);
            continue;
            }
        if (e.type == E_SPIDER)
            {
            Slot(sl, 0, 186, 196, 200, 2); Slot(sl, 1, 70, 76, 84, 1); Slot(sl, 2, 150, 158, 160, 2); Slot(sl, 3, 40, 36, 34, 1); Slot(sl, 4, 120, 124, 126, 1);
            if (!e.alive) frame = 24;
            else if (e.atk > 0) { int dd = AtkDur(e.type); frame = 16 + Clamp((dd - e.atk) * 8 / dd, 0, 7); }
            else frame = (e.anim >> 6) & 15;
            r.DrawMesh(iMSpider, frame, e.x, e.y + e.fly, e.z, e.yaw, 300, sl, fl);
            continue;
            }
        if (e.type >= E_DEER && e.type <= E_MUDCRAB)
            {
            // the land's animals: deer (elk), hare, fox, goat, mudcrab
            int k = e.type == E_DEER ? (e.var ? 1 : 0) : (e.type == E_HARE ? 2 : (e.type == E_FOX ? 3 : (e.type == E_GOAT ? 4 : 5)));
            static const u8 KAnCol[6 * 12] =
                {
                138, 98, 62,   88, 64, 44,   214, 196, 170,  200, 186, 160,      // deer: red-brown, white belly
                120, 92, 66,   70, 54, 40,   190, 170, 140,  206, 192, 166,      // elk
                150, 140, 120, 110, 100, 86, 220, 214, 200,  0, 0, 0,            // hare
                190, 104, 46,  60, 40, 30,   232, 222, 206,  0, 0, 0,            // fox
                176, 166, 150, 120, 110, 98, 214, 206, 190,  110, 100, 86,       // goat
                120, 92, 66,   90, 70, 50,   150, 110, 80,   0, 0, 0             // mudcrab
                };
            const u8* c = KAnCol + k * 12;
            Slot(sl, 0, c[0], c[1], c[2], 1); Slot(sl, 1, c[3], c[4], c[5], 1); Slot(sl, 2, c[6], c[7], c[8], 1); Slot(sl, 3, c[9], c[10], c[11], 1);
            frame = !e.alive ? 16 : ((e.anim >> 6) & 15);
            r.DrawMesh(iMAnimal[k], frame, e.x, e.y, e.z, e.yaw, 256, sl, fl);
            continue;
            }
        if (e.type == E_WOLF)
            {
            Slot(sl, 0, 118, 114, 106, 1); Slot(sl, 1, 74, 70, 66, 1); Slot(sl, 2, 196, 188, 172, 1);
            frame = !e.alive ? 16 : ((e.anim >> 6) & 15);
            r.DrawMesh(near ? iMWolf : iMWolfLod, frame, e.x, e.y, e.z, e.yaw, 256, sl, fl);
            continue;
            }
        if (e.type == E_DRAGON)
            {
            Slot(sl, 0, 116, 50, 36, 1); Slot(sl, 1, 176, 128, 86, 1); Slot(sl, 2, 150, 76, 52, 1); Slot(sl, 3, 52, 34, 28, 1);
            if (!e.alive) frame = 24;
            else if (e.fly > M || e.state == D_TAKEOFF || e.state == D_FLY) frame = (e.anim >> 7) & 15;
            else frame = 16 + ((e.anim >> 8) & 7);
            r.DrawMesh(iMDragon, frame, e.x, e.y + e.fly, e.z, e.yaw, 256, sl, fl);
            continue;
            }
        // humanoids
        int kind = 0;
        // (v0.4.49) the belt's slot (6) and the one past it: a leather brown unless set - they were left as whatever
        // the stack held (the belts came out black, red, any colour)
        Slot(sl, 6, 74, 54, 38, 4); Slot(sl, 7, 74, 54, 38, 4);
        switch (e.type)
            {
            case E_VILLAGER:
                {
                const u8* t = KTunic + (e.var % 5) * 3;
                const u8* h = KHair + (e.var % 4) * 3;
                Slot(sl, 0, 86, 70, 52, 6); Slot(sl, 1, t[0], t[1], t[2], 6); Slot(sl, 2, 222, 180, 146, 7);
                Slot(sl, 3, 120, 120, 120, 1); Slot(sl, 4, h[0], h[1], h[2], 4); Slot(sl, 5, 70, 50, 36, 4);
                kind = (e.var & 1) ? 4 : 0;
                if (e.role == ROLE_NAMED && e.pad1 == 4)
                    {
                    // Vilja: a moss-green travelling dress over brown leggings, copper hair
                    Slot(sl, 0, 92, 70, 50, 6); Slot(sl, 1, 84, 112, 64, 6); Slot(sl, 4, 168, 92, 48, 4); Slot(sl, 5, 74, 52, 36, 4);
                    kind = 4;
                    }
                else if (e.role == ROLE_NAMED && e.pad1 == 5)
                    {
                    // the Jarl: a deep red tunic, a fur mantle, a gold circlet's glint in his hair
                    Slot(sl, 0, 70, 50, 36, 4); Slot(sl, 1, 130, 36, 32, 6); Slot(sl, 4, 150, 110, 60, 4); Slot(sl, 5, 110, 90, 70, 4);
                    kind = 0;
                    }
                else if (e.role == ROLE_NAMED && e.pad1 == 7)
                    {
                    // the hold's guard: mail under a mustard-yellow tabard
                    Slot(sl, 0, 70, 60, 46, 4); Slot(sl, 1, 176, 140, 60, 6); Slot(sl, 4, 120, 120, 124, 1);
                    kind = 0;
                    }
                else if (e.role == ROLE_NAMED && e.pad1 == 8)
                    {
                    // the court wizard: a long dark-blue robe
                    Slot(sl, 0, 44, 50, 80, 6); Slot(sl, 1, 54, 62, 104, 6); Slot(sl, 4, 120, 100, 80, 4);
                    kind = 1;
                    }
                else if (e.role == ROLE_NAMED && (e.pad1 == 2 || e.pad1 == 6))
                    {
                    // Hjordis: a fighter in mail and leather, fair hair
                    Slot(sl, 0, 70, 56, 42, 4); Slot(sl, 1, 126, 128, 132, 1); Slot(sl, 4, 214, 190, 130, 4);
                    kind = 4;
                    }
                if (e.role == ROLE_MERCHANT && e.pad1 == MER_RWTRADER)
                    {
                    // Lucan: a rust-red tunic under a dark leather vest, grey-green trousers, brown boots, black hair
                    Slot(sl, 0, 118, 124, 104, 6); Slot(sl, 1, 150, 76, 46, 6); Slot(sl, 2, 204, 156, 122, 7);
                    Slot(sl, 4, 34, 28, 24, 4); Slot(sl, 5, 112, 74, 46, 4);
                    kind = 0;
                    }
                else if (e.role == ROLE_MERCHANT && e.pad1 == MER_RWINN)
                    {
                    // Orgnar: a Nord barkeep in a brown apron over a grey tunic, dark hair and beard
                    Slot(sl, 0, 84, 70, 56, 6); Slot(sl, 1, 120, 98, 70, 6); Slot(sl, 4, 52, 40, 32, 4); Slot(sl, 5, 70, 52, 38, 4);
                    kind = 0;
                    }
                else if (e.role == ROLE_NAMED && e.pad1 == 19)
                    {
                    // Delphine: a Breton innkeeper in a dark green and brown dress, her hair tied back
                    Slot(sl, 0, 70, 60, 46, 6); Slot(sl, 1, 72, 84, 60, 6); Slot(sl, 2, 210, 166, 136, 7); Slot(sl, 4, 92, 64, 40, 4);
                    kind = 4;
                    }
                else if (e.role == ROLE_NAMED && e.pad1 == 18)
                    {
                    // Camilla: a pale yellow farm dress, dark brown hair pinned up
                    Slot(sl, 0, 176, 150, 86, 6); Slot(sl, 1, 204, 172, 96, 6); Slot(sl, 2, 214, 168, 134, 7);
                    Slot(sl, 4, 74, 50, 32, 4); Slot(sl, 5, 90, 64, 42, 4);
                    kind = 4;
                    }
                else if (e.role == ROLE_NAMED && (e.pad1 == 21 || e.pad1 == 22))
                    {
                    // Riverwood's children in ChildOutfit01: a plain roughspun tunic, dark trousers, boots
                    Slot(sl, 0, 78, 64, 50, 6); Slot(sl, 1, 140, 118, 86, 6); Slot(sl, 2, 226, 186, 152, 7);
                    Slot(sl, 4, e.pad1 == 21 ? 176 : 150, e.pad1 == 21 ? 120 : 104, e.pad1 == 21 ? 70 : 62, 4); Slot(sl, 5, 70, 50, 36, 4);
                    kind = e.pad1 == 21 ? 4 : 0;
                    }
                else if (e.role == ROLE_NAMED && e.pad1 >= 23 && e.pad1 <= 28)
                    {
                    // Whiterun's outskirts, in Skyrim's farm clothes (npcs.csv: outfit and skin tint): 03 a brown work
                    // tunic with gloves (Skulvar, Bjorlam), 02 a grey-green shirt (Jervar - with his hat - and Nimriel),
                    // 01 the tan belted tunic (Severio), Variant04 a faded blue shirt (Wilmuth)
                    static const u8 KWr[6 * 12] =
                        {
                        // trousers, shirt, skin, hair
                        76, 60, 44,  112, 82, 56,  112, 97, 86,   54, 44, 36,     // Skulvar
                        80, 70, 54,  104, 106, 84, 198, 176, 168, 150, 110, 64,   // Jervar
                        84, 66, 48,  168, 140, 98, 87, 61, 51,    34, 28, 24,     // Severio (Imperial)
                        80, 70, 54,  104, 106, 84, 154, 155, 151, 92, 64, 40,     // Nimriel (Wood Elf)
                        72, 62, 50,  84, 96, 112,  167, 134, 122, 120, 96, 70,    // Wilmuth
                        76, 60, 44,  112, 82, 56,  167, 134, 122, 70, 54, 40      // Bjorlam
                        };
                    const u8* w = KWr + (e.pad1 - 23) * 12;
                    Slot(sl, 0, w[0], w[1], w[2], 6); Slot(sl, 1, w[3], w[4], w[5], 6); Slot(sl, 2, w[6], w[7], w[8], 7);
                    Slot(sl, 4, w[9], w[10], w[11], 4); Slot(sl, 5, w[9], w[10], w[11], 4);
                    kind = e.pad1 == 26 ? 4 : 0;
                    }
                else if (e.role == ROLE_NAMED && (e.pad1 == 53 || e.pad1 == 54))
                    {
                    // (v0.4.69) Valmir (a High Elf: GuardImperialOutfit's red and steel; at the end a Stormcloak
                    // officer's blue), the Stormcloak soldier
                    int sons = e.pad1 == 54 || e.var == 1;
                    if (sons) { Slot(sl, 0, 60, 54, 46, 4); Slot(sl, 1, 54, 70, 104, 6); }
                    else { Slot(sl, 0, 70, 60, 50, 4); Slot(sl, 1, 140, 40, 34, 6); }
                    Slot(sl, 2, e.pad1 == 53 ? 153 : 214, e.pad1 == 53 ? 141 : 172, e.pad1 == 53 ? 85 : 140, 7);
                    Slot(sl, 3, 130, 130, 134, 1); Slot(sl, 4, e.pad1 == 53 ? 190 : 120, e.pad1 == 53 ? 160 : 90, e.pad1 == 53 ? 100 : 60, 4); Slot(sl, 5, 70, 52, 38, 4);
                    kind = 0;
                    }
                else if (e.role == ROLE_NAMED && GameWrFolkLook(e.pad1))
                    {
                    // (v0.4.65) Whiterun's own people (wr_folk.inc, from npcs.csv: sex, skin tint, outfit, race)
                    const u8* w = GameWrFolkLook(e.pad1);
                    Slot(sl, 0, w[8], w[9], w[10], 6); Slot(sl, 1, w[5], w[6], w[7], 6); Slot(sl, 2, w[2], w[3], w[4], 7);
                    Slot(sl, 4, w[11], w[12], w[13], 4); Slot(sl, 5, w[11], w[12], w[13], 4);
                    kind = w[0] ? 4 : 0;
                    }
                if (e.role == ROLE_CARAVAN || e.role == ROLE_TRADER)
                    {
                    // cat-folk of the caravan: sandy or tabby fur, desert robes
                    static const u8 KFur[3 * 3] = { 176, 140, 92,  150, 116, 80,  120, 104, 92 };
                    static const u8 KRobe[3 * 3] = { 150, 60, 40,  70, 90, 120,  170, 130, 60 };
                    const u8* f = KFur + (e.var % 3) * 3; const u8* c = KRobe + (e.var % 3) * 3;
                    Slot(sl, 0, c[0] * 3 / 4, c[1] * 3 / 4, c[2] * 3 / 4, 6); Slot(sl, 1, c[0], c[1], c[2], 6);
                    Slot(sl, 2, f[0], f[1], f[2], 6); Slot(sl, 4, f[0] * 4 / 5, f[1] * 4 / 5, f[2] * 4 / 5, 6); Slot(sl, 5, 40, 30, 26, 4);
                    kind = 5;
                    }
                break;
                }
            case E_SMITH:
                Slot(sl, 0, 70, 54, 40, 4); Slot(sl, 1, 96, 70, 46, 4); Slot(sl, 2, 216, 168, 134, 7);
                Slot(sl, 3, 120, 120, 120, 1); Slot(sl, 4, 176, 84, 40, 4); Slot(sl, 5, 56, 42, 32, 4);
                kind = 4;
                break;
            case E_ELDER:
                Slot(sl, 0, 86, 92, 108, 6); Slot(sl, 1, 96, 102, 120, 6); Slot(sl, 2, 210, 170, 140, 7);
                Slot(sl, 3, 104, 76, 46, 4); Slot(sl, 4, 226, 226, 222, 2); Slot(sl, 5, 60, 48, 40, 4); Slot(sl, 6, 110, 80, 50, 4);
                kind = 1;
                break;
            case E_WIGHT:
                Slot(sl, 0, 56, 52, 46, 1); Slot(sl, 1, 84, 86, 80, 1); Slot(sl, 2, 150, 158, 146, 2);
                Slot(sl, 3, 120, 124, 128, 1); Slot(sl, 4, 64, 64, 60, 1); Slot(sl, 5, 48, 44, 40, 4); Slot(sl, 6, 110, 90, 60, 4);
                if (e.pad1 == 1) { Slot(sl, 1, 70, 78, 92, 1); Slot(sl, 4, 80, 96, 120, 1); }          // Restless: frost-touched
                if (e.pad1 == 2) { Slot(sl, 0, 40, 38, 36, 1); Slot(sl, 1, 92, 84, 64, 1); Slot(sl, 3, 150, 140, 110, 1); Slot(sl, 4, 52, 48, 44, 1); }   // Overlord: dark plate, bronze
                if (e.pad1 == 3) { Slot(sl, 0, 120, 150, 176, 2); Slot(sl, 1, 150, 180, 204, 2); Slot(sl, 2, 176, 204, 224, 2); Slot(sl, 3, 170, 200, 220, 2); Slot(sl, 4, 140, 170, 196, 2); Slot(sl, 5, 110, 140, 168, 2); }   // (v0.4.68) the ghosts: pale, blue-white
                if (e.pad1 == 4) { Slot(sl, 0, 46, 36, 30, 4); Slot(sl, 1, 70, 52, 40, 4); Slot(sl, 3, 120, 108, 80, 1); Slot(sl, 4, 120, 104, 70, 1); Slot(sl, 6, 60, 46, 34, 4); }   // Rahgot: robes, the bronze mask
                if (e.pad1 == 5) { Slot(sl, 1, 74, 70, 60, 1); Slot(sl, 4, 60, 56, 50, 1); }        // a ruin's boss: darker plate
                kind = 2;
                break;
            case E_BANDIT:
                {
                // fur and hide over leather, a few in iron; Arvel the Swift a Dunmer in hide with a hide helmet
                static const u8 KFurA[4 * 3] = { 110, 84, 60,  92, 70, 52,  120, 104, 80,  80, 84, 88 };
                const u8* a = KFurA + (e.var % 4) * 3;
                const u8* h = KHair + (e.var % 4) * 3;
                Slot(sl, 0, 64, 50, 38, 4); Slot(sl, 1, a[0], a[1], a[2], 4); Slot(sl, 2, 214, 172, 140, 7);
                Slot(sl, 3, 110, 110, 112, 1); Slot(sl, 4, h[0], h[1], h[2], 4); Slot(sl, 5, 60, 44, 32, 4);
                kind = (e.var % 3 == 1) ? 4 : 0;
                if (e.pad1 == 1) { Slot(sl, 1, 120, 96, 66, 4); Slot(sl, 2, 120, 126, 140, 7); Slot(sl, 4, 108, 84, 58, 4); kind = 0; }
                if (e.pad1 == 2 || e.pad1 == 3) { Slot(sl, 0, 60, 54, 46, 4); Slot(sl, 1, 54, 70, 104, 6); Slot(sl, 3, 130, 130, 134, 1); kind = 0;    // (v0.4.71) Valmir and his soldier: Stormcloak blue
                    if (e.pad1 == 2) { Slot(sl, 2, 153, 141, 85, 7); Slot(sl, 4, 190, 160, 100, 4); } }
                break;
                }
            case E_SKELETON:
                // old yellowed bone, dark hollows, a corroded ancient blade with a leather-wrapped grip
                Slot(sl, 0, 206, 196, 168, 2); Slot(sl, 1, 46, 40, 34, 1); Slot(sl, 2, 186, 176, 148, 2);
                Slot(sl, 3, 120, 112, 86, 1); Slot(sl, 4, 150, 140, 116, 2); Slot(sl, 5, 170, 160, 132, 2); Slot(sl, 6, 70, 52, 36, 4);
                kind = 6;
                break;
            case E_TROLL:
                Slot(sl, 0, 120, 120, 106, 2); Slot(sl, 1, 140, 142, 126, 2); Slot(sl, 2, 132, 140, 118, 2);
                Slot(sl, 3, 92, 66, 42, 4); Slot(sl, 4, 214, 214, 204, 2); Slot(sl, 5, 100, 102, 90, 2); Slot(sl, 6, 96, 70, 46, 4);
                kind = 3;
                break;
            default: continue;
            }
        m = near ? &iMHuman[kind] : &iMHumanLod[kind];
        int esc = e.type == E_WIGHT && (e.pad1 == 2 || e.pad1 == 4) ? 300 : 256;
        if (e.type == E_VILLAGER && e.role == ROLE_NAMED && (e.pad1 == 21 || e.pad1 == 22 || (GameWrFolkLook(e.pad1) && GameWrFolkLook(e.pad1)[1]))) esc = 184;    // a child: about 1.3 m
        if (!e.alive) frame = 24;
        else if (e.atk > 0)
            {
            int d = AtkDur(e.type);
            frame = 16 + Clamp((d - e.atk) * 8 / d, 0, 7);
            }
        else if (GameChopping(*this, e) && Abs(e.x - iS.px) + Abs(e.z - iS.pz) > 4 * M) frame = 16 + (int)((iTimeUs >> 17) & 7);   // Faendal's axe
        else if (e.state == S_IDLE && e.type <= E_SMITH) frame = 0;
        else frame = (e.anim >> 6) & 15;
        r.DrawMesh(*m, frame, e.x, e.y, e.z, e.yaw, esc, sl, fl);
        }
    // projectiles
    for (i = 0; i < MAXP; i++)
        {
        Proj& p = iP[i];
        if (!p.active) continue;
        if (p.pad == 5)
            {
            // an arrow in flight, along its way (stuck where it hit for a moment)
            const u8* c = GameItemCol(IT_IRON_ARROW);
            u8 as[12];
            Slot(as, 0, c[0], c[1], c[2], 1); Slot(as, 1, c[3], c[4], c[5], 1); Slot(as, 2, c[6], c[7], c[8], 1);
            int hs = r3d_isqrt((p.vx >> 4) * (p.vx >> 4) + (p.vz >> 4) * (p.vz >> 4)) << 4;
            int el = ((r3d_atan2(p.vy, hs) + 512) & 1023) - 512;
            r.DrawMeshRot(iMItem[IM_ARROW], 0, p.x, p.y, p.z, r3d_atan2(p.vx, p.vz), 256 - el, 256, as, 0);
            continue;
            }
        u8 sl[8];
        Slot(sl, 0, 255, 214, 90, 1); Slot(sl, 1, 255, 110, 30, 1);
        if (p.pad == 3) { Slot(sl, 0, 220, 240, 255, 1); Slot(sl, 1, 120, 170, 240, 1); }        // frost
        if (p.pad == 4) { Slot(sl, 0, 200, 230, 140, 1); Slot(sl, 1, 110, 160, 60, 1); }         // venom
        int sc = p.kind == 2 ? 256 + (350000 - p.life) * 3 / 1000 : (p.owner ? 420 : 200);
        r.DrawMesh(iMFireball, 0, p.x, p.y, p.z, (iTimeUs >> 12) & 1023, sc, sl, MF_UNLIT | MF_NOFOG);
        }
    }

// first-person: weapon in the right hand, shield or flame in the left.
// Positions are tuned for the 320x200 view (fx = fy = 160) and rescaled so the weapon covers the same
// part of the screen in every display mode.
void Game::ViewMesh(const Mesh& m, int aFrame, int x, int y, int z, int yaw, int pitch, int roll, const u8* sl, int fl)
    {
    R3D& r = *iR;
    int k = r.iFy;                        // z' = z * fy / 160
    int zz = z * k / 160;
    int xx = x * k / r.iFx;               // keeps x/z * fx constant
    r.DrawMeshView(m, aFrame, xx, y, zz, yaw, pitch, roll, sl, fl);
    }

void Game::DrawViewModel()
    {
    R3D& r = *iR;
    if (iHideView || iS.third) return;
    int up = DrawAmount();              // 0 sheathed .. 256 drawn
    int t = iTimeUs >> 10;
    int moving = (iKeys & (KEY_FWD | KEY_BACK | KEY_STRAFE_L | KEY_STRAFE_R)) != 0;
    int bobY = moving ? (r3d_sin(t * 6) * 10) >> 14 : (r3d_sin(t) * 3) >> 14;
    int bobX = moving ? (r3d_sin(t * 3) * 8) >> 14 : 0;
    int gl = EquippedId(EQ_HANDS);
    u8 hs[8];
    if (gl >= 0) { const u8* c = GameItemCol(gl); Slot(hs, 0, c[0], c[1], c[2], 1); }
    else Slot(hs, 0, 214, 168, 132, 7);
    int body = EquippedId(EQ_BODY);
    if (body >= 0) { const u8* c = GameItemCol(body); Slot(hs, 1, c[0], c[1], c[2], 1); }
    else Slot(hs, 1, 110, 90, 70, 6);
    int torch = EquippedId(EQ_SHIELD) == IT_TORCH;
    if (torch && iGrab < 0)
        {
        // the torch held up in the left hand, whether the weapon is out or not
        const u8* c = GameItemCol(IT_TORCH);
        u8 sl[12];
        Slot(sl, 0, c[0], c[1], c[2], 4); Slot(sl, 1, c[3], c[4], c[5], 1); Slot(sl, 2, 0, 0, 0, 1);
        int tx = -205 - bobX, ty = -185 + bobY, tz = 250;
        ViewMesh(iMItem[IM_TORCH], 0, tx, ty, tz, 0, 0, 0, sl, 0);
        ViewMesh(iMHand, 0, tx, ty + 30, tz, 40, 0, -80, hs, 0);
        u8 fs[12];
        Slot(fs, 0, 255, 200, 70, 1); Slot(fs, 1, 240, 96, 24, 1); Slot(fs, 2, 255, 246, 200, 1);
        int fy = (r3d_sin(t * 9) * 4) >> 14;
        ViewMesh(iMFlame, (t >> 6) & 7, tx, ty + 136 + fy, tz, t & 1023, 0, 0, fs, MF_UNLIT);
        }
    if (up <= 0) return;
    // ---- right hand + weapon ----
    // idle: grip low on the right, blade up and leaning left/forward
    int x = 110 + bobX, y = -146 + bobY - ((256 - up) * 190 >> 8), z = 250;
    int yaw = -30, pitch = 60 + ((256 - up) * 60 >> 8), roll = 70;
    if (iSwingUs > 0)
        {
        int p = (450000 - iSwingUs) * 256 / 450000;     // 0..256
        // keyframes: wind-up (raise, tilt back/right), strike (sweep left + chop forward), recover
        int kx, ky, kp, kr, ky2;
        if (p < 80)        { int k = p * 256 / 80;          kx = 30 * k >> 8;           ky = 40 * k >> 8;           kp = -70 * k >> 8;            kr = -110 * k >> 8;          ky2 = 0; }
        else if (p < 170)  { int k = (p - 80) * 256 / 90;   kx = 30 - (220 * k >> 8);   ky = 40 - (90 * k >> 8);    kp = -70 + (330 * k >> 8);    kr = -110 + (330 * k >> 8);  ky2 = 60 * k >> 8; }
        else               { int k = (p - 170) * 256 / 86;  if (k > 256) k = 256;
                             kx = -190 + (190 * k >> 8);  ky = -50 + (50 * k >> 8);   kp = 260 - (260 * k >> 8);    kr = 220 - (220 * k >> 8);   ky2 = 60 - (60 * k >> 8); }
        x += kx; y += ky; pitch += kp; roll += kr; yaw += ky2;
        }
    int w = EquippedId(EQ_WEAPON);
    if (w >= 0)
        {
        const ItemDef& d = GameItem(w);
        const u8* c = GameItemCol(w);
        u8 sl[12];
        Slot(sl, 0, c[0], c[1], c[2], 1); Slot(sl, 1, c[3], c[4], c[5], 4); Slot(sl, 2, c[6], c[7], c[8], 1);
        ViewMesh(iMItem[d.mesh], 0, x, y, z, yaw, pitch, roll, sl, 0);
        }
    ViewMesh(iMHand, 0, x, y, z, yaw, pitch, roll, hs, 0);
    // ---- left hand ----
    int sh = EquippedId(EQ_SHIELD);
    if (torch) { }
    else if (sh >= 0)
        {
        const ItemDef& d = GameItem(sh);
        const u8* c = GameItemCol(sh);
        u8 sl[12];
        Slot(sl, 0, c[0], c[1], c[2], 4); Slot(sl, 1, c[3], c[4], c[5], 4); Slot(sl, 2, c[6], c[7], c[8], 1);
        ViewMesh(iMItem[d.mesh], 0, -150 - bobX, -110 + bobY - ((256 - up) * 190 >> 8), 230, 120, 256 + 40, 0, sl, 0);
        }
    else if (iS.mp >= (20 << 8))
        {
        // flame spell ready in the left hand
        u8 sl[12];
        Slot(sl, 0, 255, 200, 70, 1); Slot(sl, 1, 240, 96, 24, 1); Slot(sl, 2, 255, 246, 200, 1);
        int fy = (r3d_sin(t * 8) * 6) >> 14;
        int dn = (256 - up) * 190 >> 8;
        ViewMesh(iMHandOpen, 0, -112 - bobX, -150 + bobY - dn, 230, 60, 210, -30, hs, 0);
        ViewMesh(iMFlame, (t >> 6) & 7, -62 - bobX, -112 + bobY + fy - dn, 236, t & 1023, 0, 0, sl, MF_UNLIT);
        }
    }

// extents of a mesh after the preview tilt (pitch about x, roll about z), measured around its pivot:
// aEy = max |y| (constant while spinning around the vertical axis), aEr = max distance from that axis
static void ViewExtent(const Mesh& m, int aPivX, int aPivY, int aPivZ, int aPitch, int aRoll, int& aEy, int& aEr)
    {
    int cp = r3d_cos(aPitch), sp = r3d_sin(aPitch), cr = r3d_cos(aRoll), sr = r3d_sin(aRoll);
    int i, ey = 1, er2 = 1;
    for (i = 0; i < m.nv; i++)
        {
        int x = m.v[i * 3] - aPivX, y = m.v[i * 3 + 1] - aPivY, z = m.v[i * 3 + 2] - aPivZ;
        int x1 = (x * cr - y * sr) >> 14, y1 = (x * sr + y * cr) >> 14;     // roll
        int y2 = (y1 * cp - z * sp) >> 14, z2 = (y1 * sp + z * cp) >> 14;  // pitch
        if (y2 < 0) y2 = -y2;
        if (y2 > ey) ey = y2;
        int r2 = x1 * x1 + z2 * z2;
        if (r2 > er2) er2 = r2;
        }
    aEy = ey; aEr = r3d_isqrt(er2);
    }

// ---------------------------------------------------------------- 2D helpers
static void LocIcon(R3D& r, int x, int y, int aKind, u16 aCol, u16 aRim);
int GameKitOutWall(const Game& g, int i, int* aX0, int* aZ0, int* aX1, int* aZ1);
// scaled text, only what falls between x = aL and aR (the "discovered" banner's wipe)
static void BigTextClip(R3D& r, int x, int y, const char* s, u16 col, int sc, int aL, int aR)
    {
    extern const unsigned char* GameFont();
    const unsigned char* f = GameFont();
    for (; *s; s++, x += 6 * sc)
        {
        int c = (unsigned char)*s;
        if (c < 32 || c > 126 || x + 5 * sc <= aL || x >= aR) continue;
        const unsigned char* g = f + (c - 32) * 8;
        int yy, xx;
        for (yy = 0; yy < 8; yy++)
            for (xx = 0; xx < 5; xx++)
                if (g[yy] & (16 >> xx))
                    {
                    int a = x + xx * sc, b = a + sc;
                    if (a < aL) a = aL;
                    if (b > aR) b = aR;
                    if (b > a) r.Rect(a, y + yy * sc, b - a, sc, col);
                    }
        }
    }
static void BigText(R3D& r, int x, int y, const char* s, u16 col, int sc)
    {
    // scaled font for titles (uses the engine font through 1-pixel text at offsets)
    extern const unsigned char* GameFont();
    const unsigned char* f = GameFont();
    for (; *s; s++, x += 6 * sc)
        {
        int c = (unsigned char)*s;
        if (c < 32 || c > 126) continue;
        const unsigned char* g = f + (c - 32) * 8;
        int yy, xx;
        for (yy = 0; yy < 8; yy++)
            for (xx = 0; xx < 5; xx++)
                if (g[yy] & (16 >> xx)) r.Rect(x + xx * sc, y + yy * sc, sc, sc, col);
        }
    }

void Game::DrawWrapped(int aX, int aY, int aW, const char* s, u16 aCol, int aMaxLines)
    {
    int maxc = aW / 6;
    char line[110];
    int lines = 0;
    while (*s && lines < aMaxLines)
        {
        while (*s == ' ') s++;
        int n = 0, lastSp = -1;
        while (s[n] && s[n] != '|' && s[n] != '\n' && n < maxc)
            {
            if (s[n] == ' ') lastSp = n;
            n++;
            }
        int take = n;
        if (s[n] && s[n] != '|' && s[n] != '\n' && lastSp > 0) take = lastSp;
        int k;
        for (k = 0; k < take && k < 109; k++) line[k] = s[k];
        line[k] = 0;
        iR->Text(aX, aY + lines * 10, line, aCol);
        lines++;
        s += take;
        if (*s == '\n') s++;          // (v0.4.72: a line of its own - Skyrim's books' paragraphs)
        if (*s == '|') break;
        }
    }

// ---------------------------------------------------------------- HUD pieces
void Game::DrawCompass(int x0, int w)
    {
    R3D& r = *iR;
    int cw = w - 60;
    if (cw > 300) cw = 300;
    int cx = x0 + w / 2;
    int left = cx - cw / 2;
    r.Shade(left, 1, cw, 11);
    r.Rect(left, 12, cw, 1, C_GREY);
    // directions: 0 = north (+z)
    static const char* const KDir[8] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
    int i;
    for (i = 0; i < 16; i++)
        {
        int b = i * 64;
        int rel = ((b - iS.yaw + 512) & 1023) - 512;
        if (Abs(rel) > 256) continue;
        int x = cx + rel * (cw / 2) / 256;
        if (i & 1) { r.Rect(x, 9, 1, 3, C_GREY); continue; }
        const char* s = KDir[i >> 1];
        r.Text(x - r.TextW(s) / 2, 2, s, (i == 0) ? C_GOLD : C_WHITE);
        }
    // quest markers
    int qx[4], qz[4], qn;
    QuestTargets(qx, qz, qn);
    for (i = 0; i < qn; i++)
        {
        int b = r3d_atan2(qx[i] - iS.px, qz[i] - iS.pz);
        int rel = ((b - iS.yaw + 512) & 1023) - 512;
        int x = cx + Clamp(rel, -256, 256) * (cw / 2) / 256;
        int k;
        for (k = 0; k < 4; k++) r.Rect(x - 3 + k, 3 + k, 7 - 2 * k, 1, C_GOLD);
        }
    // the places with map markers within a kilometre: white when found, black until then
    {
    int px = iS.cell ? iS.extX : iS.px, pz = iS.cell ? iS.extZ : iS.pz;
    for (i = 0; i < GameLocN(); i++)
        {
        LocInfo l; GameLoc(*this, i, l);
        int dx = l.x - px, dz = l.z - pz;
        if (Abs(dx) > 1000 * M || Abs(dz) > 1000 * M) continue;
        int dd = r3d_isqrt((dx >> 8) * (dx >> 8) + (dz >> 8) * (dz >> 8));
        if (dd > 1000) continue;
        int b = r3d_atan2(dx, dz);
        int rel = ((b - iS.yaw + 512) & 1023) - 512;
        if (Abs(rel) > 256) continue;
        int x = cx + rel * (cw / 2) / 256;
        int found = (iS.spare[1] >> i) & 1;
        LocIcon(r, x, 6, l.kind, found ? C_WHITE : R3D::Rgb(8, 8, 8), found ? R3D::Rgb(30, 28, 24) : R3D::Rgb(120, 116, 106));
        }
    }
    // hostile creatures that noticed you
    for (i = 0; i < iS.ne; i++)
        {
        Ent& e = iS.e[i];
        if (!e.alive || !(e.aggro || (e.type == E_DRAGON && e.state != D_SLEEP))) continue;
        int dx = e.x - iS.px, dz = e.z - iS.pz;
        if (Abs(dx) > 70 * M || Abs(dz) > 70 * M) continue;
        int b = r3d_atan2(dx, dz);
        int rel = ((b - iS.yaw + 512) & 1023) - 512;
        if (Abs(rel) > 256) continue;
        int x = cx + rel * (cw / 2) / 256;
        r.Rect(x - 1, 9, 3, 3, C_RED);
        }
    }

static void Bar(R3D& r, int x, int y, int w, int v, int vmax, u16 col)
    {
    r.Rect(x - 1, y - 1, w + 2, 6, 0);
    r.Rect(x, y, w, 4, C_DARK);
    int f = vmax > 0 ? v * w / vmax : 0;
    // grows from the centre like the classic RPG bars
    r.Rect(x + (w - f) / 2, y, f, 4, col);
    }

// as Skyrim's: a bar shows while it is not full, and for a few seconds after it changed (Game::Tick: iBarUs)
void Game::DrawBars(int x0, int w)
    {
    R3D& r = *iR;
    int bw = w / 4;
    int y = OUT_H - 9;
    if (iBarUs[1] > 0 || (iS.mp >> 8) < iS.mpMax) Bar(r, x0 + 10, y, bw, iS.mp >> 8, iS.mpMax, C_BLUE);
    if (iBarUs[0] > 0 || (iS.hp >> 8) < iS.hpMax) Bar(r, x0 + w / 2 - bw / 2, y, bw, iS.hp >> 8, iS.hpMax, C_RED);
    if (iBarUs[2] > 0 || (iS.sp >> 8) < iS.spMax) Bar(r, x0 + w - 10 - bw, y, bw, iS.sp >> 8, iS.spMax, C_GREEN);
    }

void Game::DrawDialog(int x0, int w)
    {
    R3D& r = *iR;
    char who[24];
    int n = 0;
    const char* text;
    if (iDlgPage <= -1000)
        {
        // (v0.4.53) a named person's own greeting from Skyrim: their name, the line
        int q = -1000 - iDlgPage;
        const char* nm = GameNamedName(q >> 6);
        while (nm[n] && n < 23) { who[n] = nm[n]; n++; }
        who[n] = 0;
        text = GameKitSay(q >> 6, q & 63);
        }
    else
        {
        const char* line = GameDlgLine(iDlgId < 0 ? 0 : iDlgId, iDlgPage);
        while (line[n] && line[n] != '|' && n < 23) { who[n] = line[n]; n++; }
        who[n] = 0;
        text = line[n] == '|' ? line + n + 1 : line;
        }
    int bx = x0 + 8, bw = w - 16, by = 118, bh = 72;
    r.Shade(bx, by, bw, bh);
    r.Rect(bx, by, bw, 1, C_GOLD);
    r.Text(bx + 6, by + 5, who, C_GOLD);
    DrawWrapped(bx + 6, by + 18, bw - 12, text, C_WHITE, 5);
    r.Text(bx + bw - 6 - r.TextW("[E] continue"), by + bh - 10, "[E] continue", C_GREY);
    }

void Game::DrawHud()
    {
    R3D& r = *iR;
    int x0 = r.OutX0(), w = r.OutW();
    int cx = x0 + w / 2;
    char buf[80];
    // hurt / shout screen effects
    if (iHurtUs > 0)
        {
        // the red edge, thick on the side the blow came from (ahead: the top, behind: the bottom), and a marker
        // on the edge pointing at it
        int rel = ((iHurtFrom - iS.yaw + 512) & 1023) - 512;
        int b = 5, B = 16;
        int top = Abs(rel) < 160 ? B : b, bot = Abs(rel) > 352 ? B : b;
        int lft = rel < -96 && rel > -416 ? B : b, rgt = rel > 96 && rel < 416 ? B : b;
        r.Tint(x0, 0, w, top, C_RED); r.Tint(x0, OUT_H - bot, w, bot, C_RED);
        r.Tint(x0, top, lft, OUT_H - top - bot, C_RED); r.Tint(x0 + w - rgt, top, rgt, OUT_H - top - bot, C_RED);
        // the marker: where the direction leaves an ellipse round the middle of the screen
        int mx = cx + ((r3d_sin(rel) * (w / 2 - 22)) >> 14), my = OUT_H / 2 - ((r3d_cos(rel) * (OUT_H / 2 - 20)) >> 14);
        int k;
        for (k = 0; k < 11; k++)
            {
            // a wedge pointing outwards
            int ox = (r3d_sin(rel) * (11 - k)) >> 14, oy = -((r3d_cos(rel) * (11 - k)) >> 14);
            r.Rect(mx + ox - k / 2, my + oy - k / 2, k + 1, k + 1, C_RED);
            }
        }
    if (iShoutFxUs > 0 && iS.bfb[BF_WORD] > 0)
        {
        char* p = buf;
        int i;
        for (i = 0; i < iS.bfb[BF_WORD] && i < 3; i++) { if (i) *p++ = ' '; p = r3d_puts(p, GameWord(i)); }
        *p++ = '!'; *p = 0;
        int sc = w >= 400 ? 3 : 2;
        int tw = r.TextW(buf) * sc;
        BigText(r, cx - tw / 2 + 1, 61, buf, 0, sc);
        BigText(r, cx - tw / 2, 60, buf, 0xBEFF, sc);
        }
    DrawCompass(x0, w);
    // a place found: "<NAME> DISCOVERED" on one line, large, wiped on from the left, held, then wiped off to the right
    if (iDiscUs > 0)
        {
        LocInfo l; GameLoc(*this, iDiscLoc, l);
        char up[64];
        int k;
        for (k = 0; l.name[k] && k < 48; k++) up[k] = (l.name[k] >= 'a' && l.name[k] <= 'z') ? l.name[k] - 32 : l.name[k];
        r3d_puts(up + k, " DISCOVERED");
        int sc = w >= 400 ? 3 : 2;
        if (r.TextW(up) * sc > w - 8) sc--;
        int tw = r.TextW(up) * sc, lx = cx - tw / 2, t = DISC_US - iDiscUs;
        const int KIn = 900000, KOut = DISC_US - 900000;
        int right = t < KIn ? lx + (int)((long long)(tw + 8) * t / KIn) : lx + tw + 8;
        int left = t > KOut ? lx + (int)((long long)(tw + 8) * (t - KOut) / 900000) : lx - 8;
        BigTextClip(r, lx + 1, 25, up, 0, sc, left, right);
        BigTextClip(r, lx, 24, up, C_WHITE, sc, left, right);
        }
    // X: the model the crosshair is on (to name what is not right)
    if (iShowModel)
        {
        const char* nm = LookedAtModel();
        if (nm && *nm) { r.Shade(x0 + 2, 14, r.TextW(nm) + 4, 10); r.Text(x0 + 4, 15, nm, C_GOLD); }
        }
    // crosshair
    r.Rect(cx - 1, 99, 3, 1, C_WHITE); r.Rect(cx, 98, 1, 3, C_WHITE);
    // crouching: Skyrim's eye above the crosshair
    if (iCrouchY > 75)
        {
        static const signed char KEye[] = { -3, 3, -3,  -5, -4, -2,  4, 5, -2,  -6, -6, -1,  6, 6, -1,  -6, -6, 0,  6, 6, 0,
                                            -1, 1, 0,  -5, -4, 1,  4, 5, 1,  -3, 3, 2 };   // (x0, x1, y) runs
        int k;
        for (k = 0; k < (int)sizeof(KEye); k += 3) r.Rect(cx + KEye[k], 88 + KEye[k + 2], KEye[k + 1] - KEye[k] + 1, 1, C_WHITE);
        r.Rect(cx - 1, 87, 3, 3, C_WHITE);
        }
    // target health
    if (iTargetUs > 0 && iTarget >= 0 && iTarget < iS.ne && iS.e[iTarget].alive)
        {
        Ent& e = iS.e[iTarget];
        const char* nm = GameEntName(e);
        int bw = w / 3;
        r.TextShadow(cx - r.TextW(nm) / 2, 16, nm, C_WHITE);
        Bar(r, cx - bw / 2, 26, bw, e.hp, e.maxhp, C_RED);
        }
    // messages
    int i, row = 0;
    for (i = MAXMSG - 1; i >= 0; i--)
        if (iMsgUs[i] > 0) { r.TextShadow(x0 + 4, 34 + row * 10, iMsg[i], C_WHITE); row++; }
    // activation prompt
    if (iUi == UI_PLAY && (iFocusKind || iDoorFocus >= 0))
        {
        const char* verb = "Take";
        const char* name = "";
        char info[80];
        int red = 0;
        info[0] = 0;
        if (!iFocusKind)
            {
            const DoorLink& dl = Door(iDoorFocus);
            int in = iS.cell == dl.ocell;
            verb = in ? "Enter" : "Leave"; name = "Skyrim";
            // (v0.4.74) the way out named as Skyrim names it: the worldspace it opens onto - Whiterun's streets are their
            // own world ("Whiterun"), the rest is Tamriel ("Skyrim": Riverwood, the farms, the stables, the ruins alike)
            if (!in && !dl.ocell && (dl.cell == CELL_HALLDEAD || dl.cell == CELL_SHOP || dl.cell == CELL_INN || dl.cell == CELL_HIGHHALL))
                name = "Whiterun";
            if (in) name = dl.name;
            if (!in && dl.ocell)
                {
                int k;
                for (k = 0; k < NumDoors(); k++) if (Door(k).cell == dl.ocell) name = Door(k).name;
                }
            }
        else if (iFocusKind == 3) PropText(iFocus, verb, name, info, red);
        else if (iFocusKind == 5) { verb = "Harvest"; name = GameItem(FloraItem(iFloraEnt[iFocus * 6 + 4]) - 1).name; }
        else
            {
            Ent& e = iS.e[iFocus];
            switch (e.type)
                {
                case E_ELDER: verb = "Talk"; name = "Proventus"; break;
                case E_SMITH: verb = "Talk"; name = "Adrianne"; break;
                case E_VILLAGER:
                    {
                    int mer = e.role == ROLE_MERCHANT || e.role == ROLE_TRADER;
                    verb = mer ? "Trade" : "Talk";
                    name = e.role == ROLE_STABLE ? "Skulvar Sable-Hilt" : (mer ? GameMerchantName(e.pad1) : (e.role == ROLE_CARAVAN ? "Caravan Guard" : (e.role == ROLE_NAMED ? GameNamedName(e.pad1) : "Villager")));
                    break;
                    }
                case E_CHEST: verb = e.looted ? "Search" : "Open"; name = "Chest"; break;
                case E_BANDIT:
                    if (e.alive) { verb = "Talk"; name = GameEntName(e); break; }
                    verb = "Search"; name = GameEntName(e); break;
                default: verb = "Search"; name = GameEntName(e); break;
                }
            }
        // like the games: the name, then the action key, then the details (value, weight, effect)
        r.TextShadow(cx - r.TextW(name) / 2, 124, name, C_WHITE);
        char* p = r3d_puts(buf, "E) ");
        r3d_puts(p, verb);
        r.TextShadow(cx - r.TextW(buf) / 2, 134, buf, red ? C_RED : C_GOLD);
        if (info[0]) r.TextShadow(cx - r.TextW(info) / 2, 144, info, C_GREY);
        }
    else if (iUi == UI_PLAY && iSitting >= 0) r.TextShadow(cx - r.TextW("E) Stand up") / 2, 134, "E) Stand up", C_GOLD);
    else if (iUi == UI_PLAY && iGrab < 0 && (i = ShutDoorNear(iS)) >= 0)
        {
        const char* name = KPlanDoorName[i];
        r.TextShadow(cx - r.TextW(name) / 2, 124, name, C_WHITE);
        r.TextShadow(cx - r.TextW("Closed") / 2, 134, "Closed", C_GREY);
        }
    else if (iUi == UI_PLAY && iGrab >= 0) r.TextShadow(cx - r.TextW("E) Put it down") / 2, 134, "E) Put it down", C_GREY);
    DrawBars(x0, w);
    if (iVictoryUs > 0)
        {
        const char* s = "DRAGON SLAIN";
        int sc = w >= 400 ? 3 : 2;
        BigText(r, cx - r.TextW(s) * sc / 2, 46, s, C_GOLD, sc);
        }
    DrawFps(x0, w);
    }

// the frame rate and triangles (Settings: Show FPS), top right; in the menus over the world too
void Game::DrawFps(int x0, int w)
    {
    R3D& r = *iR;
    char buf[64];
    if (iBench)
        {
        // (v0.4.82) the benchmark's progress, top left
        int t;
        char* q = r3d_puts(buf, "BENCHMARK ");
        q = r3d_puti(q, iBench); q = r3d_puts(q, "/"); q = r3d_puti(q, BenchCount());
        r3d_puts(q, iBenchPhase == 3 ? " measuring - Esc: stop" : " - Esc: stop");
        r.TextShadow(x0 + 3, 3, buf, C_GOLD);
        r.TextShadow(x0 + 3, 13, BenchName(iBench - 1, &t), C_WHITE);
        }
    if (ShowFps)
        {
        int fps10 = iFps10 > 0 ? iFps10 : (iRenderUs + iBlitUs > 0 ? 10000000 / (iRenderUs + iBlitUs) : 0);
        char* p = r3d_puti(buf, fps10 / 10);
        *p++ = '.'; *p++ = (char)('0' + fps10 % 10);
        p = r3d_puts(p, "fps ");
        p = r3d_puti(p, r.iTris);
        p = r3d_puts(p, "t");
        // (v0.4.76) where a frame's time goes: drawing (d) and the copy to the screen (c), ms, over the last second
        if (iFps10 > 0) { p = r3d_puts(p, " d"); p = r3d_puti(p, iRenderAvgMs); p = r3d_puts(p, " c"); p = r3d_puti(p, iBlitAvgMs); }
        int fy = iUi == UI_INV || iUi == UI_TRADE ? OUT_H - 36 : 16;         // (in the menus: under the preview)
        r.TextShadow(x0 + w - r.TextW(buf) - 3, fy, buf, C_GREEN);
        if (iRamTotalKB)
            {
            // game memory, free / total RAM (MB, one decimal)
            p = r3d_puts(buf, "RAM ");
            p = r3d_puti(p, iRamUsedKB / 1024); *p++ = '.'; *p++ = (char)('0' + iRamUsedKB % 1024 * 10 / 1024);
            p = r3d_puts(p, "M free ");
            p = r3d_puti(p, iRamFreeKB / 1024); *p++ = '.'; *p++ = (char)('0' + iRamFreeKB % 1024 * 10 / 1024);
            p = r3d_puts(p, "/");
            p = r3d_puti(p, iRamTotalKB / 1024);
            r3d_puts(p, "M");
            r.TextShadow(x0 + w - r.TextW(buf) - 3, fy + 10, buf, C_GREEN);
            }
        }
    }

// ---------------------------------------------------------------- container / barter (inventory layout)
const char* GameContName(Game& g);
int GameMerchantGold(Game& g);
int GameBuyPrice(int v);
int GameSellPrice(int v);

void Game::DrawItemMenu(int x0, int w)
    {
    R3D& r = *iR;
    r.Shade(x0, 0, w, OUT_H);
    int trade = iUi == UI_TRADE;
    int wide = w >= 400;
    int catW = 72, top = 16;
    int listX = x0 + catW + 8;
    int listW = wide ? (w - catW - 16) * 58 / 100 : x0 + w - 4 - listX;
    int rows = wide ? 14 : 8;
    int ids[MAXINV], cnt[MAXINV], ref[MAXINV];
    int n = MenuRows(iMenuSide, iInvCat, ids, cnt, ref);
    char buf[80];
    // tabs: theirs | yours
    {
    const char* a = GameContName(*this);
    char ta[32], tb[32];
    char* p = r3d_puts(ta, a);
    if (trade) { p = r3d_puts(p, "  "); p = r3d_puti(p, GameMerchantGold(*this)); r3d_puts(p, "g"); }
    p = r3d_puts(tb, "YOU");
    if (trade) { p = r3d_puts(p, "  "); p = r3d_puti(p, iS.gold); r3d_puts(p, "g"); }
    int wa = r.TextW(ta) + 12, wb = r.TextW(tb) + 12;
    int tx = x0 + 4;
    if (iMenuSide == 0) r.Tint(tx, 2, wa, 11, C_GOLD);
    r.Text(tx + 6, 4, ta, iMenuSide == 0 ? C_WHITE : C_GREY);
    if (iMenuSide == 1) r.Tint(tx + wa + 4, 2, wb, 11, C_GOLD);
    r.Text(tx + wa + 10, 4, tb, iMenuSide == 1 ? C_WHITE : C_GREY);
    r.Rect(x0 + 4, top - 2, w - 8, 1, C_GREY);
    }
    int statW = 104;
    int pvX, pvY, pvW, pvH;
    if (wide) { pvX = listX + listW + 6; pvY = top + 2; pvW = x0 + w - 6 - pvX; pvH = OUT_H - 24 - 52 - pvY; }
    else
        {
        int used = n < rows ? n : rows;
        if (used < 1) used = 1;
        pvX = listX; pvY = top + 4 + used * 11 + 4; pvW = x0 + w - 4 - statW - pvX; pvH = OUT_H - 24 - pvY;
        }
    int i, cats[NCATS];
    int nc = MenuCats(iMenuSide, cats);
    for (i = 0; i < nc; i++)
        {
        int y = top + 4 + i * (wide ? 13 : 11);
        if (y > OUT_H - 34) break;
        int ct = cats[i];
        u16 col = ct == iInvCat ? (iInvCol == 0 ? C_GOLD : C_WHITE) : C_GREY;
        if (ct == iInvCat && iInvCol == 0) r.Tint(x0 + 2, y - 2, catW, 11, C_GOLD);
        r.Text(x0 + 6, y, GameCatName(ct), col);
        }
    r.Rect(listX - 4, top, 1, OUT_H - 26 - top, C_GREY);
    if (iInvSel < iInvScroll) iInvScroll = iInvSel;
    if (iInvSel >= iInvScroll + rows) iInvScroll = iInvSel - rows + 1;
    int maxc = listW / 6 - 1;
    for (i = 0; i < rows && i + iInvScroll < n; i++)
        {
        int k = i + iInvScroll;
        const ItemDef& d = GameItem(ids[k]);
        char tail[16];
        tail[0] = 0;
        if (cnt[k] > 1) { char* q = tail; *q++ = ' '; *q++ = '('; q = r3d_puti(q, cnt[k]); *q++ = ')'; *q = 0; }
        int tl = 0;
        while (tail[tl]) tl++;
        char* p = buf;
        int j = 0, lim = maxc - tl - (trade ? 4 : 0);
        while (d.name[j] && j < lim) *p++ = d.name[j++];
        if (d.name[j]) p[-1] = '.';
        p = r3d_puts(p, tail);
        *p = 0;
        int y = top + 4 + i * 11;
        int sel = iInvCol == 1 && k == iInvSel;
        if (sel) r.Tint(listX - 2, y - 2, listW, 11, C_GOLD);
        r.Text(listX, y, buf, sel ? C_WHITE : 0xD69A);
        if (trade)
            {
            char pb[12];
            r3d_puti(pb, iMenuSide == 0 ? GameBuyPrice(d.value) : GameSellPrice(d.value));
            r.Text(listX + listW - 6 - r.TextW(pb), y, pb, C_GOLD);
            }
        }
    if (n == 0) r.Text(listX, top + 4, "(nothing)", C_GREY);
    if (!wide) r.Rect(listX - 2, pvY - 2, w - (listX - x0) - 2, 1, 0x630C);
    if (n > 0)
        {
        int k = iInvCol == 1 ? iInvSel : 0;
        const ItemDef& d = GameItem(ids[k]);
        const u8* c = GameItemCol(ids[k]);
        u8 sl[12];
        Slot(sl, 0, c[0], c[1], c[2], 1); Slot(sl, 1, c[3], c[4], c[5], 4); Slot(sl, 2, c[6], c[7], c[8], 1);
        // the item's own Skyrim model (data/items.hit), else the game's
        const Mesh* im = ItemModel(ids[k]);
        int ik = im ? 1 : -1;
        const Mesh& m = im ? *im : iMItem[d.mesh];
        const u8* psl = sl;
        int pcx = pvX + pvW / 2, pcy = pvY + pvH / 2;
        int rx = (pcx - x0) >> r.iDouble, ry = pcy;
        int isW = d.cat == IC_WEAPON;
        int tiltP = ik >= 0 ? 25 : (isW ? 0 : 60), tiltR = ik < 0 && isW ? 200 : 0;     // (gen_kit.py stands each up on its longest side)
        int ey, er;
        ViewExtent(m, m.cx, m.cy, m.cz, tiltP, tiltR, ey, er);
        // small things (a potato, a ring) are enlarged so the preview camera need not come too close
        int psc = 256, big = er > ey ? er : ey;
        if (big < 160) { psc = 160 * 256 / (big > 4 ? big : 4); er = er * psc >> 8; ey = ey * psc >> 8; }
        int halfW = ((pvW >> r.iDouble) / 2) * 92 / 100, halfH = (pvH / 2) * 92 / 100;
        if (halfW < 4) halfW = 4;
        if (halfH < 4) halfH = 4;
        int zx = er * r.iFx / halfW, zy = ey * r.iFy / halfH;
        int z = (zx > zy ? zx : zy) + er / 3;
        int vx = (rx - r.iW / 2) * z / r.iFx;
        int vy = (r.iH / 2 - ry) * z / r.iFy;
        r.ClearZ();
        int saveL = r.iLight;
        r.iLight = 256;
        r.iPivX = m.cx; r.iPivY = m.cy; r.iPivZ = m.cz;
        r.iViewScale = psc;
        DrawItemView(m, psl, vx, vy, z, iPreviewAngle, tiltP, tiltR);
        r.iViewScale = 256;
        r.iPivX = r.iPivY = r.iPivZ = 0;
        r.iLight = saveL;
        int sx = wide ? pvX : pvX + pvW + 4;
        int sw = wide ? pvW : x0 + w - 4 - sx;
        int ty = wide ? pvY + pvH + 4 : pvY + 2;
        DrawWrapped(sx, ty, sw, d.name, C_WHITE, 2);
        ty += r.TextW(d.name) > sw ? 22 : 12;
        char* p = buf;
        if (d.cat == IC_WEAPON) { p = r3d_puts(p, "DAMAGE "); r3d_puti(p, d.power * 2); }
        else if (d.equip && d.power) { p = r3d_puts(p, "ARMOR "); r3d_puti(p, d.power); }
        else if (d.effect == PW_HEAL) { p = r3d_puts(p, "Restore "); p = r3d_puti(p, d.power); r3d_puts(p, " Health"); }
        else if (d.effect == PW_MAGICKA) { p = r3d_puts(p, "Restore "); p = r3d_puti(p, d.power); r3d_puts(p, " Magicka"); }
        else if (d.effect == PW_STAMINA) { p = r3d_puts(p, "Restore "); p = r3d_puti(p, d.power); r3d_puts(p, " Stamina"); }
        else *p = 0;
        r.Text(sx, ty, buf, C_GOLD);
        p = r3d_puts(buf, "WEIGHT ");
        p = r3d_puti(p, d.weight10 / 10);
        if (d.weight10 % 10) { *p++ = '.'; *p++ = (char)('0' + d.weight10 % 10); }
        *p = 0;
        r.Text(sx, ty + 11, buf, C_GREY);
        p = r3d_puts(buf, trade ? (iMenuSide == 0 ? "PRICE " : "SELLS FOR ") : "VALUE ");
        r3d_puti(p, trade ? (iMenuSide == 0 ? GameBuyPrice(d.value) : GameSellPrice(d.value)) : d.value);
        r.Text(sx, ty + 22, buf, trade ? C_GOLD : C_GREY);
        }
    r.Rect(x0 + 4, OUT_H - 22, w - 8, 1, C_GREY);
    char* p = r3d_puts(buf, "WEIGHT ");
    p = r3d_puti(p, CarryWeight10() / 10);
    p = r3d_puts(p, "/300  GOLD ");
    r3d_puti(p, iS.gold);
    r.Text(x0 + 6, OUT_H - 19, buf, C_WHITE);
    const char* help = trade ? (iMenuSide == 0 ? "E buy one  Q your items  Tab done" : "E sell one  Q their goods  Tab done")
                             : (iMenuSide == 0 ? "E take one  R take all  Q store  Tab close" : "E store one  Q back  Tab close");
    r.Text(x0 + 6, OUT_H - 9, help, C_GREY);
    }

// ---------------------------------------------------------------- menu
int GameViewNear(int v);
int GameViewFar(int v);
int GameViewTex(int v);
int GameViewFull(int v);
int GameViewMiddle(int v);
int GameFullDist(int v, int aBase);
static const char* const KMenu[22] = { "Resume", "Save Game", "Load Game", "Controls", "Display", "Brightness", "Draw distance", "Far land", "Textures", "Plants", "Full models", "Middle models", "Rows", "Near land", "Small things", "Auto speed", "Show FPS", "Travel (testing)", "Benchmark (send to PC)", "New Game", "Update from server", "Quit" };
static const char* const KGammaNm[4] = { "Normal", "Bright", "Brighter", "Brightest" };
static const char* const KModeNm[4] = { "320x200", "400x200", "640x200", "640x200 (2x)" };

void Game::DrawMenu(int x0, int w)
    {
    R3D& r = *iR;
    int cx = x0 + w / 2;
    r.Shade(x0, 0, w, OUT_H);
    BigText(r, cx - r.TextW("HRIMFELL") , 14, "HRIMFELL", C_GOLD, 2);
    char buf[64];
    char* p = r3d_puts(buf, "Quest: ");
    r3d_puts(p, QuestText());
    r.Text(cx - r.TextW(buf) / 2, 36, buf, C_GREY);
    int i;
    // (v0.4.58) 22 entries (v0.4.82), 16 shown: the list scrolls with the choice
    int top = iMenuSel - 13; if (top < 0) top = 0; if (top > 22 - 16) top = 22 - 16;
    for (i = top; i < top + 16; i++)
        {
        p = r3d_puts(buf, KMenu[i]);
        if (i == 4) { p = r3d_puts(p, ": "); r3d_puts(p, KModeNm[iS.outMode & 3]); }
        if (i == 5) { p = r3d_puts(p, ": "); r3d_puts(p, KGammaNm[iS.gamma & 3]); }
        if (i == 6) { p = r3d_puts(p, ": "); p = r3d_puti(p, GameViewNear(iS.view)); r3d_puts(p, " m"); }
        if (i == 7)
            {
            int f = GameViewFar(iS.view);
            p = r3d_puts(p, ": ");
            if (!f) r3d_puts(p, "Off");
            else if (f < 1000) { p = r3d_puti(p, f); r3d_puts(p, " m"); }
            else { p = r3d_puti(p, f / 1000); *p++ = '.'; p = r3d_puti(p, f / 100 % 10); r3d_puts(p, " km"); }
            }
        if (i == 8)
            {
            int t = GameViewTex(iS.view);
            if (!t) r3d_puts(p, ": Off (faster)");
            else { p = r3d_puts(p, ": "); p = r3d_puti(p, t); r3d_puts(p, " m"); }
            }
        if (i == 9) { r3d_puts(p, (iS.view >> 9) & 1 ? ": Off (faster)" : ": On"); }
        if (i == 10)
            {
            int f = GameViewFull(iS.view);
            if (!f) r3d_puts(p, ": Off (faster)");
            else { p = r3d_puts(p, ": to "); p = r3d_puti(p, f); r3d_puts(p, " m"); }
            }
        if (i == 11) { p = r3d_puts(p, ": to "); p = r3d_puti(p, GameViewMiddle(iS.view)); r3d_puts(p, " m"); }
        if (i == 12) { r3d_puts(p, (iS.view >> 16) & 1 ? ": Half (faster)" : ": Full"); }
        if (i == 13) { r3d_puts(p, (iS.view >> 17) & 1 ? ": Flat (faster)" : ": Smooth"); }
        if (i == 14) { r3d_puts(p, (iS.view >> 18) & 1 ? ": Only near (faster)" : ": All"); }
        if (i == 15)
            {
            int a = (iS.view >> 19) & 3;
            if (!a) r3d_puts(p, ": Off");
            else { p = r3d_puts(p, a == 1 ? ": 8 fps" : ": 10 fps"); if (iAutoLevel) { p = r3d_puts(p, " (step "); p = r3d_puti(p, iAutoLevel); r3d_puts(p, ")"); } }
            }
        if (i == 16) { r3d_puts(p, ShowFps ? ": On" : ": Off"); }
        int y = 44 + (i - top) * 9;
        int tw = r.TextW(buf);
        if (i == iMenuSel) { r.Tint(cx - tw / 2 - 8, y - 2, tw + 16, 10, C_GOLD); }
        r.Text(cx - tw / 2, y, buf, i == iMenuSel ? C_WHITE : C_GREY);
        }
    r.Text(cx - r.TextW("Joystick: choose, left / right: change   Press: select   Tab: back") / 2, OUT_H - 9, "Joystick: choose, left / right: change   Press: select   Tab: back", C_GREY);
    }

// the controls page (from the menu): two columns, key and what it does
static const char* const KHelpKeys[] =
    {
    "MOVING", 0,
    "W A S D", "walk",
    "Joystick", "look round",
    "Shift", "sprint",
    "Space", "jump",
    "F", "1st / 3rd person",
    "FIGHTING", 0,
    "Press / Enter", "attack (draws the weapon)",
    "R", "draw / sheathe weapon",
    "Q", "cast the spell",
    "C", "crouch (sneak)",
    "Z or Y", "shout",
    "L", "light / put out a torch",
    "USING THINGS", 0,
    "E", "use, talk, open, take, sit",
    "Hold E", "pick up and carry",
    "E again", "put it down",
    "Attack, carrying", "throw it",
    "T", "wait (beds: sleep)",
    "MENUS", 0,
    "I", "items (E use, R drop, F fav)",
    "M", "map      J  quest",
    "In the map", "L local/world, Z X zoom, T go",
    "X", "show model names",
    "Q", "switch sides in trade",
    "R", "take all (containers)",
    "Tab", "close any menu",
    "Esc", "menu      N  screen size",
    0, 0
    };

void Game::DrawHelp(int x0, int w)
    {
    R3D& r = *iR;
    r.Shade(x0, 0, w, OUT_H);
    r.Shade(x0, 0, w, OUT_H);
    BigText(r, x0 + w / 2 - r.TextW("CONTROLS"), 4, "CONTROLS", C_GOLD, 2);
    // two columns of rows (one scrolling column on the narrow screen): headings gold, keys white, actions grey
    int wide = w >= 400;
    int col = 0, y = 26, i, row = 0;
    int cw = w / 2, kx = 8, ax = 8 + (wide ? 112 : 100);
    for (i = 0; KHelpKeys[i * 2]; i++)
        {
        const char* k = KHelpKeys[i * 2]; const char* a = KHelpKeys[i * 2 + 1];
        if (wide && !a && i > 0 && y > OUT_H - 80 && col == 0) { col = 1; y = 26; }
        if (!wide && row++ < iListScroll) continue;
        if (y > OUT_H - 22) break;
        int bx = x0 + col * cw;
        if (!a) { if (y > 26) y += 3; r.Text(bx + kx, y, k, C_GOLD); y += 11; continue; }
        r.Text(bx + kx + 4, y, k, C_WHITE);
        r.Text(bx + ax, y, a, C_GREY);
        y += 10;
        }
    if (!wide) r.Text(x0 + w - 4 - r.TextW("Joy: scroll"), 8, "Joy: scroll", C_GREY);
    r.Text(x0 + w / 2 - r.TextW("Tab / Esc / E: back") / 2, OUT_H - 11, "Tab / Esc / E: back", C_GOLD);
    }

// ---------------------------------------------------------------- inventory
void Game::DrawInventory(int x0, int w)
    {
    R3D& r = *iR;
    r.Shade(x0, 0, w, OUT_H);
    int wide = w >= 400;
    int catW = 72;
    int listX = x0 + catW + 8;
    int listW = wide ? (w - catW - 16) * 58 / 100 : x0 + w - 4 - listX;
    int rows = wide ? 15 : 9;
    int list[MAXINV];
    int n = InvList(iInvCat, list, MAXINV);
    // preview box: all the free space (wide: right column above the stats; narrow: below the used rows, left of the stats)
    int statW = 104;
    int pvX, pvY, pvW, pvH;
    if (wide) { pvX = listX + listW + 6; pvY = 4; pvW = x0 + w - 6 - pvX; pvH = OUT_H - 24 - 52 - pvY; }
    else
        {
        int used = n < rows ? n : rows;
        if (used < 1) used = 1;
        pvX = listX; pvY = 6 + used * 11 + 4; pvW = x0 + w - 4 - statW - pvX; pvH = OUT_H - 24 - pvY;
        }
    int i, cats[NCATS];
    int nc = InvCats(cats);
    for (i = 0; i < nc; i++)
        {
        int y = 6 + i * (wide ? 13 : 12);
        int ct = cats[i];
        u16 col = ct == iInvCat ? (iInvCol == 0 ? C_GOLD : C_WHITE) : C_GREY;
        if (ct == iInvCat && iInvCol == 0) r.Tint(x0 + 2, y - 2, catW, 11, C_GOLD);
        r.Text(x0 + 6, y, GameCatName(ct), col);
        }
    r.Rect(listX - 4, 4, 1, OUT_H - 26, C_GREY);
    if (iInvSel < iInvScroll) iInvScroll = iInvSel;
    if (iInvSel >= iInvScroll + rows) iInvScroll = iInvSel - rows + 1;
    char buf[80];
    int maxc = listW / 6 - 1;
    for (i = 0; i < rows && i + iInvScroll < n; i++)
        {
        int idx = list[i + iInvScroll];
        InvItem& it = iS.inv[idx];
        const ItemDef& d = GameItem(it.id);
        char cnt[12];
        cnt[0] = 0;
        if (it.count > 1) { char* q = cnt; *q++ = ' '; *q++ = '('; q = r3d_puti(q, it.count); *q++ = ')'; *q = 0; }
        int cl = 0;
        while (cnt[cl]) cl++;
        char* p = buf;
        *p++ = it.equipped ? '>' : ' ';
        int k = 0;
        int lim = maxc - 2 - cl;
        while (d.name[k] && k < lim) *p++ = d.name[k++];
        if (d.name[k]) { p[-1] = '.'; }
        p = r3d_puts(p, cnt);
        if (it.fav) *p++ = '+';
        *p = 0;
        int y = 6 + i * 11;
        int sel = iInvCol == 1 && i + iInvScroll == iInvSel;
        if (sel) r.Tint(listX - 2, y - 2, listW, 11, C_GOLD);
        r.Text(listX, y, buf, it.equipped ? C_GOLD : (sel ? C_WHITE : 0xD69A));
        }
    if (n == 0) r.Text(listX, 6, "(nothing)", C_GREY);
    if (!wide) r.Rect(listX - 2, pvY - 2, w - (listX - x0) - 2, 1, 0x630C);
    int show = -1;
    if (n > 0) show = list[iInvCol == 1 ? iInvSel : 0];
    if (show >= 0)
        {
        InvItem& it = iS.inv[show];
        const ItemDef& d = GameItem(it.id);
        const u8* c = GameItemCol(it.id);
        u8 sl[12];
        Slot(sl, 0, c[0], c[1], c[2], 1); Slot(sl, 1, c[3], c[4], c[5], 4); Slot(sl, 2, c[6], c[7], c[8], 1);
        // the item's own Skyrim model (data/items.hit), else the game's
        const Mesh* im = ItemModel(it.id);
        int ik = im ? 1 : -1;
        const Mesh& m = im ? *im : iMItem[d.mesh];
        const u8* psl = sl;
        // rotating 3D model, placed so it projects into the preview box
        int pcx = pvX + pvW / 2, pcy = pvY + pvH / 2;
        int rx = (pcx - x0) >> r.iDouble, ry = pcy;
        // fit the spinning model into ~92% of the box: vertical extent and radius around the spin axis
        int isW = d.cat == IC_WEAPON;
        int tiltP = ik >= 0 ? 25 : (isW ? 0 : 60), tiltR = ik < 0 && isW ? 200 : 0;     // (gen_kit.py stands each up on its longest side)
        int ey, er;
        ViewExtent(m, m.cx, m.cy, m.cz, tiltP, tiltR, ey, er);
        // small things (a potato, a ring) are enlarged so the preview camera need not come too close
        int psc = 256, big = er > ey ? er : ey;
        if (big < 160) { psc = 160 * 256 / (big > 4 ? big : 4); er = er * psc >> 8; ey = ey * psc >> 8; }
        int halfW = ((pvW >> r.iDouble) / 2) * 92 / 100, halfH = (pvH / 2) * 92 / 100;
        if (halfW < 4) halfW = 4;
        if (halfH < 4) halfH = 4;
        int zx = er * r.iFx / halfW, zy = ey * r.iFy / halfH;
        int z = (zx > zy ? zx : zy) + er / 3;  // small perspective margin for the near side
        int vx = (rx - r.iW / 2) * z / r.iFx;
        int vy = (r.iH / 2 - ry) * z / r.iFy;
        r.ClearZ();
        int saveL = r.iLight;
        r.iLight = 256;
        r.iPivX = m.cx; r.iPivY = m.cy; r.iPivZ = m.cz;
        r.iViewScale = psc;
        DrawItemView(m, psl, vx, vy, z, iPreviewAngle, tiltP, tiltR);
        r.iViewScale = 256;
        r.iPivX = r.iPivY = r.iPivZ = 0;
        r.iLight = saveL;
        // stats
        int sx = wide ? pvX : pvX + pvW + 4;
        int sw = wide ? pvW : x0 + w - 4 - sx;
        int ty = wide ? pvY + pvH + 4 : pvY + 2;
        DrawWrapped(sx, ty, sw, d.name, C_WHITE, 2);
        ty += r.TextW(d.name) > sw ? 22 : 12;
        char* p = buf;
        if (d.cat == IC_WEAPON) { p = r3d_puts(p, "DAMAGE "); r3d_puti(p, d.power * 2); }
        else if (d.equip && d.power) { p = r3d_puts(p, "ARMOR "); r3d_puti(p, d.power); }
        else if (d.effect == PW_HEAL) { p = r3d_puts(p, "Restore "); p = r3d_puti(p, d.power); r3d_puts(p, " Health"); }
        else if (d.effect == PW_MAGICKA) { p = r3d_puts(p, "Restore "); p = r3d_puti(p, d.power); r3d_puts(p, " Magicka"); }
        else if (d.effect == PW_STAMINA) { p = r3d_puts(p, "Restore "); p = r3d_puti(p, d.power); r3d_puts(p, " Stamina"); }
        else if (d.cat == IC_BOOK) r3d_puts(p, "E to read");
        else *p = 0;
        r.Text(sx, ty, buf, C_GOLD);
        p = r3d_puts(buf, "WEIGHT ");
        p = r3d_puti(p, d.weight10 / 10);
        if (d.weight10 % 10) { *p++ = '.'; *p++ = (char)('0' + d.weight10 % 10); }
        *p = 0;
        r.Text(sx, ty + 11, buf, C_GREY);
        p = r3d_puts(buf, "VALUE ");
        r3d_puti(p, d.value);
        r.Text(sx, ty + 22, buf, C_GREY);
        if (it.equipped) r.Text(sx, ty + 33, "Equipped", C_GOLD);
        }
    r.Rect(x0 + 4, OUT_H - 22, w - 8, 1, C_GREY);
    char* p = r3d_puts(buf, "ARMOR ");
    p = r3d_puti(p, ArmorRating());
    p = r3d_puts(p, "  WEIGHT ");
    p = r3d_puti(p, CarryWeight10() / 10);
    p = r3d_puts(p, "/300  GOLD ");
    r3d_puti(p, iS.gold);
    r.Text(x0 + 6, OUT_H - 19, buf, C_WHITE);
    r.Text(x0 + 6, OUT_H - 9, "E use/equip  R drop  F fav  I close", C_GREY);
    }

// ---------------------------------------------------------------- map
static void MapPlot(R3D& r, int x, int y, int x0, int y0, int x1, int y1, u16 c)
    {
    if (x >= x0 && x < x1 && y >= y0 && y < y1) r.iOut[y * r.iOutStride + x] = c;
    }
// the map markers' icons, 7 x 7 (Skyrim's: a city's walls and towers, a settlement's house, a Nordic ruin's arch)
static const u8 KLocIcon[3 * 7] =
    {
    0x55, 0x7F, 0x3E, 0x36, 0x3E, 0x3E, 0x7F,          // city
    0x08, 0x1C, 0x3E, 0x7F, 0x22, 0x2A, 0x2A,          // settlement
    0x3E, 0x7F, 0x63, 0x63, 0x63, 0x63, 0x63,          // Nordic ruin
    };
// an icon centred on (x, y): its shape in aCol, ringed in aRim (none when they are the same)
static void LocIcon(R3D& r, int x, int y, int aKind, u16 aCol, u16 aRim)
    {
    const u8* g = KLocIcon + aKind * 7;
    int a, b;
    if (aRim != aCol)
        for (b = -1; b <= 7; b++)
            for (a = -1; a <= 7; a++)
                {
                int on = 0, i, j;
                for (j = b - 1; j <= b + 1 && !on; j++)
                    for (i = a - 1; i <= a + 1; i++)
                        if (i >= 0 && i < 7 && j >= 0 && j < 7 && (g[j] & (64 >> i))) { on = 1; break; }
                if (on && !(a >= 0 && a < 7 && b >= 0 && b < 7 && (g[b] & (64 >> a)))) r.Rect(x - 3 + a, y - 3 + b, 1, 1, aRim);
                }
    for (b = 0; b < 7; b++)
        for (a = 0; a < 7; a++)
            if (g[b] & (64 >> a)) r.Rect(x - 3 + a, y - 3 + b, 1, 1, aCol);
    }
// two RGB565 colours mixed, f of 32 the second
static u16 Mix565(u16 c0, u16 c1, int f)
    {
    u32 a = ((u32)c0 | ((u32)c0 << 16)) & 0x07E0F81Fu, b = ((u32)c1 | ((u32)c1 << 16)) & 0x07E0F81Fu;
    u32 m = (a + (((b - a) * (u32)f) >> 5)) & 0x07E0F81Fu;
    return (u16)(m | (m >> 16));
    }

// The world map: all of the screen, north up, the land lit along its slopes; Z / X closer and further, the
// movement keys pan it; the map markers (white: found, black: not yet), the player's arrow, the name of the
// marker nearest the cross in the middle.
// the close-up's rows aRow0 .. aRow0 + aRows - 1 (from the north) in iDet: read when not there yet (DET_ROWS of them
// round them, in one read); 1 if they are
int Game::DetailRows(int aRow0, int aRows)
    {
    if (!iDataRead) return 0;
    if (!iDetOk)
        {
        u8 h[12 + 768];
        if (iDataRead(iDataCtx, "hrimfell_map.bin", 0, h, sizeof(h)) != (int)sizeof(h) || h[0] != 'H' || h[1] != 'M' || h[2] != 'A' || h[3] != 'P') { iDetOk = -1; return 0; }
        iDetW = h[4] | (h[5] << 8) | (h[6] << 16); iDetH = h[8] | (h[9] << 8) | (h[10] << 16);
        if (iDetW > DET_MAXW) { iDetOk = -1; return 0; }
        int k;
        for (k = 0; k < 256; k++) iDetPal[k] = R3D::Rgb(h[12 + k * 3], h[13 + k * 3], h[14 + k * 3]);
        iDetOk = 1;
        }
    if (iDetOk < 0) return 0;
    if (aRows > DET_ROWS) aRows = DET_ROWS;
    if (iDetRow0 >= 0 && aRow0 >= iDetRow0 && aRow0 + aRows <= iDetRow0 + DET_ROWS) return 1;
    int r0 = aRow0 - (DET_ROWS - aRows) / 2;
    if (r0 > iDetH - DET_ROWS) r0 = iDetH - DET_ROWS;
    if (r0 < 0) r0 = 0;
    int want = DET_ROWS * iDetW, got = 0, n;
    while (got < want && (n = iDataRead(iDataCtx, "hrimfell_map.bin", 12 + 768 + r0 * iDetW + got, iDet + got, want - got)) > 0) got += n;
    if (got < iDetW) { iDetRow0 = -1; return 0; }
    iDetRow0 = r0;
    return 1;
    }

// a place on the world map: world (x, z) -> screen
void Game::MapXY(const MapView& v, int wx, int wz, int& sx, int& sy)
    {
    if (v.detail) { sx = v.x0 + v.w / 2 + (wx - iMapCX) / (CELL / v.ppv); sy = OUT_H / 2 - (wz - iMapCZ) / (CELL / v.ppv); return; }
    sx = v.x0 + (int)(((long long)(wx >> 6) * MAPW * 256 / v.ww - v.u0) / v.step);
    sy = (int)(((long long)(v.wd - (wz >> 6)) * v.mh * 256 / v.wd - v.v0) / v.step);
    }

// the places' icons, the quest targets, the player's arrow (iMapBest: the place nearest the cross)
void Game::DrawMapMarks(R3D& r, const MapView& v)
    {
    int x0 = v.x0, w = v.w;
    int i, best = -1, bestD = 14 * 14;
    int cxs = x0 + w / 2, cys = OUT_H / 2;
    for (i = 0; i < GameLocN(); i++)
        {
        LocInfo l; GameLoc(*this, i, l);
        int sx, sy;
        MapXY(v, l.x, l.z, sx, sy);
        if (sx < x0 + 4 || sx > x0 + w - 5 || sy < 4 || sy > OUT_H - 5) continue;
        int found = (iS.spare[1] >> i) & 1;
        LocIcon(r, sx, sy, l.kind, found ? C_WHITE : R3D::Rgb(10, 10, 10), found ? R3D::Rgb(40, 36, 30) : R3D::Rgb(150, 144, 130));
        int d = (sx - cxs) * (sx - cxs) + (sy - cys) * (sy - cys);
        if (d < bestD) { bestD = d; best = i; }
        }
    // quest targets
    {
    int qx[4], qz[4], qn;
    QuestTargets(qx, qz, qn);
    for (i = 0; i < qn; i++)
        {
        if (iS.cell && qx[i] == iS.px && qz[i] == iS.pz) continue;
        int sx, sy, k;
        MapXY(v, qx[i], qz[i], sx, sy);
        if (sx < x0 + 4 || sx > x0 + w - 5 || sy < 8 || sy > OUT_H - 5) continue;
        for (k = 0; k < 4; k++) r.Rect(sx - 3 + k, sy - 7 + k, 7 - 2 * k, 1, C_GOLD);
        }
    }
    // the player's arrow (indoors: where they went in)
    {
    int px, py, k;
    MapXY(v, iS.cell ? iS.extX : iS.px, iS.cell ? iS.extZ : iS.pz, px, py);
    int yaw = iS.cell ? iS.extYaw : iS.yaw;
    int fx = r3d_sin(yaw), fz = r3d_cos(yaw);
    for (k = -3; k <= 6; k++)
        {
        int ax = px + (fx * k >> 14), ay = py - (fz * k >> 14), wdt = k < 0 ? 0 : (6 - k) / 2, s;
        for (s = -wdt; s <= wdt; s++) MapPlot(r, ax + (fz * s >> 14), ay + (fx * s >> 14), x0, 0, x0 + w, OUT_H, C_RED);
        }
    }
    iMapBest = best;
    }

void Game::DrawMapCross(R3D& r, int x0, int w, int)
    {
    // the cross, the name under it (found places only: the others are not known by name yet), the keys
    int best = iMapBest, cxs = x0 + w / 2, cys = OUT_H / 2;
    r.Rect(cxs - 3, cys, 7, 1, C_GOLD); r.Rect(cxs, cys - 3, 1, 7, C_GOLD);
    if (best >= 0 && ((iS.spare[1] >> best) & 1)) { LocInfo l; GameLoc(*this, best, l); r.TextShadow(cxs - r.TextW(l.name) / 2, cys + 6, l.name, C_WHITE); }
    r.Shade(x0, OUT_H - 12, w, 12);
    r.Text(x0 + 4, OUT_H - 10, iS.cell ? "Arrows move  Z/X zoom  T go there  M close" : "Arrows move Z/X zoom L local T go M close", C_GREY);
    r.TextShadow(x0 + 4, 3, "SKYRIM", C_WHITE);
    }

void Game::DrawMap(int x0, int w)
    {
    if (iMapLocal && !iS.cell) { DrawLocalMap(x0, w); return; }
    R3D& r = *iR;
    int mh = MapImgH();
    if (iMapWZ >= 3 && DetailRows(0, 0) && iDetOk > 0)
        {
        // ---- the close-up: every land vertex (4 m) a pixel, or two at the closest
        int ppv = iMapWZ == 3 ? 1 : 2;
        int gw = iDetW - 1, gh = iDetH - 1;
        int hw = w / 2 / ppv, hh = OUT_H / 2 / ppv;
        iMapCX = Clamp(iMapCX, hw * CELL, (gw - hw) * CELL); iMapCZ = Clamp(iMapCZ, hh * CELL, (gh - hh) * CELL);
        int gcx = iMapCX / CELL, gcz = iMapCZ / CELL;
        int row0 = gh - (gcz + hh);                         // the top screen row's row in the file (rows from the north)
        int rows = OUT_H / ppv + 2;
        u16 sea = R3D::Rgb(24, 22, 18);
        int ok = DetailRows(row0 < 0 ? 0 : row0, rows);
        int x, y;
        for (y = 0; y < OUT_H; y++)
            {
            u16* row = r.iOut + y * r.iOutStride + x0;
            int fr = row0 + y / ppv;
            if (!ok || fr < iDetRow0 || fr >= iDetRow0 + DET_ROWS || fr >= iDetH) { for (x = 0; x < w; x++) row[x] = sea; continue; }
            const u8* src = iDet + (fr - iDetRow0) * iDetW + (gcx - hw);
            if (ppv == 1) for (x = 0; x < w; x++) row[x] = iDetPal[src[x]];
            else for (x = 0; x < w; x++) row[x] = iDetPal[src[x >> 1]];
            }
        // the places, the quest, the player: world -> screen
        MapView v; v.x0 = x0; v.w = w; v.detail = 1; v.ppv = ppv; v.u0 = v.v0 = 0; v.step = 1; v.ww = v.wd = 1; v.mh = mh;
        DrawMapMarks(r, v);
        DrawMapCross(r, x0, w, -1);
        return;
        }
    // zoom: screen pixels a map pixel, in 16ths (1: the whole land on the screen's height, 2: across its width ...)
    int fitH = OUT_H * 16 / mh, fitW = w * 16 / MAPW;
    int z16 = iMapWZ == 0 ? fitH : iMapWZ == 1 ? fitW * 2 : fitW * 4;     // (closer than the width: it pans sideways too)
    // the centre, in map pixels * 256, kept so the view stays on the land
    int ww = r.WorldW() >> 6, wd = r.WorldD() >> 6;
    int cu = (int)((long long)(iMapCX >> 6) * MAPW * 256 / ww), cv = (int)((long long)(wd - (iMapCZ >> 6)) * mh * 256 / wd);
    int halfU = (w / 2) * 16 * 256 / z16, halfV = (OUT_H / 2) * 16 * 256 / z16;
    if (2 * halfU >= MAPW * 256) cu = MAPW * 128; else cu = Clamp(cu, halfU, MAPW * 256 - halfU);
    if (2 * halfV >= mh * 256) cv = mh * 128; else cv = Clamp(cv, halfV, mh * 256 - halfV);
    iMapCX = (int)((long long)cu * ww / (MAPW * 256)) << 6; iMapCZ = (wd - (int)((long long)cv * wd / (mh * 256))) << 6;
    int step = 16 * 256 / z16;                                 // map units (1/256 pixel) a screen pixel
    int u0 = cu - (w / 2) * step, v0 = cv - (OUT_H / 2) * step;
    u16 sea = R3D::Rgb(24, 22, 18);
    int x, y;
    for (y = 0; y < OUT_H; y++)
        {
        u16* row = r.iOut + y * r.iOutStride + x0;
        int v = v0 + y * step;
        if (v < 0 || v >= (mh - 1) * 256) { for (x = 0; x < w; x++) row[x] = sea; continue; }
        int vy = v >> 8, fy = (v >> 3) & 31;
        const u16* s0 = iMapImg + vy * MAPW, *s1 = s0 + MAPW;
        int u = u0;
        for (x = 0; x < w; x++, u += step)
            {
            if (u < 0 || u >= (MAPW - 1) * 256) { row[x] = sea; continue; }
            int ux = u >> 8, fx = (u >> 3) & 31;
            row[x] = Mix565(Mix565(s0[ux], s0[ux + 1], fx), Mix565(s1[ux], s1[ux + 1], fx), fy);
            }
        }
    MapView v; v.x0 = x0; v.w = w; v.detail = 0; v.ppv = 1; v.u0 = u0; v.v0 = v0; v.step = step; v.ww = ww; v.wd = wd; v.mh = mh;
    DrawMapMarks(r, v);
    DrawMapCross(r, x0, w, -1);
    }

// ---------------------------------------------------------------- the local map
// As in the games: a sketch of the place from above, north up, one pixel a metre, dark sepia ground lit along its
// slopes, light outlines of the buildings, walls and stairs, a door marker on every door, the player's arrow; the
// view pans with the movement keys and the name of the door nearest the cross in the middle is shown.

void Game::DrawLocalMap(int x0, int w)
    {
    R3D& r = *iR;
    r.Rect(x0, 0, w, OUT_H, R3D::Rgb(20, 18, 15));
    int mx0 = x0 + 6, my0 = 16, mw = w - 12, mh = OUT_H - 30;
    int mx1 = mx0 + mw, my1 = my0 + mh;
    int cx = iMapCX, cz = iMapCZ, hx0 = mw / 2, hy0 = mh / 2;
    int sc = iMapZoom ? 1 : 2;                                 // pixels a metre
    // the ground, 2 by 2 pixels: dark sepia, lit along its slopes
    int x, y;
    int href = r.GroundAt(cx, cz);
    // (the ground sampled on a grid fixed to the world, 2 screen pixels apart, so it slides smoothly as the view
    //  pans; each height looked up once, its neighbours to the east and north from the same rows)
    int stepw = 2 * METER / sc;                                // world units between samples
    int gx0 = cx - hx0 * METER / sc, gz0 = cz + hy0 * METER / sc;
    int ax = (gx0 / stepw) * stepw, az = (gz0 / stepw) * stepw;
    int ox = (ax - gx0) * sc / METER, oy = (gz0 - az) * sc / METER;   // the first sample's screen offset (0 or -1..)
    int nsx = mw / 2 + 3, nsy = mh / 2 + 3, nb = 2 * METER / stepw;   // samples; how many samples are 2 m
    int ra[200], rb[200];
    int* rows[2] = { ra, rb };
    if (nsx + nb > 200) nsx = 200 - nb;
    int j, k;
    for (j = 0; j < nsy; j++)
        {
        // this row and the one 2 m north of it
        int wz = az - j * stepw;
        for (k = 0; k < nsx + nb; k++) { rows[0][k] = r.GroundAt(ax + k * stepw, wz); rows[1][k] = r.GroundAt(ax + k * stepw, wz + 2 * METER); }
        y = oy + j * 2;
        if (y + 2 <= 0 || y >= mh) continue;
        for (k = 0; k < nsx; k++)
            {
            x = ox + k * 2;
            if (x + 2 <= 0 || x >= mw) continue;
            int h = rows[0][k], hr = rows[0][k + nb], hu = rows[1][k];
            int sl = ((h - hr) + (hu - h)) * 40 / METER;           // light from the north-west
            int dh = (h - href) / METER;
            int v = 46 + (dh > 20 ? 20 + (dh - 20) / 4 : (dh < -20 ? -20 + (dh + 20) / 4 : dh)) + sl;
            if (v < 14) v = 14;
            if (v > 150) v = 150;
            int rx = x < 0 ? 0 : x, ry = y < 0 ? 0 : y, rw = (x + 2 > mw ? mw : x + 2) - rx, rh = (y + 2 > mh ? mh : y + 2) - ry;
            r.Rect(mx0 + rx, my0 + ry, rw, rh, R3D::Rgb(v + 8, v + 4, v - 6 < 0 ? 0 : v - 6));
            }
        }
    // the places, drawn by kind: 1 building, 2 wall, 3 tower, 4 water, 5 stairs, 6 rock, 7 tree, 8 well, 9 small tree
    static const u8 KFill[10 * 3] = { 0,0,0,  64,58,46,  120,116,106,  132,128,118,  44,58,66,  104,100,90,  86,82,74,  60,62,46,  44,58,66,  58,60,44 };
    static const u8 KEdge[10 * 3] = { 0,0,0,  206,196,166,  214,210,200,  220,216,206,  92,116,128,  190,184,170,  150,144,132,  122,120,86,  140,160,170,  100,98,70 };
    int pass, i;
    for (pass = 0; pass < 2; pass++)            // water first, everything else over it
        for (i = 0; i < r.iObjCount; i++)
            {
            const Obj& o = r.iObj[i];
            int dx = ((o.x - cx) >> 8) * sc, dz = ((o.z - cz) >> 8) * sc;
            if (dx < -hx0 - 60 || dx > hx0 + 60 || dz < -hy0 - 60 || dz > hy0 + 60) continue;
            int hxs, hzs, ozs;
            int k = MapShape(o.type, hxs, hzs, ozs);
            if (!k || (k == 4) != (pass == 0)) continue;
            hxs = hxs * o.scale >> 8; hzs = hzs * o.scale >> 8; ozs = ozs * o.scale >> 8;
            u16 fill = R3D::Rgb(KFill[k * 3], KFill[k * 3 + 1], KFill[k * 3 + 2]), edge = R3D::Rgb(KEdge[k * 3], KEdge[k * 3 + 1], KEdge[k * 3 + 2]);
            int px = mx0 + hx0 + dx, py = my0 + hy0 - dz;
            if (k >= 7)
                {
                // round: trees, the well
                int rr = (hxs >> 8) * sc, a, b;
                if (rr < 1) rr = 1;
                for (b = -rr; b <= rr; b++)
                    for (a = -rr; a <= rr; a++)
                        {
                        int d2 = a * a + b * b;
                        if (d2 <= rr * rr) MapPlot(r, px + a, py + b, mx0, my0, mx1, my1, d2 > (rr - 1) * (rr - 1) ? edge : fill);
                        }
                continue;
                }
            // a rotated box: every pixel of its bounds tested in the object's own frame
            int cs = r3d_cos(o.yaw), sn = r3d_sin(o.yaw);
            int ext = (((hxs > hzs + ozs ? hxs : hzs + ozs) >> 8) + 2) * sc, a, b;
            for (b = -ext; b <= ext; b++)
                for (a = -ext; a <= ext; a++)
                    {
                    int wx = a * METER / sc, wz = -b * METER / sc;
                    int lx = (wx * cs - wz * sn) >> 14, lz = ((wx * sn + wz * cs) >> 14) - ozs;
                    int ax = lx < 0 ? -lx : lx, az = lz < 0 ? -lz : lz;
                    if (ax > hxs || az > hzs) continue;
                    int rim = ax > hxs - METER / sc || az > hzs - METER / sc;
                    u16 c = rim ? edge : fill;
                    if (k == 5 && !rim && ((lz + 8 * METER) / (METER / 2)) % 2) c = edge;      // treads
                    MapPlot(r, px + a, py + b, mx0, my0, mx1, my1, c);
                    }
            }
    // the buildings made of Skyrim's pieces (Riverwood): their walls, a light line each (the tall ones: not fences)
    {
    int nw = GameKitOutWall(*this, -1, 0, 0, 0, 0);
    u16 wc = R3D::Rgb(206, 196, 166);
    for (i = 0; i < nw; i++)
        {
        int ax, az, bx, bz;
        if (GameKitOutWall(*this, i, &ax, &az, &bx, &bz) < 2 * M) continue;
        int sx0 = mx0 + hx0 + ((ax - cx) >> 8) * sc, sy0 = my0 + hy0 - ((az - cz) >> 8) * sc;
        int sx1 = mx0 + hx0 + ((bx - cx) >> 8) * sc, sy1 = my0 + hy0 - ((bz - cz) >> 8) * sc;
        if ((sx0 < mx0 && sx1 < mx0) || (sx0 >= mx1 && sx1 >= mx1) || (sy0 < my0 && sy1 < my0) || (sy0 >= my1 && sy1 >= my1)) continue;
        int n = Abs(sx1 - sx0) > Abs(sy1 - sy0) ? Abs(sx1 - sx0) : Abs(sy1 - sy0), k;
        if (n > 400) continue;
        for (k = 0; k <= n; k++) MapPlot(r, n ? sx0 + (sx1 - sx0) * k / n : sx0, n ? sy0 + (sy1 - sy0) * k / n : sy0, mx0, my0, mx1, my1, wc);
        }
    }
    // door markers: every house's door, and the doors that can be used (brighter)
    int best = -1, bestD = 10 * sc;
    const char* bestName = 0;
    for (i = 0; i < KPlanDoor_N + NumDoors(); i++)
        {
        int wx, wz;
        const char* name;
        if (i < KPlanDoor_N) { wx = (CITY_CX * CELL + CELL / 2) + KPlanDoor[i * 2] * METER / 10; wz = (CITY_CZ * CELL + CELL / 2) + KPlanDoor[i * 2 + 1] * METER / 10; name = KPlanDoorName[i]; }
        else { const DoorLink& dl = Door(i - KPlanDoor_N); if (dl.ocell) continue; wx = dl.ox; wz = dl.oz; name = dl.name; }
        int px = mx0 + hx0 + ((wx - cx) >> 8) * sc, py = my0 + hy0 - ((wz - cz) >> 8) * sc;
        if (px < mx0 + 3 || px > mx1 - 4 || py < my0 + 4 || py > my1 - 4) continue;
        u16 c = i < KPlanDoor_N ? R3D::Rgb(214, 212, 204) : R3D::Rgb(255, 255, 250);
        r.Rect(px - 2, py - 3, 5, 7, c); r.Rect(px - 1, py - 4, 3, 1, c);            // the arched marker
        r.Rect(px, py - 1, 1, 2, R3D::Rgb(20, 20, 20)); r.Rect(px - 1, py, 3, 1, R3D::Rgb(20, 20, 20));
        int d = (px - (mx0 + hx0)) * (px - (mx0 + hx0)) + (py - (my0 + hy0)) * (py - (my0 + hy0));
        if (d < bestD * bestD) { bestD = r3d_isqrt(d); best = i; bestName = name; }
        }
    // the player's arrow
    {
    int px = mx0 + hx0 + ((iS.px - cx) >> 8) * sc, py = my0 + hy0 - ((iS.pz - cz) >> 8) * sc;
    int fx = r3d_sin(iS.yaw), fz = r3d_cos(iS.yaw), k;
    for (k = -3; k <= 6; k++)
        {
        int ax = px + (fx * k >> 14), ay = py - (fz * k >> 14), wdt = k < 0 ? 0 : (6 - k) / 2;
        int s;
        for (s = -wdt; s <= wdt; s++) MapPlot(r, ax + (fz * s >> 14), ay + (fx * s >> 14), mx0, my0, mx1, my1, C_WHITE);
        }
    }
    // the cross in the middle, the frame, the title, the name under the cross, the keys
    r.Rect(mx0 + hx0 - 3, my0 + hy0, 7, 1, C_GOLD); r.Rect(mx0 + hx0, my0 + hy0 - 3, 1, 7, C_GOLD);
    u16 fr = R3D::Rgb(170, 164, 150);
    r.Rect(mx0 - 2, my0 - 2, mw + 4, 1, fr); r.Rect(mx0 - 2, my1 + 1, mw + 4, 1, fr);
    r.Rect(mx0 - 2, my0 - 2, 1, mh + 4, fr); r.Rect(mx1 + 1, my0 - 2, 1, mh + 4, fr);
    {
    // the title: the place the view is in, else the hold
    const char* title = "Whiterun Hold";
    for (i = 0; i < GameLocN(); i++)
        {
        LocInfo l; GameLoc(*this, i, l);
        int dx = (iMapCX - l.x) >> 8, dz = (iMapCZ - l.z) >> 8, rr = (l.r >> 8) + 40;
        if (dx * dx + dz * dz < rr * rr) { title = l.name; break; }
        }
    r.TextShadow(x0 + (w - r.TextW(title)) / 2, 3, title, C_WHITE);
    }
    if (best >= 0) r.TextShadow(mx0 + hx0 - r.TextW(bestName) / 2, my0 + hy0 + 6, bestName, C_WHITE);
    r.Text(x0 + 4, OUT_H - 10, "Arrows move  Z/X zoom  L world  T go  M close", C_GREY);
    }

// ---------------------------------------------------------------- book
void Game::DrawBook(int x0, int w)
    {
    R3D& r = *iR;
    int bx = x0 + 16, bw = w - 32;
    r.Rect(bx, 10, bw, OUT_H - 20, R3D::Rgb(222, 206, 166));
    r.Rect(bx + bw / 2, 14, 1, OUT_H - 28, R3D::Rgb(170, 150, 110));
    const char* s = GameBook(iBookId);
    // find page
    int page = 0;
    while (page < iBookPage)
        {
        while (*s && *s != '|') s++;
        if (!*s) break;
        s++; page++;
        }
    if (page < iBookPage || !*s) { iUi = iBookProp >= 0 ? UI_PLAY : UI_INV; iBookProp = -1; return; }
    DrawWrapped(bx + 10, 22, bw - 20, s, R3D::Rgb(50, 36, 24), 15);
    const char* turn = iBookProp >= 0 ? "< > turn page" : "E / > next page";
    r.Text(bx + bw - r.TextW(turn) - 8, OUT_H - 22, turn, R3D::Rgb(110, 90, 60));
    if (iBookProp >= 0)
        {
        // lying in the world: E takes it (steals it, if it is someone's)
        const char* t = (iS.prop[iBookProp].flags & PF_OWNED) ? "E steal" : "E take";
        r.Text(bx + 8, OUT_H - 22, t, (iS.prop[iBookProp].flags & PF_OWNED) ? R3D::Rgb(160, 40, 30) : R3D::Rgb(110, 90, 60));
        }
    }

// ---------------------------------------------------------------- frame
// (v0.4.74) the loading screen: dark, the word, where to (a frame shows it before the file is read)
void Game::DrawLoading(u16* aOut, int aStride)
    {
    R3D& r = *iR;
    r.BeginOverlay(aOut, aStride);
    int w = r.OutW(), x0 = r.OutX0(), y;
    for (y = 0; y < OUT_H; y++) { u16* p = aOut + y * aStride; int x; for (x = 0; x < OUT_W; x++) p[x] = R3D::Rgb(10, 9, 8); }
    const char* t = "Loading...";
    r.TextShadow(x0 + (w - r.TextW(t)) / 2, OUT_H / 2 - 8, t, C_WHITE);
    const char* s = iLoadKind == 3 ? "the saved game" : 0;
    if (s) r.Text(x0 + (w - r.TextW(s)) / 2, OUT_H / 2 + 6, s, C_GREY);
    r.Rect(x0 + w / 2 - 40, OUT_H / 2 + 20, 80, 1, C_GOLD);
    }

void Game::Render(u16* aOut, int aStride)
    {
    if (iLoadPhase) { DrawLoading(aOut, aStride); iLoadPhase = 2; return; }
    SetupLights();
    RenderFrame(aOut, aStride);
    iR->ApplyGamma(aOut, aStride);
    }

void Game::RenderFrame(u16* aOut, int aStride)
    {
    R3D& r = *iR;
    int shake = iShakeUs > 0 ? ((r3d_sin(iTimeUs >> 9) * 6) >> 14) : 0;
    int camY = iS.py - iCrouchY;
    int pitch = iS.pitch + shake;
    if (iUi == UI_TITLE)
        {
        // slow orbit round Vindgard on its hill
        int a = (iTitleUs >> 14) & 1023;
        int tx = GameVillageX() + ((r3d_sin(a) * 110) >> 14) * M, tz = GameVillageZ() + ((r3d_cos(a) * 110) >> 14) * M;
        r.SetCamera(tx, r.GroundAt(tx, tz) + 26 * M, tz, (a + 512) & 1023, -50);
        }
    else if (iS.third) r.SetCamera(iCamX, iCamY, iCamZ, iS.yaw, pitch);
    else r.SetCamera(iS.px, camY, iS.pz, iS.yaw, pitch);
    r.iAnimMs = iTimeUs >> 10;
    r.iZoneMask = GameZoneMask(*this);
    { void GameKitPvs(Game& g); GameKitPvs(*this); }
    // a menu over the world: the world drawn once as the menu opened, kept, and only the menu drawn each frame (the
    // world stands still behind it, as Skyrim's does; waiting out the hours shows them passing)
    int menu = iUi == UI_TELE || iUi == UI_HELP || iUi == UI_CONT || iUi == UI_COOK || iUi == UI_TRADE || iUi == UI_INV ||
               iUi == UI_MAP || iUi == UI_BOOK || iUi == UI_MENU || iUi == UI_PUZZLE || (iUi == UI_WAIT && iWaitLeft == 0);
    if (!menu) iFrozenOk = 0;
    int vmDone = 0;
    if (iUi == UI_MAP) r.BeginOverlay(aOut, aStride);          // the map covers all of the screen: no world behind it
    else if (menu && iFrozenOk && iFrozen)
        {
        r.BeginOverlay(aOut, aStride);
        int y;
        for (y = 0; y < OUT_H; y++) r3d_memmove(aOut + y * aStride, iFrozen + y * OUT_W, OUT_W * 2);
        }
    else
        {
        r.BeginFrame(aOut, aStride);
        if (IsDungeon(iS.cell) && iUi != UI_TITLE) DrawDungeon();
        if (!(r.iSkip & 8)) DrawEntities();
        if (iUi != UI_TITLE)
            {
            if (!(r.iSkip & 16)) DrawProps();
            if (!(r.iSkip & 32)) { DrawFlora(); DrawGrass(); }
            { void GameFireWallDraw(Game& g); GameFireWallDraw(*this); }     // (v0.4.71) the staff's flames
            DrawPlayerBody();
            }
        // (v0.4.50) on half rows: the view model with the world (it is drawn against its depths), then the rows doubled
        if (r.iHalfOn)
            {
            if (iUi == UI_PLAY || iUi == UI_DIALOG || iUi == UI_DEAD) { DrawViewModel(); vmDone = 1; }
            r.EndWorld();
            }
        if (iUi != UI_TITLE) DrawWeather();
        if (menu && iFrozen)
            {
            int y;
            for (y = 0; y < OUT_H; y++) r3d_memmove(iFrozen + y * OUT_W, aOut + y * aStride, OUT_W * 2);
            iFrozenOk = 1;
            }
        }
    int x0 = r.OutX0(), w = r.OutW();
    if (iUi == UI_TITLE)
        {
        int cx = x0 + w / 2;
        int sc = w >= 400 ? 4 : 3;
        const char* t = "HRIMFELL";
        BigText(r, cx - r.TextW(t) * sc / 2 + 2, 30, t, 0, sc);
        BigText(r, cx - r.TextW(t) * sc / 2, 28, t, C_GOLD, sc);
        r.TextShadow(cx - r.TextW("A tale of the northern hills") / 2, 64, "A tale of the northern hills", C_WHITE);
        r.Shade(x0 + 10, 100, w - 20, 92);
        static const char* const KHelp[8] =
            {
            "Joystick: look   WASD: move   Shift: sprint",
            "Press/Enter: attack   R: draw/sheathe weapon",
            "E: use/talk  hold E: carry, attack: throw  Q: spell",
            "F: 1st/3rd person   T: wait   L: torch",
            "Z or Y: shout   I: items   M/Tab: map   J: quest",
            "Esc: menu (save, load, travel, display)",
            "",
            "Press joystick or E to begin"
            };
        int i;
        for (i = 0; i < 8; i++) r.Text(cx - r.TextW(KHelp[i]) / 2, 104 + i * 11, KHelp[i], i == 7 ? C_GOLD : C_WHITE);
        return;
        }
    if (!vmDone && (iUi == UI_PLAY || iUi == UI_DIALOG || iUi == UI_DEAD)) DrawViewModel();
    if (iFadeUs > 0) { r.Shade(x0, 0, w, OUT_H); if (iFadeUs > 200000) r.Shade(x0, 0, w, OUT_H); }
    if (iUi == UI_WAIT) { DrawWait(x0, w); return; }
    if (iUi == UI_TELE) { DrawTele(x0, w); return; }
    if (iUi == UI_HELP) { DrawHelp(x0, w); return; }
    if (iUi == UI_CONT) { DrawCont(x0, w); return; }
    if (iUi == UI_COOK) { DrawCook(x0, w); return; }
    if (iUi == UI_TRADE) { DrawTrade(x0, w); DrawFps(x0, w); return; }
    if (iUi == UI_INV) { DrawInventory(x0, w); DrawFps(x0, w); return; }
    if (iUi == UI_MAP) { DrawMap(x0, w); return; }
    if (iUi == UI_BOOK) { DrawBook(x0, w); return; }
    if (iUi == UI_MENU) { DrawMenu(x0, w); return; }
    if (iUi == UI_PUZZLE) { DrawPuzzle(x0, w); return; }
    DrawHud();
    if (iUi == UI_DIALOG) DrawDialog(x0, w);
    if (iUi == UI_DEAD)
        {
        int cx = x0 + w / 2;
        r.Tint(x0, 0, w, OUT_H, 0x8000);
        const char* s = "YOU DIED";
        BigText(r, cx - r.TextW(s), 70, s, C_WHITE, 2);
        r.TextShadow(cx - r.TextW("Press E or the joystick to wake below Whiterun") / 2, 100, "Press E or the joystick to wake below Whiterun", C_WHITE);
        }
    }
