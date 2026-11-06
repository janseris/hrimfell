// game_cells.cpp - interior cells (separate spaces behind doors), travel list for testing, waiting,
// weather, the third-person camera and the player's own body.
#include "game.h"

static const int M = METER;
static inline int Abs(int v) { return v < 0 ? -v : v; }
static int Clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static int Cm(int cm) { return cm * M / 100; }

// city frame (same as game_city.cpp): metres relative to the city centre
enum { CITY_X = CITY_CX, CITY_Z = CITY_CZ };      // (game.h)
static int Wx(int mx) { return (CITY_X * CELL + CELL / 2) + mx * M; }
static int Wz(int mz) { return (CITY_Z * CELL + CELL / 2) + mz * M; }

// interiors live in their own coordinate space: origin here, floor at iCellFloorY
enum { IN_OX = 100 * METER, IN_OZ = 100 * METER, IN_FLOOR = 100 * METER };
static int Ix(int cm) { return IN_OX + Cm(cm); }
static int Iz(int cm) { return IN_OZ + Cm(cm); }

const u8* GameItemCol(int aId);
int GameVillageX();
int GameVillageZ();

// ---------------------------------------------------------------- doors
void GameBarrowDoors(DoorLink* d, int aKit);
int GameRwDoorX(); int GameRwDoorZ(); int GameRwDoorYaw();
int GameRwDoorH(); int GameRwBalconyX(); int GameRwBalconyZ(); int GameRwBalconyYaw(); int GameRwBalconyH();
int GameInnDoorX(); int GameInnDoorZ(); int GameInnDoorYaw();
int GameKitDoors(const Game& g, DoorLink* d, int aMax);
int GameKitFloor(const Game& g, int aCell, int aX, int aZ, int aFeetY, int* aFound);
int GameKitBlocked(const Game& g, int aCell, int aX, int aZ, int aR, int aFeetY);
// door positions: outdoors in world units, indoors in interior units; yaw = facing when you come through
static void DoorTable(const Game& g, DoorLink* d)
    {
    // Vindgard Stables (below the approach, east of the road): the residence beside the stable block, door to the road
    d[0].cell = CELL_STABLES;
    d[0].ox = Wx(-82) - 3 * M / 4; d[0].oz = Wz(-51) - M / 2; d[0].oyaw = 836;
    d[0].ix = Ix(-200); d[0].iz = Iz(-440); d[0].iyaw = 0;
    d[0].name = "Whiterun Stables";
    d[0].ocell = 0;
    if (GameIsKitCell(g, CELL_STABLES)) d[0].name = 0;      // (v0.4.48: Skyrim's own stables, from the area's file: its door)
    // the Hall of the Dead (Wind District, building 25): its street door faces south
    d[1].cell = CELL_HALLDEAD; d[1].ocell = 0;
    d[1].ox = Wx(0) - 254 * M / 10; d[1].oz = Wz(0) + 453 * M / 10; d[1].oyaw = 512;
    d[1].ix = Ix(0); d[1].iz = Iz(-490); d[1].iyaw = 0;
    d[1].name = "Hall of the Dead";
    // down from the hall to the catacombs: the iron door at the hall's north end
    d[2].cell = CELL_CATACOMBS; d[2].ocell = CELL_HALLDEAD;
    d[2].ox = Ix(0); d[2].oz = Iz(690); d[2].oyaw = 512;
    d[2].ix = Ix(0); d[2].iz = Iz(-90); d[2].iyaw = 0;
    d[2].name = "Hall of the Dead Catacombs";
    // the Bitterroot (the apothecary, east of the market): its street door faces west
    d[3].cell = CELL_SHOP; d[3].ocell = 0;
    d[3].ox = Wx(0) - 1 * M / 10; d[3].oz = Wz(0) - 263 * M / 10; d[3].oyaw = 768;
    d[3].ix = Ix(300); d[3].iz = Iz(890); d[3].iyaw = 512;
    d[3].name = "Arcadia's Cauldron";
    // the Gilded Stag (the inn on its rise above the market): the front door faces west, the kitchen's back door north
    d[4].cell = CELL_INN; d[4].ocell = 0;
    d[4].ox = Wx(0) + 37 * M / 10; d[4].oz = Wz(0) - 85 * M / 10; d[4].oyaw = 768;
    d[4].ix = Ix(10); d[4].iz = Iz(860); d[4].iyaw = 256;
    d[4].name = "The Bannered Mare";
    d[5].cell = CELL_INN; d[5].ocell = 0;
    d[5].ox = Wx(0) + 86 * M / 10; d[5].oz = Wz(0) + 10 * M / 10; d[5].oyaw = 0;
    d[5].ix = Ix(505); d[5].iz = Iz(1860); d[5].iyaw = 256;
    d[5].name = "The Bannered Mare";
    // Highhall, at the top of the four flights: the great doors face south-west over the city
    d[6].cell = CELL_HIGHHALL; d[6].ocell = 0;
    d[6].ox = Wx(0) + 242 * M / 10; d[6].oz = Wz(0) + 792 * M / 10; d[6].oyaw = 612;
    d[6].ix = Ix(0); d[6].iz = Iz(30); d[6].iyaw = 0;
    d[6].name = "Dragonsreach";
    if (g.iWrKit)
        {
        // (v0.4.57) Skyrim's Whiterun: these doors where Skyrim has them (doors.csv: the doors themselves, in
        // Tamriel's units, facing where Skyrim puts you out); the game's own rooms behind them still
        static const int KWrOut[5 * 4] = { 1, 22741, -1956, 483,   3, 24940, -8409, 928,   4, 25726, -7631, 763,
                                           5, 25918, -6863, 761,   6, 27242, 630, 629 };
        int k;
        for (k = 0; k < 5; k++)
            {
            DoorLink& l = d[KWrOut[k * 4]];
            l.ox = (int)(((long long)KWrOut[k * 4 + 1] + 233472) * 36576 / 10000);
            l.oz = (int)(((long long)KWrOut[k * 4 + 2] + 176128) * 36576 / 10000);
            l.oyaw = KWrOut[k * 4 + 3];
            }
        }
    // Bleak Falls Barrow (game_dungeon.cpp)
    GameBarrowDoors(d + 7, g.iBfbKit);
    // the Riverwood Trader: its front door (Skyrim's placement: the street door faces north-west)
    d[11].cell = CELL_RWTRADER; d[11].ocell = 0;
    d[11].ox = GameRwDoorX(); d[11].oz = GameRwDoorZ(); d[11].oyaw = GameRwDoorYaw();
    d[11].ix = Ix(-273); d[11].iz = Iz(-742); d[11].iyaw = 0;
    d[11].name = "Riverwood Trader";
    d[11].hk = 3; d[11].iy = IN_FLOOR; d[11].oy = Cm(GameRwDoorH());
    // its upstairs door, onto the balcony over the street (Skyrim's FarmhouseLDoor01 00070E68, 240 units up inside)
    d[13].cell = CELL_RWTRADER; d[13].ocell = 0;
    d[13].ox = GameRwBalconyX(); d[13].oz = GameRwBalconyZ(); d[13].oyaw = GameRwBalconyYaw();
    d[13].ix = Ix(-273); d[13].iz = Iz(-642); d[13].iyaw = 0;
    d[13].name = "Riverwood Trader";
    d[13].hk = 3; d[13].iy = IN_FLOOR + Cm(343); d[13].oy = Cm(GameRwBalconyH());
    // the Sleeping Giant Inn: its front door on the porch (inside: FarmhouseLDoor01 at (-256, -422) Skyrim units)
    d[12].cell = CELL_RWINN; d[12].ocell = 0;
    d[12].ox = GameInnDoorX(); d[12].oz = GameInnDoorZ(); d[12].oyaw = GameInnDoorYaw();
    d[12].ix = Ix(-366); d[12].iz = Iz(-585); d[12].iyaw = 0;
    d[12].name = "Sleeping Giant Inn";
    // the houses built from Skyrim's pieces (game_kit.cpp): their doors, both sides, are Skyrim's
    GameKitDoors(g, d + 14, 12);
    // (without the area's file, its doors are not there: named nothing, nowhere anyone goes)
    int k;
    // (v0.4.68) Forelhost (game_dungeon.cpp): its doors, Skyrim's
    { void GameForelhostDoors(DoorLink* d); GameForelhostDoors(d + 26); }
    for (k = 0; k < 30; k++) if (!d[k].name) { d[k].name = ""; d[k].cell = 0; d[k].ocell = -1; }
    }
int GameKitDoorsN();
enum { NDOORS = 30 };

int Game::NumDoors() { return NDOORS; }

const DoorLink& Game::Door(int i)
    {
    // the table is rebuilt into iDoors on first use (no writable static data on EKA1)
    if (!iDoorsInit) { DoorTable(*this, iDoors); iDoorsInit = 1; }
    return iDoors[i];
    }

// ---------------------------------------------------------------- floors and walls inside
// wall rectangles in cm (x0, z0, x1, z1) per cell
static const int KStablesWalls[] =
    {
    -630, -480, -260, -450,   -140, -480, 790, -450,      // south wall
    -260, -480, -140, -440,                                // the (closed) door
    -630, 450, 790, 480,                                   // north
    -630, -480, -600, 480,                                 // west
    760, -480, 790, 480,                                   // east
    200, -450, 220, -260,   200, -150, 220, 150,   200, 260, 220, 450,   // partition with two doorways
    220, -10, 760, 10                                      // between the bedrooms
    };

// catacombs floor (cm above the interior floor): upper level 0, two flights of stairs down to the lower level at -300
static int CataFloorCm(int x, int z)
    {
    if (z < 300) return 0;
    if (z < 900 && ((x > -420 && x < -100) || (x > 540 && x < 820))) return -(z - 300) / 2;
    return -300;
    }

static const int KHallWalls[] =
    {
    -620, -520, 620, -500,                                 // south (the street door)
    -620, 700, 620, 720,                                   // north (the iron door to the catacombs)
    600, -520, 620, 720,                                   // east
    -620, -520, -600, 360,   -620, 540, -600, 720,         // west, with the doorway to the priest's room
    -1120, 130, -620, 150,   -1120, 700, -620, 720,   -1120, 130, -1100, 720     // the priest's room
    };
static const int KCataWalls[] =
    {
    -500, -120, 820, -100,                                 // corridor, south (the iron door from the hall)
    -1020, -270, -480, -250,   -1020, -270, -1000, 420,   -1020, 400, -480, 420,   // the side room
    -500, -270, -480, 60,   -500, 170, -480, 420,          // its east wall, the door between
    -500, 300, -400, 420,                                  // rock north of the corridor's west end
    -540, 420, -400, 900,                                  // between the gem alcove and the west stairs
    -120, 300, 560, 900,                                   // the rock between the two flights
    800, -120, 820, 900,                                   // east, along the corridor and the east stairs
    800, 880, 1520, 900,                                   // south of the amulet room
    -820, 420, -800, 1620,   -820, 1600, 940, 1620,        // the lower crypt: west, north
    900, 880, 940, 1120,   900, 1280, 940, 1620,           // its east wall, the doorway to the amulet room
    940, 1500, 1520, 1520,   1500, 880, 1520, 1520         // the amulet room
    };

enum { KHallWalls_N = sizeof(KHallWalls) / sizeof(int) / 4, KCataWalls_N = sizeof(KCataWalls) / sizeof(int) / 4 };

// the Bitterroot: two storeys. The ground floor at 0, the gallery, landing and her room at +320 cm, the stairs between.
// Walls carry the heights (cm above the ground floor) they stand between: G ground floor, U upper floor, F both.
enum { SG0 = -100, SG1 = 300, SU0 = 300, SU1 = 700 };
static const int KShopWalls[] =
    {
    -330, 900, 1030, 930, SG0, SU1,     1000, -30, 1030, 930, SG0, SU1,     -330, -30, -300, 930, SG0, SU1,   // north, east, west
    -330, -30, 520, 0, SG0, SG1,        660, -30, 1030, 0, SG0, SG1,                                         // south, the study door between
    380, -330, 400, 0, SG0, SG1,        800, -330, 820, 0, SG0, SG1,        380, -330, 820, -300, SG0, SG1,   // the study
    -15, -30, 5, 440, SG0, SG1,         -15, 580, 5, 930, SG0, SG1,         -300, 400, -15, 420, SG0, SG1,    // west wall, stair hall
    -15, 0, 5, 420, SU0, SU1,           -300, 400, -150, 440, SU0, SU1,     -300, 0, -15, 420, SU0, SU1,      // upstairs: over the storeroom
    258, 0, 275, 900, SU0, SU1,         -162, 440, -150, 770, SU0, SU1,                                       // railings
    -20, -380, 0, 0, SU0, SU1,          500, -380, 520, 0, SU0, SU1,        -20, -380, 520, -350, SU0, SU1,   // her room
    0, -15, 60, 5, SU0, SU1,            160, -15, 520, 5, SU0, SU1,         275, -15, 1030, 5, SU0, SU1
    };
enum { KShopWalls_N = sizeof(KShopWalls) / sizeof(int) / 6 };
// the Gilded Stag: the same scheme (cm, interior frame: the cross's south-west corner at 0, 0)
static const int KInnWalls[] =
    {
    -30, 500, 0, 1240, SG0, SU1,      -30, 1240, 500, 1270, SG0, SU1,   -30, 470, 500, 500, SG0, SU1,       // common room, outer
    470, -30, 500, 500, SG0, SG1,     1020, -30, 1050, 500, SG0, SG1,   470, -30, 1050, 0, SG0, SG1,        // the bar
    1020, 470, 1770, 500, SG0, SU1,   1020, 1240, 1770, 1270, SG0, SU1, 1740, 470, 1770, 1270, SG0, SU1,    // back room
    470, 1240, 500, 2430, SG0, SU1,   1020, 1240, 1050, 2430, SG0, SU1, 470, 2400, 1050, 2430, SG0, SU1,    // kitchen arm
    1010, 500, 1030, 760, SG0, SG1,   1010, 880, 1030, 1240, SG0, SG1,  630, 1240, 1020, 1260, SG0, SG1,    // inner, below
    1010, 500, 1030, 870, SU0, SU1,   1010, 1000, 1030, 1090, SU0, SU1,                                     // the rented room
    1020, 1090, 1040, 1100, SU0, SU1, 1160, 1090, 1220, 1100, SU0, SU1, 1200, 1100, 1220, 1240, SU0, SU1,   // its landing
    870, 800, 880, 1060, SU0, SU1,    870, 1055, 1020, 1065, SU0, SU1,  870, 795, 1020, 805, SU0, SU1,      // the balcony
    500, 1240, 1020, 1260, SU0, SU1,  880, 1260, 890, 1900, SU0, SU1,   890, 1260, 1020, 1560, SU0, SU1,    // over the kitchen
    500, 1900, 540, 1910, SU0, SU1,   660, 1900, 890, 1910, SU0, SU1,
    500, 2060, 700, 2070, SU0, SU1,   820, 2060, 1020, 2070, SU0, SU1
    };
enum { KInnWalls_N = sizeof(KInnWalls) / sizeof(int) / 6 };
#include "highhall.inc"
// Highhall: the floor under (x, z) is the highest of its floors and flights at or below the feet (+60 cm)
static int HHFloorCm(int x, int z, int feetCm)
    {
    int best = -100000, low = 100000, i;
    for (i = 0; i < KHHRamp_N; i++)
        {
        const int* q = KHHRamp + i * 7;
        if (x < q[0] || x > q[2] || z < q[1] || z > q[3]) continue;
        int y = q[5] + (q[6] - q[5]) * (z - q[1]) / (q[3] - q[1] + 1);
        if (y <= feetCm + 60 && y > best) best = y;
        if (y < low) low = y;
        }
    for (i = 0; i < KHHFloor_N; i++)
        {
        const int* f = KHHFloor + i * 5;
        if (x < f[0] || x > f[2] || z < f[1] || z > f[3]) continue;
        if (f[4] <= feetCm + 60 && f[4] > best) best = f[4];
        if (f[4] < low) low = f[4];
        }
    if (best > -100000) return best;
    return low < 100000 ? low : 0;
    }
static int InnFloorCm(int x, int z, int feetCm)
    {
    if (x > 655 && x < 1006 && z > 1120 && z < 1250) return Clamp((x - 655) * 320 / 351, 0, 320);      // stairs to the rented room
    if (x > 885 && x < 1025 && z > 1570 && z < 1921) return Clamp((z - 1570) * 320 / 351, 0, 320);     // the kitchen stairs
    if (feetCm > 160 && ((x >= 1000 && x < 1740 && z > 500 && z < 1240) || (x >= 870 && x < 1020 && z > 795 && z < 1065) ||
                         (x > 500 && x < 1020 && z > 1255 && z < 2400))) return 320;
    return 0;
    }
static int ShopUpper(int x, int z)
    {
    return (x > -300 && x < 0 && z >= 820 && z < 900) || (x > -150 && x < 0 && z > 420 && z < 900) ||
           (x >= 0 && x < 260 && z > 0 && z < 900) || (x > 0 && x < 500 && z > -350 && z <= 0);
    }
static int ShopFloorCm(int x, int z, int feetCm)
    {
    if (x > -290 && x < -150 && z > 470 && z < 820) return Clamp((z - 470) * 320 / 350, 0, 320);     // the stairs
    if (feetCm > 160 && ShopUpper(x, z)) return 320;
    return 0;
    }


void Game::AddRamp(int aX, int aZ0, int aLen, int aHw, int aRise)
    {
    // a flight along +z (len > 0) or -z (len < 0) from z0
    AddRampYaw(aX, aZ0, aLen < 0 ? 512 : 0, aLen < 0 ? -aLen : aLen, aHw, aRise, -1);
    }

void Game::AddRampYaw(int aX, int aZ, int aYaw, int aLen, int aHw, int aRise, int aBase)
    {
    if (iNumRamps >= 12) return;
    Ramp& r = iRamps[iNumRamps++];
    r.x = aX; r.z0 = aZ; r.dx = r3d_sin(aYaw & 1023); r.dz = r3d_cos(aYaw & 1023);
    r.len = aLen; r.hw = aHw; r.rise = aRise; r.base = aBase;
    }

int Game::FloorAt(int aCell, int aX, int aZ, int aFeetY)
    {
    if (IsDungeon(aCell) && !GameIsKitCell(*this, aCell)) return DunFloor(aCell, aX, aZ, aFeetY);
    if (aCell == CELL_CATACOMBS) return IN_FLOOR + Cm(CataFloorCm((aX - IN_OX) * 100 / M, (aZ - IN_OZ) * 100 / M));
    if (aCell == CELL_HIGHHALL) return IN_FLOOR + Cm(HHFloorCm((aX - IN_OX) * 100 / M, (aZ - IN_OZ) * 100 / M, (aFeetY - IN_FLOOR) * 100 / M));
    if (GameIsKitCell(*this, aCell)) { int found; return GameKitFloor(*this, aCell, aX, aZ, aFeetY, &found); }
    if (aCell == CELL_INN) return IN_FLOOR + Cm(InnFloorCm((aX - IN_OX) * 100 / M, (aZ - IN_OZ) * 100 / M, (aFeetY - IN_FLOOR) * 100 / M));
    if (aCell == CELL_SHOP) return IN_FLOOR + Cm(ShopFloorCm((aX - IN_OX) * 100 / M, (aZ - IN_OZ) * 100 / M, (aFeetY - IN_FLOOR) * 100 / M));
    if (aCell)  return IN_FLOOR;
    int g = iR->GroundAt(aX, aZ);
    int bf = BridgeFloor(aX, aZ, aFeetY);
    if (bf > g) g = bf;
    // Riverwood's buildings: their raised decks, porches, boardwalks and steps (game_kit.cpp)
    { int found, kf = GameKitOutFloor(*this, aX, aZ, aFeetY, iR->iLandLoaded ? iR->iLandOffset : 0, &found); if (found && kf > g) g = kf; }
    // things you can stand on: the wall walkway, the wall tops, stones and logs (reached from below by no more than
    // a step of 45 cm)
        {
        int t = iR->TopAt(aX, aZ, aFeetY + 45 * M / 100);
        if (t > g) g = t;
        }
    // flights of steps: walk on the treads, not on the cut ground below
    int i;
    for (i = 0; i < iNumRamps; i++)
        {
        Ramp& r = iRamps[i];
        int rx = (aX - r.x) >> 4, rz = (aZ - r.z0) >> 4;               // units of 16
        int along = ((rx * r.dx + rz * r.dz) >> 14) << 4, side = ((rx * r.dz - rz * r.dx) >> 14) << 4;
        // the top step reaches a metre on, onto the landing (the terrain's grid may meet it a little low)
        if (side < -r.hw || side > r.hw || along < -20 || along > r.len + M) continue;
        if (r.base < 0) r.base = iR->GroundAt(r.x, r.z0);
        int t = Clamp(along, 0, r.len);
        int h = r.base + M / 16 + t * (r.rise >> 3) / ((r.len >> 3) + 1);
        if (h > g) g = h;
        }
    return g;
    }

// (v0.4.74) how many of 8 ways (40 cm) a step can be taken from a spot, by the walker's own rules: not into a wall,
// not up more than a step
int Game::WaysOut(int aCell, int aX, int aZ, int aFeet, int aWalls)
    {
    int d, n = 0, sh = aCell ? 7 * M / 25 : 45 * M / 100;
    int ground = FloorAt(aCell, aX, aZ, aFeet);
    for (d = 0; d < 8; d++)
        {
        int mx = r3d_sin(d * 128), mz = r3d_cos(d * 128);
        int x = aX + ((mx * (M / 4)) >> 14), z = aZ + ((mz * (M / 4)) >> 14);       // (a stride: 25 cm)
        if (!ClimbOk(aCell, aX, aZ, x, z, aFeet, ground, mx, mz)) continue;
        if (aWalls && BlockedAt(aCell, x, z, 100, aFeet + sh, 7 * M / 4 - sh)) continue;
        n++;
        }
    return n;
    }

// (v0.4.74) where a door, a teleport or a new game puts the player: if the spot is closed in (inside a piece's walls -
// a ruin's door whose landing stands over its threshold, a rock), the nearest one that is not: a floor over it first
// (the landing's top), then a little on the way the player faces, then to the sides
void Game::FreeSpot()
    {
    int cell = iS.cell, x0 = iS.px, z0 = iS.pz, f0 = iS.py - 410;
    if (WaysOut(cell, x0, z0, f0) >= 4) return;
    static const signed char KOff[] = { 0, 0,  0, 5,  0, 10,  5, 0,  -5, 0,  0, 15,  5, 10,  -5, 10,  0, 20,  10, 0,  -10, 0,  0, -5,  0, 30,  0, -10,  10, 20,  -10, 20 };
    int fx = r3d_sin(iS.yaw), fz = r3d_cos(iS.yaw), k, up;
    for (k = 0; k < (int)sizeof(KOff); k += 2)
        {
        int side = KOff[k] * M / 10, fwd = KOff[k + 1] * M / 10;
        int x = x0 + ((fx * fwd + fz * side) >> 14), z = z0 + ((fz * fwd - fx * side) >> 14);
        for (up = 0; up <= 4; up++)
            {
            int f = FloorAt(cell, x, z, f0 + up * M / 2);
            if (up && f <= f0 + (up - 1) * M / 2) continue;          // (the same floor as the guess below)
            if (Abs(f - f0) > 3 * M) continue;
            if (WaysOut(cell, x, z, f) >= 4) { iS.px = x; iS.pz = z; iS.py = f + 410; iS.vy = 0; return; }
            }
        }
    }

int Game::BlockedAt(int aCell, int aX, int aZ, int aR, int aFeetY, int aHeight)
    {
    if (PropBlocked(aCell, aX, aZ, aR, aFeetY)) return 1;
    if (aCell == 0) return iR->Blocked(aX, aZ, aR, aFeetY, aFeetY + aHeight) || GameKitOutBlocked(*this, aX, aZ, aR, aFeetY, iR->iLandLoaded ? iR->iLandOffset : 0);
    if (IsDungeon(aCell) && DunBlocked(aCell, aX, aZ, aR, aFeetY)) return 1;      // (Skyrim's barrow: its gates and claw door only)
    const int* w = 0; int n = 0;
    if (aCell == CELL_STABLES) { w = KStablesWalls; n = sizeof(KStablesWalls) / sizeof(int) / 4; }
    else if (aCell == CELL_HALLDEAD) { w = KHallWalls; n = KHallWalls_N; }
    else if (aCell == CELL_CATACOMBS) { w = KCataWalls; n = KCataWalls_N; }
    int lx = (aX - IN_OX) * 100 / M, lz = (aZ - IN_OZ) * 100 / M, lr = aR * 100 / M;
    int i;
    if (aCell == CELL_SHOP || aCell == CELL_INN || aCell == CELL_HIGHHALL)
        {
        int fy = (aFeetY - IN_FLOOR) * 100 / M + 20;          // a little over the feet: the stairs' own steps never count
        const int* w6 = aCell == CELL_SHOP ? KShopWalls : (aCell == CELL_INN ? KInnWalls : KHHWall);
        int n6 = aCell == CELL_SHOP ? KShopWalls_N : (aCell == CELL_INN ? KInnWalls_N : KHHWall_N);
        for (i = 0, w = w6; i < n6; i++, w += 6)
            if (fy >= w[4] && fy < w[5] && lx + lr > w[0] && lx - lr < w[2] && lz + lr > w[1] && lz - lr < w[3]) return 1;
        }
    if (GameIsKitCell(*this, aCell) && GameKitBlocked(*this, aCell, aX, aZ, aR, aFeetY)) return 1;
    { int GameKitNav(const Game& g, int aCell); if (GameKitNav(*this, aCell)) return 0; }
    for (i = 0; i < n; i++, w += 4)
        if (lx + lr > w[0] && lx - lr < w[2] && lz + lr > w[1] && lz - lr < w[3]) return 1;
    int save = iR->iCell;
    iR->iCell = aCell;
    int b = iR->BlockedIn(aX, aZ, aR, aFeetY);
    iR->iCell = save;
    return b;
    }

// (v0.4.57) outside: Whiterun's area north of a line between Riverwood (its north edge 1965 m) and the outskirts
// (their south edge ~2222 m), Riverwood's south of it; between 2080 and 2120 m the one there is kept
int Game::AreaOutside(int aX, int aZ)
    {
    if (!iWrKit) return 0;
    int zm = aZ / METER;
    if (zm > 2120) return 2;
    if (zm < 2080) return 0;
    return iAreaCur == 2 || iAreaCur == 0 ? iAreaCur : iOutArea;
    }
// which area's file has a cell (outside: by where)
int Game::AreaOf(int aCell, int aX, int aZ)
    {
    if (iBfbKit && (aCell == CELL_BFTEMPLE || aCell == CELL_BFSANCTUM)) return 1;
    if (iWrKit && (aCell == CELL_STABLES || aCell == CELL_PELAGIA || aCell == CELL_CHILLFURROW || aCell == CELL_BATTLEBORN)) return 2;
    if (iFhKit && aCell >= CELL_FHSTRONG && aCell <= CELL_FHREFECT) return 3;     // (v0.4.68) Forelhost's halls
    return aCell ? 0 : AreaOutside(aX, aZ);
    }
