// game_kit.cpp - interiors built from Skyrim's own pieces: every model a cell uses, simplified from Skyrim's meshes
// (docs/maps/gen_kit.py -> kit_ext.inc), placed where Skyrim places it; the floors and walls you walk against are the
// building pieces' own; the doors are where Skyrim's doors are and lead where they lead.
#include "game.h"

static const int M = METER;
static int Cm(int cm) { return cm * M / 100; }
static inline int Abs(int v) { return v < 0 ? -v : v; }
static int U(int aUnits) { return aUnits * 143 / 100; }                 // Skyrim units -> cm
static int SY(int aDeg) { return (aDeg * 1024 / 360) & 1023; }
enum { IN_OX = 100 * METER, IN_OZ = 100 * METER, IN_FLOOR = 100 * METER };
static int Ix(int cm) { return IN_OX + Cm(cm); }
static int Iz(int cm) { return IN_OZ + Cm(cm); }
// engine units <-> Skyrim units inside (x 0.2734 = 70 / 256)
static int ToU(int aEng) { return aEng * 70 / M; }
int GameZoneOf(int aCell, int aX, int aZ);

static const KitCell* Find(const Game& g, int aCell)
    {
    int i;
    for (i = 0; i < g.iKitCellN; i++) if (g.iKitCells[i].cell == aCell) return &g.iKitCells[i];
    return 0;
    }
int GameIsKitCell(const Game& g, int aCell) { return aCell && Find(g, aCell) != 0; }
// (v0.4.56) a cell walked on Skyrim's navmesh (its walls the navmesh's edges, marked top over bottom): the pieces'
// own collision boxes left out there (the navmesh keeps one off them)
int GameKitNav(const Game& g, int aCell) { const KitCell* c = aCell ? Find(g, aCell) : 0; return c && c->nwall > 0 && c->wall[4] > c->wall[5]; }
static int WorldX(int aX) { return (int)(((long long)aX + 233472) * 36576 / 10000); }
static int WorldZ(int aY) { return (int)(((long long)aY + 176128) * 36576 / 10000); }
// outside: engine units <-> Skyrim's world units (x 3.6576), from the outside's origin; heights over the land's offset
static int OutU(long long aEng, int aOrigin, int aShift) { return (int)(aEng * 10000 / 36576) - aShift - aOrigin; }
int GameKitEntryFeet(const Game& g, int aCell) { const KitCell* c = Find(g, aCell); return IN_FLOOR + (c ? Cm(U(c->door[8])) : 0); }

// ---------------------------------------------------------------- the models, and everything placed
void Game::BuildKit()
    {
    R3D& r = *iR;
    int i, k;
    for (i = iLibMeshN; i < iKitMeshN && i < MAXKIT; i++) BuildKitMesh(i);   // (the library's: made once, LoadArea(.., 1))
    for (k = 0; k < iKitCellN; k++)
        {
        const KitCell& c = iKitCells[k];
        if (k < 32) iKitObj0[k] = -1;
        if (c.cell == CELL_NONE) continue;                                  // outside: PlaceKitOutside, with the land
        int o0 = -1, inrow = 1;
        for (i = 0; i < c.nstat; i++)
            {
            const int* q = c.stat + i * 7;
            if (q[0] >= MAXKIT) { inrow = 0; continue; }
            int x = Ix(U(q[1])), z = Iz(U(q[2]));
            int o = r.AddInterior(c.cell, iKitType[q[0]], x, IN_FLOOR + Cm(U(q[3])), z, SY(q[4]), q[5], 0);
            if (i == 0) o0 = o;
            if (o < 0 || o != o0 + i) inrow = 0;                            // (what can be seen from where: by its place in the row)
            if (k < 32 && i == c.nstat - 1 && inrow && o0 >= 0) iKitObj0[k] = o0;
            if (o >= 0 && !q[6]) r.iIObj[o].pad = (u8)GameZoneOf(c.cell, x, z);    // the inn's rooms: drawn when seen
            }
        for (i = 0; i < c.nflame; i++)
            {
            const int* q = c.flame + i * 4;
            IPlace(c.cell, IN_FLAME, U(q[0]), U(q[2]), U(q[1]), 0, q[3]);
            }
        for (i = 0; i < c.nlight; i++)
            {
            const int* q = c.light + i * 4;
            ILight(c.cell, U(q[0]), U(q[2]), U(q[1]), q[3] ? 1100 : 700, q[3] ? 210 : 110);
            }
        }
    }
// one of Skyrim's models (the library's or the area's): its mesh and its kind of static
void Game::BuildKitMesh(int i)
    {
    R3D& r = *iR;
    int k;
        {
        const KitMesh& km = KitM(i);
        r.MeshFromConst(iMKit[i], km.v, km.nv, km.f, km.nf, km.n, km.uv);     // drawn from the program's own data
        iMKit[i].uvsh = 2;                                                  // (v0.4.46: texel coordinates in quarters)
        r.MeshClusters(iMKit[i]);                                           // (v0.4.43: big ones, in clusters of 32 faces)
        StaticDef d;
        r3d_memset(&d, 0, sizeof(d));
        // the small things only near: a candle or a cup from 11 m, a chair or a barrel from 22 m, the building always
        int rad = iMKit[i].radius;
        d.mesh = &iMKit[i]; d.drawDist = rad < 6 * M / 10 ? 11 : (rad < 12 * M / 10 ? 22 : 60);
        for (k = 0; k < 32; k++) d.slots[k] = km.slots[k];
        d.hx = km.hx; d.hz = km.hz; d.hy = km.hy; d.bx = km.bx; d.bz = km.bz;
        // (lod: the far model's number; v0.4.46: and the middle one's + 1 in its top half - the buildings')
        int far = km.lod < 0 ? -1 : (km.lod & 0xFFFF), mid = km.lod < 0 ? -1 : (km.lod >> 16) - 1;
        if (far >= iKitMeshN || far >= MAXKIT || (i < iLibMeshN && far >= iLibMeshN)) far = -1;
        if (mid >= iKitMeshN || mid >= MAXKIT || (i < iLibMeshN && mid >= iLibMeshN)) mid = -1;
        // (v0.4.46: the distances from the Model detail page - R3D::iLodNear, iLodFar[lodCat])
        if (far >= 0 && rad < 25 * M / 10) { d.lod = &iMKit[far]; d.lodDist = 5; d.lodCat = LODC_FURN; }       // furniture: a plainer one past 5 m
        else if (far >= 0)
            {
            // a building seen from the street: its middle model from 3 m off its nearest side, its far one from 10 m; a
            // tree its far one from 12 m; the rocks and the cliffs from 6 m
            d.lod = &iMKit[far]; d.drawDist = 400;
            if (mid >= 0) { d.mid = &iMKit[mid]; d.midDist = 3; d.lodDist = 10; d.lodCat = LODC_BUILDING; }
            else { d.lodDist = km.hx ? 12 : 6; d.lodCat = km.hx ? LODC_TREE : LODC_ROCK; }
            }
        if (km.spin) { d.flags = SD_SPINZ; d.spinMs = 9000; d.pivZ = (short)km.pivz; d.yOff = (short)km.yoff; d.drawDist = 200; }
        iKitType[i] = r.RegisterStatic(d);
        }
    }

