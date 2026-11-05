// game_riverwood.cpp - Riverwood, laid out from Skyrim's own placements (docs/maps/gen_riverwood.py -> rw_ext.inc):
// its houses, the Sleeping Giant Inn, the mill and its wheel, the bridge, the gate walls, the fences, the forge and
// the yard things, and its people about their day.
#include "game.h"

static const int M = METER;
static inline int Abs(int v) { return v < 0 ? -v : v; }
static int Cm(int cm) { return cm * M / 100; }
#include "rw_ext.inc"
int GameCityDoorType(const Game& g);

static int RwX(int xcm) { return RW_OX * METER + Cm(xcm); }
static int RwZ(int zcm) { return RW_OZ * METER + Cm(zcm); }
// ---------------------------------------------------------------- building Riverwood
void Game::BuildRiverwood()
    {
    // everything that stands in Riverwood is Skyrim's own model now (game_kit.cpp, docs/maps/gen_kit.py): the inn, the
    // houses, the mill and its wheel, the bridge, the river's falls, the walls, fences and boardwalks, the yard things
    iOutObjMark = iR->iObjCount; iOutArea = iAreaCur;      // (v0.4.57: where the areas' outside pieces start)
    PlaceKitOutside();
    iOutObjN = iR->iObjCount - iOutObjMark;
    iR->BuildFarGroups(iOutObjMark, iOutObjMark + iOutObjN);      // (v0.4.74)
    }

// Riverwood's people out in the day: the smith at his forge, the mill's folk, the others about their houses
// (in Skyrim they sleep in their houses; Lucan, Delphine, Orgnar and Embry keep to the Trader and the inn)
// where: the forge, the mill, a house door (KRwThing / KRwHouse / KRwDoor), then a step off it (cm)
static const short KWho[8 * 6] =
    {
    //  named, at (0 thing kind, 1 house kind, 2 door of house), which, dx, dz, home (0 Alvor's, 1 Gerdur's, 2 Faendal's, 3 Sven's)
    11, 0, 5, 150, 0, 0,        // Alvor at the forge
    12, 2, 2, 150, 150, 0,      // Sigrid by their door
    13, 1, 7, 300, -300, 1,     // Hod at the mill
    14, 1, 7, 600, 200, 1,      // Gerdur at the mill
    15, 3, 10, 0, -70, 2,       // Faendal chopping wood at the block nearest the mill
    16, 2, 5, 120, 160, 3,      // Sven by his door (Hilde keeps to the house)
    21, 0, 5, 260, -140, 0,     // Dorthe watching her father at the forge
    22, 1, 7, 900, -100, 1      // Frodnar about the mill his mother wants him working at
    };
enum { NWHO = 8 };
static int RwDaySpot(int k, int* aX, int* aZ)
    {
    const short* w = KWho + k * 6;
    int x = 0, z = 0, j, found = 0;
    if (w[1] == 3)
        {
        // the chopping block (KRwThing kind 10) nearest the mill
        int mx = 0, mz = 0, best = 1 << 30;
        for (j = 0; j < KRwHouse_N; j++) if (KRwHouse[j * 9] == 7) { mx = KRwHouse[j * 9 + 1]; mz = KRwHouse[j * 9 + 2]; }
        for (j = 0; j < KRwThing_N; j++)
            {
            if (KRwThing[j * 5] != w[2]) continue;
            int dx = KRwThing[j * 5 + 1] - mx, dz = KRwThing[j * 5 + 2] - mz, d = (dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4);
            if (d < best) { best = d; x = KRwThing[j * 5 + 1]; z = KRwThing[j * 5 + 2]; found = 1; }
            }
        *aX = RwX(x + w[3]); *aZ = RwZ(z + w[4]);
        return found;
        }
    if (w[1] == 0) { for (j = 0; j < KRwThing_N && !found; j++) if (KRwThing[j * 5] == w[2]) { x = KRwThing[j * 5 + 1]; z = KRwThing[j * 5 + 2]; found = 1; } }
    else if (w[1] == 1) { for (j = 0; j < KRwHouse_N && !found; j++) if (KRwHouse[j * 9] == w[2]) { x = KRwHouse[j * 9 + 1]; z = KRwHouse[j * 9 + 2]; found = 1; } }
    else { for (j = 0; j < KRwDoor_N && !found; j++) if (KRwDoor[j * 9 + 8] == w[2] && KRwDoor[j * 9 + 2] < 100) { x = KRwDoor[j * 9 + 4]; z = KRwDoor[j * 9 + 5]; found = 1; } }
    *aX = RwX(x + w[3]); *aZ = RwZ(z + w[4]);
    return found;
    }