// (v0.4.55) the area whose file has the cell's things (the barrow's own, or the main one), loaded if not in
void Game::UseArea(int aArea)
    {
    if (aArea == iAreaCur || !iAreaArena) return;
    R3D& r = *iR;
    r.iNFG = 0;                                     // (v0.4.74: the far groups' models are in the arena read over now)
    // the old area's cells: their lights out of the list
    int k, i, n = 0;
    for (i = 0; i < iNLSrc; i++)
        {
        int kit = 0;
        for (k = 0; k < iKitCellN; k++) if (iKitCells[k].cell != CELL_NONE && iKitCells[k].cell == iLSrc[i].cell) kit = 1;
        if (!kit) iLSrc[n++] = iLSrc[i];
        }
    iNLSrc = n;
    r.iNumDefs = iAreaDefs; r.iArena = iAreaArena; r.iINum = iAreaIObj;
    if (!LoadArea(aArea == 1 ? "bleakfalls.har" : aArea == 2 ? "whiterun.har" : aArea == 3 ? "forelhost.har" : "riverwood.har")) { iAreaCur = -1; return; }
    BuildKit();
    iAreaCur = aArea;
    // (v0.4.57) outside: the other area's pieces taken off, this one's put out (the barrow has none: theirs stay, and
    // are the same again when their area comes back)
    if (aArea != 1 && aArea != 3 && aArea != iOutArea && iOutObjMark >= 0)     // (the ruins, 1 and 3: halls only)
        {
        r.iObjCount = iOutObjMark;
        PlaceKitOutside();
        iOutObjN = r.iObjCount - iOutObjMark;
        r.RelinkStatics();
        iOutArea = aArea;
        }
    // (v0.4.74) the outside's far groups, from the pieces this area put out
    if ((aArea == 0 || aArea == 2) && iOutArea == aArea && iOutObjMark >= 0) r.BuildFarGroups(iOutObjMark, iOutObjMark + iOutObjN);
    // the barrow's own things lying about (its goods, its urns' contents): put out the first time its file is in
    if (aArea == 1 && !iS.bfb[BF_KITSPAWN]) { SpawnKit(); iS.bfb[BF_KITSPAWN] = 1; }
    if (aArea == 1 || aArea == 3) { void GameBarrowLids(Game& g); GameBarrowLids(*this); r3d_memset(iLidOpen, 0, sizeof(iLidOpen)); }
    if (aArea == 2 && !iS.bfb[BF_WRSPAWN]) { SpawnKit(); iS.bfb[BF_WRSPAWN] = 1; }
    if (aArea == 3 && !iS.dun[FH_KITSPAWN]) { SpawnKit(); iS.dun[FH_KITSPAWN] = 1; }      // (v0.4.74) Forelhost's: its doors, its goods
    // (v0.4.67) the city's people outside stand on its floors now in (they were put out on the land's)
    {
    int i;
    for (i = 0; i < iS.ne; i++)
        {
        Ent& e = iS.e[i];
        if (e.grp == G_WRFOLK && e.cell == 0 && e.alive) e.y = FloorAt(0, e.x, e.z, iR->GroundAt(e.x, e.z) + 3 * M / 2);
        }
    }
    }

// (v0.4.74) what needs an area's file: put off till a frame has shown the loading screen (when the app wants that);
// 1 when put off
int Game::DeferLoad(int aKind, int a0, int a1, int a2, int a3, int a4)
    {
    if (!iDeferLoads || iLoadPhase) return 0;
    iLoadKind = aKind; iLoadA[0] = a0; iLoadA[1] = a1; iLoadA[2] = a2; iLoadA[3] = a3; iLoadA[4] = a4;
    iLoadPhase = 1;
    return 1;
    }
void Game::DoPendingLoad()
    {
    int d = iDeferLoads;
    iDeferLoads = 0; iLoadPhase = 0;
    switch (iLoadKind)
        {
        case 1: EnterCell(iLoadA[0], iLoadA[1], iLoadA[2], iLoadA[3], iLoadA[4]); break;
        case 2: UseArea(iLoadA[0]); break;
        case 3: LoadFinish(); break;
        case 4: Teleport(iLoadA[0]); break;
        }
    iDeferLoads = d;
    }
// into a cell or out, by a door, the map or the travel list: a loading screen first if a file is to be read
void Game::Go(int aCell, int aX, int aZ, int aYaw, int aFeet)
    {
    if (iAreaArena && AreaOf(aCell, aX, aZ) != iAreaCur && DeferLoad(1, aCell, aX, aZ, aYaw, aFeet)) return;
    EnterCell(aCell, aX, aZ, aYaw, aFeet);
    }

void Game::EnterCell(int aCell, int aX, int aZ, int aYaw, int aFeet)
    {
    UseArea(AreaOf(aCell, aX, aZ));
    if (iS.cell == 0 && aCell != 0) { iS.extX = iS.px; iS.extZ = iS.pz; iS.extYaw = iS.yaw; }
    if (aCell != iS.cell) RiverwoodSchedule(1);                  // behind the door, everyone where the hour wants them
    iS.cell = aCell;
    iR->iCell = aCell;
    iDoorFocus = -1;                                            // (v0.4.74: the last place's door no longer in reach)
    iS.px = aX; iS.pz = aZ; iS.yaw = aYaw & 1023; iS.pitch = 0;
    if (aCell == 0) iR->FocusLand(aX, aZ);                      // out of a door: the land round it first
    // outside, a door may open onto a raised deck or porch: the highest floor up to 2 m over the land
    // (a door that knows its floor: from just over it, so an upstairs door leads to the floor it is on)
    int guess = aFeet != 0x7fffffff ? aFeet + M / 2 : GameIsKitCell(*this, aCell) ? GameKitEntryFeet(*this, aCell) : (aCell ? 0 : iR->GroundAt(aX, aZ) + 2 * M);
    iS.py = FloorAt(aCell, aX, aZ, guess) + 410; iS.vy = 0;
    FreeSpot();                                                 // (v0.4.74) never into a piece's walls
    iCamDist = 0;
    iFadeUs = 350000;
    if (aCell == CELL_RWTRADER) RwTraderEnter();
    int i;
    for (i = 0; i < MAXP; i++) iP[i].active = 0;
    UpdateStage();                                              // (v0.4.67) the place's people out, the last one's in
    UpdateAtmosphere();
    }

// ---------------------------------------------------------------- interior furniture

static void Box(R3D& r, Mesh& m, int slot, int x, int y0, int z, int hx, int h, int hz)
    {
    r.MeshBox(m, 0, slot, x, y0 + h, z, 0, hx, h / 2, hz);
    }
static void BoxC(R3D& r, Mesh& m, int slot, int x0, int y0, int z0, int x1, int y1, int z1)
    {
    // box from corner to corner, centimetres
    Box(r, m, slot, Cm(x0 + x1) / 2, Cm(y0), Cm(z0 + z1) / 2, Cm(x1 - x0) / 2, Cm(y1 - y0), Cm(z1 - z0) / 2);
    }
static void RodF(R3D& r, Mesh& m, int fr, int slot, int x0, int y0, int z0, int x1, int y1, int z1, int rad, int sides)
    {
    r.MeshLimb(m, fr, slot, Cm(x0), Cm(y0), Cm(z0), Cm(x1), Cm(y1), Cm(z1), Cm(rad), Cm(rad), Cm(rad), Cm(rad), sides, 3);
    }
// box corner to corner (cm) into a given frame
static void BoxCF(R3D& r, Mesh& m, int fr, int slot, int x0, int y0, int z0, int x1, int y1, int z1)
    {
    r.MeshBox(m, fr, slot, Cm(x0 + x1) / 2, Cm(y1), Cm(z0 + z1) / 2, 0, Cm(x1 - x0) / 2, Cm(y1 - y0) / 2, Cm(z1 - z0) / 2);
    }
// a door leaf (both faces) from a hinge at (hx, hz), width w along direction (dx, dz) (Q14), y0..y1, cm
static void Leaf(R3D& r, Mesh& m, int fr, int slot, int hx, int hz, int w, int dx, int dz, int y0, int y1, int t)
    {
    int nx = -dz, nz = dx;                                   // across the leaf
    int ex = (dx * w) >> 14, ez = (dz * w) >> 14, ox = (nx * t) >> 15, oz = (nz * t) >> 15;
    int i;
    for (i = -1; i <= 1; i += 2)
        {
        int sx = hx + ox * i, sz = hz + oz * i;
        int q[12] = { Cm(sx), Cm(y0), Cm(sz),  Cm(sx + ex), Cm(y0), Cm(sz + ez),  Cm(sx + ex), Cm(y1), Cm(sz + ez),  Cm(sx), Cm(y1), Cm(sz) };
        // centre on the far side, so each face looks outward
        r.MeshQuad(m, fr, slot, q, Cm(hx + ex / 2 - ((nx * 40) >> 14) * i), Cm((y0 + y1) / 2), Cm(hz + ez / 2 - ((nz * 40) >> 14) * i));
        }
    }
static void Rod(R3D& r, Mesh& m, int slot, int x0, int y0, int z0, int x1, int y1, int z1, int rad, int sides)
    {
    r.MeshLimb(m, 0, slot, Cm(x0), Cm(y0), Cm(z0), Cm(x1), Cm(y1), Cm(z1), Cm(rad), Cm(rad), Cm(rad), Cm(rad), sides, 3);
    }


// ---- animal hides, the way Skyrim furnishes a Nord house: hung on the walls, laid over beds, as rugs.
// The hide is a 7 x 5 grid: along it tail .. hind legs .. back .. forelegs .. neck, across it the flanks;
// the leg columns reach out further, which gives the outline of a skin. Darker fur runs along the spine.
// mode 0: hung on a wall (length vertical, facing +z), centre (cx, cy, cz)
// mode 1: draped over a bed: lies at height top across |x| < bh, the flanks fall over the sides;
//         length along z centred on cz
// mode 2: a rug on the floor (with the head)
enum { HIDE_WALL = 0, HIDE_DRAPE, HIDE_RUG };
static void HidePoint(int mode, int U, int V, int v100, int cx, int cy, int cz, int top, int bh, int* o)
    {
    int fold = (r3d_sin(U * 9 + V * 5) * 3) >> 14;                 // gentle folds (cm)
    if (mode == HIDE_WALL)
        {
        o[0] = Cm(cx + V); o[1] = Cm(cy + U); o[2] = Cm(cz + 2 + (3 * (100 * 100 - v100 * v100)) / 10000 + fold / 2);
        return;
        }
    int X = V, ax = X < 0 ? -X : X, sg = X < 0 ? -1 : 1, y;
    if (mode == HIDE_RUG || ax <= bh) { y = top + (mode == HIDE_RUG ? 0 : fold + 1); o[0] = Cm(cx + X); }
    else
        {
        int over = ax - bh, drop = over * 9 / 10;
        if (top - drop > 1) { y = top - drop + fold; o[0] = Cm(cx + sg * (bh + over / 4)); }
        else
            {
            // reached the floor: the rest lies on it
            int hang = (top - 1) * 10 / 9;
            y = 1; o[0] = Cm(cx + sg * (bh + hang / 4 + over - hang));
            }
        }
    o[1] = Cm(y); o[2] = Cm(cz + U);
    }
static void MeshHide(R3D& r, Mesh& m, int mode, int sFur, int sSpine, int cx, int cy, int cz, int len, int wid, int top, int bh)
    {
    static const int KU[7] = { -100, -72, -40, 0, 40, 72, 100 };       // tail .. neck (% of half length)
    static const int KW[7] = { 16, 104, 72, 80, 74, 100, 34 };         // half width there (%)
    static const int KV[5] = { -100, -45, 0, 45, 100 };
    int P[7 * 5 * 3], i, j;
    for (i = 0; i < 7; i++)
        for (j = 0; j < 5; j++)
            {
            int v = KV[j];
            int w = (j == 0 || j == 4) ? KW[i] : (KW[i] < 80 ? KW[i] : 80);   // only the edge reaches out at the legs
            int U = KU[i] * len / 100, V = v * w / 100 * wid / 100;
            HidePoint(mode, U, V, v, cx, cy, cz, top, bh, P + (i * 5 + j) * 3);
            }
    int ccx = Cm(cx), ccy = mode == HIDE_WALL ? Cm(cy) : Cm(top - 80), ccz = mode == HIDE_WALL ? Cm(cz - 60) : Cm(cz);
    for (i = 0; i < 6; i++)
        for (j = 0; j < 4; j++)
            {
            int q[12], k;
            const int* a = P + (i * 5 + j) * 3; const int* b = P + (i * 5 + j + 1) * 3;
            const int* c = P + ((i + 1) * 5 + j + 1) * 3; const int* d = P + ((i + 1) * 5 + j) * 3;
            for (k = 0; k < 3; k++) { q[k] = a[k]; q[3 + k] = b[k]; q[6 + k] = c[k]; q[9 + k] = d[k]; }
            r.MeshQuad(m, 0, (j == 1 || j == 2) ? sSpine : sFur, q, ccx, ccy, ccz);
            }
    if (mode == HIDE_RUG)
        {
        // the head at the neck end, flat on the floor, with ears and a snout
        int hz = cz + len + 10;
        r.MeshEllipsoid(m, 0, sSpine, Cm(cx), Cm(top + 7), Cm(hz), Cm(13), Cm(9), Cm(15), 6, 3);
        r.MeshLimb(m, 0, sSpine, Cm(cx), Cm(top + 6), Cm(hz + 10), Cm(cx), Cm(top + 5), Cm(hz + 26), Cm(7), Cm(5), Cm(4), Cm(3), 5, 2);
        r.MeshEllipsoid(m, 0, sFur, Cm(cx - 9), Cm(top + 15), Cm(hz - 6), Cm(4), Cm(4), Cm(2), 4, 2);
        r.MeshEllipsoid(m, 0, sFur, Cm(cx + 9), Cm(top + 15), Cm(hz - 6), Cm(4), Cm(4), Cm(2), 4, 2);
        }
    }

static void Def0(StaticDef& d, const Mesh* m, int hxCm, int hzCm, int radCm, int hyCm)
    {
    r3d_memset(&d, 0, sizeof(d));
    d.mesh = m; d.lod = 0; d.hx = (short)Cm(hxCm); d.hz = (short)Cm(hzCm); d.rad = (short)Cm(radCm); d.hy = (short)Cm(hyCm);
    d.drawDist = 60; d.lodDist = 0;
    }
static void Col(StaticDef& d, int s, int r, int g, int b, int t)
    {
    d.slots[s * 4] = (u8)r; d.slots[s * 4 + 1] = (u8)g; d.slots[s * 4 + 2] = (u8)b; d.slots[s * 4 + 3] = (u8)t;
    }
#define PLANK(d, s)  Col(d, s, 120, 88, 58, SL_TEX | TXI_PLANKS)
#define TIMBER(d, s) Col(d, s, 104, 76, 50, SL_TEX | TXI_LOGS)
#define DARKW(d, s)  Col(d, s, 64, 46, 32, 4)
#define STONEC(d, s) Col(d, s, 120, 116, 108, SL_TEX | 1)