// (v0.4.74) a kit cell's "what can be seen" for feet at (aX, aZ, aFeet) (engine units): the region there, or the band
// above or below it (a jump, a fall); 0 off the floor's regions
static const u8* KitRegionBits(const KitCell& c, int aX, int aZ, int aFeet)
    {
    int ux = ToU(aX - IN_OX), uy = ToU(aZ - IN_OZ), feet = ToU(aFeet - IN_FLOOR);
    int sx = ux >= 0 ? ux / c.pvsRS : -((-ux + c.pvsRS - 1) / c.pvsRS), sy = uy >= 0 ? uy / c.pvsRS : -((-uy + c.pvsRS - 1) / c.pvsRS);
    int bd = feet >= 0 ? feet / c.pvsBH : -((-feet + c.pvsBH - 1) / c.pvsBH);
    static const signed char KTry[3] = { 0, -1, 1 };
    int t;
    for (t = 0; t < 3; t++)
        {
        int b = bd + KTry[t], lo = 0, hi = c.pvsN - 1;
        while (lo <= hi)
            {
            int mid = (lo + hi) >> 1;
            const short* e = c.pvsReg + mid * 4;
            int d = e[0] != sx ? e[0] - sx : (e[1] != sy ? e[1] - sy : e[2] - b);
            if (d == 0) return c.pvs + mid * ((c.nstat + 7) >> 3);
            if (d < 0) lo = mid + 1; else hi = mid - 1;
            }
        }
    return 0;
    }
// (v0.4.55) the pieces that can be seen from where the player is (host/pvs.cpp): the region under the feet, or the
// band above or below it (a jump, a fall); anywhere else - off the floor's regions - all of them
void GameKitPvs(Game& g)
    {
    R3D& r = *g.iR;
    r.iPvsBits = 0; r.iOutPvsBits = 0;
    if (g.iUi == UI_MAP) return;
    if (!g.iS.cell)
        {
        // (v0.4.65) outside: Skyrim's world units, the feet over the land's offset
        if (!g.iOutPvs || g.iOutObjN != g.iOutPvsObjN || g.iOutObjMark < 0) return;
        int off = r.iLandLoaded ? r.iLandOffset : 0;
        int ux = (int)((long long)g.iS.px * 10000 / 36576) - 233472, uy = (int)((long long)g.iS.pz * 10000 / 36576) - 176128;
        int feet = ToU(g.iS.py - 410 - off);
        int RS = g.iOutPvsRS, BH = g.iOutPvsBH;
        int sx = ux >= 0 ? ux / RS : -((-ux + RS - 1) / RS), sy = uy >= 0 ? uy / RS : -((-uy + RS - 1) / RS);
        int bd = feet >= 0 ? feet / BH : -((-feet + BH - 1) / BH);
        static const signed char KTryO[3] = { 0, -1, 1 };
        int t;
        for (t = 0; t < 3; t++)
            {
            int b = bd + KTryO[t], lo = 0, hi = g.iOutPvsRegN - 1;
            while (lo <= hi)
                {
                int mid = (lo + hi) >> 1;
                const short* e = g.iOutPvsReg + mid * 4;
                int d = e[0] != sx ? e[0] - sx : (e[1] != sy ? e[1] - sy : e[2] - b);
                if (d == 0) { r.iOutPvsBits = g.iOutPvs + mid * ((g.iOutPvsObjN + 7) >> 3); r.iOutPvsFirst = g.iOutObjMark; r.iOutPvsN = g.iOutPvsObjN; return; }
                if (d < 0) lo = mid + 1; else hi = mid - 1;
                }
            }
        return;
        }
    int k;
    for (k = 0; k < g.iKitCellN && k < 32; k++)
        {
        const KitCell& c = g.iKitCells[k];
        if (c.cell != g.iS.cell) continue;
        if (!c.pvs || g.iKitObj0[k] < 0) return;
        const u8* bits = KitRegionBits(c, g.iS.px, g.iS.pz, g.iS.py - 410);
        if (!bits) return;
        r.iPvsBits = bits; r.iPvsFirst = g.iKitObj0[k]; r.iPvsN = c.nstat;
        // (v0.4.74) the ruin's doors hide what is behind them (host/pvs counts them shut): an open one within 40 m adds
        // what can be seen from either side of it - its far side's view holds all that shows through the opening
        int nb = (c.nstat + 7) >> 3, i, mixed = 0;
        if (nb > R3D::PVSMIX) return;
        for (i = 0; i < g.iS.nprop; i++)
            {
            const Prop& p = g.iS.prop[i];
            if (p.type != PT_DOOR || p.count <= 0 || p.cell != c.cell || p.anim <= 0) continue;
            if (Abs(p.x - g.iS.px) > 40 * METER || Abs(p.z - g.iS.pz) > 40 * METER) continue;
            int cs = r3d_cos(p.yaw), sn = r3d_sin(p.yaw), lx = Cm(p.vx), lz = Cm(p.vz);
            int mx = p.x - ((lx * cs + lz * sn) >> 14), mz = p.z - ((-lx * sn + lz * cs) >> 14);
            int s;
            for (s = -1; s <= 1; s += 2)
                {
                // (the leaf along the model's z: across it, its x - 1.5 m out each way)
                const u8* b2 = KitRegionBits(c, mx + s * ((cs * 3 * METER / 2) >> 14), mz - s * ((sn * 3 * METER / 2) >> 14), p.y);
                if (!b2 || b2 == bits) continue;
                if (!mixed) { r3d_memmove(r.iPvsMix, bits, nb); mixed = 1; }
                int j;
                for (j = 0; j < nb; j++) r.iPvsMix[j] |= b2[j];
                }
            }
        if (mixed) r.iPvsBits = r.iPvsMix;
        return;
        }
    }

// Riverwood outside: the buildings among the streets, each at Skyrim's own place and height (once the land is in)
void Game::PlaceKitOutside()
    {
    R3D& r = *iR;
    int off = r.iLandLoaded ? r.iLandOffset : 0;
    int k;
    for (k = 0; k < iKitCellN; k++)
        {
        const KitCell* c = &iKitCells[k];
        if (c->cell == CELL_NONE) PlaceKitOut(c, off);
        }
    }
// one place outside (Riverwood, Bleak Falls Barrow): its trees and rocks stand where Skyrim places them, so none of
// the land's grown ones in its bounds (its "door" row: x0, y0, x1, y1 from its origin); then its pieces
void Game::PlaceKitOut(const KitCell* c, int off)
    {
    R3D& r = *iR;
    const int* b = c->door;
    if ((b[0] || b[2]) && !r.iLandBaked)                           // (baked into the land file)
        {
        int x0 = WorldX(c->ox + b[0]) / CELL, x1 = WorldX(c->ox + b[2]) / CELL, z0 = WorldZ(c->oy + b[1]) / CELL, z1 = WorldZ(c->oy + b[3]) / CELL, x, z;
        r.FocusLand((x0 + x1) / 2 * CELL, (z0 + z1) / 2 * CELL);
        for (z = z0; z <= z1; z++)
            for (x = x0; x <= x1; x++)
                if (x >= 0 && z >= 0 && x <= r.iGX && z <= r.iGZ && r.InWin(x, z)) r.iMat[z * r.iHS + x] |= VM_NOTREE;
        }
    int i;
    for (i = 0; i < c->nstat; i++)
        {
        const int* q = c->stat + i * 7;
        if (q[0] >= MAXKIT) continue;
        int o = r.AddStatic(iKitType[q[0]], WorldX(c->ox + q[1]), WorldZ(c->oy + q[2]), SY(q[4]), q[5], 0);
        if (o >= 0) r.iObj[o].y = Cm(U(q[3])) + off;
        }
    // the campfire's flames
    for (i = 0; i < c->nflame; i++)
        {
        const int* q = c->flame + i * 4;
        int o = r.AddStatic(ST_FIRE, WorldX(c->ox + q[0]), WorldZ(c->oy + q[1]), 0, 256, 0);
        if (o >= 0) r.iObj[o].y = Cm(U(q[2])) + off;
        }
    }

