// game_dungeon.cpp - Bleak Falls Barrow: the three cells of the ruin (built from grids traced off the maps,
// see docs/maps/gen_bfb.py), its traps and puzzles, the creatures' special ways, the barrow on its mountain,
// and the two quests that lead there (Farengar's Dragonstone, the Golden Claw).
#include "game.h"

static const int M = METER;
static inline int Abs(int v) { return v < 0 ? -v : v; }
static int Clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static int Cm(int cm) { return cm * M / 100; }
enum { IN_OX = 100 * METER, IN_OZ = 100 * METER, IN_FLOOR = 100 * METER };
static int Ix(int cm) { return IN_OX + Cm(cm); }
static int Iz(int cm) { return IN_OZ + Cm(cm); }
static int ToCm(int u) { return u * 100 / M; }

struct DunProp { int type, x, y, z, yaw, item; };
#include "bfb.inc"

const ItemDef& GameItem(int aId);
const u8* GameItemCol(int aId);

// ---------------------------------------------------------------- the cells' tables
enum { SS = 75 };       // cm per grid sample
enum { Z_KIND = 0, Z_Y0, Z_Y1, Z_CEIL, Z_STYLE, Z_X0, Z_Z0, Z_X1, Z_Z1 };
enum { ZK_FLAT = 0, ZK_RE, ZK_RW, ZK_RN, ZK_RS };
enum { ZS_RUIN = 0, ZS_CAVE, ZS_WATER, ZS_STONE };
// decor kinds (gen_bfb.py D_*), fixtures (F_*)
enum { D_BRAZIER = 0, D_TORCH, D_FIRE, D_BEDROLL, D_URNS, D_COFFIN, D_PILLAR, D_TABLE, D_WEB, D_EGGS, D_PLAQUE,
       D_DOOR, D_WALLCOFFIN, D_BONES, D_WATERFALL, D_RUBBLE, D_SPIRAL, D_LOGS, D_BARREL, D_SACKS, D_ARCH, D_MURAL };
enum { F_GATE = 0, F_PLATE, F_SPIKEWALL, F_BLADE, F_OIL, F_LANTERN, F_DARTS, F_DOORPT, F_COFFINLID, F_TRIGGER };

struct DunData
    {
    const u8* grid; int w, h, gx0, gz0;
    const int* zone; int nz;
    const int* put; int nput;
    const DunProp* prop;
    const int* ent;
    const int* light; int nlight;
    const int* fix; int nfix;
    const int* out;             // (v0.4.62) the ambushers asleep in an alcove or a sarcophagus: number, where they step out to
    };
static const DunData KDun[3] =
    {
    { KBFTEMPLEGrid, KBFTEMPLEW, KBFTEMPLEH, KBFTEMPLEX0, KBFTEMPLEZ0, KBFTEMPLEZone, KBFTEMPLEZone_N, KBFTEMPLEPut, KBFTEMPLEPut_N,
      KBFTEMPLEProp, KBFTEMPLEEnt, KBFTEMPLELight, KBFTEMPLELight_N, KBFTEMPLEFix, KBFTEMPLEFix_N },
    { KBFCRYPTGrid, KBFCRYPTW, KBFCRYPTH, KBFCRYPTX0, KBFCRYPTZ0, KBFCRYPTZone, KBFCRYPTZone_N, KBFCRYPTPut, KBFCRYPTPut_N,
      KBFCRYPTProp, KBFCRYPTEnt, KBFCRYPTLight, KBFCRYPTLight_N, KBFCRYPTFix, KBFCRYPTFix_N },
    { KBFSANCTUMGrid, KBFSANCTUMW, KBFSANCTUMH, KBFSANCTUMX0, KBFSANCTUMZ0, KBFSANCTUMZone, KBFSANCTUMZone_N, KBFSANCTUMPut, KBFSANCTUMPut_N,
      KBFSANCTUMProp, KBFSANCTUMEnt, KBFSANCTUMLight, KBFSANCTUMLight_N, KBFSANCTUMFix, KBFSANCTUMFix_N }
    };
// (v0.4.55) Skyrim's own barrow (data/bleakfalls.har): its working parts where Skyrim has them (docs/maps/
// gen_bfb_kit.py); no grid - its floors and walls are the halls' own (game_kit.cpp)
#include "bfb_kit.inc"
static const DunData KDunKit[2] =
    {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, KBFKTEMPLEProp, KBFKTEMPLEEnt, 0, 0, KBFKTEMPLEFix, KBFKTEMPLEFix_N, KBFKTEMPLEOut },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, KBFKSANCTUMProp, KBFKSANCTUMEnt, 0, 0, KBFKSANCTUMFix, KBFKSANCTUMFix_N, KBFKSANCTUMOut }
    };
// (v0.4.68) Forelhost's three halls (docs/maps/gen_dun_kit.py from the forelhost export): the Stronghold, the Crypt,
// the Refectory
#include "fh_kit.inc"
static const DunData KDunFh[3] =
    {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, KFH1Prop, KFH1Ent, 0, 0, KFH1Fix, KFH1Fix_N, KFH1Out },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, KFH2Prop, KFH2Ent, 0, 0, KFH2Fix, KFH2Fix_N, KFH2Out },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, KFH3Prop, KFH3Ent, 0, 0, KFH3Fix, KFH3Fix_N, KFH3Out }
    };
static int IsFh(int aCell) { return aCell >= CELL_FHSTRONG && aCell <= CELL_FHREFECT; }
// a portcullis's height now (cm raised): Bleak Falls' by its two, Forelhost's by its own
static int GateH(const Game& g, int aCell, const int* f) { return IsFh(aCell) ? g.iFhGate[f[5] & 7] : g.iBfTimer[8 + (f[5] & 1)]; }
// a claw door's rings (0 Bleak Falls', 1 Forelhost's): where each is turned now
static int RingOf(const Game& g, int aDoor, int k) { return aDoor ? g.iS.dun[FH_RING0 + k] : g.iS.bfb[BF_RING0 + k]; }
// the claw door's sinking now (cm), in its cell; -1: none there
static int ClawH(const Game& g, int aCell) { return aCell == CELL_BFSANCTUM ? g.iBfTimer[10] : (aCell == CELL_FHREFECT ? g.iFhClaw : -1); }
static const DunData* DunOf(int aCell);
static const DunData* DunOfK(const Game& g, int aCell)
    {
    switch (aCell)
        {
        case CELL_FHSTRONG: return &KDunFh[0];
        case CELL_FHCRYPT: return &KDunFh[1];
        case CELL_FHREFECT: return &KDunFh[2];
        }
    if (!g.iBfbKit) return DunOf(aCell);
    switch (aCell)
        {
        case CELL_BFTEMPLE: return &KDunKit[0];
        case CELL_BFSANCTUM: return &KDunKit[1];
        }
    return 0;
    }
static const DunData* DunOf(int aCell)
    {
    // (picked one by one: the EPOC compiler folds &KDun[aCell - 7] into a negative address its assembler refuses)
    switch (aCell)
        {
        case CELL_BFTEMPLE: return &KDun[0];
        case CELL_BFCRYPT: return &KDun[1];
        case CELL_BFSANCTUM: return &KDun[2];
        }
    return 0;
    }
static const int* ZoneOf(const DunData& d, int id) { return d.zone + (id - 1) * 9; }
static int Sample(const DunData& d, int i, int j)
    {
    if (i < 0 || j < 0 || i >= d.w || j >= d.h) return 0;
    return d.grid[j * d.w + i];
    }
// the floor of a zone at a point (cm): flat, or along a flight of steps between its low and high ends
static int ZoneY(const int* z, int x, int zc)
    {
    int a, a0, a1;
    switch (z[Z_KIND])
        {
        case ZK_RE: a = x; a0 = z[Z_X0]; a1 = z[Z_X1]; break;
        case ZK_RW: a = -x; a0 = -z[Z_X1]; a1 = -z[Z_X0]; break;
        case ZK_RN: a = zc; a0 = z[Z_Z0]; a1 = z[Z_Z1]; break;
        case ZK_RS: a = -zc; a0 = -z[Z_Z1]; a1 = -z[Z_Z0]; break;
        default: return z[Z_Y0];
        }
    if (a1 <= a0) return z[Z_Y0];
    return z[Z_Y0] + (z[Z_Y1] - z[Z_Y0]) * Clamp(a - a0, 0, a1 - a0) / (a1 - a0);
    }
static int SampleIJ(const DunData& d, int xcm, int zcm, int& i, int& j)
    {
    int dx = xcm - d.gx0, dz = zcm - d.gz0;
    if (dx < 0 || dz < 0) { i = j = -1; return 0; }
    i = dx / SS; j = dz / SS;
    return Sample(d, i, j);
    }

int Game::DunFloor(int aCell, int aX, int aZ, int aFeetY)
    {
    const DunData* d = DunOf(aCell);
    if (!d) return IN_FLOOR;
    int xc = ToCm(aX - IN_OX), zc = ToCm(aZ - IN_OZ), i, j;
    int z = SampleIJ(*d, xc, zc, i, j);
    if (!z) return aFeetY;
    return IN_FLOOR + Cm(ZoneY(ZoneOf(*d, z), xc, zc));
    }

// ---------------------------------------------------------------- doors and gates that close a passage
// a gate across a passage: its span found from the grid (the walkable run either side of its middle)
static void GateSpan(const DunData& d, int x, int z, int along, int& a0, int& a1)
    {
    int i, j;
    SampleIJ(d, x, z, i, j);
    int di = along ? 0 : 1, dj = along ? 1 : 0;
    int k0 = 0, k1 = 0;
    while (k0 < 12 && Sample(d, i - di * (k0 + 1), j - dj * (k0 + 1))) k0++;
    while (k1 < 12 && Sample(d, i + di * (k1 + 1), j + dj * (k1 + 1))) k1++;
    if (along) { a0 = d.gz0 + (j - k0) * SS; a1 = d.gz0 + (j + k1 + 1) * SS; }
    else { a0 = d.gx0 + (i - k0) * SS; a1 = d.gx0 + (i + k1 + 1) * SS; }
    }

int GameIsKitCell(const Game& g, int aCell);
// (v0.4.55) a body of radius r against a bar across the way: its middle, the direction it runs (yaw: as Skyrim turns
// a model's x), half its length, half its depth
static int KitAcross(int aX, int aZ, int aR, int cx, int cz, int aYaw, int aHalf, int aDepth)
    {
    int dx = aX - cx, dz = aZ - cz, c = r3d_cos(aYaw), s = r3d_sin(aYaw);
    int along = (dx * (c >> 4) - dz * (s >> 4)) >> 10, across = (dx * (s >> 4) + dz * (c >> 4)) >> 10;
    return Abs(along) < aHalf + aR && Abs(across) < aDepth + aR;
    }
int Game::DunBlocked(int aCell, int aX, int aZ, int aR, int aFeetY)
    {
    if (GameIsKitCell(*this, aCell))
        {
        // (v0.4.55) Skyrim's own halls: their walls are the kit's; here only the gates (down) and the claw door (shut)
        const DunData* kd = DunOfK(*this, aCell);
        if (!kd) return 0;
        int k;
        for (k = 0; k < kd->nfix; k++)
            {
            const int* f = kd->fix + k * 7;
            if (f[0] != F_GATE || GateH(*this, aCell, f) > 160) continue;
            if (KitAcross(aX, aZ, aR, Ix(f[1]), Iz(f[3]), f[4], Cm(f[6]), Cm(20))) return 1;
            }
        int ch = ClawH(*this, aCell);
        if (ch >= 0 && ch < 200)
            {
            const DunProp* p = kd->prop;
            for (; p->type >= 0; p++)
                if (p->type == PT_CLAWDOOR && KitAcross(aX, aZ, aR, Ix(p->x), Iz(p->z), p->yaw, Cm(170), Cm(30))) return 1;
            }
        return 0;
        }
    const DunData* d = DunOf(aCell);
    if (!d) return 0;
    int xc = ToCm(aX - IN_OX), zc = ToCm(aZ - IN_OZ), rc = ToCm(aR);
    int i0 = (xc - rc - d->gx0), i1 = (xc + rc - d->gx0), j0 = (zc - rc - d->gz0), j1 = (zc + rc - d->gz0);
    if (i0 < 0 || j0 < 0) return 1;
    i0 /= SS; i1 /= SS; j0 /= SS; j1 /= SS;
    int i, j;
    for (j = j0; j <= j1; j++)
        for (i = i0; i <= i1; i++)
            if (!Sample(*d, i, j)) return 1;
    // closed gates
    int k;
    for (k = 0; k < d->nfix; k++)
        {
        const int* f = d->fix + k * 7;
        if (f[0] != F_GATE) continue;
        if (iBfTimer[8 + (f[5] & 1)] > 160) continue;          // raised well clear
        int a0, a1;
        GateSpan(*d, f[1], f[3], 0, a0, a1);
        if (xc + rc > a0 && xc - rc < a1 && zc + rc > f[3] - 20 && zc - rc < f[3] + 20) return 1;
        }
    // the claw door until it opens
    if (aCell == CELL_BFSANCTUM && iBfTimer[10] < 200)
        {
        const DunProp* p = d->prop;
        for (; p->type >= 0; p++)
            if (p->type == PT_CLAWDOOR)
                {
                int a0, a1;
                GateSpan(*d, p->x, p->z, 1, a0, a1);
                if (zc + rc > a0 && zc - rc < a1 && xc + rc > p->x - 30 && xc - rc < p->x + 30) return 1;
                }
        }
    return 0;
    }

// ---------------------------------------------------------------- building the cells
static void DQuad(R3D& r, Mesh& m, int slot, const int* p, int ox, int oz, int cx, int cy, int cz)
    {
    // p: 4 points in cm (cell frame); centre (cm) on the side the face looks away from
    int q[12], k;
    for (k = 0; k < 4; k++) { q[k * 3] = Cm(p[k * 3] - ox); q[k * 3 + 1] = Cm(p[k * 3 + 1]); q[k * 3 + 2] = Cm(p[k * 3 + 2] - oz); }
    r.MeshQuad(m, 0, slot, q, Cm(cx - ox), Cm(cy), Cm(cz - oz));
    }

// building context: the tile's meshes (a new one each time one fills)
struct DunBuild
    {
    Game* g; R3D* r; const DunData* d; int cell;
    int ox, oz;          // the tile's centre (cm)
    Mesh* m;
    };
static void DunFinishMesh(DunBuild& b)
    {
    if (!b.m) return;
    b.r->MeshFinish(*b.m);
    b.r->MeshPlanarUV(*b.m, 3);             // a 2 m texture tile: the big blocks of Nordic stonework
    b.r->MeshTrim(*b.m);
    b.m = 0;
    }
static Mesh* DunMesh(DunBuild& b, int aVerts)
    {
    if (b.m && b.m->nv + aVerts <= b.m->vstride - 4) return b.m;      // (v0.4.51: its own room - MeshInit keeps to 512 since v0.4.40, this still asked MAXMV's 2048: the pieces past 512 left out)
    DunFinishMesh(b);
    Game& g = *b.g;
    if (g.iNDun >= Game::MAXDUN) return 0;
    int k = g.iNDun++;
    b.m = &g.iMDun[k];
    b.r->MeshInit(*b.m, MAXMV, MAXMV * 2, 1);
    g.iDunCell[k] = b.cell; g.iDunX[k] = Ix(b.ox); g.iDunY[k] = IN_FLOOR; g.iDunZ[k] = Iz(b.oz);
    return b.m;
    }
static void BQuad(DunBuild& b, int slot, const int* p, int cx, int cy, int cz)
    {
    Mesh* m = DunMesh(b, 4);
    if (m) DQuad(*b.r, *m, slot, p, b.ox, b.oz, cx, cy, cz);
    }

// slots: 0 ruin floor, 1 ruin wall, 2 ruin ceiling, 3 cave floor, 4 cave wall, 5 cave ceiling, 6 water, 7 stone steps
static int FloorSlot(int st) { return st == ZS_CAVE ? 3 : (st == ZS_WATER ? 6 : (st == ZS_STONE ? 7 : 0)); }
static int WallSlot(int st) { return st == ZS_RUIN || st == ZS_STONE ? 1 : 4; }
static int CeilSlot(int st) { return st == ZS_RUIN || st == ZS_STONE ? 2 : 5; }
static int IsRough(int st) { return st == ZS_CAVE || st == ZS_WATER; }

// rough rock: the corners of the 1.5 m lattice are pushed about; the ones between follow the line between them
static int JitR(int gi, int gj, int salt, int range)
    {
    return (int)(r3d_hash(gi * 7 + 3, gj * 13 + 5, salt) % (u32)(range * 2 + 1)) - range;
    }
static int Jit(int gi, int gj, int salt, int range)
    {
    if (!(gi & 1) && !(gj & 1)) return JitR(gi, gj, salt, range);
    if (gi & 1 && gj & 1) return (JitR(gi - 1, gj - 1, salt, range) + JitR(gi + 1, gj - 1, salt, range) + JitR(gi - 1, gj + 1, salt, range) + JitR(gi + 1, gj + 1, salt, range)) / 4;
    if (gi & 1) return (JitR(gi - 1, gj, salt, range) + JitR(gi + 1, gj, salt, range)) / 2;
    return (JitR(gi, gj - 1, salt, range) + JitR(gi, gj + 1, salt, range)) / 2;
    }
// the ceiling over a sample's corner (corner gi, gj of the lattice) for a zone
static int CeilAt(const DunData& d, const int* z, int gi, int gj)
    {
    int x = d.gx0 + gi * SS, zc = d.gz0 + gj * SS;
    int base = z[Z_KIND] == ZK_FLAT ? z[Z_Y0] : (z[Z_Y0] > z[Z_Y1] ? z[Z_Y0] : z[Z_Y1]);
    (void)x; (void)zc;
    int c = base + z[Z_CEIL];
    if (IsRough(z[Z_STYLE])) c += Jit(gi, gj, 11, 70);
    return c;
    }
static int FloorAtCorner(const DunData& d, const int* z, int gi, int gj)
    {
    return ZoneY(z, d.gx0 + gi * SS, d.gz0 + gj * SS);
    }

// one stretch of wall along a lattice line from corner a to corner b (cm), for the open side's zone z;
// (nx, nz): towards the open side. Ruins: one flat face. Rock: two bands that bulge at mid height.
static void BWall(DunBuild& b, const int* z, int ai, int aj, int bi, int bj, int nx, int nz)
    {
    const DunData& d = *b.d;
    int st = z[Z_STYLE];
    int ax = d.gx0 + ai * SS, az = d.gz0 + aj * SS, bx = d.gx0 + bi * SS, bz = d.gz0 + bj * SS;
    int fa = FloorAtCorner(d, z, ai, aj), fb = FloorAtCorner(d, z, bi, bj);
    if (z[Z_KIND] != ZK_FLAT) { int lo = fa < fb ? fa : fb; fa = fb = lo; if (z[Z_Y0] < lo) fa = fb = z[Z_Y0] < z[Z_Y1] ? z[Z_Y0] : z[Z_Y1]; }
    int ta = CeilAt(d, z, ai, aj), tb = CeilAt(d, z, bi, bj);
    int cx = (ax + bx) / 2 - nx * 100, cz = (az + bz) / 2 - nz * 100, cy = (fa + ta) / 2;
    if (!IsRough(st))
        {
        // Nordic stone: upright to the springing, then the vault's haunch curving in over the room to meet the
        // ceiling (two facets), so passages read as low rounded vaults, not tall shafts. Where the top would come
        // lower than 2.4 m over the floor the wall just stands straight.
        int ha = ta - fa, hb = tb - fb;
        int sa = fa + (ha * 11 >> 4), sb = fb + (hb * 11 >> 4);           // springing at 69 % of the height
        if (sa > fa + 300) sa = fa + 300; if (sb > fb + 300) sb = fb + 300;
        int ins = ((ta - sa) + (tb - sb)) / 3;                             // how far the haunch reaches in
        if (ins > 110) ins = 110;
        if (ha < 240 || hb < 240 || ins < 25)
            {
            int p[12] = { ax, fa - 10, az,  bx, fb - 10, bz,  bx, tb, bz,  ax, ta, az };
            BQuad(b, WallSlot(st), p, cx, cy, cz);
            return;
            }
        int p0[12] = { ax, fa - 10, az,  bx, fb - 10, bz,  bx, sb, bz,  ax, sa, az };
        BQuad(b, WallSlot(st), p0, cx, cy, cz);
        // the haunch's knee: half way up, a third of the reach in; the top: all the way in, at the ceiling
        int ka = (sa + ta) / 2, kb = (sb + tb) / 2, ki = ins * 5 / 12;
        int p1[12] = { ax, sa, az,  bx, sb, bz,  bx + nx * ki, kb, bz + nz * ki,  ax + nx * ki, ka, az + nz * ki };
        BQuad(b, WallSlot(st), p1, cx, cy, cz);
        int p2[12] = { ax + nx * ki, ka, az + nz * ki,  bx + nx * ki, kb, bz + nz * ki,  bx + nx * ins, tb + 2, bz + nz * ins,  ax + nx * ins, ta + 2, az + nz * ins };
        BQuad(b, WallSlot(st), p2, cx, (ka + ta) / 2, cz);
        return;
        }
    // mid ring: pushed about by the corner's own offsets (so walls meeting at a corner share the point)
    int ma = (fa + ta) / 2 + Jit(ai, aj, 3, 50), mb = (fb + tb) / 2 + Jit(bi, bj, 3, 50);
    int mxa = ax + Jit(ai, aj, 5, 28), mza = az + Jit(ai, aj, 7, 28), mxb = bx + Jit(bi, bj, 5, 28), mzb = bz + Jit(bi, bj, 7, 28);
    int p0[12] = { ax, fa - 10, az,  bx, fb - 10, bz,  mxb, mb, mzb,  mxa, ma, mza };
    BQuad(b, WallSlot(st), p0, cx, cy, cz);
    int p1[12] = { mxa, ma, mza,  mxb, mb, mzb,  bx, tb, bz,  ax, ta, az };
    BQuad(b, WallSlot(st), p1, cx, cy, cz);
    }