void Game::BuildInteriors()
    {
    R3D& r = *iR;
    iNLSrc = 0;
    Mesh* im = iMIn;
    StaticDef d;
    int i;
    // ---- stables residence shell: plank floor, timber walls, ceiling, rafters, partitions, door
    {
    Mesh& m = im[IN_SHELL];
    r.MeshInit(m, 240, 360, 1);
    BoxC(r, m, 0, -630, -20, -480, 790, 0, 480);              // floor
    BoxC(r, m, 1, -630, 330, -480, 790, 350, 480);            // ceiling
    BoxC(r, m, 2, -630, 0, -480, -260, 330, -450);            // south wall, door gap -260..-140
    BoxC(r, m, 2, -140, 0, -480, 790, 330, -450);
    BoxC(r, m, 2, -260, 225, -480, -140, 330, -450);          // over the door
    BoxC(r, m, 2, -630, 0, 450, 790, 330, 480);               // north
    BoxC(r, m, 2, -630, 0, -480, -600, 330, 480);             // west
    BoxC(r, m, 2, 760, 0, -480, 790, 330, 480);               // east
    BoxC(r, m, 2, 200, 0, -450, 220, 330, -260);              // partition with two doorways
    BoxC(r, m, 2, 200, 0, -150, 220, 330, 150);
    BoxC(r, m, 2, 200, 0, 260, 220, 330, 450);
    BoxC(r, m, 2, 200, 220, -260, 220, 330, -150);
    BoxC(r, m, 2, 200, 220, 150, 220, 330, 260);
    BoxC(r, m, 2, 220, 0, -10, 760, 330, 10);                 // bedroom wall
    for (i = 0; i < 7; i++)                                     // rafters across the room
        {
        int x = -500 + i * 200;
        BoxC(r, m, 3, x - 12, 295, -450, x + 12, 330, 450);
        }
    BoxC(r, m, 3, -600, 295, -12, 760, 320, 12);              // ridge beam
    for (i = 0; i < 4; i++)                                     // corner posts
        {
        int x = i & 1 ? 745 : -585, z = i & 2 ? 435 : -435;
        BoxC(r, m, 3, x - 15, 0, z - 15, x + 15, 330, z + 15);
        }
    BoxC(r, m, 4, -255, 0, -458, -145, 220, -450);            // the door (inside face)
    BoxC(r, m, 3, -270, 0, -455, -255, 228, -440);            // door frame
    BoxC(r, m, 3, -145, 0, -455, -130, 228, -440);
    BoxC(r, m, 3, -270, 220, -455, -130, 232, -440);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 0, 0);
    PLANK(d, 0); Col(d, 1, 70, 54, 40, 4); TIMBER(d, 2); DARKW(d, 3); Col(d, 4, 86, 60, 38, SL_TEX | TXI_PLANKS);
    iInType[IN_SHELL] = r.RegisterStatic(d);
    }
    // ---- firepit with a cooking spit
    {
    Mesh& m = im[IN_FIREPIT];
    r.MeshInit(m, 120, 180, 1);
    r.MeshLimb(m, 0, 0, 0, 0, 0, 0, Cm(22), 0, Cm(68), Cm(68), Cm(64), Cm(64), 10, 0);    // stone ring
    r.MeshConeAt(m, 0, 1, 0, 0, Cm(64), Cm(22), Cm(22), 10, 0);                              // top of the ring
    r.MeshConeAt(m, 0, 5, 0, 0, Cm(48), Cm(23), Cm(24), 10, 0);                              // embers
    Rod(r, m, 2, -32, 28, -14, 28, 31, 18, 7, 5);                                               // logs
    Rod(r, m, 2, -28, 30, 21, 32, 28, -11, 7, 5);
    Rod(r, m, 2, -78, 0, 0, -78, 82, 0, 4, 4);                                                  // spit posts
    Rod(r, m, 2, 78, 0, 0, 78, 82, 0, 4, 4);
    Rod(r, m, 3, -90, 78, 0, 90, 78, 0, 2, 4);                                                  // spit
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 72, 60);
    STONEC(d, 0); Col(d, 1, 96, 92, 86, 1); DARKW(d, 2); Col(d, 3, 60, 60, 64, 1); Col(d, 4, 150, 84, 40, 1);
    Col(d, 5, 255, 110, 30, 1);
    iInType[IN_FIREPIT] = r.RegisterStatic(d);
    }
    // ---- flame (shared animated mesh, unlit)
    Def0(d, &iMFlame, 0, 0, 0, 0);
    Col(d, 0, 255, 200, 70, 1); Col(d, 1, 240, 96, 24, 1); Col(d, 2, 255, 246, 200, 1);
    d.flags = SD_ANIM | SD_UNLIT; d.animMs = 90;
    d.drawDist = 12;            // a candle's flame: only near (its light still reaches the room); a hearth's, by its size
    iInType[IN_FLAME] = r.RegisterStatic(d);
    d.mesh = &iMFlameSmall;
    iSmallFlameType = r.RegisterStatic(d);
    // ---- round table with two chairs, a plate and a mug
    {
    Mesh& m = im[IN_TABLE];
    r.MeshInit(m, 80, 120, 1);
    r.MeshLimb(m, 0, 0, 0, Cm(72), 0, 0, Cm(78), 0, Cm(70), Cm(70), Cm(70), Cm(70), 10, 3);
    Rod(r, m, 1, 0, 0, 0, 0, 72, 0, 8, 5);
    r.MeshLimb(m, 0, 1, 0, 0, 0, 0, Cm(4), 0, Cm(30), Cm(30), Cm(30), Cm(30), 6, 2);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 75, 80);
    PLANK(d, 0); DARKW(d, 1); Col(d, 2, 150, 150, 156, 1); Col(d, 3, 110, 80, 52, 4);
    iInType[IN_TABLE] = r.RegisterStatic(d);
    }
    // ---- chair (a prop you can sit on): seat faces local -z, back at +z
    {
    Mesh& m = im[IN_CHAIR];
    r.MeshInit(m, 80, 160, 1);
    BoxC(r, m, 0, -22, 42, -22, 22, 48, 22);
    BoxC(r, m, 1, -22, 48, 16, 22, 100, 22);
    BoxC(r, m, 1, -20, 60, 18, 20, 66, 21); BoxC(r, m, 1, -20, 84, 18, 20, 90, 21);
    int k;
    for (k = 0; k < 4; k++) BoxC(r, m, 1, k & 1 ? 16 : -20, 0, k & 2 ? 16 : -20, k & 1 ? 20 : -16, 42, k & 2 ? 20 : -16);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 0, 0);
    PLANK(d, 0); DARKW(d, 1);
    iInType[IN_CHAIR] = r.RegisterStatic(d);
    }
    // ---- long table with a bench, bread, a bowl and a jug
    {
    Mesh& m = im[IN_LONGTABLE];
    r.MeshInit(m, 120, 180, 1);
    BoxC(r, m, 0, -110, 72, -45, 110, 80, 45);
    int k;
    for (k = 0; k < 4; k++) BoxC(r, m, 1, k & 1 ? 92 : -100, 0, k & 2 ? 30 : -38, k & 1 ? 100 : -92, 72, k & 2 ? 38 : -30);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 110, 45, 0, 80);
    PLANK(d, 0); DARKW(d, 1); Col(d, 2, 196, 146, 80, 1); Col(d, 3, 120, 90, 60, 4); Col(d, 4, 170, 96, 60, 1);
    iInType[IN_LONGTABLE] = r.RegisterStatic(d);
    }
    // ---- beds (as in Skyrim's Nord houses): plank frame, straw-and-linen mattress, a big fur thrown over it
    // that hangs down the sides, linen pillows at the head; double and single
    {
    int k;
    for (k = 0; k < 2; k++)
        {
        Mesh& m = im[k ? IN_BED1 : IN_BED2];
        int hx = k ? 50 : 80;
        r.MeshInit(m, 200, 320, 1);
        BoxC(r, m, 0, -hx, 0, -105, hx, 32, 105);                          // frame
        BoxC(r, m, 2, -hx + 5, 32, -100, hx - 5, 46, 98);                  // mattress
        MeshHide(r, m, HIDE_DRAPE, 1, 4, 0, 0, -18, 82, hx + 26, 48, hx - 2);   // the fur, from the foot to the pillows
        r.MeshEllipsoid(m, 0, 2, Cm(k ? 0 : -35), Cm(52), Cm(86), Cm(26), Cm(7), Cm(12), 6, 2);   // pillows
        if (!k) r.MeshEllipsoid(m, 0, 2, Cm(35), Cm(52), Cm(86), Cm(26), Cm(7), Cm(12), 6, 2);
        BoxC(r, m, 3, -hx - 4, 0, 100, hx + 4, 105, 110);                  // headboard
        BoxC(r, m, 3, -hx - 4, 0, -110, hx + 4, 55, -100);                 // footboard
        r.MeshFinish(m);
        r.MeshPlanarUV(m, 2);
        Def0(d, &m, hx, 108, 0, 60);
        PLANK(d, 0); Col(d, 1, k ? 128 : 112, k ? 96 : 86, k ? 66 : 70, SL_TEX | TXI_FUR); Col(d, 2, 210, 200, 178, 6);
        DARKW(d, 3); Col(d, 4, k ? 96 : 82, k ? 72 : 62, k ? 50 : 50, SL_TEX | TXI_FUR);
        iInType[k ? IN_BED1 : IN_BED2] = r.RegisterStatic(d);
        }
    }
    // ---- wardrobe: two doors that swing open (5 frames), shelves inside
    {
    Mesh& m = im[IN_CUPBOARD];
    r.MeshInit(m, 160, 320, 5);
    int fr;
    for (fr = 0; fr < 5; fr++)
        {
        r.iMeshCursor = 0;
        BoxCF(r, m, fr, 0, -60, 0, 23, 60, 190, 27);          // back
        BoxCF(r, m, fr, 0, -62, 0, -25, -56, 190, 25);        // sides
        BoxCF(r, m, fr, 0, 56, 0, -25, 62, 190, 25);
        BoxCF(r, m, fr, 1, -64, 190, -29, 64, 198, 29);       // top
        BoxCF(r, m, fr, 1, -56, 0, -25, 56, 10, 23);          // bottom
        BoxCF(r, m, fr, 3, -56, 95, -22, 56, 99, 23);         // shelf
        int a2 = fr * 64, ca = r3d_cos(a2), sa = r3d_sin(a2);
        Leaf(r, m, fr, 2, -56, -27, 56, ca, -sa, 10, 182, 3);   // left door, hinge at the left
        Leaf(r, m, fr, 2, 56, -27, 56, -ca, -sa, 10, 182, 3);   // right door
        }
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 62, 28, 0, 200);
    Col(d, 0, 92, 52, 34, SL_TEX | TXI_FURN); Col(d, 1, 64, 40, 28, 4); Col(d, 2, 104, 60, 38, SL_TEX | TXI_FURN); Col(d, 3, 70, 46, 30, 4);
    iInType[IN_CUPBOARD] = r.RegisterStatic(d);
    }
    // ---- wall shelf (what stands on it are props)
    {
    Mesh& m = im[IN_SHELF];
    r.MeshInit(m, 240, 400, 1);
    BoxC(r, m, 0, -75, 100, -15, 75, 106, 15);
    BoxC(r, m, 0, -75, 155, -15, 75, 161, 15);
    BoxC(r, m, 1, -72, 60, 8, -66, 165, 15); BoxC(r, m, 1, 66, 60, 8, 72, 165, 15);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 0, 0);
    TIMBER(d, 0); DARKW(d, 1); Col(d, 2, 168, 96, 60, 1); Col(d, 3, 120, 90, 60, 4); Col(d, 4, 150, 150, 156, 1);
    iInType[IN_SHELF] = r.RegisterStatic(d);
    }
    // ---- chest (the treasure chest model, closed)
    Def0(d, &iMChest, 69, 35, 0, 68);
    Col(d, 0, 92, 52, 34, SL_TEX | TXI_FURN); Col(d, 1, 58, 56, 56, 1); Col(d, 2, 104, 60, 38, SL_TEX | TXI_FURN); Col(d, 3, 24, 16, 12, 1);
    iInType[IN_CHEST] = r.RegisterStatic(d);
    // ---- dresser and end table
    {
    Mesh& m = im[IN_DRESSER];
    r.MeshInit(m, 120, 240, 5);
    int fr;
    for (fr = 0; fr < 5; fr++)
        {
        r.iMeshCursor = 0;
        BoxCF(r, m, fr, 0, -55, 0, -22, 55, 85, 25);
        BoxCF(r, m, fr, 1, -58, 85, -28, 58, 91, 28);
        BoxCF(r, m, fr, 2, -50, 8, -26, 50, 28, -22);         // lower drawers
        BoxCF(r, m, fr, 2, -50, 34, -26, 50, 54, -22);
        int o = fr * 7;                                         // the top drawer slides out
        BoxCF(r, m, fr, 2, -50, 60, -26 - o, 50, 80, -22 - o);
        BoxCF(r, m, fr, 3, -47, 60, -22 - o, 47, 62, 20 - o);   // its bottom, seen when open
        BoxCF(r, m, fr, 1, -6, 68, -29 - o, 6, 72, -26 - o);   // handle
        }
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 55, 25, 0, 90);
    Col(d, 0, 92, 52, 34, SL_TEX | TXI_FURN); Col(d, 1, 60, 40, 28, 4); Col(d, 2, 108, 64, 40, SL_TEX | TXI_FURN); Col(d, 3, 50, 34, 24, 1);
    iInType[IN_DRESSER] = r.RegisterStatic(d);
    Mesh& t = im[IN_ENDTABLE];
    r.MeshInit(t, 40, 60, 1);
    BoxC(r, t, 0, -25, 0, -25, 25, 55, 25);
    BoxC(r, t, 1, -28, 55, -28, 28, 60, 28);
    r.MeshLimb(t, 0, 2, 0, Cm(60), 0, 0, Cm(72), 0, Cm(3), Cm(3), Cm(3), Cm(3), 5, 2);       // candle
    r.MeshFinish(t);
    r.MeshPlanarUV(t, 2);
    Def0(d, &t, 25, 25, 0, 60);
    TIMBER(d, 0); DARKW(d, 1); Col(d, 2, 230, 220, 190, 1);
    iInType[IN_ENDTABLE] = r.RegisterStatic(d);
    }
    // ---- display case with a dagger and a ring under the glass
    {
    Mesh& m = im[IN_DISPLAY];
    r.MeshInit(m, 140, 240, 1);
    BoxC(r, m, 0, -55, 0, -25, 55, 80, 25);
    BoxC(r, m, 1, -55, 80, -25, 55, 82, 25);                  // velvet (what lies in it is drawn on top)
    BoxC(r, m, 3, -55, 100, -25, 55, 103, -21);               // the glass lid's frame
    BoxC(r, m, 3, -55, 100, 21, 55, 103, 25);
    BoxC(r, m, 3, -55, 100, -21, -51, 103, 21);
    BoxC(r, m, 3, 51, 100, -21, 55, 103, 21);
    for (i = 0; i < 4; i++) BoxC(r, m, 0, i & 1 ? 50 : -55, 80, i & 2 ? 20 : -25, i & 1 ? 55 : -50, 102, i & 2 ? 25 : -20);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 55, 25, 0, 100);
    Col(d, 0, 92, 52, 34, SL_TEX | TXI_FURN); Col(d, 1, 110, 30, 34, 6); Col(d, 2, 200, 204, 214, 1); DARKW(d, 3); Col(d, 4, 150, 180, 196, 1);
    iInType[IN_DISPLAY] = r.RegisterStatic(d);
    }
    // ---- garlic braids and salted salmon hanging from a rafter
    {
    Mesh& m = im[IN_HANG];
    r.MeshInit(m, 240, 400, 1);
    for (i = 0; i < 2; i++)
        {
        int x = -60 + i * 30;
        Rod(r, m, 0, x, 295, 0, x, 210, 0, 1, 3);
        int k;
        for (k = 0; k < 4; k++) r.MeshEllipsoid(m, 0, 1, Cm(x + (k & 1 ? 4 : -4)), Cm(270 - k * 16), 0, Cm(5), Cm(6), Cm(5), 5, 2);
        }
    for (i = 0; i < 2; i++)
        {
        int x = 30 + i * 35;
        Rod(r, m, 0, x, 295, 0, x, 255, 0, 1, 3);
        r.MeshEllipsoid(m, 0, 2, Cm(x), Cm(220), 0, Cm(9), Cm(34), Cm(4), 6, 3);
        r.MeshLimb(m, 0, 3, Cm(x), Cm(188), 0, Cm(x), Cm(176), 0, Cm(4), Cm(2), Cm(9), Cm(2), 4, 2);
        }
    r.MeshFinish(m);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 170, 150, 110, 1); Col(d, 1, 232, 226, 206, 1); Col(d, 2, 210, 120, 80, 1); Col(d, 3, 150, 150, 160, 1);
    iInType[IN_HANG] = r.RegisterStatic(d);
    }
    // ---- barrel, rug, candle stick, grain sacks
    {
    Mesh& m = im[IN_BARREL];
    r.MeshInit(m, 170, 230, 5);
    int fr;
    for (fr = 0; fr < 5; fr++)
        {
        r.iMeshCursor = 0;
        // staves: bulging profile, narrower towards the rims
        int R[12 * 6], *q = R, k;
        static const int KProf[6 * 2] = { 0, 30,  18, 35,  45, 38,  72, 35,  88, 31,  92, 30 };
        for (k = 0; k < 6; k++)
            {
            q[0] = 0; q[1] = Cm(KProf[k * 2]); q[2] = 0; q[3] = 16384; q[4] = 0; q[5] = 0; q[6] = 0; q[7] = 0; q[8] = 16384;
            q[9] = Cm(KProf[k * 2 + 1]); q[10] = Cm(KProf[k * 2 + 1]); q[11] = 0;
            q += 12;
            }
        r.MeshRings(m, fr, R, 6, 7, 1);
        // iron hoops
        r.MeshLimb(m, fr, 1, 0, Cm(12), 0, 0, Cm(17), 0, Cm(34), Cm(34), Cm(35), Cm(35), 7, 0);
        r.MeshLimb(m, fr, 1, 0, Cm(75), 0, 0, Cm(80), 0, Cm(34), Cm(34), Cm(33), Cm(33), 7, 0);
        r.MeshLimb(m, fr, 1, 0, Cm(86), 0, 0, Cm(90), 0, Cm(32), Cm(32), Cm(31), Cm(31), 7, 0);
        // the dark inside, then the lid, which slides aside as the barrel opens
        r.MeshConeAt(m, fr, 3, 0, 0, Cm(28), Cm(88), Cm(88), 7, 0);
        int off = Cm(fr * 9), up = fr ? Cm(2) : 0;
        r.MeshLimb(m, fr, 2, off, Cm(88) + up, 0, off, Cm(92) + up, 0, Cm(29), Cm(29), Cm(29), Cm(29), 7, 3);
        }
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 38, 92);
    Col(d, 0, 118, 84, 52, SL_TEX | TXI_PLANKS); Col(d, 1, 56, 56, 60, 1); Col(d, 2, 112, 80, 50, SL_TEX | TXI_PLANKS); Col(d, 3, 20, 14, 10, 1);
    iInType[IN_BARREL] = r.RegisterStatic(d);
    }
    // ---- garlic braid: 3 frames (full, half, bare string); taken bulbs shrink to nothing
    {
    Mesh& m = im[IN_GARLIC];
    r.MeshInit(m, 200, 400, 3);
    int fr;
    for (fr = 0; fr < 3; fr++)
        {
        r.iMeshCursor = 0;
        RodF(r, m, fr, 0, 0, 0, 0, 0, -95, 0, 1, 3);
        int k;
        for (k = 0; k < 8; k++)
            {
            int present = fr == 0 || (fr == 1 && k >= 4);
            int sz = present ? 1 : 0;
            int y = -30 - k * 9;
            int x = (k & 1) ? 5 : -5;
            r.MeshEllipsoid(m, fr, 1, Cm(x) * sz, Cm(y), 0, sz ? Cm(5) : 1, sz ? Cm(6) : 1, sz ? Cm(5) : 1, 4, 2);
            }
        r.MeshLimb(m, fr, 0, 0, Cm(-22), 0, 0, Cm(-30), 0, Cm(3), Cm(3), Cm(1), Cm(1), 3, 0);       // knot
        }
    r.MeshFinish(m);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 170, 150, 110, 1); Col(d, 1, 236, 228, 206, 1);
    iInType[IN_GARLIC] = r.RegisterStatic(d);
    }
    // ---- hanging salmon on a hook
    {
    Mesh& m = im[IN_FISH];
    r.MeshInit(m, 50, 70, 1);
    Rod(r, m, 0, 0, 0, 0, 0, -38, 0, 1, 3);
    r.MeshEllipsoid(m, 0, 1, 0, Cm(-72), 0, Cm(9), Cm(34), Cm(4), 6, 3);
    r.MeshLimb(m, 0, 2, 0, Cm(-104), 0, 0, Cm(-116), 0, Cm(4), Cm(2), Cm(9), Cm(2), 4, 2);
    r.MeshLimb(m, 0, 2, 0, Cm(-40), 0, 0, Cm(-46), 0, Cm(2), Cm(2), Cm(5), Cm(3), 4, 2);
    r.MeshFinish(m);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 170, 150, 110, 1); Col(d, 1, 214, 120, 82, 1); Col(d, 2, 150, 150, 160, 1);
    iInType[IN_FISH] = r.RegisterStatic(d);
    }
    // ---- hanging haunch of venison on a cord
    {
    Mesh& m = im[IN_HAM];
    r.MeshInitSmooth(m, 60, 120, 1);
    Rod(r, m, 0, 0, 0, 0, 0, -30, 0, 1, 3);
    r.MeshEllipsoid(m, 0, 1, 0, Cm(-62), 0, Cm(12), Cm(22), Cm(10), 6, 4);
    r.MeshLimb(m, 0, 2, 0, Cm(-30), 0, 0, Cm(-44), 0, Cm(3), Cm(3), Cm(4), Cm(4), 5, 0);
    r.MeshEllipsoid(m, 0, 2, 0, Cm(-30), 0, Cm(4), Cm(3), Cm(4), 4, 2);
    r.MeshFinish(m);
    r.MeshSmooth(m);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 170, 150, 110, 1); Col(d, 1, 160, 58, 52, 1); Col(d, 2, 226, 214, 196, 1);
    iInType[IN_HAM] = r.RegisterStatic(d);
    }
    // ---- bench (a prop you can sit on; the sitter faces local -z)
    {
    Mesh& m = im[IN_BENCH];
    r.MeshInit(m, 40, 80, 1);
    BoxC(r, m, 0, -100, 40, -13, 100, 46, 13);
    BoxC(r, m, 1, -90, 0, -8, -82, 40, 8); BoxC(r, m, 1, 82, 0, -8, 90, 40, 8);
    BoxC(r, m, 1, -82, 18, -3, 82, 24, 3);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 0, 0);
    PLANK(d, 0); DARKW(d, 1);
    iInType[IN_BENCH] = r.RegisterStatic(d);
    }
    // ---- room door: a plank leaf on a hinge at the origin, closed along +z, opening towards +x (5 frames)
    {
    Mesh& m = im[IN_DOOR];
    r.MeshInit(m, 80, 160, 5);
    int fr;
    for (fr = 0; fr < 5; fr++)
        {
        r.iMeshCursor = 0;
        int a2 = fr * 64, ca = r3d_cos(a2), sa = r3d_sin(a2);
        Leaf(r, m, fr, 0, 0, 0, 108, sa, ca, 0, 218, 5);
        // iron straps and a ring pull on both faces
        int k;
        for (k = 0; k < 2; k++)
            {
            int y = 40 + k * 130;
            Leaf(r, m, fr, 1, 0, 0, 96, sa, ca, y, y + 8, 7);
            }
        Leaf(r, m, fr, 1, (sa * 90) >> 14, (ca * 90) >> 14, 8, sa, ca, 100, 112, 9);
        }
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 104, 70, 44, SL_TEX | TXI_PLANKS); Col(d, 1, 52, 52, 56, 1);
    iInType[IN_DOOR] = r.RegisterStatic(d);
    }
    // ---- bedroll (as in Skyrim): no frame, a long narrow leather sleeping roll on the floor, fur-lined, its
    // edges curling up; a fur blanket over the lower part with its top edge turned back, and the head end
    // rolled up into a pillow
    {
    Mesh& m = im[IN_PELTBED];
    r.MeshInit(m, 260, 360, 1);
    // the mat: 7 rows along (z), 5 across (x); the outer columns curl up and in
    static const int KZ[7] = { -100, -70, -35, 0, 35, 66, 80 };
    static const int KX[5] = { -40, -30, 0, 30, 40 };
    static const int KY[5] = { 9, 3, 2, 3, 9 };
    int P[7 * 5 * 3], ii, jj;
    for (ii = 0; ii < 7; ii++)
        for (jj = 0; jj < 5; jj++)
            {
            int* o = P + (ii * 5 + jj) * 3;
            int end = ii == 0;                                    // the foot end rounds off
            int x = KX[jj] * (end ? 80 : 100) / 100 + ((jj == 0 || jj == 4) ? (jj ? -3 : 3) : 0);
            o[0] = Cm(x); o[1] = Cm(KY[jj] + (end ? 2 : 0)); o[2] = Cm(KZ[ii] + ((ii & 1) ? 2 : 0));
            }
    for (ii = 0; ii < 6; ii++)
        for (jj = 0; jj < 4; jj++)
            {
            int q[12], k;
            const int* a0 = P + (ii * 5 + jj) * 3; const int* b0 = P + (ii * 5 + jj + 1) * 3;
            const int* c0 = P + ((ii + 1) * 5 + jj + 1) * 3; const int* d0 = P + ((ii + 1) * 5 + jj) * 3;
            for (k = 0; k < 3; k++) { q[k] = a0[k]; q[3 + k] = b0[k]; q[6 + k] = c0[k]; q[9 + k] = d0[k]; }
            r.MeshQuad(m, 0, (jj == 0 || jj == 3) ? 0 : 1, q, Cm((KX[jj] + KX[jj + 1]) / 2), Cm(-40), Cm(KZ[ii]));
            }
    // the blanket: a fur over the lower part, sides falling to the mat, its top edge turned back
    MeshHide(r, m, HIDE_DRAPE, 2, 3, 0, 0, -45, 52, 46, 12, 30);
    r.MeshLimb(m, 0, 0, Cm(-34), Cm(12), Cm(8), Cm(34), Cm(12), Cm(8), Cm(4), Cm(3), Cm(4), Cm(3), 6, 3);
    // the rolled-up head end
    r.MeshLimb(m, 0, 0, Cm(-38), Cm(10), Cm(86), Cm(38), Cm(10), Cm(86), Cm(10), Cm(9), Cm(10), Cm(9), 8, 3);
    r.MeshEllipsoid(m, 0, 4, Cm(-38), Cm(10), Cm(86), Cm(2), Cm(6), Cm(6), 5, 2);
    r.MeshEllipsoid(m, 0, 4, Cm(38), Cm(10), Cm(86), Cm(2), Cm(6), Cm(6), 5, 2);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 126, 88, 58, 4); Col(d, 1, 120, 98, 76, SL_TEX | TXI_FUR); Col(d, 2, 96, 74, 56, SL_TEX | TXI_FUR);
    Col(d, 3, 70, 52, 40, SL_TEX | TXI_FUR); Col(d, 4, 90, 62, 40, 4);
    iInType[IN_PELTBED] = r.RegisterStatic(d);
    }
    // ---- hides on the walls (a bear's and a wolf's, the same model in other colours) and a bear rug
    {
    Mesh& m = im[IN_WALLPELT];
    r.MeshInit(m, 110, 60, 1);
    MeshHide(r, m, HIDE_WALL, 0, 1, 0, 0, 0, 72, 58, 0, 0);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 104, 76, 54, SL_TEX | TXI_FUR); Col(d, 1, 72, 52, 38, SL_TEX | TXI_FUR);
    iInType[IN_WALLPELT] = r.RegisterStatic(d);
    Col(d, 0, 150, 146, 140, 6); Col(d, 1, 96, 92, 90, 6);
    iInType[IN_WALLPELT2] = r.RegisterStatic(d);
    Mesh& g = im[IN_FURRUG];
    r.MeshInit(g, 180, 140, 1);
    MeshHide(r, g, HIDE_RUG, 0, 1, 0, 0, 0, 78, 62, 1, 0);
    r.MeshFinish(g);
    r.MeshPlanarUV(g, 2);
    Def0(d, &g, 0, 0, 0, 0);
    Col(d, 0, 94, 70, 50, SL_TEX | TXI_FUR); Col(d, 1, 64, 46, 34, SL_TEX | TXI_FUR);
    iInType[IN_FURRUG] = r.RegisterStatic(d);
    }
    // ---- wall torch: an iron sconce (back plate, arm and ring) holding a pitch-wrapped torch
    {
    Mesh& m = im[IN_WALLTORCH];
    r.MeshInit(m, 80, 120, 1);
    BoxC(r, m, 0, -5, 150, 0, 5, 172, 2);                               // back plate
    Rod(r, m, 0, 0, 160, 1, 0, 166, 16, 1, 4);                           // arm
    r.MeshLimb(m, 0, 0, 0, Cm(165), Cm(14), 0, Cm(169), Cm(17), Cm(4), Cm(4), Cm(4), Cm(4), 6, 0);    // ring
    Rod(r, m, 1, 0, 140, 10, 0, 184, 22, 2, 5);                          // the torch
    r.MeshEllipsoid(m, 0, 2, 0, Cm(185), Cm(22), Cm(4), Cm(6), Cm(4), 5, 2);   // pitch-soaked head
    r.MeshFinish(m);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 50, 50, 54, 1); DARKW(d, 1); Col(d, 2, 40, 28, 20, 1);
    iInType[IN_WALLTORCH] = r.RegisterStatic(d);
    }
    // ---- cooking pot hung from a tripod over the fire
    {
    Mesh& m = im[IN_POT];
    r.MeshInit(m, 110, 160, 1);
    int k;
    for (k = 0; k < 3; k++)
        {
        int a = k * 341 + 60;
        Rod(r, m, 0, (r3d_cos(a) * 70) >> 14, 0, (r3d_sin(a) * 70) >> 14, 0, 125, 0, 3, 4);
        }
    Rod(r, m, 1, 0, 125, 0, 0, 80, 0, 1, 3);                                                   // chain
    r.MeshLimb(m, 0, 1, 0, Cm(40), 0, 0, Cm(56), 0, Cm(14), Cm(14), Cm(24), Cm(24), 8, 1);       // pot belly
    r.MeshLimb(m, 0, 1, 0, Cm(56), 0, 0, Cm(72), 0, Cm(24), Cm(24), Cm(21), Cm(21), 8, 0);
    r.MeshConeAt(m, 0, 2, 0, 0, Cm(20), Cm(68), Cm(68), 8, 0);                                   // stew
    r.MeshLimb(m, 0, 1, Cm(-21), Cm(72), 0, 0, Cm(80), 0, 1, 1, 1, 1, 3, 0);                     // handle
    r.MeshLimb(m, 0, 1, Cm(21), Cm(72), 0, 0, Cm(80), 0, 1, 1, 1, 1, 3, 0);
    r.MeshFinish(m);
    Def0(d, &m, 0, 0, 0, 0);
    DARKW(d, 0); Col(d, 1, 46, 46, 50, 1); Col(d, 2, 150, 96, 52, 1);
    iInType[IN_POT] = r.RegisterStatic(d);
    }
    // ---- a single grain sack (container prop)
    {
    Mesh& m = im[IN_SACK];
    r.MeshInitSmooth(m, 40, 60, 1);
    r.MeshEllipsoid(m, 0, 0, 0, Cm(30), 0, Cm(30), Cm(32), Cm(26), 6, 4);
    r.MeshLimb(m, 0, 0, 0, Cm(58), 0, 0, Cm(70), 0, Cm(8), Cm(8), Cm(10), Cm(10), 5, 2);
    r.MeshFinish(m);
    r.MeshSmooth(m);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 196, 178, 136, 6);
    iInType[IN_SACK] = r.RegisterStatic(d);
    Mesh& g = im[IN_RUG];
    r.MeshInit(g, 8, 4, 1);
    int q0[12] = { Cm(-120), 2, Cm(-80), Cm(120), 2, Cm(-80), Cm(120), 2, Cm(80), Cm(-120), 2, Cm(80) };
    r.MeshQuad(g, 0, 0, q0, 0, -M, 0);
    int q1[12] = { Cm(-100), 3, Cm(-60), Cm(100), 3, Cm(-60), Cm(100), 3, Cm(60), Cm(-100), 3, Cm(60) };
    r.MeshQuad(g, 0, 1, q1, 0, -M, 0);
    r.MeshFinish(g);
    Def0(d, &g, 0, 0, 0, 0);
    Col(d, 0, 120, 40, 34, 6); Col(d, 1, 170, 120, 70, 6);
    iInType[IN_RUG] = r.RegisterStatic(d);
    Mesh& c = im[IN_CANDLE];
    r.MeshInit(c, 24, 30, 1);
    r.MeshLimb(c, 0, 0, 0, 0, 0, 0, Cm(4), 0, Cm(8), Cm(8), Cm(8), Cm(8), 6, 2);
    r.MeshLimb(c, 0, 1, 0, Cm(4), 0, 0, Cm(18), 0, Cm(3), Cm(3), Cm(3), Cm(3), 5, 2);
    r.MeshFinish(c);
    Def0(d, &c, 0, 0, 0, 0);
    Col(d, 0, 90, 70, 50, 1); Col(d, 1, 236, 226, 196, 1);
    iInType[IN_CANDLE] = r.RegisterStatic(d);
    Mesh& s = im[IN_SACKS];
    r.MeshInit(s, 64, 96, 1);
    r.MeshEllipsoid(s, 0, 0, Cm(-25), Cm(30), 0, Cm(28), Cm(32), Cm(24), 6, 3);
    r.MeshEllipsoid(s, 0, 0, Cm(28), Cm(28), Cm(6), Cm(26), Cm(30), Cm(22), 6, 3);
    r.MeshEllipsoid(s, 0, 0, Cm(0), Cm(62), Cm(2), Cm(24), Cm(22), Cm(20), 6, 3);
    r.MeshFinish(s);
    r.MeshSmooth(s);
    Def0(d, &s, 0, 0, 50, 80);
    Col(d, 0, 196, 178, 136, 6);
    iInType[IN_SACKS] = r.RegisterStatic(d);
    }
#ifndef NOCRYPT
    BuildCryptMeshes();
    BuildHallDeadInterior();
    BuildCatacombs();
#endif
    BuildShopMeshes();
    BuildShopInterior();
    BuildInnMeshes();
    BuildInnInterior();
    BuildHighhallMeshes();
    BuildHighhall();
    // (v0.4.55) Skyrim's own barrow when its file is there (data/bleakfalls.har): the game's built one not made
    { u8 h[8]; iBfbKit = iDataRead && iDataRead(iDataCtx, "bleakfalls.har", 0, h, 8) == 8 && h[0] == 'H' && h[3] == '1'; }
    { u8 h[8]; iWrKit = iDataRead && iDataRead(iDataCtx, "whiterun.har", 0, h, 8) == 8 && h[0] == 'H' && h[3] == '1'; }
    { u8 h[8]; iFhKit = iDataRead && iDataRead(iDataCtx, "forelhost.har", 0, h, 8) == 8 && h[0] == 'H' && h[3] == '1'; }     // (v0.4.68)
    iOutObjMark = -1; iOutArea = 0; iOutObjN = 0; iOutPvs = 0; iOutPvsReg = 0; iOutPvsRegN = 0; iOutPvsObjN = 0;
    BuildDungeonMeshes();
    // what the area's file brings starts here: the defs, the mesh arena, the interiors' objects (UseArea goes back)
    // (v0.4.56) the shared library first: its models made and kept below the areas' marks
    if (LoadArea("common.hlb", 1)) { int i; for (i = 0; i < iLibMeshN && i < MAXKIT; i++) BuildKitMesh(i); }
    iAreaDefs = iR->iNumDefs; iAreaArena = iR->iArena; iAreaIObj = iR->iINum; iAreaCur = 0;
    LoadArea("riverwood.har");      // Riverwood's models and places, from its file
    BuildKit();                     // Riverwood's interiors, from Skyrim's own pieces
    // (v0.4.48: the game's own stables residence only without Skyrim's - the area's file has Whiterun's Stables)
    if (!GameIsKitCell(*this, CELL_STABLES)) BuildStablesInterior();
    }

// place furniture: cm in the interior frame, y above the floor
int GameZoneOf(int aCell, int aX, int aZ);
void Game::IPlace(int aCell, int aIt, int aMx, int aMy, int aMz, int aYaw, int aScale)
    {
    int t = aIt == IN_FLAME && aScale < 150 ? iSmallFlameType : iInType[aIt];          // (a candle's flame: the light one)
    int i = iR->AddInterior(aCell, t, Ix(aMx), IN_FLOOR + Cm(aMy), Iz(aMz), aYaw, aScale, 0);
    if (i >= 0) iR->iIObj[i].pad = (u8)GameZoneOf(aCell, Ix(aMx), Iz(aMz));
    }

// Vindgard Stables residence (after the UESP description of a hold stables house):
// one big room with a firepit facing the door and a cooking spit, a round table with two chairs,
// a cupboard, a long table with a bench on the west wall, garlic and salmon hanging from the rafters,
// shelves; two bedrooms on the east side with beds, end tables, a dresser, a chest and a display case.
void Game::BuildStablesInterior()
    {
    int c = CELL_STABLES;
    IPlace(c, IN_SHELL, 0, 0, 0, 0, 256);
    // main room
    IPlace(c, IN_FIREPIT, -180, 0, 40, 0, 256);
    IPlace(c, IN_FLAME, -180, 22, 40, 0, 560);
    ILight(c, -180, 70, 40, 900, 190);                         // the hearth
    IPlace(c, IN_RUG, -180, 0, -190, 0, 256);
    IPlace(c, IN_TABLE, -420, 0, -280, 0, 256);
    IPlace(c, IN_CANDLE, -420, 78, -260, 0, 256);
    IPlace(c, IN_FLAME, -420, 96, -260, 0, 60);
    ILight(c, -420, 100, -260, 300, 70);
    IPlace(c, IN_LONGTABLE, -530, 0, 160, 768, 256);         // along the west wall, bench facing the room
    IPlace(c, IN_SHELF, -40, 0, 432, 512, 256);
    IPlace(c, IN_SHELF, 100, 0, -432, 0, 256);
    IPlace(c, IN_SACKS, -540, 0, -400, 0, 256);
    IPlace(c, IN_WALLPELT, -598, 185, 160, 256, 256);          // a bear hide over the long table
    // torches in iron sconces on the walls, as in a Skyrim house
    WallTorch(c, -598, -150, 256);
    WallTorch(c, -250, 448, 512);
    WallTorch(c, -400, -448, 0);
    WallTorch(c, 198, -40, 768);
    // north-east bedroom: double bed against the north wall, end table, chest at the foot, dresser,
    // a wolf hide on the east wall, a bear rug by the bed
    IPlace(c, IN_ENDTABLE, 420, 0, 410, 0, 256);
    IPlace(c, IN_FLAME, 420, 72, 410, 0, 50);
    ILight(c, 420, 76, 410, 260, 60);
    IPlace(c, IN_WALLPELT2, 758, 165, 300, 768, 200);
    IPlace(c, IN_FURRUG, 330, 0, 85, 256, 230);
    WallTorch(c, 300, 448, 512);
    // south-east bedroom: single bed against the east wall, end table, dresser, display case, a wolf hide
    IPlace(c, IN_ENDTABLE, 710, 0, -180, 0, 256);
    IPlace(c, IN_FLAME, 710, 72, -180, 0, 50);
    ILight(c, 710, 76, -180, 260, 60);
    IPlace(c, IN_WALLPELT2, 620, 160, -448, 0, 200);
    WallTorch(c, 758, -100, 768);
    }

// a light in an interior (cm); the brightest few near the camera are used each frame
void Game::ILight(int aCell, int aMx, int aMy, int aMz, int aRadCm, int aStrength)
    {
    AddLightSrc(aCell, Ix(aMx), IN_FLOOR + Cm(aMy), Iz(aMz), Cm(aRadCm), aStrength);
    }

void Game::AddLightSrc(int aCell, int aX, int aY, int aZ, int aRadius, int aStrength)
    {
    if (iNLSrc >= MAXLSRC) return;
    LightSrc& l = iLSrc[iNLSrc++];
    l.cell = aCell; l.x = aX; l.y = aY; l.z = aZ; l.r = aRadius; l.s = aStrength;
    }

// a torch in an iron sconce on a wall (the sconce's back on the wall, facing local +z), with its flame and light
void Game::WallTorch(int aCell, int aMx, int aMz, int aYaw)
    {
    IPlace(aCell, IN_WALLTORCH, aMx, 0, aMz, aYaw, 256);
    int fx = aMx + ((r3d_sin(aYaw) * 24) >> 14), fz = aMz + ((r3d_cos(aYaw) * 24) >> 14);
    IPlace(aCell, IN_FLAME, fx, 190, fz, 0, 110);
    ILight(aCell, fx, 200, fz, 700, 150);
    }

