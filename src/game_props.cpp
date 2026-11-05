// game_props.cpp - things you can use in the world: loose items (take, steal, pick up and carry),
// harvestable garlic braids and hanging fish, barrels and sacks (containers with sliding lids), chairs,
// the cooking pot, and the market merchants with their barter screen.
#include "game.h"

static const int M = METER;
int GameFullDist(int v, int aBase);
static inline int Abs(int v) { return v < 0 ? -v : v; }
static int Clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static int Cm(int cm) { return cm * M / 100; }

const ItemDef& GameItem(int aId);
const u8* GameItemCol(int aId);

enum { CITY_X = CITY_CX, CITY_Z = CITY_CZ };      // (game.h)
static int Wx(int mx) { return (CITY_X * CELL + CELL / 2) + mx * M; }
static int Wz(int mz) { return (CITY_Z * CELL + CELL / 2) + mz * M; }
#include "plan_pos.inc"
static int XD(int dm) { return (CITY_X * CELL + CELL / 2) + dm * M / 10; }
static int ZD(int dm) { return (CITY_Z * CELL + CELL / 2) + dm * M / 10; }
enum { IN_OX = 100 * METER, IN_OZ = 100 * METER, IN_FLOOR = 100 * METER };
static int Ix(int cm) { return IN_OX + Cm(cm); }
static int Iz(int cm) { return IN_OZ + Cm(cm); }

static const u16 KGold = 0xFEA0, KWhite = 0xFFFF, KGrey = 0xA514, KRed = 0xF8A2, KGreen = 0x87E8;

int GameZoneOf(int aCell, int aX, int aZ);
// ---------------------------------------------------------------- bookkeeping
int Game::AddProp(int aType, int aCell, int aX, int aY, int aZ, int aYaw, int aItem, int aCount, int aFlags)
    {
    if (iS.nprop >= MAXPROP) return -1;
    Prop& p = iS.prop[iS.nprop];
    r3d_memset(&p, 0, sizeof(Prop));
    p.type = (u8)aType; p.cell = (u8)aCell; p.x = aX; p.y = aY; p.z = aZ; p.yaw = (short)(aYaw & 1023);
    p.item = (short)aItem; p.count = (short)aCount; p.flags = (u8)aFlags;
    return iS.nprop++;
    }

int Game::NewCont()
    {
    if (iS.ncont >= MAXCONT) return 0;
    r3d_memset(&iS.cont[iS.ncont], 0, sizeof(Cont));
    return iS.ncont++;
    }

void Game::ContAdd(int aCont, int aId, int aCount)
    {
    Cont& c = iS.cont[aCont];
    int i;
    for (i = 0; i < CONTSLOTS; i++) if (c.n[i] && c.id[i] == aId) { c.n[i] = (u8)Clamp(c.n[i] + aCount, 0, 250); return; }
    for (i = 0; i < CONTSLOTS; i++) if (!c.n[i]) { c.id[i] = (u8)aId; c.n[i] = (u8)aCount; return; }
    }

int Game::CountItem(int aId)
    {
    int i, n = 0;
    for (i = 0; i < iS.ninv; i++) if (iS.inv[i].id == aId) n += iS.inv[i].count;
    return n;
    }

void Game::TakeItems(int aId, int aCount)
    {
    int i;
    for (i = iS.ninv - 1; i >= 0 && aCount > 0; i--)
        if (iS.inv[i].id == aId && !iS.inv[i].equipped)
            {
            int k = iS.inv[i].count < aCount ? iS.inv[i].count : aCount;
            aCount -= k;
            RemoveItem(i, k);
            }
    }

// ---------------------------------------------------------------- the market
// stalls as placed by BuildCity: position (m), yaw (the counter's front faces local -z)
// (the plan's stalls: position (dm), yaw, see plan_pos.inc)
static int StallYaw(int s) { return KPlanStall[s * 3 + 2]; }

// local (cm, along the counter / across it) -> world
static void StallPoint(int aStall, int aLx, int aLz, int& aX, int& aZ)
    {
    int sx = XD(KPlanStall[aStall * 3]), sz = ZD(KPlanStall[aStall * 3 + 1]);
    int yaw = StallYaw(aStall);
    int cs = r3d_cos(yaw), sn = r3d_sin(yaw), lx = Cm(aLx), lz = Cm(aLz);
    aX = sx + ((lx * cs + lz * sn) >> 14);
    aZ = sz + ((-lx * sn + lz * cs) >> 14);
    }

// goods laid out on each counter (x along the counter in cm, item)
// goods on each counter: x along the counter, z across it (the counter's middle is -88), height above the
// counter top (cm; the crates' floors are 2 cm up), item, lies along the counter (1) or turned at random (0)
static const int KGoods0[] =
    {
    -115, -97, 6, IT_CABBAGE, 0,  -86, -97, 6, IT_CABBAGE, 0,  -115, -79, 6, IT_CABBAGE, 0,  -86, -79, 6, IT_CABBAGE, 0,
    -14, -95, 8, IT_APPLE, 0,  0, -95, 8, IT_APPLE, 0,  14, -95, 8, IT_APPLE, 0,  -7, -81, 8, IT_APPLE, 0,  7, -81, 8, IT_APPLE, 0,
    0, -88, 16, IT_APPLE, 0,
    48, -94, 0, IT_LEEK, 1,  50, -82, 0, IT_LEEK, 1,  82, -95, 0, IT_CARROT, 1,  86, -83, 0, IT_CARROT, 1,
    110, -93, 0, IT_POTATO, 0,  124, -82, 0, IT_POTATO, 0,  136, -95, 0, IT_POTATO, 0,  140, -80, 0, IT_GARLIC, 0,  -1
    };
static const int KGoods1[] =
    {
    -110, -92, 0, IT_SALMON, 1,  -100, -80, 0, IT_SALMON, 1,  -40, -88, 0, IT_RAW_BEEF, 1,  10, -90, 0, IT_RAW_VENISON, 1,
    58, -86, 0, IT_RAW_VENISON, 1,  100, -94, 0, IT_SALT, 0,  114, -82, 0, IT_SALT, 0,  -1
    };
static const int KGoods2[] =
    {
    -110, -92, 1, IT_SILVER_RING, 0,  -96, -82, 1, IT_GOLD_RING, 0,  -66, -90, 1, IT_SILVER_NECKLACE, 0,  -30, -86, 1, IT_GOLD_NECKLACE, 0,
    4, -92, 1, IT_GARNET, 0,  20, -82, 1, IT_AMETHYST, 0,  36, -92, 1, IT_GARNET, 0,  -1
    };

// merchants: name, stock (item, count), purse
static const char* const KMerName[MER_COUNT] = { "Carlotta Valentia", "Anoriath", "Fralia Gray-Mane", "Ri'saad", "Arcadia", "Hulda", "Lucan Valerius", "Orgnar" };
static const char* const KMerTitle[MER_COUNT] = { "greengrocer", "butcher", "jeweller", "caravan trader", "apothecary", "innkeeper", "general goods", "innkeeper" };
// what each merchant buys, by keyword (Skyrim's vendor buy lists)
static const int KMerBuys[MER_COUNT] =
    {
    KW_V_FOOD | KW_V_FOODRAW | KW_V_INGR,
    KW_V_FOOD | KW_V_FOODRAW | KW_V_ANIMALPART | KW_V_HIDE,
    KW_V_JEWELRY | KW_V_GEM,
    // the caravan deals in anything, as Skyrim's caravans do
    KW_V_FOOD | KW_V_FOODRAW | KW_V_INGR | KW_V_POTION | KW_V_POISON | KW_V_WEAPON | KW_V_ARMOR | KW_V_CLOTHING |
    KW_V_JEWELRY | KW_V_GEM | KW_V_BOOK | KW_V_CLUTTER | KW_V_HIDE | KW_V_ANIMALPART | KW_V_TOOL | KW_V_ORE,
    // the apothecary: potions, poisons, ingredients (Skyrim's apothecary buy list)
    KW_V_POTION | KW_V_POISON | KW_V_INGR | KW_V_ANIMALPART,
    // the innkeeper: food and drink
    KW_V_FOOD | KW_V_FOODRAW | KW_DRINK,
    // the Riverwood Trader: general goods, he buys anything
    KW_V_FOOD | KW_V_FOODRAW | KW_V_INGR | KW_V_POTION | KW_V_POISON | KW_V_WEAPON | KW_V_ARMOR | KW_V_CLOTHING |
    KW_V_JEWELRY | KW_V_GEM | KW_V_BOOK | KW_V_CLUTTER | KW_V_HIDE | KW_V_ANIMALPART | KW_V_TOOL | KW_V_ORE,
    // the Sleeping Giant: food and drink
    KW_V_FOOD | KW_V_FOODRAW | KW_DRINK
    };
int GameMerchantBuys(int m, int aId) { return (GameItem(aId).kw & KMerBuys[m % MER_COUNT]) != 0; }
static const int KStock0[] = { IT_APPLE, 8, IT_CABBAGE, 6, IT_LEEK, 6, IT_POTATO, 10, IT_CARROT, 8, IT_GARLIC, 6, IT_BREAD, 4, IT_CHEESE, 3, IT_SALT, 5, -1 };
static const int KStock1[] = { IT_RAW_BEEF, 4, IT_RAW_VENISON, 4, IT_SALMON, 5, IT_SALT, 6, IT_VENISON, 2, IT_COOKED_BEEF, 2, -1 };
static const int KStock3[] = { IT_POT_MHEAL, 3, IT_POT_MSTAM, 2, IT_POT_MMAG, 2, IT_SALT, 6, IT_GARNET, 1, IT_AMETHYST, 1,
                               IT_WOLF_PELT, 2, IT_TORCH, 4, IT_IRON_DAGGER, 1, IT_EMBER_DRAUGHT, 2, -1 };
// the apothecary's chest (after Arcadia's: blisterwort, wheat, salt, healing and other potions, poisons, ingredients)
static const int KStock4[] = { IT_BLISTERWORT, 3, IT_WHEAT, 3, IT_SALT, 1, IT_LAVENDER, 4, IT_BLUE_FLOWER, 5, IT_LICHEN, 2, IT_BONE_MEAL, 2,
                               IT_WHITE_CAP, 3, IT_GRAVEBELL, 3, IT_JUNIPER, 3, IT_HOUNDROOT, 2, IT_TAPROOT, 1, IT_WISP_DUST, 1, IT_EMBER_SALTS, 1,
                               IT_SPIDER_EGG, 2, IT_PIKE_SCALES, 2, IT_IMP_CAP, 2, IT_TREANT_SAP, 1,
                               IT_POT_MHEAL, 6, IT_POT_HEAL, 3, IT_POT_MMAG, 3, IT_POT_MSTAM, 3, IT_POT_CURE, 1, IT_POT_RESFIRE, 1,
                               IT_POISON_WEAK, 3, IT_FROST_VENOM, 2, -1 };
// the inn's merchant chest (after the Bannered Mare's: mead, drinks, inn food, a salt pile)
static const int KStock5[] = { IT_CREAM_MEAD, 4, IT_BERRY_WINE, 4, IT_EMBER_DRAUGHT, 3, IT_BREAD, 4, IT_CHEESE, 3, IT_APPLE, 4,
                               IT_VENISON, 2, IT_COOKED_BEEF, 2, IT_SALMON_STEAK, 2, IT_VEG_SOUP, 2, IT_VENISON_STEW, 2, IT_SALT, 1, -1 };
static const int KStock2[] = { IT_SILVER_RING, 2, IT_GOLD_RING, 2, IT_SILVER_NECKLACE, 1, IT_GOLD_NECKLACE, 1, IT_GARNET, 2, IT_AMETHYST, 1, -1 };