// between two open samples whose floors (or ceilings) differ: the step face, looking at the lower floor
// (at the higher ceiling)
static void BStep(DunBuild& b, int ai, int aj, int bi, int bj, int ya0, int ya1, int yb0, int yb1, int nx, int nz, int slot)
    {
    // side A has heights ya0 (at corner a), ya1 (at b); side B yb0, yb1; the face looks towards (nx, nz)
    const DunData& d = *b.d;
    int ax = d.gx0 + ai * SS, az = d.gz0 + aj * SS, bx = d.gx0 + bi * SS, bz = d.gz0 + bj * SS;
    int lo0 = ya0 < yb0 ? ya0 : yb0, hi0 = ya0 < yb0 ? yb0 : ya0, lo1 = ya1 < yb1 ? ya1 : yb1, hi1 = ya1 < yb1 ? yb1 : ya1;
    if (hi0 - lo0 < 4 && hi1 - lo1 < 4) return;
    int p[12] = { ax, lo0, az,  bx, lo1, bz,  bx, hi1, bz,  ax, hi0, az };
    BQuad(b, slot, p, (ax + bx) / 2 - nx * 100, (lo0 + hi0) / 2, (az + bz) / 2 - nz * 100);
    }

// the edges of one lattice line: walls where open meets solid, steps where two floors meet.
// horizontal line jj (between sample rows jj-1 and jj), samples i0..i1-1
static void BLineH(DunBuild& b, int jj, int i0, int i1)
    {
    const DunData& d = *b.d;
    int i = i0;
    while (i < i1)
        {
        int s = Sample(d, i, jj - 1), n = Sample(d, i, jj);
        if (s == n || (!s && !n)) { i++; continue; }
        // a run of identical edges (flat zones only; rock in pieces of 1.5 m)
        int len = 1;
        const int* zs = s ? ZoneOf(d, s) : 0; const int* zn = n ? ZoneOf(d, n) : 0;
        int flat = (!zs || zs[Z_KIND] == ZK_FLAT) && (!zn || zn[Z_KIND] == ZK_FLAT);
        int rough = (zs && IsRough(zs[Z_STYLE])) || (zn && IsRough(zn[Z_STYLE]));
        int maxLen = rough ? 2 - (i & 1) : 12;
        if (flat)
            while (len < maxLen && i + len < i1 && Sample(d, i + len, jj - 1) == s && Sample(d, i + len, jj) == n) len++;
        int ai = i, bi = i + len;
        if (!n) BWall(b, zs, bi, jj, ai, jj, 0, -1);            // open to the south: the wall faces south
        else if (!s) BWall(b, zn, ai, jj, bi, jj, 0, 1);
        else
            {
            int fs0 = FloorAtCorner(d, zs, ai, jj), fs1 = FloorAtCorner(d, zs, bi, jj);
            int fn0 = FloorAtCorner(d, zn, ai, jj), fn1 = FloorAtCorner(d, zn, bi, jj);
            int lowS = fs0 + fs1 < fn0 + fn1;
            BStep(b, ai, jj, bi, jj, fs0, fs1, fn0, fn1, 0, lowS ? -1 : 1, WallSlot(lowS ? zn[Z_STYLE] : zs[Z_STYLE]));
            int cs0 = CeilAt(d, zs, ai, jj), cs1 = CeilAt(d, zs, bi, jj), cn0 = CeilAt(d, zn, ai, jj), cn1 = CeilAt(d, zn, bi, jj);
            int highS = cs0 + cs1 > cn0 + cn1;
            BStep(b, ai, jj, bi, jj, cs0, cs1, cn0, cn1, 0, highS ? -1 : 1, WallSlot(highS ? zn[Z_STYLE] : zs[Z_STYLE]));
            }
        i += len;
        }
    }
// vertical line ii (between sample columns ii-1 and ii), samples j0..j1-1
static void BLineV(DunBuild& b, int ii, int j0, int j1)
    {
    const DunData& d = *b.d;
    int j = j0;
    while (j < j1)
        {
        int w = Sample(d, ii - 1, j), e = Sample(d, ii, j);
        if (w == e || (!w && !e)) { j++; continue; }
        int len = 1;
        const int* zw = w ? ZoneOf(d, w) : 0; const int* ze = e ? ZoneOf(d, e) : 0;
        int flat = (!zw || zw[Z_KIND] == ZK_FLAT) && (!ze || ze[Z_KIND] == ZK_FLAT);
        int rough = (zw && IsRough(zw[Z_STYLE])) || (ze && IsRough(ze[Z_STYLE]));
        int maxLen = rough ? 2 - (j & 1) : 12;
        if (flat)
            while (len < maxLen && j + len < j1 && Sample(d, ii - 1, j + len) == w && Sample(d, ii, j + len) == e) len++;
        int aj = j, bj = j + len;
        if (!e) BWall(b, zw, ii, aj, ii, bj, -1, 0);
        else if (!w) BWall(b, ze, ii, bj, ii, aj, 1, 0);
        else
            {
            int fw0 = FloorAtCorner(d, zw, ii, aj), fw1 = FloorAtCorner(d, zw, ii, bj);
            int fe0 = FloorAtCorner(d, ze, ii, aj), fe1 = FloorAtCorner(d, ze, ii, bj);
            int lowW = fw0 + fw1 < fe0 + fe1;
            BStep(b, ii, aj, ii, bj, fw0, fw1, fe0, fe1, lowW ? -1 : 1, 0, WallSlot(lowW ? ze[Z_STYLE] : zw[Z_STYLE]));
            int cw0 = CeilAt(d, zw, ii, aj), cw1 = CeilAt(d, zw, ii, bj), ce0 = CeilAt(d, ze, ii, aj), ce1 = CeilAt(d, ze, ii, bj);
            int highW = cw0 + cw1 > ce0 + ce1;
            BStep(b, ii, aj, ii, bj, cw0, cw1, ce0, ce1, highW ? -1 : 1, 0, WallSlot(highW ? ze[Z_STYLE] : zw[Z_STYLE]));
            }
        j += len;
        }
    }

// floors and ceilings of one tile (16 x 16 samples): flat floors merged into rectangles, ceilings too
// where they are level; rough ceilings in 1.5 m pieces with their corners pushed about
enum { TILE = 16 };
static void BTileSurfaces(DunBuild& b, int ti, int tj)
    {
    const DunData& d = *b.d;
    u8 done[TILE * TILE];
    int i, j, k;
    for (k = 0; k < TILE * TILE; k++) done[k] = 0;
    int i0 = ti * TILE, j0 = tj * TILE;
    // floors (flat zones)
    for (j = 0; j < TILE; j++)
        for (i = 0; i < TILE; i++)
            {
            int z = Sample(d, i0 + i, j0 + j);
            if (!z || done[j * TILE + i]) continue;
            const int* zz = ZoneOf(d, z);
            if (zz[Z_KIND] != ZK_FLAT) continue;
            int w = 1, h = 1;
            while (i + w < TILE && !done[j * TILE + i + w] && Sample(d, i0 + i + w, j0 + j) == z) w++;
            for (;;)
                {
                if (j + h >= TILE) break;
                int ok = 1, q;
                for (q = 0; q < w && ok; q++) if (done[(j + h) * TILE + i + q] || Sample(d, i0 + i + q, j0 + j + h) != z) ok = 0;
                if (!ok) break;
                h++;
                }
            int q, r2;
            for (r2 = 0; r2 < h; r2++) for (q = 0; q < w; q++) done[(j + r2) * TILE + i + q] = 1;
            int x0 = d.gx0 + (i0 + i) * SS, x1 = x0 + w * SS, z0 = d.gz0 + (j0 + j) * SS, z1 = z0 + h * SS;
            int y = zz[Z_Y0];
            int p[12] = { x0, y, z0,  x1, y, z0,  x1, y, z1,  x0, y, z1 };
            BQuad(b, FloorSlot(zz[Z_STYLE]), p, (x0 + x1) / 2, y - 100, (z0 + z1) / 2);
            }
    // ceilings
    for (k = 0; k < TILE * TILE; k++) done[k] = 0;
    for (j = 0; j < TILE; j++)
        for (i = 0; i < TILE; i++)
            {
            int z = Sample(d, i0 + i, j0 + j);
            if (!z || done[j * TILE + i]) continue;
            const int* zz = ZoneOf(d, z);
            int gi = i0 + i, gj = j0 + j;
            if (IsRough(zz[Z_STYLE]))
                {
                // a 1.5 m piece when the four samples of the lattice square are all this zone, else the sample
                int w = 1, h = 1;
                if (!(gi & 1) && !(gj & 1) && i + 1 < TILE && j + 1 < TILE && Sample(d, gi + 1, gj) == z && Sample(d, gi, gj + 1) == z &&
                    Sample(d, gi + 1, gj + 1) == z && !done[j * TILE + i + 1] && !done[(j + 1) * TILE + i]) { w = 2; h = 2; }
                done[j * TILE + i] = 1;
                if (w == 2) { done[j * TILE + i + 1] = 1; done[(j + 1) * TILE + i] = 1; done[(j + 1) * TILE + i + 1] = 1; }
                int x0 = d.gx0 + gi * SS, x1 = x0 + w * SS, z0 = d.gz0 + gj * SS, z1 = z0 + h * SS;
                int p[12] = { x0, CeilAt(d, zz, gi, gj), z0,  x1, CeilAt(d, zz, gi + w, gj), z0,
                              x1, CeilAt(d, zz, gi + w, gj + h), z1,  x0, CeilAt(d, zz, gi, gj + h), z1 };
                BQuad(b, CeilSlot(zz[Z_STYLE]), p, (x0 + x1) / 2, p[1] + 200, (z0 + z1) / 2);
                continue;
                }
            if (zz[Z_KIND] != ZK_FLAT)
                {
                // over a flight: level pieces a sample each
                done[j * TILE + i] = 1;
                int x0 = d.gx0 + gi * SS, x1 = x0 + SS, z0 = d.gz0 + gj * SS, z1 = z0 + SS;
                int y = CeilAt(d, zz, gi, gj);
                int p[12] = { x0, y, z0,  x1, y, z0,  x1, y, z1,  x0, y, z1 };
                BQuad(b, CeilSlot(zz[Z_STYLE]), p, (x0 + x1) / 2, y + 100, (z0 + z1) / 2);
                continue;
                }
            int w = 1, h = 1;
            while (i + w < TILE && !done[j * TILE + i + w] && Sample(d, i0 + i + w, j0 + j) == z) w++;
            for (;;)
                {
                if (j + h >= TILE) break;
                int ok = 1, q;
                for (q = 0; q < w && ok; q++) if (done[(j + h) * TILE + i + q] || Sample(d, i0 + i + q, j0 + j + h) != z) ok = 0;
                if (!ok) break;
                h++;
                }
            int q, r2;
            for (r2 = 0; r2 < h; r2++) for (q = 0; q < w; q++) done[(j + r2) * TILE + i + q] = 1;
            int x0 = d.gx0 + gi * SS, x1 = x0 + w * SS, z0 = d.gz0 + gj * SS, z1 = z0 + h * SS;
            int y = zz[Z_Y0] + zz[Z_CEIL];
            int p[12] = { x0, y, z0,  x1, y, z0,  x1, y, z1,  x0, y, z1 };
            BQuad(b, CeilSlot(zz[Z_STYLE]), p, (x0 + x1) / 2, y + 100, (z0 + z1) / 2);
            }
    }

// a flight of steps: solid steps across its width, from below its low end up to each tread
static void BRamp(DunBuild& b, const int* z)
    {
    int lo = z[Z_Y0], hi = z[Z_Y1];
    int n = Abs(hi - lo) / 25; if (n < 2) n = 2; if (n > 24) n = 24;
    int x0 = z[Z_X0], x1 = z[Z_X1], z0 = z[Z_Z0], z1 = z[Z_Z1];
    int s;
    for (s = 0; s < n; s++)
        {
        int top = lo + (hi - lo) * (s + 1) / n;
        int bx0 = x0, bx1 = x1, bz0 = z0, bz1 = z1;
        switch (z[Z_KIND])
            {
            case ZK_RE: bx0 = x0 + (x1 - x0) * s / n; bx1 = x0 + (x1 - x0) * (s + 1) / n; break;
            case ZK_RW: bx1 = x1 - (x1 - x0) * s / n; bx0 = x1 - (x1 - x0) * (s + 1) / n; break;
            case ZK_RN: bz0 = z0 + (z1 - z0) * s / n; bz1 = z0 + (z1 - z0) * (s + 1) / n; break;
            default:    bz1 = z1 - (z1 - z0) * s / n; bz0 = z1 - (z1 - z0) * (s + 1) / n; break;
            }
        Mesh* m = DunMesh(b, 8);
        if (!m) return;
        int bot = (lo < hi ? lo : hi) - 20;
        b.r->MeshBox(*m, 0, z[Z_STYLE] == ZS_CAVE ? 3 : 7, Cm((bx0 + bx1) / 2 - b.ox), Cm(top), Cm((bz0 + bz1) / 2 - b.oz), 0,
                     Cm(bx1 - bx0) / 2, Cm(top - bot) / 2, Cm(bz1 - bz0) / 2);
        }
    }

static void PlaceDecor(Game& g, int c, const DunData& d);
void Game::BuildDungeons()
    {
    R3D& r = *iR;
    iNDun = 0;
    int c;
    for (c = CELL_BFTEMPLE; c <= CELL_BFSANCTUM; c++)
        {
        const DunData& d = *DunOf(c);
        int nti = (d.w + TILE - 1) / TILE, ntj = (d.h + TILE - 1) / TILE, ti, tj, k;
        for (tj = 0; tj < ntj; tj++)
            for (ti = 0; ti < nti; ti++)
                {
                // anything open in this tile?
                int any = 0, i, j;
                for (j = tj * TILE; j < tj * TILE + TILE && !any; j++)
                    for (i = ti * TILE; i < ti * TILE + TILE && !any; i++)
                        if (Sample(d, i, j)) any = 1;
                if (!any) continue;
                DunBuild b;
                b.g = this; b.r = &r; b.d = &d; b.cell = c; b.m = 0;
                b.ox = d.gx0 + (ti * TILE + TILE / 2) * SS; b.oz = d.gz0 + (tj * TILE + TILE / 2) * SS;
                BTileSurfaces(b, ti, tj);
                // the lattice lines this tile owns: its own and, at the grid's far edges, the last ones
                int jj0 = tj * TILE, jj1 = tj * TILE + TILE + (tj == ntj - 1 ? 1 : 0);
                int ii0 = ti * TILE, ii1 = ti * TILE + TILE + (ti == nti - 1 ? 1 : 0);
                int ie = ti * TILE + TILE, je = tj * TILE + TILE;
                for (j = jj0; j < jj1; j++) BLineH(b, j, ti * TILE, ie);
                for (i = ii0; i < ii1; i++) BLineV(b, i, tj * TILE, je);
                // flights whose middle lies in this tile
                for (k = 1; k <= d.nz; k++)
                    {
                    const int* z = ZoneOf(d, k);
                    if (z[Z_KIND] == ZK_FLAT) continue;
                    int mi = ((z[Z_X0] + z[Z_X1]) / 2 - d.gx0) / SS, mj = ((z[Z_Z0] + z[Z_Z1]) / 2 - d.gz0) / SS;
                    if (mi / TILE == ti && mj / TILE == tj) BRamp(b, z);
                    }
                DunFinishMesh(b);
                }
        PlaceDecor(*this, c, d);
        }
    // the colours: Nordic stone (grey-beige blocks, flagged floors), dark rock, water, worn steps
    u8* s = iDunSl;
    // (the ceilings are plain: dark, seldom looked at, and a cave's roof fills much of the screen)
    static const u8 KSl[32] = { 150, 118, 78, SL_TEX | TXI_NORDIC,   150, 118, 78, SL_TEX | TXI_NORDIC,   70, 56, 42, 1,
                                92, 84, 72, SL_TEX | TXI_DIRT,       96, 90, 84, SL_TEX | TXI_ROCK,     58, 55, 51, 1,
                                54, 84, 98, 1,                       150, 118, 78, SL_TEX | TXI_NORDIC };
    int k;
    for (k = 0; k < 32; k++) s[k] = KSl[k];
    }

// ---------------------------------------------------------------- the models
// box corner to corner (cm) into a frame
static void BX(R3D& r, Mesh& m, int fr, int slot, int x0, int y0, int z0, int x1, int y1, int z1)
    {
    r.MeshBox(m, fr, slot, Cm(x0 + x1) / 2, Cm(y1), Cm(z0 + z1) / 2, 0, Cm(x1 - x0) / 2, Cm(y1 - y0) / 2, Cm(z1 - z0) / 2);
    }
// a box turned about y (cm): the four sides and the top as quads
static void YBox(R3D& r, Mesh& m, int fr, int slot, int cx, int y0, int cz, int yaw, int hx, int h, int hz)
    {
    int cs = r3d_cos(yaw), sn = r3d_sin(yaw), k;
    int px[4], pz[4];
    static const int KS[8] = { -1, -1,  1, -1,  1, 1,  -1, 1 };
    for (k = 0; k < 4; k++)
        {
        int lx = KS[k * 2] * hx, lz = KS[k * 2 + 1] * hz;
        px[k] = cx + ((lx * cs + lz * sn) >> 14); pz[k] = cz + ((-lx * sn + lz * cs) >> 14);
        }
    for (k = 0; k < 4; k++)
        {
        int a = k, b2 = (k + 1) & 3;
        int q[12] = { Cm(px[a]), Cm(y0), Cm(pz[a]),  Cm(px[b2]), Cm(y0), Cm(pz[b2]),  Cm(px[b2]), Cm(y0 + h), Cm(pz[b2]),  Cm(px[a]), Cm(y0 + h), Cm(pz[a]) };
        r.MeshQuad(m, fr, slot, q, Cm(cx), Cm(y0 + h / 2), Cm(cz));
        }
    int q[12] = { Cm(px[0]), Cm(y0 + h), Cm(pz[0]),  Cm(px[1]), Cm(y0 + h), Cm(pz[1]),  Cm(px[2]), Cm(y0 + h), Cm(pz[2]),  Cm(px[3]), Cm(y0 + h), Cm(pz[3]) };
    r.MeshQuad(m, fr, slot, q, Cm(cx), Cm(y0), Cm(cz));
    }
static void Rod(R3D& r, Mesh& m, int fr, int slot, int x0, int y0, int z0, int x1, int y1, int z1, int rad, int sides)
    {
    r.MeshLimb(m, fr, slot, Cm(x0), Cm(y0), Cm(z0), Cm(x1), Cm(y1), Cm(z1), Cm(rad), Cm(rad), Cm(rad), Cm(rad), sides, 3);
    }
static void DefM(StaticDef& d, const Mesh* m, int hxCm, int hzCm, int radCm, int hyCm)
    {
    r3d_memset(&d, 0, sizeof(d));
    d.mesh = m; d.hx = (short)Cm(hxCm); d.hz = (short)Cm(hzCm); d.rad = (short)Cm(radCm); d.hy = (short)Cm(hyCm);
    d.drawDist = 60;
    }
static void Col(StaticDef& d, int s, int r, int g, int b, int t)
    {
    d.slots[s * 4] = (u8)r; d.slots[s * 4 + 1] = (u8)g; d.slots[s * 4 + 2] = (u8)b; d.slots[s * 4 + 3] = (u8)t;
    }
static void Finish(R3D& r, Mesh& m) { r.MeshFinish(m); r.MeshPlanarUV(m, 2); }