// Faendal at his chopping block in the day (he stands where the schedule puts him: his home spot)
int GameChopping(const Game& g, const Ent& e)
    {
    if (e.type != E_VILLAGER || e.role != ROLE_NAMED || e.pad1 != 15 || e.cell || !e.alive) return 0;
    int hour = g.iS.timeOfDay / 60000;
    return hour >= 8 && hour < 20 && Abs(e.x - e.hx) + Abs(e.z - e.hz) < M;
    }

void Game::SpawnRiverwood()
    {
    int k;
    for (k = 0; k < 6; k++)                                   // the grown-ups; the children start at home (game_kit.cpp)
        {
        int x, z;
        if (!RwDaySpot(k, &x, &z)) continue;
        int i = SpawnEnt(E_VILLAGER, x, z, (k * 300) & 1023);
        if (i < 0) continue;
        Ent& e = iS.e[i];
        e.y = FloorAt(0, x, z, e.y + 3 * M / 2);                             // on a raised deck where there is one
        e.role = ROLE_NAMED; e.pad1 = (u8)KWho[k * 6]; e.pad2 = 0;
        if (e.pad1 == 15) e.yaw = 0;
        e.var = (u8)(e.pad1 == 12 || e.pad1 == 14 ? 1 + 2 * (k & 1) : 2 * (k & 1));
        }
    }

// out from 8 in the morning to 8 at night, home the rest; at home a grown-up stands by a bed of the house (the k-th
// for the k-th of the household), a child where Skyrim places it. Nobody moves before the player's eyes: not while
// the player is in the house, nor outside within 60 m of the street spot; aForce (going through a door) moves everyone.
void Game::RiverwoodSchedule(int aForce)
    {
    static const u8 KHome[4] = { CELL_RWALVOR, CELL_RWGERDUR, CELL_RWFAENDAL, CELL_RWSVEN };
    int hour = iS.timeOfDay / 60000, day = hour >= 8 && hour < 20;
    int i, k;
    for (i = 0; i < iS.ne; i++)
        {
        Ent& e = iS.e[i];
        if (e.type != E_VILLAGER || e.role != ROLE_NAMED || !e.alive) continue;
        for (k = 0; k < NWHO; k++) if (KWho[k * 6] == e.pad1) break;
        if (k == NWHO) continue;
        int home = KHome[KWho[k * 6 + 5]];
        int want = day ? 0 : home;
        if (e.cell == want) continue;
        int x, y, z;
        if (!aForce)
            {
            if (iS.cell && (iS.cell == e.cell || iS.cell == want)) continue;
            if (!iS.cell)
                {
                if (e.cell == 0) { x = e.x; z = e.z; } else if (!RwDaySpot(k, &x, &z)) continue;
                if (Abs(x - iS.px) < 60 * M && Abs(z - iS.pz) < 60 * M) continue;
                }
            }
        if (want == 0)
            {
            if (!RwDaySpot(k, &x, &z)) continue;
            y = FloorAt(0, x, z, iR->GroundAt(x, z) + 3 * M / 2);          // on a raised deck where there is one
            }
        else
            {
            int c, n = 0, j, rank = 0;
            for (j = 0; j < k; j++) if (KWho[j * 6 + 5] == KWho[k * 6 + 5]) rank++;
            if (!GameKitPersonSpot(*this, e.pad1, &c, &x, &y, &z))
                {
                int b = -1;
                for (j = 0; j < iS.nprop; j++) if (iS.prop[j].cell == home && iS.prop[j].type == PT_BED) { if (n == rank || b < 0) b = j; n++; }
                if (b < 0) continue;
                // beside the bed, on the floor the bed stands on and clear of walls and things: its sides, its foot
                const Prop& p = iS.prop[b];
                int t, ok = 0;
                for (t = 0; t < 12 && !ok; t++)
                    {
                    int a = (p.yaw + (rank & 1 ? 768 : 256) + (t & 3) * 256) & 1023, dist = (9 + (t >> 2) * 4) * M / 10;
                    x = p.x + ((r3d_sin(a) * dist) >> 14); z = p.z + ((r3d_cos(a) * dist) >> 14);
                    y = FloorAt(home, x, z, p.y + M / 2);
                    ok = Abs(y - p.y) < M / 2 && !BlockedAt(home, x, z, 3 * M / 10, y);
                    }
                if (!ok) continue;
                }
            }
        e.cell = (u8)want; e.x = e.hx = x; e.z = e.hz = z; e.y = y; e.vx = e.vz = 0; e.state = S_IDLE;
        if (e.pad1 == 15 && !want) e.yaw = 0;                                 // Faendal: facing his chopping block
        }
    WhiterunSchedule(aForce);
    }