void Game::SpawnMerchants()
    {
    int m;
    for (m = 0; m < MER_CARAVAN; m++)
        {
        int x, z;
        StallPoint(m, 0, 60, x, z);                           // behind the counter
        int yaw = (StallYaw(m) + 512) & 1023;
        if (iWrKit)
            {
            // (v0.4.57) Skyrim's market: where its merchants lean on their counters (CounterLeanMarker) - Carlotta's
            // greens, Anoriath's meat, Fralia's jewellery
            static const int KLean[3 * 3] = { 25167, -6982, 225,   25326, -7390, 268,   24384, -7088, 151 };
            x = (int)(((long long)KLean[m * 3] + 233472) * 36576 / 10000); z = (int)(((long long)KLean[m * 3 + 1] + 176128) * 36576 / 10000);
            yaw = (KLean[m * 3 + 2] * 1024 / 360) & 1023;
            }
        int idx = SpawnEnt(E_VILLAGER, x, z, yaw);
        if (idx < 0) continue;
        Ent& e = iS.e[idx];
        e.role = ROLE_MERCHANT; e.pad1 = (u8)m; e.var = (u8)(m == MER_BUTCHER ? 0 : (m == MER_GROCER ? 1 : 3));
        int c = NewCont();
        e.pad2 = (u8)c;
        const int* st = m == 0 ? KStock0 : (m == 1 ? KStock1 : KStock2);
        for (; st[0] >= 0; st += 2) ContAdd(c, st[0], st[1]);
        iS.mgold[m] = m == MER_JEWELER ? 700 : (m == MER_GROCER ? 300 : 400);
        }
    // the cat-folk caravan camped across the road from the stables (south-west of the city): its trader by the fire, two guards
    {
    int idx = SpawnEnt(E_VILLAGER, Wx(-123), Wz(-38), 640);
    if (idx >= 0)
        {
        Ent& e = iS.e[idx];
        e.role = ROLE_TRADER; e.pad1 = MER_CARAVAN; e.var = 0;
        int c = NewCont();
        e.pad2 = (u8)c;
        const int* st = KStock3;
        for (; st[0] >= 0; st += 2) ContAdd(c, st[0], st[1]);
        iS.mgold[MER_CARAVAN] = 800;
        }
    idx = SpawnEnt(E_VILLAGER, Wx(-116), Wz(-47), 600);
    if (idx >= 0) { iS.e[idx].role = ROLE_CARAVAN; iS.e[idx].var = 1; }
    idx = SpawnEnt(E_VILLAGER, Wx(-112), Wz(-36), 300);
    if (idx >= 0) { iS.e[idx].role = ROLE_CARAVAN; iS.e[idx].var = 2; }
    }
    // Eira the apothecary, behind her long desk in the Bitterroot
    {
    int idx = SpawnEnt(E_VILLAGER, Ix(930), Iz(450), 768);
    if (idx >= 0)
        {
        Ent& e = iS.e[idx];
        e.cell = CELL_SHOP; e.y = IN_FLOOR;
        e.role = ROLE_MERCHANT; e.pad1 = MER_APOTHECARY; e.var = 3;
        int c = NewCont();
        e.pad2 = (u8)c;
        const int* st = KStock4;
        for (; st[0] >= 0; st += 2) ContAdd(c, st[0], st[1]);
        iS.mgold[MER_APOTHECARY] = 1000;
        }
    }
    // the Gilded Stag: Gunnhild behind her bar; Nadira serving; Eskil the bard by the fire; Hjordis in her corner;
    // Torolf, who is always here; and Vilja, a traveller from over the sea
    {
    int idx = SpawnEnt(E_VILLAGER, Ix(760), Iz(110), 0);
    if (idx >= 0)
        {
        Ent& e = iS.e[idx];
        e.cell = CELL_INN; e.y = IN_FLOOR;
        e.role = ROLE_MERCHANT; e.pad1 = MER_INN; e.var = 1;
        int c = NewCont();
        e.pad2 = (u8)c;
        const int* st = KStock5;
        for (; st[0] >= 0; st += 2) ContAdd(c, st[0], st[1]);
        iS.mgold[MER_INN] = 500;
        }
    static const int KInnPeople[5 * 5] =
        {
        // x, z (cm), yaw, var (looks: odd = a woman), walks about
        420, 820, 0, 3, 1,      950, 900, 768, 2, 0,     930, 680, 768, 7, 0,     420, 930, 256, 0, 1,     240, 1040, 256, 5, 0
        };
    int k;
    for (k = 0; k < 5; k++)
        {
        const int* p = KInnPeople + k * 5;
        int i2 = SpawnEnt(E_VILLAGER, Ix(p[0]), Iz(p[1]), p[2]);
        if (i2 < 0) continue;
        Ent& e = iS.e[i2];
        e.cell = CELL_INN; e.y = IN_FLOOR;
        e.role = ROLE_NAMED; e.pad1 = (u8)k; e.pad2 = (u8)p[4]; e.var = (u8)p[3];
        }
    }
    }

const char* GameMerchantName(int m) { return KMerName[m % MER_COUNT]; }

