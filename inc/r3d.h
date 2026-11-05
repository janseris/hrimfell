// r3d.h - fixed-point software 3D engine for the Nokia 9300 (S80 v2, ARM926, no FPU)
// Portable C++ (GCC 2.95 compatible), no floats, no writable static data (EKA1 DLL rule),
// no C library: the platform shell supplies memory and timing.
#ifndef R3D_H
#define R3D_H

typedef unsigned short u16;
typedef unsigned char u8;
typedef unsigned int u32;

enum { OUT_W = 640, OUT_H = 200 };

enum
    {
    METER = 256,                // world units per metre
    CELL = 4 * METER,           // 4 m terrain cells
    // the largest world the engine is built for: Skyrim's Tamriel (119 x 94 cells of 58.5 m, ~7 x 5.5 km) at
    // 4 m; the actual size (iGX x iGZ cells) is set by the game when it builds or loads its world
    MAXGX = 1744, MAXGZ = 1376,         // Skyrim's land at 4 m a cell (tools/make_land.py)
    CHUNK = 8,                  // terrain chunk: 8 x 8 cells (32 m)
    MAXNCX = MAXGX / CHUNK, MAXNCZ = MAXGZ / CHUNK,
    HS = 8,                     // stored height * HS = world height (+-1024 m, 3 cm steps)
    TCACHE = 768,               // chunks kept lit and treed (enough for the farthest detailed view)
    // the land is kept as a window of full detail round the player (WIN x WIN vertices, 1 km, loaded from the land
    // file in tiles of LTILE x LTILE as the player moves) and the whole of it coarse (every CSTEP-th vertex, 16 m:
    // the far land, the map, whatever is far away). Built with R3D_FULLLAND (tools: baking the land file) the
    // window is the whole land.
#ifdef R3D_FULLLAND
    WINW = MAXGX + 1, WINH = MAXGZ + 1,
#else
    WINW = 256, WINH = 256,
#endif
    LTILE = 32, CSTEP = 4,
    MAXLTX = (MAXGX + LTILE) / LTILE + 1, MAXLTZ = (MAXGZ + LTILE) / LTILE + 1,
    MAXCW = MAXGX / CSTEP + 1, MAXCH = MAXGZ / CSTEP + 1,
    TMAXTREE = 24               // trees per chunk at most
    };
// the material byte of a terrain vertex: the ground (TM_*) in the low bits, and flags
enum { VM_MAT = 15, VM_PAINT = 16, VM_NOTREE = 32, VM_TREE = 64, VM_ROCK = 128 };     // (TREE / ROCK: where Skyrim has one)

enum { MODE_GOURAUD = 0, MODE_TEX = 1 };
enum { MAXMV = 2048 };     // max vertices per mesh (the items' own models in the inventory: up to 2000)

// static world objects
enum { ST_TREE = 0, ST_HOUSE, ST_ROCK, ST_STONE, ST_FIRE, ST_COUNT, ST_CUSTOM = 8, MAXDEFS = 1800 };  // (v0.4.57: 1800 - Skyrim's Whiterun, 1354 used)   // (v0.4.48: 1400, was 1000 - Whiterun's outskirts)
// terrain materials (colour table in r3d.cpp)
enum { TM_GRASS = 0, TM_FOREST, TM_ROCK, TM_SNOW, TM_SAND, TM_DIRT, TM_BARROW, TM_COBBLE, TM_FIELD,
       TM_TUNDRA, TM_FALL, TM_REACH, TM_MARSH, TM_ASH, TM_ROAD, TM_COUNT };      // (Skyrim's regions: Whiterun, the Rift, the Reach, Hjaalmarch, Eastmarch; its paved roads)
// DrawMesh flags
enum { MF_UNLIT = 1, MF_FLASH = 2, MF_NOFOG = 4 };

struct TV
    {
    int x, y, z;        // camera space (1 m = 256)
    int sx, sy;         // screen 28.4
    int w;              // 1/z, 0..65535
    int a[3];           // r,g,b (8.8) or u,v (8.8)
    int code;           // frustum outcode
    };

struct Obj
    {
    int x, y, z;
    short yaw, scale;   // scale 256 = 1
    u16 type;           // more than 248 kinds of statics since the interiors built from Skyrim's pieces
    u8 var, flag, pad;
    int next;
    };

struct Mesh
    {
    int nv, nf, nfr, vstride, fstride;
    short* v;           // nfr * vstride * 3
    u16* f;             // nf * 4: a, b, c, slot
    short* n;           // nfr * fstride * 3 face normals (Q14)
    u16* uv;            // nf * 6 texel coordinates
    int radius;         // bounding radius (world units, frame 0)
    short* vn;          // optional per-frame vertex normals (smooth shading), nfr * vstride * 3
    int cx, cy, cz, brad;   // bounding sphere (frame 0)
    int oneN;               // face normals stored for frame 0 only (smooth meshes)
    int dropped;            // faces that did not fit (should stay 0)
    int uvsh;               // (v0.4.46) its texel coordinates' fraction bits: 2 (quarter texels: Skyrim's models), 0 (the engine's)
    short* cl; int ncl;     // (v0.4.43) clusters of CLS faces: CLW shorts each (centre, radius, normal cone, vertices), or 0
    int lazy;               // (v0.4.43) its faces' normals agree with their windings: DrawMesh looks at the faces first
                            // (turned away: in the model's own space) and works out only the vertices the rest use
    };
