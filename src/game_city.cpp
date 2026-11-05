// game_city.cpp - Vindgard: a walled hill city on the plains (three tiers: Market Rise, Tree Rise, Crown Rise)
// Original city in the Nordic hold-capital mould: a jarl's great hall on the crown of the hill, a sacred tree,
// clan halls and a temple on the middle tier, the market and townhouses inside stone walls lined with spikes,
// farms with a windmill and a meadery outside, and a ring of watchtowers.
#include "game.h"

static const int M = METER;

// city centre (cells) and tier heights relative to the plain
enum { CITY_X = CITY_CX, CITY_Z = CITY_CZ };      // (game.h)

// static type ids (filled by BuildCityMeshes)
enum
    {
    CT_HOUSE = 0, CT_HOUSE_LOD, CT_HALL, CT_GREATHALL, CT_WALL, CT_WALL_LOD, CT_TOWER, CT_LINTEL, CT_DOOR,
    CT_STALL, CT_WELL, CT_TREE, CT_TEMPLE, CT_PLINTH, CT_WINDMILL, CT_WATCH, CT_BANNER, CT_STAIRS,
    CT_FORGE, CT_HAY, CT_BARREL, CT_STATUE, CT_FENCE,
    CT_STAIRS2, CT_STALL2, CT_STALL3, CT_STABLE, CT_CART, CT_HORSE, CT_HORSE2, CT_BALE,
    CT_HALL_LOD, CT_GREAT_LOD, CT_TREE_LOD, CT_TEMPLE_LOD, CT_MILL_LOD, CT_MILLSAILS, CT_MILLSTONE,
    CT_WATER, CT_TENT, CT_TORCHPOST, CT_FLAME, CT_BRIDGE, CT_LEVER, CT_CRATES, CT_CULVERT,
    CT_TENT2, CT_TANRACK, CT_CRATE, CT_INNERGATE,
    CT_BASTION, CT_SCAFFOLD, CT_WOODSTAIR, CT_CLIFF, CT_FLAMETOP, CT_BASTION_LOD, CT_CLIFF_LOD, CT_STAIRS_LOD, CT_STAIRS2_LOD,
    CT_STAIRSW, CT_STAIRS2W, CT_STAIRS24, CT_STAIRS16, CT_STAIRSW_LOD, CT_STAIRS2W_LOD, CT_STAIRS24_LOD, CT_STAIRS16_LOD, CT_PIER,
    CT_DRAIN, CT_COUNT
    };

static void Box(R3D& r, Mesh& m, int slot, int x, int y0, int z, int hx, int h, int hz)
    {
    // box standing on y0 with height h (MeshBox hangs below its pivot)
    r.MeshBox(m, 0, slot, x, y0 + h, z, 0, hx, h / 2, hz);
    }

static void Beam(R3D& r, Mesh& m, int slot, int x0, int y0, int z0, int x1, int y1, int z1, int t)
    {
    r.MeshLimb(m, 0, slot, x0, y0, z0, x1, y1, z1, t, t, t, t, 4, 3);
    }

// straight flight of steps rising along +z from (x, y0, z0): n steps, rise / tread per step, solid sides and parapets
static void Steps(R3D& r, Mesh& m, int slot, int pslot, int x, int y0, int z0, int hw, int n, int rise, int tread)
    {
    int i;
    for (i = 0; i < n; i++)
        {
        int ya = y0 + i * rise, yb = ya + rise, za = z0 + i * tread, zb = za + tread;
        int q[12] = { x - hw, ya, za,  x + hw, ya, za,  x + hw, yb, za,  x - hw, yb, za };
        r.MeshQuad(m, 0, slot, q, x, ya + rise / 2, za + tread);           // riser faces -z
        int t[12] = { x - hw, yb, za,  x + hw, yb, za,  x + hw, yb, zb,  x - hw, yb, zb };
        r.MeshQuad(m, 0, slot, t, x, yb - rise, za + tread / 2);           // tread faces up
        }
    int zl = z0 + n * tread, yt = y0 + n * rise;
    int s;
    for (s = -1; s <= 1; s += 2)
        {
        int sx = x + s * hw;
        int q[12] = { sx, y0 - METER, z0,  sx, y0 - METER, zl,  sx, yt, zl,  sx, y0 + rise, z0 };
        r.MeshQuad(m, 0, pslot, q, x, y0, (z0 + zl) / 2);
        r.MeshLimb(m, 0, pslot, sx, y0 + rise + 60, z0 + 20, sx, yt + 60, zl, 34, 34, 34, 34, 4, 3);
        }
    }

static void Def(StaticDef& d, const Mesh* m, const Mesh* lod, int hx, int hz, int rad, int dd, int ld)
    {
    r3d_memset(&d, 0, sizeof(d));
    d.mesh = m; d.lod = lod; d.hx = (short)hx; d.hz = (short)hz; d.rad = (short)rad;
    d.drawDist = (short)dd; d.lodDist = (short)ld;
    }

static void Col(StaticDef& d, int slot, int r, int g, int b, int tex)
    {
    d.slots[slot * 4] = (u8)r; d.slots[slot * 4 + 1] = (u8)g; d.slots[slot * 4 + 2] = (u8)b; d.slots[slot * 4 + 3] = (u8)tex;
    }

// common palette
#define STONE(d, s)  Col(d, s, 126, 122, 114, SL_TEX | 7)     // dressed stone blocks (textured)
#define DSTONE(d, s) Col(d, s, 96, 92, 86, SL_TEX | 1)      // rough stone (textured)
#define WOOD(d, s)   Col(d, s, 112, 80, 50, 4)
#define DWOOD(d, s)  Col(d, s, 70, 50, 34, 4)
#define ROOF(d, s)   Col(d, s, 96, 74, 52, 4)

// ---------------------------------------------------------------- collision parts (see R3D::Blocked)
// a market stall: four posts, the counter (its top is the surface things rest on), the awning; per kind
// the sacks or the barrel behind the counter
static const int KStallParts[] =
    {
    PART_BOX, -404, -276, -364, -236, 0, 3 * M,   PART_BOX, 364, -276, 404, -236, 0, 3 * M,
    PART_BOX, -404, 236, -364, 276, 0, 3 * M,     PART_BOX, 364, 236, 404, 276, 0, 3 * M,
    PART_BOX, -384, -312, 384, -141, 0, 9 * M / 10,
    PART_BOX, -448, -330, 448, 330, 9 * M / 4 - 10, 3 * M + 20,
    // greengrocer: sacks behind the counter
    PART_BOX, -306, 78, -206, 178, 0, 110,   PART_BOX, -131, 102, -39, 194, 0, 100
    };
static const int KStallParts1[] =
    {
    PART_BOX, -404, -276, -364, -236, 0, 3 * M,   PART_BOX, 364, -276, 404, -236, 0, 3 * M,
    PART_BOX, -404, 236, -364, 276, 0, 3 * M,     PART_BOX, 364, 236, 404, 276, 0, 3 * M,
    PART_BOX, -384, -312, 384, -141, 0, 9 * M / 10,
    PART_BOX, -448, -330, 448, 330, 9 * M / 4 - 10, 3 * M + 20,
    // butcher: the barrel of salt fish
    PART_BOX, 210, 82, 302, 174, 0, 128
    };
// the city wall: body (its top is walked on) and footing; a bastion; the gate walkway's deck and posts
static const int KWallParts[] =
    {
    PART_BOX, -5 * M, -3 * M / 4, 5 * M, 3 * M / 4, -2 * M, 7 * M,   PART_BOX, -51 * M / 10, -M, 51 * M / 10, M, -2 * M, M
    };
static const int KPierParts[] = { PART_BOX, -5 * M / 2, -5 * M / 2, 5 * M / 2, 5 * M / 2, 0, 10 * M };
static const int KBastionParts[] = { PART_BOX, -37 * M / 10, -37 * M / 10, 37 * M / 10, 37 * M / 10, -2 * M, 10 * M };
static const int KScaffoldParts[] =
    {
    PART_BOX, -5 * M, -4 * M / 5, 5 * M, 4 * M / 5, 54 * M / 10, 58 * M / 10,
    PART_BOX, -5 * M, 3 * M / 4 - 12, -29 * M / 10, 3 * M / 4 + 12, 58 * M / 10, 67 * M / 10,       // the rail, with a gap
    PART_BOX, -M, 3 * M / 4 - 12, 5 * M, 3 * M / 4 + 12, 58 * M / 10, 67 * M / 10,             //   where the stair arrives
    PART_BOX, -5 * M, -3 * M / 4 - 12, -5 * M + 40, 3 * M / 4 + 12, 0, 54 * M / 10,          // end posts
    PART_BOX, 5 * M - 60, -3 * M / 4 - 12, 5 * M, 3 * M / 4 + 12, 0, 54 * M / 10
    };
// tents: the two hide walls and the back (you walk in at the front)
static const int KTentParts[] =
    {
    PART_BOX, -304, -264, -254, 264, 0, M,   PART_BOX, 254, -264, 304, 264, 0, M,   PART_BOX, -294, -279, 294, -249, 0, 3 * M / 2
    };
static const int KTentParts2[] =
    {
    PART_BOX, -690, -435, -590, 435, 0, 300,   PART_BOX, 590, -435, 690, 435, 0, 300,   PART_BOX, -690, -460, 690, -420, 0, 600,
    PART_BOX, -690, 420, -258, 460, 0, 490,    PART_BOX, 258, 420, 690, 460, 0, 490
    };
// the inner gate: two walls and two piers
static const int KInnerGateParts[] =
    {
    PART_BOX, -2432, -128, -1152, 128, -256, 2048,   PART_BOX, 1152, -128, 2432, 128, -256, 2048,
    PART_BOX, -1152, -256, -896, 256, -256, 2048,    PART_BOX, 896, -256, 1152, 256, -256, 2048
    };
// the well: a ring wall you can drop things through, the two roof posts
static const int KWellParts[] =
    {
    PART_RING, 9 * M / 10, 6 * M / 5, 0, 0, -M, M,
    PART_BOX, -M - 18, -18, -M + 18, 18, 0, 11 * M / 4,
    PART_BOX, M - 18, -18, M + 18, 18, 0, 11 * M / 4
    };