// ---------------------------------------------------------------- placing the props
void Game::SpawnProps()
    {
    R3D& r = *iR;
    int s, i;
    // (v0.4.57: Skyrim's Whiterun - its own stalls, barrels and gate: none of Vindgard's things outside)
    iBridgeProp = -1;
    if (!iWrKit)
    {
    // market goods on the counters (owned)
    for (s = 0; s < 5; s++)
        {
        const int* g = s == 0 || s == 3 ? KGoods0 : (s == 1 || s == 4 ? KGoods1 : KGoods2);
        int gy = r.GroundAt(XD(KPlanStall[s * 3]), ZD(KPlanStall[s * 3 + 1])) + Cm(90);
        int yaw = StallYaw(s);
        int n = 0;
        for (i = 0; g[i * 5] != -1; i++)
            {
            const int* e = g + i * 5;
            if (s >= 3 && (i & 1)) continue;                  // the unmanned stalls are half sold out
            int x, z;
            StallPoint(s, e[0], e[1], x, z);
            int id = e[3];
            const short* rest = iItemRest + GameItem(id).mesh * 3;
            int iy = e[4] ? yaw + 256 + (n & 1) * 20 : yaw + n * 170;
            AddProp(PT_ITEM, 0, x, gy + Cm(e[2]) + rest[1], z, iy, id, 1, PF_OWNED);
            n++;
            }
        if (s == 1 || s == 4)
            {
            // meat and salmon hanging from the rail under the awning
            int x, z;
            StallPoint(s, -60, -125, x, z);
            AddProp(PT_FISH, 0, x, gy - Cm(90) + Cm(219), z, yaw, IT_RAW_VENISON, 1, PF_OWNED);
            StallPoint(s, 60, -125, x, z);
            AddProp(PT_FISH, 0, x, gy - Cm(90) + Cm(219), z, yaw + 256, IT_SALMON, 1, PF_OWNED);
            }
        }
    // barrels by the smithy, in the market, in the Wind District, at the meadery and the stables
    static const int KBar[] = { -52, -14,  -51, -14,  -14, -8,  12, 34,  -62, -104,  -60, -104,  -62, -106,  -60, -106,  -84, -49 };
    static const int KBarGoods[] = { IT_APPLE, IT_POTATO, IT_CABBAGE, IT_SALT, IT_TANKARD, IT_BREAD, IT_CARROT, IT_LEEK, IT_SALMON };
    for (i = 0; i < 9; i++)
        {
        int x = Wx(KBar[i * 2]) + (i & 1) * 60, z = Wz(KBar[i * 2 + 1]);
        int c = NewCont();
        ContAdd(c, KBarGoods[i], 2 + (i % 3));
        if (i & 1) ContAdd(c, KBarGoods[(i + 3) % 9], 1);
        AddProp(PT_BARREL, 0, x, r.GroundAt(x, z), z, i * 170, c, 0, 0);
        }
    // the caravan camp: a bedroll in each tent (two in the wide one), a knapsack by each flap, crates by the fire
    for (i = 0; i < 3; i++)
        {
        int cs = r3d_cos(iTentYaw[i]), sn = r3d_sin(iTentYaw[i]);
        int big = iTentBig[i], k;
        for (k = 0; k < (big ? 2 : 1); k++)
            {
            int lx = big ? (k ? Cm(105) : -Cm(105)) : 0, lz = -Cm(15);
            int x = iTentX[i] + ((lx * cs + lz * sn) >> 14), z = iTentZ[i] + ((-lx * sn + lz * cs) >> 14);
            AddProp(PT_BED, 0, x, r.GroundAt(x, z), z, (iTentYaw[i] + 512) & 1023, 2, 0, 0);
            }
        int lx = big ? Cm(270) : Cm(150), lz = big ? Cm(190) : Cm(125);
        int x = iTentX[i] + ((lx * cs + lz * sn) >> 14), z = iTentZ[i] + ((-lx * sn + lz * cs) >> 14);
        int c = NewCont();
        ContAdd(c, i == 0 ? IT_SALT : (i == 1 ? IT_BREAD : IT_WOLF_PELT), 1 + i);
        if (i == 2) ContAdd(c, IT_BOOK_NIGHTTEARS, 1);
        AddProp(PT_SACK, 0, x, r.GroundAt(x, z), z, i * 200, c, 0, 0);
        }
    {
    static const int KCrate[2 * 2] = { -120, -34,  -133, -40 };
    for (i = 0; i < 2; i++)
        {
        int x = Wx(KCrate[i * 2]), z = Wz(KCrate[i * 2 + 1]);
        int c = NewCont();
        ContAdd(c, i ? IT_POTATO : IT_CABBAGE, 4); ContAdd(c, IT_SALT, 2);
        AddProp(PT_CRATE, 0, x, r.GroundAt(x, z), z, i * 300, c, 0, 0);
        }
    }
    // the drawbridge before the gate (down) and its lever
    iBridgeProp = AddProp(PT_BRIDGE, 0, iBridgeX, iBridgeY, iBridgeZ, 0, 0, 0, 0);
    AddProp(PT_LEVER, 0, iLeverX, r.GroundAt(iLeverX, iLeverZ), iLeverZ, 768, 0, 0, 0);
    }
    // ---- Vindgard Stables residence (v0.4.48: only without Skyrim's own Stables - the area's file)
    if (!iWrKit && !GameIsKitCell(*this, CELL_STABLES))
    {
    int cs = CELL_STABLES;
    // chairs at the round table, facing it
    AddProp(PT_CHAIR, cs, Ix(-515), IN_FLOOR, Iz(-280), 768, 0, 0, 0);
    AddProp(PT_CHAIR, cs, Ix(-325), IN_FLOOR, Iz(-280), 256, 0, 0, 0);
    // food on the round table (Arnvid's)
    // dishes and food on the tables (Arnvid's): everything here can be taken or moved
    static const int KTab[] =
        {
        // x, z (cm), height of the surface (cm), item, yaw
        -395, -300, 78, IT_APPLE, 0,   -445, -255, 78, IT_PLATE, 0,   -445, -255, 80, IT_CHEESE, 300,   -398, -258, 78, IT_TANKARD, 100,
        -535, 110, 80, IT_BREAD, 256,  -542, 136, 80, IT_BREAD, 300,  -525, 190, 80, IT_WOOD_BOWL, 0,  -545, 230, 80, IT_JUG, 0,  -1
        };
    for (i = 0; KTab[i * 5] != -1; i++)
        {
        const int* e = KTab + i * 5;
        AddProp(PT_ITEM, cs, Ix(e[0]), IN_FLOOR + Cm(e[2]) + iItemRest[GameItem(e[3]).mesh * 3 + 1], Iz(e[1]), e[4], e[3], 1, PF_OWNED);
        }
    // the bench at the long table (sitter faces the table)
    AddProp(PT_BENCH, cs, Ix(-455), IN_FLOOR, Iz(160), 256, 0, 0, 0);
    // garlic braids and salmon hanging from the rafters
    static const int KHang[] = { PT_GARLIC, -112, -150,  PT_GARLIC, -88, -135,  PT_FISH, -112, -95,  PT_GARLIC, 88, 160,  PT_FISH, 112, 190,  PT_FISH, 112, 230 };
    for (i = 0; i < 6; i++) AddProp(KHang[i * 3], cs, Ix(KHang[i * 3 + 1]), IN_FLOOR + Cm(298), Iz(KHang[i * 3 + 2]), i * 90, 0, 0, PF_OWNED);
    // barrels and a sack
    static const int KIBar[] = { 150, 400,  150, 320,  -560, -330 };
    for (i = 0; i < 3; i++)
        {
        int c = NewCont();
        ContAdd(c, i == 0 ? IT_SALT : (i == 1 ? IT_POTATO : IT_CARROT), 3);
        if (i == 2) ContAdd(c, IT_LEEK, 2);
        AddProp(PT_BARREL, cs, Ix(KIBar[i * 2]), IN_FLOOR, Iz(KIBar[i * 2 + 1]), i * 300, c, 0, 0);
        }
    {
    int c = NewCont();
    ContAdd(c, IT_POTATO, 4); ContAdd(c, IT_CABBAGE, 1);
    AddProp(PT_SACK, cs, Ix(-470), IN_FLOOR, Iz(-420), 100, c, 0, 0);
    }
    // the cooking pot at the hearth, and a hare roasting on the spit
    AddProp(PT_POT, cs, Ix(-110), IN_FLOOR, Iz(120), 0, 0, 0, 0);
    AddProp(PT_ITEM, cs, Ix(-180), IN_FLOOR + Cm(73), Iz(40), 0, IT_CHARRED_MEAT, 1, PF_OWNED);
    // what stands on the wall shelves: clay pots below; a bowl, drinks and tankards above
    static const int KShelf[] =
        {
        // shelf (0 = north wall, 1 = south wall), x along the shelf (cm), board height (cm), item
        0, -45, 106, IT_CLAY_POT,  0, 0, 106, IT_CLAY_POT,  0, 45, 106, IT_JUG,
        0, -40, 161, IT_WOOD_BOWL, 0, 8, 161, IT_BERRY_WINE, 0, 45, 161, IT_TANKARD,
        1, -45, 106, IT_PLATE,     1, 0, 106, IT_CLAY_POT,  1, 45, 106, IT_CLAY_POT,
        1, -40, 161, IT_CREAM_MEAD, 1, 5, 161, IT_EMBER_DRAUGHT, 1, 45, 161, IT_TANKARD,  -1
        };
    for (i = 0; KShelf[i * 4] != -1; i++)
        {
        const int* e = KShelf + i * 4;
        int x = e[0] ? 100 + e[1] : -40 - e[1], z = e[0] ? -432 : 432;
        AddProp(PT_ITEM, cs, Ix(x), IN_FLOOR + Cm(e[2]) + iItemRest[GameItem(e[3]).mesh * 3 + 1], Iz(z), i * 140, e[3], 1, PF_OWNED);
        }
    // furniture you can open: x, z (cm), yaw, type, then up to three items and counts
    static const int KFurn[] =
        {
        560, 40, 0, PT_CHEST,        IT_FUR_BOOTS, 1, IT_WOLF_PELT, 2, IT_BOOK_NIGHTTEARS, 1,
        420, -60, 512, PT_CASE,      IT_IRON_DAGGER, 1, IT_SILVER_RING, 1, -1, 0,
        730, 100, 256, PT_DRESSER,   IT_SILVER_NECKLACE, 1, IT_FUR_GAUNTLETS, 1, -1, 0,
        300, -420, 512, PT_DRESSER,  IT_BOOK_DRAGONBORN, 1, IT_GARNET, 1, IT_BREAD, 1,
        -300, 420, 0, PT_CUPBOARD,   IT_FUR_ARMOR, 1, IT_FUR_HELMET, 1, IT_SALT, 2,
        -1
        };
    for (i = 0; KFurn[i * 10] != -1; i++)
        {
        const int* e = KFurn + i * 10;
        int c = NewCont(), k;
        for (k = 0; k < 3; k++) if (e[4 + k * 2] >= 0) ContAdd(c, e[4 + k * 2], e[5 + k * 2]);
        AddProp(e[3], cs, Ix(e[0]), IN_FLOOR, Iz(e[1]), e[2], c, 0, 0);
        }
    // beds: double, single and a heap of pelts (item = which bed)
    AddProp(PT_BED, cs, Ix(560), IN_FLOOR, Iz(335), 0, 0, 0, 0);
    AddProp(PT_BED, cs, Ix(620), IN_FLOOR, Iz(-330), 256, 1, 0, 0);
    AddProp(PT_BED, cs, Ix(-545), IN_FLOOR, Iz(-120), 0, 2, 0, 0);
    // the bedroom doors, hinged on the partition, standing open
    AddProp(PT_DOOR, cs, Ix(210), IN_FLOOR, Iz(150), 0, 0, 0, 0);
    AddProp(PT_DOOR, cs, Ix(210), IN_FLOOR, Iz(-260), 0, 0, 0, 0);
    for (i = 0; i < iS.nprop; i++) if (iS.prop[i].type == PT_DOOR) iS.prop[i].anim = 4;
    }
    // ---- the Hall of the Dead: benches in the three aisles (all facing north, the chapel's towards the shrine)
    int ch = CELL_HALLDEAD;
    static const int KHBench[6 * 2] = { 0, 90,  0, -130,  400, 260,  400, 40,  -400, 20,  -400, -210 };
    for (i = 0; i < 6; i++) AddProp(PT_BENCH, ch, Ix(KHBench[i * 2]), IN_FLOOR, Iz(KHBench[i * 2 + 1]), 512, 0, 0, 0);
    // the priest's room: bed against the north wall, two wardrobes, a chest at the bed's foot, a chair at the table
    AddProp(PT_BED, ch, Ix(-1000), IN_FLOOR, Iz(585), 0, 1, 0, PF_OWNED);
    {
    static const int KPFurn[] =
        {
        -830, 672, 0, PT_CUPBOARD,   IT_FUR_BOOTS, 1, IT_BREAD, 1, -1, 0,
        -700, 672, 0, PT_CUPBOARD,   IT_SALT, 1, -1, 0, -1, 0,
        -1000, 440, 0, PT_CHEST,     IT_SILVER_RING, 1, IT_POT_MHEAL, 1, IT_GARNET, 1,
        -1
        };
    for (i = 0; KPFurn[i * 10] != -1; i++)
        {
        const int* e = KPFurn + i * 10;
        int c = NewCont(), k;
        for (k = 0; k < 3; k++) if (e[4 + k * 2] >= 0) ContAdd(c, e[4 + k * 2], e[5 + k * 2]);
        AddProp(e[3], ch, Ix(e[0]), IN_FLOOR, Iz(e[1]), e[2], c, 0, PF_OWNED);
        }
    }
    AddProp(PT_CHAIR, ch, Ix(-820), IN_FLOOR, Iz(290), 512, 0, 0, 0);
    AddProp(PT_ITEM, ch, Ix(-845), IN_FLOOR + Cm(78) + iItemRest[GameItem(IT_POT_MSTAM).mesh * 3 + 1], Iz(200), 0, IT_POT_MSTAM, 1, PF_OWNED);
    AddProp(PT_ITEM, ch, Ix(-800), IN_FLOOR + Cm(78) + iItemRest[GameItem(IT_BOOK_DRAGONBORN).mesh * 3 + 1], Iz(190), 100, IT_BOOK_DRAGONBORN, 1, PF_OWNED);
    // ---- the catacombs: the side room's door, shut (the third skeleton waits behind it); what lies about
    int cc = CELL_CATACOMBS;
    {
    int dp = AddProp(PT_DOOR, cc, Ix(-490), IN_FLOOR, Iz(60), 0, 0, 0, 0);
    if (dp >= 0) { iS.prop[dp].state = 1; iS.prop[dp].anim = 0; }
    }
    AddProp(PT_ITEM, cc, Ix(-900), IN_FLOOR + Cm(78) + iItemRest[GameItem(IT_POT_MHEAL).mesh * 3 + 1], Iz(260), 0, IT_POT_MHEAL, 1, 0);
    AddProp(PT_ITEM, cc, Ix(-860), IN_FLOOR + Cm(78) + iItemRest[GameItem(IT_TANKARD).mesh * 3 + 1], Iz(-150), 300, IT_TANKARD, 1, 0);
    // ---- the Bitterroot: the alchemy lab, things to open, chairs, the cooking pot upstairs, goods on display
    {
    int cp = CELL_SHOP, U = Cm(320);
    AddProp(PT_ALCHEMY, cp, Ix(300), IN_FLOOR, Iz(170), 0, 0, 0, PF_OWNED);
    static const int KSFurn[] =
        {
        // x, z (cm), floor (0 / 1 upstairs), yaw, type, then up to three items and counts
        -250, 40, 0, 0, PT_CHEST,        IT_POT_MMAG, 1, IT_AMETHYST, 1, IT_BLISTERWORT, 2, // the storeroom's chest
        -250, 340, 0, 0, PT_BARREL,      IT_APPLE, 3, IT_CABBAGE, 2, -1, 0,                // a food barrel
        -160, 345, 0, 300, PT_BARREL,    IT_SALMON, 3, -1, 0, -1, 0,                       // fish barrels
        -70, 350, 0, 600, PT_BARREL,     IT_SALMON, 2, -1, 0, -1, 0,
        30, 270, 1, 256, PT_CUPBOARD,    IT_FUR_BOOTS, 1, -1, 0, -1, 0,                    // upstairs: two wardrobes
        30, 410, 1, 256, PT_CUPBOARD,    IT_FUR_HELMET, 1, IT_SALT, 1, -1, 0,
        440, -322, 1, 512, PT_CUPBOARD,  IT_SILVER_NECKLACE, 1, IT_POT_HEAL, 1, -1, 0,     // her room: wardrobe, strongbox
        300, -300, 1, 0, PT_CHEST,       IT_GOLD_RING, 1, IT_POT_CURE, 1, IT_GARNET, 1,
        -1
        };
    for (i = 0; KSFurn[i * 11] != -1; i++)
        {
        const int* e = KSFurn + i * 11;
        int c = NewCont(), k;
        for (k = 0; k < 3; k++) if (e[5 + k * 2] >= 0) ContAdd(c, e[5 + k * 2], e[6 + k * 2]);
        AddProp(e[4], cp, Ix(e[0]), IN_FLOOR + (e[2] ? U : 0), Iz(e[1]), e[3], c, 0, PF_OWNED);
        }
    {
    int c = NewCont(); ContAdd(c, IT_WHEAT, 3); ContAdd(c, IT_POTATO, 2);
    AddProp(PT_SACK, cp, Ix(950), IN_FLOOR, Iz(60), 100, c, 0, PF_OWNED);
    c = NewCont(); ContAdd(c, IT_BREAD, 1); ContAdd(c, IT_LEEK, 2);
    AddProp(PT_SACK, cp, Ix(70), IN_FLOOR + U, Iz(760), 0, c, 0, PF_OWNED);
    }
    AddProp(PT_BED, cp, Ix(110), IN_FLOOR + U, Iz(-230), 768, 1, 0, PF_OWNED);
    AddProp(PT_POT, cp, Ix(200), IN_FLOOR + U, Iz(830), 0, 0, 0, 0);
    AddProp(PT_CHAIR, cp, Ix(560), IN_FLOOR, Iz(-160), 512, 0, 0, 0);
    AddProp(PT_CHAIR, cp, Ix(160), IN_FLOOR, Iz(822), 0, 0, 0, 0);
    AddProp(PT_CHAIR, cp, Ix(68), IN_FLOOR, Iz(740), 768, 0, 0, 0);
    AddProp(PT_CHAIR, cp, Ix(380), IN_FLOOR + U, Iz(-40), 512, 0, 0, 0);
    // goods on display, hers (to buy, or to steal): on the desk, the shelves behind it and by the west wall, the table
    static const int KDisp[] =
        {
        // x, z (cm), surface height (cm), item, yaw
        // the long counter: bowls of mushrooms and roots, salts, pinned dragonflies, a few potions
        850, 250, 100, IT_WOOD_BOWL, 0,   850, 250, 104, IT_IMP_CAP, 0,       845, 300, 100, IT_EMBER_SALTS, 0,  860, 345, 100, IT_BLUE_FLY, 200,
        835, 385, 100, IT_POT_MHEAL, 0,   865, 410, 100, IT_SHADOW_SALTS, 0,  850, 455, 100, IT_WOOD_BOWL, 0,    850, 455, 104, IT_WHITE_CAP, 300,
        840, 500, 100, IT_EMBER_FLY, 100, 865, 535, 100, IT_GRAVEBELL, 0,     845, 575, 100, IT_POT_HEAL, 0,     865, 600, 100, IT_SPIDER_EGG, 0,
        // the short arm: a bowl of berries, rock moss, a stamina draught
        700, 205, 100, IT_WOOD_BOWL, 0,   700, 205, 104, IT_JUNIPER, 0,       755, 210, 100, IT_ROCK_MOSS, 0,    790, 195, 100, IT_POT_MSTAM, 0,
        // the shelves behind her: potions and essences below, dried things above
        980, 250, 106, IT_POT_RESFIRE, 0, 980, 300, 106, IT_FROST_VENOM, 0,   980, 350, 106, IT_GHOST_ESSENCE, 0,
        980, 255, 161, IT_TREANT_SAP, 0,  980, 300, 161, IT_WISP_DUST, 0,     980, 345, 161, IT_TROLL_FAT, 0,
        980, 555, 106, IT_TAPROOT, 256,   980, 600, 106, IT_HOUNDROOT, 256,   980, 645, 106, IT_CLAY_POT, 0,
        980, 555, 161, IT_CRONE_FEATHERS, 0, 980, 600, 161, IT_PIKE_SCALES, 0, 980, 645, 161, IT_WRAITH_TEETH, 0,
        // the hutch: bottles below, eggs and dried things above
        700, 20, 96, IT_POT_MMAG, 0,      735, 20, 96, IT_POISON_WEAK, 0,
        690, 20, 155, IT_WARBLER_EGG, 0,  735, 18, 155, IT_RAT_TAIL, 256,     775, 20, 155, IT_BLISTERWORT, 0,
        // the south-west shelves: books, cheese, bread, lavender, bone meal
        25, 100, 106, IT_CHEESE, 0,       25, 150, 106, IT_BREAD, 300,        25, 120, 161, IT_BOOK_NIGHTTEARS, 256,  25, 80, 161, IT_LAVENDER, 0,
        25, 170, 161, IT_BONE_MEAL, 0,
        // the study's desk: a bowl with two green apples, the herbwife's book
        540, -230, 78, IT_WOOD_BOWL, 0,   535, -235, 82, IT_APPLE, 0,         548, -222, 82, IT_APPLE, 300,      590, -210, 78, IT_BOOK_HERBS, 100,
        // the table by the fire; upstairs, the kitchen corner and her room
        160, 740, 78, IT_TANKARD, 0,
        60, 790, 398, IT_BREAD, 0,        100, 800, 398, IT_BERRY_WINE, 0,    120, 780, 398, IT_VENISON, 0,
        360, -120, 398, IT_BOOK_DRAGONBORN, 0, -1
        };
    for (i = 0; KDisp[i * 5] != -1; i++)
        {
        const int* e = KDisp + i * 5;
        AddProp(PT_ITEM, cp, Ix(e[0]), IN_FLOOR + Cm(e[2]) + iItemRest[GameItem(e[3]).mesh * 3 + 1], Iz(e[1]), e[4], e[3], 1, PF_OWNED);
        }
    }
    // ---- the Gilded Stag
    {
    int ci = CELL_INN, U = Cm(320);
    // seats: chairs at the tables (each facing its table), the benches at the fire, the bard's chair, stools at the bar
    static const int KSeat[] =
        {
        // x, z (cm), floor (1 upstairs), the way the sitter faces (yaw), type
        170, 700, 0, 512, PT_CHAIR,    250, 620, 0, 768, PT_CHAIR,    170, 540, 0, 0, PT_CHAIR,        // by the door (south)
        170, 1040, 0, 0, PT_CHAIR,     250, 1120, 0, 768, PT_CHAIR,   170, 1200, 0, 512, PT_CHAIR,     // by the door (north)
        410, 1160, 0, 768, PT_CHAIR,   330, 1080, 0, 0, PT_CHAIR,                                      // north wall table
        410, 580, 0, 768, PT_CHAIR,    330, 660, 0, 512, PT_CHAIR,                                     // south wall table
        850, 580, 0, 256, PT_CHAIR,    930, 650, 0, 512, PT_CHAIR,                                     // the corner table
        760, 990, 0, 512, PT_BENCH,    760, 750, 0, 0, PT_BENCH,      940, 870, 0, 768, PT_CHAIR,      // at the fire
        690, 330, 0, 512, PT_CHAIR,    830, 330, 0, 512, PT_CHAIR,                                     // at the bar
        620, 2230, 0, 512, PT_CHAIR,   700, 2300, 0, 768, PT_CHAIR,                                    // kitchen table
        1540, 1130, 0, 256, PT_CHAIR,                                                                  // back room
        650, 2150, 1, 768, PT_CHAIR,                                                                   // the maid's room
        700, 1400, 1, 0, PT_CHAIR,                                                                     // the innkeeper's room
        -1
        };
    for (i = 0; KSeat[i * 5] != -1; i++)
        {
        const int* e = KSeat + i * 5;
        AddProp(e[4], ci, Ix(e[0]), IN_FLOOR + (e[2] ? U : 0), Iz(e[1]), (e[3] + 512) & 1023, 0, 0, 0);
        }
    // things to open: barrels and a chest by the west wall, the kitchen's, the back room's, upstairs
    static const int KIFurn[] =
        {
        // x, z (cm), floor, yaw, type, then up to three items and counts
        40, 1240 - 60, 0, 0, PT_BARREL,     IT_APPLE, 4, IT_CABBAGE, 2, -1, 0,
        40, 1010, 0, 0, PT_BARREL,          IT_POTATO, 4, IT_LEEK, 2, -1, 0,
        40, 1095, 0, 256, PT_CHEST,         IT_POT_MHEAL, 1, IT_IRON_DAGGER, 1, IT_TANKARD, 1,
        980, 1290, 0, 0, PT_BARREL,         IT_SALMON, 3, IT_SALT, 1, -1, 0,                 // kitchen: food barrel
        995, 1420, 0, 768, PT_CUPBOARD,     IT_CREAM_MEAD, 1, IT_BERRY_WINE, 2, IT_JUG, 1,   // kitchen cupboard
        820, 1300, 0, 0, PT_CHEST,          IT_GARNET, 1, IT_SILVER_RING, 1, -1, 0,          // the strongbox by the ledger
        960, 2050, 0, 0, PT_BARREL,         IT_RAW_BEEF, 2, IT_POTATO, 3, -1, 0,             // pantry
        960, 2140, 0, 300, PT_BARREL,       IT_CABBAGE, 3, -1, 0, -1, 0,
        1712, 900, 0, 768, PT_CUPBOARD,     IT_CLAY_POT, 1, IT_PLATE, 2, -1, 0,              // back room: cupboard, wardrobe
        1058, 1100, 0, 256, PT_CUPBOARD,    IT_FUR_BOOTS, 1, -1, 0, -1, 0,
        1172, 1170, 1, 768, PT_CUPBOARD,    IT_TANKARD, 1, IT_BREAD, 1, -1, 0,               // the landing's cupboard
        1700, 800, 1, 768, PT_DRESSER,      IT_SILVER_NECKLACE, 1, -1, 0, -1, 0,             // the rented room's dresser
        530, 1980, 1, 256, PT_CUPBOARD,     IT_SALT, 1, -1, 0, -1, 0,                        // upstairs landing: small wardrobe
        850, 1700, 1, 768, PT_DRESSER,      IT_BOOK_NIGHTTEARS, 1, IT_CLAY_POT, 1, -1, 0,         // the innkeeper's dresser
        700, 2372, 1, 0, PT_CUPBOARD,       IT_FUR_GAUNTLETS, 1, -1, 0, -1, 0,               // the maid's wardrobe
        950, 2130, 1, 0, PT_CHEST,          IT_POT_MSTAM, 1, IT_AMETHYST, 1, -1, 0,
        -1
        };
    for (i = 0; KIFurn[i * 11] != -1; i++)
        {
        const int* e = KIFurn + i * 11;
        int c = NewCont(), k;
        for (k = 0; k < 3; k++) if (e[5 + k * 2] >= 0) ContAdd(c, e[5 + k * 2], e[6 + k * 2]);
        AddProp(e[4], ci, Ix(e[0]), IN_FLOOR + (e[2] ? U : 0), Iz(e[1]), e[3], c, 0, PF_OWNED);
        }
    // beds: the back room's, the room for rent (count 1: it can be rented), the innkeeper's, the maid's
    AddProp(PT_BED, ci, Ix(1620), IN_FLOOR, Iz(590), 256, 1, 0, PF_OWNED);
    AddProp(PT_BED, ci, Ix(1140), IN_FLOOR + U, Iz(650), 768, 0, 1, PF_OWNED);
    AddProp(PT_BED, ci, Ix(770), IN_FLOOR + U, Iz(1360), 256, 1, 0, PF_OWNED);
    AddProp(PT_BED, ci, Ix(950), IN_FLOOR + U, Iz(2290), 0, 1, 0, PF_OWNED);
    AddProp(PT_POT, ci, Ix(690), IN_FLOOR, Iz(1620), 0, 0, 0, 0);
    // on the counter and the tables: the inn's (to buy or steal) and the guests' plates and cups
    static const int KIDisp[] =
        {
        600, 230, 100, IT_BREAD, 0,       640, 215, 100, IT_CHEESE, 100,   700, 240, 100, IT_CREAM_MEAD, 0,   740, 220, 100, IT_CREAM_MEAD, 0,
        800, 235, 100, IT_BERRY_WINE, 0,  860, 220, 100, IT_APPLE, 0,      880, 240, 100, IT_APPLE, 300,      920, 225, 100, IT_TANKARD, 0,
        620, 20, 106, IT_JUG, 0,          660, 20, 106, IT_EMBER_DRAUGHT, 0, 900, 20, 106, IT_PLATE, 0,       940, 20, 161, IT_CLAY_POT, 0,
        170, 620, 78, IT_TANKARD, 0,      150, 610, 78, IT_PLATE, 0,       160, 1120, 78, IT_WOOD_BOWL, 0,    185, 1130, 78, IT_TANKARD, 200,
        330, 1160, 78, IT_BREAD, 0,       340, 580, 78, IT_PLATE, 0,       920, 580, 78, IT_TANKARD, 0,
        985, 950, 76, IT_IRON_SWORD, 256, 985, 1050, 76, IT_CHEESE, 0,
        930, 1330, 78, IT_BOOK_DRAGONBORN, 0, 620, 2300, 78, IT_PLATE, 0,      640, 2290, 82, IT_SALMON_STEAK, 0,
        -1
        };
    for (i = 0; KIDisp[i * 5] != -1; i++)
        {
        const int* e = KIDisp + i * 5;
        AddProp(PT_ITEM, ci, Ix(e[0]), IN_FLOOR + Cm(e[2]) + iItemRest[GameItem(e[3]).mesh * 3 + 1], Iz(e[1]), e[4], e[3], 1, PF_OWNED);
        }
    }
    // a gem in the alcove at the foot of the west stairs (Whiterun's has a Stone of Barenziah there)
    AddProp(PT_ITEM, cc, Ix(-700), IN_FLOOR - Cm(300) + iItemRest[GameItem(IT_AMETHYST).mesh * 3 + 1], Iz(600), 0, IT_AMETHYST, 1, 0);
    }