enum { CLS = 16, CLSH = 4, CLW = 12, MAXCL = 160, MAXLF = CLS * MAXCL, LAZYMIN = 48 };

// game-defined static object type (buildings, walls, ...)
#ifdef R3D_STATS
struct R3DStat { const struct Mesh* m; int tris, faces, calls, pix; };
extern R3DStat g_r3dStat[256]; extern int g_r3dStatN, g_r3dPixFlat, g_r3dStage[12], g_r3dRows[3];
#endif
struct StaticDef
    {
    const Mesh* mesh;       // near model
    const Mesh* lod;        // far model (or 0)
    const Mesh* mid;        // (v0.4.46) a middle one between them (or 0): past midDist, until lodDist
    u8 slots[32];           // up to 8 colour slots (r, g, b, tex)
    short hx, hz;           // collision half extents in local x/z (0 = none)
    short bx, bz;           // the box's middle in local x/z (models whose origin is at a corner)
    short rad;              // circular collision radius (0 = none)
    short drawDist, lodDist;// metres
    short midDist;          // metres (v0.4.46)
    u8 lodCat;              // (v0.4.46) whose distances: 0 its own (lodDist, midDist), else R3D::iLodFar[lodCat] (LODC_...)
    short yOff;             // vertical offset (world units)
    short hy;               // collision height above the object's y (interiors; 0 = unlimited)
    short animMs;           // SD_ANIM: milliseconds per frame (0 = 100)
    u8 flags;               // SD_ANIM: cycle frames
    u8 frame;               // fixed frame (when not animated)
    short pivZ;             // SD_SPINZ: the hub's local z (it turns about the local z axis at yOff)
    short spinMs;           // SD_SPINY / SD_SPINZ: milliseconds per full turn
    const int* parts;       // collision parts (see R3D::Blocked), 0 = use hx/hz or rad
    // (v0.4.74) a box for it from afar (a few pixels: a house of a town across the plains): its far model's bounds and
    // colours - the roof's, the sides' (BoxInfo); boxState 0 not looked at yet, 1 has one, 2 none
    short bb[6]; u8 boxCol[6]; u8 boxState;
    int nparts;
    };
enum { LODC_OWN = 0, LODC_BUILDING, LODC_TREE, LODC_FURN, LODC_ROCK };
enum { SD_ANIM = 1, SD_UNLIT = 2, SD_WALLED = 4, SD_SPINY = 8, SD_SPINZ = 16 };
// collision parts of a static, 7 ints each, world units at scale 256 in the object's frame:
// PART_BOX x0, z0, x1, z1, y0, y1 / PART_RING r_in, r_out, -, -, y0, y1 (solid for anything wider than the hole)
enum { PART_BOX = 0, PART_RING = 1 };   // SD_WALLED: hidden by the city wall when the camera is outside it
enum { SL_TEX = 0x80 };     // slot tex byte flag: draw this slot's faces textured (TX index in low bits)
enum { SL_SKY = 0xC0 };     // (the two top bits) one of Skyrim's own textures instead: its number in the low 6 bits
// texture ids for SL_TEX slots
enum { TXI_GRASS = 0, TXI_ROCK, TXI_SNOW, TXI_SAND, TXI_FUR, TXI_NEEDLE, TXI_FURN, TXI_BLOCK, TXI_DIRT, TXI_LOGS,
       TXI_THATCH, TXI_NORDIC, TXI_PLANKS, TXI_SHINGLE, TXI_FIELD, TXI_SLATE, TXI_DSTAVE, TXI_ROPE };
enum { MAXIOBJ = 5600 };   // (v0.4.68: 5600 - Forelhost's three halls, 4189 pieces)   // (v0.4.55: 3600 - Skyrim's barrow: its two halls about 3000 with their flames; v0.4.48: 1400 - all the interiors from Skyrim's pieces share it; Whiterun's four took it to 2218)

// triangle-half state shared with the ARM assembly walkers (layout fixed, see spans.s)
struct FlatCtx;
struct HalfState
    {
    int lx, ldx, rx, rdx;
    int a0, da0, a1, da1, a2, da2, a3, da3;
    int dws, d1s, d2s, d3s;
    int dw, d1, d2, d3;
    u16* crow; u16* zrow; int count; int W;
    union { int col; const u8* tex; } x;
    const u16* pal;
    int cstride;
    int xlo, xhi;           // (v0.4.43) the rows clipped to these columns (0, W; less where the occlusion map hides the rest)
    };
// (v0.4.43) what tri_flat1 (spans.s) needs of the renderer
// (v0.4.71) what the perspective row walkers (spans.s persp_tex*) need: the row's run, its premultiplied u, v and w at
// its first pixel and their steps, how long a run may be
struct PerspCtx { u16* cp; u16* zp; int n, wS, dw, u0, v0, D1, D2, seg; const int* rc; const u8* tex; const u16* pal; int k, uA, vA; };
// (v0.4.73) what tex_tri (spans.s) works on and gives: a textured triangle's set-up, both its halves
struct TexTri
    {
    TV* v[3]; const int* rc; int H;                  // 0
    int persp, wmax;                                 // 20
    int W, cstride; const u16* pal; const u8* tex; u16* cb; u16* zb; int cstrideE;     // 28
    int pa0, pb0, pa1, pb1;                          // 56
    HalfState hs[2];                                 // 72 (116 bytes each)
    int scratch[28];                                 // 304
    };