// ---------------------------------------------------------------- Whiterun's outskirts' people (v0.4.48)
// From Skyrim's own (tools/skyrim_area.py whiterun: npcs.csv, packages.csv): where each one works in the day - their
// package's place and radius (Skyrim units) from its hour for so many hours - and where they are the rest: in the
// house they live in (the area's file: where Skyrim places them inside), or away (Severio Pelagia sleeps in his house
// in the city, which is not built here). Bjorlam keeps to the carriage day and night (CartDriverOnCart).
//  named, x, y, z (Skyrim), radius, from hour, hours, home cell (0: away), walks about
static const int KWrWho[6 * 9] =
    {
    23, 18705, -11525, -4591, 350, 8, 12, CELL_STABLES, 0,        // Skulvar Sable-Hilt: WhiterunSkulvarStablesSellPackage
    24, 17390, -9723, -4615, 512, 8, 12, CELL_STABLES, 0,         // Jervar: WhiterunJervarStablesPackage1
    25, 20260, -15910, -4505, 512, 8, 12, 0, 1,                   // Severio Pelagia: PelagiaFarmWork8x12 (night: his house in the city)
    26, 20260, -15910, -4505, 512, 8, 12, CELL_PELAGIA, 1,        // Nimriel: PelagiaFarmWork8x12
    27, 30371, -11484, -4205, 900, 8, 12, CELL_CHILLFURROW, 1,    // Wilmuth: ChillfurrowFarmWork8x12
    28, 17590, -12412, -4745, 0, 0, 24, 0, 0                      // Bjorlam, by the carriage: CartDriverOnCart
    };
enum { NWRWHO = 6, CELL_AWAY = 250 };
// (v0.4.65) and the city's own (the open city: Whiterun's area), generated: their rows by the hour, names and looks
#include "wr_folk.inc"
const char* GameWrFolkName(int aNamed) { return aNamed >= KWrFolk0 && aNamed < KWrFolk0 + KWrFolkN ? KWrFolkName[aNamed - KWrFolk0] : 0; }
const u8* GameWrFolkLook(int aNamed) { return aNamed >= KWrFolk0 && aNamed < KWrFolk0 + KWrFolkN ? KWrFolkLook + (aNamed - KWrFolk0) * 14 : 0; }
// the k-th row (the outskirts' first, then the city's), field f
static int WrRow(int k, int f) { return k < NWRWHO ? KWrWho[k * 9 + f] : KWrFolkRow[(k - NWRWHO) * 9 + f]; }
static int WrRowN(const Game& g) { return NWRWHO + (g.iWrKit ? KWrFolkRowN : 0); }
static int WrX(int aX) { return (int)(((long long)aX + 233472) * 36576 / 10000); }
static int WrZ(int aY) { return (int)(((long long)aY + 176128) * 36576 / 10000); }
// the k-th's spot by day: within their radius, a little apart from the others there
static void WrDaySpot(int k, int* aX, int* aZ)
    {
    int a = (k * 337) & 1023, d = WrRow(k, 4) * 2 / 5;
    *aX = WrX(WrRow(k, 1) + ((r3d_sin(a) * d) >> 14)); *aZ = WrZ(WrRow(k, 2) + ((r3d_cos(a) * d) >> 14));
    }