// each frame: the lights near the camera (fires, candles, torches; the torch in your hand)
void Game::SetupLights()
    {
    R3D& r = *iR;
    r.ClearLights();
    r.iAmbient = iS.cell ? 30 : 0;
    int t = iTimeUs >> 10;
    // a torch in the hand lights everything round you
    if (EquippedId(EQ_SHIELD) == IT_TORCH && iUi != UI_TITLE)
        {
        int fl = 220 + ((r3d_sin(t * 3) * 20 + r3d_sin(t * 7) * 16) >> 14);
        r.AddLight(iS.px + ((r3d_sin(iS.yaw - 200) * 40) >> 8), iS.py - M / 4, iS.pz + ((r3d_cos(iS.yaw - 200) * 40) >> 8), 9 * M, 180 * fl >> 8);
        }
    // outdoors the fires matter only when the day is not bright
    if (iS.cell == 0 && r.iLight > 200) return;
    int picked[R3D::MAXLIGHTS], np = 0, k;
    int maxL = iS.cell ? 4 : R3D::MAXLIGHTS;              // indoors the nearest four do (each costs every vertex)
    while (r.iNumLights < maxL)
        {
        int best = -1, bd = 1 << 30;
        for (k = 0; k < iNLSrc; k++)
            {
            const LightSrc& l = iLSrc[k];
            if (l.cell != iS.cell) continue;
            int j, used = 0;
            for (j = 0; j < np; j++) if (picked[j] == k) used = 1;
            if (used) continue;
            int dx = (l.x - r.iCamX) >> 8, dz = (l.z - r.iCamZ) >> 8, dd = dx * dx + dz * dz;
            int reach = (l.r >> 8) + 24;
            if (dd > reach * reach || dd >= bd) continue;
            best = k; bd = dd;
            }
        if (best < 0) break;
        picked[np++] = best;
        const LightSrc& l = iLSrc[best];
        int fl = 226 + ((r3d_sin(t * 4 + best * 300) * 18 + r3d_sin(t * 9 + best * 77) * 12) >> 14);
        r.AddLight(l.x, l.y, l.z, l.r, l.s * fl >> 8);
        }
    }
// ---------------------------------------------------------------- the places on the map (Skyrim's map markers)
// name, where (the marker), how near counts as there (discovers it), kind (its icon)
int GameBarrowX(); int GameBarrowZ(); int GameRiverwoodX(); int GameRiverwoodZ();
enum { NLOC = 3 };
int GameLocN() { return NLOC; }
void GameLoc(const Game& g, int i, LocInfo& o)
    {
    switch (i)
        {
        case 0: o.name = "Whiterun"; o.x = CITY_CX * CELL + CELL / 2; o.z = CITY_CZ * CELL + CELL / 2; o.r = 150 * M; o.kind = LOC_CITY; return;
        case 1: o.name = "Riverwood"; o.x = GameRiverwoodX(); o.z = GameRiverwoodZ(); o.r = 70 * M; o.kind = LOC_TOWN; return;
        default: o.name = "Bleak Falls Barrow"; o.x = GameBarrowX(); o.z = GameBarrowZ(); o.r = 60 * M; o.kind = LOC_RUIN; return;
        }
    }
// a place reached for the first time: marked found (iS.spare[1], a bit each) and its name shown
void Game::Discover()
    {
    // (outdoors only, where you are: never from inside a house, whose way out may be anywhere)
    if (iS.cell) return;
    int x = iS.px, z = iS.pz, i;
    for (i = 0; i < NLOC; i++)
        {
        if (iS.spare[1] & (1 << i)) continue;
        LocInfo l; GameLoc(*this, i, l);
        int dx = (l.x - x) >> 8, dz = (l.z - z) >> 8, rr = l.r >> 8;
        if (dx * dx + dz * dz > rr * rr) continue;
        iS.spare[1] |= 1 << i;
        iDiscUs = DISC_US; iDiscLoc = i;
        return;
        }
    }

// ---------------------------------------------------------------- travel list (testing)
struct TravelSpot { const char* name; int cell; int x, z, yaw, feet; };
int GameRiverwoodX(); int GameRiverwoodZ();
int GameBarrowX(); int GameBarrowZ(); int GameTowerX(); int GameTowerZ(); int GameBarrowOutX(int a, int b); int GameBarrowOutZ(int a, int b); int GameBarrowDoorYaw();
int GameBarrowSillY(const Game& g);
static int Travel(Game& g, int i, TravelSpot& t)
    {
    t.feet = 0x7fffffff;
    if (g.iBfbKit && i >= 17 && i <= 19)
        {
        // (v0.4.55) Skyrim's own barrow: a step in from its doors - the main one, the temple's below, the sanctum's
        const DoorLink& d = g.Door(i == 17 ? 7 : 9);
        int out = i == 18, yaw = out ? d.oyaw : d.iyaw, x = out ? d.ox : d.ix, z = out ? d.oz : d.iz;
        t.name = i == 17 ? "Bleak Falls Temple (inside)" : (i == 18 ? "Bleak Falls Temple (below)" : "Bleak Falls Sanctum");
        t.cell = out ? CELL_BFTEMPLE : d.cell; t.yaw = yaw; t.feet = out ? d.oy : d.iy;
        t.x = x + ((r3d_sin(yaw) * 3 * M / 2) >> 14); t.z = z + ((r3d_cos(yaw) * 3 * M / 2) >> 14);
        return 1;
        }
    switch (i)
        {
        case 0: t.name = "Whiterun - outside the gate"; t.cell = 0; t.x = Wx(-70); t.z = Wz(-12); t.yaw = 256; return 1;
        case 1: t.name = "Whiterun - Plains District"; t.cell = 0; t.x = Wx(-52); t.z = Wz(-12); t.yaw = 256; return 1;
        case 2: t.name = "Whiterun - Market"; t.cell = 0; t.x = Wx(-12); t.z = Wz(-12); t.yaw = 256; return 1;
        case 3: t.name = "Whiterun - Wind District"; t.cell = 0; t.x = Wx(-4); t.z = Wz(12); t.yaw = 0; return 1;
        case 9: t.name = "Whiterun - below Dragonsreach"; t.cell = 0; t.x = Wx(0); t.z = Wz(52); t.yaw = 0; return 1;
        case 4: t.name = "Stables - outside"; t.cell = 0; t.x = Wx(-88); t.z = Wz(-50); t.yaw = 256; return 1;
        case 5: t.name = "Stables - inside"; t.cell = CELL_STABLES; t.x = Ix(-200); t.z = Iz(-330); t.yaw = 0; return 1;
        case 6: t.name = "Windmill"; t.cell = 0; t.x = Wx(-140); t.z = Wz(-75); t.yaw = 768; return 1;
        case 7: t.name = "Gate approach and bridge"; t.cell = 0; t.x = Wx(-112); t.z = Wz(-27); t.yaw = 0; return 1;
        case 8: t.name = "Caravan camp"; t.cell = 0; t.x = Wx(-118); t.z = Wz(-36); t.yaw = 692; return 1;
        // inside Whiterun's buildings: a step in from the door
        case 10: case 11: case 12: case 13: case 14:
            {
            static const u8 KD[5] = { 1, 2, 3, 4, 6 };
            static const char* const KN[5] = { "Hall of the Dead", "Hall of the Dead - Catacombs", "Arcadia's Cauldron", "The Bannered Mare", "Dragonsreach" };
            const DoorLink& dl = g.Door(KD[i - 10]);
            t.name = KN[i - 10]; t.cell = dl.cell; t.yaw = dl.iyaw;
            t.x = dl.ix + ((r3d_sin(dl.iyaw) * 3 * M / 2) >> 14); t.z = dl.iz + ((r3d_cos(dl.iyaw) * 3 * M / 2) >> 14);
            return 1;
            }
        case 15: t.name = "Bleak Falls Tower"; t.cell = 0; t.x = GameTowerX() - 12 * M; t.z = GameTowerZ() + 8 * M; t.yaw = 300; return 1;
        case 16: t.name = "Bleak Falls Barrow - the terrace"; t.cell = 0; t.x = GameBarrowOutX(0, 8); t.z = GameBarrowOutZ(0, 8); t.yaw = (GameBarrowDoorYaw() + 512) & 1023; return 1;
        case 17: t.name = "Bleak Falls Temple (inside)"; t.cell = CELL_BFTEMPLE; t.x = g.Door(7).ix - 2 * M; t.z = g.Door(7).iz; t.yaw = 768; return 1;
        case 18: t.name = "Bleak Falls Temple (below)"; t.cell = CELL_BFCRYPT; t.x = g.Door(8).ix + 2 * M; t.z = g.Door(8).iz; t.yaw = 256; return 1;
        case 19: t.name = "Bleak Falls Sanctum"; t.cell = CELL_BFSANCTUM; t.x = g.Door(9).ix - 2 * M; t.z = g.Door(9).iz; t.yaw = 768; return 1;
        case 20: t.name = "Riverwood - the inn and the street"; t.cell = 0; t.x = GameRiverwoodX() + 8 * M; t.z = GameRiverwoodZ() + 4 * M; t.yaw = 640; return 1;
        case 21: t.name = "Riverwood - the mill"; t.cell = 0; t.x = GameRiverwoodX() - 26 * M; t.z = GameRiverwoodZ() + 8 * M; t.yaw = 768; return 1;
        case 22: t.name = "Riverwood - Alvor's forge"; t.cell = 0; t.x = GameRiverwoodX() - 12 * M; t.z = GameRiverwoodZ() + 2 * M; t.yaw = 0; return 1;
        case 23: t.name = "Riverwood Trader (inside)"; t.cell = CELL_RWTRADER; t.x = Ix(-273); t.z = Iz(-675); t.yaw = 40; return 1;
        case 24: t.name = "Sleeping Giant Inn (inside)"; t.cell = CELL_RWINN; t.x = Ix(-366); t.z = Iz(-460); t.yaw = 0; return 1;
        case 25: t.name = "Sleeping Giant - Delphine's secret room"; t.cell = CELL_RWINN; t.x = Ix(523); t.z = Iz(1430); t.yaw = 512; return 1;
        case 30: case 31: case 32: case 33:
            {
            // (v0.4.68) Forelhost: before its door on the mountain, just inside each hall, on its top by the word wall
            const DoorLink& dl = g.Door(i == 30 ? 26 : (i == 33 ? 29 : 26 + i - 31));
            if (i == 30 || i == 33)
                {
                t.name = i == 30 ? "Forelhost (its door)" : "Forelhost (its top, the word wall)"; t.cell = 0; t.yaw = dl.oyaw;
                t.x = dl.ox + ((r3d_sin(dl.oyaw) * (2 * M)) >> 14); t.z = dl.oz + ((r3d_cos(dl.oyaw) * (2 * M)) >> 14);
                return 1;
                }
            t.name = i == 31 ? "Forelhost Stronghold" : "Forelhost Crypt"; t.cell = i == 31 ? CELL_FHSTRONG : CELL_FHCRYPT; t.yaw = dl.iyaw;
            t.x = dl.ix + ((r3d_sin(dl.iyaw) * (6 * M / 5)) >> 14); t.z = dl.iz + ((r3d_cos(dl.iyaw) * (6 * M / 5)) >> 14);
            return 1;
            }
        case 26: case 27: case 28: case 29:
            {
            // the houses built from Skyrim's pieces: just inside their doors
            const DoorLink& dl = g.Door(14 + i - 26);
            t.name = dl.name; t.cell = dl.cell; t.yaw = dl.iyaw;
            t.x = dl.ix + ((r3d_sin(dl.iyaw) * (6 * M / 5)) >> 14); t.z = dl.iz + ((r3d_cos(dl.iyaw) * (6 * M / 5)) >> 14);
            return 1;
            }
        }
    return 0;
    }
enum { NTRAVEL = 34 };      // every place in the game is listed here (keep it so when adding one)

void Game::Teleport(int aIdx)
    {
    TravelSpot t;
    if (!Travel(*this, aIdx, t)) return;
    if (iAreaArena && AreaOf(t.cell, t.x, t.z) != iAreaCur && DeferLoad(4, aIdx)) return;     // (v0.4.74) the loading screen first
    EnterCell(t.cell, t.x, t.z, t.yaw, t.feet);
    if (aIdx == 25) iS.py = FloorAt(t.cell, t.x, t.z, IN_FLOOR - 3 * M) + 410;     // the secret room, under the inn
    if (aIdx == 16) iS.py = FloorAt(0, t.x, t.z, GameBarrowSillY(*this) + M) + 410;   // on the barrow's terrace
    iUi = UI_PLAY;
    Msg(t.name);
    }

void Game::TeleInput()
    {
    if (Pressed(KEY_LOOK_U) || Pressed(KEY_FWD)) iTeleSel = (iTeleSel + NTRAVEL - 1) % NTRAVEL;
    if (Pressed(KEY_LOOK_D) || Pressed(KEY_BACK)) iTeleSel = (iTeleSel + 1) % NTRAVEL;
    if (Pressed(KEY_FIRE)) Teleport(iTeleSel);
    }

// ---------------------------------------------------------------- the benchmark (v0.4.82)
// The travel list's places in its order (each area's together: a file read per area), and the Windmill again facing
// Whiterun (the city across the plains: the heaviest view). Each: the move (with its loading time when an area file is
// read), 1.5 s to settle, then 3 s measured: frames, the time a frame (fps), its drawing (d, the game's own work
// included) and its copy to the screen (c), the triangles - average, least, most. Where the player stood is logged
// too, so the emulator can take the same view.
int GameViewNear(int v); int GameViewFar(int v); int GameViewTex(int v); int GameViewFull(int v); int GameViewMiddle(int v);
enum { BENCH_FACE_WR = 100, BENCH_SETTLE_US = 1500000, BENCH_MEASURE_US = 3000000 };
static const u8 KBenchExtra = 6;            // after this place: itself again, facing Whiterun
int Game::BenchCount() const { return NTRAVEL + 1; }
const char* Game::BenchName(int aStep, int* aTravel)
    {
    int t = aStep <= KBenchExtra ? aStep : aStep - 1, face = aStep == KBenchExtra + 1;
    *aTravel = t + (face ? BENCH_FACE_WR : 0);
    TravelSpot sp;
    if (!Travel(*this, t, sp)) return "?";
    return face ? "Windmill, facing Whiterun" : sp.name;
    }
void Game::BenchPut(const char* aS)
    {
    while (*aS && iBenchLen < BENCH_LOG - 1) iBenchLog[iBenchLen++] = *aS++;
    iBenchLog[iBenchLen] = 0;
    }
// a number right-aligned in aWidth characters
void Game::BenchNum(int aV, int aWidth)
    {
    char b[16];
    char* e = r3d_puti(b, aV);
    int n = (int)(e - b);
    while (n++ < aWidth) BenchPut(" ");
    BenchPut(b);
    }
// a time in microseconds as milliseconds with one decimal
static char* PutMs(char* p, int aUs)
    {
    int t = (aUs + 50) / 100;
    p = r3d_puti(p, t / 10); *p++ = '.'; *p++ = (char)('0' + t % 10); *p = 0;
    return p;
    }
void Game::BenchStart()
    {
    iBenchLen = 0; iBenchLog[0] = 0;
    BenchPut("=== Hrimfell " HRIMFELL_VERSION " benchmark ===\n");
    char b[160];
    char* p = r3d_puts(b, "settings: display "); p = r3d_puti(p, iS.outMode & 3);
    p = r3d_puts(p, " (0 320x200, 1 400x200, 2 640x200, 3 640x200 2x), draw distance "); p = r3d_puti(p, GameViewNear(iS.view));
    p = r3d_puts(p, " m, far land "); p = r3d_puti(p, GameViewFar(iS.view));
    p = r3d_puts(p, " m, textures "); p = r3d_puti(p, GameViewTex(iS.view)); r3d_puts(p, " m,\n");
    BenchPut(b);
    p = r3d_puts(b, "  plants "); p = r3d_puts(p, (iS.view >> 9) & 1 ? "off" : "on");
    p = r3d_puts(p, ", full models "); p = r3d_puti(p, GameViewFull(iS.view));
    p = r3d_puts(p, " m, middle models "); p = r3d_puti(p, GameViewMiddle(iS.view));
    p = r3d_puts(p, " m, rows "); p = r3d_puts(p, (iS.view >> 16) & 1 ? "half" : "full");
    p = r3d_puts(p, ", near land "); p = r3d_puts(p, (iS.view >> 17) & 1 ? "flat" : "smooth");
    p = r3d_puts(p, ", small things "); p = r3d_puts(p, (iS.view >> 18) & 1 ? "only near" : "all");
    p = r3d_puts(p, ", auto speed "); p = r3d_puti(p, (iS.view >> 19) & 3); r3d_puts(p, " (off for the test)\n");
    BenchPut(b);
    p = r3d_puts(b, "  view bits "); p = r3d_puti(p, iS.view);
    p = r3d_puts(p, " (emulator VIEW=), brightness "); p = r3d_puti(p, iS.gamma & 3);
    p = r3d_puts(p, "; RAM free "); p = r3d_puti(p, iRamFreeKB);
    p = r3d_puts(p, " KB of "); p = r3d_puti(p, iRamTotalKB);
    p = r3d_puts(p, ", the game "); p = r3d_puti(p, iRamUsedKB); r3d_puts(p, " KB\n");
    BenchPut(b);
    BenchPut("each place: load ms (an area file read; - none), then over 3 s after 1.5 s to settle: frames, fps,\n"
             "triangles avg (min-max), d = a frame's game work + drawing ms avg (min-max), c = the copy to the screen\n"
             "ms avg (max), the slowest frame ms; then where: cell x z y yaw pitch (game units), the game's hour\n\n");
    iBenchView = iS.view;
    iS.view &= ~(3 << 19); iAutoLevel = 0; ApplyViewSetting();
    iBench = 1; iBenchPhase = 0; iBenchTotalUs = 0;
    iUi = UI_PLAY;
    }
void Game::BenchStop(const char* aWhy)
    {
    if (!iBench) return;
    char b[96];
    char* p = r3d_puts(b, "\n=== ");
    p = r3d_puts(p, aWhy); p = r3d_puts(p, " after ");
    p = r3d_puti(p, iBenchTotalUs / 1000000); r3d_puts(p, " s ===\n");
    BenchPut(b);
    iBench = 0;
    iS.view = iBenchView; ApplyViewSetting();
    iRequest = 5;
    }
// once a frame, from ReportTimes (the frame before this one: its drawing, its copy, its whole time)
void Game::BenchFrame(int aRenderUs, int aBlitUs, int aFrameUs)
    {
    if (!iBench) return;
    if (aFrameUs > 0) iBenchTotalUs += aFrameUs;
    int step = iBench - 1, t;
    if (iUi == UI_DEAD) { BenchPut("  (died here)\n"); Respawn(); }
    if (iUi != UI_PLAY && iUi != UI_DEAD) iUi = UI_PLAY;     // (a book or a talk opened by itself: closed)
    switch (iBenchPhase)
        {
        case 0:
            BenchName(step, &t);
            Teleport(t % BENCH_FACE_WR);
            iBenchPhase = 1; iBenchUs = 0; iBenchLoaded = 0;
            return;
        case 1:
            iBenchUs += aFrameUs;
            if (iLoadPhase) { iBenchLoaded = 1; return; }
            // (this report's frame was the one that read the file)
            iBenchLoadUs = iBenchUs;
            BenchName(step, &t);
            if (t >= BENCH_FACE_WR)
                {
                int cx = CITY_CX * CELL + CELL / 2, cz = CITY_CZ * CELL + CELL / 2;
                iS.yaw = r3d_atan2(cx - iS.px, cz - iS.pz) & 1023;
                }
            iS.drawn = 0; iDrawUs = 0; iS.pitch = 0;
            iBenchPhase = 2; iBenchUs = 0;
            return;
        case 2:
            iBenchUs += aFrameUs;
            if (iBenchUs < BENCH_SETTLE_US) return;
            iBenchPhase = 3; iBenchUs = 0;
            iBenchN = 0; iBenchTris = 0; iBenchTrisMin = 0x7fffffff; iBenchTrisMax = 0;
            iBenchR = 0; iBenchRMin = 0x7fffffff; iBenchRMax = 0; iBenchB = 0; iBenchBMax = 0; iBenchF = 0; iBenchFMax = 0;
            return;
        case 3:
            {
            int tr = iR->iTris;
            iBenchN++; iBenchUs += aFrameUs; iBenchF += aFrameUs; iBenchR += aRenderUs; iBenchB += aBlitUs; iBenchTris += tr;
            if (tr < iBenchTrisMin) iBenchTrisMin = tr;
            if (tr > iBenchTrisMax) iBenchTrisMax = tr;
            if (aRenderUs < iBenchRMin) iBenchRMin = aRenderUs;
            if (aRenderUs > iBenchRMax) iBenchRMax = aRenderUs;
            if (aBlitUs > iBenchBMax) iBenchBMax = aBlitUs;
            if (aFrameUs > iBenchFMax) iBenchFMax = aFrameUs;
            if (iBenchUs < BENCH_MEASURE_US) return;
            int n = iBenchN;
            char b[200];
            char* p = r3d_puti(b, step + 1); p = r3d_puts(p, ". ");
            p = r3d_puts(p, BenchName(step, &t));
            r3d_puts(p, "\n   load ");
            BenchPut(b);
            if (iBenchLoaded) { PutMs(b, iBenchLoadUs); BenchPut(b); } else BenchPut("-");
            BenchPut("  frames "); BenchNum(n, 0);
            int fps100 = n * 1000000 / (iBenchF / 100 > 0 ? iBenchF / 100 : 1);      // (no 64-bit division on the phone)
            p = r3d_puts(b, "  fps "); p = r3d_puti(p, fps100 / 100); *p++ = '.'; *p++ = (char)('0' + fps100 / 10 % 10); *p++ = (char)('0' + fps100 % 10); *p = 0;
            BenchPut(b);
            BenchPut("  tris "); BenchNum(iBenchTris / n, 0); BenchPut(" ("); BenchNum(iBenchTrisMin, 0); BenchPut("-"); BenchNum(iBenchTrisMax, 0); BenchPut(")");
            p = r3d_puts(b, "  d "); p = PutMs(p, iBenchR / n); p = r3d_puts(p, " ("); p = PutMs(p, iBenchRMin); p = r3d_puts(p, "-"); p = PutMs(p, iBenchRMax);
            p = r3d_puts(p, ")  c "); p = PutMs(p, iBenchB / n); p = r3d_puts(p, " ("); p = PutMs(p, iBenchBMax);
            p = r3d_puts(p, ")  slowest "); p = PutMs(p, iBenchFMax); r3d_puts(p, "\n   at ");
            BenchPut(b);
            BenchNum(iS.cell, 0); BenchPut(" "); BenchNum(iS.px, 0); BenchPut(" "); BenchNum(iS.pz, 0); BenchPut(" "); BenchNum(iS.py, 0);
            BenchPut(" "); BenchNum(iS.yaw, 0); BenchPut(" "); BenchNum(iS.pitch, 0); BenchPut("  hour "); BenchNum(iS.timeOfDay / 60000, 0); BenchPut("\n");
            if (step + 1 >= BenchCount()) { BenchStop("done"); return; }
            iBench++; iBenchPhase = 0;
            return;
            }
        }
    }

static const u16 KGold = 0xFEA0, KWhite = 0xFFFF, KGrey = 0xA514;

void Game::DrawTele(int x0, int w)
    {
    R3D& r = *iR;
    int cx = x0 + w / 2;
    r.Shade(x0, 0, w, OUT_H);
    r.Text(cx - r.TextW("TRAVEL (TESTING)") / 2, 16, "TRAVEL (TESTING)", KGold);
    int i;
    if (iTeleSel < iTeleScroll) iTeleScroll = iTeleSel;
    if (iTeleSel >= iTeleScroll + 10) iTeleScroll = iTeleSel - 9;
    for (i = iTeleScroll; i < NTRAVEL && i < iTeleScroll + 10; i++)
        {
        TravelSpot t;
        Travel(*this, i, t);
        int y = 36 + (i - iTeleScroll) * 15;
        int tw = r.TextW(t.name);
        if (i == iTeleSel) r.Tint(cx - tw / 2 - 8, y - 3, tw + 16, 13, KGold);
        r.Text(cx - tw / 2, y, t.name, i == iTeleSel ? KWhite : KGrey);
        }
    r.Text(cx - r.TextW("Joystick: choose   Press/E: go   Esc: back") / 2, OUT_H - 12, "Joystick: choose   Press/E: go   Esc: back", KGrey);
    }

// ---------------------------------------------------------------- doors in the world
// the floor outside a door with its height known: over the land's offset
int Game::DoorOutY(const DoorLink& d) const { return d.oy + (d.ocell == 0 && iR->iLandLoaded ? iR->iLandOffset : 0); }   // (v0.4.55: between two interiors, as it is)

void Game::FindDoor()
    {
    iDoorFocus = -1;
    int fx = r3d_sin(iS.yaw), fz = r3d_cos(iS.yaw);
    int i;
    for (i = 0; i < NDOORS; i++)
        {
        const DoorLink& d = Door(i);
        int dx, dz;
        if (iS.cell == d.ocell) { dx = d.ox - iS.px; dz = d.oz - iS.pz; }
        else if (iS.cell == d.cell) { dx = d.ix - iS.px; dz = d.iz - iS.pz; }
        else continue;
        if (Abs(dx) > 3 * M || Abs(dz) > 3 * M) continue;
        // doors one over the other (a house's street door and its upstairs one): the one on your floor
        int feet = iS.py - 410;
        if (iS.cell == d.cell && (d.hk & 1) && Abs(feet - d.iy) > 3 * M / 2) continue;
        if (iS.cell == d.ocell && (d.hk & 2) && Abs(feet - DoorOutY(d)) > 3 * M / 2) continue;
        int dd = r3d_isqrt((dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4)) << 4;
        if (dd > 5 * M / 2) continue;
        int dot = dd ? (dx * fx + dz * fz) / dd : 16384;
        if (dot < 8000 && dd > M) continue;
        iDoorFocus = i;
        return;
        }
    }

void Game::UseDoor(int i)
    {
    const DoorLink& d = Door(i);
    if (iS.cell == d.ocell)
        {
        // step inside: just in front of the inner door, facing into the room
        int x = d.ix + ((r3d_sin(d.iyaw) * (6 * M / 5)) >> 14), z = d.iz + ((r3d_cos(d.iyaw) * (6 * M / 5)) >> 14);
        Go(d.cell, x, z, d.iyaw, (d.hk & 1) ? d.iy : 0x7fffffff);
        }
    else
        {
        int x = d.ox + ((r3d_sin(d.oyaw) * (6 * M / 5)) >> 14), z = d.oz + ((r3d_cos(d.oyaw) * (6 * M / 5)) >> 14);
        Go(d.ocell, x, z, d.oyaw, (d.hk & 2) ? DoorOutY(d) : 0x7fffffff);
        }
    }

// ---------------------------------------------------------------- waiting and weather
int Game::HostilesNear(int aDist)
    {
    int i;
    for (i = 0; i < iS.ne; i++)
        {
        const Ent& e = iS.e[i];
        if (!e.alive || e.cell != iS.cell) continue;
        if (e.type < E_WOLF || e.type > E_DRAGON) continue;
        if (e.type == E_DRAGON && e.state == D_SLEEP) continue;
        int dx = e.x - iS.px, dz = e.z - iS.pz;
        int lim = e.type == E_DRAGON ? aDist * 3 : aDist;
        if (Abs(dx) < lim && Abs(dz) < lim) return 1;
        }
    return 0;
    }