// (v0.4.73) what face_shade (spans.s) needs of DrawFaces and the renderer, and where it puts its records
struct FaceCtx
    {
    const u16* F; const short* N; const u16* UV; TV* tv; const u16* FL; int nl, j;       // 0
    const u8* slots; const u16* si; int lx, ly, lz, flags, light, amb, pl, mla;          // 28
    const int* ll; int ln, us, texdist, notex, texmip, dbl, fog0, fogK, fogSq, fogR, fogG, fogB;     // 68
    const u16* libShade; const u16* skyShade; const u16* shadePal;                       // 120
    const u8* libHi2; const u8* skyHi2; const u8* libHi; const u8* skyHi; const u8* libTex1; const u8* libTex;
    const u8* skyTex32; const u8* skyTex16; const u8* skyTex1; const u8* skyTex; const u8* tex;     // 132
    int* out; int* outEnd;                                                                 // 176
    int lr, lg, lb;                                                                        // 184 (v0.4.74: the light's colour)
    };
struct FlatCtx { const int* rc; int H; u16* cb; int cstride; u16* zb; int W; int occ; int col; int tris; HalfState* hs; };

class R3D
    {
public:
    static int MemNeeded();
    void Init(u8* aMem);

    // ---- world building (call FinishWorld() when done) ----
    void SetWorldSize(int aGX, int aGZ);            // cells (multiples of CHUNK, at most MAXGX x MAXGZ)
    int iGX, iGZ, iGVX, iNCX, iNCZ;                 // the world: cells, vertices per row, chunks
    int WorldW() const { return iGX * CELL; }
    int WorldD() const { return iGZ * CELL; }
    void GenTerrain(int aSeed);
    void Flatten(int aX, int aZ, int aR, int aH, int aEdge);    // world units
    void Lower(int aX, int aZ, int aR, int aH, int aEdge);      // dig down only
    void AddHeight(int aX, int aZ, int aR, int aH);
    void PaintPath(int aX0, int aZ0, int aX1, int aZ1, int aHalfW, int aMat);
    void PaintDisc(int aX, int aZ, int aR, int aMat);
    int  AddStatic(int aType, int aX, int aZ, int aYaw, int aScale, int aVar);
    void MeshTrim(Mesh& m);                         // give back a just-built mesh's unused room
    int  RegisterStatic(const StaticDef& aDef);     // returns a type id >= ST_CUSTOM
    int iNumDefs;
    void ScatterTrees(int aSeed);
    void FinishWorld();
    void RelinkStatics();   // (v0.4.57) the statics into their chunks again (after some were taken off and others put)
    int  GroundAt(int aX, int aZ);
    int  Blocked(int aX, int aZ, int aR, int aY0 = -(1 << 30), int aY1 = 1 << 30);   // aY0..aY1: the mover's height span      // 1 if a static object overlaps the circle
    int  MaterialAt(int aX, int aZ);

    // ---- output ----
    // 0 = 320x200, 1 = 400x200, 2 = 640x200 (1:1, centred), 3 = 320 rendered doubled to 640
    void SetOutputMode(int aMode);
    // (v0.4.50) half rows (a setting): the world drawn on 100 rows, each shown twice (EndWorld)
    int iHalfRows, iHalfOn;
    int iFlatLand;          // (v0.4.50, a setting) the near land in flat colours, not smooth
    int iClutterNear;       // (v0.4.50, a setting) the small things outside only within CLUTTER_M
    void EndWorld();
    int  OutX0() const { return iNative ? (OUT_W - iW) / 2 : 0; }
    int  OutW() const { return iNative ? iW : OUT_W; }

    // ---- frame ----
    void SetCamera(int aX, int aY, int aZ, int aYaw, int aPitch);
    // light 0..256, fog/horizon colour, zenith colour, star brightness 0..255
    void SetAtmosphere(int aLight, int aFogR, int aFogG, int aFogB, int aZenR, int aZenG, int aZenB, int aStars);
    void BeginFrame(u16* aOut, int aStride);       // sky, terrain, static objects
    void DrawMesh(const Mesh& m, int aFrame, int aX, int aY, int aZ, int aYaw, int aScale, const u8* aSlots, int aFlags);
    void DrawMeshRot(const Mesh& m, int aFrame, int aX, int aY, int aZ, int aYaw, int aPitch, int aScale, const u8* aSlots, int aFlags);
    void DrawMeshMat(const Mesh& m, int aFrame, int aX, int aY, int aZ, const int* A, int aScale, const u8* aSlots, int aFlags);
    void DrawMeshRoll(const Mesh& m, int aFrame, int aX, int aY, int aZ, int aYaw, int aRoll, int aScale, const u8* aSlots, int aFlags);
    // mesh in camera space (first-person weapon): rotation by yaw/pitch/roll, translation in camera units
    void DrawMeshView(const Mesh& m, int aFrame, int aX, int aY, int aZ, int aYaw, int aPitch, int aRoll, const u8* aSlots, int aFlags);
    int  ProjectWorld(int aX, int aY, int aZ, int& aSx, int& aSy);   // output pixel coords; 0 if behind
    void ClearZ();
    // fog start/end in metres (weather); also limits the view distance
    void SetFog(int aStartM, int aEndM);
    int iFog0, iFogK, iVis, iVis2, iPalFogR, iPalFogG, iPalFogB;
    // view distances (settings): the detailed land and its trees and buildings reach iNearM metres; beyond it,
    // when iFarM > iNearM, the far land (a coarse sheet of the heights) carries on to iFarM, hazing into the sky
    void SetViewDistance(int aNearM, int aFarM);
    int iFogSq;             // the fog grows with the distance squared (far land on)
    // the fog's share at view depth aZ, 0..256
    inline int FogF(int aZ) const
        {
        int f = ((aZ - iFog0) * iFogK) >> 16;
        if (f <= 0) return 0;
        if (f >= 256) return 256;
        return iFogSq ? f * f >> 8 : f;
        }
    int iNearM, iFarM, iObjD, iMaxD;
    int iFogEndM;           // the fog's end the weather asks for (SetFog), before the view distances
    int iFog0M;
    int iFarOn, iFarVis;    // the far land drawn this frame, to this many metres
    void ApplyView();
    // ---- interior cells: separate object lists drawn without terrain or sky ----
    int  AddInterior(int aCell, int aType, int aX, int aY, int aZ, int aYaw, int aScale, int aVar);
    int  BlockedIn(int aX, int aZ, int aR, int aFeetY);
    int  TopAt(int aX, int aZ, int aYMax);     // highest static top (StaticDef::hy) under a point
    void DrawInterior();
    Obj* iIObj; int iINum;
    int  iCell;             // 0 = outdoors
    u32  iZoneMask;         // interiors: which rooms (Obj::pad, 1..31) are drawn this frame; 0 always is
    const u8* iPvsBits;     // (v0.4.55) interiors: of the pieces iPvsFirst.. (iPvsN), which can be seen from here (0: all)
    int  iPvsFirst, iPvsN;
    const Mesh* iFallTree[2]; u8 iFallSl[8];   // (v0.4.66) the land's grown trees in the Rift: Skyrim's aspen (near, far; 0: the game's own)
    const u8* iOutPvsBits;  // (v0.4.65) outside: of the statics iOutPvsFirst.. (iOutPvsN), which can be seen from here (0: all)
    int  iOutPvsFirst, iOutPvsN;
    int  iAnimMs;           // game clock for animated statics (set by the game each frame)
    int  iWallX, iWallZ, iWallR, iWallTop;   // city wall occluder: centre, radius, top height (world units)
    int  iOccN, iOcc[4 * 4];   // wall lines along x hiding what stands behind them (on +z): x0, x1, z, top

    // ---- 2D on the output (output pixel coordinates) ----
    void Text(int aX, int aY, const char* aS, u16 aCol);
    void TextShadow(int aX, int aY, const char* aS, u16 aCol);
    int  TextW(const char* aS);
    void Shade(int aX, int aY, int aW, int aH);         // darken
    void Rect(int aX, int aY, int aW, int aH, u16 aCol);
    void Tint(int aX, int aY, int aW, int aH, u16 aCol); // 50% blend
    static u16 Rgb(int r, int g, int b);

    // ---- mesh building ----
    u8*  Alloc(int aBytes);
    void MeshInit(Mesh& m, int aMaxV, int aMaxF, int aFrames);
    void MeshFromData(Mesh& m, const short* aV, int aNv, const u16* aF, int aNf);   // a ready model: exact size, planar texels
    void MeshFromConst(Mesh& m, const short* aV, int aNv, const u16* aF, int aNf, const short* aN, const u16* aUV);   // a ready model drawn from the program's own data (no memory used)
    // box hanging below pivot (px,py,pz), swung around local x by aSwing, then around local z by aRoll
    void MeshBox(Mesh& m, int aFrame, int aSlot, int aPx, int aPy, int aPz, int aSwing, int aHx, int aHy, int aHz);
    void MeshBoxR(Mesh& m, int aFrame, int aSlot, int aPx, int aPy, int aPz, int aSwing, int aRoll, int aHx, int aHy, int aHz);
    void MeshCone(Mesh& m, int aFrame, int aSlot, int aR0, int aY0, int aY1, int aSides, int aRot);
    void MeshConeAt(Mesh& m, int aFrame, int aSlot, int aCx, int aCz, int aR0, int aY0, int aY1, int aSides, int aRot);
    void MeshRoof(Mesh& m, int aFrame, int aSlot, int aHx, int aHz, int aY0, int aY1, int aOver);
    // generic quad / triangle from explicit points (frame 0 adds faces); points are object space
    void MeshQuad(Mesh& m, int aFrame, int aSlot, const int* aP, int aCx, int aCy, int aCz);
    void MeshTri(Mesh& m, int aFrame, int aSlot, const int* aP, int aCx, int aCy, int aCz);
    // tapered elliptic cylinder from p0 to p1; radii along two axes at each end; caps: 1 = start, 2 = end
    void MeshLimb(Mesh& m, int aFrame, int aSlot, int x0, int y0, int z0, int x1, int y1, int z1,
                  int r0a, int r0b, int r1a, int r1b, int aSides, int aCaps);
    void MeshEllipsoid(Mesh& m, int aFrame, int aSlot, int cx, int cy, int cz, int rx, int ry, int rz, int aSides, int aRings);
    void MeshFinish(Mesh& m);
    void MeshSmooth(Mesh& m);
    void MeshFlame(Mesh& m, int aFrame, int aR, int aH, int aOuter, int aInner, int aCore, int aY0);
    void MeshPlanarUV(Mesh& m, int aShift = 3);
    void MeshInitSmooth(Mesh& m, int aMaxV, int aMaxF, int aFrames);
    // loft: aRings * 12 ints {cx,cy,cz, ax,ay,az, bx,by,bz, ra,rb, slot}; caps 1 = first ring, 2 = last
    void MeshRings(Mesh& m, int aFrame, const int* aR, int aRings, int aSides, int aCaps);       // after MeshFinish: per-vertex normals -> Gouraud lighting
    int  iMeshCursor;
    int  iSkip;             // tests: parts left out (1 far land, 2 land, 4 objects, 8 people, 16 props, 32 plants, 64 sky, 512 clusters)
    int  iMeshDropped;      // parts that did not fit their mesh (should stay 0)
    int  iPivX, iPivY, iPivZ;
    int  iViewScale;        // DrawMeshView model scale (256 = 1), for previews of small things    // DrawMeshView rotates around this object-space point
    int* iMeshLev;
    u8 iClVis[MAXCL];       // (v0.4.43) DrawMesh: the clusters seen this time (iClOn: DrawFaces skips the rest)
    int iClOn, iStampN;
    // (v0.4.46) the plainer models' distances (metres; the menu's Full models / Middle models): the buildings' full
    // model to iLodNear (-1: never - the middle one even close), their middle one to iLodFar[LODC_BUILDING]; the
    // others' full one to iLodFar[their kind] (-1: never)
    int iLodNear, iLodFar[5];
    const u8* iSkyHi2; int iSkyHi2N;   // (v0.4.46) the first iSkyHi2N of Skyrim's pictures at 256 x 256
    int iClSkipped;         // (stats) clusters left out
    void MeshClusters(Mesh& m);
    int ClusterCull(const Mesh& m, const int* K, const TV& t, int aScale, int cs, int sn);
    int LazyFaces(const Mesh& m, const int* K, const TV& t, int aScale, int cs, int sn);
    HalfState iFlatHS;      // (v0.4.43) RasterFlat's walker state
    FlatCtx iFlatCtx;
    u32* iSortKey; int* iSortCnt;   // (v0.4.70) the statics of a pass, nearest first: (distance m << 20 | index), counts
    u16* iFaceList; int iFaceN;   // (v0.4.43) LazyFaces: the faces to draw (MAXLF)
    u16* iLazyList;         // and the vertices they use (MAXMV)
    int LazyCull(const Mesh& m, int ex, int ey, int ez, int sh, int margin);
    int LazyList(const Mesh& m, int st);
    void LazyXform(const Mesh& m, int aN, const int* K, const TV& t);

    // ---- state (public for simplicity) ----
    int iW, iH;
    u16* iZB;
    u16* iCB;
    u16* iOut; int iOutStride;
    int iCStride, iDouble, iNative, iBorderFrames, iOutMode;
    int iFx, iFy, iFx16, iFy16, iCx16, iCy16, iHW, iHH, iLenX, iLenY;

    int iCamX, iCamY, iCamZ, iYaw, iPitch;
    int iM[9];
    int iSunX, iSunY, iSunZ;
    // the sun in the sky (direction towards it, how visible it is) and the cloud layer
    int iSunVis, iSunDayLev;        // 0..256: sun disc visibility; daylight factor baked into textured terrain
    int iCloud, iCloudR, iCloudG, iCloudB;      // cover 0..256 and colour
    int iBakedSun;                  // the sun as it is now (packed): a cached chunk lit for another is re-lit
    void SetSun(int aDx, int aDy, int aDz, int aDayLev);
    void BakeTerrainLight();
    void DrawSunAndClouds(int yh);
    int iFogR, iFogG, iFogB, iZenR, iZenG, iZenB, iLight, iStars;
    // point lights (fires, candles, torches): world position, reach, strength (light levels /256)
    enum { MAXLIGHTS = 6 };
    int iNumLights;
    int iLW[MAXLIGHTS * 5];         // x, y, z, radius, strength
    int iLC[MAXLIGHTS * 3];         // the same positions in camera space (per frame)
    int iLInv[MAXLIGHTS];           // (256 << 16) / (radius/16)^2
    int iLightMask;                 // the lights near what is being drawn (LightsNear)
    int iLightList[MAXLIGHTS * 8];  // (v0.4.73) those lights in a compact list (SetLightList): x, y, z, R, 2R, (R/16)^2, inv, strength
    int iLightListN, iLightListMask, iLightListFrame;
    int iLightR, iLightG, iLightB;  // (v0.4.74) the light's colour (/256): white outside, firelight's warmth indoors
    void SetLightColour(int aR, int aG, int aB);
    void SetLightList();
    int iMeshLightAdd;              // a small mesh's light from the lights, the same for all its faces (-1: per face)
    int iAmbient;                   // added to every face's light level (indoors)
    int iGamma;                     // 0 = off, 1..3 brighter
    u16* iGammaLut;
    void ClearLights() { iNumLights = 0; }
    void AddLight(int aX, int aY, int aZ, int aRadius, int aStrength);     // world position; before BeginFrame
    void PrepareLights();
    int  PointLight(int cx, int cy, int cz);           // camera-space point -> extra light level
    int  LightsNear(int cx, int cy, int cz, int rad);
    int  iMeshNearLight;
    void SetGamma(int aG);
    void ApplyGamma(u16* aOut, int aStride);

    // per vertex height / HS and ground (VM_* bits), for the vertices in the window: iHeight[z * iHS + x] (the
    // pointers are offset so that the world's own vertex numbers index them; outside the window: Hgt / MatV)
    short* iHeight;
    u8* iMat;
    int iHS;                            // the window's row length
    int iWX0, iWZ0, iWW, iWH;           // the window: its first vertex, its size
    short* iWinH; u8* iWinM;            // the window's storage
    short* iCH; u8* iCM; int iCW, iCHt; // the coarse land: every CSTEP-th vertex, iCW per row, iCHt rows
    int InWin(int x, int z) const { return (unsigned)(x - iWX0) < (unsigned)iWW && (unsigned)(z - iWZ0) < (unsigned)iWH; }
    int CIdx(int x, int z) const
        {
        int cx = (x + CSTEP / 2) / CSTEP, cz = (z + CSTEP / 2) / CSTEP;
        if (cx < 0) cx = 0; if (cz < 0) cz = 0; if (cx >= iCW) cx = iCW - 1; if (cz >= iCHt) cz = iCHt - 1;
        return cz * iCW + cx;
        }
    int Hgt(int x, int z) const { return InWin(x, z) ? iHeight[z * iHS + x] : iCH[CIdx(x, z)]; }
    int MatV(int x, int z) const { return InWin(x, z) ? iMat[z * iHS + x] : iCM[CIdx(x, z)]; }
    int iLandBaked;                     // the land file has the world's own changes in it (roads, the city's hill ...)
    void FocusLand(int aX, int aZ);     // the window round a point (world units): loads what is not there
    int UpdateLandWindow(int aX, int aZ);   // as the player moves: 1 when tiles were loaded
    void FlatLand();                    // no land file: level ground
    // after tiles are loaded (vertex rect, inclusive): the game re-settles what stands there
    typedef void (*TLandHook)(void* aCtx, int aX0, int aZ0, int aX1, int aZ1);
    TLandHook iLandHook; void* iLandHookCtx;
    int iNTX, iNTZ; u32* iTileOff;      // the land file's tiles: their places in the file
    int iLandLoads;                     // tiles read so far (tests)
    int* iChunkMinH; int* iChunkMaxH;
    int* iChunkObj;
    short* iChunkSlot;  // the chunk's slot in the cache, or -1
    short* iChunkWater; // the chunk's water level / HS, or NOWATER
    enum { NOWATER = -32768 };
    int WaterAt(int aX, int aZ) const;                // the water level at a point (world units), very low if none
    int WaterOfChunk(int aCi) const { return iChunkWater[aCi] == NOWATER ? -(1 << 28) : iChunkWater[aCi] * HS; }
    // Skyrim's land from tools/make_land.py, read through aRead (returns the bytes it gave, 0 at the end);
    // returns 0 if the data is not a land file (the world is then left as it was)
    typedef int (*TLandRead)(void* aCtx, u8* aBuf, int aLen);
    typedef int (*TLandSeek)(void* aCtx, int aPos);     // to a byte of the file; 0 if it cannot
    int LoadLand(TLandRead aRead, TLandSeek aSeek, void* aCtx);     // (the file is read from again as the player moves)
    void BeginOverlay(u16* aOut, int aStride);      // a frame drawn on what is already there (menus over the world)
    TLandSeek iLandSeek;
    void LoadTiles(int aTX0, int aTZ0, int aTX1, int aTZ1);         // tiles (inclusive) into the window
    void LandSettled(int aX0, int aZ0, int aX1, int aZ1);           // after loading: chunks' heights, objects, the game
    int iLandLoaded;
    int iLandOffset;    // engine height - Skyrim height (world units)
    int iLandPos, iLandLen; TLandRead iLandRead; void* iLandCtx;
    int LandByte();
    Obj* iObj; int iObjCount;
    // the chunk cache: the land near the camera with its light baked, and its trees
    struct TChunk
        {
        int chunk;                      // chunk index, -1 free
        int key;                        // the sun it was lit for
        int used;                       // frame it was last drawn or touched
        int ntree;
        u8 lit[81 * 3];                 // vertex colours (Gouraud)
        u8 lev[81];                     // light levels (textured)
        Obj tree[TMAXTREE];
        };
    TChunk* iTC;
    int iTCNext, iRelit;
    TChunk* Cached(int aCx, int aCz, int aLight);  // the chunk's cache entry (filled / re-lit as needed)
    void FillChunk(TChunk& t, int aCx, int aCz, int aTrees);
    void VertexColour(int aGx, int aGz, int& aR, int& aG, int& aB, int& aLev);
    int  LodOf(int aCx, int aCz);
    void DrawFarLand();

    u8* iTex; u16* iBasePal; u16* iShadePal;
    TV* iVerts;
    TV* iFarV;          // a row pair of the far land
    u8* iFarCol;        // the far land's lit colours, a vertex every 16 cells (baked with the sun)
    int iFarColKey, iFarColRow, iFarColNext;     // the far land's lighting: for which sun; re-lit a few rows a frame (the row, the sun)
    int iTris, iPixels, iMode;
    int iNoTex;                     // settings: textured models drawn flat in their colours (faster)
    int iTexDist;                   // settings: how far textured faces are drawn so (units; flat beyond)
    const u16* iCurPal; const u8* iCurTex;
    // Skyrim's textures (the area's file): n pictures of 64 x 64 on one palette (256 r, g, b), and that palette
    // lit and fogged as iShadePal is (16 light levels x 8 fog steps)
    // (iSkyTex1: the same blurred, for faces seen at under a pixel a texel; 0: none)
    // what is hidden: the z buffer's farthest (lowest w) in each 8 x 8 block of what has been drawn (OccBuild), and
    // whether DrawMesh skips a mesh whose bounding sphere is behind it (on from OccBuild to the next ClearZ)
    enum { GUARD = 640 };
#ifndef PERSP_M
#define PERSP_M 20
#endif
#ifndef PERSP_SH
#define PERSP_SH 2
#endif
#ifndef PERSP_LIM
#define PERSP_LIM 2
#endif
#ifndef PERSP_SEG
#define PERSP_SEG 32
#endif
#ifndef LANDFLAT
#define LANDFLAT 6
#endif      // pixels round the screen within which triangles are not cut (Outcode)
    enum { OCCB = 8, OCCW = 640 / 8, OCCH = 200 / 8 };
    int iOccOn, iOccSkipped;
    void OccBuild();
    void LandTri(TV* a, TV* b, TV* c);
    int  OccHiddenSphere(const TV& aT, int aRad);
    int  OccHiddenTri(const TV* a, const TV* b, const TV* c);
    int  iOccTris;          // triangles left out as hidden (tests)
    const u8* iSkyTex; const u8* iSkyTex1; const u8* iSkyRgb; int iSkyN; u16* iSkyShade;
    // (and the first iSkyHiN of them at 128 x 128, for faces seen close; iTexQ: the face now drawn uses one)
    const u8* iSkyHi; int iSkyHiN, iTexQ;      // (iTexQ 2: a cut-out picture, texel 0 not drawn)
    const u8* iSkyKey;                          // per picture: 1 cut out (its texel 0 the key), or 0 for none
    // (v0.4.42 test) the pictures at 32 x 32 and 16 x 16, each tiled to fill 64 x 64 (the spans' mask unchanged, the
    // coordinates halved / quartered); iTexMip 0: the blurred 64 for faces seen small, 1: the 32, 2: the 32 and the 16
    const u8* iSkyTex32; const u8* iSkyTex16; int iTexMip;
    void SetSkyTextures(const u8* aRgb, const u8* aTex, const u8* aTex1, int aN, const u8* aHi = 0, int aHiN = 0);
    void BuildSkyShade();
    // (v0.4.56) the shared library's textures (data/common.hlb): a slot's look byte 0x40 + n (the area's: 0xC0 + n)
    const u8* iLibRgb; const u8* iLibTex; const u8* iLibTex1; const u8* iLibHi; const u8* iLibKey; const u8* iLibHi2;
    int iLibN, iLibHiN, iLibHi2N; u16* iLibShade;
    void SetLibTextures(const u8* aRgb, const u8* aTex, const u8* aTex1, int aN, const u8* aHi, int aHiN, const u8* aKey, const u8* aHi2, int aHi2N);
    int iFlatCol;
    int* iRecip;
    u8* iArena; u8* iArenaEnd; int iArenaOver;
    TV* iMeshTV;
    int iFrameCount;

    // internals
    void SetRes();
    void GenTextures();
    void BuildPalettes();
    void BuildStaticMeshes();
    void SetupCamera();
    void TransformPt(int aX, int aY, int aZ, TV& aV);
    void Outcode(TV& aV);
    void XfCtx(int* aX, int c0, int c2, int c3, int c4, int c5, int c6, int c7, int c8, const TV& t);
    void Project(TV& aV);
    void FogColour(int aZ, int aR, int aG, int aB, TV& aV);
    void DrawTri(TV* a, TV* b, TV* c, int aKind);
    void ClipAndDraw(TV* a, TV* b, TV* c, int aKind);
    void Raster(TV* a, TV* b, TV* c, int aKind);
    void DrawIStatic(const Obj& o);
    void OccMidBuild();             // (v0.4.75) the occlusion map made again among a pass's pieces (judged frame by frame)
    int OccMidOk() const { return !iOccMidOff || !(iFrameCount & 7); }
    int iOccMid, iOccMidGain0, iOccMidOff;  // (v0.4.75) an interior's piece, its model for how far (DrawInterior)
    void RasterFlat(TV* a, TV* b, TV* c);  // the same for K_FLAT, with only what it needs
    void RasterTex(TV* a, TV* b, TV* c);   // (v0.4.71) the same for K_TEX
    HalfState iTexHS;       // (v0.4.71) RasterTex's walker state
    TexTri iTexTri;         // (v0.4.73) tex_tri's
    FaceCtx iFaceCtx;       // (v0.4.73) face_shade's
    int iFaceRec[64 * 3];   // (v0.4.73) its records
    u16 iSlotInfo[256];     // (v0.4.71) what a slot's picture byte says (DrawFaces), worked out once a frame: SlotInfo()
    int SlotInfo(int aB);
    const void* iSlotKeyP[5]; int iSlotKeyN[6];     // (v0.4.73) the pictures iSlotInfo was made for (they change mid-frame: each area's own)
    void SlotInfoCheck();
    void RunHalf(int aKind, HalfState& h);
    void RunHalfPersp(HalfState& h);
    void RunBand(int aKind, int aPersp, HalfState& h, int aY, int aWmax);
    int iBandSkip;          // (stats) rows' runs left out by RunBand
    void DrawSky();
    void DrawTerrain();
    void DrawChunk(int cx, int cz);
    void DrawStatic(Obj& o); void DrawOldStatic(Obj& o, int d2);     // (v0.4.70)
    // (v0.4.74) the boxes things are drawn as from afar: one model, its corners and colours set for each
    Mesh iBoxMesh; short iBoxV[8 * 3]; u16 iBoxF[12 * 4]; short iBoxN[12 * 3]; u16 iBoxUV[12 * 6]; int iBoxPx;
    void BoxInfo(StaticDef& d);
    // (v0.4.74) far groups: the outside pieces of each 64 m square (a town's houses, walls, rocks) merged when an area
    // is read into one plain model - their far models snapped together on a grid - drawn in their stead from afar
    enum { MAXFG = 64, FG_SLOTS = 32 };
    struct FarGroup { Mesh m; int x, y, z, x0, z0, x1, z1, nobj; u8 slots[FG_SLOTS * 4]; };
    FarGroup iFG[MAXFG]; int iNFG; u8* iObjGrp;
    enum { PVSMIX = 1024 }; u8* iPvsMix;
    // (v0.4.76) the current interior's pieces: their list (DrawInterior reads only those, and a hidden one's record
    // not at all), and a grid of 4 m squares for TopAt / BlockedIn (they had read all the pieces of every loaded hall:
    // Forelhost's Stronghold, 17 % of a frame's cache misses). Made again when the cell or the pieces change.
    enum { IG_SQ = 4 * 256, IG_MAXSQ = 4096, IG_MAXENT = 16384 };
    u16* iICell; int iICellN;
    int* iIGHead; u16* iIGList; int iIGx0, iIGz0, iIGw, iIGh, iIGsq, iIGok;
    int iIGCell, iIGNum;
    enum { IR_CELLS = 64 }; int iIRange[IR_CELLS * 2], iIRangeNum;  // (v0.4.76) each cell's pieces: from, to (end)
    void IRangeOf(int aCell, int& aFrom, int& aTo);
    void IGridMake();
    int IGridAt(int aX, int aZ, int aR, int& aX0, int& aX1, int& aZ0, int& aZ1) const;
    int iOccEvery;          // (v0.4.75) the occlusion map made again after this many triangles (0: at pieces 16, 48, 128)
      // (v0.4.74) what can be seen, with what is seen beyond the open doors (8192 pieces)
    int iFGFarM, iFGGridU; u8 iFGFar[MAXFG];
    void BuildFarGroups(int aFirst, int aLast);
    int  FarGroupMesh(FarGroup& g, int gi, int aFirst, int aLast, u8* aTmp, int aTmpN);
    void DrawObjects(int aP0, int aP1);     // pass 0: the buildings and big pieces, 1: the rest (and the trees)
    void DrawTrees(int aCcx, int aCcz);
    void DrawFaces(const Mesh& m, int aFrame, int aLx, int aLy, int aLz, const u8* aSlots, int aFlags);
#ifdef R3D_STATS
    void DrawFacesX(const Mesh& m, int aFrame, int aLx, int aLy, int aLz, const u8* aSlots, int aFlags);
#endif
    void ClearBorders();
    // (v0.4.49) the big arrays last: the fields before them within 4 KB of the start, each one loaded in one ARM
    // instruction (behind iDefs, 112 KB, every one took three)
    StaticDef iDefs[MAXDEFS];
    u8 iLandBuf[4096];
    TV iClipA[12], iClipB[12];
    u16 iOccMap[OCCW * OCCH];
    Mesh iMSt[ST_COUNT + 3];    // static meshes (+ far tree, rock variant, mid tree)
    };

int r3d_sin(int a);
int r3d_cos(int a);
int r3d_isqrt(int v);
int r3d_atan2(int y, int x);            // 0..1023
void r3d_memset(void* p, int v, int n);
void r3d_memmove(void* d, const void* s, int n);
u32 r3d_hash(int x, int z, int s);
int r3d_clamp(int v, int lo, int hi);
int r3d_abs(int v);
char* r3d_puts(char* d, const char* s);
char* r3d_puti(char* d, int v);

#endif