const ItemDef& GameItem(int aId);
void Game::SpawnKit()
    {
    int i, k;
    for (k = 0; k < iKitCellN; k++)
        {
        const KitCell& c = iKitCells[k];
        for (i = 0; i < c.nprop; i++)
            {
            const int* q = c.prop + i * 6;
            int t = q[0], item = q[5], yaw = SY(q[4]);
            if (t == PT_CHAIR || t == PT_BENCH || t == PT_BED) yaw = (yaw + 512) & 1023;
            if (t == PT_DOOR) yaw = (256 - SY(q[4])) & 1023;
            if (t == PT_BARREL || t == PT_CHEST || t == PT_CUPBOARD || t == PT_DRESSER || t == PT_SACK)
                {
                int cn = NewCont();
                switch (t)
                    {
                    case PT_BARREL: ContAdd(cn, IT_APPLE, 2); ContAdd(cn, IT_POTATO, 2); break;
                    case PT_SACK: ContAdd(cn, IT_WHEAT, 2); break;
                    case PT_CUPBOARD: case PT_DRESSER: ContAdd(cn, IT_FUR_BOOTS, 1); break;
                    default: ContAdd(cn, IT_GARNET, 1); break;
                    }
                item = cn;
                }
            int model = -1, cnt = 0;
            if (t == PT_HARVEST)
                {
                // (gen_kit.py: its model * 16 + what it gives: 1 rabbit, 2 pheasant, 3..5 a small, medium, large purse)
                model = item >> 4;
                int what = item & 15;
                item = what == 1 ? IT_RAW_RABBIT : what == 2 ? IT_PHEASANT : -1;          // (-1: a purse: gold)
                cnt = what >= 3 ? what - 3 : 1;
                }
            int x = U(q[1]), z = U(q[2]);
            if (t == PT_DOOR && item > 0)
                {
                // (v0.4.74) a ruin's door: on its hinge (the leaf's end, across its middle), turned as Skyrim has it, shut;
                // drawn with Skyrim's model swung about the hinge (DrawProps), its leaf the width of the door
                int mi = item & 2047, w = (item >> 11) & 511, hx = ((item >> 20) & 511) - 256;
                int y2 = SY(q[4]), cs = r3d_cos(y2), sn = r3d_sin(y2);
                int lx = U(hx), lz = -U(w / 2);
                int pi = AddProp(PT_DOOR, c.cell, Ix(x + ((lx * cs + lz * sn) >> 14)), IN_FLOOR + Cm(U(q[3])), Iz(z + ((-lx * sn + lz * cs) >> 14)), y2, 0, U(w), 0);
                if (pi >= 0) { Prop& p = iS.prop[pi]; p.state = 1; p.model = (short)mi; p.vx = lx; p.vz = lz; }
                continue;
                }
            if (t == PT_DOOR) { x -= (r3d_sin(yaw) * 54) >> 14; z -= (r3d_cos(yaw) * 54) >> 14; }
            int pi = AddProp(t, c.cell, Ix(x), IN_FLOOR + Cm(U(q[3])), Iz(z), yaw, item, cnt, t == PT_DOOR ? 0 : PF_OWNED);
            if (pi >= 0 && t == PT_DOOR) iS.prop[pi].state = 1;                     // shut
            if (pi >= 0) iS.prop[pi].model = (short)model;
            }
        for (i = 0; i < c.nitem; i++)
            {
            const int* q = c.item + i * 4;
            AddProp(PT_ITEM, c.cell, Ix(U(q[1])), IN_FLOOR + Cm(U(q[3])) + iItemRest[GameItem(q[0]).mesh * 3 + 1], Iz(U(q[2])), i * 97, q[0], 1, PF_OWNED);
            }
        for (i = 0; i < c.npeople; i++)
            {
            const int* q = c.people + i * 5;
            int idx = SpawnEnt(E_VILLAGER, Ix(U(q[1])), Iz(U(q[2])), SY(q[4]));
            if (idx < 0) continue;
            Ent& e = iS.e[idx];
            e.cell = c.cell; e.y = IN_FLOOR + Cm(U(q[3]));
            e.role = ROLE_NAMED; e.pad1 = (u8)q[0]; e.pad2 = 0; e.var = (u8)(q[0] == 17 || q[0] == 12 || q[0] == 14 || q[0] == 21 ? 1 : 0);
            }
        }
    }

int GameKitPersonSpot(const Game& g, int aWho, int* aCell, int* aX, int* aY, int* aZ)
    {
    int k, i;
    for (k = 0; k < g.iKitCellN; k++)
        {
        const KitCell& c = g.iKitCells[k];
        for (i = 0; i < c.npeople; i++)
            {
            const int* q = c.people + i * 5;
            if (q[0] != aWho) continue;
            *aCell = c.cell; *aX = Ix(U(q[1])); *aZ = Iz(U(q[2])); *aY = IN_FLOOR + Cm(U(q[3]));
            return 1;
            }
        }
    return 0;
    }

// ---------------------------------------------------------------- floors and walls: the building pieces' own
// the square of the grid a point (Skyrim units) is in, or -1
static int Square(const KitCell* c, int px, int py)
    {
    const int* g = c->grid;
    int gx = (px - g[0]) >> g[4], gy = (py - g[1]) >> g[4];       // squares of 2^g[4] units
    if (gx < 0 || gy < 0 || gx >= g[2] || gy >= g[3]) return -1;
    return gy * g[2] + gx;
    }

// the floor triangle under one point: the highest a step up or less (best), and the lowest there (low)
static void FloorOne(const KitCell* c, int px, int py, int feet, int& best, int& low)
    {
    int sq = Square(c, px, py);
    if (sq < 0) return;
    const int* g = c->grid + 5;
    int k, k1 = g[sq + 1];
    for (k = g[sq]; k < k1; k++)
        {
        const short* t = c->floor + c->glist[k] * 9;
        int x0 = t[0], y0 = t[1], x1 = t[3], y1 = t[4], x2 = t[6], y2 = t[7];
        if ((px < x0 && px < x1 && px < x2) || (px > x0 && px > x1 && px > x2) || (py < y0 && py < y1 && py < y2) || (py > y0 && py > y1 && py > y2)) continue;
        int d = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2);
        if (d == 0) continue;
        int w0 = (y1 - y2) * (px - x2) + (x2 - x1) * (py - y2);
        int w1 = (y2 - y0) * (px - x2) + (x0 - x2) * (py - y2);
        int w2 = d - w0 - w1;
        if (d > 0 ? (w0 < 0 || w1 < 0 || w2 < 0) : (w0 > 0 || w1 > 0 || w2 > 0)) continue;
        int z = (int)(((long long)w0 * t[2] + (long long)w1 * t[5] + (long long)w2 * t[8]) / d);
        if (z <= feet + 32 && z > best) best = z;
        if (z < low) low = z;
        }
    }