// ---------------------------------------------------------------- base objects
// Every placed prop is a reference to a base object, as in Skyrim: an item lying about (MISC/ALCH/...),
// flora you harvest (FLOR), a container (CONT), furniture with a marker for sitting, sleeping or crafting
// (FURN), a door (DOOR). The base decides the name, the verb, the model, the footprint and what E does.
struct BaseDef
    {
    int kind, marker, mesh;             // BO_*, FM_* (furniture), IN_* model
    const char* name; const char* verb;
    int focusY, focusR;                 // where the crosshair finds it (cm above / radius)
    int hx, hz, top;                    // box footprint (cm) for walking and setting things down; 0 = none
    };
static const BaseDef KBase[PT_COUNT] =
    {
    { BO_NONE,  0,        0,           "",             "",        60, 38,   0,   0,   0 },
    { BO_ITEM,  0,        0,           "",             "Take",     4, 14,   0,   0,   0 },
    { BO_FLORA, 0,        IN_GARLIC,   "Garlic Braid", "Harvest", -62, 16,  0,   0,   0 },
    { BO_FLORA, 0,        IN_FISH,     "Salmon",       "Take",    -72, 16,  0,   0,   0 },
    { BO_CONT,  0,        IN_BARREL,   "Barrel",       "Search",  50, 42,   0,   0,   0 },
    { BO_CONT,  0,        IN_SACK,     "Sack",         "Search",  35, 36,   0,   0,   0 },
    { BO_FURN,  FM_SIT,   IN_CHAIR,    "Chair",        "Sit",     55, 36,   0,   0,   0 },
    { BO_FURN,  FM_CRAFT, IN_POT,      "Cooking Pot",  "Cook",    60, 38,   0,   0,   0 },
    { BO_FURN,  FM_SIT,   IN_BENCH,    "Bench",        "Sit",     40, 60,   0,   0,   0 },
    { BO_CONT,  0,        IN_CHEST,    "Chest",        "Open",    45, 50,  69,  35,  68 },
    { BO_CONT,  0,        IN_DISPLAY,  "Display Case", "Open",    90, 45,  56,  26, 103 },
    { BO_CONT,  0,        IN_DRESSER,  "Dresser",      "Search",  75, 48,  58,  28,  91 },
    { BO_CONT,  0,        IN_CUPBOARD, "Wardrobe",     "Open",   110, 55,  64,  29, 198 },
    { BO_FURN,  FM_SLEEP, IN_BED2,     "Double Bed",   "Sleep",   45, 70,  84, 110,  52 },
    { BO_DOOR,  0,        IN_DOOR,     "Door",         "Open",   110, 50,   0,   0,   0 },
    { BO_STAT,  0,        0,           "Drawbridge",   "",         0,  0,   0,   0,   0 },
    { BO_ACTI,  0,        0,           "Lever",        "Pull",    90, 40,   0,   0,   0 },
    { BO_CONT,  0,        0,           "Crate",        "Search",  50, 40,  45,  35,  90 },
    { BO_FURN,  FM_CRAFT, IN_ALCHLAB,  "Alchemy Lab",  "Use",     95, 60,  56,  56,  88 },
    { BO_FLORA, 0,        0,           "Rabbit",       "Harvest", -25, 35,  0,   0,   0 },
    // Bleak Falls Barrow
    { BO_ACTI,  0,        IN_PUZPILLAR, "Pillar",      "Rotate", 120, 55,  42,  42, 210 },
    { BO_ACTI,  0,        IN_CHAIN,    "Pull Chain",   "Pull",   120, 45,   0,   0,   0 },
    { BO_ACTI,  0,        IN_CLAWDOOR, "Door",         "Examine", 150, 90,  0,   0,   0 },
    { BO_ACTI,  0,        IN_WORDWALL, "Word Wall",    "Examine", 200, 160, 0,   0,   0 },
    { BO_ACTI,  0,        0,           "Well",         "Climb in", 60, 90,   0,   0,   0 }      // (v0.4.69) Forelhost's well
    };
// the three beds share one base each in Skyrim; here the reference's item picks the variant
static const BaseDef KBed[2] =
    {
    { BO_FURN,  FM_SLEEP, IN_BED1,     "Bed",          "Sleep",   45, 70,  54, 110,  52 },
    { BO_FURN,  FM_SLEEP, IN_PELTBED,  "Bedroll",      "Sleep",   15, 70,   0,   0,   0 }    // walked over, as in Skyrim
    };
static const BaseDef& Base(const Prop& p)
    {
    if (p.type == PT_BED && p.item > 0) return KBed[(p.item - 1) & 1];
    return KBase[p.type < PT_COUNT ? p.type : 0];
    }
int GamePropKind(const Prop& p) { return Base(p).kind; }

// furniture footprints (cm): half width, half depth, top; 0 for things that are not boxes
static int Footprint(const Prop& p, int& hx, int& hz, int& top)
    {
    const BaseDef& b = Base(p);
    hx = b.hx; hz = b.hz; top = b.top;
    return b.hx > 0;
    }

// world offset -> the prop's own frame (the inverse of the mesh rotation)
static void ToLocal(const Prop& p, int aX, int aZ, int& lx, int& lz)
    {
    int cs = r3d_cos(p.yaw), sn = r3d_sin(p.yaw);
    int dx = aX - p.x, dz = aZ - p.z;
    lx = (dx * cs - dz * sn) >> 14;
    lz = (dx * sn + dz * cs) >> 14;
    }

// a door leaf: the segment from the hinge to (108 cm along its swing); distance from (lx, lz)
static int DoorDist(const Prop& p, int lx, int lz)
    {
    int a = Clamp(p.anim, 0, 4) * 64;
    int ux = r3d_sin(a), uz = r3d_cos(a);
    int t = (lx * ux + lz * uz) >> 14;
    t = Clamp(t, 0, p.count > 0 ? Cm(p.count) : Cm(108));        // (v0.4.74: a ruin's door: its own width)
    int ex = lx - ((ux * t) >> 14), ez = lz - ((uz * t) >> 14);
    return Abs(ex) > Abs(ez) ? Abs(ex) + Abs(ez) / 2 : Abs(ez) + Abs(ex) / 2;
    }

// ---------------------------------------------------------------- world queries
// the highest surface under (x, z) at or below y + a little: floor, table tops, barrels
// the lowered drawbridge's deck is walked on (from above: wading under it in the stream is not lifted)
int Game::BridgeFloor(int aX, int aZ, int aFeetY)
    {
    if (iBridgeProp < 0 || iBridgeProp >= iS.nprop) return -(1 << 30);
    const Prop& p = iS.prop[iBridgeProp];
    if (p.anim > 1 || aFeetY < p.y - M) return -(1 << 30);
    int lx, lz;
    ToLocal(p, aX, aZ, lx, lz);
    if (Abs(lx) < 23 * M / 10 && lz < 3 * M / 2 && lz > -10 * M - M / 5) return p.y;     // deck (and its abutment)
    return -(1 << 30);
    }