void Game::BuildCityMeshes()
    {
    R3D& r = *iR;
    Mesh* cm = iMCity;
    StaticDef d;
    int i;

    // ---- townhouse: stone footing, timber walls, steep shingle roof, corner posts, crossed gable beams, chimney
    {
    Mesh& m = cm[CT_HOUSE];
    r.MeshInit(m, 192, 240, 1);
    // stone footing reaches 2.5 m into the ground for slopes but only 0.3 m above it; door at ground level
    Box(r, m, 0, 0, -5 * M / 2, 0, 5 * M, 5 * M / 2 + 3 * M / 10, 7 * M / 2);
    Box(r, m, 3, 0, 3 * M / 10, 0, 9 * M / 2, 3 * M, 3 * M);                      // timber walls
    r.MeshRoof(m, 0, 1, 9 * M / 2, 3 * M, 33 * M / 10, 39 * M / 5, 3 * M / 4);      // roof (variant tint) + gables (slot 2)
    Beam(r, m, 2, -9 * M / 2 - 16, 0, -3 * M - 16, -9 * M / 2 - 16, 33 * M / 10, -3 * M - 16, 22);   // corner posts
    Beam(r, m, 2, 9 * M / 2 + 16, 0, -3 * M - 16, 9 * M / 2 + 16, 33 * M / 10, -3 * M - 16, 22);
    Beam(r, m, 2, -5 * M - 40, 34 * M / 5, 0, -5 * M - 120, 44 * M / 5, -M, 18);   // crossed gable beams
    Beam(r, m, 2, -5 * M - 40, 34 * M / 5, 0, -5 * M - 120, 44 * M / 5, M, 18);
    Beam(r, m, 2, 5 * M + 40, 34 * M / 5, 0, 5 * M + 120, 44 * M / 5, -M, 18);
    Beam(r, m, 2, 5 * M + 40, 34 * M / 5, 0, 5 * M + 120, 44 * M / 5, M, 18);
    Box(r, m, 5, 3 * M, 24 * M / 5, M, M / 2, 9 * M / 2, M / 2);                   // chimney
    Box(r, m, 6, 0, 3 * M / 10, -3 * M - 12, 3 * M / 4, 2 * M + M / 4, 14);        // door
    Box(r, m, 0, 0, -M, -3 * M - 60, M, M + M / 5, M / 3);                          // door step
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Mesh& l = cm[CT_HOUSE_LOD];
    r.MeshInit(l, 48, 48, 1);
    Box(r, l, 3, 0, -2 * M, 0, 9 * M / 2, 2 * M + 33 * M / 10, 3 * M);
    r.MeshRoof(l, 0, 1, 9 * M / 2, 3 * M, 33 * M / 10, 39 * M / 5, 3 * M / 4);
    r.MeshFinish(l);
    Def(d, &m, &l, 5 * M, 7 * M / 2, 0, 120, 40);
    STONE(d, 0); Col(d, 1, 96, 74, 52, SL_TEX | TXI_THATCH); DWOOD(d, 2); Col(d, 3, 112, 80, 50, SL_TEX | TXI_LOGS); WOOD(d, 4); DSTONE(d, 5); Col(d, 6, 46, 32, 24, SL_TEX | TXI_PLANKS);
    iCityType[CT_HOUSE] = r.RegisterStatic(d);
    }
    // ---- clan hall / mead hall: long, stone footing, upturned prow beams
    {
    Mesh& m = cm[CT_HALL];
    r.MeshInit(m, 192, 240, 1);
    Box(r, m, 0, 0, -5 * M / 2, 0, 21 * M / 2, 5 * M / 2 + 2 * M / 5, 5 * M);
    Box(r, m, 1, 0, 2 * M / 5, 0, 10 * M, 7 * M / 2, 9 * M / 2);
    r.MeshRoof(m, 0, 3, 10 * M, 9 * M / 2, 39 * M / 10, 99 * M / 10, M);
    Beam(r, m, 2, -11 * M, 9 * M, 0, -13 * M, 12 * M, -M, 30);     // prow beams
    Beam(r, m, 2, -11 * M, 9 * M, 0, -13 * M, 12 * M, M, 30);
    Beam(r, m, 2, 11 * M, 9 * M, 0, 13 * M, 12 * M, -M, 30);
    Beam(r, m, 2, 11 * M, 9 * M, 0, 13 * M, 12 * M, M, 30);
    Box(r, m, 6, 0, 2 * M / 5, -9 * M / 2 - 12, M, 3 * M, 14);      // door
    for (i = -1; i <= 1; i += 2)
        Beam(r, m, 2, i * 2 * M, 0, -6 * M, i * 2 * M, 4 * M, -6 * M, 28);   // porch posts
    Box(r, m, 3, 0, 4 * M, -11 * M / 2, 3 * M, M / 4, 3 * M / 2);    // porch roof
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    {
    Mesh& l = cm[CT_HALL_LOD];
    r.MeshInit(l, 24, 24, 1);
    Box(r, l, 1, 0, -2 * M, 0, 10 * M, 2 * M + 39 * M / 10, 9 * M / 2);
    r.MeshRoof(l, 0, 3, 10 * M, 9 * M / 2, 39 * M / 10, 99 * M / 10, M);
    r.MeshFinish(l);
    }
    Def(d, &m, &cm[CT_HALL_LOD], 21 * M / 2, 5 * M, 0, 150, 60);
    STONE(d, 0); Col(d, 1, 112, 80, 50, SL_TEX | TXI_LOGS); DWOOD(d, 2); Col(d, 3, 104, 96, 62, SL_TEX | TXI_THATCH); WOOD(d, 4); Col(d, 6, 46, 32, 24, SL_TEX | TXI_PLANKS);
    iCityType[CT_HALL] = r.RegisterStatic(d);
    }
    // ---- Highhall: the jarl's great hall on the crown (stone platform, tall timber hall, raised clerestory, porch)
    {
    Mesh& m = cm[CT_GREATHALL];
    r.MeshInit(m, 480, 520, 1);
    Box(r, m, 0, 0, -2 * M, 0, 13 * M, 4 * M, 8 * M);                    // stone platform
    Box(r, m, 1, 0, 2 * M, 2 * M, 11 * M, 7 * M, 6 * M);                  // main hall
    r.MeshRoof(m, 0, 3, 11 * M, 6 * M, 9 * M, 17 * M, M);                // main roof (gables slot 4)
    Box(r, m, 1, 0, 14 * M, 2 * M, 6 * M, 4 * M, 3 * M);                  // raised clerestory
    r.MeshRoof(m, 0, 3, 6 * M, 3 * M, 18 * M, 23 * M, M / 2);
    // corner towers of stone
    Box(r, m, 0, -11 * M, 2 * M, -4 * M, 3 * M / 2, 12 * M, 3 * M / 2);
    Box(r, m, 0, 11 * M, 2 * M, -4 * M, 3 * M / 2, 12 * M, 3 * M / 2);
    r.MeshConeAt(m, 0, 3, -11 * M, -4 * M, 5 * M / 2, 14 * M, 18 * M, 4, 128);
    r.MeshConeAt(m, 0, 3, 11 * M, -4 * M, 5 * M / 2, 14 * M, 18 * M, 4, 128);
    // front porch: pillars + roof
    for (i = 0; i < 4; i++)
        {
        int x = (i * 2 - 3) * 2 * M;
        r.MeshLimb(m, 0, 2, x, 2 * M, -7 * M, x, 8 * M, -7 * M, 28, 28, 24, 24, 5, 0);
        }
    Box(r, m, 2, 0, 8 * M, -6 * M, 7 * M, M / 2, 2 * M);
    r.MeshRoof(m, 0, 3, 7 * M, 2 * M, 17 * M / 2, 11 * M, M / 2);
    Box(r, m, 6, 0, 2 * M, -4 * M - 20, 2 * M, 5 * M, 16);               // great doors
    // prow beams on the gable
    Beam(r, m, 2, 0, 17 * M, -7 * M, -M, 20 * M, -9 * M, 34);
    Beam(r, m, 2, 0, 17 * M, -7 * M, M, 20 * M, -9 * M, 34);
    // front steps
    Steps(r, m, 5, 0, 0, 0, -8 * M - 8 * 2 * M / 5, 3 * M, 8, M / 4, 2 * M / 5);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    {
    Mesh& l = cm[CT_GREAT_LOD];
    r.MeshInit(l, 40, 48, 1);
    Box(r, l, 0, 0, -2 * M, 0, 13 * M, 4 * M, 8 * M);
    Box(r, l, 1, 0, 2 * M, 2 * M, 11 * M, 7 * M, 6 * M);
    r.MeshRoof(l, 0, 3, 11 * M, 6 * M, 9 * M, 17 * M, M);
    Box(r, l, 0, -11 * M, 2 * M, -4 * M, 3 * M / 2, 12 * M, 3 * M / 2);
    Box(r, l, 0, 11 * M, 2 * M, -4 * M, 3 * M / 2, 12 * M, 3 * M / 2);
    r.MeshFinish(l);
    }
    Def(d, &m, &cm[CT_GREAT_LOD], 13 * M, 8 * M, 0, 175, 70);
    STONE(d, 0); Col(d, 1, 112, 80, 50, SL_TEX | TXI_LOGS); DWOOD(d, 2); Col(d, 3, 84, 70, 56, SL_TEX | TXI_SHINGLE); WOOD(d, 4); Col(d, 5, 150, 146, 138, SL_TEX | 7); Col(d, 6, 46, 32, 24, SL_TEX | TXI_PLANKS);
    iCityType[CT_GREATHALL] = r.RegisterStatic(d);
    }
    // ---- city wall segment (9.6 m), after Whiterun's: coarse grey-beige masonry on a battered footing of rough
    //      stone, a course of dark sawn logs set into both faces, crumbling merlons (one broken). Kept to few
    //      faces: there are some sixty segments round the city. -z is outside.
    //      Walkable along the top (no rampart, as in the game); collision is the body only.
    {
    Mesh& m = cm[CT_WALL];
    r.MeshInit(m, 200, 260, 1);
    Box(r, m, 1, 0, -2 * M, 0, 51 * M / 10, 3 * M, M);                        // battered footing (rough stone)
    Box(r, m, 0, 0, M, 0, 5 * M, 6 * M, 3 * M / 4);                           // the wall body
    static const int KMer[3 * 2] = { -30, 100,  0, 45,  30, 95 };          // merlons: x (dm), height (cm); one broken
    for (i = 0; i < 3; i++)
        Box(r, m, 0, KMer[i * 2] * M / 10, 7 * M, 0, 7 * M / 10, KMer[i * 2 + 1] * M / 100, 3 * M / 4);
    // the timber course along both faces
    Box(r, m, 2, 0, 5 * M, -3 * M / 4 - 8, 5 * M, 30, 10);
    Box(r, m, 2, 0, 5 * M, 3 * M / 4 + 8, 5 * M, 30, 10);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Mesh& l = cm[CT_WALL_LOD];
    r.MeshInit(l, 16, 24, 1);
    Box(r, l, 0, 0, -2 * M, 0, 5 * M, 9 * M, 3 * M / 4);
    r.MeshFinish(l);
    Def(d, &m, &l, 5 * M, 3 * M / 4, 0, 175, 24);
    Col(d, 0, 132, 124, 110, SL_TEX | 7); Col(d, 1, 112, 106, 96, SL_TEX | 1); DWOOD(d, 2); Col(d, 3, 46, 42, 38, 1);
    d.hy = 7 * M; d.parts = KWallParts; d.nparts = 2;
    iCityType[CT_WALL] = r.RegisterStatic(d);
    }
    // ---- square bastion (9.5 m wall height + crenellations), rough footing, iron braziers on two corners, carved
    //      dragon-head beams rising at the other two (the Nordic motif)
    {
    Mesh& m = cm[CT_BASTION];
    r.MeshInit(m, 320, 400, 1);
    Box(r, m, 1, 0, -2 * M, 0, 37 * M / 10, 7 * M / 2, 37 * M / 10);
    Box(r, m, 0, 0, 3 * M / 2, 0, 16 * M / 5, 17 * M / 2, 16 * M / 5);
    int s2;
    for (s2 = 0; s2 < 4; s2++)
        {
        // crenellations: two merlons per side
        int k;
        for (k = -1; k <= 1; k += 2)
            {
            int a = k * 2 * M, b = 3 * M;
            int x = (s2 & 1) ? (s2 & 2 ? b : -b) : a, z = (s2 & 1) ? a : (s2 & 2 ? b : -b);
            Box(r, m, 0, x, 10 * M, z, (s2 & 1) ? 3 * M / 10 : 3 * M / 5, M, (s2 & 1) ? 3 * M / 5 : 3 * M / 10);
            }
        }
    // iron braziers on two corners
    for (s2 = -1; s2 <= 1; s2 += 2)
        {
        r.MeshLimb(m, 0, 4, s2 * 27 * M / 10, 10 * M, s2 * 27 * M / 10, s2 * 27 * M / 10, 10 * M + 60, s2 * 27 * M / 10, 22, 22, 40, 40, 6, 1);
        r.MeshLimb(m, 0, 5, s2 * 27 * M / 10, 10 * M + 40, s2 * 27 * M / 10, s2 * 27 * M / 10, 10 * M + 62, s2 * 27 * M / 10, 34, 34, 30, 30, 6, 2);
        // carved dragon heads on the other two corners
        int cx = s2 * 3 * M, cz = -s2 * 3 * M;
        Beam(r, m, 2, cx, 10 * M, cz, cx + s2 * 60, 12 * M, cz - s2 * 60, 22);
        r.MeshEllipsoid(m, 0, 2, cx + s2 * 80, 12 * M + 20, cz - s2 * 80, 30, 22, 44, 5, 2);
        }
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Mesh& bl = cm[CT_BASTION_LOD];
    r.MeshInit(bl, 24, 24, 1);
    Box(r, bl, 0, 0, -2 * M, 0, 16 * M / 5, 13 * M, 16 * M / 5);
    r.MeshFinish(bl);
    Def(d, &m, &bl, 0, 0, 0, 175, 28);
    Col(d, 0, 132, 124, 110, SL_TEX | 7); Col(d, 1, 112, 106, 96, SL_TEX | 1); DWOOD(d, 2); Col(d, 4, 50, 50, 54, 1); Col(d, 5, 255, 120, 40, 1);
    d.parts = KBastionParts; d.nparts = 1;
    iCityType[CT_BASTION] = r.RegisterStatic(d);
    // the braziers' flames (placed at the corners, 10.6 m up)
    Def(d, &iMFlame, 0, 0, 0, 0, 175, 0);
    Col(d, 0, 255, 200, 70, 1); Col(d, 1, 240, 96, 24, 1); Col(d, 2, 255, 246, 200, 1);
    d.flags = SD_ANIM | SD_UNLIT; d.animMs = 90; d.yOff = 10 * M + 60;
    iCityType[CT_FLAMETOP] = r.RegisterStatic(d);
    }
    // ---- timber walkway inside the wall by the gate (a lookout platform 5.8 m up on posts, rail on the town
    //      side), and its wooden stair: 8 m long along -z, 1.4 m wide, rising 5.8 m
    {
    Mesh& m = cm[CT_SCAFFOLD];
    r.MeshInit(m, 160, 200, 1);
    Box(r, m, 0, 0, 56 * M / 10, 0, 5 * M, M / 5, 4 * M / 5);                  // deck
    for (i = 0; i < 6; i++)
        {
        int x = -5 * M + 20 + i * 2 * M - (i == 5 ? 40 : 0);
        Beam(r, m, 1, x, 0, -3 * M / 4, x, 56 * M / 10, -3 * M / 4, 16);
        Beam(r, m, 1, x, 0, 3 * M / 4, x, 67 * M / 10, 3 * M / 4, 16);
        }
    Beam(r, m, 1, -5 * M, 66 * M / 10, 3 * M / 4, -29 * M / 10, 66 * M / 10, 3 * M / 4, 10);   // rail (gap at the stair head)
    Beam(r, m, 1, -M, 66 * M / 10, 3 * M / 4, 5 * M, 66 * M / 10, 3 * M / 4, 10);
    Beam(r, m, 1, -5 * M, 50 * M / 10, 0, 5 * M, 50 * M / 10, 0, 14);                   // stringer
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def(d, &m, 0, 0, 0, 0, 90, 0);
    Col(d, 0, 118, 88, 58, SL_TEX | TXI_PLANKS); DWOOD(d, 1);
    d.hy = 58 * M / 10; d.parts = KScaffoldParts; d.nparts = 5;
    iCityType[CT_SCAFFOLD] = r.RegisterStatic(d);
    Mesh& t = cm[CT_WOODSTAIR];
    r.MeshInit(t, 260, 300, 1);
    int n = 23;
    for (i = 0; i < n; i++)
        {
        int z0 = -i * 8 * M / n, z1 = -(i + 1) * 8 * M / n, y = (i + 1) * 58 * M / 10 / n;
        int q[12] = { -7 * M / 10, y, z0,  7 * M / 10, y, z0,  7 * M / 10, y, z1,  -7 * M / 10, y, z1 };
        r.MeshQuad(t, 0, 0, q, 0, y - M, (z0 + z1) / 2);
        int rq[12] = { -7 * M / 10, y - 58 * M / 10 / n, z0,  7 * M / 10, y - 58 * M / 10 / n, z0,  7 * M / 10, y, z0,  -7 * M / 10, y, z0 };
        r.MeshQuad(t, 0, 0, rq, 0, y, z0 + M);
        }
    for (i = -1; i <= 1; i += 2)
        {
        Beam(r, t, 1, i * 75, 0, 0, i * 75, 58 * M / 10, -8 * M, 14);                  // stringers
        Beam(r, t, 1, i * 75, 90, 0, i * 75, 58 * M / 10 + 90, -8 * M, 9);            // hand rails
        Beam(r, t, 1, i * 75, 0, -4 * M, i * 75, 29 * M / 10 + 90, -4 * M, 12);       // a prop halfway
        }
    r.MeshFinish(t);
    r.MeshPlanarUV(t, 2);
    Def(d, &t, 0, 0, 0, 0, 90, 0);
    Col(d, 0, 118, 88, 58, SL_TEX | TXI_PLANKS); DWOOD(d, 1);
    iCityType[CT_WOODSTAIR] = r.RegisterStatic(d);
    }
    // ---- rock bluff: a craggy slab of the hill's rock, 13 m high, used to face the cliffs under Highhall
    {
    Mesh& m = cm[CT_CLIFF];
    r.MeshInit(m, 120, 200, 1);
    r.MeshLimb(m, 0, 0, 0, 0, 0, 0, 13 * M, 0, 6 * M, 4 * M, 5 * M, 3 * M, 7, 2);
    r.MeshEllipsoid(m, 0, 0, -3 * M, 9 * M, -2 * M, 3 * M, 4 * M, 2 * M, 5, 3);
    r.MeshEllipsoid(m, 0, 1, 2 * M, 5 * M, -3 * M, 3 * M, 3 * M, 2 * M, 5, 3);
    r.MeshEllipsoid(m, 0, 1, 4 * M, 11 * M, -M, 2 * M, 2 * M, 2 * M, 5, 2);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Mesh& l = cm[CT_CLIFF_LOD];
    r.MeshInit(l, 24, 24, 1);
    r.MeshLimb(l, 0, 0, 0, 0, 0, 0, 13 * M, 0, 6 * M, 4 * M, 4 * M, 3 * M, 4, 1);
    r.MeshFinish(l);
    r.MeshPlanarUV(l, 2);
    Def(d, &m, &l, 0, 0, 5 * M, 175, 40);
    Col(d, 0, 118, 112, 104, SL_TEX | 1); Col(d, 1, 100, 96, 90, SL_TEX | 1);
    iCityType[CT_CLIFF] = r.RegisterStatic(d);
    }
    // ---- round tower with conical roof
    {
    Mesh& m = cm[CT_TOWER];
    r.MeshInit(m, 80, 112, 1);
    r.MeshLimb(m, 0, 0, 0, -2 * M, 0, 0, 11 * M, 0, 3 * M, 3 * M, 11 * M / 4, 11 * M / 4, 8, 0);
    r.MeshLimb(m, 0, 1, 0, 11 * M, 0, 0, 12 * M, 0, 13 * M / 4, 13 * M / 4, 13 * M / 4, 13 * M / 4, 8, 0);
    r.MeshCone(m, 0, 2, 7 * M / 2, 12 * M, 17 * M, 8, 64);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def(d, &m, 0, 0, 0, 3 * M + 40, 175, 0);
    STONE(d, 0); DSTONE(d, 1); ROOF(d, 2);
    iCityType[CT_TOWER] = r.RegisterStatic(d);
    }
    // ---- gate lintel (no collision) and an open gate wing
    {
    Mesh& m = cm[CT_LINTEL];
    r.MeshInit(m, 48, 48, 1);
    Box(r, m, 0, 0, 6 * M, 0, 4 * M, 3 * M, M);
    Box(r, m, 1, 0, 9 * M, 0, 4 * M, M / 2, 3 * M / 2);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def(d, &m, 0, 0, 0, 0, 175, 0);
    STONE(d, 0); DWOOD(d, 1);
    iCityType[CT_LINTEL] = r.RegisterStatic(d);
    Mesh& g = cm[CT_DOOR];
    r.MeshInit(g, 40, 48, 1);
    Box(r, g, 0, 0, 0, 0, 7 * M / 4, 6 * M, M / 8);
    Box(r, g, 1, 0, 2 * M, -20, 7 * M / 4, M / 4, M / 6);
    Box(r, g, 1, 0, 4 * M, -20, 7 * M / 4, M / 4, M / 6);
    r.MeshFinish(g);
    Def(d, &g, 0, 7 * M / 4, M / 4, 0, 120, 0);
    DWOOD(d, 0); Col(d, 1, 60, 60, 64, 1);
    iCityType[CT_DOOR] = r.RegisterStatic(d);
    }
    // ---- market stalls, as in the games: four posts, a gabled canopy of striped cloth (pale yellow and blue) with plank
    //      gable ends, the counter and real goods
    {
    int kind;
    for (kind = 0; kind < 3; kind++)
        {
        Mesh& m = cm[kind == 0 ? CT_STALL : (kind == 1 ? CT_STALL2 : CT_STALL3)];
        r.MeshInit(m, 380, 600, 1);
        int px[4] = { -3 * M / 2, 3 * M / 2, -3 * M / 2, 3 * M / 2 }, pz[4] = { -M, -M, M, M };
        for (i = 0; i < 4; i++) Beam(r, m, 0, px[i], 0, pz[i], px[i], 9 * M / 4, pz[i], 14);
        {
        int sd, st, ey = 9 * M / 4, ry = 33 * M / 10, ez = 27 * M / 20, hx = 15 * M / 8;
        for (sd = -1; sd <= 1; sd += 2)
            for (st = 0; st < 4; st++)
                {
                int xa = -hx + st * hx / 2, xb = xa + hx / 2;
                int q[12] = { xa, ey, sd * ez,  xb, ey, sd * ez,  xb, ry, 0,  xa, ry, 0 };
                r.MeshQuad(m, 0, (st & 1) ? 7 : 1, q, 0, 0, 0);                   // underside
                r.MeshQuad(m, 0, (st & 1) ? 7 : 1, q, 0, ry + sd * 0, sd * 4 * M);   // top
                }
        for (sd = -1; sd <= 1; sd += 2)
            {
            int t[9] = { sd * hx, ey, -ez,  sd * hx, ey, ez,  sd * hx, ry, 0 };
            r.MeshTri(m, 0, 0, t, 0, ey, 0);
            r.MeshTri(m, 0, 0, t, sd * 4 * M, ey, 0);
            }
        Beam(r, m, 0, -hx, ry, 0, hx, ry, 0, 12);                                   // ridge pole
        }
        // counter: table top on legs, front board
        Box(r, m, 0, 0, 4 * M / 5, -M + 30, 3 * M / 2, M / 10, M / 3);
        Box(r, m, 0, 0, M / 5, -M - 46, 3 * M / 2 - 20, 3 * M / 5, 8);
        int top = 9 * M / 10;
        if (kind == 0)
            {
            // greengrocer: two open crates on the counter (the produce in them is real, takeable)
            for (i = 0; i < 2; i++)
                {
                int cx = i ? 0 : -M, cz = -M + 30;
                Box(r, m, 6, cx, top, cz, 64, 5, 46);                    // floor
                Box(r, m, 6, cx, top, cz - 44, 64, 46, 4);               // front / back
                Box(r, m, 6, cx, top, cz + 44, 64, 46, 4);
                Box(r, m, 6, cx - 62, top, cz, 4, 46, 46);               // ends
                Box(r, m, 6, cx + 62, top, cz, 4, 46, 46);
                }
            // sacks behind
            r.MeshEllipsoid(m, 0, 5, -M, 50, M / 2, 50, 56, 44, 6, 3);
            r.MeshEllipsoid(m, 0, 5, -M / 3, 46, M / 2 + 20, 46, 50, 40, 6, 3);
            }
        else if (kind == 1)
            {
            // butcher: a hanging rail under the awning (the meat and fish on it are takeable), barrel of salt fish
            r.MeshLimb(m, 0, 0, -6 * M / 5, 9 * M / 4 - 10, -5 * M / 4, 6 * M / 5, 9 * M / 4 - 10, -5 * M / 4, 6, 6, 6, 6, 4, 3);
            r.MeshLimb(m, 0, 6, M, 0, M / 2, M, M / 2, M / 2, 46, 46, 46, 46, 7, 2);               // barrel of salt fish
            }
        else
            {
            // jeweller: a velvet cloth along the counter, a small chest, folded cloth
            Box(r, m, 4, 0, top, -M + 30, 7 * M / 5, 2, 26);
            Box(r, m, 6, -6 * M / 5, top, M / 3, 30, 30, 22);
            Box(r, m, 4, 6 * M / 5, top, -M + 30, 30, 24, 40);
            Box(r, m, 5, 6 * M / 5, top + 24, -M + 30, 26, 20, 36);
            }
        r.MeshFinish(m);
        r.MeshPlanarUV(m);
        Def(d, &m, 0, 3 * M / 2, M, 0, 80, 0);
        WOOD(d, 0); Col(d, 1, 206, 190, 120, 6); Col(d, 7, 104, 132, 160, 6); Col(d, 6, 150, 112, 64, 4);
        d.hy = 9 * M / 10;     // counter top: things can be put down on it
        d.parts = kind == 1 ? KStallParts1 : KStallParts; d.nparts = kind == 0 ? 8 : (kind == 1 ? 7 : 6);
        if (kind == 0) { Col(d, 5, 176, 150, 104, 6); Col(d, 6, 140, 104, 64, SL_TEX | TXI_PLANKS); }
        else if (kind == 1) { Col(d, 2, 170, 176, 182, 1); Col(d, 3, 150, 60, 50, 1); }
        else { Col(d, 4, 110, 30, 34, 6); }
        d.flags |= SD_WALLED; d.drawDist = 55;
    iCityType[kind == 0 ? CT_STALL : (kind == 1 ? CT_STALL2 : CT_STALL3)] = r.RegisterStatic(d);
        }
    }
    // ---- well: round stone shaft with a lip, dark water, two posts, windlass with rope and bucket, small roof
    {
    Mesh& m = cm[CT_WELL];
    r.MeshInit(m, 160, 220, 1);
    r.MeshLimb(m, 0, 0, 0, -M / 2, 0, 0, 4 * M / 5, 0, 11 * M / 10, 11 * M / 10, 11 * M / 10, 11 * M / 10, 12, 0);
    r.MeshLimb(m, 0, 0, 0, 4 * M / 5, 0, 0, M - 10, 0, 6 * M / 5, 6 * M / 5, 6 * M / 5, 6 * M / 5, 12, 0);      // lip
    r.MeshConeAt(m, 0, 4, 0, 0, 6 * M / 5, M - 10, M - 9, 12, 0);              // lip top ring
    r.MeshConeAt(m, 0, 5, 0, 0, 9 * M / 10, M - 4, M - 3, 12, 0);              // dark water opening
    Beam(r, m, 1, -M, M - 20, 0, -M, 3 * M / 2, 0, 16);
    Beam(r, m, 1, M, M - 20, 0, M, 3 * M / 2, 0, 16);
    r.MeshLimb(m, 0, 1, -M - 20, 4 * M / 3, 0, M + 20, 4 * M / 3, 0, 22, 22, 22, 22, 6, 3);   // the crank's log
    r.MeshLimb(m, 0, 1, M + 20, 4 * M / 3, 0, M + 40, 4 * M / 3 + 40, 0, 8, 8, 8, 8, 4, 3);   // its handle
    // the round paved platform with its stone edge
    r.MeshLimb(m, 0, 7, 0, -20, 0, 0, 10, 0, 3 * M, 3 * M, 3 * M, 3 * M, 16, 0);
    r.MeshConeAt(m, 0, 7, 0, 0, 3 * M, 10, 11, 16, 0);
    r.MeshFinish(m);
    r.MeshPlanarUV(m);
    Def(d, &m, 0, 0, 0, 6 * M / 5 + 20, 90, 0);
    d.parts = KWellParts; d.nparts = 3;
    Col(d, 0, 126, 122, 114, SL_TEX | 1); DWOOD(d, 1); ROOF(d, 2); WOOD(d, 3); Col(d, 4, 140, 134, 124, 1); Col(d, 5, 18, 26, 34, 1); Col(d, 6, 170, 150, 110, 1);
    Col(d, 7, 150, 146, 136, SL_TEX | 7);
    d.flags |= SD_WALLED; d.drawDist = 60;
    iCityType[CT_WELL] = r.RegisterStatic(d);
    }
    // ---- the Elder Ash: sacred tree in a stone planter, golden crown
    {
    Mesh& m = cm[CT_TREE];
    r.MeshInit(m, 400, 520, 1);
    r.MeshLimb(m, 0, 0, 0, -M / 2, 0, 0, 3 * M / 4, 0, 9 * M / 2, 9 * M / 2, 9 * M / 2, 9 * M / 2, 12, 2);   // planter
    r.MeshLimb(m, 0, 1, 0, 0, 0, 0, 7 * M, M / 2, M, M, M / 2, M / 2, 6, 0);                              // trunk
    static const int KBr[5 * 3] = { 4, 9, 1,  -4, 10, -1,  1, 11, 4,  -1, 10, -4,  0, 12, 0 };
    for (i = 0; i < 5; i++)
        r.MeshLimb(m, 0, 1, 0, 6 * M, M / 2, KBr[i * 3] * M, KBr[i * 3 + 1] * M, KBr[i * 3 + 2] * M, 2 * M / 5, 2 * M / 5, M / 6, M / 6, 4, 0);
    static const int KCan[6 * 4] = { 4, 10, 1, 3,  -4, 11, -1, 3,  1, 12, 4, 3,  -1, 11, -4, 3,  0, 13, 0, 4,  2, 9, -2, 3 };
    for (i = 0; i < 6; i++)
        r.MeshEllipsoid(m, 0, i & 1 ? 2 : 3, KCan[i * 4] * M, KCan[i * 4 + 1] * M, KCan[i * 4 + 2] * M,
                        KCan[i * 4 + 3] * M, KCan[i * 4 + 3] * M * 3 / 4, KCan[i * 4 + 3] * M, 6, 3);
    r.MeshFinish(m);
    r.MeshSmooth(m);
    {
    Mesh& l = cm[CT_TREE_LOD];
    r.MeshInit(l, 60, 80, 1);
    r.MeshLimb(l, 0, 0, 0, -M / 2, 0, 0, 3 * M / 4, 0, 9 * M / 2, 9 * M / 2, 9 * M / 2, 9 * M / 2, 6, 2);
    r.MeshLimb(l, 0, 1, 0, 0, 0, 0, 8 * M, M / 2, M, M, M / 2, M / 2, 4, 0);
    r.MeshEllipsoid(l, 0, 3, 0, 11 * M, 0, 6 * M, 4 * M, 6 * M, 6, 3);
    r.MeshFinish(l);
    }
    Def(d, &m, &cm[CT_TREE_LOD], 0, 0, 9 * M / 2 + 20, 175, 60);
    STONE(d, 0); Col(d, 1, 92, 70, 50, 4); Col(d, 2, 220, 168, 64, 1); Col(d, 3, 236, 196, 92, 1);
    iCityType[CT_TREE] = r.RegisterStatic(d);
    }
    // ---- temple of the Sky-Mother: stone nave, apse, porch, round window
    {
    Mesh& m = cm[CT_TEMPLE];
    r.MeshInit(m, 192, 240, 1);
    Box(r, m, 0, 0, -M, 0, 6 * M, 8 * M, 4 * M);
    r.MeshRoof(m, 0, 2, 6 * M, 4 * M, 7 * M, 12 * M, M / 2);
    r.MeshLimb(m, 0, 0, 6 * M, -M, 0, 6 * M, 6 * M, 0, 3 * M, 3 * M, 3 * M, 3 * M, 8, 2);       // apse
    r.MeshConeAt(m, 0, 2, 6 * M, 0, 7 * M / 2, 6 * M, 9 * M, 8, 0);
    for (i = -1; i <= 1; i += 2) r.MeshLimb(m, 0, 1, -8 * M, -M, i * 2 * M, -8 * M, 6 * M, i * 2 * M, 30, 30, 26, 26, 5, 0);
    Box(r, m, 0, -15 * M / 2, 6 * M, 0, 2 * M, M / 2, 3 * M);
    r.MeshConeAt(m, 0, 3, -6 * M - 10, 0, 3 * M / 4, 9 * M, 9 * M, 8, 0);                     // round window disc
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    // rotate the window disc to face -x: simplest is to keep it as a flat ornament on the gable
    {
    Mesh& l = cm[CT_TEMPLE_LOD];
    r.MeshInit(l, 30, 36, 1);
    Box(r, l, 0, 0, -M, 0, 6 * M, 8 * M, 4 * M);
    r.MeshRoof(l, 0, 2, 6 * M, 4 * M, 7 * M, 12 * M, M / 2);
    r.MeshFinish(l);
    }
    Def(d, &m, &cm[CT_TEMPLE_LOD], 7 * M, 4 * M, 0, 160, 60);
    Col(d, 0, 150, 146, 136, 1); Col(d, 1, 170, 166, 156, 1); Col(d, 2, 70, 84, 100, 1); Col(d, 3, 120, 170, 220, 1);
    iCityType[CT_TEMPLE] = r.RegisterStatic(d);
    }
    // ---- statue plinth and the hero statue (man model, stone coloured, scaled)
    {
    Mesh& m = cm[CT_PLINTH];
    r.MeshInit(m, 32, 48, 1);
    Box(r, m, 0, 0, -M / 2, 0, 3 * M / 2, 2 * M + M / 2, 3 * M / 2);
    Box(r, m, 1, 0, 0, 0, 2 * M, M / 2, 2 * M);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def(d, &m, 0, 2 * M, 2 * M, 0, 120, 0);
    STONE(d, 0); DSTONE(d, 1);
    iCityType[CT_PLINTH] = r.RegisterStatic(d);
    Def(d, &iMHuman[0], &iMHumanLod[0], 0, 0, 0, 120, 30);
    for (i = 0; i < 7; i++) Col(d, i, 150, 148, 140, 1);
    d.yOff = 2 * M;
    d.frame = 18;   // sword raised
    iCityType[CT_STATUE] = r.RegisterStatic(d);
    }
    // ---- windmill: a hollow tapered tower you can walk into. The sails (separate, turning slowly) drive the
    //      wind shaft down through the tower to an edge-runner millstone crushing wheat in a stone trough.
    {
    Mesh& m = cm[CT_WINDMILL];
    r.MeshInit(m, 420, 520, 1);
    const int SIDES = 12;
    int k;
    for (k = 0; k < SIDES; k++)
        {
        int a0 = k * 1024 / SIDES, a1 = (k + 1) * 1024 / SIDES, am = (a0 + a1) / 2;
        int door = r3d_cos(am) > 15000;                      // the panel facing +z holds the doorway
        int lv;
        for (lv = 0; lv < 2; lv++)
            {
            int y0 = lv ? 3 * M : 0, y1 = lv ? 9 * M : 3 * M;
            int r0 = lv ? 27 * M / 10 : 3 * M, r1 = lv ? 2 * M + M / 5 : 27 * M / 10;
            int yb = y0;
            if (door && !lv) yb = 5 * M / 2;                   // lintel above a 2.5 m door
            if (yb >= y1) continue;
            int rb = r0 + (r1 - r0) * (yb - y0) / (y1 - y0);
            int s0 = r3d_sin(a0), c0 = r3d_cos(a0), s1 = r3d_sin(a1), c1 = r3d_cos(a1);
            int o[12] = { (s0 * rb) >> 14, yb, (c0 * rb) >> 14,  (s1 * rb) >> 14, yb, (c1 * rb) >> 14,
                          (s1 * r1) >> 14, y1, (c1 * r1) >> 14,  (s0 * r1) >> 14, y1, (c0 * r1) >> 14 };
            r.MeshQuad(m, 0, 0, o, 0, (yb + y1) / 2, 0);                      // outside
            int ib = rb - M / 4, i1 = r1 - M / 4;
            int in[12] = { (s0 * ib) >> 14, yb, (c0 * ib) >> 14,  (s1 * ib) >> 14, yb, (c1 * ib) >> 14,
                           (s1 * i1) >> 14, y1, (c1 * i1) >> 14,  (s0 * i1) >> 14, y1, (c0 * i1) >> 14 };
            int sm = r3d_sin(am), cm2 = r3d_cos(am);
            r.MeshQuad(m, 0, 7, in, (sm * 6 * M) >> 14, (yb + y1) / 2, (cm2 * 6 * M) >> 14);   // inside (faces the axis)
            }
        if (door)
            {
            // door jambs through the wall thickness
            int s0 = r3d_sin(a0), c0 = r3d_cos(a0), s1 = r3d_sin(a1), c1 = r3d_cos(a1);
            int j0[12] = { (s0 * 3 * M) >> 14, 0, (c0 * 3 * M) >> 14,  (s0 * (3 * M - M / 4)) >> 14, 0, (c0 * (3 * M - M / 4)) >> 14,
                           (s0 * (3 * M - M / 4)) >> 14, 5 * M / 2, (c0 * (3 * M - M / 4)) >> 14,  (s0 * 3 * M) >> 14, 5 * M / 2, (c0 * 3 * M) >> 14 };
            r.MeshQuad(m, 0, 4, j0, (s1 * 3 * M) >> 14, M, (c1 * 3 * M) >> 14);
            int j1[12] = { (s1 * 3 * M) >> 14, 0, (c1 * 3 * M) >> 14,  (s1 * (3 * M - M / 4)) >> 14, 0, (c1 * (3 * M - M / 4)) >> 14,
                           (s1 * (3 * M - M / 4)) >> 14, 5 * M / 2, (c1 * (3 * M - M / 4)) >> 14,  (s1 * 3 * M) >> 14, 5 * M / 2, (c1 * 3 * M) >> 14 };
            r.MeshQuad(m, 0, 4, j1, (s0 * 3 * M) >> 14, M, (c0 * 3 * M) >> 14);
            }
        }
    r.MeshLimb(m, 0, 3, 0, -M, 0, 0, 0, 0, 13 * M / 4, 13 * M / 4, 3 * M, 3 * M, SIDES, 0);   // stone footing
    r.MeshConeAt(m, 0, 4, 0, 0, 3 * M - M / 4, 6, 7, SIDES, 0);                           // plank floor
    r.MeshCone(m, 0, 1, 5 * M / 2, 9 * M, 12 * M, SIDES, 0);                              // cap
    r.MeshConeAt(m, 0, 4, 0, 0, 2 * M, 17 * M / 2, 17 * M / 2 - 2, SIDES, 0);              // ceiling, seen from below
    // wind shaft: horizontal from the sails into the cap, vertical down to the millstone; crown gear
    r.MeshLimb(m, 0, 4, 0, 9 * M, -3 * M, 0, 9 * M, 0, 24, 24, 24, 24, 6, 3);
    r.MeshLimb(m, 0, 4, 0, 9 * M, 0, 0, M / 2, 0, 26, 26, 26, 26, 6, 3);
    r.MeshLimb(m, 0, 4, 0, 85 * M / 10, 0, 0, 87 * M / 10, 0, 3 * M / 4, 3 * M / 4, 3 * M / 4, 3 * M / 4, 10, 3);
    // trough of wheat round the foot of the shaft
    r.MeshLimb(m, 0, 3, 0, 6, 0, 0, M / 2, 0, 3 * M / 2, 3 * M / 2, 3 * M / 2, 3 * M / 2, 12, 0);
    r.MeshConeAt(m, 0, 5, 0, 0, 7 * M / 5, M / 2 - 4, M / 2 + 12, 12, 0);
    // sacks of grain and flour inside
    r.MeshEllipsoid(m, 0, 6, 2 * M, 50, -M / 2, 46, 54, 40, 6, 3);
    r.MeshEllipsoid(m, 0, 6, 9 * M / 5, 44, M / 3, 42, 48, 38, 6, 3);
    r.MeshEllipsoid(m, 0, 5, -2 * M, 30, M / 2, 60, 34, 60, 6, 2);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    {
    Mesh& l = cm[CT_MILL_LOD];
    r.MeshInit(l, 40, 40, 1);
    r.MeshLimb(l, 0, 0, 0, -M, 0, 0, 9 * M, 0, 3 * M, 3 * M, 2 * M + M / 5, 2 * M + M / 5, 6, 0);
    r.MeshCone(l, 0, 1, 5 * M / 2, 9 * M, 12 * M, 6, 0);
    r.MeshFinish(l);
    }
    Def(d, &m, &cm[CT_MILL_LOD], 0, 0, 0, 175, 60);
    Col(d, 0, 200, 190, 170, 1); Col(d, 1, 96, 74, 52, SL_TEX | TXI_SHINGLE); Col(d, 3, 126, 122, 114, SL_TEX | 7);
    Col(d, 4, 112, 82, 52, SL_TEX | TXI_PLANKS); Col(d, 5, 214, 178, 86, SL_TEX | TXI_FIELD); Col(d, 6, 196, 182, 150, 6); Col(d, 7, 150, 132, 104, SL_TEX | TXI_PLANKS);
    iCityType[CT_WINDMILL] = r.RegisterStatic(d);
    }
    // ---- sails: built round their hub, turned smoothly about the wind shaft (16 s a turn)
    {
    Mesh& m = cm[CT_MILLSAILS];
    r.MeshInit(m, 48, 40, 1);
    int k;
    for (k = 0; k < 4; k++)
        {
        int a = k * 256;
        int ca = r3d_cos(a), sa = r3d_sin(a);
        int L = 7 * M, W = M;
        int ex = (ca * L) >> 14, ey = (sa * L) >> 14;
        int wx = (-sa * W) >> 14, wy = (ca * W) >> 14;
        int q[12] = { (ca * M) >> 14, (sa * M) >> 14, 0,  ex, ey, 0,  ex + wx, ey + wy, 0,  ((ca * M) >> 14) + wx, ((sa * M) >> 14) + wy, 0 };
        r.MeshQuad(m, 0, 0, q, 0, 0, M);
        r.MeshQuad(m, 0, 0, q, 0, 0, -M);
        }
    r.MeshLimb(m, 0, 1, 0, 0, -30, 0, 0, 30, 40, 40, 40, 40, 6, 3);       // hub
    r.MeshFinish(m);
    Def(d, &m, 0, 0, 0, 0, 175, 0);
    Col(d, 0, 214, 204, 184, 6); DWOOD(d, 1);
    d.flags = SD_SPINZ; d.spinMs = 16000; d.yOff = 9 * M; d.pivZ = -3 * M;
    iCityType[CT_MILLSAILS] = r.RegisterStatic(d);
    }
    // ---- edge-runner millstone on its axle, rolling round the trough (one model, turned smoothly, 16 s)
    {
    Mesh& m = cm[CT_MILLSTONE];
    r.MeshInit(m, 60, 80, 1);
    int wr = 3 * M / 4, cx = 3 * M / 4, cy = M / 2 + wr - 20;
    r.MeshLimb(m, 0, 1, 0, cy, 0, cx + 60, cy, 0, 10, 10, 10, 10, 4, 0);
    r.MeshLimb(m, 0, 0, cx - 80, cy, 0, cx + 80, cy, 0, wr, wr, wr, wr, 10, 3);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def(d, &m, 0, 0, 0, 0, 40, 0);
    Col(d, 0, 150, 146, 138, SL_TEX | 1); DWOOD(d, 1);
    d.flags = SD_SPINY; d.spinMs = 16000;
    iCityType[CT_MILLSTONE] = r.RegisterStatic(d);
    }
    // ---- outer watchtower: square stone tower, wooden fighting platform and roof
    {
    Mesh& m = cm[CT_WATCH];
    r.MeshInit(m, 96, 128, 1);
    Box(r, m, 0, 0, -2 * M, 0, 5 * M / 2, 16 * M, 5 * M / 2);
    Box(r, m, 1, 0, 14 * M, 0, 7 * M / 2, M / 2, 7 * M / 2);
    for (i = 0; i < 4; i++)
        Beam(r, m, 1, (i & 1 ? 3 : -3) * M, 14 * M, (i & 2 ? 3 : -3) * M, (i & 1 ? 3 : -3) * M, 17 * M, (i & 2 ? 3 : -3) * M, 18);
    r.MeshCone(m, 0, 2, 5 * M, 17 * M, 21 * M, 4, 128);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def(d, &m, 0, 5 * M / 2, 5 * M / 2, 0, 175, 0);
    DSTONE(d, 0); WOOD(d, 1); ROOF(d, 2);
    iCityType[CT_WATCH] = r.RegisterStatic(d);
    }
    // ---- banner: pole + hanging cloth (dark red with a gold band)
    {
    Mesh& m = cm[CT_BANNER];
    r.MeshInit(m, 220, 300, 1);
    Beam(r, m, 0, 0, 0, -36, 0, 7 * M, -36, 12);
    Beam(r, m, 0, -M, 13 * M / 2, -10, M, 13 * M / 2, -10, 8);
    Beam(r, m, 0, 0, 13 * M / 2, -36, 0, 13 * M / 2, -10, 6);
    // cloth: a thin slab so both faces are real; the city's emblem stands 3 cm off each face:
    // a gold sun above twin peaks (the wind over the hills), with a gold hem
    Box(r, m, 1, 0, 3 * M, 0, M - 20, 7 * M / 2, 4);
    int side;
    for (side = -1; side <= 1; side += 2)
        {
        int z = side * 12;
        int k;
        // sun: 10-gon fan
        int sx = 0, sy = 26 * M / 5, sr = 2 * M / 5;
        for (k = 0; k < 10; k++)
            {
            int a0 = k * 1024 / 10, a1 = (k + 1) * 1024 / 10;
            int tri[9] = { sx, sy, z,  sx + ((r3d_cos(a0) * sr) >> 14), sy + ((r3d_sin(a0) * sr) >> 14), z,
                           sx + ((r3d_cos(a1) * sr) >> 14), sy + ((r3d_sin(a1) * sr) >> 14), z };
            r.MeshTri(m, 0, 2, tri, 0, sy, -side * M);
            }
        // rays
        for (k = 0; k < 8; k++)
            {
            int a = k * 128 + 64;
            int ca = r3d_cos(a), sa = r3d_sin(a);
            int r0 = sr + 16, r1 = sr + 70;
            int tri[9] = { sx + ((ca * r1) >> 14), sy + ((sa * r1) >> 14), z,
                           sx + ((ca * r0 - sa * 22) >> 14), sy + ((sa * r0 + ca * 22) >> 14), z,
                           sx + ((ca * r0 + sa * 22) >> 14), sy + ((sa * r0 - ca * 22) >> 14), z };
            r.MeshTri(m, 0, 2, tri, 0, sy, -side * M);
            }
        // twin peaks
        int p1[9] = { -4 * M / 5, 4 * M, z,  -M / 5, 49 * M / 10, z,  2 * M / 5, 4 * M, z };
        r.MeshTri(m, 0, 2, p1, 0, 4 * M, -side * M);
        int p2[9] = { 0, 4 * M, z,  M / 2, 46 * M / 10, z,  4 * M / 5, 4 * M, z };
        r.MeshTri(m, 0, 2, p2, 0, 4 * M, -side * M);
        // hem
        int h[12] = { -M + 30, 32 * M / 10, z,  M - 30, 32 * M / 10, z,  M - 30, 35 * M / 10, z,  -M + 30, 35 * M / 10, z };
        r.MeshQuad(m, 0, 2, h, 0, 33 * M / 10, -side * M);
        }
    r.MeshFinish(m);
    Def(d, &m, 0, 0, 0, 0, 90, 0);
    DWOOD(d, 0); Col(d, 1, 140, 30, 30, 6); Col(d, 2, 230, 190, 80, 1);
    iCityType[CT_BANNER] = r.RegisterStatic(d);
    }
    // ---- stone stairs between the levels: 12.5 cm risers, 22 cm treads, solid sides; per kind the number of steps and
    //      the half width: 32 (4 m up), 40 (5 m), wide 32 and wide 40 (6 m across), 24 (3 m), 16 (2 m)
    {
    static const int KStair[6 * 4] = { CT_STAIRS, CT_STAIRS_LOD, 32, 125,  CT_STAIRS2, CT_STAIRS2_LOD, 40, 125,
                                       CT_STAIRSW, CT_STAIRSW_LOD, 32, 300,  CT_STAIRS2W, CT_STAIRS2W_LOD, 40, 300,
                                       CT_STAIRS24, CT_STAIRS24_LOD, 24, 200,  CT_STAIRS16, CT_STAIRS16_LOD, 16, 250 };
    int k;
    for (k = 0; k < 6; k++)
        {
        const int* t = KStair + k * 4;
        Mesh& m = cm[t[0]];
        int n = t[2], hw = t[3] * M / 100;
        r.MeshInit(m, n * 8 + 40, n * 4 + 40, 1);
        Steps(r, m, 0, 1, 0, 0, 0, hw, n, M / 8, 22 * M / 100);
        r.MeshFinish(m);
        r.MeshPlanarUV(m, 2);
        // far off: a plain slope between solid sides
        Mesh& l = cm[t[1]];
        r.MeshInit(l, 16, 8, 1);
        {
        int len = n * 22 * M / 100, rise = n * M / 8;
        int q[12] = { -hw, 0, 0,  hw, 0, 0,  hw, rise, len,  -hw, rise, len };
        r.MeshQuad(l, 0, 0, q, 0, -M, len + M);
        int sd;
        for (sd = -1; sd <= 1; sd += 2)
            {
            int tr[9] = { sd * hw, 0, 0,  sd * hw, rise, len,  sd * hw, 0, len };
            r.MeshTri(l, 0, 1, tr, -sd * 4 * hw, rise / 3, len / 2);
            }
        }
        r.MeshFinish(l);
        r.MeshPlanarUV(l, 2);
        Def(d, &m, &l, 0, 0, 0, 120, 30);
        Col(d, 0, 140, 136, 128, SL_TEX | 7); Col(d, 1, 120, 116, 108, SL_TEX | 7);
        d.drawDist = 90;
        iCityType[t[0]] = r.RegisterStatic(d);
        }
    }
    // ---- a drain: where a channel ends, its water runs down through an iron grate between stone kerbs, under a lintel
    //      slab (the channel comes in along local +z)
    {
    Mesh& m = cm[CT_DRAIN];
    r.MeshInit(m, 120, 140, 1);
    Box(r, m, 0, -9 * M / 10, -M / 2, 0, M / 5, M / 2 + M / 4, M);                  // the kerbs
    Box(r, m, 0, 9 * M / 10, -M / 2, 0, M / 5, M / 2 + M / 4, M);
    Box(r, m, 0, 0, -M / 2, -M, 11 * M / 10, M / 2 + M / 3, M / 5);                // the end wall, a slab over it
    Box(r, m, 0, 0, M / 4, -M + M / 4, 11 * M / 10, M / 8, M / 2);
    int q[12] = { -7 * M / 10, -M / 4, -4 * M / 5,  7 * M / 10, -M / 4, -4 * M / 5,  7 * M / 10, -M / 4, M,  -7 * M / 10, -M / 4, M };
    r.MeshQuad(m, 0, 1, q, 0, -M, 0);                                            // dark water going down
    for (i = 0; i < 5; i++) Beam(r, m, 2, -3 * M / 5 + i * 3 * M / 10, -M / 8, -4 * M / 5, -3 * M / 5 + i * 3 * M / 10, -M / 8, M / 5, 8);   // the grate
    Beam(r, m, 2, -7 * M / 10, -M / 8, -M / 4, 7 * M / 10, -M / 8, -M / 4, 8);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def(d, &m, 0, 0, 0, 0, 60, 0);
    STONE(d, 0); Col(d, 1, 24, 32, 38, 1); Col(d, 2, 40, 40, 44, 1);
    iCityType[CT_DRAIN] = r.RegisterStatic(d);
    }
    // ---- a stone pier: a square block of masonry 5 m across and 10 m high that carries the landings of the stairs up
    //      to Highhall over the water; its top is walked on (placed with a preset height)
    {
    Mesh& m = cm[CT_PIER];
    r.MeshInit(m, 40, 40, 1);
    Box(r, m, 0, 0, 0, 0, 5 * M / 2, 10 * M, 5 * M / 2);
    Box(r, m, 1, 0, 10 * M - M / 3, 0, 5 * M / 2 + 12, M / 3, 5 * M / 2 + 12);       // a coping course round the top
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def(d, &m, 0, 0, 0, 0, 150, 0);
    STONE(d, 0); DSTONE(d, 1);
    d.hy = 10 * M; d.parts = KPierParts; d.nparts = 1;
    iCityType[CT_PIER] = r.RegisterStatic(d);
    }
    // ---- smithy: open shed with forge glow and anvil
    {
    Mesh& m = cm[CT_FORGE];
    r.MeshInit(m, 128, 160, 1);
    for (i = 0; i < 4; i++) Beam(r, m, 0, (i & 1 ? 3 : -3) * M, 0, (i & 2 ? 2 : -2) * M, (i & 1 ? 3 : -3) * M, 7 * M / 2, (i & 2 ? 2 : -2) * M, 20);
    r.MeshRoof(m, 0, 1, 3 * M, 2 * M, 7 * M / 2, 6 * M, M / 2);
    Box(r, m, 3, -M, 0, M, 3 * M / 2, 3 * M / 2, M);           // forge hearth
    Box(r, m, 4, -M, 3 * M / 2, M, M, M / 6, 3 * M / 4);       // glowing coals
    Box(r, m, 5, 2 * M, 0, -M / 2, M / 4, M, M / 3);           // anvil
    Box(r, m, 5, 2 * M, M, -M / 2, M / 2, M / 4, M / 4);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def(d, &m, 0, 3 * M, 2 * M, 0, 100, 0);
    WOOD(d, 0); ROOF(d, 1); WOOD(d, 2); DSTONE(d, 3); Col(d, 4, 255, 120, 30, 1); Col(d, 5, 54, 54, 58, 1);
    d.flags |= SD_WALLED;
    iCityType[CT_FORGE] = r.RegisterStatic(d);
    }
    // ---- haystack, barrel, fence
    {
    Mesh& m = cm[CT_HAY];
    r.MeshInit(m, 32, 32, 1);
    r.MeshCone(m, 0, 0, 3 * M / 2, 0, 3 * M, 7, 0);
    r.MeshFinish(m);
    Def(d, &m, 0, 0, 0, 3 * M / 2, 90, 0);
    Col(d, 0, 210, 180, 90, 6);
    iCityType[CT_HAY] = r.RegisterStatic(d);
    Mesh& b = cm[CT_BARREL];
    r.MeshInit(b, 32, 32, 1);
    r.MeshLimb(b, 0, 0, 0, 0, 0, 0, 5 * M / 4, 0, M / 2, M / 2, M / 2, M / 2, 7, 2);
    r.MeshFinish(b);
    Def(d, &b, 0, 0, 0, M / 2, 60, 0);
    WOOD(d, 0);
    d.flags |= SD_WALLED;
    iCityType[CT_BARREL] = r.RegisterStatic(d);
    Mesh& f = cm[CT_FENCE];
    r.MeshInit(f, 48, 72, 1);
    Beam(r, f, 0, -2 * M, 0, 0, -2 * M, 3 * M / 2, 0, 12);
    Beam(r, f, 0, 2 * M, 0, 0, 2 * M, 3 * M / 2, 0, 12);
    Beam(r, f, 0, -2 * M, 5 * M / 4, 0, 2 * M, 5 * M / 4, 0, 8);
    Beam(r, f, 0, -2 * M, 2 * M / 3, 0, 2 * M, 2 * M / 3, 0, 8);
    r.MeshFinish(f);
    Def(d, &f, 0, 2 * M, M / 6, 0, 70, 0);
    DWOOD(d, 0);
    iCityType[CT_FENCE] = r.RegisterStatic(d);
    }
    // ---- stable block: open-fronted timber shed with four stalls, mangers, hay, sloping shingle roof
    {
    Mesh& m = cm[CT_STABLE];
    r.MeshInit(m, 340, 600, 1);
    Box(r, m, 0, 0, -M, 0, 6 * M + 20, M + 10, 2 * M + 20);                     // plank floor / plinth
    Box(r, m, 1, 0, 10, 2 * M, 6 * M, 7 * M / 2, M / 8);                          // back wall (+z)
    Box(r, m, 1, -6 * M, 10, 0, M / 8, 3 * M, 2 * M);                             // end walls
    Box(r, m, 1, 6 * M, 10, 0, M / 8, 3 * M, 2 * M);
    for (i = 0; i < 5; i++)
        {
        int x = (i * 3 - 6) * M;
        Beam(r, m, 2, x, 10, -2 * M, x, 11 * M / 4, -2 * M, 20);                  // front posts
        if (i > 0 && i < 4) Box(r, m, 2, x, 10, M / 2, M / 12, 5 * M / 4, 3 * M / 2);   // stall partitions
        }
    Beam(r, m, 2, -6 * M, 11 * M / 4, -2 * M, 6 * M, 11 * M / 4, -2 * M, 18);      // front beam
    // single-pitch roof, high at the back
    int rf[12] = { -13 * M / 2, 11 * M / 4, -11 * M / 4,  13 * M / 2, 11 * M / 4, -11 * M / 4,  13 * M / 2, 15 * M / 4, 5 * M / 2,  -13 * M / 2, 15 * M / 4, 5 * M / 2 };
    r.MeshQuad(m, 0, 3, rf, 0, 0, 0);
    r.MeshQuad(m, 0, 3, rf, 0, 9 * M, 0);
    for (i = 0; i < 4; i++)
        {
        int x = (i * 3 - 9) * M / 2;
        Box(r, m, 2, x, 10, 7 * M / 4, 5 * M / 4, 3 * M / 4, M / 4);                // manger
        r.MeshEllipsoid(m, 0, 4, x, 3 * M / 4, 7 * M / 4, M, M / 4, M / 5, 6, 2);   // hay in it
        r.MeshEllipsoid(m, 0, 4, x + (i & 1 ? 40 : -60), 30, M / 2, 3 * M / 4, M / 6, M / 2, 6, 2);   // straw
        }
    r.MeshFinish(m);
    r.MeshPlanarUV(m);
    Def(d, &m, 0, 6 * M, 2 * M + 20, 0, 140, 0);
    Col(d, 0, 110, 84, 58, SL_TEX | TXI_PLANKS); Col(d, 1, 108, 80, 52, SL_TEX | TXI_PLANKS); DWOOD(d, 2); Col(d, 3, 96, 74, 52, SL_TEX | TXI_SHINGLE); Col(d, 4, 206, 178, 96, 6);
    iCityType[CT_STABLE] = r.RegisterStatic(d);
    }
    // ---- carriage: wagon bed with sides, four spoked wheels, driver's bench and shafts
    {
    Mesh& m = cm[CT_CART];
    r.MeshInit(m, 200, 260, 1);
    Box(r, m, 0, 0, 3 * M / 4, 0, 3 * M / 5, M / 10, 6 * M / 5);
    Box(r, m, 0, -3 * M / 5, 3 * M / 4, 0, M / 20, M / 2, 6 * M / 5);
    Box(r, m, 0, 3 * M / 5, 3 * M / 4, 0, M / 20, M / 2, 6 * M / 5);
    Box(r, m, 0, 0, 3 * M / 4, -6 * M / 5, 3 * M / 5, M / 2, M / 20);
    Box(r, m, 1, 0, 5 * M / 4, 13 * M / 10, 3 * M / 5, M / 12, M / 4);           // bench
    for (i = 0; i < 4; i++)
        {
        int x = i & 1 ? 7 * M / 10 : -7 * M / 10, z = i & 2 ? 4 * M / 5 : -4 * M / 5;
        r.MeshLimb(m, 0, 2, x - 12, M / 2, z, x + 12, M / 2, z, M / 2, M / 2, M / 2, M / 2, 10, 3);   // wheel
        r.MeshLimb(m, 0, 3, x - 16, M / 2, z, x + 16, M / 2, z, 30, 30, 30, 30, 5, 3);                // hub
        }
    Beam(r, m, 1, -M / 3, 4 * M / 5, 6 * M / 5, -M / 3, M / 2, 3 * M, 10);           // shafts
    Beam(r, m, 1, M / 3, 4 * M / 5, 6 * M / 5, M / 3, M / 2, 3 * M, 10);
    Box(r, m, 4, 0, 4 * M / 5 + 20, -M / 3, M / 3, M / 3, M / 3);                 // a crate of goods
    r.MeshFinish(m);
    r.MeshPlanarUV(m);
    Def(d, &m, 0, 4 * M / 5, 7 * M / 4, 0, 90, 0);
    Col(d, 0, 118, 88, 58, SL_TEX | TXI_PLANKS); DWOOD(d, 1); Col(d, 2, 80, 60, 42, 4); Col(d, 3, 60, 60, 64, 1); Col(d, 4, 140, 110, 70, SL_TEX | TXI_PLANKS);
    d.drawDist = 55;
    iCityType[CT_CART] = r.RegisterStatic(d);
    }
    // ---- hay bales
    {
    Mesh& m = cm[CT_BALE];
    r.MeshInit(m, 24, 36, 1);
    Box(r, m, 0, 0, 0, 0, 3 * M / 5, 2 * M / 5, M / 3);
    Box(r, m, 0, M / 4, 2 * M / 5, 0, 3 * M / 5, 2 * M / 5, M / 3);
    r.MeshFinish(m);
    r.MeshPlanarUV(m);
    Def(d, &m, 0, 3 * M / 5, M / 3, 0, 70, 0);
    Col(d, 0, 214, 186, 104, SL_TEX | 3);
    d.drawDist = 45;
    iCityType[CT_BALE] = r.RegisterStatic(d);
    }
    // ---- horses (two coats), slowly idling
    BuildHorse(iMHorse);
    Def(d, &iMHorse, 0, 0, 0, M, 70, 0);
    Col(d, 0, 120, 74, 44, 1); Col(d, 1, 40, 28, 22, 1); Col(d, 2, 50, 44, 40, 1); Col(d, 3, 110, 70, 46, 1); Col(d, 4, 16, 12, 10, 1);
    d.flags = SD_ANIM; d.animMs = 190; d.drawDist = 50;
    iCityType[CT_HORSE] = r.RegisterStatic(d);
    Col(d, 0, 160, 156, 150, 1); Col(d, 1, 220, 216, 206, 1); Col(d, 3, 140, 136, 130, 1);
    iCityType[CT_HORSE2] = r.RegisterStatic(d);
    // ---- stream water: a 4 m piece of moving water (pieces are laid along the stream bed)
    {
    Mesh& m = cm[CT_WATER];
    r.MeshInit(m, 16, 8, 2);
    int fr;
    for (fr = 0; fr < 2; fr++)
        {
        r.iMeshCursor = 0;
        int q[12] = { -7 * M / 4, 0, -9 * M / 4,  7 * M / 4, 0, -9 * M / 4,  7 * M / 4, 0, 9 * M / 4,  -7 * M / 4, 0, 9 * M / 4 };
        r.MeshQuad(m, fr, 0, q, 0, -M, 0);
        // a lighter ripple band that drifts down the piece
        int z0 = fr ? -M : M / 2;
        int w[12] = { -M, 3, z0,  M / 2, 3, z0,  M / 2, 3, z0 + M / 3,  -M, 3, z0 + M / 3 };
        r.MeshQuad(m, fr, 1, w, 0, -M, z0);
        }
    r.MeshFinish(m);
    Def(d, &m, 0, 0, 0, 0, 50, 0);
    Col(d, 0, 58, 98, 120, 1); Col(d, 1, 120, 160, 176, 1);
    d.flags = SD_ANIM; d.animMs = 450;
    iCityType[CT_WATER] = r.RegisterStatic(d);
    }
    // ---- caravan tents, as pitched in Skyrim (open at the front, local +z; you can walk in, collision is the hide
    //      walls only).
    //      Small (one person): an A-frame of rough branches crossed and lashed above the ridge, with a crossbar
    //      along each side; stitched hides over it, the one on the right running on down over the ground as a
    //      flap weighted with stones; a hay pile inside to sleep on (the bedroll is a prop). 1.75 m to the ridge,
    //      2.3 m wide, 2.05 m deep.
    //      Large: the round hide hut of the big caravan and bandit tents: a bell-shaped vault whose skirt spreads
    //      on the ground, a patchwork of pale hides and dark furs, the front hides rolled up over the doorway on a
    //      timber lintel, poles leaning against the front, stones on the hem, a rug inside. 2.5 m high, 4.8 m
    //      wide (5.4 m at the hem), 3.4 m deep.
    {
    int big;
    for (big = 0; big < 2; big++)
        {
        Mesh& m = cm[big ? CT_TENT2 : CT_TENT];
        r.MeshInit(m, big ? 720 : 480, big ? 560 : 460, 1);
        int H = big ? 5 * M / 2 : 7 * M / 4, W = big ? 12 * M / 5 : 115 * M / 100, L = big ? 17 * M / 10 : 103 * M / 100;
        int k, b;
        if (big)
            {
            // the vault's cross-section (x, y in % of W, H), left to right: the skirt flares out at the bottom
            static const int KProf[13 * 2] = { -112, 0,  -103, 12,  -97, 36,  -86, 64,  -64, 87,  -33, 98,  0, 100,
                                               33, 98,  64, 87,  86, 64,  97, 36,  103, 12,  112, 0 };
            // the patchwork: slot per segment, back half / front half (0 worn hide, 1 pale hide, 2 dark fur)
            static const int KPatch[12 * 2] = { 2, 0,  2, 2,  0, 1,  1, 0,  1, 1,  2, 1,  1, 1,  1, 0,  0, 1,  2, 0,  0, 2,  2, 0 };
            int px[13], py[13];
            for (k = 0; k < 13; k++) { px[k] = KProf[k * 2] * W / 100; py[k] = KProf[k * 2 + 1] * H / 100; }
            for (k = 0; k < 12; k++)
                for (b = 0; b < 2; b++)
                    {
                    int z0 = b ? 0 : -L, z1 = b ? L : 0;
                    int q[12] = { px[k], py[k], z0,  px[k + 1], py[k + 1], z0,  px[k + 1], py[k + 1], z1,  px[k], py[k], z1 };
                    int xm = (px[k] + px[k + 1]) / 2, ym = (py[k] + py[k + 1]) / 2;
                    r.MeshQuad(m, 0, KPatch[k * 2 + b], q, 0, H / 4, (z0 + z1) / 2);
                    r.MeshQuad(m, 0, 4, q, xm * 2, ym * 2 + H / 4, (z0 + z1) / 2);
                    }
            // the back, bulging a little
            for (k = 0; k < 12; k++)
                {
                int t[9] = { px[k], py[k], -L,  px[k + 1], py[k + 1], -L,  0, H * 45 / 100, -L - M / 4 };
                r.MeshTri(m, 0, (k & 3) == 1 ? 2 : ((k & 1) ? 1 : 0), t, 0, H / 3, 0);
                r.MeshTri(m, 0, 4, t, 0, H / 3, -3 * L);
                }
            // the front: hides hanging either side of the doorway, and over it
            int dw = W * 42 / 100, dh = H * 78 / 100;
            for (k = -1; k <= 1; k += 2)
                {
                int q[12] = { k * W * 112 / 100, 0, L,  k * dw, 0, L,  k * dw, dh, L,  k * W * 86 / 100, H * 64 / 100, L };
                r.MeshQuad(m, 0, k < 0 ? 2 : 0, q, 0, H / 3, 0);
                r.MeshQuad(m, 0, 4, q, 0, H / 3, 3 * L);
                int t[9] = { k * W * 86 / 100, H * 64 / 100, L,  k * dw, dh, L,  k * W * 64 / 100, H * 87 / 100, L };
                r.MeshTri(m, 0, 1, t, 0, H / 3, 0);
                r.MeshTri(m, 0, 4, t, 0, H / 3, 3 * L);
                }
            {
            int q[12] = { -dw, dh, L,  dw, dh, L,  W * 33 / 100, H * 98 / 100, L,  -W * 33 / 100, H * 98 / 100, L };
            r.MeshQuad(m, 0, 1, q, 0, H / 3, 0);
            r.MeshQuad(m, 0, 4, q, 0, H / 3, 3 * L);
            for (k = -1; k <= 1; k += 2)
                {
                int t[9] = { k * dw, dh, L,  k * W * 64 / 100, H * 87 / 100, L,  k * W * 33 / 100, H * 98 / 100, L };
                r.MeshTri(m, 0, 1, t, 0, H / 3, 0);
                r.MeshTri(m, 0, 4, t, 0, H / 3, 3 * L);
                }
            }
            // the front hides rolled up over the doorway, the timber lintel under them
            r.MeshLimb(m, 0, 1, -W * 62 / 100, H * 93 / 100, L + 30, W * 62 / 100, H * 93 / 100, L + 30, 40, 40, 40, 40, 6, 3);
            r.MeshLimb(m, 0, 3, -dw - 20, dh, L - 20, dw + 20, dh, L - 20, 14, 14, 14, 14, 4, 0);
            // two poles leaning against the front on each side
            for (k = -1; k <= 1; k += 2)
                {
                r.MeshLimb(m, 0, 3, k * (dw + 30), 0, L + M * 7 / 10, k * (dw + 50), dh - 30, L + 8, 14, 14, 11, 11, 4, 0);
                r.MeshLimb(m, 0, 3, k * (W * 112 / 100 + 20), 0, L + M / 2, k * W * 84 / 100, H * 66 / 100, L + 8, 14, 14, 11, 11, 4, 0);
                }
            // stones on the hem
            static const int KStone[4 * 3] = { -108, 10, 60,  -100, -40, 50,  104, 50, 55,  110, -10, 45 };
            for (k = 0; k < 4; k++)
                r.MeshEllipsoid(m, 0, 6, KStone[k * 3] * W / 100, 12, KStone[k * 3 + 1] * L / 100, KStone[k * 3 + 2], 36, KStone[k * 3 + 2] * 4 / 5, 5, 2);
            // a red rug with a blue band down the middle
            int RW = W * 3 / 4, RL = L * 4 / 5;
            int rug[12] = { -RW, 3, -RL,  RW, 3, -RL,  RW, 3, RL,  -RW, 3, RL };
            r.MeshQuad(m, 0, 5, rug, 0, -M, 0);
            int mid[12] = { -16, 4, -RL,  16, 4, -RL,  16, 4, RL,  -16, 4, RL };
            r.MeshQuad(m, 0, 7, mid, 0, -M, 0);
            }
        else
            {
            // hide panels: per side three bands front to back, each two quads (outside, inside); the middle band
            // is a lighter patch, as the stitched hides are never quite the same colour
            for (k = -1; k <= 1; k += 2)
                for (b = 0; b < 3; b++)
                    {
                    int z0 = -L + b * (2 * L) / 3, z1 = -L + (b + 1) * (2 * L) / 3;
                    int sag = (b == 1) ? 6 : 0;                                // the hide sags a little between poles
                    int q[12] = { 0, H, z0,  k * W, 0, z0,  k * W, 0, z1,  0, H, z1 };
                    q[1] -= sag; q[10] -= sag;
                    r.MeshQuad(m, 0, b == 1 ? 1 : 0, q, 0, H / 3, (z0 + z1) / 2);
                    r.MeshQuad(m, 0, 4, q, k * 2 * W, H, (z0 + z1) / 2);          // inside: darker hide
                    if (b) r.MeshLimb(m, 0, 2, 0, H - 8, z0, k * W, 4, z0, 5, 5, 5, 5, 3, 0);
                    }
            // the right-hand hide runs on over the ground as a flap, held down by stones
            {
            int f[12] = { W, 2, -L - 10,  W + 4 * M / 5, 4, -L + 20,  W + 4 * M / 5, 4, L - 10,  W, 2, L + 10 };
            r.MeshQuad(m, 0, 0, f, W / 2, -M, 0);
            static const int KStone[5 * 3] = { 120, -80, 26,  150, -20, 20,  140, 40, 30,  125, 85, 22,  160, 70, 16 };
            for (k = 0; k < 5; k++)
                r.MeshEllipsoid(m, 0, 6, KStone[k * 3] * W / 100, 10, KStone[k * 3 + 1] * L / 100, KStone[k * 3 + 2], KStone[k * 3 + 2] * 4 / 5, KStone[k * 3 + 2], 5, 2);
            }
            // the back: two hides meeting at a seam
            int back[9] = { -W, 0, -L,  W, 0, -L,  0, H, -L };
            r.MeshTri(m, 0, 0, back, 0, H / 3, 0);
            r.MeshTri(m, 0, 4, back, 0, H / 3, -2 * L);
            for (k = -1; k <= 1; k += 2)
                {
                r.MeshLimb(m, 0, 2, 0, H, L + 4, k * W, 0, L + 4, 7, 7, 7, 7, 3, 0);     // front edge trim
                // the A-frame: a straight branch each side at each end, crossing above the ridge and sticking out
                int e;
                for (e = -1; e <= 1; e += 2)
                    {
                    int z = e * (L + 14);
                    r.MeshLimb(m, 0, 3, k * W + k * 12, 0, z, -k * 36, H + 50, z, 13, 13, 9, 9, 4, 3);
                    }
                // a crossbar along each side, half-way up, its ends sticking out past the frame
                r.MeshLimb(m, 0, 3, k * (W * 55 / 100 + 14), H * 45 / 100, -L - 40, k * (W * 55 / 100 + 14), H * 45 / 100, L + 40, 9, 9, 9, 9, 4, 3);
                }
            r.MeshLimb(m, 0, 3, 0, H + 4, -L - 30, 0, H + 4, L + 30, 10, 10, 10, 10, 4, 3);   // ridge pole
            r.MeshLimb(m, 0, 2, -12, H + 2, L + 14, 12, H + 2, L + 14, 9, 9, 9, 9, 4, 3);       // lashings
            r.MeshLimb(m, 0, 2, -12, H + 2, -L - 14, 12, H + 2, -L - 14, 9, 9, 9, 9, 4, 3);
            // a pile of hay to sleep on
            r.MeshEllipsoid(m, 0, 5, 0, 4, -L / 5, W * 62 / 100, 16, L * 72 / 100, 6, 2);
            }
        r.MeshFinish(m);
        r.MeshPlanarUV(m);
        Def(d, &m, 0, 0, 0, 0, 65, 0);
        Col(d, 0, 186, 154, 112, 6); Col(d, 1, 214, 190, 150, 6); Col(d, 2, 82, 56, 36, 4); Col(d, 3, 104, 78, 50, 4);
        Col(d, 4, 128, 100, 70, 6); Col(d, 5, 196, 170, 96, 6); Col(d, 6, 120, 118, 112, SL_TEX | TXI_ROCK); Col(d, 7, 50, 76, 140, 6);
        if (big) { Col(d, 0, 166, 128, 88, 6); Col(d, 1, 232, 220, 192, 6); Col(d, 2, 104, 74, 52, SL_TEX | TXI_FUR); Col(d, 4, 92, 64, 44, SL_TEX | TXI_FUR); Col(d, 5, 150, 40, 34, 6); }
        d.parts = big ? KTentParts2 : KTentParts; d.nparts = big ? 5 : 3;
        iCityType[big ? CT_TENT2 : CT_TENT] = r.RegisterStatic(d);
        }
    }
    // ---- tanning rack: two posts, a cross pole, a hide laced into a frame of sticks
    {
    Mesh& m = cm[CT_TANRACK];
    r.MeshInit(m, 80, 120, 1);
    Beam(r, m, 0, -M, 0, 0, -M, 2 * M, 0, 12);
    Beam(r, m, 0, M, 0, 0, M, 2 * M, 0, 12);
    Beam(r, m, 0, -M - 20, 2 * M - 10, 0, M + 20, 2 * M - 10, 0, 9);
    Beam(r, m, 0, -4 * M / 5, M / 3, 0, 4 * M / 5, M / 3, 0, 7);
    int h[12] = { -3 * M / 4, M / 2, 6,  3 * M / 4, M / 2, 6,  2 * M / 3, 9 * M / 5, 6,  -2 * M / 3, 9 * M / 5, 6 };
    r.MeshQuad(m, 0, 1, h, 0, M, -M);
    r.MeshQuad(m, 0, 2, h, 0, M, M);
    r.MeshFinish(m);
    Def(d, &m, 0, 0, 0, 30, 55, 0);
    DWOOD(d, 0); Col(d, 1, 176, 140, 100, 6); Col(d, 2, 150, 116, 84, 6);
    iCityType[CT_TANRACK] = r.RegisterStatic(d);
    }
    // ---- a crate of produce (a container prop: cabbages and potatoes showing over the top)
    {
    Mesh& m = cm[CT_CRATE];
    r.MeshInit(m, 80, 120, 1);
    Box(r, m, 0, 0, 0, 0, 45, 90, 35);
    r.MeshEllipsoid(m, 0, 1, -18, 96, -6, 20, 16, 20, 5, 2);
    r.MeshEllipsoid(m, 0, 1, 16, 94, 8, 18, 14, 18, 5, 2);
    r.MeshEllipsoid(m, 0, 2, 4, 100, -14, 10, 8, 12, 4, 2);
    r.MeshEllipsoid(m, 0, 2, -6, 100, 16, 10, 8, 12, 4, 2);
    r.MeshFinish(m);
    r.MeshPlanarUV(m);
    Def(d, &m, 0, 0, 0, 0, 50, 0);
    Col(d, 0, 140, 104, 64, SL_TEX | TXI_PLANKS); Col(d, 1, 110, 160, 80, 1); Col(d, 2, 170, 130, 80, 1);
    iCityType[CT_CRATE] = r.RegisterStatic(d);
    }
    // ---- the inner gate before Dragonsreach: a stone arch between two short walls
    {
    Mesh& m = cm[CT_INNERGATE];
    r.MeshInit(m, 60, 90, 1);
    Box(r, m, 0, -7 * M, -M, 0, 5 * M / 2, 6 * M, M / 2);
    Box(r, m, 0, 7 * M, -M, 0, 5 * M / 2, 6 * M, M / 2);
    Box(r, m, 0, -4 * M, -M, 0, M / 2, 8 * M, M);
    Box(r, m, 0, 4 * M, -M, 0, M / 2, 8 * M, M);
    Box(r, m, 1, 0, 11 * M / 2, 0, 9 * M / 2, 3 * M / 2, M);
    r.MeshFinish(m);
    r.MeshPlanarUV(m, 2);
    Def(d, &m, 0, 0, 0, 0, 120, 0);
    STONE(d, 0); DSTONE(d, 1);
    d.parts = KInnerGateParts; d.nparts = 4;
    iCityType[CT_INNERGATE] = r.RegisterStatic(d);
    }
    // ---- torch post: a stout pole with an iron fire basket (its flame is a separate, unlit static)
    {
    Mesh& m = cm[CT_TORCHPOST];
    r.MeshInit(m, 60, 90, 1);
    Beam(r, m, 0, 0, 0, 0, 0, 2 * M, 0, 22);
    r.MeshLimb(m, 0, 1, 0, 2 * M, 0, 0, 2 * M + 70, 0, 26, 26, 44, 44, 6, 1);   // basket
    r.MeshLimb(m, 0, 2, 0, 2 * M + 40, 0, 0, 2 * M + 72, 0, 34, 34, 30, 30, 6, 2); // embers
    r.MeshFinish(m);
    Def(d, &m, 0, 0, 0, 30, 70, 0);
    DWOOD(d, 0); Col(d, 1, 50, 50, 54, 1); Col(d, 2, 255, 120, 40, 1);
    iCityType[CT_TORCHPOST] = r.RegisterStatic(d);
    Def(d, &iMFlame, 0, 0, 0, 0, 70, 0);
    Col(d, 0, 255, 200, 70, 1); Col(d, 1, 240, 96, 24, 1); Col(d, 2, 255, 246, 200, 1);
    d.flags = SD_ANIM | SD_UNLIT; d.animMs = 90; d.yOff = 2 * M + 64;
    iCityType[CT_FLAME] = r.RegisterStatic(d);
    }
    // ---- the drawbridge: a plank deck hinged at the gate end (local z = 0) reaching 10 m along -z, with rails,
    //      cross beams and the chains up to two posts; 5 frames from down to raised (0..80 degrees)
    {
    Mesh& m = cm[CT_BRIDGE];
    r.MeshInit(m, 200, 260, 5);
    int fr;
    for (fr = 0; fr < 5; fr++)
        {
        r.iMeshCursor = 0;
        int a = fr * 57;                                  // 20 degrees a frame
        int ca = r3d_cos(a), sa = r3d_sin(a);
        int L = 10 * M, W = 11 * M / 5;
        // corners of the deck (x, along) raised about the hinge: along d -> (y = d*sin, z = -d*cos)
        int k;
        for (k = 0; k < 5; k++)
            {
            // deck in five plank sections so the raised deck still looks like boards
            int d0 = k * L / 5, d1 = (k + 1) * L / 5 - 6;
            int y0 = (d0 * sa) >> 14, z0 = -((d0 * ca) >> 14), y1 = (d1 * sa) >> 14, z1 = -((d1 * ca) >> 14);
            int top[12] = { -W, y0, z0,  W, y0, z0,  W, y1, z1,  -W, y1, z1 };
            int ym = (y0 + y1) / 2, zm = (z0 + z1) / 2, ny = (ca * M) >> 14, nz = (sa * M) >> 14;   // deck normal * 1 m
            r.MeshQuad(m, fr, 0, top, 0, ym - ny, zm - nz);
            int und[12] = { -W, y0 - 40, z0,  W, y0 - 40, z0,  W, y1 - 40, z1,  -W, y1 - 40, z1 };
            r.MeshQuad(m, fr, 1, und, 0, ym + ny, zm + nz);
            }
        int ey = (L * sa) >> 14, ez = -((L * ca) >> 14);
        int s2;
        for (s2 = -1; s2 <= 1; s2 += 2)
            {
            r.MeshLimb(m, fr, 1, s2 * W, 10, 0, s2 * W, ey + 10, ez, 24, 24, 24, 24, 4, 3);                       // side beam
            r.MeshLimb(m, fr, 1, s2 * W, 2 * M / 3, 0, s2 * W, ey + ((2 * M / 3 * ca) >> 14), ez + ((2 * M / 3 * sa) >> 14), 12, 12, 12, 12, 4, 3);   // rail
            r.MeshLimb(m, fr, 2, s2 * W, ey + 20, ez, s2 * (W + 50), 5 * M, M / 2, 6, 6, 6, 6, 3, 0);            // chain
            }
        // the two posts the chains run to (fixed)
        r.MeshLimb(m, fr, 1, -W - 60, 0, M / 2, -W - 60, 5 * M + 40, M / 2, 30, 30, 26, 26, 5, 3);
        r.MeshLimb(m, fr, 1, W + 60, 0, M / 2, W + 60, 5 * M + 40, M / 2, 30, 30, 26, 26, 5, 3);
        }
    r.MeshFinish(m);
    r.MeshPlanarUV(m);
    Def(d, &m, 0, 0, 0, 0, 90, 0);
    Col(d, 0, 118, 88, 58, SL_TEX | TXI_PLANKS); DWOOD(d, 1); Col(d, 2, 56, 56, 60, 1);
    iCityType[CT_BRIDGE] = r.RegisterStatic(d);
    }
    // ---- the drawbridge lever: a post with a handle that swings over (5 frames)
    {
    Mesh& m = cm[CT_LEVER];
    r.MeshInit(m, 40, 60, 5);
    int fr;
    for (fr = 0; fr < 5; fr++)
        {
        r.iMeshCursor = 0;
        r.MeshBox(m, fr, 0, 0, M, 0, 0, 22, M / 2, 22);
        int a = -160 + fr * 80;
        int hx = 0, hy = M + ((r3d_cos(a) * 3 * M / 4) >> 14), hz = (r3d_sin(a) * 3 * M / 4) >> 14;
        r.MeshLimb(m, fr, 1, 0, M - 10, 0, hx, hy, hz, 8, 8, 8, 8, 4, 3);
        r.MeshEllipsoid(m, fr, 2, hx, hy, hz, 14, 14, 14, 4, 2);
        }
    r.MeshFinish(m);
    Def(d, &m, 0, 0, 0, 0, 50, 0);
    DWOOD(d, 0); Col(d, 1, 60, 60, 64, 1); Col(d, 2, 90, 60, 40, 4);
    iCityType[CT_LEVER] = r.RegisterStatic(d);
    }
    // ---- crates and sacks of the caravan's goods
    {
    Mesh& m = cm[CT_CRATES];
    r.MeshInit(m, 60, 90, 1);
    Box(r, m, 0, -40, 0, 0, 36, 70, 36);
    Box(r, m, 0, 40, 0, 10, 34, 60, 30);
    Box(r, m, 0, -10, 70, 6, 30, 50, 28);
    r.MeshEllipsoid(m, 0, 1, 90, 34, -50, 36, 40, 30, 5, 3);
    r.MeshFinish(m);
    r.MeshPlanarUV(m);
    Def(d, &m, 0, 0, 0, 70, 60, 0);
    Col(d, 0, 140, 104, 64, SL_TEX | TXI_PLANKS); Col(d, 1, 196, 178, 136, 6);
    iCityType[CT_CRATES] = r.RegisterStatic(d);
    }
    // ---- culvert: a stone arch in the hill's foot where the stream comes out from under the city
    {
    Mesh& m = cm[CT_CULVERT];
    r.MeshInit(m, 40, 60, 1);
    Box(r, m, 0, -2 * M, -M, 0, M / 2, 3 * M, M);
    Box(r, m, 0, 2 * M, -M, 0, M / 2, 3 * M, M);
    Box(r, m, 0, 0, 3 * M / 2, 0, 5 * M / 2, M / 2, M);
    Box(r, m, 1, 0, -M, -M / 2, 3 * M / 2, 5 * M / 2, 20);                     // the dark mouth
    r.MeshFinish(m);
    r.MeshPlanarUV(m);
    Def(d, &m, 0, 0, 0, 0, 90, 0);
    STONE(d, 0); Col(d, 1, 16, 20, 22, 1);
    iCityType[CT_CULVERT] = r.RegisterStatic(d);
    }
    }