// the carved animals of the puzzles: 0 snake, 1 bird (eagle), 2 whale as flat relief silhouettes (a strip of
// quads along each animal's line, standing proud of its face); 3 bear, 4 moth, 5 owl (the claw door) as blocks.
// Drawn on a face looking +z at depth z, centred (0, cy), units of s/100 cm.
static void GQuad(R3D& r, Mesh& m, int fr, int slot, const int* xy, int cy, int z, int s)
    {
    int q[12], k;
    for (k = 0; k < 4; k++) { q[k * 3] = Cm(xy[k * 2] * s / 100); q[k * 3 + 1] = Cm(cy + xy[k * 2 + 1] * s / 100); q[k * 3 + 2] = Cm(z); }
    r.MeshQuad(m, fr, slot, q, 0, Cm(cy), Cm(z - 20));
    }
// a thick line through points (x, y, half width) as quads
static void GLine(R3D& r, Mesh& m, int fr, int slot, const short* p, int n, int cy, int z, int s)
    {
    int k;
    for (k = 0; k + 1 < n; k++)
        {
        const short* a = p + k * 3; const short* b = p + k * 3 + 3;
        int dx = b[0] - a[0], dy = b[1] - a[1];
        int l = r3d_isqrt(dx * dx + dy * dy); if (l < 1) l = 1;
        int nx = -dy * 64 / l, ny = dx * 64 / l;                    // unit normal, x64
        int q[8] = { a[0] - nx * a[2] / 64, a[1] - ny * a[2] / 64,  b[0] - nx * b[2] / 64, b[1] - ny * b[2] / 64,
                     b[0] + nx * b[2] / 64, b[1] + ny * b[2] / 64,  a[0] + nx * a[2] / 64, a[1] + ny * a[2] / 64 };
        // keep the winding facing +z whichever way the line runs
        if ((q[2] - q[0]) * (q[7] - q[1]) - (q[3] - q[1]) * (q[6] - q[0]) < 0)
            { int t; t = q[2]; q[2] = q[6]; q[6] = t; t = q[3]; q[3] = q[7]; q[7] = t; }
        GQuad(r, m, fr, slot, q, cy, z, s);
        }
    }
static void Glyph(R3D& r, Mesh& m, int fr, int slot, int kind, int cy, int z, int s)
    {
    if (kind == 0)
        {
        // the snake: an S from the head (top) down to the tail, thick in the middle
        static const short KS[] = { 6, 40, 9,  -10, 32, 8,  -14, 18, 8,  -2, 8, 8,  12, -2, 8,  14, -16, 7,  2, -26, 6,  -10, -34, 4,  -6, -46, 2 };
        GLine(r, m, fr, slot, KS, 9, cy, z, s);
        static const int KH[8] = { 0, 36,  14, 40,  12, 50,  0, 48 };       // the head, a little wider
        GQuad(r, m, fr, slot, KH, cy, z, s);
        return;
        }
    if (kind == 1)
        {
        // the bird: wings spread and raised, body, head and beak, a fanned tail
        static const int KB[6 * 8] =
            {
            -6, -20,   6, -20,   7, 18,  -7, 18,                 // body
            -6, 14,  -46, 40,  -50, 28,   -6, 2,                 // left wing
             6, 2,    50, 28,   46, 40,    6, 14,                // right wing
            -5, 18,    5, 18,    6, 30,   -6, 30,                // head
             2, 28,   12, 25,    6, 30,    2, 32,                // beak
            -4, -18,   4, -18,   14, -38, -14, -38               // tail
            };
        int k;
        for (k = 0; k < 6; k++) GQuad(r, m, fr, slot, KB + k * 8, cy, z, s);
        return;
        }
    if (kind == 2)
        {
        // the whale: leaping up to the right, head low left, the flukes at the top
        static const short KW[] = { -38, -26, 6,  -22, -16, 14,  0, -2, 15,  18, 12, 11,  30, 24, 5 };
        GLine(r, m, fr, slot, KW, 5, cy, z, s);
        static const int KT[2 * 8] = { 28, 22,  32, 26,  30, 44,  22, 40,      // left fluke
                                       28, 22,  46, 18,  48, 28,  32, 26 };    // right fluke
        GQuad(r, m, fr, slot, KT, cy, z, s); GQuad(r, m, fr, slot, KT + 8, cy, z, s);
        static const int KF[8] = { -8, -14,  2, -10,  -2, -26,  -10, -24 };   // the flipper
        GQuad(r, m, fr, slot, KF, cy, z, s);
        return;
        }
    // the claw door's creatures: five boxes each (x, y, half w, half h)
    static const short KG[3 * 5 * 4] =
        {
        // bear: body, head, legs
        0, 6, 34, 16,   -40, 16, 12, 11,   -24, -22, 7, 12,   20, -22, 7, 12,   36, 18, 6, 6,
        // moth: four wings and the body
        -22, 14, 20, 14,   22, 14, 20, 14,   -16, -16, 14, 12,   16, -16, 14, 12,   0, 0, 5, 30,
        // owl: body, head, ears, feet
        0, -6, 18, 26,   0, 28, 16, 12,   -12, 42, 5, 6,   12, 42, 5, 6,   0, -36, 14, 4
        };
    int k;
    for (k = 0; k < 5; k++)
        {
        const short* g = KG + ((kind - 3) * 5 + k) * 4;
        int cx = g[0] * s / 100, yy = cy + g[1] * s / 100, hw = g[2] * s / 100, hh = g[3] * s / 100;
        BX(r, m, fr, slot, cx - hw, yy - hh, z, cx + hw, yy + hh, z + 3);
        }
    }

// ---------------------------------------------------------------- Nordic stonework: columns, arches, murals
static void NordicStone(Game& g)
    {
    R3D& r = *g.iR;
    Mesh* im = g.iMIn;
    StaticDef d;
    int i;
    // ---- a great round column: a stepped foot, a shaft that swells and narrows like a carved trunk, a capital
    {
    Mesh& m = im[IN_NORCOL];
    r.MeshInit(m, 200, 320, 1);
    static const short KC[6 * 2] = { 0, 70,  40, 58,  120, 64,  260, 56,  340, 52,  400, 70 };     // height, radius
    r.MeshLimb(m, 0, 0, 0, Cm(-2), 0, 0, Cm(30), 0, Cm(80), Cm(80), Cm(76), Cm(76), 8, 2);          // the foot
    for (i = 0; i + 1 < 6; i++)
        r.MeshLimb(m, 0, i == 4 ? 0 : 1, 0, Cm(KC[i * 2]), 0, 0, Cm(KC[i * 2 + 2]), 0, Cm(KC[i * 2 + 1]), Cm(KC[i * 2 + 1]), Cm(KC[i * 2 + 3]), Cm(KC[i * 2 + 3]), 8, 2);
    Finish(r, m);
    DefM(d, &m, 0, 0, 62, 400);
    Col(d, 0, 140, 110, 72, SL_TEX | TXI_NORDIC); Col(d, 1, 156, 124, 82, SL_TEX | TXI_NORDIC);
    g.iInType[IN_NORCOL] = r.RegisterStatic(d);
    }
    // ---- a heavy round arch across a passage 300 cm wide (spanning local x, the passage running along z):
    // squat piers either side and a thick band of voussoirs over, springing at 210 cm, crown at 330 cm
    {
    Mesh& m = im[IN_ARCH];
    r.MeshInit(m, 220, 320, 1);
    YBox(r, m, 0, 0, -136, 0, 0, 0, 22, 210, 30);
    YBox(r, m, 0, 0, 136, 0, 0, 0, 22, 210, 30);
    enum { NS = 6 };
    int ox[NS + 1], oy[NS + 1], ix[NS + 1], iy[NS + 1];
    for (i = 0; i <= NS; i++)
        {
        int a = 256 - i * 512 / NS;                          // from the left pier (a = 256) over to the right (-256)
        int s = r3d_sin(a), c = r3d_cos(a);
        ix[i] = -(s * 114 >> 14); iy[i] = 210 + (c * 114 >> 14);       // the soffit (inner edge)
        ox[i] = -(s * 158 >> 14); oy[i] = 210 + (c * 158 >> 14);       // the extrados
        }
    for (i = 0; i < NS; i++)
        {
        int zf = 30, zb = -30;
        int f[12] = { Cm(ix[i]), Cm(iy[i]), Cm(zf),  Cm(ix[i + 1]), Cm(iy[i + 1]), Cm(zf),  Cm(ox[i + 1]), Cm(oy[i + 1]), Cm(zf),  Cm(ox[i]), Cm(oy[i]), Cm(zf) };
        r.MeshQuad(m, 0, 1, f, 0, Cm(210), 0);
        int bk[12] = { Cm(ix[i]), Cm(iy[i]), Cm(zb),  Cm(ix[i + 1]), Cm(iy[i + 1]), Cm(zb),  Cm(ox[i + 1]), Cm(oy[i + 1]), Cm(zb),  Cm(ox[i]), Cm(oy[i]), Cm(zb) };
        r.MeshQuad(m, 0, 1, bk, 0, Cm(210), 0);
        int u[12] = { Cm(ix[i]), Cm(iy[i]), Cm(zb),  Cm(ix[i + 1]), Cm(iy[i + 1]), Cm(zb),  Cm(ix[i + 1]), Cm(iy[i + 1]), Cm(zf),  Cm(ix[i]), Cm(iy[i]), Cm(zf) };
        r.MeshQuad(m, 0, 0, u, 0, Cm(400), 0);
        }
    Finish(r, m);
    DefM(d, &m, 0, 0, 0, 0);
    Col(d, 0, 140, 110, 72, SL_TEX | TXI_NORDIC); Col(d, 1, 158, 126, 84, SL_TEX | TXI_NORDIC);
    g.iInType[IN_ARCH] = r.RegisterStatic(d);
    }
    // ---- a mural of the Hall of Stories: a carved panel in a raised frame, a figure between two beasts and a
    // dragon's wings over them (facing +z, set against a wall)
    {
    Mesh& m = im[IN_MURAL];
    r.MeshInit(m, 220, 320, 1);
    BX(r, m, 0, 0, -130, 40, -12, 130, 300, 0);                  // the frame
    BX(r, m, 0, 1, -112, 58, 0, 112, 282, 4);                    // the panel
    Glyph(r, m, 0, 2, 3, 120, 6, 60);                            // a bear
    YBox(r, m, 0, 2, 0, 90, 8, 0, 9, 110, 2);                    // the hero: body
    r.MeshEllipsoid(m, 0, 2, 0, Cm(212), Cm(8), Cm(12), Cm(14), Cm(4), 6, 2);
    static const int KW[2 * 8] = { 0, 30,  -90, 70,  -80, 50,  0, 20,     0, 20,  80, 50,  90, 70,  0, 30 };
    GQuad(r, m, 0, 2, KW, 200, 8, 100); GQuad(r, m, 0, 2, KW + 8, 200, 8, 100);
    Finish(r, m);
    DefM(d, &m, 0, 0, 0, 0);
    Col(d, 0, 132, 104, 70, SL_TEX | TXI_NORDIC); Col(d, 1, 92, 74, 52, 1); Col(d, 2, 170, 146, 104, 1);
    g.iInType[IN_MURAL] = r.RegisterStatic(d);
    }
    }