void Game::StartWait()
    {
    if (HostilesNear(30 * M)) { Msg("You cannot wait with enemies nearby."); return; }
    if (iS.vy != 0) { Msg("You cannot wait in the air."); return; }
    iUi = UI_WAIT;
    iSleeping = 0;
    if (iWaitSel < 1) iWaitSel = 1;
    iWaitLeft = 0; iWaitAccUs = 0;
    }

// one game hour passes: the clock, the weather and the body all move on
void Game::WaitHour()
    {
    iS.timeOfDay += 60000;
    if (iS.timeOfDay >= 1440000) { iS.timeOfDay -= 1440000; iS.day++; }
    StepWeather();
    RiverwoodSchedule();
    iWBlend = 256;
    iS.hp += (iS.hpMax << 8) / 3; iS.mp += (iS.mpMax << 8) / 2; iS.sp = iS.spMax << 8;
    if (iS.hp > (iS.hpMax << 8)) iS.hp = iS.hpMax << 8;
    if (iS.mp > (iS.mpMax << 8)) iS.mp = iS.mpMax << 8;
    UpdateAtmosphere();
    }

void Game::WaitInput(int aDtUs)
    {
    if (iWaitLeft > 0)
        {
        iWaitAccUs += aDtUs;
        while (iWaitAccUs >= 1000000 && iWaitLeft > 0)
            {
            iWaitAccUs -= 1000000;
            WaitHour();
            iWaitLeft--;
            }
        if (iWaitLeft == 0)
            {
            iUi = UI_PLAY;
            if (iSleeping) Msg("You feel well rested.");
            iSleeping = 0;
            }
        return;
        }
    if (Pressed(KEY_LOOK_R) || Pressed(KEY_STRAFE_R) || Pressed(KEY_LOOK_U) || Pressed(KEY_FWD)) iWaitSel = iWaitSel >= 24 ? 1 : iWaitSel + 1;
    if (Pressed(KEY_LOOK_L) || Pressed(KEY_STRAFE_L) || Pressed(KEY_LOOK_D) || Pressed(KEY_BACK)) iWaitSel = iWaitSel <= 1 ? 24 : iWaitSel - 1;
    if (Pressed(KEY_FIRE)) { iWaitLeft = iWaitSel; iWaitAccUs = 0; }
    }

// hour-by-hour weather: the longer a weather lasts the likelier it changes
void Game::StepWeather()
    {
    iS.weatherHours++;
    u32 h = r3d_hash(iS.day * 24 + iS.timeOfDay / 60000, iS.weatherHours, 4242);
    int chance = 6 + iS.weatherHours * 9;
    if ((int)(h % 100) >= chance) return;
    // transition table (percent): rows = from, columns = to {clear, cloudy, fog, rain, snow}
    static const u8 KTo[W_COUNT * W_COUNT] =
        {
         0, 62, 16, 14,  8,
        44,  0, 12, 30, 14,
        50, 40,  0, 10,  0,
        25, 60, 15,  0,  0,
        30, 60, 10,  0,  0
        };
    int roll = (int)((h >> 8) % 100), w;
    const u8* row = KTo + iS.weather * W_COUNT;
    for (w = 0; w < W_COUNT; w++) { roll -= row[w]; if (roll < 0) break; }
    if (w >= W_COUNT || w == iS.weather) return;
    iPrevWeather = iS.weather;
    iS.weather = w;
    iS.weatherHours = 0;
    iWBlend = 0;
    }

const char* Game::WeatherName()
    {
    static const char* const KN[W_COUNT] = { "Clear", "Overcast", "Fog", "Rain", "Snow" };
    if (iS.weather == W_RAIN && iS.cell == 0 && iS.py > 46 * M) return "Snow";
    return KN[iS.weather % W_COUNT];
    }

// light / fog parameters of each weather: light (/256), grey mix of the sky (/256), fog start/end (m)
static const int KWx[W_COUNT * 4] =
    {
    256,   0, 45, 170,
    200, 170, 38, 150,
    196, 210,  6,  55,
    158, 200, 20,  95,
    206, 220, 14,  80
    };

void Game::WeatherParams(int& aLight, int& aGrey, int& aF0, int& aF1)
    {
    const int* a = KWx + (iPrevWeather % W_COUNT) * 4;
    const int* b = KWx + (iS.weather % W_COUNT) * 4;
    int t = iWBlend;
    aLight = a[0] + ((b[0] - a[0]) * t >> 8);
    aGrey = a[1] + ((b[1] - a[1]) * t >> 8);
    aF0 = a[2] + ((b[2] - a[2]) * t >> 8);
    aF1 = a[3] + ((b[3] - a[3]) * t >> 8);
    }

// rain streaks / snow flakes in a box around the camera
void Game::DrawWeather()
    {
    if (iS.cell) return;
    int w = iS.weather;
    if (iWBlend < 128 && (iPrevWeather == W_RAIN || iPrevWeather == W_SNOW)) w = iPrevWeather;
    if (w != W_RAIN && w != W_SNOW) return;
    R3D& r = *iR;
    int snow = w == W_SNOW || r.iCamY > 46 * M;
    int n = snow ? 140 : 170;
    int S = 14 * M, H = 7 * M;
    int t = iTimeUs >> 10;                              // ~ms
    u16 col = snow ? R3D::Rgb(240, 242, 250) : R3D::Rgb(214, 220, 236);
    int i;
    for (i = 0; i < n; i++)
        {
        u32 h = r3d_hash(i, 77, 5);
        int bx = (int)(h & 0x3FFF), bz = (int)((h >> 14) & 0x3FFF);
        if (i & 1) { bx ^= 0x2A5F; bz ^= 0x1C3B; }
        int fall = snow ? (t * M / 1000) : (t * 9 * M / 1000);
        int y = H - 1 - (int)((fall + (int)((h >> 20) & 0xFFF) * 4) % H);
        int x = (bx * S >> 14), z = (bz * S >> 14);
        if (snow) { x += (r3d_sin((t >> 2) + i * 70) * 30) >> 14; }
        // wrap into a box centred on the camera so the field moves with the world, not the view
        // box centre a few metres ahead of the camera, so most of the field is in view
        int bcx = r.iCamX + ((r3d_sin(r.iYaw) * (S * 2 / 5)) >> 14), bcz = r.iCamZ + ((r3d_cos(r.iYaw) * (S * 2 / 5)) >> 14);
        int dx = ((x - bcx) % S + S + S / 2) % S - S / 2;
        int dz = ((z - bcz) % S + S + S / 2) % S - S / 2;
        int wx = bcx + dx, wz = bcz + dz, wy = r.iCamY - 2 * M + y;
        dx = wx - r.iCamX; dz = wz - r.iCamZ;
        int sx, sy;
        if (r.ProjectWorld(wx, wy, wz, sx, sy) != 1) continue;
        if (snow) { int sz = (dx > -4 * M && dx < 4 * M && dz > -4 * M && dz < 4 * M) ? 3 : 2; r.Rect(sx, sy, sz, sz, col); continue; }
        int sx2, sy2;
        if (r.ProjectWorld(wx, wy - M / 2, wz, sx2, sy2) != 1) continue;
        int len = sy2 - sy;
        if (len < 2) len = 2;
        if (len > 24) len = 24;
        r.Tint(sx, sy, 1, len, col);
        }
    }

void Game::DrawWait(int x0, int w)
    {
    R3D& r = *iR;
    int cx = x0 + w / 2;
    char buf[64];
    int bw = 200, bh = 70, bx = cx - bw / 2, by = 108;
    r.Shade(bx, by, bw, bh);
    r.Rect(bx, by, bw, 1, KGold);
    // clock: day, time and weather
    int mins = iS.timeOfDay / 1000;
    char* p = r3d_puts(buf, "Day ");
    p = r3d_puti(p, iS.day + 1);
    p = r3d_puts(p, ", ");
    int hh = mins / 60, mm = mins % 60;
    if (hh < 10) *p++ = '0';
    p = r3d_puti(p, hh);
    *p++ = ':';
    if (mm < 10) *p++ = '0';
    p = r3d_puti(p, mm);
    p = r3d_puts(p, "   ");
    r3d_puts(p, WeatherName());
    r.Text(cx - r.TextW(buf) / 2, by + 6, buf, KWhite);
    // Skyrim's slider: 24 steps in a row, the chosen hours lit; while waiting they empty one per hour
    int sx0 = bx + 10, sw = bw - 20, sy0 = by + 42, k;
    int lit = iWaitLeft > 0 ? iWaitLeft : iWaitSel;
    for (k = 0; k < 24; k++)
        {
        int x = sx0 + k * sw / 24, x1 = sx0 + (k + 1) * sw / 24 - 2;
        u16 col = k < lit ? KGold : 0x4208;
        r.Rect(x, sy0, x1 - x, 8, col);
        }
    p = r3d_puts(buf, iWaitLeft > 0 ? (iSleeping ? "Sleeping... " : "Waiting... ") : (iSleeping ? "Sleep for " : "Wait for "));
    p = r3d_puti(p, lit);
    r3d_puts(p, lit == 1 ? " hour" : " hours");
    r.Text(cx - r.TextW(buf) / 2, by + 22, buf, KGold);
    const char* help = iWaitLeft > 0 ? "Tab: stop" : "Joy: hours  E: wait  Tab: back";
    r.Text(cx - r.TextW(help) / 2, by + 54, help, KGrey);
    }

// ---------------------------------------------------------------- weapon and camera
enum { DRAW_US = 380000 };

void Game::ToggleWeapon()
    {
    if (iDrawUs > 0 || iSwingUs > 0) return;
    iS.drawn = !iS.drawn;
    iDrawUs = DRAW_US;
    }

// 0 = weapon away, 256 = fully raised (for the first-person view model)
int Game::DrawAmount()
    {
    int k = iDrawUs * 256 / DRAW_US;
    return iS.drawn ? 256 - k : k;
    }

// third person: behind the shoulder, pulled in by walls, terrain and ceilings
void Game::UpdateCamera(int dt)
    {
    int px = iS.px, py = iS.py - 30, pz = iS.pz;
    if (!iS.third || iUi == UI_TITLE)
        {
        iCamX = iS.px; iCamY = iS.py; iCamZ = iS.pz; iCamDist = 0;
        return;
        }
    int cp = r3d_cos(iS.pitch), sp = r3d_sin(iS.pitch);
    int fx = (r3d_sin(iS.yaw) * cp) >> 14, fz = (r3d_cos(iS.yaw) * cp) >> 14, fy = sp;
    int rx = r3d_cos(iS.yaw), rz = -r3d_sin(iS.yaw);        // right
    int want = 27 * M / 10;
    int feet = iS.py - 410;
    int ceil = iS.cell ? FloorAt(iS.cell, px, pz, feet) + 3 * M : 0x7FFFFFFF;
    // march out from the pivot until something is in the way
    int allow = want, k;
    for (k = 1; k <= 10; k++)
        {
        int dist = want * k / 10;
        int x = px - ((fx * dist) >> 14) + ((rx * (dist * 2 / 11)) >> 14);
        int z = pz - ((fz * dist) >> 14) + ((rz * (dist * 2 / 11)) >> 14);
        int y = py - ((fy * dist) >> 14) + dist / 10;
        int g = FloorAt(iS.cell, x, z, feet);
        if (y < g + 40 || y > ceil - 20 || BlockedAt(iS.cell, x, z, 24, y - 120)) { allow = want * (k - 1) / 10; break; }
        }
    // pull in at once, ease back out
    if (allow < iCamDist) iCamDist = allow;
    else iCamDist += (allow - iCamDist) * Clamp(dt / 1000, 1, 100) / 200 + 1;
    if (iCamDist > allow) iCamDist = allow;
    int dist = iCamDist;
    iCamX = px - ((fx * dist) >> 14) + ((rx * (dist * 2 / 11)) >> 14);
    iCamZ = pz - ((fz * dist) >> 14) + ((rz * (dist * 2 / 11)) >> 14);
    iCamY = py - ((fy * dist) >> 14) + dist / 10;
    }

static void Slot4(u8* s, int i, int r, int g, int b, int t) { s[i * 4] = (u8)r; s[i * 4 + 1] = (u8)g; s[i * 4 + 2] = (u8)b; s[i * 4 + 3] = (u8)t; }

void Game::DrawPlayerBody()
    {
    if (!iS.third || iCamDist < M / 2) return;
    u8 sl[32];
    Slot4(sl, 0, 70, 56, 44, 4);                    // trousers
    Slot4(sl, 1, 118, 92, 64, 4);                   // tunic / armour
    Slot4(sl, 2, 214, 168, 132, 7);                 // skin
    Slot4(sl, 3, 170, 176, 186, 1);                 // blade
    Slot4(sl, 4, 120, 76, 40, 4);                   // hair
    Slot4(sl, 5, 66, 48, 34, 4);                    // boots
    Slot4(sl, 6, 80, 58, 40, 4);                    // belt / grip / scabbard
    int body = EquippedId(EQ_BODY);
    if (body >= 0) { const u8* c = GameItemCol(body); Slot4(sl, 1, c[0], c[1], c[2], 1); }
    int feet = EquippedId(EQ_FEET);
    if (feet >= 0) { const u8* c = GameItemCol(feet); Slot4(sl, 5, c[0], c[1], c[2], 1); }
    int wpn = EquippedId(EQ_WEAPON);
    if (wpn >= 0) { const u8* c = GameItemCol(wpn); Slot4(sl, 3, c[0], c[1], c[2], 1); }
    int drawn = iS.drawn ? (iDrawUs < DRAW_US / 2) : (iDrawUs > DRAW_US / 2);
    const Mesh& m = iMPlayer[drawn ? 1 : 0];
    int frame;
    int by = iS.py - 410;
    int byaw = iS.yaw;
    int bx = iS.px, bz = iS.pz;
    if (iSitting >= 0) { frame = 25; by = iSitY; byaw = iSitYaw; bx = iSitX; bz = iSitZ; }      // stays seated; only the camera turns
    else if (iUi == UI_DEAD) frame = 24;
    else if (iSwingUs > 0) frame = 16 + Clamp((450000 - iSwingUs) * 8 / 450000, 0, 7);
    else if (iKeys & (KEY_FWD | KEY_BACK | KEY_STRAFE_L | KEY_STRAFE_R)) frame = (iPlayerAnim >> 6) & 15;
    else frame = 0;
    iR->DrawMesh(m, frame, bx, by, bz, byaw, 256, sl, 0);
    if (EquippedId(EQ_SHIELD) == IT_TORCH && iSitting < 0)
        {
        // the torch held out in the left hand, its flame above
        int cs = r3d_cos(byaw), sn = r3d_sin(byaw);
        int lx = -Cm(30), lz = Cm(24);
        int tx = bx + ((lx * cs + lz * sn) >> 14), tz = bz + ((-lx * sn + lz * cs) >> 14), ty = by + Cm(88);
        const u8* c = GameItemCol(IT_TORCH);
        u8 ts[12];
        Slot4(ts, 0, c[0], c[1], c[2], 4); Slot4(ts, 1, c[3], c[4], c[5], 1); Slot4(ts, 2, 0, 0, 0, 1);
        iR->DrawMesh(iMItem[IM_TORCH], 0, tx, ty, tz, byaw, 256, ts, 0);
        u8 fs[12];
        Slot4(fs, 0, 255, 200, 70, 1); Slot4(fs, 1, 240, 96, 24, 1); Slot4(fs, 2, 255, 246, 200, 1);
        iR->DrawMesh(iMFlame, (iTimeUs >> 16) & 7, tx, ty + Cm(52), tz, 0, 70, fs, MF_UNLIT);
        }
    }

// ================================================================ the Hall of the Dead and its catacombs
// (after Whiterun's: a three-aisled stone hall with the shrine of Arkay - here the Keeper - lavender, benches,
// burial niches and the priest's room; below it two levels of catacombs with urn niches, coffins and braziers,
// guarded by three skeletons that rise again)

typedef char KInCountFits[IN_COUNT <= 136 ? 1 : -1];    // iMIn / iInType hold 136
void Game::BuildCryptMeshes()
    {
    R3D& r = *iR;
    Mesh* im = iMIn;
    StaticDef d;
    int i, k;
    // ---- the hall: flagged floor, block walls, a stone ceiling on ribs, the street door, the iron door north
    {
    Mesh& m = im[IN_HALLSHELL];
    r.MeshInit(m, 300, 440, 1);
    BoxC(r, m, 0, -600, -20, -500, 600, 0, 700);              // floors
    BoxC(r, m, 0, -1100, -20, 150, -620, 0, 700);
    BoxC(r, m, 2, -620, 420, -520, 620, 440, 720);            // ceilings
    BoxC(r, m, 2, -1120, 320, 130, -600, 340, 720);
    BoxC(r, m, 1, -620, 0, -520, -90, 420, -500);             // south wall, the door between
    BoxC(r, m, 1, 90, 0, -520, 620, 420, -500);
    BoxC(r, m, 1, -90, 250, -520, 90, 420, -500);
    BoxC(r, m, 4, -88, 0, -500, 88, 248, -494);               // the street door (inside face)
    BoxC(r, m, 3, -110, 0, -500, -88, 272, -486); BoxC(r, m, 3, 88, 0, -500, 110, 272, -486); BoxC(r, m, 3, -110, 248, -500, 110, 272, -486);
    BoxC(r, m, 1, -620, 0, 700, -70, 420, 720);               // north wall, the iron door between
    BoxC(r, m, 1, 70, 0, 700, 620, 420, 720);
    BoxC(r, m, 1, -70, 240, 700, 70, 420, 720);
    BoxC(r, m, 5, -68, 0, 694, 68, 238, 700);                 // the iron door down
    BoxC(r, m, 3, -92, 0, 686, -68, 262, 700); BoxC(r, m, 3, 68, 0, 686, 92, 262, 700); BoxC(r, m, 3, -92, 238, 686, 92, 262, 700);
    BoxC(r, m, 1, 600, 0, -520, 620, 420, 720);               // east
    BoxC(r, m, 1, -620, 0, -520, -600, 420, 360);             // west, the doorway to the priest's room
    BoxC(r, m, 1, -620, 0, 540, -600, 420, 720);
    BoxC(r, m, 1, -620, 230, 360, -600, 420, 540);
    BoxC(r, m, 1, -1120, 0, 130, -620, 320, 150);             // the priest's room
    BoxC(r, m, 1, -1120, 0, 700, -620, 320, 720);
    BoxC(r, m, 1, -1120, 0, 130, -1100, 320, 720);
    for (i = 0; i < 3; i++)                                     // stone ribs across the ceiling, over the pillars
        {
        int z = i == 0 ? -250 : (i == 1 ? 80 : 410);
        BoxC(r, m, 3, -600, 392, z - 20, 600, 420, z + 20);
        }
    BoxC(r, m, 3, -220, 396, -500, -180, 420, 700); BoxC(r, m, 3, 180, 396, -500, 220, 420, 700);
    BoxC(r, m, 3, -600, 0, -500, 600, 14, -490);              // a dark plinth along the walls
    BoxC(r, m, 3, -600, 0, 690, 600, 14, 700);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 112, 106, 98, SL_TEX | TXI_SLATE); Col(d, 1, 124, 116, 104, SL_TEX | TXI_BLOCK); Col(d, 2, 84, 80, 74, SL_TEX | TXI_ROCK);
    Col(d, 3, 78, 72, 64, 1); Col(d, 4, 92, 66, 42, SL_TEX | TXI_PLANKS); Col(d, 5, 58, 58, 62, 1);
    iInType[IN_HALLSHELL] = r.RegisterStatic(d);
    }
    // ---- the catacombs: an upper corridor and side room, two flights of steps, the lower crypt, the amulet room
    {
    Mesh& m = im[IN_CATASHELL];
    r.MeshInit(m, 480, 720, 1);
    // floors
    BoxC(r, m, 0, -480, -20, -100, 800, 0, 300);              // upper corridor
    BoxC(r, m, 0, -1000, -20, -250, -480, 0, 400);            // side room (and its door sill)
    BoxC(r, m, 0, -800, -320, 900, 900, -300, 1600);          // lower crypt
    BoxC(r, m, 0, 900, -320, 900, 1500, -300, 1500);          // amulet room and its doorway
    BoxC(r, m, 0, -800, -320, 420, -540, -300, 900);          // gem alcove
    // ceilings
    BoxC(r, m, 2, -500, 300, -120, 820, 320, 300);
    BoxC(r, m, 2, -1020, 280, -270, -480, 300, 420);
    BoxC(r, m, 2, -420, 300, 300, -100, 320, 600); BoxC(r, m, 2, -420, 150, 600, -100, 170, 900);
    BoxC(r, m, 2, 540, 300, 300, 820, 320, 600); BoxC(r, m, 2, 540, 150, 600, 820, 170, 900);
    BoxC(r, m, 2, -820, 100, 880, 940, 120, 1620);
    BoxC(r, m, 2, 900, 60, 880, 1520, 80, 1520);
    BoxC(r, m, 2, -820, 20, 420, -540, 40, 900);
    // walls (block), with their heights
    BoxC(r, m, 1, -500, 0, -120, 820, 300, -100);             // corridor south
    BoxC(r, m, 5, -68, 0, -100, 68, 238, -94);                // the iron door up to the hall
    BoxC(r, m, 3, -92, 0, -100, -68, 262, -86); BoxC(r, m, 3, 68, 0, -100, 92, 262, -86); BoxC(r, m, 3, -92, 238, -100, 92, 262, -86);
    BoxC(r, m, 1, -1020, 0, -270, -480, 280, -250);           // side room
    BoxC(r, m, 1, -1020, 0, -270, -1000, 280, 420);
    BoxC(r, m, 1, -1020, -300, 400, -480, 280, 420);
    BoxC(r, m, 1, -500, 0, -250, -480, 300, 60);
    BoxC(r, m, 1, -500, 0, 170, -480, 300, 400);
    BoxC(r, m, 1, -500, 220, 60, -480, 300, 170);
    BoxC(r, m, 3, -505, 0, 52, -475, 230, 60); BoxC(r, m, 3, -505, 0, 170, -475, 230, 178); BoxC(r, m, 3, -505, 220, 52, -475, 232, 178);
    BoxC(r, m, 1, -500, -300, 300, -400, 300, 420);           // rock west of the west stairs
    BoxC(r, m, 1, -540, -300, 420, -400, 0, 900);
    BoxC(r, m, 1, -420, 170, 600, -400, 300, 900);
    BoxC(r, m, 1, -120, -300, 300, 560, 300, 900);            // the rock between the flights
    BoxC(r, m, 1, 800, -300, -120, 820, 300, 900);            // east
    BoxC(r, m, 1, 800, -300, 880, 1520, 80, 900);             // amulet room south
    BoxC(r, m, 1, -820, -300, 420, -800, 100, 1620);          // lower crypt west, north, east
    BoxC(r, m, 1, -820, -300, 1600, 940, 100, 1620);
    BoxC(r, m, 1, 900, -300, 880, 940, 100, 1120);
    BoxC(r, m, 1, 900, -300, 1280, 940, 100, 1620);
    BoxC(r, m, 1, 900, -80, 1120, 940, 100, 1280);
    BoxC(r, m, 1, 940, -300, 1500, 1520, 60, 1520);           // amulet room north, east
    BoxC(r, m, 1, 1500, -300, 880, 1520, 60, 1520);
    BoxC(r, m, 1, -800, 0, 880, -540, 100, 900);              // over the alcove's mouth
    BoxC(r, m, 1, -420, 150, 880, -100, 170, 900); BoxC(r, m, 1, 540, 150, 880, 820, 170, 900);   // over the stairs' feet
    for (i = 0; i < 4; i++)                                     // stone ribs across the crypt
        BoxC(r, m, 3, -800, 70, 1000 + i * 180 - 15, 900, 100, 1000 + i * 180 + 15);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 100, 94, 86, SL_TEX | TXI_SLATE); Col(d, 1, 108, 100, 90, SL_TEX | TXI_BLOCK); Col(d, 2, 70, 66, 60, SL_TEX | TXI_ROCK);
    Col(d, 3, 66, 60, 54, 1); Col(d, 4, 118, 112, 102, SL_TEX | TXI_SLATE); Col(d, 5, 54, 54, 58, 1);
    iInType[IN_CATASHELL] = r.RegisterStatic(d);
    }
    // ---- a flight of ten steps down (60 cm deep, 30 cm each), 240 cm wide, starting at z = 0 going +z
    {
    Mesh& m = im[IN_STEPS];
    r.MeshInit(m, 90, 130, 1);
    for (i = 0; i < 10; i++) BoxC(r, m, 0, -120, -320, i * 60, 120, -i * 30 - 15, 60 + i * 60);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 118, 112, 102, SL_TEX | TXI_SLATE);
    iInType[IN_STEPS] = r.RegisterStatic(d);
    }
    // ---- a burial niche: a dark recess with two stone ledges and five urns (back to the wall, facing +z)
    {
    Mesh& m = im[IN_URNS];
    r.MeshInit(m, 200, 300, 1);
    BoxC(r, m, 3, -85, 0, -6, 85, 170, 2);                    // the recess
    BoxC(r, m, 0, -90, 40, -6, 90, 50, 34);                   // ledges
    BoxC(r, m, 0, -90, 110, -6, 90, 120, 30);
    BoxC(r, m, 0, -90, 170, -6, 90, 182, 34);                 // the lintel over it
    for (i = 0; i < 5; i++)
        {
        int top = i < 3, x = top ? -55 + i * 55 : -28 + (i - 3) * 56, y = top ? 50 : 120, h = top ? 34 : 30;
        r.MeshEllipsoid(m, 0, 1, Cm(x), Cm(y + h / 2), Cm(14), Cm(12), Cm(h / 2), Cm(12), 6, 3);
        r.MeshLimb(m, 0, 2, Cm(x), Cm(y + h - 4), Cm(14), Cm(x), Cm(y + h + 4), Cm(14), Cm(7), Cm(7), Cm(8), Cm(8), 5, 2);
        }
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 85, 20, 0, 180);
    Col(d, 0, 116, 108, 96, SL_TEX | TXI_BLOCK); Col(d, 1, 150, 112, 76, 1); Col(d, 2, 110, 96, 62, 1); Col(d, 3, 34, 30, 28, 1);
    iInType[IN_URNS] = r.RegisterStatic(d);
    }
    // ---- a coffin on a stone plinth: tapered plank box, its lid, iron bands (length along z)
    {
    Mesh& m = im[IN_COFFIN];
    r.MeshInit(m, 80, 120, 1);
    BoxC(r, m, 0, -40, 0, -110, 40, 30, 110);
    BoxC(r, m, 1, -30, 30, -100, 30, 76, 100);                // the box
    BoxC(r, m, 1, -33, 76, -103, 33, 84, 103);                // its lid
    BoxC(r, m, 3, -24, 84, -96, 24, 88, 96);                  // a raised board along the lid
    BoxC(r, m, 2, -34, 30, 50, 34, 87, 58); BoxC(r, m, 2, -34, 30, -58, 34, 87, -50);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 40, 105, 0, 88);
    Col(d, 0, 110, 102, 92, SL_TEX | TXI_BLOCK); Col(d, 1, 96, 70, 46, SL_TEX | TXI_PLANKS); Col(d, 2, 60, 60, 64, 1); Col(d, 3, 80, 58, 38, SL_TEX | TXI_PLANKS);
    iInType[IN_COFFIN] = r.RegisterStatic(d);
    }
    // ---- the Keeper's shrine: a stone altar, a golden sun-disc upright on it, two candles
    {
    Mesh& m = im[IN_SHRINE];
    r.MeshInit(m, 120, 180, 1);
    BoxC(r, m, 0, -75, 0, -35, 75, 20, 35);
    BoxC(r, m, 0, -65, 20, -28, 65, 90, 28);
    BoxC(r, m, 0, -75, 90, -35, 75, 100, 35);
    r.MeshLimb(m, 0, 1, 0, Cm(100), Cm(-12), 0, Cm(108), Cm(-12), Cm(10), Cm(6), Cm(8), Cm(5), 6, 2);   // foot
    r.MeshLimb(m, 0, 1, 0, Cm(145), Cm(-16), 0, Cm(145), Cm(-8), Cm(34), Cm(34), Cm(34), Cm(34), 10, 3); // the disc
    r.MeshLimb(m, 0, 3, 0, Cm(145), Cm(-18), 0, Cm(145), Cm(-6), Cm(14), Cm(14), Cm(14), Cm(14), 8, 3); // its dark middle
    r.MeshLimb(m, 0, 1, 0, Cm(108), Cm(-12), 0, Cm(112), Cm(-12), Cm(4), Cm(4), Cm(4), Cm(4), 4, 0);
    for (i = -1; i <= 1; i += 2) r.MeshLimb(m, 0, 2, Cm(i * 55), Cm(100), Cm(10), Cm(i * 55), Cm(124), Cm(10), Cm(4), Cm(4), Cm(4), Cm(4), 5, 2);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 75, 35, 0, 180);
    Col(d, 0, 150, 142, 126, SL_TEX | TXI_BLOCK); Col(d, 1, 214, 176, 80, 1); Col(d, 2, 232, 222, 194, 1); Col(d, 3, 120, 70, 40, 1);
    iInType[IN_SHRINE] = r.RegisterStatic(d);
    }
    // ---- a stone planter of lavender
    {
    Mesh& m = im[IN_PLANTER];
    r.MeshInit(m, 120, 180, 1);
    BoxC(r, m, 0, -32, 0, -32, 32, 40, 32);
    r.MeshEllipsoid(m, 0, 1, 0, Cm(52), 0, Cm(28), Cm(16), Cm(28), 6, 2);
    for (i = 0; i < 5; i++)
        {
        int a = i * 205, x = (r3d_sin(a) * 16) >> 14, z = (r3d_cos(a) * 16) >> 14;
        r.MeshLimb(m, 0, 2, Cm(x), Cm(56), Cm(z), Cm(x * 3 / 2), Cm(86), Cm(z * 3 / 2), Cm(5), Cm(5), Cm(3), Cm(3), 4, 2);
        }
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 32, 32, 0, 40);
    Col(d, 0, 134, 126, 112, SL_TEX | TXI_BLOCK); Col(d, 1, 92, 116, 76, 1); Col(d, 2, 148, 116, 196, 1);
    iInType[IN_PLANTER] = r.RegisterStatic(d);
    }
    // ---- an iron brazier on three legs, glowing coals in the bowl (the flame goes on top)
    {
    Mesh& m = im[IN_BRAZIER];
    r.MeshInit(m, 60, 90, 1);
    for (i = 0; i < 3; i++)
        {
        int a = i * 341, x = (r3d_sin(a) * 34) >> 14, z = (r3d_cos(a) * 34) >> 14;
        r.MeshLimb(m, 0, 0, Cm(x), 0, Cm(z), Cm(x / 2), Cm(80), Cm(z / 2), Cm(3), Cm(3), Cm(3), Cm(3), 3, 0);
        }
    r.MeshLimb(m, 0, 0, 0, Cm(70), 0, 0, Cm(96), 0, Cm(14), Cm(14), Cm(38), Cm(38), 8, 1);
    r.MeshEllipsoid(m, 0, 1, 0, Cm(94), 0, Cm(32), Cm(6), Cm(32), 6, 2);
    r.MeshFinish(m);
    Def0(d, &m, 0, 0, 38, 96);
    Col(d, 0, 52, 50, 50, 1); Col(d, 1, 230, 110, 40, 1);
    iInType[IN_BRAZIER] = r.RegisterStatic(d);
    }
    // ---- a stone pillar for the hall (base, shaft, capital)
    {
    Mesh& m = im[IN_PILLAR];
    r.MeshInit(m, 30, 40, 1);
    BoxC(r, m, 1, -46, 0, -46, 46, 22, 46);
    BoxC(r, m, 0, -34, 22, -34, 34, 396, 34);
    BoxC(r, m, 1, -46, 396, -46, 46, 420, 46);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 46, 420);
    Col(d, 0, 128, 120, 108, SL_TEX | TXI_BLOCK); Col(d, 1, 92, 86, 78, 1);
    iInType[IN_PILLAR] = r.RegisterStatic(d);
    }
    }