int Game::SurfaceAt(int aCell, int aX, int aZ, int aY)
    {
    int s = FloorAt(aCell, aX, aZ, aY);
    if (aCell == 0)
        {
        // the open shaft of the well: what falls in goes down
        int wx = aX - iWellX, wz = aZ - iWellZ, ri = 85 * M / 100;
        if (Abs(wx) < ri && Abs(wz) < ri && (wx >> 4) * (wx >> 4) + (wz >> 4) * (wz >> 4) < (ri >> 4) * (ri >> 4)) return s - 4 * M;
        }
    int save = iR->iCell;
    iR->iCell = aCell;
    int t = iR->TopAt(aX, aZ, aY + 30);
    iR->iCell = save;
    if (t > s) s = t;
    int i;
    for (i = 0; i < iS.nprop; i++)
        {
        const Prop& p = iS.prop[i];
        if (p.cell != aCell || (p.flags & PF_GONE)) continue;
        int hx, hz, tp;
        if (Footprint(p, hx, hz, tp))
            {
            int lx, lz;
            ToLocal(p, aX, aZ, lx, lz);
            int top = p.y + Cm(tp);
            if (Abs(lx) < Cm(hx) && Abs(lz) < Cm(hz) && top <= aY + 30 && top > s) s = top;
            continue;
            }
        if (p.type != PT_BARREL && p.type != PT_SACK) continue;
        int dx = aX - p.x, dz = aZ - p.z, r = Cm(30);
        if (Abs(dx) < r && Abs(dz) < r)
            {
            int top = p.y + (p.type == PT_BARREL ? Cm(92) : Cm(62));
            if (top <= aY + 30 && top > s) s = top;
            }
        }
    return s;
    }

int Game::PropBlocked(int aCell, int aX, int aZ, int aR, int aFeetY)
    {
    int i;
    for (i = 0; i < iS.nprop; i++)
        {
        const Prop& p = iS.prop[i];
        if (p.cell != aCell || (p.flags & PF_GONE)) continue;
        // on another floor (a gallery above, the room below): not in the way
        if (aFeetY != -0x7FFFFFFF && (p.y > aFeetY + Cm(150) || p.y + Cm(220) < aFeetY)) continue;
        if (Abs(aX - p.x) > 3 * M || Abs(aZ - p.z) > 3 * M) continue;
        int hx, hz, tp, lx, lz;
        if (Footprint(p, hx, hz, tp))
            {
            if (tp && aFeetY != -0x7FFFFFFF && aFeetY > p.y + Cm(tp) - Cm(10)) continue;    // over it (stairs above a barrel)
            ToLocal(p, aX, aZ, lx, lz);
            if (Abs(lx) < Cm(hx) + aR && Abs(lz) < Cm(hz) + aR) return 1;
            continue;
            }
        if (p.type == PT_DOOR)
            {
            ToLocal(p, aX, aZ, lx, lz);
            if (DoorDist(p, lx, lz) < Cm(5) + aR) return 1;
            continue;
            }
        if (p.type == PT_BRIDGE)
            {
            // raised, the deck stands as a wall before the gate
            if (p.anim < 2) continue;
            ToLocal(p, aX, aZ, lx, lz);
            if (Abs(lx) < 12 * M / 5 + aR && lz > -M - aR && lz < M / 5 + aR) return 1;
            continue;
            }
        if (p.type != PT_BARREL && p.type != PT_SACK) continue;
        if (aFeetY != -0x7FFFFFFF && aFeetY > p.y + Cm(p.type == PT_BARREL ? 85 : 50)) continue;      // over its top
        int r = Cm(36) + aR, dx = aX - p.x, dz = aZ - p.z;
        if (Abs(dx) < r && Abs(dz) < r && (dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4) < (r >> 4) * (r >> 4)) return 1;
        }
    return 0;
    }

// aim: the prop whose centre lies nearest the view ray, within reach
static void PropCentre(const Prop& p, int& x, int& y, int& z, int& r)
    {
    const BaseDef& b = Base(p);
    x = p.x; y = p.y + Cm(b.focusY); z = p.z; r = Cm(b.focusR);
    if (b.kind == BO_DOOR)
        {
        // the middle of the leaf, wherever it has swung to
        int a = Clamp(p.anim, 0, 4) * 64;
        int half = p.type == PT_DOOR && p.count > 0 ? Cm(p.count / 2) : Cm(54);
        int lx = (r3d_sin(a) * half) >> 14, lz = (r3d_cos(a) * half) >> 14;
        int cs = r3d_cos(p.yaw), sn = r3d_sin(p.yaw);
        x += (lx * cs + lz * sn) >> 14; z += (-lx * sn + lz * cs) >> 14;
        }
    }

void Game::FindPropFocus()
    {
    if (iGrab >= 0 || iSitting >= 0) return;
    int cp = r3d_cos(iS.pitch), sp = r3d_sin(iS.pitch);
    int dx = (r3d_sin(iS.yaw) * cp) >> 14, dz = (r3d_cos(iS.yaw) * cp) >> 14, dy = sp;
    int best = -1, bestT = 3 * M;
    int i;
    for (i = 0; i < iS.nprop; i++)
        {
        const Prop& p = iS.prop[i];
        if (p.cell != iS.cell || (p.flags & PF_GONE) || Base(p).kind == BO_STAT) continue;
        int x, y, z, r;
        PropCentre(p, x, y, z, r);
        int vx = x - iS.px, vy = y - iS.py, vz = z - iS.pz;
        if (Abs(vx) > 3 * M || Abs(vz) > 3 * M) continue;
        int t = (vx * dx + vy * dy + vz * dz) >> 14;
        if (t < Cm(20) || t > Cm(260)) continue;
        int px = vx - ((dx * t) >> 14), py = vy - ((dy * t) >> 14), pz = vz - ((dz * t) >> 14);
        int rr = r + t / 14, d2 = px * px + py * py + pz * pz;
        if (d2 > rr * rr) continue;
        // (v0.4.74) the nearest along the view, but one only grazed counts up to a metre farther: the note aimed at
        // beats the alchemy table under it
        int sc = t + (d2 >> 4) * (Cm(100) >> 4) / (((rr * rr) >> 8) + 1);              // (d2 <= rr*rr: within 32 bits)
        if (sc >= bestT) continue;
        best = i; bestT = sc;
        }
    if (best < 0) return;
    // an entity straight ahead and nearer keeps the focus
    if (iFocusKind == 1)
        {
        const Ent& e = iS.e[iFocus];
        int ex = e.x - iS.px, ez = e.z - iS.pz;
        if (Abs(ex) + Abs(ez) < bestT) return;
        }
    iFocusKind = 3; iFocus = best;
    }

// ---------------------------------------------------------------- using things
void Game::TakeProp(int i)
    {
    Prop& p = iS.prop[i];
    if (p.type != PT_ITEM || (p.flags & PF_GONE)) return;
    AddItem(p.item, p.count > 0 ? p.count : 1);
    p.flags |= PF_GONE;
    if (p.flags & PF_OWNED) Msg2(GameItem(p.item).name, " stolen");
    else Msg2(GameItem(p.item).name, " added");
    }

void Game::UseProp(int i)
    {
    if (UseDunProp(i)) return;
    Prop& p = iS.prop[i];
    const BaseDef& b = Base(p);
    if (b.kind == BO_CONT)
        {
        iOpenCont = p.item; iOpenProp = i;
        // lids, drawers and doors are seen opening before the menu comes up
        if (p.type == PT_SACK || p.type == PT_CASE) OpenItemMenu(UI_CONT);
        else iOpenWaitUs = 450000;
        return;
        }
    if (b.kind == BO_FURN && b.marker == FM_SLEEP)
        {
        // someone else's bed: only the inn's room for rent, paid for the day (10 gold, as in Skyrim's inns)
        if (p.flags & PF_OWNED)
            {
            int nowH = iS.day * 24 + iS.timeOfDay / 60000;
            if (p.count != 1) { Msg("That bed belongs to someone else."); return; }
            if (iS.spare[0] <= nowH)
                {
                // in Riverwood, Delphine's own words (DelphineRentRoomTopic)
                if (iS.gold < 10)
                    {
                    if (p.cell == CELL_RWINN) { Msg("Delphine: What does this look like, the Temple"); Msg("of Mara? No gold, no bed."); }
                    else Msg("The room is 10 gold a night.");
                    return;
                    }
                iS.gold -= 10; iS.spare[0] = nowH + 24;
                if (p.cell == CELL_RWINN) { Msg("Delphine: Sure thing. It's yours for a day."); Msg("10 gold removed"); }
                else Msg("You rent the room for 10 gold.");
                }
            }
        StartWait();
        if (iUi == UI_WAIT) { iSleeping = 1; if (iWaitSel < 8) iWaitSel = 8; }
        return;
        }
    switch (p.type)
        {
        case PT_ITEM:
            // a tap takes it, holding E picks it up to carry (decided in UpdateProps)
            iUsePending = i; iUseHeldUs = 0;
            break;
        case PT_GARLIC:
            if (p.state >= 2) { Msg("Nothing left to harvest."); break; }
            p.state++;
            AddItem(IT_GARLIC, 2);
            Msg("Garlic (2) added");
            break;
        case PT_HARVEST:
            {
            // the whole of it: the meat off the hook, the purse's coins (Skyrim's: small 6-23, medium 12-37, large 23-56)
            int stolen = (p.flags & PF_OWNED) != 0;
            p.flags |= PF_GONE;
            if (p.item < 0)
                {
                static const short KPurse[6] = { 6, 23, 12, 37, 23, 56 };
                int k = Clamp(p.count, 0, 2), lo = KPurse[k * 2], hi = KPurse[k * 2 + 1];
                int g = lo + (int)(r3d_hash(iTimeUs, i, 51) % (u32)(hi - lo + 1));
                iS.gold += g;
                char b[40]; char* q = r3d_puti(b, g); r3d_puts(q, stolen ? " gold stolen" : " gold added");
                Msg(b);
                }
            else { AddItem(p.item, 1); Msg2(GameItem(p.item).name, stolen ? " stolen" : " added"); }
            break;
            }
        case PT_FISH:
            {
            int id = p.item ? p.item : IT_SALMON;
            p.flags |= PF_GONE;
            AddItem(id, 1);
            Msg2(GameItem(id).name, (p.flags & PF_OWNED) && p.cell == 0 ? " stolen" : " added");
            break;
            }
        case PT_DOOR:
            if (p.state == 0)
                {
                // closing: not onto someone standing in the doorway
                int a = p.anim;
                p.anim = 0;
                int lx, lz;
                ToLocal(p, iS.px, iS.pz, lx, lz);
                int hit = DoorDist(p, lx, lz) < Cm(5) + 100;
                p.anim = (short)a;
                if (hit) { Msg("Something is in the way."); break; }
                }
            p.state ^= 1;
            break;
        case PT_CHAIR: case PT_BENCH:
            {
            // sit facing out of the seat (local -z); on a bench take the nearest of three places
            int cs = r3d_cos(p.yaw), sn = r3d_sin(p.yaw);
            int lx = 0;
            if (p.type == PT_BENCH)
                {
                int dx = iS.px - p.x, dz = iS.pz - p.z;
                lx = (dx * cs - dz * sn) >> 14;
                lx = lx < -Cm(30) ? -Cm(60) : (lx > Cm(30) ? Cm(60) : 0);
                }
            iSitting = i;
            iSitX = p.x + ((lx * cs) >> 14); iSitZ = p.z + ((-lx * sn) >> 14);
            iSitY = p.y;
            iSitYaw = (p.yaw + 512) & 1023;
            iS.px = iSitX; iS.pz = iSitZ;
            iS.yaw = iSitYaw; iS.pitch = 0;
            iS.py = p.y + Cm(118);
            iS.vy = 0;
            break;
            }
        case PT_POT: case PT_ALCHEMY:
            iListSel = 0; iListScroll = 0;
            iCookMode = p.type == PT_ALCHEMY;
            iUi = UI_COOK;
            break;
        case PT_LEVER:
            if (iBridgeProp >= 0)
                {
                Prop& b2 = iS.prop[iBridgeProp];
                b2.state ^= 1; p.state = b2.state;
                Msg(b2.state ? "The drawbridge rises." : "The drawbridge comes down.");
                }
            break;
        }
    }

// stand up beside the seat: try in front, to either side, then behind, and take the first clear spot
void Game::StandUp()
    {
    if (iSitting < 0) return;
    int fx = r3d_sin(iSitYaw), fz = r3d_cos(iSitYaw);       // the way the sitter faces
    static const int KTry[6 * 2] = { 0, 65,  -75, 0,  75, 0,  -75, 30,  75, 30,  0, -65 };   // (side, forward) cm
    int k, x = iSitX, z = iSitZ;
    for (k = 0; k < 6; k++)
        {
        int sd = Cm(KTry[k * 2]), fw = Cm(KTry[k * 2 + 1]);
        int tx = iSitX + ((fx * fw + fz * sd) >> 14), tz = iSitZ + ((fz * fw - fx * sd) >> 14);
        int feet = FloorAt(iS.cell, tx, tz, iSitY);
        if (!BlockedAt(iS.cell, tx, tz, 100, feet)) { x = tx; z = tz; break; }
        }
    iS.px = x; iS.pz = z;
    iS.py = FloorAt(iS.cell, x, z, iSitY) + 410;
    iSitting = -1;
    }