// the floor under (px, py) for feet at `feet` (all in the cell's units): the highest floor a step up or less, looked
// for at the point and 10 units round it (the stairs' steps are log tops with gaps between); else, when aLowest, the
// lowest one under the point itself. 0 when there is none.
static int FloorU(const KitCell* c, int px, int py, int feet, int aLowest, int& z)
    {
    int best = -100000, low = 100000;
    FloorOne(c, px, py, feet, best, low);
    int lowHere = low;
    FloorOne(c, px + 10, py, feet, best, low); FloorOne(c, px - 10, py, feet, best, low);
    FloorOne(c, px, py + 10, feet, best, low); FloorOne(c, px, py - 10, feet, best, low);
    if (best > -100000) { z = best; return 1; }
    if (!aLowest) return 0;
    if (lowHere < 100000) { z = lowHere; return 1; }
    if (low < 100000) { z = low; return 1; }
    return 0;
    }

int GameKitFloor(const Game& g, int aCell, int aX, int aZ, int aFeetY, int* aFound)
    {
    const KitCell* c = Find(g, aCell);
    *aFound = 0;
    if (!c) return IN_FLOOR;
    int feet = ToU(aFeetY - IN_FLOOR), z;
    if (!FloorU(c, ToU(aX - IN_OX), ToU(aZ - IN_OZ), feet, 1, z)) return IN_FLOOR + Cm(U(feet));
    *aFound = 1;
    return IN_FLOOR + Cm(U(z));
    }

// a wall in the way of a body of radius r whose feet are at `feet` (the cell's units): walls that rise above a step
// and come below the head
static int BlockedU(const KitCell* c, int px, int py, int r, int feet)
    {
    const int* g = c->grid;
    int sq = Square(c, px, py), n = g[2] * g[3];
    if (sq < 0 || r > 40) return 0;
    int k, k1 = g[5 + n + 1 + sq + 1];
    for (k = g[5 + n + 1 + sq]; k < k1; k++)
        {
        const short* w = c->wall + c->glist[k] * 6;
        if (w[4] > w[5]) { if (feet >= w[4] || feet <= w[5]) continue; }     // (v0.4.55: a navmesh's edge: the feet at its height)
        else if (w[5] <= feet + 30 || w[4] >= feet + 120) continue;
        int x0 = w[0], y0 = w[1], x1 = w[2], y1 = w[3];
        if (px + r < (x0 < x1 ? x0 : x1) || px - r > (x0 > x1 ? x0 : x1) || py + r < (y0 < y1 ? y0 : y1) || py - r > (y0 > y1 ? y0 : y1)) continue;
        int dx = x1 - x0, dy = y1 - y0, l2 = dx * dx + dy * dy;
        int t = (px - x0) * dx + (py - y0) * dy;
        int cx, cy;
        if (t <= 0 || l2 == 0) { cx = x0; cy = y0; }
        else if (t >= l2) { cx = x1; cy = y1; }
        else { cx = x0 + (int)((long long)dx * t / l2); cy = y0 + (int)((long long)dy * t / l2); }
        int ex = px - cx, ey = py - cy;
        if (ex * ex + ey * ey < r * r) return 1;
        }
    return 0;
    }

// (a body a little slimmer than the game's 39 cm: Skyrim's own is about 30 cm, and its log stairs are 1.1 m wide)
int GameKitBlocked(const Game& g, int aCell, int aX, int aZ, int aR, int aFeetY)
    {
    const KitCell* c = Find(g, aCell);
    if (!c) return 0;
    return BlockedU(c, ToU(aX - IN_OX), ToU(aZ - IN_OZ), ToU(aR) * 3 / 4, ToU(aFeetY - IN_FLOOR));
    }

// outside: Riverwood's decks, porches, boardwalks and steps, a step up or less from the feet (else none: the land);
// aOff: the land's height offset (engine units)
// the places outside built from Skyrim's pieces (Riverwood, Bleak Falls Barrow ...): every outdoor cell whose
// squares take in the point
int GameKitOutFloor(const Game& g, int aX, int aZ, int aFeetY, int aOff, int* aFound)
    {
    *aFound = 0;
    int i, best = 0;
    for (i = 0; i < g.iKitCellN; i++)
        {
        const KitCell* c = &g.iKitCells[i];
        if (c->cell != CELL_NONE) continue;
        int px = OutU(aX, c->ox, 233472), py = OutU(aZ, c->oy, 176128), z;
        if (Square(c, px, py) < 0) continue;
        if (!FloorU(c, px, py, ToU(aFeetY - aOff), 0, z)) continue;
        int y = Cm(U(z)) + aOff;
        if (!*aFound || y > best) best = y;
        *aFound = 1;
        }
    return best;
    }
int GameKitOutBlocked(const Game& g, int aX, int aZ, int aR, int aFeetY, int aOff)
    {
    int i;
    for (i = 0; i < g.iKitCellN; i++)
        {
        const KitCell* c = &g.iKitCells[i];
        if (c->cell != CELL_NONE) continue;
        int px = OutU(aX, c->ox, 233472), py = OutU(aZ, c->oy, 176128);
        if (Square(c, px, py) < 0) continue;
        if (BlockedU(c, px, py, ToU(aR) * 3 / 4, ToU(aFeetY - aOff))) return 1;
        }
    return 0;
    }

// the outdoor walls (for the local map): how many with i < 0, else wall i's ends in the world and its height (engine)
int GameKitOutWall(const Game& g, int i, int* aX0, int* aZ0, int* aX1, int* aZ1)
    {
    int k, n = 0;
    const KitCell* c = 0;
    for (k = 0; k < g.iKitCellN; k++)
        {
        const KitCell* t = &g.iKitCells[k];
        if (t->cell != CELL_NONE) continue;
        if (i >= 0 && i >= n && i < n + t->nwall) { c = t; i -= n; break; }
        n += t->nwall;
        }
    if (i < 0) return n;
    if (!c) return 0;
    const short* w = c->wall + i * 6;
    *aX0 = WorldX(w[0] + c->ox); *aZ0 = WorldZ(w[1] + c->oy); *aX1 = WorldX(w[2] + c->ox); *aZ1 = WorldZ(w[3] + c->oy);
    return (w[5] - w[4]) * M / 70;
    }

// ---------------------------------------------------------------- the doors: Skyrim's, both sides
int GameKitDoors(const Game& g, DoorLink* d, int aMax)
    {
    int k, n = 0;
    for (k = 0; k < g.iKitCellN && n < aMax; k++)
        {
        const KitCell& c = g.iKitCells[k];
        const int* q = c.door;
        if (c.cell == CELL_NONE || (!q[0] && !q[1] && !q[4])) continue;
        DoorLink& l = d[n++];
        l.cell = c.cell; l.ocell = 0;
        l.ix = Ix(U(q[0])); l.iz = Iz(U(q[1]));
        l.iyaw = r3d_atan2(q[6] - q[0], q[7] - q[1]) & 1023;            // inside, facing where Skyrim puts you in
        l.ox = WorldX(q[4]); l.oz = WorldZ(q[5]);
        l.oyaw = r3d_atan2(q[2] - q[4], q[3] - q[5]) & 1023;            // outside, facing where Skyrim puts you out
        l.name = c.name;
        }
    return n;
    }