// a brazier with its fire and light (cm, y = the floor there)
static void InBrazier(Game& g, int aCell, int aX, int aY, int aZ)
    {
    g.IPlace(aCell, IN_BRAZIER, aX, aY, aZ, 0, 256);
    g.IPlace(aCell, IN_FLAME, aX, aY + 96, aZ, 0, 300);
    g.ILight(aCell, aX, aY + 140, aZ, 800, 170);
    }

void Game::BuildHallDeadInterior()
    {
    int c = CELL_HALLDEAD, i;
    IPlace(c, IN_HALLSHELL, 0, 0, 0, 0, 256);
    // two rows of pillars make the three aisles
    for (i = 0; i < 6; i++) IPlace(c, IN_PILLAR, i & 1 ? 200 : -200, 0, i / 2 == 0 ? -250 : (i / 2 == 1 ? 80 : 410), 0, 256);
    // the middle aisle: six planters of lavender
    static const int KPlant[9 * 2] = { -130, 480,  130, 480,  -130, 250,  130, 250,  -130, -360,  130, -360,   // middle
                                       -540, -430,  -540, -330,  -540, 630 };                               // west aisle
    for (i = 0; i < 9; i++) IPlace(c, IN_PLANTER, KPlant[i * 2], 0, KPlant[i * 2 + 1], i * 100, 256);
    // the chapel in the east aisle: the Keeper's shrine against the north wall, candles beside it
    IPlace(c, IN_SHRINE, 400, 0, 640, 512, 256);
    ILight(c, 400, 160, 600, 500, 120);
    // burial niches in the walls
    static const int KNiche[8 * 3] = { 598, -350, 768,   598, -50, 768,   598, 250, 768,   -598, -150, 256,
                                       -400, 698, 512,   -400, -498, 0,   200, 698, 512,   400, -498, 0 };
    for (i = 0; i < 8; i++) IPlace(c, IN_URNS, KNiche[i * 3], 0, KNiche[i * 3 + 1], KNiche[i * 3 + 2], 256);
    // light: torches on the walls, a brazier at each end of the middle aisle
    WallTorch(c, 598, 100, 768);
    WallTorch(c, -598, -400, 256);
    WallTorch(c, -598, 620, 256);
    InBrazier(*this, c, -330, 0, 600);
    InBrazier(*this, c, 330, 0, -430);
    // the priest's room: a table and a bookshelf (the bed, wardrobes, chest and chair are props)
    IPlace(c, IN_TABLE, -820, 0, 210, 0, 256);
    IPlace(c, IN_CANDLE, -790, 78, 230, 0, 256);
    IPlace(c, IN_FLAME, -790, 96, 230, 0, 60);
    ILight(c, -790, 100, 230, 400, 90);
    IPlace(c, IN_SHELF, -1098, 0, 250, 256, 256);
    IPlace(c, IN_RUG, -900, 0, 420, 256, 230);
    WallTorch(c, -1098, 520, 256);
    }

void Game::BuildCatacombs()
    {
    int c = CELL_CATACOMBS, i;
    IPlace(c, IN_CATASHELL, 0, 0, 0, 0, 256);
    IPlace(c, IN_STEPS, -260, 0, 300, 0, 256);
    IPlace(c, IN_STEPS, 680, 0, 300, 0, 256);
    // urn niches: the corridor, the side room, the lower crypt
    static const int KNiche[9 * 4] =
        {
        // x, y (floor), z, yaw
        300, 0, -98, 0,   600, 0, -98, 0,   200, 0, 298, 512,   -998, 0, 0, 256,   -998, 0, 200, 256,
        -798, -300, 1100, 256,   -798, -300, 1350, 256,   200, -300, 902, 0,   -260, -300, 1598, 512
        };
    for (i = 0; i < 9; i++) IPlace(c, IN_URNS, KNiche[i * 4], KNiche[i * 4 + 1], KNiche[i * 4 + 2], KNiche[i * 4 + 3], 256);
    // coffins along the lower crypt's north wall and in its middle
    static const int KCoffin[7 * 3] = { -560, 1490, 256,   20, 1490, 256,   520, 1490, 256,   -100, 1180, 0,   250, 1180, 0,   1150, 1420, 256,  -755, 700, 0 };
    for (i = 0; i < 7; i++) IPlace(c, IN_COFFIN, KCoffin[i * 3], -300, KCoffin[i * 3 + 1], KCoffin[i * 3 + 2], 256);
    // the side room: two tables, a candle
    IPlace(c, IN_TABLE, -880, 0, 280, 0, 256);
    IPlace(c, IN_TABLE, -880, 0, -140, 0, 256);
    IPlace(c, IN_CANDLE, -850, 78, 300, 0, 256);
    IPlace(c, IN_FLAME, -850, 96, 300, 0, 60);
    ILight(c, -850, 100, 300, 450, 90);
    // light: a torch in the corridor, braziers below
    WallTorch(c, 400, 298, 512);
    WallTorch(c, -300, -98, 0);
    InBrazier(*this, c, -400, -300, 1300);
    InBrazier(*this, c, 560, -300, 1300);
    // the amulet room: a brazier against the east wall, a small table to its right, a coffin
    InBrazier(*this, c, 1400, -300, 1200);
    IPlace(c, IN_ENDTABLE, 1420, -300, 1060, 0, 256);
    ILight(c, 1420, -230, 1060, 260, 50);
    }

void Game::ReviveSkeletons()
    {
    // the dead of the catacombs do not stay down: ten days after one falls it stands again at its post
    int i;
    for (i = 0; i < iS.ne; i++)
        {
        Ent& e = iS.e[i];
        if (e.type != E_SKELETON || e.alive || iS.day - e.cool < 10) continue;
        e.alive = 1; e.hp = e.maxhp; e.state = S_IDLE; e.aggro = 0; e.looted &= ~1; e.atk = 0;
        e.x = e.hx; e.z = e.hz; e.y = FloorAt(e.cell, e.x, e.z, 0);
        }
    }

// ================================================================== the Bitterroot (after Whiterun's Arcadia's Cauldron)
// A two-storey shop: the trading room (fireplace, the alchemy lab in the middle, the long desk on the east side),
// a study, a locked storeroom and the stairs on the west side; upstairs a gallery over the shop, a kitchen corner,
// the apothecary's own room. Walls and floors know which storey they belong to.
// (functions below are referenced from the shared tables near the top of the file)
void Game::BuildShopMeshes()
    {
    R3D& r = *iR;
    Mesh* im = iMIn;
    StaticDef d;
    int i;
    // ---- the ground floor's shell: planked floor, timber walls, the doors
    {
    Mesh& m = im[IN_SHOPSHELL];
    r.MeshInit(m, 400, 600, 1);
    BoxC(r, m, 6, -300, -20, 0, 1000, 0, 900);                // floors: flagstones in the shop, the storeroom, the stair hall
    BoxC(r, m, 0, 400, -20, -300, 800, 0, 0);                 // the study
    BoxC(r, m, 1, -330, 0, 900, 1030, 640, 930);              // north wall (the front door's inside face below)
    BoxC(r, m, 4, 240, 0, 894, 360, 230, 900);
    BoxC(r, m, 3, 222, 0, 886, 240, 246, 900); BoxC(r, m, 3, 360, 0, 886, 378, 246, 900); BoxC(r, m, 3, 222, 230, 886, 378, 248, 900);
    BoxC(r, m, 1, 1000, 0, -30, 1030, 640, 930);              // east wall, the back door
    BoxC(r, m, 4, 994, 0, 640, 1000, 220, 760);
    BoxC(r, m, 3, 986, 0, 622, 1000, 236, 640); BoxC(r, m, 3, 986, 0, 760, 1000, 236, 778);
    BoxC(r, m, 1, -330, 0, -30, -300, 640, 930);              // west (outer)
    BoxC(r, m, 1, -330, 0, -30, 520, 320, 0);                 // south, the study's door between
    BoxC(r, m, 1, 660, 0, -30, 1030, 320, 0);
    BoxC(r, m, 1, 520, 230, -30, 660, 320, 0);
    BoxC(r, m, 1, 500, 320, -30, 1030, 640, 0);
    BoxC(r, m, 1, 380, 0, -330, 400, 320, 0);                 // the study's walls and ceiling
    BoxC(r, m, 1, 800, 0, -330, 820, 320, 0);
    BoxC(r, m, 1, 380, 0, -330, 820, 320, -300);
    BoxC(r, m, 2, 400, 300, -300, 800, 320, 0);
    BoxC(r, m, 1, -15, 0, -30, 5, 320, 440);                  // the shop's west wall, the stairs' doorway
    BoxC(r, m, 1, -15, 0, 580, 5, 320, 930);
    BoxC(r, m, 1, -15, 230, 440, 5, 320, 580);
    BoxC(r, m, 5, -9, 0, 165, 5, 225, 265);                   // the storeroom's locked iron-bound door (shop side)
    BoxC(r, m, 1, -300, 0, 400, -15, 320, 420);               // storeroom / stair hall
    BoxC(r, m, 2, -300, 300, 0, -15, 320, 420);               // the storeroom's ceiling, the solid upper part over it
    BoxC(r, m, 1, -300, 320, 0, -15, 640, 420);
    BoxC(r, m, 2, -330, 640, -30, 1030, 660, 930);            // the roof's underside
    for (i = 0; i < 4; i++) BoxC(r, m, 3, 300 + i * 200 - 14, 600, -30, 300 + i * 200 + 14, 640, 930);   // roof beams
    BoxC(r, m, 3, 300 - 14, 420, 0, 300 + 14, 600, 900);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 0, 0);
    PLANK(d, 0); Col(d, 1, 178, 164, 136, 6); Col(d, 2, 90, 70, 50, SL_TEX | TXI_PLANKS); DARKW(d, 3); Col(d, 4, 86, 60, 38, SL_TEX | TXI_PLANKS); Col(d, 5, 70, 68, 70, 1);
    Col(d, 6, 112, 106, 98, SL_TEX | TXI_SLATE);
    iInType[IN_SHOPSHELL] = r.RegisterStatic(d);
    }
    // ---- the upper floor: the gallery and landing on their joists, a railing, the apothecary's room
    {
    Mesh& m = im[IN_SHOPUPPER];
    r.MeshInit(m, 320, 480, 1);
    BoxC(r, m, 0, 0, 300, 0, 260, 320, 900);                  // gallery floor
    BoxC(r, m, 0, -150, 300, 420, 0, 320, 900);               // landing beside the stairwell
    BoxC(r, m, 0, -300, 300, 820, -150, 320, 900);
    BoxC(r, m, 0, 0, 300, -350, 500, 320, 0);                 // her room's floor
    BoxC(r, m, 3, 255, 280, 0, 275, 320, 900);                // the gallery's edge beam
    BoxC(r, m, 3, 258, 400, 0, 272, 410, 900);                // railing, its posts
    for (i = 0; i < 6; i++) BoxC(r, m, 3, 259, 320, i * 175 + 5, 271, 400, i * 175 + 17);
    BoxC(r, m, 3, -160, 400, 440, -150, 410, 770);            // round the stairwell
    for (i = 0; i < 3; i++) BoxC(r, m, 3, -161, 320, 440 + i * 160, -149, 400, 452 + i * 160);
    BoxC(r, m, 3, -300, 320, 400, -150, 410, 420);
    BoxC(r, m, 1, -20, 320, -380, 0, 620, 0);                 // her room: walls, the door from the gallery, ceiling
    BoxC(r, m, 1, 500, 320, -380, 520, 620, 0);
    BoxC(r, m, 1, -20, 320, -380, 520, 620, -350);
    BoxC(r, m, 1, 0, 320, -15, 60, 620, 5);
    BoxC(r, m, 1, 160, 320, -15, 500, 620, 5);
    BoxC(r, m, 1, 60, 530, -15, 160, 620, 5);
    BoxC(r, m, 2, -20, 600, -380, 520, 620, 0);
    for (i = 0; i < 4; i++) BoxC(r, m, 3, i * 70 - 4, 290, 0, i * 70 + 8, 300, 900);    // joists under the gallery
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 0, 0);
    PLANK(d, 0); Col(d, 1, 178, 164, 136, 6); Col(d, 2, 90, 70, 50, SL_TEX | TXI_PLANKS); DARKW(d, 3);
    iInType[IN_SHOPUPPER] = r.RegisterStatic(d);
    }
    // ---- the stairs: nine treads up 320 cm over 420 cm (going +z), 140 cm wide, with a stringer
    {
    Mesh& m = im[IN_SHOPSTAIRS];
    r.MeshInit(m, 96, 144, 1);
    for (i = 0; i < 9; i++) BoxC(r, m, 0, -70, 0, i * 39, 70, i * 36 + 36, i * 39 + 39);       // each step solid to the floor
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 0, 0);
    PLANK(d, 0); DARKW(d, 1);
    iInType[IN_SHOPSTAIRS] = r.RegisterStatic(d);
    }
    // ---- the alchemy lab (as Whiterun's apothecary has it): a round table on a pedestal, flasks of green and blue,
    //      a still, a mortar and pestle
    {
    Mesh& m = im[IN_ALCHLAB];
    r.MeshInit(m, 220, 330, 1);
    r.MeshLimb(m, 0, 0, 0, Cm(80), 0, 0, Cm(88), 0, Cm(56), Cm(56), Cm(56), Cm(56), 10, 3);                 // top
    r.MeshLimb(m, 0, 1, 0, 0, 0, 0, Cm(80), 0, Cm(14), Cm(14), Cm(10), Cm(10), 6, 0);                      // pedestal
    r.MeshLimb(m, 0, 1, 0, 0, 0, 0, Cm(8), 0, Cm(40), Cm(40), Cm(36), Cm(36), 8, 2);                       // foot
    for (i = 0; i < 3; i++)
        {
        int a = i * 341 + 60, x = (r3d_sin(a) * 30) >> 14, z = (r3d_cos(a) * 30) >> 14;
        r.MeshEllipsoid(m, 0, 4 + (i & 1), Cm(x), Cm(98), Cm(z), Cm(9), Cm(10), Cm(9), 6, 3);             // round flasks
        r.MeshLimb(m, 0, 4 + (i & 1), Cm(x), Cm(106), Cm(z), Cm(x), Cm(122), Cm(z), Cm(3), Cm(3), Cm(3), Cm(3), 4, 0);
        r.MeshLimb(m, 0, 6, Cm(x), Cm(122), Cm(z), Cm(x), Cm(126), Cm(z), Cm(4), Cm(4), Cm(4), Cm(4), 4, 2);
        }
    r.MeshEllipsoid(m, 0, 2, Cm(-8), Cm(100), Cm(-8), Cm(12), Cm(12), Cm(12), 6, 3);                       // the still
    r.MeshLimb(m, 0, 2, Cm(-8), Cm(110), Cm(-8), Cm(18), Cm(118), Cm(-20), Cm(2), Cm(2), Cm(2), Cm(2), 4, 0);
    r.MeshLimb(m, 0, 7, Cm(26), Cm(88), Cm(14), Cm(26), Cm(96), Cm(14), Cm(8), Cm(8), Cm(10), Cm(10), 6, 1);   // mortar, pestle
    r.MeshLimb(m, 0, 7, Cm(28), Cm(94), Cm(14), Cm(36), Cm(106), Cm(10), Cm(2), Cm(2), Cm(2), Cm(2), 4, 3);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 56, 56, 0, 130);
    Col(d, 0, 104, 72, 46, SL_TEX | TXI_PLANKS); DARKW(d, 1); Col(d, 2, 196, 120, 70, 1); Col(d, 3, 50, 48, 46, 1);
    Col(d, 4, 90, 170, 110, 1); Col(d, 5, 80, 150, 190, 1); Col(d, 6, 90, 60, 40, 1); Col(d, 7, 150, 140, 126, 1);
    iInType[IN_ALCHLAB] = r.RegisterStatic(d);
    }
    // ---- the long desk (length along z), a front panel, a shelf behind
    {
    Mesh& m = im[IN_COUNTER];
    r.MeshInit(m, 60, 90, 1);
    BoxC(r, m, 0, -32, 0, -190, 32, 92, 190);
    BoxC(r, m, 1, -38, 92, -196, 38, 100, 196);
    BoxC(r, m, 2, -36, 10, -186, -32, 84, 186);
    BoxC(r, m, 1, -40, 0, -196, 40, 8, 196);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 38, 196, 0, 100);
    Col(d, 0, 92, 52, 34, SL_TEX | TXI_FURN); DARKW(d, 1); Col(d, 2, 108, 64, 40, SL_TEX | TXI_PLANKS);
    iInType[IN_COUNTER] = r.RegisterStatic(d);
    }
    // ---- a stone fireplace with its chimney breast (back to the wall, opening towards +z)
    {
    Mesh& m = im[IN_FIREPLACE];
    r.MeshInit(m, 60, 90, 1);
    BoxC(r, m, 0, -120, 0, -40, -70, 150, 30);                // jambs
    BoxC(r, m, 0, 70, 0, -40, 120, 150, 30);
    BoxC(r, m, 0, -130, 150, -40, 130, 175, 38);              // mantel
    BoxC(r, m, 0, -95, 175, -40, 95, 640, 16);                // chimney breast up to the roof
    BoxC(r, m, 1, -70, 0, -40, 70, 150, -30);                 // sooty back
    BoxC(r, m, 0, -130, 0, 30, 130, 8, 70);                   // hearth stone
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 130, 40, 0, 175);
    Col(d, 0, 132, 124, 112, SL_TEX | TXI_BLOCK); Col(d, 1, 26, 22, 20, 1);
    iInType[IN_FIREPLACE] = r.RegisterStatic(d);
    }
    // ---- a fire pit sunk into the flagstones: a stone kerb, dark embers, half-burnt logs (the flame goes on top)
    {
    Mesh& m = im[IN_FIREPIT2];
    r.MeshInit(m, 100, 150, 1);
    BoxC(r, m, 1, -115, -2, -85, 115, 1, 85);                 // the pit's dark floor
    BoxC(r, m, 0, -130, -2, -100, 130, 10, -82);              // the kerb
    BoxC(r, m, 0, -130, -2, 82, 130, 10, 100);
    BoxC(r, m, 0, -130, -2, -82, -112, 10, 82);
    BoxC(r, m, 0, 112, -2, -82, 130, 10, 82);
    r.MeshEllipsoid(m, 0, 2, 0, Cm(4), 0, Cm(70), Cm(6), Cm(50), 6, 2);                                      // embers
    r.MeshLimb(m, 0, 3, Cm(-50), Cm(8), Cm(-20), Cm(50), Cm(10), Cm(20), Cm(8), Cm(8), Cm(7), Cm(7), 5, 3);   // logs
    r.MeshLimb(m, 0, 3, Cm(-40), Cm(10), Cm(30), Cm(45), Cm(8), Cm(-25), Cm(7), Cm(7), Cm(7), Cm(7), 5, 3);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 115, 10);
    Col(d, 0, 96, 92, 86, SL_TEX | TXI_BLOCK); Col(d, 1, 24, 20, 18, 1); Col(d, 2, 220, 100, 36, 1); Col(d, 3, 50, 36, 26, 1);
    iInType[IN_FIREPIT2] = r.RegisterStatic(d);
    }
    // ---- the short arm of the L-shaped counter (length along z, 150 cm)
    {
    Mesh& m = im[IN_COUNTER2];
    r.MeshInit(m, 40, 60, 1);
    BoxC(r, m, 0, -32, 0, -75, 32, 92, 75);
    BoxC(r, m, 1, -38, 92, -81, 38, 100, 81);
    BoxC(r, m, 2, -36, 10, -71, -32, 84, 71);
    BoxC(r, m, 1, -40, 0, -81, 40, 8, 81);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 38, 81, 0, 100);
    Col(d, 0, 92, 52, 34, SL_TEX | TXI_FURN); DARKW(d, 1); Col(d, 2, 108, 64, 40, SL_TEX | TXI_PLANKS);
    iInType[IN_COUNTER2] = r.RegisterStatic(d);
    }
    // ---- woven mats hung as decoration (facing +z): a hexagonal one and a round one, in three rings of colour
    for (i = 0; i < 2; i++)
        {
        Mesh& m = im[i ? IN_MATROUND : IN_MATHEX];
        int sides = i ? 12 : 6, rr = i ? 26 : 32;
        r.MeshInit(m, 80, 120, 1);
        r.MeshLimb(m, 0, 0, 0, 0, 0, 0, 0, Cm(2), Cm(rr), Cm(rr), Cm(rr), Cm(rr), sides, 3);
        r.MeshLimb(m, 0, 1, 0, 0, Cm(2), 0, 0, Cm(3), Cm(rr * 7 / 10), Cm(rr * 7 / 10), Cm(rr * 7 / 10), Cm(rr * 7 / 10), sides, 3);
        r.MeshLimb(m, 0, 2, 0, 0, Cm(3), 0, 0, Cm(4), Cm(rr * 4 / 10), Cm(rr * 4 / 10), Cm(rr * 4 / 10), Cm(rr * 4 / 10), sides, 3);
        r.MeshFinish(m);
        Def0(d, &m, 0, 0, 0, 0);
        Col(d, 0, 150, 116, 70, 6); Col(d, 1, i ? 70 : 150, i ? 110 : 70, i ? 80 : 50, 6); Col(d, 2, 196, 168, 100, 6);
        iInType[i ? IN_MATROUND : IN_MATHEX] = r.RegisterStatic(d);
        }
    // ---- a woven rug in stripes of slate blue, rust and mustard (300 x 180 cm)
    {
    Mesh& m = im[IN_RUG2];
    r.MeshInit(m, 80, 120, 1);
    for (i = 0; i < 9; i++) BoxC(r, m, i % 3, -150, 0, -90 + i * 20, 150, 1, -70 + i * 20);
    r.MeshFinish(m);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 92, 112, 120, 6); Col(d, 1, 156, 82, 56, 6); Col(d, 2, 176, 150, 84, 6);
    iInType[IN_RUG2] = r.RegisterStatic(d);
    }
    // ---- a hutch: a cupboard below, open shelves above with candles (back to the wall, facing +z)
    {
    Mesh& m = im[IN_HUTCH];
    r.MeshInit(m, 100, 150, 1);
    BoxC(r, m, 0, -60, 0, -22, 60, 90, 22);                   // the cupboard
    BoxC(r, m, 2, -54, 10, 22, -2, 82, 24); BoxC(r, m, 2, 2, 10, 22, 54, 82, 24);   // its doors
    BoxC(r, m, 0, -60, 90, -22, 60, 96, 22);
    BoxC(r, m, 0, -60, 96, -22, 60, 210, -16);                // back of the shelves
    BoxC(r, m, 0, -60, 96, -16, -54, 210, 18); BoxC(r, m, 0, 54, 96, -16, 60, 210, 18);
    BoxC(r, m, 0, -54, 150, -16, 54, 155, 18);                // shelf
    BoxC(r, m, 0, -62, 205, -22, 62, 214, 22);                // top
    r.MeshLimb(m, 0, 3, Cm(-40), Cm(155), Cm(6), Cm(-40), Cm(170), Cm(6), Cm(3), Cm(3), Cm(3), Cm(3), 4, 2);  // candles
    r.MeshLimb(m, 0, 3, Cm(42), Cm(96), Cm(6), Cm(42), Cm(114), Cm(6), Cm(3), Cm(3), Cm(3), Cm(3), 4, 2);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 62, 22, 0, 214);
    Col(d, 0, 96, 66, 42, SL_TEX | TXI_FURN); DARKW(d, 1); Col(d, 2, 110, 74, 48, SL_TEX | TXI_PLANKS); Col(d, 3, 232, 222, 196, 1);
    iInType[IN_HUTCH] = r.RegisterStatic(d);
    }
    // ---- carved double doors (slate-blue, arched panels) standing open, in a heavy frame (the doorway is x -70..70)
    {
    Mesh& m = im[IN_DBLDOOR];
    r.MeshInit(m, 80, 120, 1);
    BoxC(r, m, 0, -88, 0, -10, -70, 250, 12);                 // frame
    BoxC(r, m, 0, 70, 0, -10, 88, 250, 12);
    BoxC(r, m, 0, -88, 230, -10, 88, 252, 12);
    BoxC(r, m, 1, -70, 0, -76, -64, 228, -10);                // the leaves, swung in against the jambs
    BoxC(r, m, 1, 64, 0, -76, 70, 228, -10);
    BoxC(r, m, 2, -64, 20, -68, -62, 200, -18);               // their carved panels (inner faces)
    BoxC(r, m, 2, 62, 20, -68, 64, 200, -18);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 0, 0);
    DARKW(d, 0); Col(d, 1, 92, 112, 112, SL_TEX | TXI_PLANKS); Col(d, 2, 140, 120, 80, 1);
    iInType[IN_DBLDOOR] = r.RegisterStatic(d);
    }
    // ---- an iron cauldron on three stubby feet
    {
    Mesh& m = im[IN_CAULDRON];
    r.MeshInit(m, 90, 130, 1);
    r.MeshEllipsoid(m, 0, 0, 0, Cm(30), 0, Cm(34), Cm(26), Cm(34), 8, 3);
    r.MeshLimb(m, 0, 0, 0, Cm(44), 0, 0, Cm(52), 0, Cm(30), Cm(30), Cm(32), Cm(32), 8, 0);
    r.MeshLimb(m, 0, 1, 0, Cm(46), 0, 0, Cm(48), 0, Cm(28), Cm(28), Cm(28), Cm(28), 8, 2);
    for (i = 0; i < 3; i++) { int a = i * 341, x = (r3d_sin(a) * 24) >> 14, z = (r3d_cos(a) * 24) >> 14; Rod(r, m, 0, x, 0, z, x, 10, z, 4, 3); }
    r.MeshFinish(m);
    Def0(d, &m, 0, 0, 36, 52);
    Col(d, 0, 52, 52, 54, 1); Col(d, 1, 60, 80, 50, 1);
    iInType[IN_CAULDRON] = r.RegisterStatic(d);
    }
    // ---- the timber frame of the shop's plaster walls: posts, the beam at the gallery's height, the wall plate, braces
    {
    Mesh& m = im[IN_SHOPFRAME];
    r.MeshInit(m, 300, 450, 1);
    static const int KPost[] = { 20, 886,  240, 886,  470, 886,  700, 886,  980, 886,     // north wall (x, z)
                                 986, 20,  986, 250,  986, 520,  986, 610,  986, 790,     // east wall
                                 20, 14,  400, 14,  800, 14,  980, 14 };                  // south wall
    for (i = 0; i < 14; i++) BoxC(r, m, 0, KPost[i * 2] - 12, 0, KPost[i * 2 + 1] - 12, KPost[i * 2] + 12, 640, KPost[i * 2 + 1] + 12);
    BoxC(r, m, 0, 0, 300, 884, 1000, 326, 900);               // beams at the gallery's height
    BoxC(r, m, 0, 984, 300, 0, 1000, 326, 900);
    BoxC(r, m, 0, 0, 300, 0, 1000, 326, 16);
    BoxC(r, m, 0, 0, 590, 884, 1000, 610, 900);               // wall plates
    BoxC(r, m, 0, 984, 590, 0, 1000, 610, 900);
    static const int KBrace[] = { 240, 886, 1,  470, 886, -1,  700, 886, 1,  986, 250, 1,  986, 520, -1,  986, 790, -1 };
    for (i = 0; i < 6; i++)
        {
        int x = KBrace[i * 3], z = KBrace[i * 3 + 1], s = KBrace[i * 3 + 2];
        if (z > 800) Rod(r, m, 0, x, 450, z, x + s * 110, 590, z, 6, 4);
        else Rod(r, m, 0, x, 450, z, x, 590, z + s * 110, 6, 4);
        }
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 86, 62, 40, SL_TEX | TXI_LOGS);
    iInType[IN_SHOPFRAME] = r.RegisterStatic(d);
    }
    // ---- the gallery's railing: two rails with a zigzag of carved laths between (900 cm along z)
    {
    Mesh& m = im[IN_RAILING];
    r.MeshInit(m, 200, 300, 1);
    BoxC(r, m, 0, -6, 92, 0, 6, 100, 900);
    BoxC(r, m, 0, -6, 0, 0, 6, 8, 900);
    for (i = 0; i < 15; i++)
        {
        int z = i * 60;
        Rod(r, m, 1, 0, 8, z, 0, 92, z + 30, 2, 3);
        Rod(r, m, 1, 0, 92, z + 30, 0, 8, z + 60, 2, 3);
        }
    r.MeshFinish(m);
    Def0(d, &m, 0, 0, 0, 0);
    DARKW(d, 0); Col(d, 1, 120, 92, 60, 4);
    iInType[IN_RAILING] = r.RegisterStatic(d);
    }
    // ---- bundles of drying herbs hanging on strings from a pole (along x)
    {
    Mesh& m = im[IN_HERBS];
    r.MeshInit(m, 120, 180, 1);
    Rod(r, m, 0, -90, 0, 0, 90, 0, 0, 2, 4);
    for (i = 0; i < 6; i++)
        {
        int x = -75 + i * 30;
        Rod(r, m, 0, x, 0, 0, x, -18, 0, 1, 3);
        r.MeshLimb(m, 0, 1 + i % 3, Cm(x), Cm(-18), 0, Cm(x), Cm(-48), 0, Cm(4), Cm(4), Cm(9), Cm(9), 5, 2);
        }
    r.MeshFinish(m);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 120, 100, 70, 1); Col(d, 1, 110, 140, 70, 1); Col(d, 2, 150, 120, 190, 1); Col(d, 3, 170, 150, 90, 1);
    iInType[IN_HERBS] = r.RegisterStatic(d);
    }
    }