// ---------------------------------------------------------------- layout
static int Wx(int mx) { return (CITY_X * CELL + CELL / 2) + mx * M; }
static int Wz(int mz) { return (CITY_Z * CELL + CELL / 2) + mz * M; }

void Game::Place(int aCt, int aMx, int aMz, int aYaw, int aScale, int aVar)
    {
    // buildings stand on a levelled pad at the height of their doorstep, so doors meet the ground
    int door = 0, rad = 0;      // local -z distance of the doorstep, pad radius (world units)
    switch (aCt)
        {
        case CT_HOUSE: door = 4 * M; rad = 6 * M; break;
        case CT_HALL: door = 6 * M; rad = 10 * M; break;
        case CT_TEMPLE: door = 0; rad = 8 * M; break;
        case CT_FORGE: door = 0; rad = 5 * M; break;
        case CT_WINDMILL: door = 0; rad = 5 * M; break;
        case CT_WATCH: door = 0; rad = 4 * M; break;
        case CT_STABLE: door = 0; rad = 8 * M; break;
        }
    int x = Wx(aMx), z = Wz(aMz);
    if (rad)
        {
        int yaw = aYaw & 1023;
        int dx = (-door * r3d_sin(yaw)) >> 14, dz = (-door * r3d_cos(yaw)) >> 14;
        int h = iR->GroundAt(x + dx, z + dz);
        iR->Flatten(x, z, rad, h, 4 * M);
        }
    iR->AddStatic(iCityType[aCt], x, z, aYaw & 1023, aScale, aVar);
    }