// ---------------------------------------------------------------- the area's file (tools/kit_pack.py)
static int RdI(const u8* p) { return (int)((u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24)); }
static int RdS(const u8* p) { return (short)(p[0] | (p[1] << 8)); }
static const u16 KZeroUV[256 * 6] = { 0 };       // (the plants' faces are not textured)
// the file into iAreaMem, then its tables of models and cells after it (pointing into it): 1 if it was read
// (v0.4.56) aLib: the shared library (data/common.hlb, tools/lib_pack.py): its models numbered first (0..iLibMeshN-1,
// the areas' own after them), its textures the library's bank; read once, before any area
int Game::LoadArea(const char* aName, int aLib)
    {
    if (aLib) { iLibMeshes = 0; iLibMeshN = 0; if (iR) iR->SetLibTextures(0, 0, 0, 0, 0, 0, 0, 0, 0); }
    else
    {
    iKitMeshN = iLibMeshN; iKitCellN = 0; iKitMeshes = 0; iKitCells = 0;
    { void GameFloraColDefault(u8* aD); GameFloraColDefault(iFloraCol); int q; for (q = 0; q < FK_COUNT; q++) iFloraFar[q] = 0; }
    { for (int q = 0; q < IT_COUNT; q++) iItemKit[q] = -1; }
    if (iR) { iR->SetSkyTextures(0, 0, 0, 0); iR->iSkyKey = 0; iR->iSkyTex32 = iR->iSkyTex16 = 0; iR->iSkyHi2 = 0; iR->iSkyHi2N = 0; }
    }
    u8* mem = aLib ? iLibMem : iAreaMem;
    if (!iDataRead || !mem) return 0;
    const int cap = (aLib ? LIB_KB : AREA_KB) * 1024;
    int len = 0, n;
    // (v0.4.76) the whole file in one read where it can (the phone's reader opens, seeks, reads and closes the file each
    // call: a 6 MB area in 64 KB pieces was ~95 of them - the MP4 player's lesson: big reads)
    while (len < cap && (n = iDataRead(iDataCtx, aName, len, mem + len, cap - len)) > 0) len += n;
    const u8* b = mem;
    int ver = len >= 8 ? RdI(b + 4) : 0;
    if (len < 16 || b[0] != 'H' || b[1] != 'A' || b[2] != 'R' || b[3] != '1' || ver < 1 || ver > 3) return 0;
    int nm = RdI(b + 8), nc = RdI(b + 12);
    int tab = (len + 7) & ~7;
    if (tab + nm * (int)sizeof(KitMesh) + nc * (int)sizeof(KitCell) > cap) return 0;
    if (aLib && nc) return 0;
    if (!aLib && iLibMeshN + nm > MAXKIT) return 0;
    KitMesh* km = (KitMesh*)(mem + tab);
    KitCell* kc = (KitCell*)(mem + tab + nm * sizeof(KitMesh));
    int p = 16, i, k;
    #define AL() (p = (p + 3) & ~3)
    for (i = 0; i < nm; i++)
        {
        KitMesh& m = km[i];
        AL();
        m.nv = RdI(b + p); m.nf = RdI(b + p + 4); p += 8;
        for (k = 0; k < 32; k++) m.slots[k] = b[p + k];
        p += 32;
        m.hx = (short)RdS(b + p); m.hz = (short)RdS(b + p + 2); m.hy = (short)RdS(b + p + 4);
        m.bx = (short)RdS(b + p + 6); m.bz = (short)RdS(b + p + 8); p += 12;
        m.lod = RdI(b + p); m.spin = RdI(b + p + 4); m.pivz = RdI(b + p + 8); m.yoff = RdI(b + p + 12); p += 16;
        m.name = "";
        AL(); m.v = (const short*)(b + p); p += m.nv * 6;
        AL(); m.f = (const unsigned short*)(b + p); p += m.nf * 8;
        AL(); m.n = (const short*)(b + p); p += m.nf * 6;
        AL(); m.uv = (const unsigned short*)(b + p); p += m.nf * 12;
        if (p > len) return 0;
        }
    for (i = 0; i < nc; i++)
        {
        KitCell& c = kc[i];
        AL();
        c.cell = RdI(b + p); c.nstat = RdI(b + p + 4); c.nprop = RdI(b + p + 8); c.nitem = RdI(b + p + 12);
        c.nflame = RdI(b + p + 16); c.nlight = RdI(b + p + 20); c.npeople = RdI(b + p + 24);
        c.nfloor = RdI(b + p + 28); c.nwall = RdI(b + p + 32); c.ox = RdI(b + p + 36); c.oy = RdI(b + p + 40); p += 44;
        int L[12];
        for (k = 0; k < 12; k++) L[k] = RdI(b + p + 4 * k);
        p += 48;
        AL(); c.name = (const char*)(b + p); p += L[0];
        const int* ia[6];
        for (k = 0; k < 6; k++) { AL(); ia[k] = L[1 + k] ? (const int*)(b + p) : 0; p += L[1 + k] * 4; }
        c.stat = ia[0]; c.prop = ia[1]; c.item = ia[2]; c.flame = ia[3]; c.light = ia[4]; c.people = ia[5];
        AL(); c.floor = L[7] ? (const short*)(b + p) : 0; p += L[7] * 2;
        AL(); c.wall = L[8] ? (const short*)(b + p) : 0; p += L[8] * 2;
        AL(); c.door = L[9] ? (const int*)(b + p) : 0; p += L[9] * 4;
        AL(); c.grid = L[10] ? (const int*)(b + p) : 0; p += L[10] * 4;
        AL(); c.glist = L[11] ? (const unsigned short*)(b + p) : 0; p += L[11] * 2;
        if (p > len) return 0;
        }
    #undef AL
    if (ver >= 2)
        for (i = 0; i < nm && p + 2 <= len; i++)
            {
            int l = b[p] | (b[p + 1] << 8);
            if (p + 2 + l > len) break;
            km[i].name = (const char*)(b + p + 2); p += 2 + l;
            }
    if (ver >= 3)
        {
        // Skyrim's textures: "HTX1", n, the palette (768), the averages (3n, to a multiple of 4), n pictures of 4096
        p = (p + 3) & ~3;
        // (HTX2: then the n pictures blurred; HTX3: then nh and the first nh at 128 x 128)
        if (p + 8 <= len && b[p] == 'H' && b[p + 1] == 'T' && b[p + 2] == 'X' && b[p + 3] >= '1' && b[p + 3] <= '3')
            {
            int p0 = p, n = RdI(b + p + 4), two = b[p + 3] >= '2';
            int pt = (p + 8 + 768 + 3 * n + 3) & ~3;
            int ph = pt + n * 4096 * (1 + two), nh = 0;
            if (b[p + 3] == '3' && ph + 4 <= len) { nh = RdI(b + ph); ph += 4; if (nh < 0 || nh > n || ph + nh * 16384 > len) nh = 0; }
            int ok = n > 0 && n <= 64 && pt + n * 4096 * (1 + two) <= len;
            if (ok && !aLib) iR->SetSkyTextures(b + p + 8, b + pt, two ? b + pt + n * 4096 : 0, n, nh ? b + ph : 0, nh);
            p = ph + nh * 16384;
            const u8* key = 0; const u8* hi2 = 0; int n2 = 0;
            // (HTXK, v0.4.40: which are cut out - their texel 0 is the key)
            if (p + 8 <= len && b[p] == 'H' && b[p + 1] == 'T' && b[p + 2] == 'X' && b[p + 3] == 'K' && RdI(b + p + 4) == n && p + 8 + n <= len)
                { key = b + p + 8; p = (p + 8 + n + 3) & ~3; }
            // (HTXM, v0.4.42: each at 32 and at 16, tiled to 64 x 64)
            if (p + 8 <= len && b[p] == 'H' && b[p + 1] == 'T' && b[p + 2] == 'X' && b[p + 3] == 'M' && RdI(b + p + 4) == n && p + 8 + n * 8192 <= len)
                { if (!aLib) { iR->iSkyTex32 = b + p + 8; iR->iSkyTex16 = b + p + 8 + n * 4096; } p += 8 + n * 8192; }
            // (HTXH, v0.4.46: the first n2 at 256 x 256 - the signs, the pelts)
            p = (p + 3) & ~3;
            if (p + 8 <= len && b[p] == 'H' && b[p + 1] == 'T' && b[p + 2] == 'X' && b[p + 3] == 'H')
                {
                int m2 = RdI(b + p + 4);
                if (m2 > 0 && m2 <= nh && p + 8 + m2 * 65536 <= len) { hi2 = b + p + 8; n2 = m2; p += 8 + m2 * 65536; }
                }
            if (aLib) { if (ok) iR->SetLibTextures(b + p0 + 8, b + pt, two ? b + pt + n * 4096 : 0, n, nh ? b + ph : 0, nh, key, hi2, n2); }
            else { iR->iSkyKey = key; iR->iSkyHi2 = hi2; iR->iSkyHi2N = n2; }
            }
        if (aLib) { iLibMeshes = km; iLibMeshN = nm; iKitMeshN = nm; return 1; }
        // the land's plants (gen_kit.py: kit_flora.bin)
        p = (p + 3) & ~3;
        iFloraN = 0;
        if (p + 28 <= len && b[p] == 'H' && b[p + 1] == 'F' && b[p + 2] == 'L' && b[p + 3] == '1')
            {
            int n = RdI(b + p + 4), gw = RdI(b + p + 20), gh = RdI(b + p + 24);
            int pf = p + 28, pe = (pf + (gw * gh + 1) * 2 + 3) & ~3;
            if (n > 0 && gw > 0 && gh > 0 && pe + n * 6 <= len)
                {
                iFloraOX = RdI(b + p + 8); iFloraOZ = RdI(b + p + 12); iFloraSq = RdI(b + p + 16); iFloraGW = gw; iFloraGH = gh;
                iFloraFirst = (const u16*)(b + pf); iFloraEnt = b + pe; iFloraN = n > FLORA_MAX ? FLORA_MAX : n;
                RebuildFloraTaken();
                p = pe + n * 6;
                }
            }
        // the animals' places (gen_kit.py: after the plants)
        iAnimN = 0;
        if (p + 8 <= len && b[p] == 'H' && b[p + 1] == 'A' && b[p + 2] == 'N' && b[p + 3] == '1')
            {
            int n = RdI(b + p + 4);
            if (n > 0 && p + 8 + n * 12 <= len) { iAnimEnt = b + p + 8; iAnimN = n > ANIM_MAX ? ANIM_MAX : n; }
            r3d_memset(iAnimOut, 0, sizeof(iAnimOut));
            p += 8 + n * 12;
            }
        // each plant kind's look from Skyrim's (gen_kit.py flora_models): its colours; a bush's lobes (smooth), or its
        // leaf cards with their picture (cut out); and a far form
        p = (p + 3) & ~3;
        for (i = 0; i < FK_COUNT; i++) iFloraTex[i] = 255;
        if (p + 8 <= len && b[p] == 'H' && b[p + 1] == 'F' && b[p + 2] == 'M' && b[p + 3] == '2')
            {
            int nk = RdI(b + p + 4), k;
            p += 8;
            for (k = 0; k < nk && p + 12 <= len; k++)
                {
                const u8* c = b + p;
                int tex = b[p + 6], fl = b[p + 7];
                int nv = b[p + 8] | (b[p + 9] << 8), nf = b[p + 10] | (b[p + 11] << 8);
                p += 12;
                if (k < FK_COUNT && (c[0] | c[1] | c[2])) { int j; for (j = 0; j < 6; j++) iFloraCol[k * 6 + j] = c[j]; }
                if (!nv) continue;
                const short* v = (const short*)(b + p); p += nv * 6; p = (p + 3) & ~3;
                const u16* f = (const u16*)(b + p); p += nf * 8;
                const short* nn = (const short*)(b + p); p += nf * 6;
                const short* vn = 0; const u16* uv = KZeroUV;
                if (fl & 1) { vn = (const short*)(b + p); p += nv * 6; }
                if (fl & 2) { uv = (const u16*)(b + p); p += nf * 12; }
                p = (p + 3) & ~3;
                if (k < FK_COUNT && nf <= 512 && p <= len)
                    {
                    iR->MeshFromConst(iMFlora[k], v, nv, f, nf, nn, uv);
                    iMFlora[k].uvsh = 2;
                    iMFlora[k].vn = (short*)vn;                // (lobes: lit at their corners)
                    if (fl & 2) iFloraTex[k] = (u8)tex;
                    }
                // its far form
                if (p + 4 > len) break;
                int nv2 = b[p] | (b[p + 1] << 8), nf2 = b[p + 2] | (b[p + 3] << 8);
                p += 4;
                const short* v2 = (const short*)(b + p); p += nv2 * 6; p = (p + 3) & ~3;
                const u16* f2 = (const u16*)(b + p); p += nf2 * 8;
                const short* n2 = (const short*)(b + p); p += nf2 * 6;
                const u16* uv2 = KZeroUV;
                if (fl & 2) { uv2 = (const u16*)(b + p); p += nf2 * 12; }
                p = (p + 3) & ~3;
                if (k < FK_COUNT && nv2 && nf2 <= 256 && p <= len) { iR->MeshFromConst(iMFloraFar[k], v2, nv2, f2, nf2, n2, uv2); iMFloraFar[k].uvsh = 2; iFloraFar[k] = 1; }
                }
            }
        // (v0.4.66) the land's grown trees on the Rift's ground: drawn as Skyrim's aspen (its middling one)
        iR->iFallTree[0] = iR->iFallTree[1] = 0;
        {
        const int ka = FK_TREE0 + 1;
        if (iFloraTex[ka] != 255 && iMFlora[ka].nf)
            {
            const u8* c = iFloraCol + ka * 6;
            u8* sl = iR->iFallSl;
            sl[0] = c[0]; sl[1] = c[1]; sl[2] = c[2]; sl[3] = (u8)(iFloraTex[ka] < 0x40 ? 0xC0 | iFloraTex[ka] : iFloraTex[ka]);
            sl[4] = c[3]; sl[5] = c[4]; sl[6] = c[5]; sl[7] = 1;
            iR->iFallTree[0] = &iMFlora[ka]; iR->iFallTree[1] = iFloraFar[ka] ? &iMFloraFar[ka] : 0;
            }
        }
        // the items' own models (gen_kit.py's kit_items.txt)
        p = (p + 3) & ~3;
        if (p + 8 <= len && b[p] == 'H' && b[p + 1] == 'I' && b[p + 2] == 'M' && b[p + 3] == '1')
            {
            int n = RdI(b + p + 4);
            for (i = 0; i < n && p + 12 + i * 4 <= len; i++)
                {
                int it = b[p + 8 + i * 4] | (b[p + 9 + i * 4] << 8), mk = b[p + 10 + i * 4] | (b[p + 11 + i * 4] << 8);
                if (it < IT_COUNT && mk < iLibMeshN + nm) iItemKit[it] = (short)mk;
                }
            }
        }
    // (v0.4.55) what can be seen from where (host/pvs.cpp): "HPVS" n, then per cell: cell, pieces, regions, the
    // regions' size and the bands' height, the regions (4 shorts each), their bits; found from the file's last 8 bytes
    // (where it starts, "PVS1")
    for (i = 0; i < nc; i++) { kc[i].pvs = 0; kc[i].pvsReg = 0; kc[i].pvsN = 0; kc[i].pvsRS = kc[i].pvsBH = 1; }
    if (!aLib) { iOutPvs = 0; iOutPvsReg = 0; iOutPvsRegN = 0; }
    if (len >= 24 && b[len - 4] == 'P' && b[len - 3] == 'V' && b[len - 2] == 'S' && b[len - 1] == '1')
        {
        int q = RdI(b + len - 8);
        if (q > 0 && !(q & 3) && q + 8 <= len - 8 && b[q] == 'H' && b[q + 1] == 'P' && b[q + 2] == 'V' && b[q + 3] == 'S')
            {
            int n = RdI(b + q + 4);
            q += 8;
            for (k = 0; k < n && q + 20 <= len - 8; k++)
                {
                int cell = RdI(b + q), nobj = RdI(b + q + 4), nreg = RdI(b + q + 8), rs = RdI(b + q + 12), bh = RdI(b + q + 16);
                q += 20;
                const short* reg = (const short*)(b + q); q += nreg * 8;
                const u8* bits = b + q; q += (nreg * ((nobj + 7) >> 3) + 3) & ~3;
                if (q > len - 8 || nreg < 0 || rs <= 0 || bh <= 0) break;
                for (i = 0; i < nc; i++)
                    if (kc[i].cell == cell && kc[i].nstat == nobj) { kc[i].pvsReg = reg; kc[i].pvs = bits; kc[i].pvsN = nreg; kc[i].pvsRS = rs; kc[i].pvsBH = bh; }
                // (v0.4.65) the outside's (cell 0x7FFF): over all the statics the area puts outside, in order
                if (cell == 0x7FFF) { iOutPvsReg = reg; iOutPvs = bits; iOutPvsRegN = nreg; iOutPvsRS = rs; iOutPvsBH = bh; iOutPvsObjN = nobj; }
                }
            }
        }
    iKitMeshes = km; iKitMeshN = iLibMeshN + nm; iKitCells = kc; iKitCellN = nc;
    iDoorsInit = 0;                         // the door table again, with the area's doors
    return 1;
    }