void Game::BuildDungeonMeshes()
    {
    R3D& r = *iR;
    Mesh* im = iMIn;
    StaticDef d;
    int i, fr;
    NordicStone(*this);
    // ---- a cobweb: strands from a centre and two rings round it, in a vertical sheet (facing z), 240 cm across
    {
    Mesh& m = im[IN_WEB];
    r.MeshInit(m, 200, 300, 1);
    for (i = 0; i < 7; i++)
        {
        int a = i * 146 + 30;
        int x = (r3d_sin(a) * 120) >> 14, y = 140 + ((r3d_cos(a) * 110) >> 14);
        Rod(r, m, 0, 0, 0, 140, 0, x, y, (i & 1) * 20 - 10, 1, 3);
        }
    for (i = 0; i < 7; i++)
        {
        int a0 = i * 146 + 30, a1 = a0 + 146, k;
        for (k = 1; k <= 2; k++)
            Rod(r, m, 0, 0, (r3d_sin(a0) * 45 * k) >> 14, 140 + ((r3d_cos(a0) * 42 * k) >> 14), 0, (r3d_sin(a1) * 45 * k) >> 14, 140 + ((r3d_cos(a1) * 42 * k) >> 14), 0, 1, 3);
        }
    Finish(r, m);
    DefM(d, &m, 0, 0, 0, 0);
    Col(d, 0, 214, 214, 206, 1);
    iInType[IN_WEB] = r.RegisterStatic(d);
    }
    // ---- spider eggs: a sticky clutch of pale sacs
    {
    Mesh& m = im[IN_EGGS];
    r.MeshInit(m, 120, 200, 1);
    static const short KE[6 * 4] = { 0, 18, 0, 20,   26, 14, 12, 16,   -24, 15, 8, 17,   8, 12, -24, 14,   -12, 34, -6, 14,   14, 30, 18, 12 };
    for (i = 0; i < 6; i++) r.MeshEllipsoid(m, 0, i == 0 ? 1 : 0, Cm(KE[i * 4]), Cm(KE[i * 4 + 1]), Cm(KE[i * 4 + 2]), Cm(KE[i * 4 + 3]), Cm(KE[i * 4 + 3] * 5 / 4), Cm(KE[i * 4 + 3]), 6, 3);
    r.MeshFinish(m);
    DefM(d, &m, 0, 0, 40, 50);
    Col(d, 0, 206, 210, 196, 2); Col(d, 1, 186, 196, 186, 2);
    iInType[IN_EGGS] = r.RegisterStatic(d);
    }
    // ---- the carved heads above the gate: a stern stone face, and under its chin a tablet with a creature;
    // three frames (snake, bird, whale)
    {
    Mesh& m = im[IN_PLAQUE];
    r.MeshInit(m, 220, 340, 3);
    for (fr = 0; fr < 3; fr++)
        {
        r.iMeshCursor = 0;
        r.MeshEllipsoid(m, fr, 0, 0, Cm(118), Cm(-8), Cm(36), Cm(42), Cm(20), 8, 3);     // the head
        BX(r, m, fr, 0, -34, 132, -4, 34, 144, 16);                                      // the brow
        BX(r, m, fr, 0, -7, 100, 4, 7, 130, 20);                                         // the nose
        BX(r, m, fr, 0, -40, 92, -10, 40, 100, 8);                                       // the jaw's ledge
        BX(r, m, fr, 1, -32, 0, -10, 32, 92, 2);                                         // the tablet (beard)
        Glyph(r, m, fr, 2, fr, 46, 4, 62);
        }
    Finish(r, m);
    for (fr = 0; fr < 3; fr++)
        {
        DefM(d, &m, 0, 0, 0, 0);
        d.frame = (u8)fr;
        Col(d, 0, 150, 118, 78, SL_TEX | TXI_NORDIC); Col(d, 1, 70, 56, 40, 1); Col(d, 2, 214, 196, 150, 1);
        int t = r.RegisterStatic(d);
        if (fr == 0) iInType[IN_PLAQUE] = t;
        }
    }
    // ---- a wall coffin: an upright stone lid in its niche, carved, facing +z
    {
    Mesh& m = im[IN_WALLCOFFIN];
    r.MeshInit(m, 80, 140, 1);
    BX(r, m, 0, 0, -60, 0, -30, -46, 230, 10);
    BX(r, m, 0, 0, 46, 0, -30, 60, 230, 10);
    BX(r, m, 0, 0, -60, 230, -30, 60, 250, 10);
    BX(r, m, 0, 1, -46, 0, -30, 46, 220, -6);
    BX(r, m, 0, 2, -30, 120, -6, 30, 200, -3);
    BX(r, m, 0, 2, -8, 30, -6, 8, 120, -3);
    Finish(r, m);
    DefM(d, &m, 60, 30, 0, 250);
    Col(d, 0, 118, 112, 100, SL_TEX | TXI_NORDIC); Col(d, 1, 64, 62, 58, SL_TEX | TXI_ROCK); Col(d, 2, 92, 88, 80, 1);
    iInType[IN_WALLCOFFIN] = r.RegisterStatic(d);
    }
    // ---- the spiral stairs: a low round stone kerb about a dark well, the first steps going down
    {
    Mesh& m = im[IN_SPIRAL];
    r.MeshInit(m, 360, 520, 1);
    for (i = 0; i < 8; i++)
        {
        int a = i * 128, x = (r3d_sin(a) * 120) >> 14, z = (r3d_cos(a) * 120) >> 14;
        YBox(r, m, 0, 0, x, 0, z, a + 256, 48, 60, 14);
        }
    for (i = 0; i < 4; i++)
        {
        int a = i * 160, x = (r3d_sin(a) * 60) >> 14, z = (r3d_cos(a) * 60) >> 14;
        YBox(r, m, 0, 1, x, -20 - i * 25, z, a, 40, 20, 18);
        }
    BX(r, m, 0, 2, -100, -2, -100, 100, 0, 100);
    Rod(r, m, 0, 0, 0, -120, 0, 0, 240, 0, 14, 6);
    r.MeshFinish(m);
    DefM(d, &m, 0, 0, 130, 60);
    Col(d, 0, 112, 106, 94, SL_TEX | TXI_NORDIC); Col(d, 1, 96, 92, 84, SL_TEX | TXI_NORDIC); Col(d, 2, 14, 12, 12, 1);
    iInType[IN_SPIRAL] = r.RegisterStatic(d);
    }
    // ---- a waterfall: falling sheets (four frames that move down), its pool; 7 m tall
    {
    Mesh& m = im[IN_WATERFALL];
    r.MeshInit(m, 120, 200, 4);
    for (fr = 0; fr < 4; fr++)
        {
        r.iMeshCursor = 0;
        for (i = 0; i < 5; i++)
            {
            int x = -80 + i * 40, y0 = ((fr * 40 + i * 70) % 160);
            BX(r, m, fr, i & 1, x - 14, 0, -6, x + 14, 700, 6);
            BX(r, m, fr, 2, x - 10, 700 - y0 - 120, 6, x + 10, 700 - y0, 8);
            }
        r.MeshEllipsoid(m, fr, 1, 0, 0, Cm(80), Cm(150), Cm(12), Cm(110), 8, 2);
        }
    r.MeshFinish(m);
    DefM(d, &m, 0, 0, 0, 0);
    d.flags = SD_ANIM | SD_UNLIT; d.animMs = 140;
    Col(d, 0, 176, 196, 206, 1); Col(d, 1, 150, 178, 192, 1); Col(d, 2, 226, 236, 240, 1);
    iInType[IN_WATERFALL] = r.RegisterStatic(d);
    }
    // ---- rubble: fallen blocks and stones
    {
    Mesh& m = im[IN_RUBBLE];
    r.MeshInit(m, 120, 200, 1);
    YBox(r, m, 0, 0, -40, 0, 10, 80, 50, 30, 40);
    YBox(r, m, 0, 0, 50, 0, -20, 300, 40, 26, 30);
    YBox(r, m, 0, 0, 10, 30, 20, 500, 30, 22, 26);
    r.MeshEllipsoid(m, 0, 1, Cm(-70), Cm(10), Cm(-50), Cm(20), Cm(14), Cm(18), 5, 2);
    r.MeshEllipsoid(m, 0, 1, Cm(80), Cm(8), Cm(40), Cm(16), Cm(12), Cm(14), 5, 2);
    Finish(r, m);
    DefM(d, &m, 0, 0, 70, 50);
    Col(d, 0, 114, 108, 96, SL_TEX | TXI_NORDIC); Col(d, 1, 96, 92, 86, SL_TEX | TXI_ROCK);
    iInType[IN_RUBBLE] = r.RegisterStatic(d);
    }
    // ---- bones: a skull, ribs and a long bone
    {
    Mesh& m = im[IN_BONES];
    r.MeshInit(m, 120, 200, 1);
    r.MeshEllipsoid(m, 0, 0, 0, Cm(10), 0, Cm(10), Cm(9), Cm(12), 6, 3);
    Rod(r, m, 0, 0, 20, 4, 30, 50, 4, -20, 3, 4);
    for (i = 0; i < 3; i++) Rod(r, m, 0, 0, 30 + i * 10, 2, 8, -18 + i * 6, 14, 12, 2, 3);
    Rod(r, m, 0, 0, -30, 2, 30, 14, 2, -50, 3, 4);
    r.MeshFinish(m);
    DefM(d, &m, 0, 0, 0, 0);
    Col(d, 0, 206, 196, 168, 2);
    iInType[IN_BONES] = r.RegisterStatic(d);
    }
    // ---- a chopping block and logs
    {
    Mesh& m = im[IN_LOGS];
    r.MeshInit(m, 100, 160, 1);
    r.MeshLimb(m, 0, 0, 0, 0, 0, 0, Cm(55), 0, Cm(32), Cm(32), Cm(30), Cm(30), 8, 2);
    Rod(r, m, 0, 1, 70, 12, -60, 70, 12, 60, 12, 6);
    Rod(r, m, 0, 1, 96, 12, -55, 96, 12, 55, 12, 6);
    Rod(r, m, 0, 1, 83, 32, -50, 83, 32, 50, 11, 6);
    Rod(r, m, 0, 2, -10, 55, 0, 20, 90, 10, 3, 4);
    r.MeshFinish(m);
    DefM(d, &m, 0, 0, 40, 55);
    Col(d, 0, 120, 90, 60, SL_TEX | TXI_LOGS); Col(d, 1, 110, 82, 54, SL_TEX | TXI_LOGS); Col(d, 2, 90, 70, 50, 4);
    iInType[IN_LOGS] = r.RegisterStatic(d);
    }
    // ---- an iron gate (portcullis), 240 cm tall: frame 0 150 cm wide, frame 1 360 cm wide
    {
    Mesh& m = im[IN_GATE];
    r.MeshInit(m, 160, 280, 2);
    for (fr = 0; fr < 2; fr++)
        {
        r.iMeshCursor = 0;
        int w = fr ? 180 : 75;
        for (i = 0; i < 7; i++)
            {
            int x = -w + 10 + (2 * w - 20) * i / 6;
            Rod(r, m, fr, 0, x, 0, 0, x, 250, 0, 3, 4);
            }
        BX(r, m, fr, 0, -w, 60, -3, w, 68, 3);
        BX(r, m, fr, 0, -w, 160, -3, w, 168, 3);
        BX(r, m, fr, 0, -w, 236, -4, w, 248, 4);
        }
    r.MeshFinish(m);
    DefM(d, &m, 0, 0, 0, 0);
    Col(d, 0, 56, 54, 52, 1);
    iInType[IN_GATE] = r.RegisterStatic(d);
    }
    // ---- a swinging blade: from a pivot 260 cm up, a long arm and a crescent blade at its foot (swings about x)
    {
    Mesh& m = im[IN_BLADE];
    r.MeshInit(m, 80, 140, 1);
    Rod(r, m, 0, 0, 0, 0, 0, 0, -200, 0, 4, 4);
    int q[12] = { 0, Cm(-180), Cm(-70),  0, Cm(-230), Cm(-40),  0, Cm(-230), Cm(40),  0, Cm(-180), Cm(70) };
    int k;
    for (k = 0; k < 2; k++)
        {
        int q2[12]; int j;
        for (j = 0; j < 12; j++) q2[j] = q[j];
        q2[0] = q2[3] = q2[6] = q2[9] = Cm(k ? 2 : -2);
        r.MeshQuad(m, 0, 1, q2, Cm(k ? -20 : 20), Cm(-200), 0);
        }
    BX(r, m, 0, 0, -6, -200, -10, 6, -186, 10);
    r.MeshFinish(m);
    DefM(d, &m, 0, 0, 0, 0);
    Col(d, 0, 60, 56, 52, 1); Col(d, 1, 168, 168, 164, 1);
    iInType[IN_BLADE] = r.RegisterStatic(d);
    }
    // ---- a spike wall: a spiked iron grate, hinged at its local x = 0 edge, 150 x 220, spikes towards +z
    {
    Mesh& m = im[IN_SPIKEWALL];
    r.MeshInit(m, 220, 380, 1);
    BX(r, m, 0, 0, 0, 0, -6, 150, 220, 0);
    for (i = 0; i < 12; i++)
        {
        int x = 20 + (i % 4) * 36, y = 30 + (i / 4) * 70;
        r.MeshLimb(m, 0, 1, Cm(x), Cm(y), 0, Cm(x), Cm(y), Cm(40), Cm(5), Cm(5), Cm(1), Cm(1), 4, 1);
        }
    r.MeshFinish(m);
    DefM(d, &m, 0, 0, 0, 0);
    Col(d, 0, 70, 64, 58, SL_TEX | TXI_NORDIC); Col(d, 1, 120, 112, 104, 1);
    iInType[IN_SPIKEWALL] = r.RegisterStatic(d);
    }
    // ---- a puzzle pillar: a three-sided stone pillar on a round foot, a creature on each face (snake, eagle,
    // whale at yaw 0, 341, 683); turned with E. Face 0 looks +z.
    {
    // a three-sided stone stele on a round carved foot; a creature on each face (snake, bird, whale on the
    // faces looking yaw 0, 341, 683); turned a face at a time with E. Face 0 looks +z.
    Mesh& m = im[IN_PUZPILLAR];
    r.MeshInit(m, 260, 420, 1);
    r.MeshLimb(m, 0, 0, 0, 0, 0, 0, Cm(22), 0, Cm(54), Cm(54), Cm(50), Cm(50), 10, 2);     // the foot
    static const int A = 24, R = 48;                                                        // apothem, corner radius
    int cx[3], cz[3];
    for (i = 0; i < 3; i++) { int a = 170 + i * 341; cx[i] = (r3d_sin(a) * R) >> 14; cz[i] = (r3d_cos(a) * R) >> 14; }
    for (i = 0; i < 3; i++)
        {
        // face i lies between corner i-1 and corner i (face 0 between the corners at -170 and +170)
        int a0 = (i + 2) % 3, a1 = i;
        int q[12] = { Cm(cx[a0]), Cm(22), Cm(cz[a0]),  Cm(cx[a1]), Cm(22), Cm(cz[a1]),  Cm(cx[a1] * 3 / 4), Cm(196), Cm(cz[a1] * 3 / 4),  Cm(cx[a0] * 3 / 4), Cm(196), Cm(cz[a0] * 3 / 4) };
        r.MeshQuad(m, 0, 1, q, 0, Cm(110), 0);
        }
    {
    // the rounded crown: the three faces meet over a low point
    int k;
    for (k = 0; k < 3; k++)
        {
        int a0 = (k + 2) % 3, a1 = k;
        int q[12] = { Cm(cx[a0] * 3 / 4), Cm(196), Cm(cz[a0] * 3 / 4),  Cm(cx[a1] * 3 / 4), Cm(196), Cm(cz[a1] * 3 / 4),  0, Cm(214), 0,  0, Cm(214), 0 };
        r.MeshQuad(m, 0, 1, q, 0, Cm(150), 0);
        }
    }
    // the creatures: built facing +z just proud of face 0 (which leans in a little: set at its height), then turned
    (void)A;
    for (i = 0; i < 3; i++)
        {
        int a = i * 341, k;
        int nv0 = m.nv;
        Glyph(r, m, 0, 2, i, 104, 22, 50);
        int cs = r3d_cos(a), sn = r3d_sin(a);
        for (k = nv0; k < m.nv; k++)
            {
            short* v = m.v + k * 3;
            int x = v[0], z = v[2];
            v[0] = (short)((x * cs + z * sn) >> 14); v[2] = (short)((-x * sn + z * cs) >> 14);
            }
        }
    Finish(r, m);
    DefM(d, &m, 0, 0, 0, 0);
    Col(d, 0, 150, 118, 78, SL_TEX | TXI_NORDIC); Col(d, 1, 92, 72, 50, 1); Col(d, 2, 222, 204, 160, 1);
    iInType[IN_PUZPILLAR] = r.RegisterStatic(d);
    }
    // ---- the claw door: a great round carved stone in an arched frame (facing +z), 320 cm tall
    {
    Mesh& m = im[IN_CLAWDOOR];
    r.MeshInit(m, 460, 640, 1);
    BX(r, m, 0, 0, -170, 0, -20, -130, 330, 10);
    BX(r, m, 0, 0, 130, 0, -20, 170, 330, 10);
    BX(r, m, 0, 0, -170, 300, -20, 170, 340, 10);
    BX(r, m, 0, 1, -130, 0, -14, 130, 300, 0);
    for (i = 0; i < 12; i++)
        {
        int a = i * 85, x = (r3d_sin(a) * 118) >> 14, y = 150 + ((r3d_cos(a) * 118) >> 14);
        YBox(r, m, 0, 2, x, y - 10, 2, 0, 12, 20, 3);
        }
    r.MeshEllipsoid(m, 0, 2, 0, Cm(150), Cm(2), Cm(20), Cm(20), Cm(4), 8, 2);
    Finish(r, m);
    DefM(d, &m, 0, 0, 0, 0);
    Col(d, 0, 104, 98, 88, SL_TEX | TXI_NORDIC); Col(d, 1, 92, 84, 72, SL_TEX | TXI_NORDIC); Col(d, 2, 70, 64, 56, 1);
    iInType[IN_CLAWDOOR] = r.RegisterStatic(d);
    }
    // ---- the claw door's rings: a band and the creature at its top. Frame ring * 3 + creature (bear, moth, owl)
    {
    Mesh& m = im[IN_CLAWRING];
    r.MeshInit(m, 200, 300, 9);
    for (fr = 0; fr < 9; fr++)
        {
        r.iMeshCursor = 0;
        int ring = fr / 3, an = fr % 3, rad = 105 - ring * 28;
        for (i = 0; i < 8; i++)
            {
            int a0 = i * 128, a1 = a0 + 128;
            Rod(r, m, fr, 0, (r3d_sin(a0) * rad) >> 14, 150 + ((r3d_cos(a0) * rad) >> 14), 4, (r3d_sin(a1) * rad) >> 14, 150 + ((r3d_cos(a1) * rad) >> 14), 4, 5, 3);
            }
        Glyph(r, m, fr, 1, 3 + an, 150 + rad - 4, 6, 30 - ring * 4);
        }
    Finish(r, m);
    DefM(d, &m, 0, 0, 0, 0);
    Col(d, 0, 132, 120, 96, 1); Col(d, 1, 214, 190, 120, 1);
    iInType[IN_CLAWRING] = r.RegisterStatic(d);
    }
    // ---- the word wall: a curved wall of standing slabs (concave towards +z), runes in rows, a dragon's head
    // carved over it; 560 cm wide, 470 tall
    {
    Mesh& m = im[IN_WORDWALL];
    r.MeshInit(m, 500, 760, 1);
    for (i = 0; i < 5; i++)
        {
        int a = (i - 2) * 70, x = (r3d_sin(a) * 300) >> 14, z = -((r3d_cos(a) * 300) >> 14) + 300;
        YBox(r, m, 0, 0, x, 0, z - 20, -a, 58, 380 - Abs(i - 2) * 40, 20);
        int row;
        for (row = 0; row < 2; row++)
            {
            YBox(r, m, 0, 1, x, 140 + row * 70, z + 2, -a, 44 - (row & 1) * 8, 8, 2);
            }
        }
    YBox(r, m, 0, 0, 0, 0, -40, 0, 330, 30, 40);                          // the base
    r.MeshEllipsoid(m, 0, 2, 0, Cm(420), Cm(-10), Cm(60), Cm(50), Cm(30), 8, 3);   // the dragon's head
    r.MeshLimb(m, 0, 2, Cm(-40), Cm(450), Cm(-10), Cm(-110), Cm(520), Cm(-20), Cm(14), Cm(14), Cm(4), Cm(4), 5, 1);
    r.MeshLimb(m, 0, 2, Cm(40), Cm(450), Cm(-10), Cm(110), Cm(520), Cm(-20), Cm(14), Cm(14), Cm(4), Cm(4), 5, 1);
    Finish(r, m);
    DefM(d, &m, 0, 0, 0, 0);
    Col(d, 0, 132, 136, 140, SL_TEX | TXI_ROCK); Col(d, 1, 40, 44, 50, 1); Col(d, 2, 70, 72, 78, SL_TEX | TXI_ROCK);
    iInType[IN_WORDWALL] = r.RegisterStatic(d);
    }
    // ---- the Overlord's sarcophagus (facing +z along its length) and its lid
    {
    Mesh& m = im[IN_SARCOPH];
    r.MeshInit(m, 60, 100, 1);
    BX(r, m, 0, 0, -60, 0, -120, 60, 70, 120);
    BX(r, m, 0, 1, -48, 60, -108, 48, 72, 108);
    BX(r, m, 0, 0, -66, 0, -126, 66, 14, 126);
    Finish(r, m);
    DefM(d, &m, 60, 120, 0, 70);
    Col(d, 0, 104, 100, 94, SL_TEX | TXI_NORDIC); Col(d, 1, 20, 18, 16, 1);
    iInType[IN_SARCOPH] = r.RegisterStatic(d);
    Mesh& l = im[IN_SARCLID];
    r.MeshInit(l, 40, 80, 1);
    BX(r, l, 0, 0, -62, 70, -122, 62, 86, 122);
    BX(r, l, 0, 1, -30, 86, -80, 30, 94, 80);
    Finish(r, l);
    DefM(d, &l, 0, 0, 0, 0);
    Col(d, 0, 112, 108, 100, SL_TEX | TXI_NORDIC); Col(d, 1, 86, 82, 76, SL_TEX | TXI_NORDIC);
    iInType[IN_SARCLID] = r.RegisterStatic(d);
    }
    // ---- an oil lantern: a clay pot on a chain (the pot at 0, the chain up 150 cm)
    {
    Mesh& m = im[IN_LANTERN];
    r.MeshInit(m, 60, 100, 1);
    r.MeshEllipsoid(m, 0, 0, 0, 0, 0, Cm(16), Cm(18), Cm(16), 6, 3);
    Rod(r, m, 0, 1, 0, 16, 0, 0, 150, 0, 1, 3);
    r.MeshFinish(m);
    DefM(d, &m, 0, 0, 0, 0);
    Col(d, 0, 150, 96, 60, 2); Col(d, 1, 50, 48, 46, 1);
    iInType[IN_LANTERN] = r.RegisterStatic(d);
    }
    // ---- a pressure plate: a round stone with a rune, a little proud of the floor
    {
    Mesh& m = im[IN_PLATE];
    r.MeshInit(m, 60, 100, 1);
    r.MeshLimb(m, 0, 0, 0, 0, 0, 0, Cm(5), 0, Cm(40), Cm(40), Cm(38), Cm(38), 8, 2);
    BX(r, m, 0, 1, -4, 4, -24, 4, 6, 24); BX(r, m, 0, 1, -18, 4, -4, 18, 6, 4);
    r.MeshFinish(m);
    DefM(d, &m, 0, 0, 0, 0);
    Col(d, 0, 132, 124, 108, SL_TEX | TXI_NORDIC); Col(d, 1, 60, 56, 50, 1);
    iInType[IN_PLATE] = r.RegisterStatic(d);
    }
    // ---- a pull chain: a bracket on the wall, the chain and its ring (frame 1: pulled down)
    {
    Mesh& m = im[IN_CHAIN];
    r.MeshInit(m, 80, 140, 2);
    for (fr = 0; fr < 2; fr++)
        {
        r.iMeshCursor = 0;
        int dn = fr ? 30 : 0;
        BX(r, m, fr, 0, -8, 200, -2, 8, 216, 12);
        Rod(r, m, fr, 0, 0, 205, 8, 0, 110 - dn, 8, 2, 3);
        r.MeshLimb(m, fr, 0, 0, Cm(100 - dn), Cm(8), 0, Cm(80 - dn), Cm(8), Cm(10), Cm(3), Cm(10), Cm(3), 6, 2);
        }
    r.MeshFinish(m);
    DefM(d, &m, 0, 0, 0, 0);
    Col(d, 0, 74, 70, 66, 1);
    iInType[IN_CHAIN] = r.RegisterStatic(d);
    }
    // ---- a pool of oil on the floor (160 cm across at scale 256)
    {
    Mesh& m = im[IN_OIL];
    r.MeshInit(m, 30, 60, 1);
    r.MeshLimb(m, 0, 0, 0, 0, 0, 0, Cm(2), 0, Cm(80), Cm(70), Cm(80), Cm(70), 10, 2);
    r.MeshFinish(m);
    DefM(d, &m, 0, 0, 0, 0);
    Col(d, 0, 28, 26, 22, 1);
    iInType[IN_OIL] = r.RegisterStatic(d);
    }
    // ---- a dart
    {
    Mesh& m = im[IN_DART];
    r.MeshInit(m, 12, 24, 1);
    r.MeshLimb(m, 0, 0, 0, 0, Cm(-12), 0, 0, Cm(12), Cm(1), Cm(1), Cm(1), Cm(1), 3, 1);
    r.MeshFinish(m);
    DefM(d, &m, 0, 0, 0, 0);
    Col(d, 0, 40, 36, 30, 1);
    iInType[IN_DART] = r.RegisterStatic(d);
    }
    BuildSpider(iMSpider);
    if (!iBfbKit) BuildDungeons();          // (v0.4.55: with Skyrim's own barrow, not the game's built one)
    }

// the frostbite spider: a pale bristling abdomen with dark bands, a smaller head-body with fangs, eight legs
// in two joints. Frames as the others: 0..15 walking, 16..23 striking, 24 dead (legs curled)
void Game::BuildSpider(Mesh& m)
    {
    R3D& r = *iR;
    r.MeshInit(m, 260, 480, 25);
    int fr;
    for (fr = 0; fr < 25; fr++)
        {
        r.iMeshCursor = 0;
        int dead = fr == 24, atk = fr >= 16 && fr < 24;
        int ph = (fr & 15) * 64;
        int lift = dead ? -40 : 0, rear = atk ? ((r3d_sin((fr - 16) * 128) * 30) >> 14) : 0;
        r.MeshEllipsoid(m, fr, 0, 0, Cm(80 + lift + rear / 2), Cm(-70), Cm(62), Cm(52), Cm(78), 7, 4);          // abdomen
        r.MeshEllipsoid(m, fr, 1, 0, Cm(96 + lift + rear / 2), Cm(-70), Cm(40), Cm(20), Cm(60), 6, 2);          // its band
        r.MeshEllipsoid(m, fr, 2, 0, Cm(66 + lift + rear), Cm(30), Cm(36), Cm(30), Cm(40), 6, 3);               // head-body
        r.MeshLimb(m, fr, 3, Cm(-12), Cm(60 + lift + rear), Cm(62), Cm(-10), Cm(36 + lift + rear), Cm(76), Cm(5), Cm(5), Cm(1), Cm(1), 4, 1);   // fangs
        r.MeshLimb(m, fr, 3, Cm(12), Cm(60 + lift + rear), Cm(62), Cm(10), Cm(36 + lift + rear), Cm(76), Cm(5), Cm(5), Cm(1), Cm(1), 4, 1);
        int k;
        for (k = 0; k < 8; k++)
            {
            int side = k < 4 ? -1 : 1, n = k & 3;
            int hz = 50 - n * 22;                                   // where it joins the body
            int swing = dead ? 0 : ((r3d_sin(ph + (n & 1) * 512 + (side > 0 ? 512 : 0)) * 22) >> 14);
            if (atk && n == 0) swing = -30;
            int kx = side * 110, ky = 120 + lift, kz = hz + 30 - n * 20 + swing;      // the knee, high
            int fx = side * (dead ? 60 : 170), fy = dead ? 40 : 0, fz = hz + 40 - n * 34 + swing * 2;
            if (atk && n == 0) { fy = 70 + rear; fz = hz + 90; }
            if (dead) { kx = side * 70; ky = 70; }
            r.MeshLimb(m, fr, 4, Cm(side * 26), Cm(70 + lift + rear / 2), Cm(hz), Cm(kx), Cm(ky), Cm(kz), Cm(7), Cm(7), Cm(5), Cm(5), 4, 0);
            r.MeshLimb(m, fr, 4, Cm(kx), Cm(ky), Cm(kz), Cm(fx), Cm(fy), Cm(fz), Cm(5), Cm(5), Cm(2), Cm(2), 4, 1);
            }
        }
    r.MeshFinish(m);
    }

// ---------------------------------------------------------------- the barrow on its mountain
// Bleak Falls Barrow's outside is Skyrim's own pieces (docs/maps/gen_kit.py's BleakFallsOutside, from the area's
// file, placed with Riverwood's: game_kit.cpp); here its doors, Bleak Falls Tower and the hidden way out of the
// sanctum, its bandits and the tower's chest (docs/maps/gen_bfb_ext.py -> bfb_ext.inc). Positions in centimetres
// from the main door.
#include "bfb_ext.inc"
static int BfOX() { return BFE_DOOR_XM * METER + BFE_DOOR_XCM * METER / 100; }
static int BfOZ() { return BFE_DOOR_ZM * METER + BFE_DOOR_ZCM * METER / 100; }
static int BfX(int xcm) { return BfOX() + Cm(xcm); }
void GameBfDoor(int* aX, int* aZ) { *aX = BfOX(); *aZ = BfOZ(); }
static int BfZ(int zcm) { return BfOZ() + Cm(zcm); }
// a point turned with a heading: aYaw's forward is local +z
static int TurnX(int aLx, int aLz, int aYaw) { return (aLx * r3d_cos(aYaw) + aLz * r3d_sin(aYaw)) >> 14; }
static int TurnZ(int aLx, int aLz, int aYaw) { return (-aLx * r3d_sin(aYaw) + aLz * r3d_cos(aYaw)) >> 14; }