// yaw facing from (x,z) towards the city point (tx,tz), metres
static int Face(int x, int z, int tx, int tz) { return r3d_atan2((tx - x) * M, (tz - z) * M); }

// ---------------------------------------------------------------- Vindgard, laid out after Whiterun
// Everything inside the walls follows the reference map docs/maps/ref_whiterun_city_map.png, the outside the online
// map's approach detail; the tables in city_plan.inc are generated from the plan by docs/maps/gen_layout.py
// (decimetres in the city frame, heights in decimetres above the lower city).
// Levels: the lower city (gate, Plains, Market) h0; the inn's rise +2 m; the Wind District +4; Jorrvaskr's terrace
// +6; the Skyforge's rock +9; the pool below Highhall +8 (its platform +9); Highhall's terrace +18. The main gate
// faces west into the lower city; the wide stairs climb from the market through the tier wall to the Wind District;
// from the Elder Ash four flights climb to Highhall, the upper three carried over the pool and the moat on piers.
#include "city_plan.inc"
#include "plan_pos.inc"
static int XD(int dm) { return (CITY_X * CELL + CELL / 2) + dm * M / 10; }
static int ZD(int dm) { return (CITY_Z * CELL + CELL / 2) + dm * M / 10; }

// is the point (dm) inside the polygon (dm)
static int InPolyD(const int* p, int n, int x, int z)
    {
    int i, j, c = 0;
    for (i = 0, j = n - 1; i < n; j = i++)
        {
        int xi = p[i * 2], zi = p[i * 2 + 1], xj = p[j * 2], zj = p[j * 2 + 1];
        if ((zi > z) != (zj > z) && x < (xj - xi) * (z - zi) / (zj - zi) + xi) c = !c;
        }
    return c;
    }