void Game::BuildShopInterior()
    {
    int c = CELL_SHOP;
    IPlace(c, IN_SHOPSHELL, 0, 0, 0, 0, 256);
    IPlace(c, IN_SHOPUPPER, 0, 0, 0, 0, 256);
    IPlace(c, IN_SHOPSTAIRS, -220, 0, 466, 0, 256);
    // the trading room, as Whiterun's: plaster walls in a timber frame, a fire pit sunk into the middle of the floor,
    // woven rugs round it; the L-shaped counter on the east side hung with woven mats, the apothecary behind it with
    // shelves of jars and herbs drying overhead; on the south wall the hutch, the carved doors to her study, a cauldron
    // and the round alchemy table; the gallery's carved railing above
    IPlace(c, IN_SHOPFRAME, 0, 0, 0, 0, 256);
    IPlace(c, IN_RAILING, 266, 320, 0, 0, 256);
    IPlace(c, IN_FIREPIT2, 480, 0, 480, 0, 256);
    IPlace(c, IN_FLAME, 480, 6, 480, 0, 560);
    ILight(c, 480, 100, 480, 1100, 210);
    IPlace(c, IN_RUG2, 480, 0, 720, 0, 256);
    IPlace(c, IN_RUG2, 480, 0, 240, 0, 230);
    IPlace(c, IN_RUG2, 690, 0, 480, 256, 200);
    IPlace(c, IN_COUNTER, 850, 0, 420, 0, 256);
    IPlace(c, IN_COUNTER2, 735, 0, 205, 256, 256);
    IPlace(c, IN_MATHEX, 811, 50, 330, 768, 256);
    IPlace(c, IN_MATROUND, 811, 50, 530, 768, 256);
    IPlace(c, IN_MATHEX, 700, 50, 244, 0, 220);
    IPlace(c, IN_MATROUND, 997, 175, 300, 768, 256);
    IPlace(c, IN_MATHEX, 997, 170, 470, 768, 256);
    IPlace(c, IN_MATROUND, 150, 175, 899, 512, 256);
    IPlace(c, IN_MATHEX, 600, 170, 899, 512, 230);
    IPlace(c, IN_SHELF, 985, 0, 300, 768, 256);
    IPlace(c, IN_SHELF, 985, 0, 600, 768, 256);
    IPlace(c, IN_SHELF, 20, 0, 130, 256, 256);
    IPlace(c, IN_HERBS, 900, 300, 300, 256, 256);
    IPlace(c, IN_HERBS, 900, 300, 560, 256, 256);
    IPlace(c, IN_HERBS, 640, 300, 110, 0, 256);
    IPlace(c, IN_HERBS, 380, 300, 760, 0, 256);
    IPlace(c, IN_CANDLE, 860, 100, 600, 0, 256); IPlace(c, IN_FLAME, 860, 118, 600, 0, 60);
    ILight(c, 860, 120, 600, 450, 90);
    IPlace(c, IN_HUTCH, 735, 0, 24, 0, 256);
    IPlace(c, IN_FLAME, 695, 172, 30, 0, 50); IPlace(c, IN_FLAME, 777, 116, 30, 0, 50);
    ILight(c, 735, 160, 60, 400, 80);
    IPlace(c, IN_DBLDOOR, 590, 0, 0, 0, 256);
    IPlace(c, IN_CAULDRON, 450, 0, 50, 0, 256);
    IPlace(c, IN_TABLE, 160, 0, 740, 0, 256);
    IPlace(c, IN_SACKS, 940, 0, 70, 0, 220);
    WallTorch(c, 998, 120, 768);
    WallTorch(c, 400, 898, 512);
    WallTorch(c, 20, 700, 256);
    // the study: a desk with a bowl of apples and the herbwife's book on it, a shelf of books
    IPlace(c, IN_TABLE, 560, 0, -230, 0, 256);
    IPlace(c, IN_SHELF, 798, 0, -150, 768, 256);
    IPlace(c, IN_CANDLE, 590, 78, -240, 0, 256); IPlace(c, IN_FLAME, 590, 96, -240, 0, 60);
    ILight(c, 590, 100, -240, 400, 80);
    // the storeroom (barrels and the chest are props); the stair hall
    WallTorch(c, -298, 650, 256);
    // upstairs: the kitchen corner's table, a candle; her room's table
    IPlace(c, IN_TABLE, 90, 320, 800, 0, 256);
    IPlace(c, IN_CANDLE, 60, 398, 820, 0, 256); IPlace(c, IN_FLAME, 60, 416, 820, 0, 60);
    ILight(c, 60, 420, 820, 500, 90);
    IPlace(c, IN_TABLE, 380, 320, -110, 0, 256);
    IPlace(c, IN_CANDLE, 410, 398, -100, 0, 256); IPlace(c, IN_FLAME, 410, 416, -100, 0, 60);
    ILight(c, 410, 420, -100, 450, 90);
    IPlace(c, IN_RUG, 220, 320, -180, 0, 220);
    }

// ================================================================== the Gilded Stag (after Whiterun's Bannered Mare)
// A cross: the common room in the middle (front door west, the fire pit at the centre), the bar to the south,
// the kitchen to the north with the innkeeper's and the serving maid's rooms over it, the back room to the east
// with the room for rent over it and its balcony over the common room. Two flights of stairs.
void Game::BuildInnMeshes()
    {
    R3D& r = *iR;
    Mesh* im = iMIn;
    StaticDef d;
    int i;
    // ---- ground floor: floors, outer walls (two storeys), the bar's low ceiling, the doors, the inner walls
    {
    Mesh& m = im[IN_INNSHELL];
    r.MeshInit(m, 330, 500, 1);
    BoxC(r, m, 0, 0, -20, 500, 1020, 0, 1240);                // common room
    BoxC(r, m, 0, 500, -20, 0, 1020, 0, 500);                 // bar
    BoxC(r, m, 0, 1020, -20, 500, 1740, 0, 1240);             // back room
    BoxC(r, m, 0, 500, -20, 1240, 1020, 0, 2400);             // kitchen and pantry
    BoxC(r, m, 1, -30, 0, 500, 0, 640, 1240);                 // west (the front door's wall)
    BoxC(r, m, 1, -30, 0, 1240, 500, 640, 1270);
    BoxC(r, m, 1, -30, 0, 470, 500, 640, 500);
    BoxC(r, m, 1, 470, 0, -30, 500, 400, 500);                // the bar's walls
    BoxC(r, m, 1, 1020, 0, -30, 1050, 400, 500);
    BoxC(r, m, 1, 470, 0, -30, 1050, 400, 0);
    BoxC(r, m, 1, 500, 400, 470, 1020, 640, 500);             // over the bar's opening
    BoxC(r, m, 2, 500, 400, 0, 1020, 420, 470);               // the bar's ceiling
    BoxC(r, m, 1, 1020, 0, 470, 1770, 640, 500);              // back room: south, north, east
    BoxC(r, m, 1, 1020, 0, 1240, 1770, 640, 1270);
    BoxC(r, m, 1, 1740, 0, 470, 1770, 640, 1270);
    BoxC(r, m, 1, 470, 0, 1240, 500, 640, 2430);              // kitchen arm: west, east, north
    BoxC(r, m, 1, 1020, 0, 1240, 1050, 640, 2430);
    BoxC(r, m, 1, 470, 0, 2400, 1050, 640, 2430);
    BoxC(r, m, 4, 0, 0, 800, 6, 230, 920);                    // the front door
    BoxC(r, m, 3, 0, 0, 782, 14, 248, 800); BoxC(r, m, 3, 0, 0, 920, 14, 248, 938); BoxC(r, m, 3, 0, 230, 782, 14, 250, 938);
    BoxC(r, m, 4, 500, 0, 1800, 506, 225, 1920);              // the back door (kitchen)
    BoxC(r, m, 3, 500, 0, 1782, 514, 243, 1800); BoxC(r, m, 3, 500, 0, 1920, 514, 243, 1938);
    BoxC(r, m, 1, 1010, 0, 500, 1030, 320, 760);              // common room / back room, a doorway
    BoxC(r, m, 1, 1010, 0, 880, 1030, 320, 1240);
    BoxC(r, m, 1, 1010, 230, 760, 1030, 320, 880);
    BoxC(r, m, 1, 630, 0, 1240, 1020, 320, 1260);             // common room / kitchen, a doorway
    BoxC(r, m, 1, 500, 230, 1240, 630, 320, 1260);
    BoxC(r, m, 2, -30, 640, 470, 1050, 660, 1270);            // the common room's roof, its beams
    for (i = 0; i < 3; i++) BoxC(r, m, 3, -30, 590, 650 + i * 230, 1050, 640, 680 + i * 230);
    BoxC(r, m, 3, 0, 0, 500, 1020, 14, 510);                  // dark skirting where the bar opens
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 0, 0);
    PLANK(d, 0); TIMBER(d, 1); Col(d, 2, 90, 70, 50, SL_TEX | TXI_PLANKS); DARKW(d, 3); Col(d, 4, 86, 60, 38, SL_TEX | TXI_PLANKS);
    iInType[IN_INNSHELL] = r.RegisterStatic(d);
    }
    // ---- upper floor: the floors over the back room and the kitchen, the balcony, roofs, the upstairs walls
    {
    Mesh& m = im[IN_INNUPPER];
    r.MeshInit(m, 300, 450, 1);
    BoxC(r, m, 0, 1020, 300, 500, 1740, 320, 1240);           // over the back room
    BoxC(r, m, 0, 880, 300, 800, 1020, 320, 1060);            // the balcony
    BoxC(r, m, 0, 500, 300, 1260, 890, 320, 2400);            // over the kitchen, round the stairwell
    BoxC(r, m, 0, 890, 300, 1260, 1020, 320, 1570);
    BoxC(r, m, 0, 890, 300, 1920, 1020, 320, 2400);
    BoxC(r, m, 2, 1020, 640, 470, 1770, 660, 1270);           // roofs
    BoxC(r, m, 2, 470, 640, 1270, 1050, 660, 2430);
    BoxC(r, m, 1, 1010, 320, 500, 1030, 640, 870);            // the rented room's west wall, the balcony doorway
    BoxC(r, m, 1, 1010, 320, 1000, 1030, 640, 1090);
    BoxC(r, m, 1, 1010, 540, 870, 1030, 640, 1000);
    BoxC(r, m, 1, 1020, 320, 1090, 1040, 640, 1100);          // the landing: its south wall with the door, its east wall
    BoxC(r, m, 1, 1160, 320, 1090, 1220, 640, 1100);
    BoxC(r, m, 1, 1040, 540, 1090, 1160, 640, 1100);
    BoxC(r, m, 1, 1200, 320, 1100, 1220, 640, 1240);
    BoxC(r, m, 3, 870, 400, 800, 880, 410, 1060);             // the balcony's railing
    BoxC(r, m, 3, 870, 400, 1055, 1020, 410, 1065);
    BoxC(r, m, 3, 870, 400, 795, 1020, 410, 805);
    for (i = 0; i < 4; i++) BoxC(r, m, 3, 868, 320, 800 + i * 84, 880, 400, 812 + i * 84);
    BoxC(r, m, 3, 860, 270, 800, 1020, 300, 1070);            // the beam carrying it
    BoxC(r, m, 1, 500, 320, 1240, 1020, 640, 1260);           // the kitchen arm's upper storey: its wall to the hall
    BoxC(r, m, 1, 880, 320, 1260, 890, 640, 1900);            // the innkeeper's room (south)
    BoxC(r, m, 1, 890, 320, 1260, 1020, 640, 1560);
    BoxC(r, m, 1, 500, 320, 1900, 540, 640, 1910);
    BoxC(r, m, 1, 660, 320, 1900, 890, 640, 1910);
    BoxC(r, m, 1, 540, 540, 1900, 660, 640, 1910);
    BoxC(r, m, 1, 500, 320, 2060, 700, 640, 2070);            // the serving maid's room (north)
    BoxC(r, m, 1, 820, 320, 2060, 1020, 640, 2070);
    BoxC(r, m, 1, 700, 540, 2060, 820, 640, 2070);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 0, 0);
    PLANK(d, 0); TIMBER(d, 1); Col(d, 2, 90, 70, 50, SL_TEX | TXI_PLANKS); DARKW(d, 3);
    iInType[IN_INNUPPER] = r.RegisterStatic(d);
    }
    }

void Game::BuildInnInterior()
    {
    int c = CELL_INN, U = 320;
    IPlace(c, IN_INNSHELL, 0, 0, 0, 0, 256);
    IPlace(c, IN_INNUPPER, 0, 0, 0, 0, 256);
    IPlace(c, IN_SHOPSTAIRS, 655, 0, 1185, 256, 256);         // up to the room for rent (east along the north wall)
    IPlace(c, IN_SHOPSTAIRS, 955, 0, 1570, 0, 256);           // up from the kitchen (north along its east wall)
    // the common room: the fire pit in the middle, the warmth of the whole house
    IPlace(c, IN_FIREPIT, 760, 0, 870, 256, 300);
    IPlace(c, IN_FLAME, 760, 22, 870, 0, 640);
    ILight(c, 760, 90, 870, 1300, 220);
    IPlace(c, IN_TABLE, 170, 0, 620, 0, 256);                 // tables by the front door
    IPlace(c, IN_TABLE, 170, 0, 1120, 0, 256);
    IPlace(c, IN_TABLE, 330, 0, 1160, 0, 256);                // against the north wall
    IPlace(c, IN_TABLE, 330, 0, 580, 0, 256);                 // against the south wall
    IPlace(c, IN_TABLE, 930, 0, 580, 0, 230);                 // the warrior-woman's corner
    IPlace(c, IN_LONGTABLE, 985, 0, 1000, 768, 200);          // narrow table on the east wall
    IPlace(c, IN_SACKS, 760, 0, 1210, 0, 200);                // under the stairs
    IPlace(c, IN_RUG, 380, 0, 870, 256, 320);
    WallTorch(c, 2, 700, 256);
    WallTorch(c, 2, 1040, 256);
    WallTorch(c, 400, 1238, 512);
    WallTorch(c, 400, 502, 0);
    // the bar: the counter, the cupboards behind it, game and herbs hanging from the beam
    IPlace(c, IN_COUNTER, 760, 0, 230, 256, 256);
    IPlace(c, IN_SHELF, 620, 0, 18, 0, 256);
    IPlace(c, IN_SHELF, 900, 0, 18, 0, 256);
    IPlace(c, IN_HANG, 700, 70, 110, 0, 256);
    IPlace(c, IN_HERBS, 860, 360, 110, 0, 256);
    IPlace(c, IN_CANDLE, 640, 100, 230, 0, 256); IPlace(c, IN_FLAME, 640, 118, 230, 0, 60);
    ILight(c, 700, 150, 200, 600, 120);
    // the back room (east): a bed in the south-east corner, an end table, a low table and chair, a wardrobe
    IPlace(c, IN_ENDTABLE, 1530, 0, 560, 0, 256);
    IPlace(c, IN_TABLE, 1600, 0, 1130, 0, 230);
    IPlace(c, IN_RUG, 1350, 0, 850, 0, 260);
    WallTorch(c, 1738, 900, 768);
    // the kitchen: the fire with its spit on the west wall, a table in the north-west, shelves, the ledger table
    IPlace(c, IN_FIREPIT, 600, 0, 1620, 256, 256);
    IPlace(c, IN_FLAME, 600, 22, 1620, 0, 520);
    ILight(c, 600, 80, 1620, 900, 190);
    IPlace(c, IN_TABLE, 620, 0, 2300, 0, 256);
    IPlace(c, IN_SHELF, 560, 0, 1262, 0, 256);
    IPlace(c, IN_SHELF, 520, 0, 2300, 256, 256);
    IPlace(c, IN_TABLE, 930, 0, 1330, 0, 230);
    IPlace(c, IN_HANG, 760, 30, 1450, 256, 256);
    IPlace(c, IN_SACKS, 940, 0, 2320, 0, 220);
    WallTorch(c, 1018, 2200, 768);
    // upstairs: the room for rent (a double bed on the west wall, end tables, a dresser), its balcony and landing
    IPlace(c, IN_ENDTABLE, 1055, U, 790, 0, 256);
    IPlace(c, IN_ENDTABLE, 1055, U, 530, 0, 256);
    IPlace(c, IN_FLAME, 1055, U + 72, 790, 0, 50);
    ILight(c, 1055, U + 76, 790, 450, 90);
    IPlace(c, IN_FURRUG, 1400, U, 800, 256, 240);
    IPlace(c, IN_TABLE, 945, U, 1022, 0, 130);                // a low table on the balcony
    // the innkeeper's room (south of the landing) and the serving maid's (north)
    IPlace(c, IN_ENDTABLE, 700, U, 1300, 0, 256);
    IPlace(c, IN_FLAME, 700, U + 72, 1300, 0, 50);
    ILight(c, 700, U + 76, 1300, 450, 90);
    IPlace(c, IN_LONGTABLE, 620, U, 1300, 0, 160);
    IPlace(c, IN_ENDTABLE, 760, U, 2340, 0, 256);
    IPlace(c, IN_FLAME, 760, U + 72, 2340, 0, 50);
    ILight(c, 760, U + 76, 2340, 450, 90);
    IPlace(c, IN_TABLE, 580, U, 2150, 0, 200);
    }

// ================================================================== Highhall (after Dragonsreach), the main hall level
// Generated from the reference plan (docs/maps/gen_highhall.py -> highhall.inc): the entrance hall with its pillars and
// benches, the great stairs between stone ledges with braziers, the great hall (the long fire, two feast tables, carpets,
// pillars carrying galleries and great arched ribs), the dais and the throne, the west stairs up to the galleries, the
// balcony behind the throne and the war room; the kitchen to the west with the stairs down to the storeroom; the court
// wizard's workshop, study and bedroom to the east.
static void BoxSlab(R3D& r, Mesh& m, int slot, int x0, int z0, int x1, int z1, int y)
    {
    BoxC(r, m, slot, x0, y - 20, z0, x1, y, z1);
    }