// the barrow's doors in the door table (game_cells.cpp): in from the terrace, the spiral stairs, up to the
// sanctum, out onto the mountain
static int KU(int aUnits) { return aUnits * 143 / 100; }
void GameBarrowDoors(DoorLink* d, int aKit)
    {
    if (aKit)
        {
        // (v0.4.55) Skyrim's own barrow (data/bleakfalls.har): its doors where Skyrim has them (the bleakfalls export's
        // doors.csv; Skyrim units in each cell's frame; facing: where Skyrim puts you through them)
        d[0].cell = CELL_BFTEMPLE; d[0].ocell = 0;
        d[0].ox = BfOX() + (r3d_sin(BFE_DOOR_YAW) * 3 * M / 4 >> 14); d[0].oz = BfOZ() + (r3d_cos(BFE_DOOR_YAW) * 3 * M / 4 >> 14);
        d[0].oyaw = BFE_DOOR_YAW;
        d[0].ix = Ix(KU(-7413)); d[0].iz = Iz(KU(979)); d[0].iyaw = 256;           // NorLargeDoor 00037603 (1.2 m short of where Skyrim puts you)
        d[0].name = "Bleak Falls Barrow";
        d[0].hk = 3; d[0].oy = Cm(BFE_DOOR_H); d[0].iy = IN_FLOOR + Cm(KU(573));
        d[1].name = 0;                                                            // (no crypt of its own: the temple has it all)
        d[2].cell = CELL_BFSANCTUM; d[2].ocell = CELL_BFTEMPLE;                   // NorDoorMediumLoad01 00038DFB <-> 00037B0C
        d[2].ox = Ix(KU(1079)); d[2].oz = Iz(KU(-11152)); d[2].oyaw = 0;
        d[2].ix = Ix(KU(7173)); d[2].iz = Iz(KU(1166)); d[2].iyaw = 0;
        d[2].name = "Bleak Falls Sanctum";
        d[2].hk = 3; d[2].oy = IN_FLOOR + Cm(KU(-2675)); d[2].iy = IN_FLOOR + Cm(KU(-627));
        d[3].cell = CELL_BFSANCTUM; d[3].ocell = 0;                               // AutoLoadDoor01 00031898: the way out
        d[3].ox = BfX(BFE_OUT_X); d[3].oz = BfZ(BFE_OUT_Z); d[3].oyaw = BFE_OUT_YAW;
        d[3].ix = Ix(KU(-13568)); d[3].iz = Iz(KU(-470)); d[3].iyaw = 256;
        d[3].name = "Skyrim";
        d[3].hk = 3; d[3].oy = Cm(BFE_DOOR_H + BFE_OUT_H); d[3].iy = IN_FLOOR + Cm(KU(1688));
        return;
        }
    d[0].cell = CELL_BFTEMPLE; d[0].ocell = 0;
    d[0].ox = BfOX() + (r3d_sin(BFE_DOOR_YAW) * 3 * M / 4 >> 14); d[0].oz = BfOZ() + (r3d_cos(BFE_DOOR_YAW) * 3 * M / 4 >> 14);
    d[0].oyaw = BFE_DOOR_YAW;
    d[0].ix = Ix(BFD_IN_X); d[0].iz = Iz(BFD_IN_Z); d[0].iyaw = BFD_IN_YAW;
    d[0].name = "Bleak Falls Barrow";
    d[0].hk = 2; d[0].oy = Cm(BFE_DOOR_H);                     // out onto the terrace (Skyrim's platform at the sill)
    d[1].cell = CELL_BFCRYPT; d[1].ocell = CELL_BFTEMPLE;
    d[1].ox = Ix(BFD_DOWN_X); d[1].oz = Iz(BFD_DOWN_Z); d[1].oyaw = BFD_DOWN_YAW;
    d[1].ix = Ix(BFD_CRYPT_X); d[1].iz = Iz(BFD_CRYPT_Z); d[1].iyaw = BFD_CRYPT_YAW;
    d[1].name = "Bleak Falls Temple";
    d[2].cell = CELL_BFSANCTUM; d[2].ocell = CELL_BFCRYPT;
    d[2].ox = Ix(BFD_UP_X); d[2].oz = Iz(BFD_UP_Z); d[2].oyaw = BFD_UP_YAW;
    d[2].ix = Ix(BFD_SANC_X); d[2].iz = Iz(BFD_SANC_Z); d[2].iyaw = BFD_SANC_YAW;
    d[2].name = "Bleak Falls Sanctum";
    d[3].cell = CELL_BFSANCTUM; d[3].ocell = 0;
    d[3].ox = BfX(BFE_OUT_X); d[3].oz = BfZ(BFE_OUT_Z); d[3].oyaw = BFE_OUT_YAW;
    d[3].ix = Ix(BFD_OUT_X); d[3].iz = Iz(BFD_OUT_Z); d[3].iyaw = BFD_OUT_YAW;
    d[3].name = "Skyrim";
    d[3].hk = 2; d[3].oy = Cm(BFE_DOOR_H + BFE_OUT_H);
    }
// (v0.4.68) Forelhost's doors (the forelhost export's doors.csv: where Skyrim puts you through each, Skyrim units - the
// open world's, or each hall's own frame; facing as Skyrim turns you): the main door on the mountain, the Stronghold
// down to the Crypt, the Crypt up to the Refectory, the Refectory's great door out onto the ruin's top
static int FhWX(int aX) { return (int)(((long long)aX + 233472) * 36576 / 10000); }
static int FhWZ(int aY) { return (int)(((long long)aY + 176128) * 36576 / 10000); }
static int FhY(int aZ) { return Cm(KU(aZ)); }
static int FhYaw(int aDeg) { return (aDeg * 1024 / 360) & 1023; }
void GameForelhostDoors(DoorLink* d)
    {
    d[0].cell = CELL_FHSTRONG; d[0].ocell = 0;                                // NorDoorSmLoad02 00015DCB <-> NorDoorSmLoad01 00022F66
    d[0].ox = FhWX(182971); d[0].oz = FhWZ(-113764); d[0].oyaw = FhYaw(140);
    d[0].ix = Ix(KU(513)); d[0].iz = Iz(KU(-1337)); d[0].iyaw = 0;
    d[0].name = "Forelhost";
    d[0].hk = 3; d[0].oy = FhY(17862); d[0].iy = IN_FLOOR + FhY(0);
    d[1].cell = CELL_FHCRYPT; d[1].ocell = CELL_FHSTRONG;                     // NorDoorMediumLoad01 0002319E <-> 00023048
    d[1].ox = Ix(KU(-1152)); d[1].oz = Iz(KU(3136)); d[1].oyaw = 512;
    d[1].ix = Ix(KU(-1028)); d[1].iz = Iz(KU(3254)); d[1].iyaw = 0;
    d[1].name = "Forelhost Crypt";
    d[1].hk = 3; d[1].oy = IN_FLOOR + FhY(-256); d[1].iy = IN_FLOOR + FhY(-512);
    d[2].cell = CELL_FHREFECT; d[2].ocell = CELL_FHCRYPT;                     // NorDoorSmLoad02 00023240 <-> 00023049
    d[2].ox = Ix(KU(3008)); d[2].oz = Iz(KU(7552)); d[2].oyaw = 256;
    d[2].ix = Ix(KU(2624)); d[2].iz = Iz(KU(8192)); d[2].iyaw = 768;
    d[2].name = "Forelhost Refectory";
    d[2].hk = 3; d[2].oy = IN_FLOOR + FhY(-256); d[2].iy = IN_FLOOR + FhY(256);
    d[3].cell = CELL_FHREFECT; d[3].ocell = 0;                                // NorLargeDoor 0002417A <-> 0002417B: the top
    d[3].ox = FhWX(181656); d[3].oz = FhWZ(-112153); d[3].oyaw = FhYaw(140);
    d[3].ix = Ix(KU(1280)); d[3].iz = Iz(KU(-960)); d[3].iyaw = 0;
    d[3].name = "Skyrim";
    d[3].hk = 3; d[3].oy = FhY(18726); d[3].iy = IN_FLOOR + FhY(2240);
    }
int GameBarrowX() { return BfOX(); }
int GameBarrowSillY(const Game& g) { return Cm(BFE_DOOR_H) + (g.iR->iLandLoaded ? g.iR->iLandOffset : 0); }
int GameBarrowZ() { return BfOZ(); }
// a point a metres to the right and b metres out from the barrow's main door
int GameBarrowOutX(int a, int b) { return BfOX() + TurnX(a * METER, b * METER, BFE_DOOR_YAW); }
int GameBarrowOutZ(int a, int b) { return BfOZ() + TurnZ(a * METER, b * METER, BFE_DOOR_YAW); }
int GameBarrowDoorYaw() { return BFE_DOOR_YAW; }
int GameTowerX() { return BfX(BFE_TOWER_X); }
int GameTowerZ() { return BfZ(BFE_TOWER_Z); }

// ---------------------------------------------------------------- furnishing the cells
static void DoorPair(Game& g, int c, int x, int z, int yawIn)
    {
    // closed double doors in the wall behind the door point (yawIn: the way into the room)
    int wx = x - ((r3d_sin(yawIn) * 55) >> 14), wz = z - ((r3d_cos(yawIn) * 55) >> 14);
    if (yawIn == 256 || yawIn == 768) { g.IPlace(c, IN_DOOR, wx, 0, wz - 108, 0, 256); g.IPlace(c, IN_DOOR, wx, 0, wz + 108, 512, 256); }
    else { g.IPlace(c, IN_DOOR, wx - 108, 0, wz, 256, 256); g.IPlace(c, IN_DOOR, wx + 108, 0, wz, 768, 256); }
    }
static void DoorPairY(Game& g, int c, int x, int y, int z, int yawIn)
    {
    int wx = x - ((r3d_sin(yawIn) * 55) >> 14), wz = z - ((r3d_cos(yawIn) * 55) >> 14);
    if (yawIn == 256 || yawIn == 768) { g.IPlace(c, IN_DOOR, wx, y, wz - 108, 0, 256); g.IPlace(c, IN_DOOR, wx, y, wz + 108, 512, 256); }
    else { g.IPlace(c, IN_DOOR, wx - 108, y, wz, 256, 256); g.IPlace(c, IN_DOOR, wx + 108, y, wz, 768, 256); }
    }

static void PlaceDecor(Game& g, int c, const DunData& d)
    {
    R3D& r = *g.iR;
    int k;
    for (k = 0; k < d.nput; k++)
        {
        const int* p = d.put + k * 6;
        int x = p[1], y = p[2], z = p[3], yaw = p[4], sc = p[5];
        switch (p[0])
            {
            case D_BRAZIER: g.IPlace(c, IN_BRAZIER, x, y, z, 0, 256); g.IPlace(c, IN_FLAME, x, y + 96, z, 0, 300); break;
            case D_TORCH:
                g.IPlace(c, IN_WALLTORCH, x, y, z, yaw, 256);
                g.IPlace(c, IN_FLAME, x + ((r3d_sin(yaw) * 24) >> 14), y + 190, z + ((r3d_cos(yaw) * 24) >> 14), 0, 110);
                break;
            case D_FIRE: g.IPlace(c, IN_FIREPIT, x, y, z, 0, 256); g.IPlace(c, IN_FLAME, x, y + 22, z, 0, 560); break;
            case D_BEDROLL: g.IPlace(c, IN_PELTBED, x, y, z, yaw, 256); break;
            case D_URNS: g.IPlace(c, IN_URNS, x, y, z, yaw, 256); break;
            case D_COFFIN: g.IPlace(c, IN_COFFIN, x, y, z, yaw, 256); break;
            case D_PILLAR: g.IPlace(c, IN_NORCOL, x, y, z, yaw, sc); break;
            case D_ARCH: g.IPlace(c, IN_ARCH, x, y, z, yaw, sc); break;
            case D_MURAL: g.IPlace(c, IN_MURAL, x, y, z, yaw, 256); break;
            case D_TABLE: g.IPlace(c, IN_TABLE, x, y, z, yaw, 256); break;
            case D_WEB: g.IPlace(c, IN_WEB, x, y, z, yaw, sc); break;
            case D_EGGS: g.IPlace(c, IN_EGGS, x, y, z, yaw, 256); break;
            case D_PLAQUE:
                // 0 snake, 1 snake (fallen, leaning on the floor), 2 whale
                r.AddInterior(c, g.iInType[IN_PLAQUE] + (sc == 2 ? 2 : 0), Ix(x), IN_FLOOR + Cm(y), Iz(z), yaw, 256, 0);
                break;
            case D_DOOR: g.IPlace(c, IN_DOOR, x - (yaw == 0 || yaw == 512 ? 54 : 0), y, z, 256, 256); break;
            case D_WALLCOFFIN: g.IPlace(c, IN_WALLCOFFIN, x, y, z, yaw, 256); break;
            case D_BONES: g.IPlace(c, IN_BONES, x, y, z, yaw, 256); break;
            case D_WATERFALL: g.IPlace(c, IN_WATERFALL, x, y, z, yaw, 256); break;
            case D_RUBBLE: g.IPlace(c, IN_RUBBLE, x, y, z, yaw, 256); break;
            case D_SPIRAL: g.IPlace(c, IN_SPIRAL, x, y, z, yaw, 256); break;
            case D_LOGS: g.IPlace(c, IN_LOGS, x, y, z, yaw, 256); break;
            }
        }
    for (k = 0; k < d.nlight; k++)
        {
        const int* l = d.light + k * 5;
        g.ILight(c, l[0], l[1], l[2], l[3], l[4]);
        }
    // the fixed parts of the traps: pressure plates, oil pools, the Overlord's sarcophagus
    for (k = 0; k < d.nfix; k++)
        {
        const int* f = d.fix + k * 7;
        if (f[0] == F_PLATE) g.IPlace(c, IN_PLATE, f[1], f[2], f[3], 0, 256);
        else if (f[0] == F_OIL) g.IPlace(c, IN_OIL, f[1], f[2] + 1, f[3], 0, f[6] * 256 / 80);
        else if (f[0] == F_COFFINLID) g.IPlace(c, IN_SARCOPH, f[1], f[2], f[3], f[4], 256);
        }
    // the doors of the cells
    if (c == CELL_BFTEMPLE) DoorPair(g, c, BFD_IN_X, BFD_IN_Z, BFD_IN_YAW);
    if (c == CELL_BFCRYPT) DoorPairY(g, c, BFD_UP_X, 300, BFD_UP_Z, BFD_UP_YAW);
    if (c == CELL_BFSANCTUM) { DoorPair(g, c, BFD_SANC_X, BFD_SANC_Z, BFD_SANC_YAW); DoorPairY(g, c, BFD_OUT_X, 250, BFD_OUT_Z, BFD_OUT_YAW); }
    }

void Game::SpawnDungeons()
    {
    int c, k;
    r3d_memset(iBfTimer, 0, sizeof(iBfTimer));
    for (k = 0; k < 32; k++) iS.bfb[k] = 0;
    iS.bfb[BF_PILLAR0] = 1; iS.bfb[BF_PILLAR1] = 2; iS.bfb[BF_PILLAR2] = 1;     // eagle, whale, eagle
    iS.bfb[BF_RING0] = 2; iS.bfb[BF_RING1] = 0; iS.bfb[BF_RING2] = 1;           // owl, bear, moth
    for (c = CELL_BFTEMPLE; c <= CELL_BFSANCTUM; c++)
        {
        const DunData* dp = DunOfK(*this, c);
        if (!dp) continue;
        const DunData& d = *dp;
        const DunProp* p;
        for (p = d.prop; p->type >= 0; p++)
            {
            int item = p->item, count = 0;
            if (p->type == PT_CHEST)
                {
                int cn = NewCont();
                switch (p->item)
                    {
                    case 1: ContAdd(cn, IT_POT_MHEAL, 1); ContAdd(cn, IT_IRON_DAGGER, 1); ContAdd(cn, IT_GARNET, 1); break;
                    case 2: ContAdd(cn, IT_SILVER_RING, 1); ContAdd(cn, IT_POT_MSTAM, 2); ContAdd(cn, IT_AMETHYST, 1); break;
                    case 3: ContAdd(cn, IT_POT_HEAL, 1); ContAdd(cn, IT_IRON_HELMET, 1); break;
                    case 4: ContAdd(cn, IT_GOLD_RING, 1); ContAdd(cn, IT_POT_MMAG, 2); break;
                    case 5: ContAdd(cn, IT_STEEL_SWORD, 1); ContAdd(cn, IT_POT_MHEAL, 2); break;
                    case 6: ContAdd(cn, IT_IRON_SHIELD, 1); ContAdd(cn, IT_GARNET, 2); break;
                    default: ContAdd(cn, IT_GOLD_NECKLACE, 1); ContAdd(cn, IT_POT_HEAL, 2); ContAdd(cn, IT_BARROW_BLADE, 1); ContAdd(cn, IT_AMETHYST, 2); break;
                    }
                item = cn;
                }
            if (p->type == PT_ITEM) count = 1;
            int y = IN_FLOOR + Cm(p->y);
            if (p->type == PT_ITEM) y += iItemRest[GameItem(p->item).mesh * 3 + 1];
            AddProp(p->type, c, Ix(p->x), y, Iz(p->z), p->yaw, item, count, 0);
            }
        }
    // (v0.4.68) Forelhost: its halls' chests, levers and claw door, the Glass Claw on its pedestal, the word wall on
    // the ruin's top (outside), its claw door's rings as Skyrim leaves them
    for (k = 0; k < FH_COUNT; k++) iS.dun[k] = 0;
    iS.dun[FH_RING0] = 1; iS.dun[FH_RING1] = 2; iS.dun[FH_RING2] = 0;          // owl, snake, fox
    for (k = 0; k < 8; k++) iFhGate[k] = 0;
    iFhClaw = 0;
    for (k = 0; k < 50; k++) iFireWall[k] = 0;
    for (c = CELL_FHSTRONG; c <= CELL_FHREFECT; c++)
        {
        const DunData* dp = DunOfK(*this, c);
        if (!dp) continue;
        const DunProp* p;
        for (p = dp->prop; p->type >= 0; p++)
            {
            int item = p->item;
            if (p->type == PT_CHEST)
                {
                int cn = NewCont();
                switch (p->item)
                    {
                    case 1: ContAdd(cn, IT_POT_MHEAL, 1); ContAdd(cn, IT_STEEL_DAGGER, 1); ContAdd(cn, IT_GARNET, 1); break;
                    case 2: ContAdd(cn, IT_GOLD_RING, 1); ContAdd(cn, IT_POT_MSTAM, 2); ContAdd(cn, IT_AMETHYST, 1); break;
                    case 3: ContAdd(cn, IT_POT_HEAL, 1); ContAdd(cn, IT_IRON_HELMET, 1); break;
                    case 4: ContAdd(cn, IT_SILVER_RING, 1); ContAdd(cn, IT_POT_MMAG, 2); break;
                    case 5: ContAdd(cn, IT_STEEL_SWORD, 1); ContAdd(cn, IT_POT_MHEAL, 2); break;
                    case 6: ContAdd(cn, IT_IRON_SHIELD, 1); ContAdd(cn, IT_GARNET, 2); break;
                    default: ContAdd(cn, IT_GOLD_NECKLACE, 1); ContAdd(cn, IT_POT_HEAL, 2); ContAdd(cn, IT_NORD_FROST, 1); ContAdd(cn, IT_AMETHYST, 2); break;
                    }
                item = cn;
                }
            AddProp(p->type, c, Ix(p->x), IN_FLOOR + Cm(p->y), Iz(p->z), p->yaw, item, 0, 0);
            }
        }
    {
    const int c = CELL_FHSTRONG + KFHClawCell;
    AddProp(PT_ITEM, c, Ix(KFHClawX), IN_FLOOR + Cm(KFHClawY) + iItemRest[GameItem(IT_GLASS_CLAW).mesh * 3 + 1], Iz(KFHClawZ), KFHClawYaw, IT_GLASS_CLAW, 1, 0);
    int wx = (int)(((long long)KFHWallX + 233472) * 36576 / 10000), wz = (int)(((long long)KFHWallY + 176128) * 36576 / 10000);
    int wy = (int)((long long)KFHWallZ * 143 / 100 * METER / 100) + (iR->iLandLoaded ? iR->iLandOffset : 0);
    AddProp(PT_WORDWALL, 0, wx, wy, wz, (KFHWallRot * 1024 / 360) & 1023, 1, 0, 0);
    }
    { void GameFhQuestProps(Game& g); GameFhQuestProps(*this); }        // (v0.4.69) the journal, the well's key, the well
    int cn = NewCont();
    ContAdd(cn, IT_POT_MHEAL, 1); ContAdd(cn, IT_FUR_GAUNTLETS, 1); ContAdd(cn, IT_LICHEN, 2);
    // Skyrim's TreasBanditChest 0001EBF7 in the tower (7580.3, -42295.4, 4891.7, turned 157.2): on the tower's floor
    {
    int x = BfX(15161), z = BfZ(1494), y = Cm(4892 * 143 / 100) + (iR->iLandLoaded ? iR->iLandOffset : 0);
    AddProp(PT_CHEST, 0, x, FloorAt(0, x, z, y + M / 2), z, 447, cn, 0, 0);
    }
    }