void Game::UpdateProps(int dt)
    {
    int i;
    if (iOpenWaitUs > 0)
        {
        iOpenWaitUs -= dt;
        if (iOpenProp >= 0)
            {
            const Prop& op = iS.prop[iOpenProp];
            if (op.cell != iS.cell || Abs(op.x - iS.px) > 3 * M || Abs(op.z - iS.pz) > 3 * M) { iOpenWaitUs = 0; iOpenProp = -1; }
            }
        if (iOpenWaitUs <= 0) { iOpenWaitUs = 0; if (iOpenProp >= 0 && iUi == UI_PLAY) OpenItemMenu(UI_CONT); }
        }
    if (!(iKeys & KEY_FIRE)) iFireLock = 0;
    // take or pick up
    if (iUsePending >= 0)
        {
        if (iKeys & KEY_USE)
            {
            iUseHeldUs += dt;
            if (iUseHeldUs >= 350000)
                {
                // picked up: it hops up a little and drifts to the crosshair
                iGrab = iUsePending; iUsePending = -1; iGrabArmed = 0;
                Prop& g = iS.prop[iGrab];
                g.flags &= ~PF_FALL; g.y += Cm(6);
                }
            }
        else
            {
            // a book is opened to read where it lies (it can be taken from the open book); anything else is taken
            const Prop& bp = iS.prop[iUsePending];
            if (bp.type == PT_ITEM && GameItem(bp.item).cat == IC_BOOK)
                { iBookId = GameItem(bp.item).book; iBookPage = 0; iBookProp = iUsePending; iUi = UI_BOOK; }
            else TakeProp(iUsePending);
            iUsePending = -1;
            }
        }
    // carried: floats at the crosshair until E is pressed again (or a menu opens)
    if (iGrab >= 0)
        {
        Prop& p = iS.prop[iGrab];
        if (!(iKeys & KEY_USE)) iGrabArmed = 1;
        if (iUi != UI_PLAY || p.cell != iS.cell)
            {
            p.flags |= PF_FALL; p.vy = 0;
            iGrab = -1;
            }
        else
            {
            int cp = r3d_cos(iS.pitch), sp = r3d_sin(iS.pitch);
            int d = Cm(95);
            int x = iS.px + ((((r3d_sin(iS.yaw) * cp) >> 14) * d) >> 14);
            int z = iS.pz + ((((r3d_cos(iS.yaw) * cp) >> 14) * d) >> 14);
            int y = iS.py + ((sp * d) >> 14) - Cm(5);     // centred on the crosshair
            if (Pressed(KEY_FIRE))
                {
                // thrown: off along the view, a little upwards
                int sp2 = Cm(500);
                p.vx = (((r3d_sin(iS.yaw) * cp) >> 14) * sp2) >> 14;
                p.vz = (((r3d_cos(iS.yaw) * cp) >> 14) * sp2) >> 14;
                p.vy = ((sp * sp2) >> 14) + Cm(200);
                p.flags |= PF_FALL;
                iGrab = -1; iFireLock = 1;
                return;
                }
            if (!BlockedAt(iS.cell, x, z, 12, y - Cm(10), Cm(20)))
                {
                int floor = SurfaceAt(iS.cell, x, z, y);
                if (y < floor) y = floor;
                // ease towards the hold point so the pick-up is seen, not a jump cut
                p.x += (x - p.x) / 2; p.z += (z - p.z) / 2; p.y += (y - p.y) / 2;
                }
            p.yaw = (short)((p.yaw + dt / 20000) & 1023);
            if (p.flags & PF_OWNED) { }      // carrying someone's goods is not yet theft
            }
        }
    int dtMs = dt / 1000;
    // lids, drawers and doors move a step every 90 ms, whatever the frame rate
    iPropTickUs += dt;
    int step = iPropTickUs >= 90000;
    if (step) iPropTickUs -= 90000;
    if (iPropTickUs > 90000) iPropTickUs = 0;
    for (i = 0; i < iS.nprop; i++)
        {
        Prop& p = iS.prop[i];
        if (p.flags & PF_FALL)
            {
            p.vy -= 2500 * dtMs / 1000;                       // ~9.8 m/s^2
            int ny = p.y + p.vy * dtMs / 1000;
            if (p.vx || p.vz)
                {
                int nx = p.x + p.vx * dtMs / 1000, nz = p.z + p.vz * dtMs / 1000;
                if (BlockedAt(p.cell, nx, nz, 12, ny - Cm(5), Cm(15))) { p.vx = -p.vx / 4; p.vz = -p.vz / 4; }
                else { p.x = nx; p.z = nz; }
                }
            int floor = SurfaceAt(p.cell, p.x, p.z, p.y);
            if (p.type == PT_ITEM) floor += iItemRest[GameItem(p.item).mesh * 3 + 1];
            if (ny <= floor) { ny = floor; p.vy = 0; p.vx = 0; p.vz = 0; p.flags &= ~PF_FALL; }
            p.y = ny;
            // down the well: gone with a splash
            if (p.cell == 0 && p.y < iR->GroundAt(p.x, p.z) - M)
                {
                p.flags = (u8)((p.flags | PF_GONE) & ~PF_FALL);
                if (Abs(p.x - iS.px) < 30 * M && Abs(p.z - iS.pz) < 30 * M) Msg("Splash!");
                }
            }
        int kind = Base(p).kind;
        if (step && (p.type == PT_BRIDGE || p.type == PT_LEVER))
            {
            // the bridge swings up or down a frame at a time (slowly: every other step)
            int want = p.state ? 4 : 0;
            if (p.anim < want) p.anim++; else if (p.anim > want) p.anim--;
            }
        if (step && ((kind == BO_CONT && p.type != PT_SACK && p.type != PT_CASE) || kind == BO_DOOR))
            {
            int want = p.type == PT_DOOR ? (p.state ? 0 : 4) : (((iUi == UI_CONT || iOpenWaitUs > 0) && iOpenProp == i) ? 4 : 0);
            if (p.anim < want) p.anim++;
            else if (p.anim > want) p.anim--;
            }
        }
    // leaving the chair
    if (iSitting >= 0 && (iKeys & (KEY_FWD | KEY_BACK | KEY_STRAFE_L | KEY_STRAFE_R | KEY_JUMP))) StandUp();
    }

// ---------------------------------------------------------------- drawing
void Game::DrawProps()
    {
    R3D& r = *iR;
    int i;
    for (i = 0; i < iS.nprop; i++)
        {
        const Prop& p = iS.prop[i];
        if (p.cell != iS.cell || (p.flags & (PF_GONE | PF_NODRAW))) continue;
        if (p.cell == CELL_RWINN && p.type != PT_DOOR && !((r.iZoneMask >> GameZoneOf(p.cell, p.x, p.z)) & 1)) continue;
        int dx = (p.x - r.iCamX) >> 8, dz = (p.z - r.iCamZ) >> 8;
        int lim = p.type == PT_ITEM ? 12 : 40;
        if (dx * dx + dz * dz > lim * lim) continue;
        if (p.type == PT_ITEM)
            {
            const ItemDef& d = GameItem(p.item);
            const u8* c = GameItemCol(p.item);
            u8 sl[12];
            int k;
            for (k = 0; k < 3; k++) { sl[k * 4] = c[k * 3]; sl[k * 4 + 1] = c[k * 3 + 1]; sl[k * 4 + 2] = c[k * 3 + 2]; sl[k * 4 + 3] = 1; }
            const short* rest = iItemRest + d.mesh * 3;
            r.DrawMeshRot(iMItem[d.mesh], 0, p.x, p.y, p.z, p.yaw, rest[0], rest[2], sl, iGrab == i ? MF_FLASH : 0);
            continue;
            }
        if (p.type >= PT_PILLAR) { DrawDunProp(p); continue; }
        if (p.type == PT_BRIDGE || p.type == PT_LEVER || p.type == PT_CRATE) { DrawCityProp(p); continue; }
        const BaseDef& bd = Base(p);
        int in = bd.mesh, frame = 0;
        if (p.type == PT_GARLIC) frame = p.state;
        else if (p.type == PT_FISH) in = p.item == IT_RAW_VENISON ? IN_HAM : IN_FISH;
        else if (bd.kind == BO_DOOR || p.type == PT_BARREL || p.type == PT_DRESSER || p.type == PT_CUPBOARD) frame = Clamp(p.anim, 0, 4);
        if (p.type == PT_HARVEST)
            {
            // drawn with its own model from the area (the rabbit on its hook, the purse)
            if (p.model < 0 || p.model >= iKitMeshN || p.model >= MAXKIT || iKitType[p.model] < ST_CUSTOM) continue;
            const StaticDef& d = r.iDefs[iKitType[p.model] - ST_CUSTOM];
            r.DrawMesh(iMKit[p.model], 0, p.x, p.y, p.z, p.yaw, 256, d.slots, 0);
            continue;
            }
        if (p.type == PT_DOOR && p.count > 0)
            {
            // (v0.4.74) a ruin's door: Skyrim's model swung about its hinge (p.x, p.z), a quarter turn open
            if (p.model < 0 || p.model >= iKitMeshN || p.model >= MAXKIT || iKitType[p.model] < ST_CUSTOM) continue;
            const StaticDef& d = r.iDefs[iKitType[p.model] - ST_CUSTOM];
            int y2 = (p.yaw + Clamp(p.anim, 0, 4) * 64) & 1023, cs = r3d_cos(y2), sn = r3d_sin(y2);
            int lx = Cm(p.vx), lz = Cm(p.vz);
            r.DrawMesh(iMKit[p.model], 0, p.x - ((lx * cs + lz * sn) >> 14), p.y, p.z - ((-lx * sn + lz * cs) >> 14), y2, 256, d.slots, 0);
            continue;
            }
        if (p.type == PT_CHEST)
            {
            const StaticDef& d = r.iDefs[iInType[IN_CHEST] - ST_CUSTOM];
            r.DrawMesh(iMChest, Clamp(p.anim, 0, 4), p.x, p.y, p.z, p.yaw, 256, d.slots, 0);     // the lid swinging up
            continue;
            }
        const StaticDef& d = r.iDefs[iInType[in] - ST_CUSTOM];
        r.DrawMesh(iMIn[in], frame, p.x, p.y, p.z, p.yaw, 256, d.slots, 0);
        if (p.type == PT_CASE) DrawCaseGoods(p);
        }
    }

// what lies on the velvet of a display case: its container's first three things, side by side
void Game::DrawCaseGoods(const Prop& p)
    {
    R3D& r = *iR;
    const Cont& c = iS.cont[p.item];
    int cs = r3d_cos(p.yaw), sn = r3d_sin(p.yaw);
    int k, n = 0;
    for (k = 0; k < CONTSLOTS && n < 3; k++)
        {
        if (!c.n[k]) continue;
        const ItemDef& d = GameItem(c.id[k]);
        const u8* col = GameItemCol(c.id[k]);
        u8 sl[12];
        int j;
        for (j = 0; j < 3; j++) { sl[j * 4] = col[j * 3]; sl[j * 4 + 1] = col[j * 3 + 1]; sl[j * 4 + 2] = col[j * 3 + 2]; sl[j * 4 + 3] = 1; }
        const short* rest = iItemRest + d.mesh * 3;
        int lx = Cm(-30 + n * 30);
        int x = p.x + ((lx * cs) >> 14), z = p.z + ((-lx * sn) >> 14);
        r.DrawMeshRot(iMItem[d.mesh], 0, x, p.y + Cm(82) + rest[1], z, p.yaw + 256, rest[0], rest[2], sl, 0);
        n++;
        }
    }

// prompt for the HUD: verb, name and a line of details like value and weight
void Game::PropText(int i, const char*& aVerb, const char*& aName, char* aInfo, int& aRed)
    {
    const Prop& p = iS.prop[i];
    aRed = 0; aInfo[0] = 0;
    if (DunPropText(i, aVerb, aName)) return;
    switch (p.type)
        {
        case PT_ITEM:
            {
            const ItemDef& d = GameItem(p.item);
            aName = d.name;
            aRed = (p.flags & PF_OWNED) != 0;
            aVerb = d.cat == IC_BOOK ? "Read" : (aRed ? "Steal" : "Take");     // a book is opened where it lies
            char* q = r3d_puts(aInfo, "Value ");
            q = r3d_puti(q, d.value);
            q = r3d_puts(q, "   Weight ");
            q = r3d_puti(q, d.weight10 / 10);
            *q++ = '.'; *q++ = (char)('0' + d.weight10 % 10); *q = 0;
            if (d.cat == IC_FOOD && d.power)
                {
                q = r3d_puts(q, "   +");
                q = r3d_puti(q, d.power);
                r3d_puts(q, d.effect == PW_MAGICKA ? " magicka" : (d.effect == PW_STAMINA ? " stamina" : " health"));
                }
            break;
            }
        case PT_GARLIC: aVerb = "Harvest"; aName = p.state >= 2 ? "Garlic Braid (bare)" : "Garlic Braid"; break;
        case PT_HARVEST:
            aName = p.item < 0 ? "Coin Purse" : (p.item == IT_PHEASANT ? "Pheasant" : "Rabbit");
            aRed = (p.flags & PF_OWNED) != 0;
            aVerb = aRed ? "Steal" : (p.item < 0 ? "Take" : "Harvest");
            break;
        case PT_FISH:
            aName = p.item == IT_RAW_VENISON ? "Venison" : "Salmon";
            aRed = p.cell == 0 && (p.flags & PF_OWNED);
            aVerb = aRed ? "Steal" : "Take";
            break;
        case PT_DOOR: aVerb = p.state ? "Open" : "Close"; aName = "Door"; break;
        default: aVerb = Base(p).verb; aName = Base(p).name; break;
        }
    }