// distance (dm) from a point to a segment
static int SegDistD(int px, int pz, int ax, int az, int bx, int bz)
    {
    int dx = bx - ax, dz = bz - az;
    int l2 = dx * dx + dz * dz;
    int t = l2 ? ((((px - ax) * dx + (pz - az) * dz) >> 4) * 256) / ((l2 >> 4) + 1) : 0;
    if (t < 0) t = 0;
    if (t > 256) t = 256;
    int qx = ax + dx * t / 256 - px, qz = az + dz * t / 256 - pz;
    return r3d_isqrt(qx * qx + qz * qz);
    }
// level the ground inside a polygon (dm) to h, blending out over e metres round it
static void FlatPolyD(R3D& r, const int* p, int n, int h, int e)
    {
    if (r.iLandBaked) return;                       // (the land file has it: only baking the land changes it)
    int i, mnx = 1 << 30, mxx = -(1 << 30), mnz = 1 << 30, mxz = -(1 << 30);
    for (i = 0; i < n; i++)
        {
        if (p[i * 2] < mnx) mnx = p[i * 2];
        if (p[i * 2] > mxx) mxx = p[i * 2];
        if (p[i * 2 + 1] < mnz) mnz = p[i * 2 + 1];
        if (p[i * 2 + 1] > mxz) mxz = p[i * 2 + 1];
        }
    int gx0 = XD(mnx - e * 10) / CELL - 1, gx1 = XD(mxx + e * 10) / CELL + 1, gz0 = ZD(mnz - e * 10) / CELL - 1, gz1 = ZD(mxz + e * 10) / CELL + 1;
    if (gx0 < 0) gx0 = 0;
    if (gz0 < 0) gz0 = 0;
    if (gx1 > r.iGX) gx1 = r.iGX;
    if (gz1 > r.iGZ) gz1 = r.iGZ;
    int gx, gz;
    for (gz = gz0; gz <= gz1; gz++)
        for (gx = gx0; gx <= gx1; gx++)
            {
            int x = (gx * CELL - XD(0)) * 10 / M, z = (gz * CELL - ZD(0)) * 10 / M;
            int t = 256;
            if (!InPolyD(p, n, x, z))
                {
                int d = 1 << 30, j;
                for (i = 0, j = n - 1; i < n; j = i++)
                    {
                    int dd = SegDistD(x, z, p[j * 2], p[j * 2 + 1], p[i * 2], p[i * 2 + 1]);
                    if (dd < d) d = dd;
                    }
                if (!e || d >= e * 10) continue;
                t = 256 - d * 256 / (e * 10);
                }
            short& hh = r.iHeight[gz * r.iHS + gx];
            int hv = hh * HS;
            hv += (h - hv) * t >> 8;
            hh = (short)(hv / HS);
            }
    }