// (v0.4.67) a group's people and creatures (game_stage.cpp: out only while the player is there): a barrow cell's
// (its kit table: Skyrim's own), or the barrow's outside bandits
void Game::SpawnDunEnts(int aGrp)
    {
    int k;
    int c = GameGrpCell(aGrp);
    if (c)
        {
        const DunData* dp = DunOfK(*this, c);
        if (!dp) return;
        const DunData& d = *dp;
        const int* e;
        int row = 0, rowIdx[64];
        for (e = d.ent; e[0] >= 0; e += 8, row++)
            {
            int idx = SpawnEnt(e[0], Ix(e[1]), Iz(e[3]), e[4]);
            if (row < 64) rowIdx[row] = idx;
            if (idx < 0) continue;
            Ent& n = iS.e[idx];
            n.cell = (u8)c; n.y = IN_FLOOR + Cm(e[2]);
            n.var = (u8)e[5]; n.pad1 = (u8)e[6]; n.pad2 = (u8)e[7];
            if (n.type == E_WIGHT)
                {
                // the Overlord, the Restless, (v0.4.68) the ghosts, Rahgot the dragon priest, a ruin's boss
                static const short KHp[6] = { 50, 90, 260, 60, 400, 200 };
                int hp = KHp[n.pad1 < 6 ? n.pad1 : 0];
                n.hp = n.maxhp = (short)hp;
                }
            if (n.type == E_BANDIT && n.pad1 == 1) { n.hp = n.maxhp = 25; }     // Arvel the Swift
            if (n.pad2 & 2) { n.alive = 0; n.state = S_DEAD; n.hp = 0; }
            }
        // (v0.4.62) an ambusher's home: where it steps out to from its alcove or sarcophagus
        if (d.out)
            for (e = d.out; e[0] >= 0; e += 3)
                if (e[0] < row && e[0] < 64 && rowIdx[e[0]] >= 0) { Ent& n = iS.e[rowIdx[e[0]]]; n.hx = Ix(e[1]); n.hz = Iz(e[2]); }
        return;
        }
    if (aGrp != G_BFOUT) return;
    // outside: Skyrim's eight bandits, at the tower and on the terrace (KBfeBandit in Skyrim's order): what each
    // fights with - E3DemoTowerBanditMelee, E3DemoTowerBanditMissile, LvlBanditMeleeAnyAggro1024,
    // LvlBanditMissileAggro1024, LvlBanditMeleeTankAggro1024 (heavy armour), LvlBanditMissile, LvlBanditMelee2H,
    // LvlBanditMissile
    static const u8 KKind[8] = { BK_1H, BK_ARCHER, BK_1H, BK_ARCHER, BK_HEAVY, BK_ARCHER, BK_2H, BK_ARCHER };
    int nb = 0;
        for (k = 0; k < KBfeBandit_N; k++)
            {
            const int* b = KBfeBandit + k * 4;
            int idx = SpawnEnt(E_BANDIT, BfX(b[0]), BfZ(b[1]), b[3]);
            if (idx >= 0)
                {
                Ent& n = iS.e[idx];
                n.var = (u8)(nb % 5 + 3);
                SetBanditKind(n, KKind[k]);
                // standing where Skyrim has them: on the platforms, in the tower (heights from the door's sill)
                n.y = FloorAt(0, n.x, n.z, Cm(BFE_DOOR_H + b[2]) + (iR->iLandLoaded ? iR->iLandOffset : 0) + METER / 2);
                }
            nb++;
            }
    }

// ---------------------------------------------------------------- speech: a line split over the message rows
static void Say(Game& g, const char* aWho, const char* aText)
    {
    char b[200];
    int n = 0;
    while (*aWho && n < 40) b[n++] = *aWho++;
    b[n++] = ':'; b[n++] = ' ';
    while (*aText && n < 199) b[n++] = *aText++;
    b[n] = 0;
    // up to 54 characters a row, broken at spaces
    int s = 0;
    while (s < n)
        {
        int e = s + 54;
        if (e >= n) e = n;
        else { int k = e; while (k > s + 20 && b[k] != ' ') k--; if (k > s + 20) e = k; }
        char row[56]; int k;
        for (k = 0; k < e - s && k < 55; k++) row[k] = b[s + k];
        row[k] = 0;
        g.Msg(row);
        s = e;
        while (s < n && b[s] == ' ') s++;
        }
    }

// the barrow's conversations (dialogue ids 100 + DD_*): "Speaker|text" pages
enum { DD_FAR_QUEST = 0, DD_FAR_NAG, DD_FAR_RETURN, DD_FAR_EARLY, DD_ARVEL_SPIDER, DD_ARVEL_FREE,
       DD_LUCAN_SCENE, DD_LUCAN_QUEST, DD_LUCAN_RETURN, DD_LUCAN_EARLY, DD_CAM_0, DD_CAM_1, DD_CAM_2, DD_CAM_3, DD_COUNT };
#define GAME_LINES_DUN
#include "game_lines.inc"      // (v0.4.75) KDunLines: the people's lines (see game_lines_stub.inc)
static const short KDunDlg[DD_COUNT * 2] = { 0, 5,  5, 1,  6, 2,  8, 1,  9, 1,  10, 4,
                                             14, 5,  19, 6,  25, 3,  28, 6,  34, 1,  35, 1,  36, 1,  37, 1 };
const char* GameFhDlgLine(int aId, int aPage);
int GameFhDlgCount(int aId);
const char* GameDunDlgLine(int aId, int aPage)
    {
    if (aId >= DD_COUNT) return GameFhDlgLine(aId - DD_COUNT, aPage);     // (v0.4.69: Forelhost's, after the barrow's)
    if (aId < 0) aId = 0;
    int n = KDunDlg[aId * 2 + 1];
    return KDunLines[KDunDlg[aId * 2] + Clamp(aPage, 0, n - 1)];
    }
int GameDunDlgCount(int aId) { return aId >= DD_COUNT ? GameFhDlgCount(aId - DD_COUNT) : (aId >= 0 ? KDunDlg[aId * 2 + 1] : 1); }

void GameClawHome(Game& g);
// the find of the claw: its journal, the quest moves on
static void GotClaw(Game& g)
    {
    if (g.iS.bfb[BF_QCLAW] < 50)
        {
        if (g.iS.bfb[BF_QCLAW] == 0) g.Msg("Quest started: The Golden Claw");
        g.iS.bfb[BF_QCLAW] = 50;
        g.Msg("Return the Golden Claw to Lucan Valerius");
        }
    g.Msg("Its palm is carved: a bear, a moth, an owl.");
    }

void Game::DunDialogDone(int aId)
    {
    if (aId >= DD_COUNT) { FhDialogDone(aId - DD_COUNT); return; }
    switch (aId)
        {
        case DD_FAR_QUEST:
            if (iS.bfb[BF_QBLEAK] < 10)
                {
                iS.bfb[BF_QBLEAK] = 10;
                Msg("Quest started: Bleak Falls Barrow");
                Msg("Retrieve the Dragonstone");
                }
            break;
        case DD_FAR_RETURN: case DD_FAR_EARLY:
            if (CountItem(IT_DRAGONSTONE) > 0)
                {
                TakeItems(IT_DRAGONSTONE, 1);
                iS.bfb[BF_QBLEAK] = 190;
                iS.gold += 200;
                Msg("Dragonstone removed");
                Msg("200 Gold added");
                Msg("Quest completed: Bleak Falls Barrow");
                }
            break;
        case DD_LUCAN_QUEST:
            if (iS.bfb[BF_QCLAW] < 10 || iS.bfb[BF_QCLAW] == 12)
                {
                if (iS.bfb[BF_QCLAW] == 0) Msg("Quest started: The Golden Claw");
                iS.bfb[BF_QCLAW] = 10;
                Msg("Retrieve the Golden Claw from Bleak Falls Barrow");
                }
            break;
        case DD_LUCAN_RETURN: case DD_LUCAN_EARLY:
            if (CountItem(IT_GOLDEN_CLAW) > 0)
                {
                TakeItems(IT_GOLDEN_CLAW, 1);
                iS.bfb[BF_QCLAW] = 200;
                iS.gold += 400;
                Msg("Golden Claw removed");
                Msg("400 Gold added");
                Msg("Quest completed: The Golden Claw");
                GameClawHome(*this);
                }
            break;
        case DD_ARVEL_FREE:
            {
            int i;
            for (i = 0; i < iS.ne; i++)
                {
                Ent& e = iS.e[i];
                if (e.type == E_BANDIT && e.pad1 == 1 && e.alive && (e.pad2 & 16))
                    {
                    e.pad2 &= ~16; e.cool = 0;
                    iS.bfb[BF_ARVEL] = 1;
                    Say(*this, "Arvel the Swift", KSAY_0);
                    }
                }
            break;
            }
        }
    }

int Game::FarengarTalk()
    {
    int q = iS.bfb[BF_QBLEAK], has = CountItem(IT_DRAGONSTONE) > 0;
    if (q >= 190) return 0;
    if (q == 0) StartDialog(100 + (has ? DD_FAR_EARLY : DD_FAR_QUEST));
    else StartDialog(100 + (has ? DD_FAR_RETURN : DD_FAR_NAG));
    return 1;
    }

// Lucan Valerius: the quest when there is something to say, otherwise a line and his wares (returns 0: trade)
int Game::LucanTalk(int aEnt)
    {
    int q = iS.bfb[BF_QCLAW], has = CountItem(IT_GOLDEN_CLAW) > 0, opened = iS.bfb[BF_CLAWDOOR];
    iTradeEnt = aEnt;
    if (q < 200 && has && opened) { StartDialog(100 + (q < 10 || q == 12 ? DD_LUCAN_EARLY : DD_LUCAN_RETURN)); return 1; }
    if (q < 10 || q == 12) { StartDialog(100 + DD_LUCAN_QUEST); return 1; }
    if (q >= 200) Say(*this, "Lucan", KSAY_1);
    else if (has) Say(*this, "Lucan", KSAY_2);
    else Say(*this, "Lucan", KSAY_3);
    return 0;
    }

int Game::CamillaTalk()
    {
    int q = iS.bfb[BF_QCLAW];
    if (q >= 200) StartDialog(100 + DD_CAM_3);
    else if (CountItem(IT_GOLDEN_CLAW) > 0 && iS.bfb[BF_CLAWDOOR]) StartDialog(100 + DD_CAM_2);
    else if (q == 10) StartDialog(100 + DD_CAM_1);
    else StartDialog(100 + DD_CAM_0);
    return 1;
    }

// the first time in: the two of them arguing over the theft (while the claw is still gone)
void Game::RwTraderEnter()
    {
    if (iS.bfb[BF_LUCAN] || iS.bfb[BF_QCLAW] >= 200) return;
    iS.bfb[BF_LUCAN] = 1;
    StartDialog(100 + DD_LUCAN_SCENE);
    }

int Game::DunTalk(Ent& e)
    {
    if (!(e.type == E_BANDIT && e.pad1 == 1 && e.alive && (e.pad2 & 16))) return 0;
    StartDialog(100 + (iS.bfb[BF_SPIDER] == 2 ? DD_ARVEL_FREE : DD_ARVEL_SPIDER));
    return 1;
    }

// ---------------------------------------------------------------- the creatures' own ways in the barrow
static int Dist(int dx, int dz) { return r3d_isqrt((dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4)) << 4; }
static void Spit(Game& g, Ent& e, int aKind, int aDmg)
    {
    int i;
    for (i = 0; i < MAXP; i++) if (!g.iP[i].active) break;
    if (i == MAXP) return;
    Proj& p = g.iP[i];
    p.active = 1; p.owner = 1; p.kind = 0; p.pad = (u8)aKind;
    p.x = e.x; p.z = e.z; p.y = e.y + 3 * M / 2;
    int tx = g.iS.px - p.x, ty = (g.iS.py - 200) - p.y, tz = g.iS.pz - p.z;
    int L = r3d_isqrt((tx >> 4) * (tx >> 4) + (ty >> 4) * (ty >> 4) + (tz >> 4) * (tz >> 4)) << 4;
    if (L < 1) L = 1;
    int sp = 14 * M;
    p.vx = tx * (sp >> 4) / (L >> 4); p.vy = ty * (sp >> 4) / (L >> 4); p.vz = tz * (sp >> 4) / (L >> 4);
    p.life = 2500000; p.dmg = aDmg;
    }

// 1: handled here (the usual hunting is skipped this tick)
int Game::DunEntUpdate(Ent& e, int dt, int d)
    {
    int dtMs = dt / 1000;
    // asleep in a wall coffin or in the sarcophagus, or hanging in the dark above
    if (e.pad2 & 64) return 1;
    // (v0.4.62) Skyrim's ambushers: standing asleep in their alcoves, or lying in a sarcophagus out of sight (8); come
    // near and they wake, step out to where Skyrim stands them (Ent::hx, hz), then fight
    if (e.type == E_WIGHT && (e.pad2 & 40))
        {
        int coffin = e.pad2 & 8;
        if (!(d < (coffin ? 4 * M : 11 * M / 2) && e.cell == iS.cell) && !e.aggro) return 1;     // (or struck)
        e.pad2 &= ~40; iShakeUs = coffin ? 250000 : 160000;
        if (Abs(e.hx - e.x) + Abs(e.hz - e.z) > M / 3) e.pad2 |= 128; else e.aggro = 1;
        // (v0.4.62) out of the wall: a lunge, its arms swinging (the attack's frames) as it bursts out
        if (!coffin) e.atk = 900000;
        }
    if (e.pad2 & 8) return 1;
    if (e.type == E_WIGHT && (e.pad2 & 128))
        {
        if (e.atk > 0) e.atk -= dt;
        int sp = e.atk > 450000 ? 3 * M : 3 * M / 2;                     // (the first moment a lunge)
        int dx = e.hx - e.x, dz = e.hz - e.z, l = Dist(dx, dz), st = sp * dtMs / 1000 + 1;
        if (l <= st) { e.x = e.hx; e.z = e.hz; e.pad2 &= ~128; e.aggro = 1; e.atk = 0; }
        else { e.x += dx * st / l; e.z += dz * st / l; }
        e.yaw = r3d_atan2(dx, dz); e.state = S_WANDER;
        e.anim += (3 * M / 2) * dtMs * 1024 / (360 * 1000);
        e.y = FloorAt(e.cell, e.x, e.z, e.y + M / 2);
        return 1;
        }
    if (e.type == E_SPIDER && e.fly > 0)
        {
        e.fly -= 9 * M * dtMs / 1000;
        if (e.fly <= 0) { e.fly = 0; e.aggro = 1; iShakeUs = 250000; }
        return 1;
        }
    if (e.type == E_BANDIT && e.pad1 == 1)
        {
        // Arvel the Swift: stuck in the web, or running for the claw's door
        if (e.pad2 & 16) { e.yaw = r3d_atan2(iS.px - e.x, iS.pz - e.z); e.state = S_IDLE; return 1; }
        const int* ap = iBfbKit ? KBFKArvelPath : KArvelPath;
        int apn = iBfbKit ? KBFKArvelPath_N : KArvelPath_N;
        if (iS.bfb[BF_ARVEL] == 1 && !(e.pad2 & 128) && e.cool >= 0 && e.cool < apn)
            {
            int tx = Ix(ap[e.cool * 2]), tz = Iz(ap[e.cool * 2 + 1]);
            int dx = tx - e.x, dz = tz - e.z, l = Dist(dx, dz);
            int st = 11 * M / 2 * dtMs / 1000;
            if (l <= st + 8)
                {
                e.x = tx; e.z = tz; e.cool++;
                // at the end of his run he turns to fight whoever follows (the field is a cooldown again)
                if (e.cool >= apn) { e.pad2 |= 128; e.cool = 0; e.aggro = 1; }
                }
            else { e.x += dx * st / l; e.z += dz * st / l; }
            e.yaw = r3d_atan2(dx, dz);
            e.state = S_WANDER;
            e.anim += (11 * M / 2) * dtMs * 1024 / (360 * 1000);
            e.y = FloorAt(e.cell, e.x, e.z, e.y + M / 2);
            return 1;
            }
        }
    if (e.type == E_BANDIT && (e.pad2 & 4) && !iS.bfb[BF_LEVERMAN] && !e.aggro) { e.state = S_IDLE; return 1; }
    if (e.type == E_BANDIT && (e.pad2 & 1) && !iS.bfb[BF_TALK] && !e.aggro && d > 8 * M)
        {
        e.state = S_IDLE;
        return 1;
        }
    // ranged: the Restless Draugr's frost, the spider's venom; the Overlord's shout
    if (e.cool > 0) e.cool -= dt;
    if (e.aggro && e.cell == iS.cell && e.cool <= 0)
        {
        if (e.type == E_WIGHT && e.pad1 == 1 && d > 4 * M && d < 16 * M) { Spit(*this, e, 3, 9); e.cool = 3200000; }
        else if (e.type == E_SPIDER && d > 4 * M && d < 13 * M) { Spit(*this, e, 4, 6); e.cool = 3800000; }
        else if (e.type == E_WIGHT && e.pad1 == 2 && d < 9 * M && d > 2 * M)
            {
            e.cool = 7000000;
            Msg("\"FUS!\"");
            HurtPlayer(12, e.x, e.z);
            int dx = iS.px - e.x, dz = iS.pz - e.z, l = Abs(dx) + Abs(dz) + 1, k;
            for (k = 0; k < 8; k++)
                {
                int nx = iS.px + dx * (M / 2) / l, nz = iS.pz + dz * (M / 2) / l;
                if (BlockedAt(iS.cell, nx, nz, 100, iS.py - 410)) break;
                iS.px = nx; iS.pz = nz;
                }
            iShakeUs = 400000;
            }
        }
    return 0;
    }

void Game::DunKilled(Ent& e)
    {
    if (e.type == E_BANDIT && e.pad1 == 1) { iS.bfb[BF_ARVEL] = 2; e.pad2 &= ~16; }
    if (e.type == E_SPIDER) iS.bfb[BF_SPIDER] = 2;
    }

// what is found on them
// What a bandit carries, as Skyrim's bandits at level 1 (UESP / the Elder Scrolls wiki, Bandits (Skyrim)): its weapon
// (one-handed: a mace, sword or war axe; two-handed: a warhammer, battleaxe or greatsword; archers: a bow, arrows and
// a dagger), its armour (light: fur; the heavy ones: iron and a shield), gold, and now and then lockpicks, food, a
// gem, jewellery, an ingot. The picks are fixed by where it stood (the same bandit, the same things).
// (what is taken is listed on as few message lines as it fits: "Iron Sword, Fur Armor, Fur Boots, 12 Gold")
struct LootList { char line[60]; int n; };
static void Flush(Game& g, LootList& l) { if (l.n) { r3d_puts(l.line + l.n, " added"); g.Msg(l.line); l.n = 0; } }
static void Note(Game& g, LootList& l, const char* aText)
    {
    int k = 0;
    while (aText[k]) k++;
    if (l.n && l.n + 2 + k > 46) Flush(g, l);
    char* p = l.line + l.n;
    if (l.n) { *p++ = ','; *p++ = ' '; }
    p = r3d_puts(p, aText);
    l.n = (int)(p - l.line);
    }
static void Take(Game& g, LootList& l, int aItem, int aCount)
    {
    g.AddItem(aItem, aCount);
    char b[48];
    char* p = r3d_puts(b, GameItem(aItem).name);
    if (aCount > 1) { p = r3d_puts(p, " ("); p = r3d_puti(p, aCount); *p++ = ')'; *p = 0; }
    Note(g, l, b);
    }