// ---------------------------------------------------------------- recipes
struct Recipe { short result, ing[4]; };
static const Recipe KRecipes[] =
    {
    { IT_VENISON,        { IT_RAW_VENISON, IT_SALT, -1, -1 } },
    { IT_COOKED_BEEF,    { IT_RAW_BEEF, IT_SALT, -1, -1 } },
    { IT_SALMON_STEAK,   { IT_SALMON, IT_SALT, -1, -1 } },
    { IT_VEG_SOUP,       { IT_CABBAGE, IT_LEEK, IT_POTATO, IT_CARROT } },
    { IT_CABBAGE_SOUP,   { IT_CABBAGE, IT_POTATO, IT_SALT, -1 } },
    { IT_APPLE_STEW,     { IT_APPLE, IT_CABBAGE, IT_SALT, -1 } },
    { IT_VENISON_STEW,   { IT_RAW_VENISON, IT_POTATO, IT_LEEK, IT_SALT } },
    { IT_GARLIC_BREAD,   { IT_BREAD, IT_GARLIC, -1, -1 } }
    };
enum { NRECIPES = sizeof(KRecipes) / sizeof(Recipe) };
// the alchemy lab: two ingredients that share a virtue make a potion (after Skyrim's: blue mountain flower,
// wheat and blisterwort restore health; lavender and bone meal touch stamina; frost lichen the mind)
static const Recipe KBrews[] =
    {
    { IT_POT_MHEAL,      { IT_BLUE_FLOWER, IT_WHEAT, -1, -1 } },
    { IT_POT_HEAL,       { IT_BLISTERWORT, IT_BLUE_FLOWER, -1, -1 } },
    { IT_POT_MSTAM,      { IT_LAVENDER, IT_BONE_MEAL, -1, -1 } },
    { IT_POT_MMAG,       { IT_LICHEN, IT_LAVENDER, -1, -1 } },
    { IT_POT_HEAL,       { IT_BLISTERWORT, IT_TREANT_SAP, -1, -1 } },
    { IT_POT_MMAG,       { IT_WISP_DUST, IT_WHITE_CAP, -1, -1 } },
    { IT_POT_MSTAM,      { IT_PIKE_SCALES, IT_HOUNDROOT, -1, -1 } },
    { IT_POT_RESFIRE,    { IT_WRAITH_TEETH, IT_JUNIPER, -1, -1 } },
    { IT_POT_CURE,       { IT_GHOST_ESSENCE, IT_TAPROOT, -1, -1 } },
    { IT_POISON_WEAK,    { IT_IMP_CAP, IT_CRONE_FEATHERS, -1, -1 } },
    { IT_FROST_VENOM,    { IT_GRAVEBELL, IT_SHADOW_SALTS, -1, -1 } }
    };
enum { NBREWS = sizeof(KBrews) / sizeof(Recipe) };
static const Recipe* Recipes(int aMode, int& aN) { aN = aMode ? NBREWS : NRECIPES; return aMode ? KBrews : KRecipes; }

static int CanCook(Game& g, const Recipe& r)
    {
    int k;
    for (k = 0; k < 4; k++) if (r.ing[k] >= 0 && g.CountItem(r.ing[k]) < 1) return 0;
    return 1;
    }

void Game::CookInput()
    {
    int n;
    const Recipe* list = Recipes(iCookMode, n);
    if (iListSel >= n) iListSel = 0;
    if (Pressed(KEY_LOOK_U) || Pressed(KEY_FWD)) iListSel = (iListSel + n - 1) % n;
    if (Pressed(KEY_LOOK_D) || Pressed(KEY_BACK)) iListSel = (iListSel + 1) % n;
    if (!Pressed(KEY_FIRE)) return;
    const Recipe& r = list[iListSel];
    if (!CanCook(*this, r)) { Msg("You lack the ingredients."); return; }
    int k;
    for (k = 0; k < 4; k++) if (r.ing[k] >= 0) TakeItems(r.ing[k], 1);
    AddItem(r.result, 1);
    Msg2(GameItem(r.result).name, iCookMode ? " created" : " cooked");
    }

// ---------------------------------------------------------------- containers and barter
// Both use the inventory layout: categories, a list, a turning preview and the item's details.
// Side 0 = the barrel / merchant, side 1 = the player; Q switches sides, Tab leaves.
int GameBuyPrice(int v) { return (v * 14 + 9) / 10; }
int GameSellPrice(int v) { int p = v * 4 / 10; return p < 1 && v > 0 ? 1 : p; }

static int CatMatch(int aCat, const ItemDef& d, int aFav)
    {
    if (aCat == 0) return aFav;
    if (aCat == 1) return 1;
    return d.cat == aCat - 2;
    }

int Game::MenuRows(int aSide, int aCat, int* aId, int* aCount, int* aRef)
    {
    int n = 0, i;
    if (aSide == 0)
        {
        int ci = iUi == UI_TRADE ? iS.e[iTradeEnt].pad2 : iOpenCont;
        const Cont& c = iS.cont[ci];
        for (i = 0; i < CONTSLOTS; i++)
            {
            if (!c.n[i]) continue;
            if (!CatMatch(aCat == 0 ? 1 : aCat, GameItem(c.id[i]), 1)) continue;
            aId[n] = c.id[i]; aCount[n] = c.n[i]; aRef[n] = i; n++;
            }
        return n;
        }
    int mer = iUi == UI_TRADE ? iS.e[iTradeEnt].pad1 % MER_COUNT : -1;
    for (i = 0; i < iS.ninv; i++)
        {
        const InvItem& it = iS.inv[i];
        if (it.equipped) continue;
        if (mer >= 0 && !GameMerchantBuys(mer, it.id)) continue;      // as in Skyrim: only what they deal in
        if (!CatMatch(aCat, GameItem(it.id), it.fav)) continue;
        aId[n] = it.id; aCount[n] = it.count; aRef[n] = i; n++;
        }
    return n;
    }

// the categories worth listing on a side: "All" always, the others only when something is in them
int Game::MenuCats(int aSide, int* aOut)
    {
    int ids[MAXINV], cnt[MAXINV], ref[MAXINV];
    int n = 0, c;
    for (c = 0; c < NCATS; c++)
        {
        if (c == 0 && aSide == 0) continue;                    // no favourites in a barrel
        if (c == 1 || MenuRows(aSide, c, ids, cnt, ref) > 0) aOut[n++] = c;
        }
    return n;
    }

static int CatStep(const int* aCats, int aN, int aCur, int aDir)
    {
    int k, at = 0;
    for (k = 0; k < aN; k++) if (aCats[k] == aCur) at = k;
    return aCats[(at + aDir + aN) % aN];
    }

// joystick/WASD: category column <-> list, up/down; the press moves one item
void Game::ItemMenuInput()
    {
    int ids[MAXINV], cnt[MAXINV], ref[MAXINV], cats[NCATS];
    int nc = MenuCats(iMenuSide, cats);
    int k, ok = 0;
    for (k = 0; k < nc; k++) if (cats[k] == iInvCat) ok = 1;
    if (!ok) iInvCat = 1;                                      // the category emptied
    int n = MenuRows(iMenuSide, iInvCat, ids, cnt, ref);
    if (Pressed(KEY_LOOK_U) || Pressed(KEY_FWD))
        {
        if (iInvCol == 0) { iInvCat = CatStep(cats, nc, iInvCat, -1); iInvSel = 0; iInvScroll = 0; }
        else if (iInvSel > 0) iInvSel--;
        }
    if (Pressed(KEY_LOOK_D) || Pressed(KEY_BACK))
        {
        if (iInvCol == 0) { iInvCat = CatStep(cats, nc, iInvCat, 1); iInvSel = 0; iInvScroll = 0; }
        else if (iInvSel < n - 1) iInvSel++;
        }
    if (Pressed(KEY_LOOK_R) || Pressed(KEY_STRAFE_R)) { if (iInvCol == 0 && n > 0) { iInvCol = 1; iInvSel = 0; } }
    if (Pressed(KEY_LOOK_L) || Pressed(KEY_STRAFE_L)) iInvCol = 0;
    if (Pressed(KEY_FIRE))
        {
        if (iInvCol == 0) { if (n > 0) { iInvCol = 1; iInvSel = 0; } }
        else if (iInvSel < n) MoveOne(ids[iInvSel], ref[iInvSel]);
        }
    n = MenuRows(iMenuSide, iInvCat, ids, cnt, ref);
    if (iInvSel >= n) iInvSel = n > 0 ? n - 1 : 0;
    if (n == 0 && iInvCol == 1) iInvCol = 0;
    }

// take / store / buy / sell a single item
void Game::MoveOne(int aId, int aRef)
    {
    int trade = iUi == UI_TRADE;
    int ci = trade ? iS.e[iTradeEnt].pad2 : iOpenCont;
    int mi = trade ? iS.e[iTradeEnt].pad1 % MER_COUNT : 0;
    Cont& c = iS.cont[ci];
    if (iMenuSide == 0)
        {
        if (trade)
            {
            int price = GameBuyPrice(GameItem(aId).value);
            if (iS.gold < price) { Msg("You don't have enough gold."); return; }
            iS.gold -= price; iS.mgold[mi] += price;
            }
        AddItem(aId, 1);
        c.n[aRef]--;
        }
    else
        {
        int k, free = 0;
        for (k = 0; k < CONTSLOTS; k++) if (!c.n[k] || c.id[k] == aId) free = 1;
        if (!free) { Msg(trade ? "The merchant has no room." : "It's full."); return; }
        if (trade)
            {
            int price = GameSellPrice(GameItem(aId).value);
            if (iS.mgold[mi] < price) { Msg("The merchant can't afford it."); return; }
            iS.gold += price; iS.mgold[mi] -= price;
            }
        RemoveItem(aRef, 1);
        ContAdd(ci, aId, 1);
        }
    }

void Game::OpenItemMenu(int aUi)
    {
    iUi = aUi;
    iMenuSide = 0; iInvCat = 1; iInvCol = 1; iInvSel = 0; iInvScroll = 0;
    }

void Game::ContInput() { ItemMenuInput(); }
void Game::TradeInput() { ItemMenuInput(); }

void Game::CloseScreens()
    {
    iUi = UI_PLAY;
    iOpenWaitUs = 0;
    iOpenProp = -1;
    iTradeEnt = -1;
    }

// keys typed on the item screens
void Game::ListKey(int c)
    {
    if (c == CH_ESC || c == CH_TAB || c == 'i' || c == 'I') { CloseScreens(); return; }
    if ((c == 'q' || c == 'Q') && iUi != UI_COOK)
        {
        iMenuSide ^= 1; iInvSel = 0; iInvScroll = 0; iInvCol = 1; iInvCat = 1;
        return;
        }
    if (c == 'e' || c == 'E' || c == CH_ENTER)
        {
        u32 h = iHit; iHit |= KEY_FIRE;                 // (E as a press of the joystick)
        if (iUi == UI_COOK) CookInput();
        else ItemMenuInput();
        iHit = h;
        return;
        }
    if ((c == 'r' || c == 'R') && iUi == UI_CONT)
        {
        // take everything
        Cont& k = iS.cont[iOpenCont];
        int s;
        for (s = 0; s < CONTSLOTS; s++) if (k.n[s]) { AddItem(k.id[s], k.n[s]); k.n[s] = 0; }
        CloseScreens();
        }
    }

void Game::DrawCont(int x0, int w) { DrawItemMenu(x0, w); }
void Game::DrawTrade(int x0, int w) { DrawItemMenu(x0, w); }

// container titles in capitals, as the menu shows them
static const char* KCapsName(const Prop& p)
    {
    switch (p.type)
        {
        case PT_SACK: return "SACK";
        case PT_CHEST: return "CHEST";
        case PT_CASE: return "DISPLAY CASE";
        case PT_DRESSER: return "DRESSER";
        case PT_CUPBOARD: return "WARDROBE";
        }
    return "BARREL";
    }

const char* GameContName(Game& g)
    {
    if (g.iUi == UI_TRADE) return KMerName[g.iS.e[g.iTradeEnt].pad1 % MER_COUNT];
    if (g.iOpenProp >= 0) return KCapsName(g.iS.prop[g.iOpenProp]);
    return "BARREL";
    }
int GameMerchantGold(Game& g) { return g.iS.mgold[g.iS.e[g.iTradeEnt].pad1 % MER_COUNT]; }

void Game::DrawCook(int x0, int w)
    {
    R3D& r = *iR;
    r.Shade(x0, 0, w, OUT_H);
    const char* title = iCookMode ? "ALCHEMY LAB" : "COOKING POT";
    r.Text(x0 + w / 2 - r.TextW(title) / 2, 6, title, KGold);
    int n;
    const Recipe* list = Recipes(iCookMode, n);
    int rows = 8, first = iListSel - rows + 1, i;
    if (first < 0) first = 0;
    for (i = 0; i < rows && first + i < n; i++)
        {
        int k = first + i, y = 22 + i * 19;
        const Recipe& rc = list[k];
        int ok = CanCook(*this, rc);
        if (k == iListSel) r.Tint(x0 + 4, y - 2, w - 8, 19, KGold);
        r.Text(x0 + 8, y, GameItem(rc.result).name, ok ? (k == iListSel ? KWhite : KGreen) : KGrey);
        char buf[96];
        char* q = buf;
        int j;
        for (j = 0; j < 4 && rc.ing[j] >= 0; j++)
            {
            if (j) q = r3d_puts(q, ", ");
            q = r3d_puts(q, GameItem(rc.ing[j]).name);
            }
        r.Text(x0 + 16, y + 9, buf, KGrey);
        }
    const char* hint = iCookMode ? "E: combine   Joystick: choose   Tab: done" : "E: cook   Joystick: choose   Tab: done";
    r.Text(x0 + w / 2 - r.TextW(hint) / 2, OUT_H - 12, hint, KGrey);
    }