void Game::BuildCityTerrain()
    {
    R3D& r = *iR;
    int i, k;
    int h0 = r.GroundAt(Wx(0), Wz(0));
    if (r.iLandLoaded) h0 = 15571 * M / 100;   // Skyrim's: the main gate's sill (-3520 game units = -50.3 m, + 206 m)
    if (h0 < 19 * M) h0 = 19 * M;          // high enough that the plain 10 m below stays above the water
    // on Skyrim's land the city stands on Whiterun's own hill: what of the hill rises above the city's top
    // terrace (18 m over the lower city) round it is cut down, easing back into the land 150..260 m out
    if (r.iLandLoaded && !r.iLandBaked)
        {
        int cx = Wx(0), cz = Wz(27);
        int gx0 = (cx - 260 * M) / CELL, gx1 = (cx + 260 * M) / CELL, gz0 = (cz - 260 * M) / CELL, gz1 = (cz + 260 * M) / CELL;
        int gx, gz;
        for (gz = gz0; gz <= gz1; gz++)
            for (gx = gx0; gx <= gx1; gx++)
                {
                if (gx < 0 || gz < 0 || gx > r.iGX || gz > r.iGZ) continue;
                int dx = (gx * CELL - cx) >> 8, dz = (gz * CELL - cz) >> 8;
                int d = r3d_isqrt(dx * dx + dz * dz);
                int cap = h0 + 20 * M + (d > 150 ? (d - 150) * 60 * M / 110 : 0);
                short& hh = r.iHeight[gz * r.iHS + gx];
                if (hh * HS > cap) hh = (short)(cap / HS);
                }
        }
    // the wall hides low things in the lower city from anyone outside it
    r.iWallX = Wx(-18); r.iWallZ = Wz(-18); r.iWallR = 44 * M; r.iWallTop = h0 + 8 * M;
    // (v0.4.48: the plain below the city's south-west side no longer levelled - Skyrim's own land there, the Stables and
    //  the farms standing on it as Skyrim stands them: the area's file)
    // the hill at the lower city's height, then the rises
    FlatPolyD(r, KPlanOutline, KPlanOutline_N, h0, 18);
    for (i = 0; i < KPlanLevel_N; i++)
        {
        const int* l = KPlanLevel + i * 4;
        FlatPolyD(r, KPlanLevelPt + l[0] * 2, l[1], h0 + l[2] * M / 10, l[3]);
        }
    // paving: the streets, the market square, the Elder Ash's plaza
    r.PaintDisc(Wx(0), Wz(20), 95 * M, TM_GRASS);
    for (i = 0; i < KPlanStreet_N; i++)
        {
        const int* p = KPlanStreetPt + KPlanStreet[i * 2] * 2;
        for (k = 0; k + 1 < KPlanStreet[i * 2 + 1]; k++)
            r.PaintPath(XD(p[k * 2]), ZD(p[k * 2 + 1]), XD(p[k * 2 + 2]), ZD(p[k * 2 + 3]), 2 * M, TM_COBBLE);
        }
    r.PaintDisc(XD(PL_WELL_X), ZD(PL_WELL_Z), 9 * M, TM_COBBLE);
    r.PaintDisc(XD(PL_TREE_X), ZD(PL_TREE_Z), 10 * M, TM_COBBLE);
    // the road from the river road up to the approach, the river road itself, the stables yard
    for (k = 0; k + 1 < KPlanRiverRoad_N; k++)
        r.PaintPath(XD(KPlanRiverRoad[k * 2]), ZD(KPlanRiverRoad[k * 2 + 1]), XD(KPlanRiverRoad[k * 2 + 2]), ZD(KPlanRiverRoad[k * 2 + 3]), 2 * M, TM_DIRT);
    // (v0.4.48: the game's own stables yard and wheat fields gone - Skyrim's outskirts instead)
    }