void GameBanditLoot(Game& g, Ent& e, u32 h)
    {
    LootList l; l.n = 0; l.line[0] = 0;
    int kind = BanditKind(e);
    u32 h2 = r3d_hash(h, 17, 4242);
    switch (kind)
        {
        case BK_2H: { static const short K[3] = { IT_IRON_WARHAMMER, IT_IRON_BATTLEAXE, IT_IRON_GREATSWORD }; Take(g, l, K[h % 3], 1); break; }
        case BK_ARCHER:
            Take(g, l, (h & 1) ? IT_HUNTING_BOW : IT_LONG_BOW, 1);
            Take(g, l, IT_IRON_ARROW, 4 + (int)(h2 % 9));
            Take(g, l, IT_IRON_DAGGER, 1);
            break;
        default: { static const short K[3] = { IT_IRON_MACE, IT_IRON_SWORD, IT_IRON_AXE }; Take(g, l, K[h % 3], 1); break; }
        }
    if (kind == BK_HEAVY) { Take(g, l, IT_IRON_ARMOR, 1); Take(g, l, IT_IRON_BOOTS, 1); Take(g, l, IT_IRON_SHIELD, 1); if (h2 & 4) Take(g, l, IT_IRON_HELMET, 1); }
    else { Take(g, l, IT_FUR_ARMOR, 1); Take(g, l, IT_FUR_BOOTS, 1); if (h2 & 8) Take(g, l, IT_FUR_HELMET, 1); }
    int gold = 3 + (int)((h >> 8) % 23);
    g.iS.gold += gold;
    { char b[32]; char* p = r3d_puti(b, gold); r3d_puts(p, " Gold"); Note(g, l, b); }
    if (kind != BK_HEAVY && kind != BK_2H && (h2 >> 4) % 3 == 0) Take(g, l, IT_LOCKPICK, 1 + (int)((h2 >> 6) & 1));
    if (kind == BK_1H || kind == BK_ARCHER)
        {
        // the light ones' odds and ends: food, a gem, a ring or necklace, an ingot
        static const short KFood[4] = { IT_BREAD, IT_CHEESE, IT_APPLE, IT_SALMON };
        static const short KGem[2] = { IT_GARNET, IT_AMETHYST };
        static const short KJewel[3] = { IT_SILVER_RING, IT_GOLD_RING, IT_SILVER_NECKLACE };
        if ((h2 >> 8) % 3 == 0) Take(g, l, KFood[(h2 >> 10) & 3], 1);
        if ((h2 >> 12) % 6 == 0) Take(g, l, KGem[(h2 >> 15) & 1], 1);
        if ((h2 >> 16) % 6 == 0) Take(g, l, KJewel[(h2 >> 19) % 3], 1);
        if ((h2 >> 21) % 5 == 0) Take(g, l, IT_IRON_INGOT, 1);
        }
    Flush(g, l);
    }

int GameDunLoot(Game& g, Ent& e)
    {
    u32 h = r3d_hash(e.hx, e.hz, e.type);
    switch (e.type)
        {
        case E_BANDIT:
            if (e.pad1 == 1)
                {
                g.AddItem(IT_GOLDEN_CLAW, 1); g.AddItem(IT_ARVEL_JOURNAL, 1); g.AddItem(IT_IRON_SWORD, 1);
                g.Msg("Golden Claw added"); g.Msg("Arvel's Journal added");
                GotClaw(g);
                return 1;
                }
            GameBanditLoot(g, e, h);
            return 1;
        case E_SKEEVER: g.AddItem(IT_RAT_TAIL, 1); g.Msg("Skeever Tail added"); return 1;
        case E_SPIDER: g.AddItem(IT_FROST_VENOM, 1); g.AddItem(IT_SPIDER_EGG, 2); g.Msg("Frostbite Venom added"); g.Msg("Spider Egg (2) added"); return 1;
        case E_WIGHT:
            if (e.pad1 == 2)
                {
                g.AddItem(IT_DRAGONSTONE, 1); g.AddItem(IT_NORD_FROST, 1); g.iS.gold += 60;
                g.Msg("Dragonstone added"); g.Msg("Ancient Nord War Axe of Frost added"); g.Msg("Gold added");
                if (g.iS.bfb[BF_QBLEAK] < 180) { g.iS.bfb[BF_QBLEAK] = 180; g.Msg("Bring the Dragonstone to Farengar"); }
                return 1;
                }
            if (e.pad1 == 4)
                {
                // Rahgot: his mask (Skyrim's dragon priest mask), his gold
                g.AddItem(IT_RAHGOT, 1); g.AddItem(IT_RAHGOT_STAFF, 1); g.iS.gold += 120; g.iS.dun[FH_RAHGOT] = 1;
                g.Msg("Rahgot added"); g.Msg("Dragon Priest Staff added"); g.Msg("Gold added");
                return 1;
                }
            if (e.pad1 == 5) { g.iS.gold += 50; g.AddItem(IT_POT_HEAL, 1); g.Msg("Gold added"); g.Msg("Potion of Healing added"); return 1; }
            if (e.pad1 == 3) { g.AddItem(IT_GHOST_ESSENCE, 1); g.Msg("Ectoplasm added"); return 1; }
            if (e.cell >= CELL_BFTEMPLE && e.pad1 == 1)
                {
                g.iS.gold += 25; g.AddItem(IT_POT_MMAG, 1);
                g.Msg("Gold added"); g.Msg("Potion of Minor Magicka added");
                return 1;
                }
            return 0;
        }
    return 0;
    }

const char* GameEntName(const Ent& e)
    {
    switch (e.type)
        {
        case E_VILLAGER: return "Villager";
        case E_ELDER: return "Proventus";
        case E_SMITH: return "Adrianne";
        case E_WOLF: return "Wolf";
        case E_WIGHT:
            {
            static const char* const KWight[6] = { "Draugr", "Restless Draugr", "Draugr Overlord", "Ghost", "Rahgot", "Draugr Wight" };
            return KWight[e.pad1 < 6 ? e.pad1 : 0];
            }
        case E_SKELETON: return "Skeleton";
        case E_BANDIT: return e.pad1 == 1 ? "Arvel the Swift" : (e.pad1 == 2 ? "Valmir" : (e.pad1 == 3 ? "Stormcloak Soldier" : (e.var & 1 ? "Bandit Outlaw" : "Bandit")));
        case E_SKEEVER: return "Skeever";
        case E_SPIDER: return "Frostbite Spider";
        case E_TROLL: return "Troll";
        case E_DRAGON: return "Ash-Wing";
        case E_CHEST: return "Chest";
        case E_DEER: return e.var ? "Elk" : "Deer";
        case E_HARE: return "Hare";
        case E_FOX: return "Fox";
        case E_GOAT: return "Goat";
        case E_MUDCRAB: return "Mudcrab";
        }
    return "";
    }

// ---------------------------------------------------------------- traps, triggers and the moving parts
static int FindEnt(Game& g, int aType, int aPad1, int aPad2Bit)
    {
    int i;
    for (i = 0; i < g.iS.ne; i++)
        {
        const Ent& e = g.iS.e[i];
        if (e.type == aType && e.pad1 == aPad1 && (!aPad2Bit || (e.pad2 & aPad2Bit))) return i;
        }
    return -1;
    }
// the swing of a blade (units of 1024 a turn), from the shared clock and its place in the row
static int BladeAngle(int aMs, int k) { return (r3d_sin((aMs * 1024 / 1700 + k * 300) & 1023) * 150) >> 14; }

// (v0.4.62) the moving lids: which of the cell's (its sleepers' lids, in order)
static int LidNo(const DunData* d, int k)
    {
    int i, n = 0;
    for (i = 0; i < k; i++) if (d->fix[i * 7] == F_COFFINLID && d->fix[i * 7 + 5] == 1) n++;
    n += d == &KDunKit[1] ? 4 : 0;          // (the sanctum's after the temple's)
    n += d == &KDunFh[1] ? 3 : (d == &KDunFh[2] ? 6 : 0);      // (v0.4.68: Forelhost's, hall by hall)
    return n < 8 ? n : 7;
    }
void Game::UpdateDungeon(int dt)
    {
    int dtMs = dt / 1000, k;
    FhQuestUpdate(dt);                              // (v0.4.69) Siege on the Dragon Cult
    // gates, the claw door and the sarcophagus' lid move towards where they should be (also after a load)
    int want0 = iS.bfb[BF_GATE0] ? 240 : 0, want1 = iS.bfb[BF_GATE1] ? 240 : 0, wantC = iS.bfb[BF_CLAWDOOR] ? 340 : 0;
    int st = 120 * dtMs / 1000 + 1;
    iBfTimer[8] += iBfTimer[8] < want0 ? st : (iBfTimer[8] > want0 ? -st : 0);
    iBfTimer[9] += iBfTimer[9] < want1 ? st : (iBfTimer[9] > want1 ? -st : 0);
    iBfTimer[10] += iBfTimer[10] < wantC ? st : 0;
    for (k = 0; k < 8; k++)
        {
        int w = iS.dun[FH_GATE0 + k] ? 240 : 0;
        iFhGate[k] += iFhGate[k] < w ? st : (iFhGate[k] > w ? -st : 0);
        }
    iFhClaw += iFhClaw < (iS.dun[FH_CLAWDOOR] ? 340 : 0) ? st : 0;
    iBfTimer[11] += dtMs;
    if (iBfTimer[11] > 1700 * 60) iBfTimer[11] -= 1700 * 60;
    for (k = 0; k < 2; k++)
        {
        // a burning pool: the lantern falls, the fire burns for a while, then the oil is gone
        if (iS.bfb[BF_OIL0 + k] == 1)
            {
            if (iBfTimer[5 + k] < 260) iBfTimer[5 + k] += 600 * dtMs / 1000 + 1;
            else if (iBfTimer[3 + k] <= 0) iS.bfb[BF_OIL0 + k] = 2;
            }
        if (iBfTimer[3 + k] > 0) iBfTimer[3 + k] -= dt;
        }
    if (iBfTimer[0] > 0) iBfTimer[0] -= dt;
    if (iBfTimer[14] > 0) iBfTimer[14] -= dt;
    if (iBfTimer[1] > 0) iBfTimer[1] -= dt;
    // the Overlord: the word learned, and a moment later the lid grinds aside
    if (iS.bfb[BF_WORD] && iBfTimer[12] < 2400000) iBfTimer[12] += dt;
    if (iS.bfb[BF_WORD] && !iS.bfb[BF_LORD])
        {
        if (iBfTimer[12] > 1800000)
            {
            iS.bfb[BF_LORD] = 1;
            int i = FindEnt(*this, E_WIGHT, 2, 64);
            if (i >= 0) { iS.e[i].pad2 &= ~64; iS.e[i].aggro = 1; }
            iShakeUs = 500000;
            }
        }
    const DunData* d = DunOfK(*this, iS.cell);
    // (v0.4.62) a sleeper's lid grinds aside as it wakes (it lies under it: within a metre of the lid, pad2 8)
    if (d)
        for (k = 0; k < d->nfix; k++)
            {
            const int* f = d->fix + k * 7;
            if (f[0] != F_COFFINLID || f[5] != 1) continue;
            int lx = Ix(f[1]), lz = Iz(f[3]), asleep = 0, i;
            for (i = 0; i < iS.ne; i++)
                {
                const Ent& e = iS.e[i];
                if (e.alive && e.cell == iS.cell && e.type == E_WIGHT && (e.pad2 & 8) && Abs(e.x - lx) < M && Abs(e.z - lz) < M) asleep = 1;
                }
            short& o = iLidOpen[LidNo(d, k)];
            if (!asleep && o < 90) { o = (short)(o + 90 * dtMs / 500 + 1); if (o > 90) o = 90; }
            if (asleep) o = 0;
            }
    // (v0.4.68) Forelhost's word wall on the ruin's top: walking up to it is enough
    if (!iS.cell && !iS.dun[FH_WORD])
        {
        int i;
        for (i = 0; i < iS.nprop; i++)
            {
            const Prop& p = iS.prop[i];
            if (p.type != PT_WORDWALL || p.cell != 0 || p.item != 1) continue;
            if (Dist(p.x - iS.px, p.z - iS.pz) < 5 * M && Abs(iS.py - 410 - p.y) < 4 * M)
                {
                iS.dun[FH_WORD] = 1;
                iShoutFxUs = 1200000; iShakeUs = 300000;
                Msg("Word of Power learned: STRUN (Storm)");
                Msg("Storm Call - its shout is not in the game yet.");
                }
            }
        }
    // speech and scripted moments need the player there
    if (iS.cell == CELL_BFTEMPLE)
        {
        // the two bandits in the hall talk about Arvel while the player listens from the passage
        int a = FindEnt(*this, E_BANDIT, 0, 1);
        if (!iS.bfb[BF_TALK] && a >= 0 && iS.e[a].alive && !iS.e[a].aggro)
            {
            const Ent& e = iS.e[a];
            if (Dist(e.x - iS.px, e.z - iS.pz) < 22 * M)
                {
                static const char* const KTalk[4] =
                    {
                    "So we're just supposed to sit here while Arvel runs off with that golden claw?",
                    "That dark elf wants to go on ahead, let him. Better than us risking our necks.",
                    "What if Arvel doesn't come back? I want my share from that claw!",
                    "Just shut it and keep an eye out for trouble."
                    };
                int t0 = iBfTimer[7];
                iBfTimer[7] += dt;
                int line;
                for (line = 0; line < 4; line++)
                    {
                    int at = line * 4000000;
                    if (t0 <= at && iBfTimer[7] > at) Say(*this, "Bandit", KTalk[line]);
                    }
                if (t0 <= 4000000 && iBfTimer[7] > 4000000 && iS.bfb[BF_QCLAW] == 0)
                    {
                    iS.bfb[BF_QCLAW] = 12;
                    Msg("Quest started: The Golden Claw");
                    }
                if (iBfTimer[7] > 15000000) iS.bfb[BF_TALK] = 1;
                }
            }
        // the bandit at the lever: he pulls it before you can stop him
        int b = FindEnt(*this, E_BANDIT, 0, 4);
        if (!iS.bfb[BF_LEVERMAN] && b >= 0 && iS.e[b].alive)
            {
            const Ent& e = iS.e[b];
            if (Dist(e.x - iS.px, e.z - iS.pz) < 9 * M)
                {
                if (iBfTimer[15] == 0) Say(*this, "Bandit", KSAY_4);
                iBfTimer[15] += dt;
                if (iBfTimer[15] > 1500000)
                    {
                    iS.bfb[BF_LEVERMAN] = 1;
                    iBfTimer[2] = 2500000;
                    int i;
                    for (i = 0; i < iS.nprop; i++) if (iS.prop[i].type == PT_LEVER && iS.prop[i].cell == CELL_BFTEMPLE) iS.prop[i].state ^= 1;
                    }
                }
            }
        }
    if (iS.cell == CELL_BFCRYPT || (iBfbKit && iS.cell == CELL_BFTEMPLE))      // (v0.4.55: Skyrim's temple has the spider's lair)
        {
        // the spider drops from the ceiling as you come into its lair
        int s = FindEnt(*this, E_SPIDER, 0, 8);
        if (s >= 0)
            {
            Ent& e = iS.e[s];
            if (Dist(e.x - iS.px, e.z - iS.pz) < 9 * M)
                {
                e.pad2 &= ~8; e.fly = 5 * M; iS.bfb[BF_SPIDER] = 1;
                int a = FindEnt(*this, E_BANDIT, 1, 16);
                if (a >= 0) Say(*this, "Arvel the Swift", KSAY_5);
                }
            }
        // Arvel calls out from the web
        int a = FindEnt(*this, E_BANDIT, 1, 16);
        if (a >= 0 && iS.e[a].alive)
            {
            const Ent& e = iS.e[a];
            int dd = Dist(e.x - iS.px, e.z - iS.pz);
            if (iBfTimer[13] > 0) iBfTimer[13] -= dt;
            if (dd < 20 * M && iBfTimer[13] <= 0 && iUi == UI_PLAY)
                {
                iBfTimer[13] = 7000000;
                static const char* const KCall[4] = { "Is... is someone coming? Is that you Harknir? Bjorn? Soling? You. Over here!", "Hey, over here!", "Get me out of here!", "Don't leave me, for Arkay's sake!" };
                static const char* const KPanic[3] = { "Ah, kill it. Kill it!", "Get it away from me. Get it away!", "Don't let it get me. Help!" };
                int sp = iS.bfb[BF_SPIDER];
                if (sp == 1) Say(*this, "Arvel the Swift", KPanic[(iTimeUs >> 20) % 3]);
                else if (sp == 2) Say(*this, "Arvel the Swift", KSAY_6);
                else Say(*this, "Arvel the Swift", KCall[iBfTimer[16] ? 1 + ((iTimeUs >> 20) % 3) : 0]);
                iBfTimer[16] = 1;
                }
            }
        }
    if (!d) return;
    // fixtures of the cell the player is in
    int ms = iBfTimer[11];
    int feet = iS.py - 410;
    for (k = 0; k < d->nfix; k++)
        {
        const int* f = d->fix + k * 7;
        int fx = Ix(f[1]), fz = Iz(f[3]), fy = IN_FLOOR + Cm(f[2]);
        switch (f[0])
            {
            case F_BLADE:
                {
                int active = f[5] == 0 ? !iS.bfb[BF_BLADES0] : iS.bfb[BF_BLADES1] == 1;
                if (!active || iBfTimer[0] > 0) break;
                int yaw = f[4] - 256, cs = r3d_cos(yaw), sn = r3d_sin(yaw);
                int dx = iS.px - fx, dz = iS.pz - fz;
                int lx = (dx * cs - dz * sn) >> 14, lz = (dx * sn + dz * cs) >> 14;
                int a = BladeAngle(ms, k);
                int bz = -((r3d_sin(a) * Cm(215)) >> 14), by = Cm(260) - ((r3d_cos(a) * Cm(215)) >> 14);
                if (Abs(lx) < Cm(45) && Abs(lz - bz) < Cm(55) && by < Cm(160) && Abs(feet - fy) < Cm(60))
                    {
                    HurtPlayer(30, fx - (lz < bz ? -M : M) * sn / 16384, fz);
                    iBfTimer[0] = 700000;
                    }
                break;
                }
            case F_PLATE:
                {
                if (iBfTimer[14] > 0 && f[5] == 0) break;
                int on = Abs(iS.px - fx) < Cm(45) && Abs(iS.pz - fz) < Cm(45) && Abs(feet - fy) < Cm(40);
                // Arvel running for the door steps on it
                int a = -1, i;
                for (i = 0; i < iS.ne; i++)
                    {
                    const Ent& e = iS.e[i];
                    if (e.alive && e.cell == iS.cell && e.type == E_BANDIT && e.pad1 == 1 && !(e.pad2 & 16) && Abs(e.x - fx) < Cm(55) && Abs(e.z - fz) < Cm(55)) a = i;
                    }
                if (!on && a < 0) break;
                if (f[5] == 0)
                    {
                    // the spike wall swings out from the passage's wall
                    if (iBfTimer[1] > 0) break;
                    iBfTimer[1] = 900000; iBfTimer[14] = 3000000;
                    Msg("*click*");
                    int s;
                    for (s = 0; s < d->nfix; s++)
                        {
                        const int* w = d->fix + s * 7;
                        if (w[0] != F_SPIKEWALL) continue;
                        int wx = Ix(w[1]), wz = Iz(w[3]);
                        if (Abs(iS.px - wx) < Cm(260) && Abs(iS.pz - wz) < Cm(240)) HurtPlayer(75, wx, wz);
                        for (i = 0; i < iS.ne; i++)
                            {
                            Ent& e = iS.e[i];
                            if (e.alive && e.cell == iS.cell && e.type >= E_WOLF && Abs(e.x - wx) < Cm(260) && Abs(e.z - wz) < Cm(240)) HurtEnt(e, 75, 2 * M, wx, wz);
                            }
                        }
                    }
                else if (iS.bfb[BF_BLADES1] == 0) { iS.bfb[BF_BLADES1] = 1; Msg("*click*"); }
                break;
                }
            case F_DARTS:
                {
                if (iBfTimer[2] <= 0) break;
                int before = iBfTimer[2];
                iBfTimer[2] -= dt;
                if ((before / 90000) == (iBfTimer[2] / 90000)) break;
                if (Abs(iS.px - fx) < Cm(160) && Abs(iS.pz - fz) < Cm(140) && Abs(feet - fy) < Cm(80)) HurtPlayer(1, fx - M, fz);
                int i;
                for (i = 0; i < iS.ne; i++)
                    {
                    Ent& e = iS.e[i];
                    if (e.alive && e.cell == iS.cell && e.type == E_BANDIT && Abs(e.x - fx) < Cm(160) && Abs(e.z - fz) < Cm(140)) HurtEnt(e, 6, 0, fx - M, fz);
                    }
                break;
                }
            case F_LANTERN:
                {
                int id = f[5] & 1;
                if (iS.bfb[BF_OIL0 + id]) break;
                // a spell through the lantern brings it down into the oil
                int i;
                for (i = 0; i < MAXP; i++)
                    {
                    const Proj& p = iP[i];
                    if (!p.active || p.owner || (p.kind != 0 && p.kind != 2)) continue;
                    if (Abs(p.x - fx) < Cm(80) && Abs(p.z - fz) < Cm(80) && Abs(p.y - fy) < Cm(90))
                        {
                        iS.bfb[BF_OIL0 + id] = 1; iBfTimer[5 + id] = 0; iBfTimer[3 + id] = 7000000;
                        Msg("The lantern falls into the oil!");
                        }
                    }
                break;
                }
            case F_OIL:
                {
                int id = f[5] & 1;
                // a fireball landing in the pool lights it as well
                if (!iS.bfb[BF_OIL0 + id])
                    {
                    int i;
                    for (i = 0; i < MAXP; i++)
                        {
                        const Proj& p = iP[i];
                        if (p.active && !p.owner && p.kind == 2 && Abs(p.x - fx) < Cm(f[6]) && Abs(p.z - fz) < Cm(f[6]))
                            { iS.bfb[BF_OIL0 + id] = 1; iBfTimer[5 + id] = 260; iBfTimer[3 + id] = 7000000; }
                        }
                    }
                if (iS.bfb[BF_OIL0 + id] != 1 || iBfTimer[5 + id] < 260 || iBfTimer[3 + id] <= 0) break;
                // burning: everything in the pool takes the fire's bite four times a second
                if ((iBfTimer[3 + id] / 250000) == ((iBfTimer[3 + id] + dt) / 250000)) break;
                int r2 = Cm(f[6]);
                if (Abs(iS.px - fx) < r2 && Abs(iS.pz - fz) < r2 && Abs(feet - fy) < Cm(80)) HurtPlayer(3, fx, fz);
                int i;
                for (i = 0; i < iS.ne; i++)
                    {
                    Ent& e = iS.e[i];
                    if (e.alive && e.cell == iS.cell && e.type >= E_WOLF && e.type < E_DRAGON && Abs(e.x - fx) < r2 && Abs(e.z - fz) < r2) HurtEnt(e, 6, 0, fx, fz);
                    }
                break;
                }
            }
        }
    // the word wall: walking up to it is enough
    if (iS.cell == CELL_BFSANCTUM && !iS.bfb[BF_WORD])
        {
        int i;
        for (i = 0; i < iS.nprop; i++)
            {
            const Prop& p = iS.prop[i];
            if (p.type != PT_WORDWALL || p.cell != iS.cell) continue;
            if (Dist(p.x + (iBfbKit ? 0 : 2 * M) - iS.px, p.z - iS.pz) < 5 * M && iS.py - 410 > p.y - (iBfbKit ? Cm(250) : Cm(40)))     // (Skyrim's: its trigger's middle, about 2 m over the dais)
                {
                iS.bfb[BF_WORD] = 1; iBfTimer[12] = 0;
                iShoutFxUs = 1200000; iShakeUs = 300000;
                Msg("Word of Power learned: FUS (Force)");
                Msg("Shout: Unrelenting Force. Z (or Y) to shout.");
                }
            }
        }
    // the claw door, once sunk into the floor, is gone
    for (k = 0; k < iS.nprop; k++) if (iS.prop[k].type == PT_CLAWDOOR && ClawH(*this, iS.prop[k].cell) >= 335) iS.prop[k].flags |= PF_GONE;
    // pull chains spring back
    for (k = 0; k < iS.nprop; k++)
        if (iS.prop[k].type == PT_PULLCHAIN && iS.prop[k].anim > 0) iS.prop[k].anim = (short)(iS.prop[k].anim - 1);
    // the pillars grind round a third of a turn in about two thirds of a second
    {
    int turn = 341 * dtMs / 650 + 1;
    for (k = 0; k < iS.nprop; k++)
        if (iS.prop[k].type == PT_PILLAR && iS.prop[k].anim > 0) iS.prop[k].anim = (short)(iS.prop[k].anim > turn ? iS.prop[k].anim - turn : 0);
    }
    }