void Game::BuildHighhallMeshes()
    {
    R3D& r = *iR;
    Mesh* im = iMIn;
    StaticDef d;
    int i, k;
    // ---- the shell: walls and floors in two meshes (each holds 60 boxes or so)
    for (k = 0; k < 2; k++)
        {
        Mesh& m = im[k ? IN_HHSHELL2 : IN_HHSHELL1];
        r.MeshInit(m, 500, 750, 1);
        int half = (KHHBox_N + KHHSlab_N) / 2;
        for (i = k ? half : 0; i < (k ? KHHBox_N + KHHSlab_N : half); i++)
            {
            if (i < KHHBox_N) { const int* b = KHHBox + i * 7; BoxC(r, m, b[0], b[1], b[2], b[3], b[4], b[5], b[6]); }
            else { const int* s = KHHSlab + (i - KHHBox_N) * 6; BoxSlab(r, m, s[0], s[1], s[2], s[3], s[4], s[5]); }
            }
        r.MeshFinish(m);
        r.MeshPlanarUV(m, 2);
        Def0(d, &m, 0, 0, 0, 0);
        PLANK(d, 0); Col(d, 1, 104, 80, 56, SL_TEX | TXI_PLANKS); Col(d, 2, 112, 110, 106, SL_TEX | TXI_BLOCK); DARKW(d, 3);
        Col(d, 4, 120, 92, 62, SL_TEX | TXI_PLANKS); Col(d, 5, 104, 100, 94, SL_TEX | TXI_SLATE);
        iInType[k ? IN_HHSHELL2 : IN_HHSHELL1] = r.RegisterStatic(d);
        }
    // ---- the steps of every flight (each step solid down to the floor below it)
    {
    Mesh& m = im[IN_HHSTEPS];
    r.MeshInit(m, 480, 720, 1);
    for (i = 0; i < KHHRamp_N; i++)
        {
        const int* q = KHHRamp + i * 7;
        int len = q[3] - q[1], rise = q[6] - q[5], n = Abs(rise) / 30 + 1, s;
        if (n > 20) n = 20;
        int base = q[5] < q[6] ? q[5] : q[6];
        for (s = 0; s < n; s++)
            {
            int za = q[1] + len * s / n, zb = q[1] + len * (s + 1) / n;
            int top = q[5] + rise * (s + 1) / n;
            if (rise < 0) top = q[5] + rise * s / n;
            BoxC(r, m, 0, q[0], base - 20, za, q[2], top, zb);
            }
        }
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 124, 96, 66, SL_TEX | TXI_PLANKS);
    iInType[IN_HHSTEPS] = r.RegisterStatic(d);
    }
    // ---- ceilings and roofs, the great arched ribs over the hall at each pair of pillars, beams along the galleries
    {
    Mesh& m = im[IN_HHROOF];
    r.MeshInit(m, 480, 720, 1);
    BoxC(r, m, 0, -1250, 900, 0, 1290, 920, 1932);            // over the entrance hall
    BoxC(r, m, 0, -1660, 1400, 1932, 1680, 1420, 5110);        // the great hall's roof
    BoxC(r, m, 0, -3170, 900, 2720, -1636, 920, 4280);        // kitchen, wizard, storeroom, war room
    BoxC(r, m, 0, 1659, 900, 2720, 3190, 920, 4280);
    BoxC(r, m, 0, -3550, 600, 4273, -2310, 620, 5480);
    BoxC(r, m, 0, -830, 1200, 5091, 830, 1220, 5800);
    BoxC(r, m, 1, -800 - 18, 980, 1932, -800 + 18, 1010, 5091);    // beams along the tops of the pillar rows
    BoxC(r, m, 1, 800 - 18, 980, 1932, 800 + 18, 1010, 5091);
    BoxC(r, m, 1, -800 - 14, 670, 1932, -800 + 14, 700, 4909);     // the galleries' edges
    BoxC(r, m, 1, 800 - 14, 670, 1932, 800 + 14, 700, 4909);
    static const int KRibZ[5] = { 2068, 2705, 3386, 4114, 4900 };
    for (k = 0; k < 5; k++)
        {
        int z = KRibZ[k], s;
        for (s = 0; s < 4; s++)                                   // a pointed arch from each pillar top to the ridge
            {
            int a0 = s * 256 / 4, a1 = (s + 1) * 256 / 4;
            int x0 = 800 - ((800 * r3d_sin(a0)) >> 14), y0 = 1000 + ((380 * (16384 - r3d_cos(a0))) >> 14);
            int x1 = 800 - ((800 * r3d_sin(a1)) >> 14), y1 = 1000 + ((380 * (16384 - r3d_cos(a1))) >> 14);
            Rod(r, m, 1, x0, y0, z, x1, y1, z, 16, 3);
            Rod(r, m, 1, -x0, y0, z, -x1, y1, z, 16, 3);
            }
        BoxC(r, m, 1, -800, 1240, z - 12, 800, 1262, z + 12);    // the tie beam across
        }
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 84, 66, 48, SL_TEX | TXI_PLANKS); Col(d, 1, 120, 96, 68, SL_TEX | TXI_LOGS);
    iInType[IN_HHROOF] = r.RegisterStatic(d);
    }
    // ---- a pillar of the hall: a carved base, a tall round shaft, a carved collar at the top (700 cm)
    {
    Mesh& m = im[IN_HHPILLAR];
    r.MeshInit(m, 60, 90, 1);
    r.MeshLimb(m, 0, 1, 0, 0, 0, 0, Cm(140), 0, Cm(54), Cm(54), Cm(50), Cm(50), 8, 0);
    r.MeshLimb(m, 0, 0, 0, Cm(140), 0, 0, Cm(670), 0, Cm(40), Cm(40), Cm(38), Cm(38), 8, 0);
    r.MeshLimb(m, 0, 1, 0, Cm(640), 0, 0, Cm(700), 0, Cm(48), Cm(48), Cm(52), Cm(52), 8, 2);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 0, 0, 54, 700);
    Col(d, 0, 150, 126, 92, SL_TEX | TXI_LOGS); Col(d, 1, 132, 112, 84, SL_TEX | TXI_FURN);
    iInType[IN_HHPILLAR] = r.RegisterStatic(d);
    }
    // ---- a long pale carpet with a darker border (300 x 600 cm at scale 256)
    {
    Mesh& m = im[IN_CARPET];
    r.MeshInit(m, 48, 72, 1);
    BoxC(r, m, 0, -150, 0, -300, 150, 1, 300);
    BoxC(r, m, 1, -150, 0, -300, -132, 2, 300); BoxC(r, m, 1, 132, 0, -300, 150, 2, 300);
    BoxC(r, m, 1, -132, 0, -300, 132, 2, -282); BoxC(r, m, 1, -132, 0, 282, 132, 2, 300);
    r.MeshFinish(m);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 190, 170, 130, 6); Col(d, 1, 140, 104, 70, 6);
    iInType[IN_CARPET] = r.RegisterStatic(d);
    }
    // ---- a feast table (840 cm along z) with its runner cloth, candlesticks, jugs and goblets
    {
    Mesh& m = im[IN_FEAST];
    r.MeshInit(m, 200, 300, 1);
    BoxC(r, m, 0, -55, 72, -420, 55, 80, 420);
    for (i = 0; i < 4; i++) BoxC(r, m, 1, -45, 0, -380 + i * 253 - 8, 45, 72, -380 + i * 253 + 8);   // trestles
    BoxC(r, m, 2, -30, 80, -420, 30, 81, 420);               // the runner
    for (i = 0; i < 4; i++)
        {
        int z = -330 + i * 220;
        r.MeshLimb(m, 0, 3, 0, Cm(80), Cm(z), 0, Cm(110), Cm(z), Cm(3), Cm(3), Cm(3), Cm(3), 4, 2);          // candlestick
        r.MeshLimb(m, 0, 4, 0, Cm(110), Cm(z), 0, Cm(122), Cm(z), Cm(3), Cm(3), Cm(3), Cm(3), 4, 2);         // candle
        r.MeshLimb(m, 0, 3, Cm(-36), Cm(80), Cm(z + 60), Cm(-36), Cm(96), Cm(z + 60), Cm(3), Cm(3), Cm(5), Cm(5), 5, 1);   // goblets
        r.MeshLimb(m, 0, 3, Cm(36), Cm(80), Cm(z + 100), Cm(36), Cm(96), Cm(z + 100), Cm(3), Cm(3), Cm(5), Cm(5), 5, 1);
        }
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 55, 420, 0, 80);
    Col(d, 0, 112, 78, 50, SL_TEX | TXI_PLANKS); DARKW(d, 1); Col(d, 2, 200, 190, 160, 6); Col(d, 3, 170, 150, 100, 1); Col(d, 4, 232, 222, 196, 1);
    iInType[IN_FEAST] = r.RegisterStatic(d);
    }
    // ---- the throne: a broad seat, armrests, a tall carved back with a crest (facing +z)
    {
    Mesh& m = im[IN_THRONE];
    r.MeshInit(m, 80, 120, 1);
    BoxC(r, m, 0, -55, 0, -40, 55, 50, 40);
    BoxC(r, m, 1, -48, 50, -34, 48, 58, 34);                  // the cushion
    BoxC(r, m, 0, -68, 0, -40, -55, 95, 40); BoxC(r, m, 0, 55, 0, -40, 68, 95, 40);   // armrests
    BoxC(r, m, 0, -62, 50, -52, 62, 240, -38);                // the back
    BoxC(r, m, 2, -40, 120, -40, 40, 220, -37);               // its carved panel
    r.MeshLimb(m, 0, 2, 0, Cm(240), Cm(-45), 0, Cm(290), Cm(-45), Cm(40), Cm(6), Cm(6), Cm(6), 4, 3);   // crest
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 68, 52, 0, 60);
    Col(d, 0, 92, 60, 38, SL_TEX | TXI_FURN); Col(d, 1, 120, 40, 34, 6); Col(d, 2, 170, 140, 80, 1);
    iInType[IN_THRONE] = r.RegisterStatic(d);
    }
    // ---- a tall bookcase full of books (back to the wall, facing +z)
    {
    Mesh& m = im[IN_BOOKCASE];
    r.MeshInit(m, 120, 180, 1);
    BoxC(r, m, 0, -60, 0, -22, 60, 220, -18);
    BoxC(r, m, 0, -62, 0, -22, -56, 220, 20); BoxC(r, m, 0, 56, 0, -22, 62, 220, 20);
    for (i = 0; i < 4; i++)
        {
        int y = 10 + i * 52;
        BoxC(r, m, 0, -56, y, -18, 56, y + 5, 20);
        BoxC(r, m, 1 + (i % 3), -52, y + 5, -16, -8, y + 34, 14);
        BoxC(r, m, 1 + ((i + 1) % 3), -6, y + 5, -16, 50, y + 30, 14);
        }
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 62, 22, 0, 220);
    Col(d, 0, 96, 66, 42, SL_TEX | TXI_FURN); Col(d, 1, 110, 40, 34, 6); Col(d, 2, 60, 80, 54, 6); Col(d, 3, 120, 100, 60, 6);
    iInType[IN_BOOKCASE] = r.RegisterStatic(d);
    }
    // ---- a banner of the hold: pale cloth with a golden emblem, hung from a rod (facing +z), 250 cm long
    {
    Mesh& m = im[IN_BANNER2];
    r.MeshInit(m, 60, 90, 1);
    BoxC(r, m, 0, -55, -250, -2, 55, 0, 2);
    r.MeshLimb(m, 0, 1, 0, Cm(-90), Cm(2), 0, Cm(-90), Cm(5), Cm(34), Cm(34), Cm(34), Cm(34), 8, 3);
    Rod(r, m, 2, -64, 0, 0, 64, 0, 0, 3, 4);
    r.MeshFinish(m);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 222, 212, 180, 6); Col(d, 1, 196, 150, 70, 1); DARKW(d, 2);
    iInType[IN_BANNER2] = r.RegisterStatic(d);
    }
    // ---- a chandelier: an iron ring with candles, hanging on a chain (ring at y 0, chain to y 400)
    {
    Mesh& m = im[IN_CHANDELIER];
    r.MeshInit(m, 140, 210, 1);
    for (i = 0; i < 8; i++)
        {
        int a0 = i * 128, a1 = a0 + 128;
        int x0 = (r3d_sin(a0) * 110) >> 14, z0 = (r3d_cos(a0) * 110) >> 14, x1 = (r3d_sin(a1) * 110) >> 14, z1 = (r3d_cos(a1) * 110) >> 14;
        Rod(r, m, 0, x0, 0, z0, x1, 0, z1, 3, 3);
        r.MeshLimb(m, 0, 1, Cm(x0), Cm(2), Cm(z0), Cm(x0), Cm(16), Cm(z0), Cm(3), Cm(3), Cm(3), Cm(3), 3, 2);
        }
    Rod(r, m, 0, 0, 0, 0, 0, 400, 0, 2, 3);
    for (i = 0; i < 4; i++) { int a = i * 256; Rod(r, m, 0, (r3d_sin(a) * 110) >> 14, 0, (r3d_cos(a) * 110) >> 14, 0, 90, 0, 2, 3); }
    r.MeshFinish(m);
    Def0(d, &m, 0, 0, 0, 0);
    Col(d, 0, 40, 38, 36, 1); Col(d, 1, 236, 226, 200, 1);
    iInType[IN_CHANDELIER] = r.RegisterStatic(d);
    }
    // ---- the arcane enchanter: a dark stone table, an orb glowing violet on it, a skull, candles
    {
    Mesh& m = im[IN_ENCHANTER];
    r.MeshInit(m, 80, 120, 1);
    BoxC(r, m, 0, -70, 0, -45, 70, 85, 45);
    BoxC(r, m, 0, -78, 85, -52, 78, 92, 52);
    r.MeshEllipsoid(m, 0, 1, 0, Cm(110), 0, Cm(16), Cm(16), Cm(16), 6, 3);
    r.MeshEllipsoid(m, 0, 2, Cm(45), Cm(100), Cm(-20), Cm(9), Cm(9), Cm(10), 5, 2);
    r.MeshLimb(m, 0, 2, Cm(-50), Cm(92), Cm(20), Cm(-50), Cm(112), Cm(20), Cm(3), Cm(3), Cm(3), Cm(3), 4, 2);
    r.MeshLimb(m, 0, 2, Cm(-60), Cm(92), Cm(-25), Cm(-60), Cm(106), Cm(-25), Cm(3), Cm(3), Cm(3), Cm(3), 4, 2);
    r.MeshFinish(m);
    Def0(d, &m, 78, 52, 0, 92);
    Col(d, 0, 70, 66, 74, SL_TEX | TXI_BLOCK); Col(d, 1, 180, 110, 230, 1); Col(d, 2, 220, 210, 190, 1);
    iInType[IN_ENCHANTER] = r.RegisterStatic(d);
    }
    // ---- a big map of the land on a wall (parchment, a frame, the lines of roads and rivers), facing +z, 300 x 200
    {
    Mesh& m = im[IN_WALLMAP];
    r.MeshInit(m, 80, 120, 1);
    BoxC(r, m, 0, -160, 90, -4, 160, 300, 0);
    BoxC(r, m, 1, -150, 100, 0, 150, 290, 2);
    BoxC(r, m, 2, -110, 180, 2, 60, 184, 3); BoxC(r, m, 2, -20, 120, 2, -16, 270, 3); BoxC(r, m, 3, 40, 130, 2, 130, 134, 3);
    BoxC(r, m, 3, 60, 134, 2, 64, 250, 3);
    r.MeshFinish(m);
    Def0(d, &m, 0, 0, 0, 0);
    DARKW(d, 0); Col(d, 1, 210, 190, 140, 6); Col(d, 2, 120, 90, 60, 1); Col(d, 3, 80, 110, 150, 1);
    iInType[IN_WALLMAP] = r.RegisterStatic(d);
    }
    // ---- the war room's map table: a great table, the map spread on it, flags of red and blue
    {
    Mesh& m = im[IN_MAPTABLE];
    r.MeshInit(m, 120, 180, 1);
    BoxC(r, m, 0, -200, 82, -100, 200, 90, 100);
    for (i = 0; i < 4; i++) BoxC(r, m, 0, i & 1 ? 170 : -190, 0, i & 2 ? 70 : -90, i & 1 ? 190 : -170, 82, i & 2 ? 90 : -70);
    BoxC(r, m, 1, -180, 90, -85, 180, 91, 85);
    for (i = 0; i < 6; i++)
        {
        int x = -140 + i * 55, z = (i & 1) ? 40 : -30;
        r.MeshLimb(m, 0, 2 + (i % 2), Cm(x), Cm(91), Cm(z), Cm(x), Cm(112), Cm(z), Cm(5), Cm(5), Cm(1), Cm(1), 4, 1);
        }
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def0(d, &m, 200, 100, 0, 92);
    Col(d, 0, 100, 70, 46, SL_TEX | TXI_PLANKS); Col(d, 1, 208, 190, 142, 6); Col(d, 2, 190, 50, 40, 1); Col(d, 3, 50, 80, 180, 1);
    iInType[IN_MAPTABLE] = r.RegisterStatic(d);
    }
    }

void Game::BuildHighhall()
    {
    int c = CELL_HIGHHALL, i;
    IPlace(c, IN_HHSHELL1, 0, 0, 0, 0, 256);
    IPlace(c, IN_HHSHELL2, 0, 0, 0, 0, 256);
    IPlace(c, IN_HHSTEPS, 0, 0, 0, 0, 256);
    IPlace(c, IN_HHROOF, 0, 0, 0, 0, 256);
    for (i = 0; i < KHHPillar_N; i++)
        {
        const int* p = KHHPillar + i * 3;
        IPlace(c, IN_HHPILLAR, p[0], p[2], p[1], 0, p[2] ? 256 : 330);
        }
    // the galleries' railings (the carved zigzag), along both inner edges and the balcony over the throne
    for (i = 0; i < 3; i++) { IPlace(c, IN_RAILING, -790, 700, 1932 + i * 900, 0, 256); IPlace(c, IN_RAILING, 790, 700, 1932 + i * 900, 0, 256); }
    IPlace(c, IN_RAILING, -773, 700, 4915, 256, 256); IPlace(c, IN_RAILING, -127, 700, 4915, 256, 256);
    static const int KWhat[17] = { IN_CARPET, IN_FEAST, IN_FIREPIT2, IN_BRAZIER, IN_THRONE, IN_TABLE, IN_BOOKCASE, IN_BANNER2,
                                   IN_CHANDELIER, IN_ENCHANTER, IN_WALLMAP, IN_MAPTABLE, IN_FIREPLACE, IN_SACKS, IN_HANG, IN_SHELF, IN_RUG2 };
    for (i = 0; i < KHHPut_N; i++)
        {
        const int* p = KHHPut + i * 6;
        int w = p[0], x = p[1], y = p[2], z = p[3];
        switch (w)
            {
            case 2:
                IPlace(c, IN_FIREPIT2, x, y, z, 256, p[5]);
                IPlace(c, IN_FLAME, x, y + 6, z, 0, 700);
                ILight(c, x, y + 120, z, 1500, 230);
                break;
            case 3:
                IPlace(c, IN_BRAZIER, x, y, z, 0, 256);
                IPlace(c, IN_FLAME, x, y + 96, z, 0, 300);
                ILight(c, x, y + 140, z, 800, 160);
                break;
            case 7: IPlace(c, IN_BANNER2, x, y + 620, z, p[4], 256); break;
            case 8:
                IPlace(c, IN_CHANDELIER, x, y + 520, z, 0, 256);
                ILight(c, x, y + 520, z, 1100, 120);
                break;
            case 12:
                IPlace(c, IN_FIREPLACE, x, y, z, p[4], 256);
                IPlace(c, IN_FIREPIT, x, y, z - 90, 256, 256);
                IPlace(c, IN_FLAME, x, y + 22, z - 90, 0, 520);
                ILight(c, x, y + 80, z - 100, 1000, 200);
                break;
            case 14: IPlace(c, IN_HANG, x, y + 150, z, p[4], 256); break;
            default: IPlace(c, KWhat[w], x, y, z, p[4], p[5]); break;
            }
        }
    // candles in the kitchen, the wizard's rooms, the war room
    ILight(c, -2300, 400, 3100, 700, 110);
    ILight(c, 2400, 400, 3600, 800, 130);
    ILight(c, 2950, 400, 3400, 500, 90);
    ILight(c, 0, 820, 5400, 800, 130);
    ILight(c, -2900, 200, 4800, 700, 110);
    WallTorch(c, -1225, 700, 256);
    WallTorch(c, 1270, 700, 768);
    }

const ItemDef& GameItem(int aId);
// Highhall's things you can use (chairs, benches, barrels, the alchemy lab, beds, chests) and its people
void Game::SpawnHighhall()
    {
    int i;
    for (i = 0; i < KHHProp_N; i++)
        {
        const HHProp& p = KHHProp[i];
        int item = p.item, flags = 0;
        if (p.type == PT_BARREL || p.type == PT_CHEST || p.type == PT_CUPBOARD || p.type == PT_SACK || p.type == PT_CRATE || p.type == PT_CASE)
            {
            int cn = NewCont();
            switch (p.type)
                {
                case PT_BARREL: ContAdd(cn, IT_APPLE, 2); ContAdd(cn, IT_CABBAGE, 2); break;
                case PT_SACK: ContAdd(cn, IT_POTATO, 3); break;
                case PT_CRATE: ContAdd(cn, IT_SALT, 2); ContAdd(cn, IT_LEEK, 1); break;
                case PT_CASE: ContAdd(cn, IT_IRON_DAGGER, 1); break;
                case PT_CUPBOARD: ContAdd(cn, IT_FUR_BOOTS, 1); ContAdd(cn, IT_JUG, 1); break;
                default: ContAdd(cn, IT_POT_MHEAL, 1); ContAdd(cn, IT_GARNET, 1); break;
                }
            item = cn; flags = PF_OWNED;
            }
        if (p.type == PT_BED || p.type == PT_ALCHEMY) flags = PF_OWNED;
        int yaw = (p.type == PT_CHAIR || p.type == PT_BENCH) ? (p.yaw + 512) & 1023 : p.yaw;
        AddProp(p.type, CELL_HIGHHALL, Ix(p.x), IN_FLOOR + Cm(p.y), Iz(p.z), yaw, item, 0, flags);
        }
    // food and drink on the feast tables
    static const int KFeast[] = { -441, 3300, IT_BREAD,  -441, 3600, IT_CHEESE,  -441, 3850, IT_VENISON,  500, 3250, IT_APPLE,  500, 3500, IT_CREAM_MEAD,  500, 3800, IT_SALMON_STEAK, -1 };
    for (i = 0; KFeast[i * 3] != -1; i++)
        AddProp(PT_ITEM, CELL_HIGHHALL, Ix(KFeast[i * 3]), IN_FLOOR + Cm(380) + iItemRest[GameItem(KFeast[i * 3 + 2]).mesh * 3 + 1], Iz(KFeast[i * 3 + 1]), i * 150, KFeast[i * 3 + 2], 1, PF_OWNED);
    // the people: the Jarl by his throne, his housecarl, guards, the court wizard, servants
    for (i = 0; i < KHHPeople_N; i++)
        {
        const int* q = KHHPeople + i * 5;
        int idx = SpawnEnt(E_VILLAGER, Ix(q[0]), Iz(q[1]), q[3]);
        if (idx < 0) continue;
        Ent& e = iS.e[idx];
        e.cell = CELL_HIGHHALL; e.y = IN_FLOOR + Cm(q[2]);
        int role = q[4];
        e.role = ROLE_NAMED; e.pad1 = (u8)(5 + (role == 4 ? (i & 1 ? 4 : 5) : role)); e.pad2 = role == 4 ? 1 : 0;
        e.var = (u8)(role == 1 || role == 4 ? 3 : (role == 3 ? 2 : 0));
        }
    }

// ================================================================== the Riverwood Trader (RiverwoodRiverwoodTrader)
// Everything below is placed from the cell's own references in Skyrim.esm (tools/skyrim_places.py -> refs_interiors.csv),
// written in Skyrim units and turned into cm here (x 1.43); Skyrim rotations in degrees, clockwise from north.
static int U(int aUnits) { return aUnits * 143 / 100; }
static int SY(int aDeg) { return (aDeg * 1024 / 360) & 1023; }
// the shop is built from Skyrim's own pieces (game_kit.cpp, docs/maps/gen_kit.py); its stock, furniture and people here

void Game::SpawnRwTrader()
    {
    int c = CELL_RWTRADER;
    // (v0.4.40: its furniture, containers and goods come from Skyrim's references like every house's: game_kit.cpp,
    //  gen_kit.py - typed in here before, they missed the upstairs table)
    // the claw back on the counter once it is home (MS13GoldenClaw)
    if (iS.bfb[BF_QCLAW] >= 200)
        AddProp(PT_ITEM, c, Ix(U(-62)), IN_FLOOR + Cm(U(81)) + iItemRest[GameItem(IT_GOLDEN_CLAW).mesh * 3 + 1], Iz(U(-119)), SY(181), IT_GOLDEN_CLAW, 1, PF_OWNED);
    // Lucan Valerius: the Riverwood Trader's keeper (Imperial; his merchant chest: Skyrim's general-goods stock)
    {
    int idx = SpawnEnt(E_VILLAGER, Ix(U(32)), Iz(U(-124)), SY(205));
    if (idx >= 0)
        {
        Ent& e = iS.e[idx];
        e.cell = c; e.y = IN_FLOOR;
        e.role = ROLE_MERCHANT; e.pad1 = MER_RWTRADER; e.var = 0;
        int cn = NewCont();
        e.pad2 = (u8)cn;
        static const int KStock[] = { IT_IRON_SWORD, 1, IT_STEEL_SWORD, 1, IT_IRON_DAGGER, 1, IT_IRON_AXE, 1, IT_IRON_HELMET, 1,
                                      IT_FUR_ARMOR, 1, IT_FUR_HELMET, 1, IT_FUR_BOOTS, 1, IT_FUR_GAUNTLETS, 1, IT_HIDE_SHIELD, 1,
                                      IT_POT_MHEAL, 2, IT_POT_MMAG, 1, IT_POT_RESFIRE, 1, IT_POT_CURE, 1,
                                      IT_BREAD, 2, IT_CHEESE, 3, IT_CABBAGE, 1, IT_POTATO, 6, IT_WOOD_BOWL, 2, IT_TANKARD, 2,
                                      IT_WOLF_PELT, 2, IT_TORCH, 5, -1 };
        const int* st = KStock;
        for (; st[0] >= 0; st += 2) ContAdd(cn, st[0], st[1]);
        iS.mgold[MER_RWTRADER] = 750;
        }
    }
    // Camilla Valerius, his sister, by the counter (she keeps to the shop while the claw is gone)
    {
    int idx = SpawnEnt(E_VILLAGER, Ix(U(19)), Iz(U(-251)), SY(359));
    if (idx >= 0)
        {
        Ent& e = iS.e[idx];
        e.cell = c; e.y = IN_FLOOR;
        e.role = ROLE_NAMED; e.pad1 = 18; e.pad2 = 0; e.var = 1;
        }
    }
    }

// the Golden Claw home again: Lucan puts it back on his counter
void GameClawHome(Game& g)
    {
    g.AddProp(PT_ITEM, CELL_RWTRADER, Ix(U(-62)), IN_FLOOR + Cm(U(81)) + g.iItemRest[GameItem(IT_GOLDEN_CLAW).mesh * 3 + 1], Iz(U(-119)),
              SY(181), IT_GOLDEN_CLAW, 1, PF_OWNED);
    }

// ================================================================== the Sleeping Giant Inn (RiverwoodSleepingGiantInn)
// Built in Skyrim units (x east, y north; heights up; x 1.43 = cm); its placements come from docs/maps/gen_sginn.py.
#include "sginn_ext.inc"
// a box in Skyrim units: plan x0..x1, y0..y1, heights h0..h1
// the inn likewise built from Skyrim's pieces (game_kit.cpp); its furniture, doors, goods and people here

void Game::SpawnSgInn()
    {
    int c = CELL_RWINN, i;
    // (v0.4.40: its furniture, doors and goods come from Skyrim's references like every house's: game_kit.cpp,
    //  gen_kit.py; SgInnRentBed() marks the bed Delphine rents out once they are there)
    // the people (KSgPeople: Embry, Delphine, Orgnar)
    for (i = 0; i < KSgPeople_N; i++)
        {
        const int* q = KSgPeople + i * 4;
        int idx = SpawnEnt(E_VILLAGER, Ix(U(q[0])), Iz(U(q[1])), SY(q[3]));
        if (idx < 0) continue;
        Ent& e = iS.e[idx];
        e.cell = c; e.y = IN_FLOOR;
        if (i == 2)
            {
            e.role = ROLE_MERCHANT; e.pad1 = MER_RWINN; e.var = 2;
            int cn = NewCont();
            e.pad2 = (u8)cn;
            static const int KStock[] = { IT_BREAD, 4, IT_CHEESE, 3, IT_APPLE, 4, IT_SALMON_STEAK, 2, IT_VENISON_STEW, 2,
                                          IT_VEG_SOUP, 2, IT_CABBAGE_SOUP, 2, IT_COOKED_BEEF, 2, IT_SALT, 1, -1 };
            const int* st = KStock;
            for (; st[0] >= 0; st += 2) ContAdd(cn, st[0], st[1]);
            iS.mgold[MER_RWINN] = 500;
            }
        else { e.role = ROLE_NAMED; e.pad1 = (u8)(i == 1 ? 19 : 20); e.pad2 = 0; e.var = (u8)(i == 1 ? 1 : 0); }
        }
    }

// the bed Delphine rents out (the south-east room's, Skyrim x -700): marked once the kit's props are placed
void GameSgInnRentBed(Game& g)
    {
    int i, bx = Ix(U(-700)), best = -1, bd = 1 << 30;
    for (i = 0; i < g.iS.nprop; i++)
        {
        const Prop& p = g.iS.prop[i];
        if (p.cell != CELL_RWINN || p.type != PT_BED) continue;
        int d = p.x > bx ? p.x - bx : bx - p.x;
        if (d < bd) { bd = d; best = i; }
        }
    if (best >= 0 && bd < METER) g.iS.prop[best].count = 1;
    }

// ---------------------------------------------------------------- rooms drawn only when they can be seen
// The Sleeping Giant: 1 the south-west room, 2 the south-east room, 3 Delphine's room, 4 the passage and secret room;
// 5 (the rooms' own walls) whenever any of 1..3 is. The hall (0) is always drawn.
static int SgZoneU(int x, int y)
    {
    if (y > 700 && x > -640) return 4;
    if (y > 420 && x < -615) return 3;
    if (y < -420 && x < -630) return x < -898 ? 1 : 2;
    return 0;
    }
int GameZoneOf(int aCell, int aX, int aZ)
    {
    if (aCell != CELL_RWINN) return 0;
    return SgZoneU((aX - IN_OX) * 70 / M, (aZ - IN_OZ) * 70 / M);
    }
u32 GameZoneMask(Game& g)
    {
    if (g.iS.cell != CELL_RWINN) return 0xFFFFFFFF;
    int me = GameZoneOf(CELL_RWINN, g.iS.px, g.iS.pz);
    u32 m = 1u | (1u << me);
    if (me == 0)
        {
        // from the hall, a room shows through its open door
        int i;
        for (i = 0; i < g.iS.nprop; i++)
            {
            const Prop& p = g.iS.prop[i];
            if (p.cell != CELL_RWINN || p.type != PT_DOOR || (p.state && p.anim == 0)) continue;
            m |= 1u << GameZoneOf(CELL_RWINN, p.x, p.z + (p.z < IN_OZ ? -M : M));
            }
        }
    if (m & 0xE) m |= 1u << 5;
    return m;
    }