int GameIsKitCell(const Game& g, int aCell);
void Game::SpawnWhiterun()
    {
    if (!GameIsKitCell(*this, CELL_STABLES)) return;                  // (only with the area's file: Skyrim's outskirts; v0.4.57: Whiterun's own area loaded first)
    int k;
    for (k = 0; k < NWRWHO; k++)
        {
        const int* w = KWrWho + k * 9;
        if (w[7]) continue;                                           // (who lives in a house there starts at home: SpawnKit)
        int x, z;
        WrDaySpot(k, &x, &z);
        int i = SpawnEnt(E_VILLAGER, x, z, w[0] == 28 ? (1024 - 275 * 1024 / 360 + 256) & 1023 : (k * 300) & 1023);
        if (i < 0) continue;
        Ent& e = iS.e[i];
        e.y = FloorAt(0, x, z, iR->GroundAt(x, z) + 3 * M / 2);
        e.role = ROLE_NAMED; e.pad1 = (u8)w[0]; e.pad2 = (u8)w[8]; e.var = (u8)k;
        }
    }
// (v0.4.65) the city's people: one each, away until the hour has them out (WhiterunSchedule); v0.4.67: out only
// while the player is in or near the city (game_stage.cpp, G_WRFOLK)
void Game::SpawnWrFolk()
    {
    int k;
    if (!iWrKit) return;
    for (k = NWRWHO; k < WrRowN(*this); k++)
        {
        int nm = WrRow(k, 0), j;
        for (j = 0; j < iS.ne; j++) if (iS.e[j].type == E_VILLAGER && iS.e[j].role == ROLE_NAMED && iS.e[j].pad1 == nm) break;
        if (j < iS.ne) continue;
        int x, z;
        WrDaySpot(k, &x, &z);
        int i = SpawnEnt(E_VILLAGER, x, z, (k * 300) & 1023);
        if (i < 0) continue;
        Ent& e = iS.e[i];
        e.cell = CELL_AWAY; e.y = FloorAt(0, x, z, iR->GroundAt(x, z) + 3 * M / 2);
        e.role = ROLE_NAMED; e.pad1 = (u8)nm; e.pad2 = (u8)WrRow(k, 8); e.var = (u8)k;
        }
    }
// as Riverwood's: about their work by day, home (or away) the rest, never moved before the player's eyes
void Game::WhiterunSchedule(int aForce)
    {
    int hour = iS.timeOfDay / 60000;
    int i, k;
    for (i = 0; i < iS.ne; i++)
        {
        Ent& e = iS.e[i];
        if (e.type != E_VILLAGER || e.role != ROLE_NAMED || !e.alive) continue;
        // their row for the hour (the first that has them out), else their first: home, or away
        int n = WrRowN(*this), k0 = -1;
        for (k = 0; k < n; k++)
            {
            if (WrRow(k, 0) != e.pad1) continue;
            if (k0 < 0) k0 = k;
            if (((hour - WrRow(k, 5) + 24) % 24) < WrRow(k, 6)) break;
            }
        if (k0 < 0) continue;
        int day = k < n;
        if (!day) k = k0;
        int home = WrRow(k, 7);
        int want = day ? 0 : (home ? home : CELL_AWAY);
        if (e.cell == want && (want || e.var == k)) continue;
        int x, y, z;
        if (!aForce)
            {
            if (iS.cell && (iS.cell == e.cell || iS.cell == want)) continue;
            if (!iS.cell && e.cell == 0 && Abs(e.x - iS.px) < 60 * M && Abs(e.z - iS.pz) < 60 * M) continue;
            if (!iS.cell && want == 0) { WrDaySpot(k, &x, &z); if (Abs(x - iS.px) < 60 * M && Abs(z - iS.pz) < 60 * M) continue; }
            }
        if (want == 0)
            {
            WrDaySpot(k, &x, &z);
            y = FloorAt(0, x, z, iR->GroundAt(x, z) + 3 * M / 2);
            }
        else if (want == CELL_AWAY) { x = e.x; y = e.y; z = e.z; }
        else
            {
            int c;
            if (!GameKitPersonSpot(*this, e.pad1, &c, &x, &y, &z)) continue;
            y = FloorAt(want, x, z, y + M / 2);                       // on the floor there (a rug, a step)
            }
        e.cell = (u8)want; e.x = e.hx = x; e.z = e.hz = z; e.y = y; e.vx = e.vz = 0; e.state = S_IDLE;
        e.var = (u8)k; e.pad2 = (u8)WrRow(k, 8);
        }
    }