// ---------------------------------------------------------------- drawing
static const u8* DefSl(Game& g, int aIn) { return g.iR->iDefs[g.iInType[aIn] - ST_CUSTOM].slots; }

// with Skyrim's barrow in: each moving lid's own piece in the cell (NorSarcophagusTop, within half a metre) taken out
// - the game draws it, moving - and the lid's kind kept for the Overlord's too
void GameBarrowLids(Game& g)
    {
    R3D& r = *g.iR;
    int c, i, k;
    g.iLidType = -1;
    for (i = 0; i < r.iINum; i++)
        {
        const Obj& o = r.iIObj[i];
        if (o.type < ST_CUSTOM || (o.flag != CELL_BFTEMPLE && o.flag != CELL_BFSANCTUM && !IsFh(o.flag))) continue;
        int t;
        for (t = 0; t < g.iKitMeshN && t < Game::MAXKIT; t++) if (g.iKitType[t] == o.type) break;
        if (t >= g.iKitMeshN || t >= Game::MAXKIT) continue;
        const char* nm = g.KitM(t).name;
        static const char KName[] = "NorSarcophagusTop ";
        int j;
        for (j = 0; KName[j] && nm[j] == KName[j]; j++) {}
        if (KName[j]) continue;
        g.iLidType = o.type;
        for (c = CELL_BFTEMPLE; c <= CELL_FHREFECT; c++)
            {
            if (c > CELL_BFSANCTUM && !IsFh(c)) continue;
            const DunData* d = DunOfK(g, c);
            if (!d || o.flag != c) continue;
            for (k = 0; k < d->nfix; k++)
                {
                const int* f = d->fix + k * 7;
                if (f[0] != F_COFFINLID || f[5] != 1) continue;
                if (Abs(o.x - Ix(f[1])) < METER / 2 && Abs(o.z - Iz(f[3])) < METER / 2) r.iIObj[i].flag = 0;      // (drawn moving instead)
                }
            }
        }
    }

void Game::DrawDungeon()
    {
    R3D& r = *iR;
    int k;
    // the cell's walls, floors and ceilings: the tiles near enough to matter
    for (k = 0; k < iNDun; k++)
        {
        if (iDunCell[k] != iS.cell) continue;
        int dx = (iDunX[k] - r.iCamX) >> 8, dz = (iDunZ[k] - r.iCamZ) >> 8;
        int rad = iMDun[k].radius >> 8;
        int lim = 46 + rad;
        if (dx * dx + dz * dz > lim * lim) continue;
        r.DrawMesh(iMDun[k], 0, iDunX[k], iDunY[k], iDunZ[k], 0, 256, iDunSl, 0);
        }
    const DunData* d = DunOfK(*this, iS.cell);
    if (!d) return;
    int ms = iBfTimer[11];
    for (k = 0; k < d->nfix; k++)
        {
        const int* f = d->fix + k * 7;
        int x = Ix(f[1]), y = IN_FLOOR + Cm(f[2]), z = Iz(f[3]);
        int dx = (x - r.iCamX) >> 8, dz = (z - r.iCamZ) >> 8;
        if (dx * dx + dz * dz > 40 * 40) continue;
        switch (f[0])
            {
            case F_GATE:
                {
                if (iBfbKit || IsFh(iS.cell))
                    {
                    // (v0.4.55) Skyrim's own gate's place: across the way it runs, as wide as Skyrim's
                    r.DrawMesh(iMIn[IN_GATE], 1, x, y + Cm(GateH(*this, iS.cell, f)), z, f[4], f[6] * 256 / 180, DefSl(*this, IN_GATE), 0);
                    break;
                    }
                int a0, a1;
                GateSpan(*d, f[1], f[3], 0, a0, a1);
                int wide = a1 - a0 > 250;
                r.DrawMesh(iMIn[IN_GATE], wide, Ix((a0 + a1) / 2), y + Cm(iBfTimer[8 + (f[5] & 1)]), z, 0, 256, DefSl(*this, IN_GATE), 0);
                break;
                }
            case F_BLADE:
                {
                int active = f[5] == 0 ? !iS.bfb[BF_BLADES0] : iS.bfb[BF_BLADES1] == 1;
                int a = active ? BladeAngle(ms, k) : 0;
                int yaw = f[4] - 256, cs = r3d_cos(yaw), sn = r3d_sin(yaw), c = r3d_cos(a), s = r3d_sin(a);
                int A[9] = { cs, (sn * s) >> 14, (sn * c) >> 14,   0, c, -s,   -sn, (cs * s) >> 14, (cs * c) >> 14 };
                r.DrawMeshMat(iMIn[IN_BLADE], 0, x, y + Cm(260), z, A, 256, DefSl(*this, IN_BLADE), 0);
                break;
                }
            case F_SPIKEWALL:
                {
                // resting flat along the wall, swung out across the passage while it fires
                int t = iBfTimer[1], sw = 0;
                if (t > 0) { int ph = 900000 - t; sw = ph < 250000 ? ph * 256 / 250000 : (t < 400000 ? t * 256 / 400000 : 256); }
                r.DrawMesh(iMIn[IN_SPIKEWALL], 0, x, y, z, 512 - sw, 320, DefSl(*this, IN_SPIKEWALL), 0);
                break;
                }
            case F_LANTERN:
                {
                int id = f[5] & 1, drop = iS.bfb[BF_OIL0 + id] ? iBfTimer[5 + id] : 0;
                if (iS.bfb[BF_OIL0 + id] == 2 || drop >= 255) break;
                r.DrawMesh(iMIn[IN_LANTERN], 0, x, y - Cm(drop), z, 0, 256, DefSl(*this, IN_LANTERN), 0);
                if (!drop) r.DrawMesh(iMFlame, (iTimeUs >> 16) & 7, x, y + Cm(10), z, 0, 50, DefSl(*this, IN_FLAME), MF_UNLIT);
                break;
                }
            case F_OIL:
                {
                int id = f[5] & 1;
                if (iS.bfb[BF_OIL0 + id] != 1 || iBfTimer[5 + id] < 255 || iBfTimer[3 + id] <= 0) break;
                int n, rr = f[6];
                for (n = 0; n < 5; n++)
                    {
                    int a = n * 205 + (iTimeUs >> 14);
                    int fx2 = x + Cm((r3d_sin(a) * rr / 2) >> 14), fz2 = z + Cm((r3d_cos(a) * rr / 2) >> 14);
                    r.DrawMesh(iMFlame, ((iTimeUs >> 16) + n) & 7, fx2, y, fz2, 0, 420, DefSl(*this, IN_FLAME), MF_UNLIT);
                    }
                break;
                }
            case F_DARTS:
                {
                if (iBfTimer[2] <= 0) break;
                int n;
                for (n = 0; n < 6; n++)
                    {
                    int ph = (iTimeUs / 1000 + n * 53) % 300;
                    int dxs = Cm(-160 + ph * 320 / 300), dz2 = Cm(-120 + n * 48);
                    r.DrawMeshRoll(iMIn[IN_DART], 0, x + dxs, y + Cm(60 + (n & 1) * 50), z + dz2, 256, 256, 256, DefSl(*this, IN_DART), 0);
                    }
                break;
                }
            case F_COFFINLID:
                {
                int open = iS.bfb[BF_LORD] ? Clamp(iBfTimer[12] - 1800000, 0, 600000) * 90 / 600000 : 0;
                if (iS.bfb[BF_LORD] && iBfTimer[12] < 1800000) open = 90;
                if (f[5] == 1) open = iLidOpen[LidNo(d, k)];             // (v0.4.62: a sleeper's: as it wakes)
                if (iBfbKit && iLidType >= ST_CUSTOM)
                    {
                    // (v0.4.62) Skyrim's own lid (its piece in the cell hidden: GameBarrowLids)
                    const StaticDef& sd = r.iDefs[iLidType - ST_CUSTOM];
                    int yaw = f[4], cs = r3d_cos(yaw), sn = r3d_sin(yaw);
                    r.DrawMesh(*sd.mesh, 0, x + ((Cm(open) * cs) >> 14), y - Cm(open / 4), z - ((Cm(open) * sn) >> 14), yaw + open, 256, sd.slots, 0);
                    break;
                    }
                int yaw = f[4], cs = r3d_cos(yaw), sn = r3d_sin(yaw);
                r.DrawMesh(iMIn[IN_SARCLID], 0, x + ((Cm(open) * cs) >> 14), y - Cm(open / 4), z - ((Cm(open) * sn) >> 14), yaw + open, 256, DefSl(*this, IN_SARCLID), 0);
                break;
                }
            }
        }
    }

void Game::DrawDunProp(const Prop& p)
    {
    R3D& r = *iR;
    switch (p.type)
        {
        case PT_PILLAR:
            {
            // p.anim: how far it still has to turn to the face it now shows (the stone grinds round)
            int sym = iS.bfb[BF_PILLAR0 + (p.item % 3)] % 3;
            r.DrawMesh(iMIn[IN_PUZPILLAR], 0, p.x, p.y, p.z, p.yaw - sym * 341 + p.anim, 256, DefSl(*this, IN_PUZPILLAR), 0);
            break;
            }
        case PT_PULLCHAIN:
            r.DrawMesh(iMIn[IN_CHAIN], p.anim > 0 ? 1 : 0, p.x, p.y, p.z, p.yaw, 256, DefSl(*this, IN_CHAIN), 0);
            break;
        case PT_CLAWDOOR:
            {
            int h = ClawH(*this, p.cell);
            int sink = Cm(h);
            if (h >= 335) break;
            r.DrawMesh(iMIn[IN_CLAWDOOR], 0, p.x, p.y - sink, p.z, p.yaw, 256, DefSl(*this, IN_CLAWDOOR), 0);
            int k;
            for (k = 0; k < 3; k++)
                r.DrawMesh(iMIn[IN_CLAWRING], k * 3 + RingOf(*this, p.cell == CELL_FHREFECT, k) % 3, p.x, p.y - sink, p.z, p.yaw, 256, DefSl(*this, IN_CLAWRING), 0);
            break;
            }
        case PT_WORDWALL:
            if (iBfbKit) break;                          // (v0.4.55: Skyrim's own wall is in its halls)
            r.DrawMesh(iMIn[IN_WORDWALL], 0, p.x, p.y, p.z, p.yaw, 256, DefSl(*this, IN_WORDWALL), iS.bfb[BF_WORD] ? 0 : MF_FLASH * ((iTimeUs >> 19) & 1));
            break;
        }
    }

// ---------------------------------------------------------------- using the barrow's things
int Game::DunPropText(int i, const char*& aVerb, const char*& aName)
    {
    const Prop& p = iS.prop[i];
    switch (p.type)
        {
        case PT_PILLAR: aVerb = "Rotate"; aName = "Pillar"; return 1;
        case PT_PULLCHAIN: aVerb = "Pull"; aName = "Pull Chain"; return 1;
        case PT_CLAWDOOR: aVerb = iS.bfb[BF_CLAWDOOR] ? "" : "Examine"; aName = "Door"; return 1;
        case PT_WORDWALL: aVerb = "Examine"; aName = "Word Wall"; return 1;
        case PT_WELL: aVerb = "Climb in"; aName = "Well"; return 1;
        case PT_LEVER: if (IsDungeon(p.cell)) { aVerb = "Pull"; aName = "Lever"; return 1; } break;
        }
    return 0;
    }

int Game::UseDunProp(int i)
    {
    Prop& p = iS.prop[i];
    switch (p.type)
        {
        case PT_PILLAR:
            if (p.anim > 0) return 1;                     // still turning
            iS.bfb[BF_PILLAR0 + (p.item % 3)] = (iS.bfb[BF_PILLAR0 + (p.item % 3)] + 1) % 3;
            p.anim = 341;
            iShakeUs = 120000;
            return 1;
        case PT_PULLCHAIN:
            p.anim = 8;
            if (p.item == 0 && !iS.bfb[BF_BLADES0]) { iS.bfb[BF_BLADES0] = 1; Msg("The blades slow and stop."); }
            else if (p.item == 1 && !iS.bfb[BF_GATE1]) { iS.bfb[BF_GATE1] = 1; Msg("Somewhere a gate grinds open."); }
            else if (p.item == 2 && iS.bfb[BF_BLADES1] != 2) { iS.bfb[BF_BLADES1] = 2; Msg("The blades slow and stop."); }
            return 1;
        case PT_LEVER:
            if (!IsDungeon(p.cell)) return 0;
            p.state ^= 1;
            if (p.item >= 100)
                {
                // (v0.4.68) a ruin's lever: the portcullis it works goes up, or down again
                int g = p.item - 100;
                if (g < 8) { iS.dun[FH_GATE0 + g] ^= 1; Msg(iS.dun[FH_GATE0 + g] ? "The portcullis rises." : "The portcullis falls."); }
                return 1;
                }
            if (iS.bfb[BF_GATE0]) return 1;
            // the answer above the gate: snake, snake, whale from the left (here: the pillars south to north)
            if (iS.bfb[BF_PILLAR2] == 0 && iS.bfb[BF_PILLAR1] == 0 && iS.bfb[BF_PILLAR0] == 2) { iS.bfb[BF_GATE0] = 1; Msg("The gate rises."); }
            else { iBfTimer[2] = 2500000; Msg("*click*"); }
            return 1;
        case PT_CLAWDOOR:
            {
            int fh = p.cell == CELL_FHREFECT;
            if (fh ? iS.dun[FH_CLAWDOOR] : iS.bfb[BF_CLAWDOOR]) return 1;
            if (CountItem(fh ? IT_GLASS_CLAW : IT_GOLDEN_CLAW) <= 0) { Msg("A great round door: three carved rings, and"); Msg("a hollow the shape of a claw's palm."); return 1; }
            iPuzRing = 0; iPuzDoor = fh; iUi = UI_PUZZLE;
            return 1;
            }
        case PT_WORDWALL:
            if (p.item == 1 ? iS.dun[FH_WORD] : iS.bfb[BF_WORD]) Msg("The words are silent now.");
            return 1;
        case PT_WELL: { int GameFhUseWell(Game& g); return GameFhUseWell(*this); }
        }
    return 0;
    }

// ---------------------------------------------------------------- the claw door's rings
// each door's three symbols (every ring turns through them): the claw's palm, outer ring to inner - Bleak Falls' Golden
// Claw: Bear, Moth, Owl; Forelhost's Glass Claw: Fox, Owl, Snake
static const char* const KAnimal[2][3] = { { "Bear", "Moth", "Owl" }, { "Fox", "Owl", "Snake" } };
void Game::PuzzleInput()
    {
    int* rb = iPuzDoor ? 0 : &iS.bfb[BF_RING0];
    u8* rf = iPuzDoor ? &iS.dun[FH_RING0] : 0;
    #define RING(k) (rb ? rb[k] : (int)rf[k])
    #define SETRING(k, v) do { if (rb) rb[k] = (v); else rf[k] = (u8)(v); } while (0)
    if (Pressed(KEY_LOOK_U) || Pressed(KEY_FWD)) iPuzRing = (iPuzRing + 2) % 3;
    if (Pressed(KEY_LOOK_D) || Pressed(KEY_BACK)) iPuzRing = (iPuzRing + 1) % 3;
    if (Pressed(KEY_LOOK_L) || Pressed(KEY_STRAFE_L)) SETRING(iPuzRing, (RING(iPuzRing) + 2) % 3);
    if (Pressed(KEY_LOOK_R) || Pressed(KEY_STRAFE_R)) SETRING(iPuzRing, (RING(iPuzRing) + 1) % 3);
    if (Pressed(KEY_FIRE))
        {
        if (RING(0) == 0 && RING(1) == 1 && RING(2) == 2)
            {
            if (iPuzDoor) iS.dun[FH_CLAWDOOR] = 1; else iS.bfb[BF_CLAWDOOR] = 1;
            iUi = UI_PLAY; iShakeUs = 600000;
            Msg("The claw turns. The door rumbles open.");
            }
        else Msg("The claw turns, but the door does not move.");
        }
    }
void Game::DrawPuzzle(int x0, int w)
    {
    R3D& r = *iR;
    int cx = x0 + w / 2;
    r.Shade(x0 + 10, 20, w - 20, 150);
    const char* t = "THE CLAW DOOR";
    r.Text(cx - r.TextW(t) / 2, 28, t, 0xFEA0);
    static const char* const KRing[3] = { "Outer ring", "Middle ring", "Inner ring" };
    int k;
    for (k = 0; k < 3; k++)
        {
        char b[40];
        char* q = r3d_puts(b, KRing[k]);
        q = r3d_puts(q, ":  < ");
        q = r3d_puts(q, KAnimal[iPuzDoor & 1][RingOf(*this, iPuzDoor, k) % 3]);
        r3d_puts(q, " >");
        int y = 52 + k * 18, tw = r.TextW(b);
        if (k == iPuzRing) r.Tint(cx - tw / 2 - 8, y - 3, tw + 16, 13, 0xFEA0);
        r.Text(cx - tw / 2, y, b, k == iPuzRing ? 0xFFFF : 0xA514);
        }
    const char* h = iPuzDoor ? "The Glass Claw's palm: Fox, Owl, Snake" : "The Golden Claw's palm: Bear, Moth, Owl";
    r.Text(cx - r.TextW(h) / 2, 112, h, 0xA514);
    const char* u = "Up/down: ring   Left/right: turn   E: the claw";
    r.Text(cx - r.TextW(u) / 2, 150, u, 0xA514);
    }