// lower the ground under a flight of steps (foot (x, z) world, uphill along yaw, n steps of 12.5 cm on 22 cm treads,
// hw half wide) so the steps stand clear of the slope; level the foot and a landing at the top
static void CarveDir(R3D& r, int x, int z, int yaw, int n, int hw, int aBase)
    {
    int sn = r3d_sin(yaw), cs = r3d_cos(yaw);
    int len = n * 22;                                  // cm
    int k;
    // the foot and a landing beyond the top first, then the ground under the flight cut below the treads
    r.Flatten(x - (sn * 3 * M / 2 >> 14), z - (cs * 3 * M / 2 >> 14), hw + 5 * M / 4, aBase, 2 * M);
    int top = len * M / 100 + hw + M;
    r.Flatten(x + (sn * top >> 14), z + (cs * top >> 14), hw + M / 2, aBase + n * M / 8, 3 * M);
    for (k = 0; k * 100 <= len + 60; k++)
        {
        int steps = k * 100 / 22;
        if (steps > n) steps = n;
        int h = aBase + steps * M / 8 - M / 3;
        int px = x + (sn * k * M >> 14), pz = z + (cs * k * M >> 14);
        if (r.GroundAt(px, pz) > h || k == 0) r.Flatten(px, pz, hw + M, k == 0 ? aBase : h, M);
        }
    }

// an iron brazier's flame on a bastion corner, with its light at night
void Game::Brazier(int aX, int aZ, int aScale)
    {
    iR->AddStatic(iCityType[CT_FLAMETOP], aX, aZ, 0, 140, 0);
    AddLightSrc(0, aX, iR->GroundAt(aX, aZ) + (10 * M + 80) * aScale / 256, aZ, 12 * M, 150);
    }

// wall segments along a polyline (dm), facing in (inside is to the right of the direction of travel)
void Game::WallRun(const int* aP, int aN, int aScale)
    {
    int i, k;
    for (i = 0; i + 1 < aN; i++)
        {
        int ax = aP[i * 2], az = aP[i * 2 + 1], bx = aP[i * 2 + 2], bz = aP[i * 2 + 3];
        int dx = bx - ax, dz = bz - az;
        int len = r3d_isqrt(dx * dx + dz * dz);                     // dm
        int seg = 960 * aScale / 2560;                              // dm per segment at this scale
        int n = (len + seg - 1) / seg;
        if (n < 1) n = 1;
        int yaw = r3d_atan2(dz * M, -dx * M);
        for (k = 0; k < n; k++)
            iR->AddStatic(iCityType[CT_WALL], XD(ax) + dx * M / 10 * (2 * k + 1) / (2 * n), ZD(az) + dz * M / 10 * (2 * k + 1) / (2 * n), yaw, aScale, 0);
        }
    }

// a line of water pieces along a polyline (dm) at the ground, narrow (scale 100) or as a canal (160)
static void WaterLine(R3D& r, int aType, const int* p, int n, int aScale)
    {
    int i, k, step = 45 * aScale / 256;                            // dm per piece
    for (i = 0; i + 1 < n; i++)
        {
        int dx = p[i * 2 + 2] - p[i * 2], dz = p[i * 2 + 3] - p[i * 2 + 1];
        int len = r3d_isqrt(dx * dx + dz * dz);
        int np = len / step + 1, yaw = r3d_atan2(dx * M, dz * M);
        for (k = 0; k < np; k++)
            {
            int x = XD(p[i * 2]) + dx * M / 10 * (2 * k + 1) / (2 * np), z = ZD(p[i * 2 + 1]) + dz * M / 10 * (2 * k + 1) / (2 * np);
            int idx = r.AddStatic(aType, x, z, yaw, aScale, 0);
            if (idx >= 0) r.iObj[idx].y = r.GroundAt(x, z) + 6;
            }
        }
    }

void Game::BuildCity()
    {
    R3D& r = *iR;
    int i, k;
    int h0 = r.GroundAt(XD(PL_WELL_X), ZD(PL_WELL_Z));
    // ---- the walls, the towers on them, the main gate
    for (i = 0; i < KPlanWallRun_N; i++)
        WallRun(KPlanWallPt + KPlanWallRun[i * 3] * 2, KPlanWallRun[i * 3 + 1], KPlanWallRun[i * 3 + 2]);
    for (i = 0; i < KPlanWallSeg_N; i++)
        r.AddStatic(iCityType[CT_WALL], XD(KPlanWallSeg[i * 4]), ZD(KPlanWallSeg[i * 4 + 1]), KPlanWallSeg[i * 4 + 2], KPlanWallSeg[i * 4 + 3], 0);
    for (i = 0; i < KPlanTower_N; i++)
        {
        const int* t = KPlanTower + i * 4;
        r.AddStatic(iCityType[CT_BASTION], XD(t[0]), ZD(t[1]), 0, t[2], 0);
        if (t[3]) Brazier(XD(t[0]) + 27 * M / 10 * t[2] / 256, ZD(t[1]) + 27 * M / 10 * t[2] / 256, t[2]);
        }
    int gx = XD(PL_GATE_X), gz = ZD(PL_GATE_Z);
    r.AddStatic(iCityType[CT_BASTION], gx, gz + 13 * M / 2, 0, 256, 0);
    r.AddStatic(iCityType[CT_BASTION], gx, gz - 13 * M / 2, 0, 256, 0);
    Brazier(gx + 27 * M / 10, gz + 13 * M / 2 + 27 * M / 10, 256);
    Brazier(gx - 27 * M / 10, gz - 13 * M / 2 - 27 * M / 10, 256);
    r.AddStatic(iCityType[CT_LINTEL], gx, gz, 256, 256, 0);
    r.AddStatic(iCityType[CT_DOOR], gx + 3 * M, gz + 4 * M, 456, 256, 0);
    r.AddStatic(iCityType[CT_DOOR], gx + 3 * M, gz - 4 * M, 56, 256, 0);
    r.AddStatic(iCityType[CT_BANNER], gx - 6 * M, gz + 6 * M, 256, 256, 0);
    r.AddStatic(iCityType[CT_BANNER], gx - 6 * M, gz - 6 * M, 256, 256, 0);
    // from the lower city the tier wall hides most of the Wind District behind it (the stairs' gap stays open)
    {
    static const int KOcc[2 * 3] = { -415, -92, 71,  10, 230, 62 };          // x0, x1, z (dm)
    r.iOccN = 2;
    for (i = 0; i < 2; i++)
        {
        int* oc = r.iOcc + i * 4;
        oc[0] = XD(KOcc[i * 3]); oc[1] = XD(KOcc[i * 3 + 1]); oc[2] = ZD(KOcc[i * 3 + 2]); oc[3] = h0 + 8 * M;
        }
    }
    // ---- the stairs: cut into the slopes, or standing on the piers over the water
    for (i = 0; i < KPlanStair_N; i++)
        {
        const int* s = KPlanStair + i * 8;
        int x = XD(s[1]), z = ZD(s[2]), base = h0 + s[4] * M / 10, hw = s[6] * M / 100;
        if (s[7]) CarveDir(r, x, z, s[3], s[5], hw, base);
        int idx = r.AddStatic(iCityType[s[0]], x, z, s[3], 256, 0);
        if (idx >= 0) r.iObj[idx].y = base;
        AddRampYaw(x, z, s[3], s[5] * 22 * M / 100, hw, s[5] * M / 8, base);
        }
    for (i = 0; i < KPlanPier_N; i++)
        {
        const int* p = KPlanPier + i * 4;
        int idx = r.AddStatic(iCityType[CT_PIER], XD(p[0]), ZD(p[1]), p[3], 256, 0);
        if (idx >= 0) r.iObj[idx].y = h0 + p[2] * M / 10 - 10 * M;
        }
    for (i = 0; i < KPlanCliff_N; i++)
        {
        const int* c = KPlanCliff + i * 4;
        int idx = r.AddStatic(iCityType[CT_CLIFF], XD(c[0]), ZD(c[1]), c[2], 256, 0);
        if (idx >= 0) r.iObj[idx].y = h0 + c[3] * M / 10 - 13 * M + M / 3;
        }
    // ---- the buildings
    for (i = 0; i < KPlanBuilding_N; i++)
        {
        const int* b = KPlanBuilding + i * 7;
        r.AddStatic(iCityType[b[0]], XD(b[1]), ZD(b[2]), b[3], b[4], b[0] == CT_HOUSE ? b[5] : 0);
        }
    // the market: the well, stalls round it facing it
    r.AddStatic(iCityType[CT_WELL], XD(PL_WELL_X), ZD(PL_WELL_Z), 0, 256, 0);
    iWellX = XD(PL_WELL_X); iWellZ = ZD(PL_WELL_Z);
    static const int KStallType[5] = { CT_STALL, CT_STALL2, CT_STALL3, CT_STALL, CT_STALL2 };
    for (i = 0; i < KPlanStall_N && i < 5; i++)
        r.AddStatic(iCityType[KStallType[i]], XD(KPlanStall[i * 3]), ZD(KPlanStall[i * 3 + 1]), KPlanStall[i * 3 + 2], 256, 0);
    // the smithy's outdoor forge, the Elder Ash, the shrine, the sky-forge
    r.AddStatic(iCityType[CT_FORGE], XD(PL_FORGE_X), ZD(PL_FORGE_Z), 256, 230, 0);
    r.AddStatic(iCityType[CT_TREE], XD(PL_TREE_X), ZD(PL_TREE_Z), 0, 256, 0);
    r.AddStatic(iCityType[CT_PLINTH], XD(PL_TALOS_X), ZD(PL_TALOS_Z), 0, 200, 0);
    r.AddStatic(iCityType[CT_STATUE], XD(PL_TALOS_X), ZD(PL_TALOS_Z), r3d_atan2((PL_TREE_X - PL_TALOS_X) * M, (PL_TREE_Z - PL_TALOS_Z) * M), 300, 0);
    r.AddStatic(iCityType[CT_FORGE], XD(PL_SKYFORGE_X), ZD(PL_SKYFORGE_Z), 640, 256, 0);
    // ---- water: the west canal, the channels beside the stairs, the pool and the moat round the piers
    for (i = 0; i < KPlanChan_N; i++)
        WaterLine(r, iCityType[CT_WATER], KPlanChanPt + KPlanChan[i * 3] * 2, KPlanChan[i * 3 + 1], KPlanChan[i * 3 + 2] ? 160 : 100);
    // the channels beside the wide stairs end in drains at the stairs' foot
    for (i = 0; i < KPlanDrain_N; i++)
        r.AddStatic(iCityType[CT_DRAIN], XD(KPlanDrain[i * 2]), ZD(KPlanDrain[i * 2 + 1]), 0, 256, 0);
    {
    int mnx = 1 << 30, mxx = -(1 << 30), mnz = 1 << 30, mxz = -(1 << 30);
    for (i = 0; i < KPlanPool_N; i++)
        {
        if (KPlanPool[i * 2] < mnx) mnx = KPlanPool[i * 2];
        if (KPlanPool[i * 2] > mxx) mxx = KPlanPool[i * 2];
        if (KPlanPool[i * 2 + 1] < mnz) mnz = KPlanPool[i * 2 + 1];
        if (KPlanPool[i * 2 + 1] > mxz) mxz = KPlanPool[i * 2 + 1];
        }
    int x, z;
    for (z = mnz + 22; z < mxz; z += 45)
        for (x = mnx + 17; x < mxx; x += 35)
            {
            if (!InPolyD(KPlanPool, KPlanPool_N, x, z)) continue;
            if (x > KPlanPlatform[0] - 10 && x < KPlanPlatform[2] + 10 && z < KPlanPlatform[1] + 10 && z > KPlanPlatform[3] - 10) continue;
            int idx = r.AddStatic(iCityType[CT_WATER], XD(x), ZD(z), 0, 256, 0);
            if (idx >= 0) r.iObj[idx].y = h0 + 86 * M / 10;
            }
    }
    // (v0.4.48: the game's own farm and windmill, meadery, east farm and watchtowers gone - Skyrim's own outskirts, from
    //  the area's file: Pelagia Farm, Honningbrew Meadery, Chillfurrow and Battle-Born Farms)
    // (v0.4.48: and its stables - the residence, the stable block, the horses, the carriage: Skyrim's own Stables)
    BuildApproach();
    }