// ---------------------------------------------------------------- the land's plants (v0.4.39)
// the kinds' colours (slot 0, slot 1): bushes, ferns, flowers, crops (gen_kit.py's FLORA)
static const u8 KFloraCol[Game::FK_COUNT * 6] =
    {
    70, 92, 44,   54, 72, 36,       // 0 thicket
    150, 138, 64, 70, 90, 40,       // 1 yellow shrub
    112, 112, 74, 80, 90, 56,       // 2 tundra shrub
    104, 84, 62,  80, 64, 48,       // 3 dead shrub
    62, 104, 42,  50, 80, 34,       // 4 fern
    150, 84, 172, 70, 110, 50,      // 5 purple mountain flower
    74, 104, 206, 70, 110, 50,      // 6 blue mountain flower
    204, 52, 50,  70, 110, 50,      // 7 red mountain flower
    146, 112, 206, 76, 108, 60,     // 8 lavender
    170, 92, 170, 92, 112, 62,      // 9 thistle
    232, 232, 222, 110, 120, 80,    // 10 tundra cotton
    64, 112, 44,  50, 90, 36,       // 11 clover
    204, 40, 40,  206, 214, 210,    // 12 snowberries
    104, 152, 72, 84, 128, 60,      // 13 cabbage
    84, 124, 52,  70, 100, 44,      // 14 potato
    96, 144, 72,  80, 120, 60,      // 15 leek
    204, 174, 84, 170, 150, 70,     // 16 wheat
    164, 62, 42,  120, 50, 36,      // 17 vine maple
    222, 226, 232, 190, 196, 204,   // 18 snowy bush
    192, 112, 44, 150, 90, 40,      // 19 autumn bush
    150, 140, 128, 120, 110, 100,   // 20 driftwood
    220, 160, 60,  196, 190, 178,   // 21 aspen, tall (leaves, bark)
    220, 160, 60,  196, 190, 178,   // 22 aspen
    220, 160, 60,  196, 190, 178,   // 23 aspen, small
    40, 74, 42,    92, 64, 40,      // 24 pine
    196, 202, 210, 92, 64, 40       // 25 snowy pine
    };
void GameFloraColDefault(u8* aD) { int i; for (i = 0; i < Game::FK_COUNT * 6; i++) aD[i] = KFloraCol[i]; }

// what a kind gives when picked (0: nothing, a bush)
int Game::FloraItem(int aKind)
    {
    switch (aKind)
        {
        case 5: return IT_PURPLE_FLOWER + 1;
        case 6: return IT_BLUE_FLOWER + 1;
        case 7: return IT_RED_FLOWER + 1;
        case 8: return IT_LAVENDER + 1;
        case 9: return IT_THISTLE + 1;
        case 10: return IT_TUNDRA_COTTON + 1;
        case 12: return IT_SNOWBERRIES + 1;
        case 13: return IT_CABBAGE + 1;
        case 14: return IT_POTATO + 1;
        case 15: return IT_LEEK + 1;
        case 16: return IT_WHEAT + 1;
        }
    return 0;
    }
void Game::FloraPos(int i, int& aX, int& aZ)
    {
    const u8* e = iFloraEnt + i * 6;
    aX = iFloraOX + (e[0] | (e[1] << 8)) * (METER / 8); aZ = iFloraOZ + (e[2] | (e[3] << 8)) * (METER / 8);
    }

// the plants within 24 m (the Plants setting on): their models, turned and sized as Skyrim has them; a picked
// flower shows its stems only
void Game::DrawFlora()
    {
    if (!iFloraN || iS.cell) return;
    R3D& r = *iR;
    // (v0.4.66) the trees among them (kinds FK_TREE0..: the Rift's) out to the object distance, the Plants setting or
    // not; their far form past 16 m (the Full models setting's tree distance)
    int plants = !((iS.view >> 9) & 1);
    int treeD = r.iObjD >> 8, reachM = treeD > 24 ? treeD : 24;
    int reach = reachM * METER;
    int tfar = GameFullDist(iS.view, 8);
    int sx0 = (r.iCamX - reach - iFloraOX) / iFloraSq, sx1 = (r.iCamX + reach - iFloraOX) / iFloraSq;
    int sz0 = (r.iCamZ - reach - iFloraOZ) / iFloraSq, sz1 = (r.iCamZ + reach - iFloraOZ) / iFloraSq;
    if (sx0 < 0) sx0 = 0;
    if (sz0 < 0) sz0 = 0;
    if (sx1 >= iFloraGW) sx1 = iFloraGW - 1;
    if (sz1 >= iFloraGH) sz1 = iFloraGH - 1;
    int sx, sz, i;
    u8 sl[8];
    for (sz = sz0; sz <= sz1; sz++)
        for (sx = sx0; sx <= sx1; sx++)
            {
            int b = sz * iFloraGW + sx;
            for (i = iFloraFirst[b]; i < iFloraFirst[b + 1] && i < iFloraN; i++)
                {
                int x, z;
                FloraPos(i, x, z);
                int dx = (x - r.iCamX) >> 8, dz = (z - r.iCamZ) >> 8;
                const u8* e = iFloraEnt + i * 6;
                int k = e[4];
                if (k >= FK_COUNT) continue;
                int d2 = dx * dx + dz * dz;
                if (k >= FK_TREE0)
                    {
                    if (d2 > treeD * treeD) continue;
                    }
                else if (!plants || d2 > 24 * 24) continue;
                const u8* c = iFloraCol + k * 6;
                int taken = (iFloraTaken[i >> 3] >> (i & 7)) & 1;
                sl[0] = taken ? c[3] : c[0]; sl[1] = taken ? c[4] : c[1]; sl[2] = taken ? c[5] : c[2];
                sl[3] = iFloraTex[k] != 255 ? (u8)(iFloraTex[k] < 0x40 ? 0xC0 | iFloraTex[k] : iFloraTex[k]) : 1;      // (v0.4.56: lib_pack.py writes the look byte itself: 0x40 + n the library's)      // (Skyrim's leaf cards: their picture, cut out)
                sl[4] = c[3]; sl[5] = c[4]; sl[6] = c[5]; sl[7] = 1;
                int sc = (205 + (e[5] & 3) * 51) * 5 / 2;          // (the models are made in cm-ish units: 2.5 x)
                int ffar = k >= FK_TREE0 ? tfar : GameFullDist(iS.view, 7);     // (the far form past 7 m - with the Full models setting; -1: always)
                const Mesh& fm = iFloraFar[k] && (ffar < 0 || dx * dx + dz * dz > ffar * ffar) ? iMFloraFar[k] : iMFlora[k];
                r.DrawMesh(fm, 0, x, r.GroundAt(x, z), z, (e[5] >> 2) * 16, sc, sl, 0);
                }
            }
    }

// grass: a tuft every 3 m (each nudged by a hash of where it is) within 15 m where the ground grows it, in its
// colours (Skyrim's grassy ground textures: the plains, the fields, the tundra, the Reach, the marsh, the Rift)
void Game::DrawGrass()
    {
    if (iS.cell || (iS.view >> 9) & 1) return;
    R3D& r = *iR;
    int step = 5 * METER / 2, reach = 13;
    int gx0 = (r.iCamX - reach * METER) / step, gx1 = (r.iCamX + reach * METER) / step;
    int gz0 = (r.iCamZ - reach * METER) / step, gz1 = (r.iCamZ + reach * METER) / step;
    int fx = r3d_sin(r.iYaw), fz = r3d_cos(r.iYaw);
    int gx, gz;
    u8 sl[8];
    for (gz = gz0; gz <= gz1; gz++)
        for (gx = gx0; gx <= gx1; gx++)
            {
            u32 h = (u32)(gx * 73856093) ^ (u32)(gz * 19349663);
            h ^= h >> 13; h *= 0x5bd1e995; h ^= h >> 15;
            int x = gx * step + (int)(h & 511) * step / 512, z = gz * step + (int)((h >> 9) & 511) * step / 512;
            int dx = x - r.iCamX, dz = z - r.iCamZ;
            if ((dx >> 8) * (dx >> 8) + (dz >> 8) * (dz >> 8) > reach * reach) continue;
            if (((dx >> 4) * fx + (dz >> 4) * fz) < -(2 * METER << 10)) continue;          // (behind: not drawn anyway)
            int m = r.MaterialAt(x, z) & VM_MAT;
            int g, l;
            switch (m)
                {
                case TM_GRASS: g = 0; break;
                case TM_FIELD: g = 1; break;
                case TM_TUNDRA: g = 2; break;
                case TM_REACH: g = 3; break;
                case TM_MARSH: g = 4; break;
                case TM_FALL: g = 5; break;
                default: continue;
                }
            static const u8 KTuft[6 * 3] = { 72, 116, 42,  188, 168, 86,  140, 132, 70,  110, 116, 60,  104, 112, 78,  176, 120, 52 };
            l = 230 + (int)((h >> 18) & 31);
            sl[0] = (u8)(KTuft[g * 3] * l >> 8); sl[1] = (u8)(KTuft[g * 3 + 1] * l >> 8); sl[2] = (u8)(KTuft[g * 3 + 2] * l >> 8); sl[3] = 1;
            sl[4] = (u8)(KTuft[g * 3] * 7 >> 3); sl[5] = (u8)(KTuft[g * 3 + 1] * 7 >> 3); sl[6] = (u8)(KTuft[g * 3 + 2] * 7 >> 3); sl[7] = 1;
            r.DrawMesh(iMTuft, 0, x, r.GroundAt(x, z), z, (int)(h >> 20) & 1023, 560 + (int)((h >> 24) & 255), sl, 0);
            }
    }

// a plant to pick, straight ahead within 2.5 m: focus kind 5
void Game::FindFloraFocus()
    {
    if (!iFloraN || iS.cell || iFocusKind || iGrab >= 0 || iSitting >= 0) return;
    int fx = r3d_sin(iS.yaw), fz = r3d_cos(iS.yaw);
    int sx = (iS.px - iFloraOX) / iFloraSq, sz = (iS.pz - iFloraOZ) / iFloraSq, best = -1, bestD = 25 * METER / 10, ax, az;
    for (az = sz - 1; az <= sz + 1; az++)
        for (ax = sx - 1; ax <= sx + 1; ax++)
            {
            if (ax < 0 || az < 0 || ax >= iFloraGW || az >= iFloraGH) continue;
            int b = az * iFloraGW + ax, i;
            for (i = iFloraFirst[b]; i < iFloraFirst[b + 1] && i < iFloraN; i++)
                {
                if (!FloraItem(iFloraEnt[i * 6 + 4]) || ((iFloraTaken[i >> 3] >> (i & 7)) & 1)) continue;
                int x, z;
                FloraPos(i, x, z);
                int dx = x - iS.px, dz = z - iS.pz;
                if (Abs(dx) > bestD || Abs(dz) > bestD) continue;
                int d = r3d_isqrt((dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4)) << 4;
                if (d > bestD) continue;
                if (d >= 30 && (dx * fx + dz * fz) / d < 12000) continue;     // ahead, within 40 degrees
                best = i; bestD = d;
                }
            }
    if (best >= 0) { iFocusKind = 5; iFocus = best; }
    }

// Skyrim's exterior cell (4096 units) of a point (engine units)
static void SkyCell(int aX, int aZ, int& aCx, int& aCy)
    {
    // engine x = (X + 233472) * 3.6576 (units): Skyrim's x 0 at engine 853 948; a cell (4096 units) 14 982 engine units
    int X = aX - 853947, Y = aZ - 644206;
    aCx = X >= 0 ? X / 14982 : -((-X + 14981) / 14982); aCy = Y >= 0 ? Y / 14982 : -((-Y + 14981) / 14982);
    }

void Game::RebuildFloraTaken()
    {
    if (!iFloraTaken) return;
    r3d_memset(iFloraTaken, 0, (FLORA_MAX + 7) / 8);
    int k;
    if (iS.npick < 0 || iS.npick > GameState::FLORA_PICKS) iS.npick = 0;
    for (k = 0; k < iS.npick; k++)
        {
        int i = iS.pick[k].idx;
        if (i < iFloraN) iFloraTaken[i >> 3] |= (u8)(1 << (i & 7));
        }
    }

// (every frame, a few at a time): a picked plant whose cell is loaded (within two cells of the player's, outdoors)
// starts its wait anew; one whose wait is over (240 game hours) grows back
void Game::UpdatePicks()
    {
    if (!iS.npick) return;
    int now = GameHour(), pcx = 0, pcy = 0, out = !iS.cell, k, any = 0;
    if (out) SkyCell(iS.px, iS.pz, pcx, pcy);
    for (k = 0; k < iS.npick; )
        {
        int i = iS.pick[k].idx, x, z, cx, cy;
        if (i >= iFloraN) { iS.pick[k] = iS.pick[--iS.npick]; any = 1; continue; }
        FloraPos(i, x, z); SkyCell(x, z, cx, cy);
        if (out && cx - pcx <= 2 && pcx - cx <= 2 && cy - pcy <= 2 && pcy - cy <= 2) iS.pick[k].hour = now;
        else if (now - iS.pick[k].hour >= 240) { iS.pick[k] = iS.pick[--iS.npick]; any = 1; continue; }
        k++;
        }
    if (any) RebuildFloraTaken();
    }

void Game::TakeFlora(int i)
    {
    if (i < 0 || i >= iFloraN) return;
    int it = FloraItem(iFloraEnt[i * 6 + 4]) - 1;
    if (it < 0 || ((iFloraTaken[i >> 3] >> (i & 7)) & 1)) return;
    iFloraTaken[i >> 3] |= (u8)(1 << (i & 7));
    if (iS.npick < GameState::FLORA_PICKS) { iS.pick[iS.npick].idx = (u16)i; iS.pick[iS.npick].pad = 0; iS.pick[iS.npick].hour = GameHour(); iS.npick++; }
    AddItem(it, 1);
    Msg2(GameItem(it).name, " added");
    }