// ---------------------------------------------------------------- what the crosshair is on (X: its name shown)
// the line of sight against one model's triangles (the line in the model's own frame, units; its direction Q14):
// how far along it the nearest is hit (units), or -1
static int RayMesh(const Mesh& m, int ex, int ey, int ez, int dx, int dy, int dz)
    {
    const short* v = m.v;
    const u16* f = m.f;
    int k, best = -1;
    for (k = 0; k < m.nf; k++)
        {
        const short* A = v + f[k * 4] * 3; const short* B = v + f[k * 4 + 1] * 3; const short* C = v + f[k * 4 + 2] * 3;
        long long e1x = B[0] - A[0], e1y = B[1] - A[1], e1z = B[2] - A[2];
        long long e2x = C[0] - A[0], e2y = C[1] - A[1], e2z = C[2] - A[2];
        long long px = dy * e2z - dz * e2y, py = dz * e2x - dx * e2z, pz = dx * e2y - dy * e2x;
        long long det = e1x * px + e1y * py + e1z * pz;
        if (det == 0) continue;
        long long tx = ex - A[0], ty = ey - A[1], tz = ez - A[2];
        long long u = tx * px + ty * py + tz * pz;
        long long qx = ty * e1z - tz * e1y, qy = tz * e1x - tx * e1z, qz = tx * e1y - ty * e1x;
        long long w = dx * qx + dy * qy + dz * qz;
        long long tt = e2x * qx + e2y * qy + e2z * qz;
        if (det < 0) { det = -det; u = -u; w = -w; tt = -tt; }
        if (u < 0 || w < 0 || u + w > det || tt <= 0) continue;
        int t = (int)((tt << 14) / det);
        if (best < 0 || t < best) best = t;
        }
    return best;
    }