// props that use city models: the drawbridge, its lever, crates
void Game::DrawCityProp(const Prop& p)
    {
    int ct = p.type == PT_BRIDGE ? CT_BRIDGE : (p.type == PT_CRATE ? CT_CRATE : CT_LEVER);
    const StaticDef& d = iR->iDefs[iCityType[ct] - ST_CUSTOM];
    iR->DrawMesh(iMCity[ct], p.anim < 0 ? 0 : (p.anim > 4 ? 4 : p.anim), p.x, p.y, p.z, p.yaw, 256, d.slots, 0);
    }

// ---------------------------------------------------------------- the approach to the gate (after Whiterun)
// The road comes up from the river road past the carriage and the stables (east of it) and the caravan (west of
// it), through the outer gate into the walled approach, north up its west side over the drawbridge across the
// stream, east along the top, south down the inner side through the inner gate and east into the city gate. The
// stream spills out of the city's west canal through the wall north of the gate, falls into the approach, runs
// west under the drawbridge, out through a water gate and down to a pond.

// level the ground along a polyline: points (x, z dm, height dm above h0)
static void GradeD(R3D& r, const int* P, int n, int h0)
    {
    int i, k;
    for (i = 0; i + 1 < n; i++)
        {
        const int* a = P + i * 3; const int* b = a + 3;
        int dx = b[0] - a[0], dz = b[1] - a[1];
        int steps = r3d_isqrt(dx * dx + dz * dz) / 10 + 1;
        for (k = 0; k <= steps; k++)
            {
            int x = XD(a[0]) + dx * M / 10 * k / steps, z = ZD(a[1]) + dz * M / 10 * k / steps;
            r.Flatten(x, z, 2 * M, h0 + (a[2] + (b[2] - a[2]) * k / steps) * M / 10, 2 * M);
            }
        }
    }

void Game::BuildApproach()
    {
    R3D& r = *iR;
    int h0 = r.GroundAt(XD(PL_WELL_X), ZD(PL_WELL_Z));
    int i, k;
    // the approach's ground (three parts at their heights), the road graded up through it
    for (i = 0; i < KPlanApLevel_N; i++)
        FlatPolyD(r, KPlanApLevelPt + KPlanApLevel[i * 3] * 2, KPlanApLevel[i * 3 + 1], h0 + KPlanApLevel[i * 3 + 2] * M / 10, 2);
    // (v0.4.48) the road outside the approach on Skyrim's own ground: its plan was for the old levelled plain, 7 m
    // above the ground by the stables and the carriage (gone with Skyrim's outskirts). The first two points keep to
    // the ground; from there it climbs evenly to where the approach's ground begins (point ROAD_IN).
    {
    enum { ROAD_IN = 5 };
    int P[KPlanRoad_N * 3], len[KPlanRoad_N], tot = 0;
    for (i = 0; i < KPlanRoad_N * 3; i++) P[i] = KPlanRoad[i];
    for (i = 0; i < 3; i++) P[i * 3 + 2] = (r.GroundAt(XD(P[i * 3]), ZD(P[i * 3 + 1])) - h0) * 10 / M;
    len[2] = 0;
    for (i = 3; i <= ROAD_IN; i++)
        {
        int dx = P[i * 3] - P[i * 3 - 3], dz = P[i * 3 + 1] - P[i * 3 - 2];
        tot += r3d_isqrt(dx * dx + dz * dz); len[i] = tot;
        }
    for (i = 3; i < ROAD_IN; i++) P[i * 3 + 2] = P[8] + (P[ROAD_IN * 3 + 2] - P[8]) * len[i] / (tot ? tot : 1);
    GradeD(r, P, KPlanRoad_N, h0);
    }
    for (i = 0; i + 1 < KPlanRoad_N; i++)
        r.PaintPath(XD(KPlanRoad[i * 3]), ZD(KPlanRoad[i * 3 + 1]), XD(KPlanRoad[i * 3 + 3]), ZD(KPlanRoad[i * 3 + 4]), 2 * M, i < 5 ? TM_DIRT : TM_COBBLE);
    int deckY = h0 - 45 * M / 10;
    // the stream bed: dug along the path, always falling
    enum { NS = KPlanStream_N, POND = 11 };
    int bed = 1 << 30, bedAt[NS];
    for (i = 0; i < NS; i++)
        {
        int x = XD(KPlanStream[i * 2]), z = ZD(KPlanStream[i * 2 + 1]);
        int want = r.GroundAt(x, z) - 17 * M / 10;
        if (i >= 5 && want > deckY - 2 * M) want = deckY - 2 * M;      // under the drawbridge and on
        if (want < bed) bed = want;
        bedAt[i] = bed;
        bed -= M / 8;
        }
    for (i = 0; i + 1 < NS; i++)
        {
        int ax = XD(KPlanStream[i * 2]), az = ZD(KPlanStream[i * 2 + 1]), bx = XD(KPlanStream[i * 2 + 2]), bz = ZD(KPlanStream[i * 2 + 3]);
        int dx = (bx - ax) >> 8, dz = (bz - az) >> 8;
        int len = r3d_isqrt(dx * dx + dz * dz);               // metres
        int steps = len / 2 + 1;
        for (k = 0; k <= steps; k++)
            {
            int x = ax + (bx - ax) * k / steps, z = az + (bz - az) * k / steps;
            r.Lower(x, z, 3 * M / 2, bedAt[i] + (bedAt[i + 1] - bedAt[i]) * k / steps, 2 * M);
            r.PaintDisc(x, z, 5 * M / 2, TM_SAND);
            }
        int yaw = r3d_atan2(bx - ax, bz - az);
        int np = len * 2 / 7 + 1;
        for (k = 0; k < np; k++)
            {
            int x = ax + (bx - ax) * (2 * k + 1) / (2 * np), z = az + (bz - az) * (2 * k + 1) / (2 * np);
            int idx = r.AddStatic(iCityType[CT_WATER], x, z, yaw, 256, 0);
            if (idx >= 0) r.iObj[idx].y = bedAt[i] + (bedAt[i + 1] - bedAt[i]) * (2 * k + 1) / (2 * np) + 3 * M / 4;
            }
        }
    {
    int px = XD(PL_POND_X), pz = ZD(PL_POND_Z);
    r.Lower(px, pz, 4 * M, bedAt[POND], 3 * M);
    r.PaintDisc(px, pz, 6 * M, TM_SAND);
    static const int KPond[4 * 2] = { -7, -9,  7, -9,  -7, 9,  7, 9 };
    for (i = 0; i < 4; i++)
        {
        int idx = r.AddStatic(iCityType[CT_WATER], px + KPond[i * 2] * M / 4, pz + KPond[i * 2 + 1] * M / 4, 0, 256, 0);
        if (idx >= 0) r.iObj[idx].y = bedAt[POND] + 3 * M / 4;
        }
    }
    // the drawbridge (a prop: its deck runs south from the hinge) and its lever; the banks made good again
    iBridgeX = XD(PL_BRIDGE_X); iBridgeZ = ZD(PL_BRIDGE_Z); iBridgeY = deckY;
    iLeverX = XD(PL_LEVER_X); iLeverZ = ZD(PL_LEVER_Z);
    r.Flatten(iBridgeX, iBridgeZ + 3 * M / 2, 2 * M, deckY, M);
    r.Flatten(iBridgeX, iBridgeZ - 23 * M / 2, 2 * M, deckY, M);
    // the culvert where the canal spills out of the city, the water gate in the approach's west wall
    r.AddStatic(iCityType[CT_CULVERT], XD(KPlanStream[0]), ZD(KPlanStream[1]), 256, 256, 0);
    r.AddStatic(iCityType[CT_CULVERT], XD(PL_WATERGATE_X), ZD(PL_WATERGATE_Z), 256, 256, 0);
    // the approach's walls, the outer gate and the inner gate
    for (i = 0; i < KPlanApWall_N; i++) WallRun(KPlanApWallPt + KPlanApWall[i * 2] * 2, KPlanApWall[i * 2 + 1], 200);
    {
    static const int KGate[2 * 2] = { PL_OUTERGATE_X, PL_OUTERGATE_Z,  PL_INNERGATE_X, PL_INNERGATE_Z };
    for (i = 0; i < 2; i++)
        {
        int x = XD(KGate[i * 2]), z = ZD(KGate[i * 2 + 1]);
        r.AddStatic(iCityType[CT_BASTION], x - 4 * M, z, 0, 170, 0);
        r.AddStatic(iCityType[CT_BASTION], x + 4 * M, z, 0, 170, 0);
        r.AddStatic(iCityType[CT_LINTEL], x, z, 0, 200, 0);
        }
    }
    // torches on posts: the city gate, the bridge, both approach gates (v0.4.48: not the old stables door)
    {
    static const int KTorch[8 * 2] = { PL_GATE_X - 30, PL_GATE_Z - 50,  PL_GATE_X - 30, PL_GATE_Z + 50,  PL_BRIDGE_X + 30, PL_BRIDGE_Z + 20,
                                       PL_OUTERGATE_X - 70, PL_OUTERGATE_Z - 40,  PL_OUTERGATE_X + 70, PL_OUTERGATE_Z - 40,
                                       PL_INNERGATE_X - 70, PL_INNERGATE_Z - 30,  PL_INNERGATE_X + 70, PL_INNERGATE_Z - 30,
                                       PL_STABLES_X - 60, PL_STABLES_Z + 30 };
    for (i = 0; i < 7; i++)
        {
        int x = XD(KTorch[i * 2]), z = ZD(KTorch[i * 2 + 1]);
        r.AddStatic(iCityType[CT_TORCHPOST], x, z, 0, 256, 0);
        r.AddStatic(iCityType[CT_FLAME], x, z, 0, 120, 0);
        AddLightSrc(0, x, r.GroundAt(x, z) + 5 * M / 2, z, 11 * M, 170);
        }
    }
    // the caravan west of the road: hide tents round a fire (two small, one wide), a tanning rack, the wagon,
    // crates (the bedrolls, knapsacks and crates are props, placed with the new game)
    int cx = XD(PL_CARAVAN_X), cz = ZD(PL_CARAVAN_Z);
    r.Flatten(cx, cz, 10 * M, r.GroundAt(cx, cz), 5 * M);
    r.PaintDisc(cx, cz, 10 * M, TM_DIRT);
    static const int KTent[3 * 3] = { -5, 4, 0,  -1, -7, 0,  8, -2, 1 };
    for (i = 0; i < 3; i++)
        {
        int x = cx + KTent[i * 3] * M, z = cz + KTent[i * 3 + 1] * M;
        int yaw = r3d_atan2(cx - x, cz - z) & 1023;
        r.AddStatic(iCityType[KTent[i * 3 + 2] ? CT_TENT2 : CT_TENT], x, z, yaw, 256, 0);
        iTentX[i] = x; iTentZ[i] = z; iTentYaw[i] = yaw; iTentBig[i] = KTent[i * 3 + 2];
        }
    r.AddStatic(ST_FIRE, cx, cz, 0, 200, 0);
    AddLightSrc(0, cx, r.GroundAt(cx, cz) + M, cz, 12 * M, 190);
    r.AddStatic(iCityType[CT_TANRACK], cx + 6 * M, cz + 6 * M, r3d_atan2(-6, -6), 256, 0);
    r.AddStatic(iCityType[CT_CART], cx + 3 * M, cz + 10 * M, 300, 256, 0);
    r.AddStatic(iCityType[CT_CRATES], cx - 8 * M, cz - 2 * M, 700, 256, 0);
    }

// ---------------------------------------------------------------- the local map
// how a static is drawn on the local map: its kind (1 building, 2 wall, 3 tower, 4 water, 5 stairs, 6 rock, 7 tree,
// 8 well, 9 small tree; 0 not at all), its half size across and along (world units, unscaled) and how far its
// middle lies along local +z (stairs rise from their foot)
int Game::MapShape(int aType, int& aHx, int& aHz, int& aOz)
    {
    aOz = 0;
    if (aType == ST_TREE) { aHx = aHz = 3 * M / 2; return 9; }
    if (aType == ST_ROCK) { aHx = aHz = 3 * M / 2; return 6; }
    if (aType < ST_CUSTOM) return 0;
    int ct;
    for (ct = 0; ct < CT_COUNT; ct++) if (iCityType[ct] == aType) break;
    const StaticDef& d = iR->iDefs[aType - ST_CUSTOM];
    switch (ct)
        {
        case CT_HOUSE: case CT_HALL: case CT_GREATHALL: case CT_TEMPLE: case CT_STABLE: case CT_FORGE: case CT_TENT: case CT_TENT2:
        case CT_STALL: case CT_STALL2: case CT_STALL3: case CT_CART:
            aHx = d.hx ? d.hx : 3 * M / 2; aHz = d.hz ? d.hz : M;
            if (ct == CT_TENT) { aHx = 115 * M / 100; aHz = M; }
            if (ct == CT_TENT2) { aHx = 12 * M / 5; aHz = 17 * M / 10; }
            if (ct == CT_STALL || ct == CT_STALL2 || ct == CT_STALL3) { aHx = 7 * M / 4; aHz = 13 * M / 10; }
            return 1;
        case CT_WINDMILL: case CT_WATCH: aHx = aHz = 3 * M; return 1;
        case CT_WALL: aHx = 5 * M; aHz = 3 * M / 4; return 2;
        case CT_LINTEL: aHx = 9 * M / 2; aHz = M; return 2;
        case CT_INNERGATE: aHx = 19 * M / 2; aHz = M / 2; return 2;
        case CT_FENCE: aHx = 2 * M; aHz = M / 5; return 2;
        case CT_BASTION: aHx = aHz = 37 * M / 10; return 3;
        case CT_PIER: aHx = aHz = 5 * M / 2; return 3;
        case CT_WATER: aHx = 7 * M / 4; aHz = 9 * M / 4; return 4;
        case CT_DRAIN: aHx = 11 * M / 10; aHz = M; return 3;
        case CT_STAIRS: case CT_STAIRS2: case CT_STAIRSW: case CT_STAIRS2W: case CT_STAIRS24: case CT_STAIRS16:
            {
            int n = ct == CT_STAIRS2 || ct == CT_STAIRS2W ? 40 : (ct == CT_STAIRS24 ? 24 : (ct == CT_STAIRS16 ? 16 : 32));
            aHx = ct == CT_STAIRSW || ct == CT_STAIRS2W ? 3 * M : (ct == CT_STAIRS24 ? 2 * M : (ct == CT_STAIRS16 ? 5 * M / 2 : 5 * M / 4));
            aHz = n * 11 * M / 100; aOz = aHz;
            return 5;
            }
        case CT_WOODSTAIR: aHx = 7 * M / 10; aHz = 4 * M; aOz = -4 * M; return 5;
        case CT_CLIFF: aHx = aHz = 5 * M; return 6;
        case CT_TREE: aHx = aHz = 9 * M / 2; return 7;
        case CT_WELL: aHx = aHz = 6 * M / 5; return 8;
        }
    return 0;
    }

// the city's door model, for other places (Riverwood)
int GameCityDoorType(const Game& g) { return g.iCityType[CT_DOOR]; }