// where a Riverwood building of a kind stands (KRwHouse: 7 the mill, 8 the water wheel, 9 the bridge), engine units
int GameRwHousePos(int aKind, int* aX, int* aZ)
    {
    int j;
    for (j = 0; j < KRwHouse_N; j++) if (KRwHouse[j * 9] == aKind) { *aX = RwX(KRwHouse[j * 9 + 1]); *aZ = RwZ(KRwHouse[j * 9 + 2]); return KRwHouse[j * 9 + 4] | 4096; }
    return 0;
    }
int GameRiverwoodX() { return RwX(0); }
int GameRiverwoodZ() { return RwZ(0); }
int GameRwDoor(int k, int aPart) { return k < KRwDoor_N ? KRwDoor[k * 9 + aPart] : 0; }
int GameRwDoorN() { return KRwDoor_N; }
// the Riverwood Trader's street door (KRwDoor: house 0 at ground level), facing out along its own exit marker
static const int* TraderDoor()
    {
    int k;
    for (k = 0; k < KRwDoor_N; k++) if (KRwDoor[k * 9 + 8] == 0 && KRwDoor[k * 9 + 2] < 100) return KRwDoor + k * 9;
    return KRwDoor;
    }
static const int* HouseDoor(int aHouse)
    {
    int k;
    for (k = 0; k < KRwDoor_N; k++) if (KRwDoor[k * 9 + 8] == aHouse && KRwDoor[k * 9 + 2] < 100) return KRwDoor + k * 9;
    return KRwDoor;
    }
int GameInnDoorX() { return RwX(HouseDoor(1)[0]); }
int GameInnDoorZ() { return RwZ(HouseDoor(1)[1]); }
int GameInnDoorYaw() { const int* d = HouseDoor(1); return r3d_atan2(d[4] - d[0], d[5] - d[1]) & 1023; }
int GameRwDoorX() { return RwX(TraderDoor()[0]); }
int GameRwDoorZ() { return RwZ(TraderDoor()[1]); }
int GameRwDoorYaw() { const int* d = TraderDoor(); return r3d_atan2(d[4] - d[0], d[5] - d[1]) & 1023; }
int GameRwDoorH() { return TraderDoor()[2]; }
// the Trader's upstairs door (KRwDoor: house 0, up on the balcony)
static const int* BalconyDoor()
    {
    int k;
    for (k = 0; k < KRwDoor_N; k++) if (KRwDoor[k * 9 + 8] == 0 && KRwDoor[k * 9 + 2] >= 100) return KRwDoor + k * 9;
    return TraderDoor();
    }
int GameRwBalconyX() { return RwX(BalconyDoor()[0]); }
int GameRwBalconyZ() { return RwZ(BalconyDoor()[1]); }
int GameRwBalconyYaw() { const int* d = BalconyDoor(); return r3d_atan2(d[4] - d[0], d[5] - d[1]) & 1023; }
int GameRwBalconyH() { return BalconyDoor()[6]; }