// the Skyrim model the crosshair is on: the nearest triangle of the area's models along the line of sight within
// 12 m (each thing's bounding sphere first, then its triangles): its editor id and file, or 0
const char* Game::LookedAtModel()
    {
    R3D& r = *iR;
    int ey = iS.py - iCrouchY;
    int cp = r3d_cos(iS.pitch), sp = r3d_sin(iS.pitch);
    int dx = (r3d_sin(iS.yaw) * cp) >> 14, dy = sp, dz = (r3d_cos(iS.yaw) * cp) >> 14;   // Q14
    const Obj* list = iS.cell ? r.iIObj : r.iObj;
    int n = iS.cell ? r.iINum : r.iObjCount, i, k;
    int best = -1, bestT = 12 * METER;
    for (i = 0; i < n; i++)
        {
        const Obj& o = list[i];
        if (iS.cell && o.flag != iS.cell) continue;
        if (o.type < ST_CUSTOM) continue;
        const StaticDef& d = r.iDefs[o.type - ST_CUSTOM];
        const Mesh* m = d.mesh;
        if (!m || !m->v || o.scale <= 0) continue;
        int cs = r3d_cos(o.yaw), sn = r3d_sin(o.yaw);
        int mx = (m->cx * o.scale) >> 8, mz = (m->cz * o.scale) >> 8;
        int cx = o.x + ((mx * cs + mz * sn) >> 14), cz = o.z + ((-mx * sn + mz * cs) >> 14), cy = o.y + ((m->cy * o.scale) >> 8);
        int rad = (m->brad * o.scale) >> 8;
        int ox = (cx - iS.px) >> 4, oy = (cy - ey) >> 4, oz = (cz - iS.pz) >> 4, rr = rad >> 4;   // (1/16 units)
        int t = (ox * dx + oy * dy + oz * dz) >> 14;
        int d2 = ox * ox + oy * oy + oz * oz;
        if (d2 > rr * rr && (t <= 0 || d2 - t * t > rr * rr || t - rr > bestT / 16)) continue;
        // the line in the model's frame: turned back by its yaw, scaled back to its own size
        int wx = iS.px - o.x, wy = ey - o.y, wz = iS.pz - o.z;
        int lx = (int)(((long long)(wx * (long long)cs - wz * (long long)sn) >> 14) * 256 / o.scale);
        int lz = (int)(((long long)(wx * (long long)sn + wz * (long long)cs) >> 14) * 256 / o.scale);
        int ly = wy * 256 / o.scale;
        int ldx = (dx * cs - dz * sn) >> 14, ldz = (dx * sn + dz * cs) >> 14;
        int h = RayMesh(*m, lx, ly, lz, ldx, dy, ldz);
        if (h < 0) continue;
        h = h * o.scale >> 8;
        if (h < bestT) { bestT = h; best = i; }
        }
    if (best < 0) return 0;
    int type = list[best].type;
    for (k = 0; k < iKitMeshN && k < MAXKIT; k++)
        if (iKitType[k] == type) return KitM(k).name;
    return 0;
    }

// ---------------------------------------------------------------- the items' own models (data/items.hit)
// "HIT1", u32 n, n * (u16 item, u16 0, u32 offset, u32 size); each model as gen_kit.py's item_blob() writes it. The
// table is read once; a model when the inventory shows it (into iItemBuf), kept until another is shown.
const Mesh* Game::ItemModel(int aId)
    {
    if (aId < 0 || aId >= IT_COUNT || !iDataRead || !iItemBuf) return 0;
    if (iItemTab == 0)
        {
        int i;
        for (i = 0; i < IT_COUNT; i++) iItemOff[i] = iItemLen[i] = 0;
        iItemTab = -1;
        u8* b = iItemBuf;
        int n = iDataRead(iDataCtx, "items.hit", 0, b, 8);
        if (n == 8 && b[0] == 'H' && b[1] == 'I' && b[2] == 'T' && b[3] == '1')
            {
            int ne = RdI(b + 4);
            if (ne > 0 && 8 + ne * 12 <= ITEMBUF_KB * 1024 && iDataRead(iDataCtx, "items.hit", 8, b, ne * 12) == ne * 12)
                {
                for (i = 0; i < ne; i++)
                    {
                    const u8* e = b + i * 12;
                    int id = e[0] | (e[1] << 8);
                    if (id < IT_COUNT) { iItemOff[id] = (u32)RdI(e + 4); iItemLen[id] = (u32)RdI(e + 8); }
                    }
                iItemTab = 1;
                }
            }
        iItemShown = -1;
        }
    if (iItemTab < 0 || !iItemLen[aId] || iItemLen[aId] > ITEMBUF_KB * 1024u) return 0;
    if (iItemShown != aId)
        {
        iItemShown = -1;
        int len = (int)iItemLen[aId];
        if (iDataRead(iDataCtx, "items.hit", (int)iItemOff[aId], iItemBuf, len) != len) return 0;
        const u8* b = iItemBuf;
        int nv = b[0] | (b[1] << 8), nf = b[2] | (b[3] << 8), nt = b[4], hi = b[5];
        if (nv <= 0 || nv > MAXMV || nf <= 0) return 0;
        int p = 8;
        iItemSlots = b + p; p += 32;
        // its palette lit 16 ways (no fog: the preview has none), where R3D's iSkyShade has them
        const u8* pal = b + p; p += 768;
        int i, l;
        for (l = 0; l < 16; l++)
            {
            u16* q = iItemShade + l * 8 * 256;
            int lk = 40 + l * 16;
            for (i = 0; i < 256; i++)
                {
                int rr = pal[i * 3] * lk >> 8, gg = pal[i * 3 + 1] * lk >> 8, bb = pal[i * 3 + 2] * lk >> 8;
                if (rr > 255) rr = 255;
                if (gg > 255) gg = 255;
                if (bb > 255) bb = 255;
                q[i] = (u16)(((rr & 0xF8) << 8) | ((gg & 0xFC) << 3) | (bb >> 3));
                }
            }
        const short* v = (const short*)(b + p); p += nv * 6; p = (p + 3) & ~3;
        const u16* f = (const u16*)(b + p); p += nf * 8;
        const short* n = (const short*)(b + p); p += nf * 6;
        const u16* uv = (const u16*)(b + p); p += nf * 12; p = (p + 3) & ~3;
        iItemTexN = nt; iItemTex = b + p; iItemTex1 = b + p + nt * 4096; iItemHi = hi ? b + p + nt * 8192 : 0; iItemHiN = hi ? nt : 0;
        if (p + nt * 8192 + (hi ? nt * 16384 : 0) > len) return 0;
        iR->MeshFromConst(iMItemView, v, nv, f, nf, n, uv);
        iMItemView.uvsh = 2;
        iItemShown = aId;
        }
    return &iMItemView;
    }

// a model from ItemModel() in the inventory's preview: with its own textures whatever the Textures setting (one
// thing, seen big)
void Game::DrawItemView(const Mesh& m, const u8* aSlots, int x, int y, int z, int aYaw, int aPitch, int aRoll)
    {
    R3D& r = *iR;
    if (&m != &iMItemView) { r.DrawMeshView(m, 0, x, y, z, aYaw, aPitch, aRoll, aSlots, 0); return; }
    const u8* t0 = r.iSkyTex; const u8* t1 = r.iSkyTex1; const u8* th = r.iSkyHi; u16* sh = r.iSkyShade; const u8* ky = r.iSkyKey;
    const u8* t32 = r.iSkyTex32; const u8* t16 = r.iSkyTex16; const u8* t256 = r.iSkyHi2; int n256 = r.iSkyHi2N;
    r.iSkyKey = 0; r.iSkyTex32 = r.iSkyTex16 = 0; r.iSkyHi2 = 0; r.iSkyHi2N = 0;
    int n0 = r.iSkyN, nh = r.iSkyHiN, td = r.iTexDist, nt = r.iNoTex;
    r.iSkyTex = iItemTex; r.iSkyTex1 = iItemTex1; r.iSkyHi = iItemHi; r.iSkyN = iItemTexN; r.iSkyHiN = iItemHiN; r.iSkyShade = iItemShade;
    r.iTexDist = 1 << 30; r.iNoTex = 0;
    r.DrawMeshView(m, 0, x, y, z, aYaw, aPitch, aRoll, iItemSlots, 0);
    r.iSkyTex = t0; r.iSkyTex1 = t1; r.iSkyHi = th; r.iSkyN = n0; r.iSkyHiN = nh; r.iTexDist = td; r.iNoTex = nt; r.iSkyShade = sh; r.iSkyKey = ky; r.iSkyTex32 = t32; r.iSkyTex16 = t16; r.iSkyHi2 = t256; r.iSkyHi2N = n256;
    }

// (v0.4.74) why a point is blocked by a kit's walls (the probes): 0 not; 1 a wall; 2 a navmesh's edge. aCell 0: outside
int GameKitWhy(const Game& g, int aCell, int aX, int aZ, int aR, int aFeetY, int aOff, int* aWall)
    {
    int i;
    for (i = 0; i < g.iKitCellN; i++)
        {
        const KitCell* c = &g.iKitCells[i];
        int px, py, feet;
        if (aCell) { if (c->cell != aCell) continue; px = ToU(aX - IN_OX); py = ToU(aZ - IN_OZ); feet = ToU(aFeetY - IN_FLOOR); }
        else { if (c->cell != CELL_NONE) continue; px = OutU(aX, c->ox, 233472); py = OutU(aZ, c->oy, 176128); feet = ToU(aFeetY - aOff); }
        const int* gr = c->grid;
        int sq = Square(c, px, py), n = gr[2] * gr[3], r = ToU(aR) * 3 / 4;
        if (sq < 0) continue;
        int k, k1 = gr[5 + n + 1 + sq + 1];
        for (k = gr[5 + n + 1 + sq]; k < k1; k++)
            {
            const short* w = c->wall + c->glist[k] * 6;
            if (w[4] > w[5]) { if (feet >= w[4] || feet <= w[5]) continue; }
            else if (w[5] <= feet + 30 || w[4] >= feet + 120) continue;
            int x0 = w[0], y0 = w[1], x1 = w[2], y1 = w[3];
            if (px + r < (x0 < x1 ? x0 : x1) || px - r > (x0 > x1 ? x0 : x1) || py + r < (y0 < y1 ? y0 : y1) || py - r > (y0 > y1 ? y0 : y1)) continue;
            int dx = x1 - x0, dy = y1 - y0, l2 = dx * dx + dy * dy;
            int t = (px - x0) * dx + (py - y0) * dy;
            int cx, cy;
            if (t <= 0 || l2 == 0) { cx = x0; cy = y0; }
            else if (t >= l2) { cx = x1; cy = y1; }
            else { cx = x0 + (int)((long long)dx * t / l2); cy = y0 + (int)((long long)dy * t / l2); }
            int ex = px - cx, ey = py - cy;
            if (ex * ex + ey * ey < r * r) { if (aWall) { aWall[0] = x0; aWall[1] = y0; aWall[2] = x1; aWall[3] = y1; aWall[4] = w[4]; aWall[5] = w[5]; aWall[6] = c->cell; aWall[7] = feet; } return w[4] > w[5] ? 2 : 1; }
            }
        }
    return 0;
    }
