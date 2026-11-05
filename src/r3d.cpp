// r3d.cpp - fixed-point software 3D engine for the Nokia 9300 (see r3d.h)
#include "r3d.h"
#include "tables.h"

// ---------------------------------------------------------------- constants
enum
    {
    NEAR = 64,                  // 0.25 m
    FOG0 = 45 * METER,
    FOG1 = 170 * METER,
    FOGK = 256 * 65536 / (FOG1 - FOG0),   // fog factor scale: ((z - FOG0) * FOGK) >> 16 = 0..256
    FARD = 400 * METER,         // the farthest the detailed land is ever drawn (settings)
    WATER = 6 * METER,
    MAXGV = (MAXGX + 1) * (MAXGZ + 1),
    MAXFARV = 160,              // far land: vertices per row at most
    NTEX = 16,                  // the palette's hues (16 shades each)
    NPAT = 18,                  // texture patterns: one per hue, then patterns in a hue of their own (KPatRow)
    MAXOBJ = 4600,              // (v0.4.48: 4600, was 3000 - Whiterun's outskirts)
    MAXW = 640, MAXH = 200
    };

enum { TX_GRASS = 0, TX_ROCK, TX_SNOW, TX_SAND, TX_FUR, TX_NEEDLE, TX_FURN, TX_BLOCK, TX_DIRT, TX_LOGS, TX_THATCH, TX_NORDIC, TX_PLANKS, TX_SHINGLE, TX_FIELD, TX_SLATE,
       TX_DSTAVE, TX_ROPE };
// the hue each pattern past the first 16 is drawn in: Skyrim's dark static barrel's staves (the planks' grain in the
// fur's dark brown), its rope (an old, dim brown: the earth's hue, low shades)
static const u8 KPatRow[NPAT - NTEX] = { TX_FUR, TX_DIRT };
enum { K_FLAT = 0, K_GOURAUD = 1, K_TEX = 2 };

static const u8 KMatRGB[NTEX * 3] =
    {
    74, 112, 46,  112, 108, 102,  232, 238, 248,  186, 170, 118,
    82, 60, 44,  40, 78, 44,  92, 52, 34,  132, 126, 116,
    124, 98, 64,  118, 84, 52,  176, 150, 86,  150, 118, 78,
    126, 92, 60,  92, 70, 52,  186, 160, 80,  92, 94, 100
    };

// terrain material -> texture
static const u8 KTerrTex[TM_COUNT] = { TX_GRASS, TX_NEEDLE, TX_ROCK, TX_SNOW, TX_SAND, TX_DIRT, TX_DIRT, TX_BLOCK, TX_FIELD,
                                       TX_GRASS, TX_DIRT, TX_DIRT, TX_GRASS, TX_ROCK, TX_BLOCK };

// terrain material colours
static const u8 KTerrRGB[TM_COUNT * 3] =
    {
    78, 116, 48,    // grass
    54, 86, 40,     // forest floor
    112, 108, 102,  // rock
    232, 238, 248,  // snow
    186, 170, 118,  // sand
    124, 98, 64,    // dirt road
    86, 80, 66,     // barrow earth
    132, 128, 120,  // cobblestone
    188, 164, 84,   // wheat field
    138, 128, 72,   // tundra: Whiterun's dry grass
    150, 104, 48,   // the Rift's autumn floor
    122, 100, 70,   // the Reach's red earth
    100, 106, 80,   // Hjaalmarch's frozen marsh
    96, 90, 84,     // Eastmarch's ash and steam
    104, 100, 88    // Skyrim's roads: grey stone set in the earth (landscape\roads\road01)
    };

#ifdef __arm__
#define USE_ASM 1
extern "C" void half_gouraud1(HalfState* s);
extern "C" void half_tex1(HalfState* s);
extern "C" int tex_tri(TexTri* t);
extern "C" int face_shade(FaceCtx* c);
extern "C" void persp_rows1(HalfState* h, PerspCtx* c); extern "C" void persp_rowsq1(HalfState* h, PerspCtx* c); extern "C" void persp_rowsk1(HalfState* h, PerspCtx* c);
extern "C" void persp_rowsh1(HalfState* h, PerspCtx* c); extern "C" void persp_rowskq1(HalfState* h, PerspCtx* c); extern "C" void persp_rowskh1(HalfState* h, PerspCtx* c);
extern "C" int point_light(const int* L, int n, int cx, int cy, int cz);
extern "C" void persp_tex1(PerspCtx* c); extern "C" void persp_texq1(PerspCtx* c); extern "C" void persp_texk1(PerspCtx* c);
extern "C" void persp_texh1(PerspCtx* c); extern "C" void persp_texkq1(PerspCtx* c); extern "C" void persp_texkh1(PerspCtx* c);
extern "C" void half_flat1(HalfState* s);
extern "C" int tri_flat1(TV* v0, TV* v1, TV* v2, FlatCtx* c);
extern "C" int face_cull(const u16* F, const TV* tv, const u16* FL, int n, u16* out);
extern "C" void occ_build(const u16* z, int w, int bw, int bh, u16* map, int occw);
extern "C" int z_skip(const u16* z, int x, int x1);
extern "C" void xform_v(const short* V, TV* tv, int n, const int* K, const u16* L);
extern "C" void half_gouraud2(HalfState* s);
extern "C" void half_tex2(HalfState* s);
extern "C" void half_texq1(HalfState* s);
extern "C" void half_texq2(HalfState* s);
extern "C" void half_texh1(HalfState* s);
extern "C" void half_texkq1(HalfState* s);
extern "C" void half_texkh1(HalfState* s);
extern "C" void half_texk1(HalfState* s);
extern "C" void half_texk2(HalfState* s);
extern "C" void half_flat2(HalfState* s);
extern "C" void fill32(u32* p, u32 v, int nwords);
#endif

// ---------------------------------------------------------------- helpers
int r3d_sin(int a) { return KSinTab[a & 1023]; }
int r3d_cos(int a) { return KSinTab[(a + 256) & 1023]; }

int r3d_isqrt(int v)
    {
    if (v <= 0) return 0;
    unsigned int r = 0, b = 1u << 30, x = (unsigned int)v;
    while (b > x) b >>= 2;
    while (b)
        {
        if (x >= r + b) { x -= r + b; r = (r >> 1) + b; }
        else r >>= 1;
        b >>= 2;
        }
    return (int)r;
    }

void r3d_memmove(void* d, const void* s, int n)
    {
    u8* a = (u8*)d; const u8* b = (const u8*)s;
    int i;
    if (a <= b) for (i = 0; i < n; i++) a[i] = b[i];
    else for (i = n - 1; i >= 0; i--) a[i] = b[i];
    }

void r3d_memset(void* p, int v, int n)
    {
    u8* d = (u8*)p;
    u32 w = (u32)(v & 255) * 0x01010101u;
    while (n > 0 && ((unsigned long)d & 3)) { *d++ = (u8)v; n--; }
    u32* d4 = (u32*)d;
    while (n >= 16) { d4[0] = w; d4[1] = w; d4[2] = w; d4[3] = w; d4 += 4; n -= 16; }
    while (n >= 4) { *d4++ = w; n -= 4; }
    d = (u8*)d4;
    while (n-- > 0) *d++ = (u8)v;
    }

static u32 Hash(int x, int z, int s)
    {
    u32 h = (u32)x * 374761393u + (u32)z * 668265263u + (u32)s * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
    }

// value noise, 0..65535, period in cells
static int Noise(int x, int z, int period, int seed)
    {
    int ix = x / period, iz = z / period;
    int fx = ((x - ix * period) << 16) / period;
    int fz = ((z - iz * period) << 16) / period;
    fx = (int)(((unsigned)fx * (unsigned)fx >> 16) * (unsigned)(3 * 65536 - 2 * fx) >> 16);
    fz = (int)(((unsigned)fz * (unsigned)fz >> 16) * (unsigned)(3 * 65536 - 2 * fz) >> 16);
    int a = Hash(ix, iz, seed) & 0xFFFF, b = Hash(ix + 1, iz, seed) & 0xFFFF;
    int c = Hash(ix, iz + 1, seed) & 0xFFFF, d = Hash(ix + 1, iz + 1, seed) & 0xFFFF;
    int ab = a + (((b - a) * (fx >> 1)) >> 15);
    int cd = c + (((d - c) * (fx >> 1)) >> 15);
    return ab + (((cd - ab) * (fz >> 1)) >> 15);
    }

static inline int Clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline int Abs(int v) { return v < 0 ? -v : v; }

// (a * b) >> 30 with a 64-bit intermediate: one SMULL on ARM
#ifdef __arm__
static inline int MulHi30(int a, int b)
    {
    int lo, hi;
    __asm__("smull %0, %1, %2, %3" : "=&r"(lo), "=&r"(hi) : "r"(a), "r"(b));
    return (hi << 2) | (int)((unsigned)lo >> 30);
    }
#else
static inline int MulHi30(int a, int b) { return (int)(((long long)a * b) >> 30); }
#endif
// (a * b) >> s with a 64-bit intermediate, 0 < s < 32
#ifdef USE_ASM
static inline int MulShift(int a, int b, int s)
    {
    int lo, hi;
    __asm__("smull %0, %1, %2, %3" : "=&r"(lo), "=&r"(hi) : "r"(a), "r"(b));
    return (hi << (32 - s)) | (int)((unsigned)lo >> s);
    }
#else
static inline int MulShift(int a, int b, int s) { return (int)(((long long)a * b) >> s); }
#endif

static inline u16 Rgb(int r, int g, int b)
    {
    r = Clamp(r, 0, 255); g = Clamp(g, 0, 255); b = Clamp(b, 0, 255);
    return (u16)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    }

static char* PutStr(char* d, const char* s) { while (*s) *d++ = *s++; *d = 0; return d; }
static char* PutInt(char* d, int v)
    {
    char t[12]; int n = 0;
    if (v < 0) { *d++ = '-'; v = -v; }
    do { t[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) *d++ = t[--n];
    *d = 0;
    return d;
    }
// fixed 1 decimal: v10 = value*10
static char* PutDec(char* d, int v10) { d = PutInt(d, v10 / 10); *d++ = '.'; *d++ = (char)('0' + Abs(v10) % 10); *d = 0; return d; }

// exported helpers
u32 r3d_hash(int x, int z, int s) { return Hash(x, z, s); }
int r3d_clamp(int v, int lo, int hi) { return Clamp(v, lo, hi); }
int r3d_abs(int v) { return Abs(v); }
char* r3d_puts(char* d, const char* s) { return PutStr(d, s); }
char* r3d_puti(char* d, int v) { return PutInt(d, v); }
u16 R3D::Rgb(int r, int g, int b) { return ::Rgb(r, g, b); }

// angle of (x, y) in 0..1023 units (0 = +y axis / "north", increasing clockwise towards +x)
int r3d_atan2(int x, int y)
    {
    if (x == 0 && y == 0) return 0;
    int ax = Abs(x), ay = Abs(y);
    int a;
    // octant approximation: atan(t) ~ t*(128 + 0.27*(1-t))...: use 0..128 for 0..45 deg
    if (ax <= ay)
        {
        int t = (ax << 10) / ay;                            // 0..1024
        a = (t * 128 + t * (1024 - t) / 1024 * 35 / 128) >> 10;   // ~ atan(t)/(2pi)*1024
        }
    else
        {
        int t = (ay << 10) / ax;
        a = 256 - ((t * 128 + t * (1024 - t) / 1024 * 35 / 128) >> 10);
        }
    // a in 0..256 for first quadrant (x>=0,y>=0), measured from +y towards +x
    if (x >= 0 && y >= 0) return a;
    if (x >= 0 && y < 0) return 512 - a;
    if (x < 0 && y < 0) return 512 + a;
    return (1024 - a) & 1023;
    }

// ---------------------------------------------------------------- memory
enum { ARENA_KB = 4972 };     // (v0.4.70: +40 - the statics' order, MAXOBJ x 8 bytes)     // (v0.4.68: +50 - MAXIOBJ 5600)     // (v0.4.57: +256 - Skyrim's Whiterun left 87 KB)     // the mesh arena (KB; v0.4.55: -450 - the game's built barrow gone (Skyrim's own, its own file), MAXIOBJ 3600; v0.4.51: +54, the barrow's pieces that had no room (and a margin); v0.4.48: +24, the interiors' objects (MAXIOBJ 2400); v0.4.40: +70, the vertex scratch for MAXMV 2048; v0.4.43: +16, the face and vertex lists)
static int Align(int v) { return (v + 7) & ~7; }

int R3D::MemNeeded()
    {
    return Align(MAXW * MAXH * 2)              // z buffer
        + Align(WINW * WINH * 2)                // heights: the window
        + Align(WINW * WINH)                    // ground: the window
        + Align(MAXCW * MAXCH * 2) + Align(MAXCW * MAXCH)     // the coarse land
        + Align((MAXLTX * MAXLTZ + 1) * 4)      // the land file's tiles
        + Align(MAXNCX * MAXNCZ * 4) * 3 + Align(MAXNCX * MAXNCZ * 2) * 2
        + Align(TCACHE * sizeof(TChunk))        // the chunks near the camera, lit, with their trees
        + Align(2 * MAXFARV * sizeof(TV))
        + Align((MAXGX / 16 + 1) * (MAXGZ / 16 + 1) * 3)
        + Align(MAXOBJ * sizeof(Obj))
        + Align(NPAT * 64 * 64)
        + Align(256 * 2) + Align(16 * 8 * 256 * 2) * 3
        + Align(81 * sizeof(TV))
        + 65536 * 4
        + 65536 * 2                             // gamma table
        + ARENA_KB * 1024                       // mesh arena (static + game meshes)
        + 1024;
    }

void R3D::Init(u8* aMem)
    {
    u8* p = aMem;
    {
    // (v0.4.74) the box model (BoxInfo, DrawStatic): 8 corners, 12 faces; its corners set for each thing drawn as it
    Mesh& b = iBoxMesh;
    r3d_memset(&b, 0, sizeof(b));
    b.v = iBoxV; b.f = iBoxF; b.n = iBoxN; b.uv = iBoxUV; b.nfr = 1; b.vstride = 8; b.fstride = 12;
    iMeshCursor = 0;
    MeshBox(b, 0, 1, 0, 1000, 0, 0, 1000, 500, 1000);
    int k;
    for (k = 0; k < b.nf; k++) if (b.n[k * 3 + 1] > 8000) b.f[k * 4 + 3] = 0;    // (the top: slot 0, the roof's colour)
    iBoxPx = 5;
    }
    iOccN = 0; iSkip = 0; iHalfRows = 0; iHalfOn = 0; iFlatLand = 0; iClutterNear = 0;
    iZB = (u16*)p; p += Align(MAXW * MAXH * 2);
    iWinH = (short*)p; p += Align(WINW * WINH * 2);
    iWinM = p; p += Align(WINW * WINH);
    iCH = (short*)p; p += Align(MAXCW * MAXCH * 2);
    iCM = p; p += Align(MAXCW * MAXCH);
    iTileOff = (u32*)p; p += Align((MAXLTX * MAXLTZ + 1) * 4);
    iHeight = iWinH; iMat = iWinM; iHS = WINW; iWX0 = iWZ0 = 0; iWW = iWH = 0;
    iCW = iCHt = 1; iLandBaked = 0; iLandHook = 0; iLandHookCtx = 0; iNTX = iNTZ = 0; iLandLoads = 0; iLandSeek = 0;
    iChunkMinH = (int*)p; p += Align(MAXNCX * MAXNCZ * 4);
    iChunkMaxH = (int*)p; p += Align(MAXNCX * MAXNCZ * 4);
    iChunkObj = (int*)p; p += Align(MAXNCX * MAXNCZ * 4);
    iChunkSlot = (short*)p; p += Align(MAXNCX * MAXNCZ * 2);
    iChunkWater = (short*)p; p += Align(MAXNCX * MAXNCZ * 2);
    iLandLoaded = 0; iLandOffset = 0;
    iTC = (TChunk*)p; p += Align(TCACHE * sizeof(TChunk));
    iFarV = (TV*)p; p += Align(2 * MAXFARV * sizeof(TV));
    iFarCol = p; p += Align((MAXGX / 16 + 1) * (MAXGZ / 16 + 1) * 3);
    iFarColKey = -1; iFarColRow = 0;
    SetWorldSize(128, 128);
    iNearM = 170; iFarM = 0; iFogSq = 0; iFogEndM = 170; iFog0M = 45; iFarOn = 0; iFarVis = 0;
    iObj = (Obj*)p; p += Align(MAXOBJ * sizeof(Obj));
    iTex = p; p += Align(NPAT * 64 * 64);
    iBasePal = (u16*)p; p += Align(256 * 2);
    iShadePal = (u16*)p; p += Align(16 * 8 * 256 * 2);
    iSkyShade = (u16*)p; p += Align(16 * 8 * 256 * 2);
    iLibShade = (u16*)p; p += Align(16 * 8 * 256 * 2);
    iLibRgb = iLibTex = iLibTex1 = iLibHi = iLibKey = iLibHi2 = 0; iLibN = iLibHiN = iLibHi2N = 0;
    iPvsBits = 0; iPvsFirst = iPvsN = 0;
    iOutPvsBits = 0; iOutPvsFirst = iOutPvsN = 0; iFallTree[0] = iFallTree[1] = 0;
    iSkyTex = 0; iSkyTex1 = 0; iSkyRgb = 0; iSkyN = 0; iTexDist = 50 * METER; iSkyHi = 0; iSkyHiN = 0; iTexQ = 0; iSkyKey = 0; iSkyTex32 = iSkyTex16 = 0; iTexMip = 0;
    iVerts = (TV*)p; p += Align(81 * sizeof(TV));
    iRecip = (int*)p; p += 65536 * 4;
    iGammaLut = (u16*)p; p += 65536 * 2;
    iNumLights = 0; iAmbient = 0; iGamma = 0;
    iArena = p;
    iArenaEnd = p + ARENA_KB * 1024;
    iRecip[0] = 0x7FFFFFFF;
    int n;
    for (n = 1; n < 65536; n++) iRecip[n] = (1 << 30) / n;

    int sx = 9000, sy = 12000, sz = -6000;
    int l = r3d_isqrt(sx * sx / 16 + sy * sy / 16 + sz * sz / 16) * 4;
    iSunX = sx * 16384 / l; iSunY = sy * 16384 / l; iSunZ = sz * 16384 / l;
    SetAtmosphere(256, 178, 196, 218, 70, 110, 190, 0);
    iMode = MODE_GOURAUD;
    iViewScale = 256;
    iMeshTV = (TV*)Alloc(MAXMV * sizeof(TV));
    iIObj = (Obj*)Alloc(MAXIOBJ * sizeof(Obj));
    iINum = 0; iCell = 0;
    iCell = 0;
    SetFog(FOG0 >> 8, FOG1 >> 8);
    iMeshLev = (int*)Alloc(MAXMV * 4);
    iLodNear = 3; iLodFar[0] = 0; iLodFar[LODC_BUILDING] = 10; iLodFar[LODC_TREE] = 12; iLodFar[LODC_FURN] = 5; iLodFar[LODC_ROCK] = 6;
    iSkyHi2 = 0; iSkyHi2N = 0;
    iClOn = 0; iStampN = -0x7FFFFFFF; iClSkipped = 0; iBandSkip = 0;
    iFaceList = (u16*)Alloc(MAXLF * 2); iFaceN = 0;
    iSortKey = (u32*)Alloc(MAXOBJ * 2 * 4); iSortCnt = (int*)Alloc(513 * 4);
    iLightR = iLightG = iLightB = 256;
    iPvsMix = Alloc(PVSMIX);
    iOccEvery = 0;
    iObjGrp = Alloc(MAXOBJ); r3d_memset(iObjGrp, 0, MAXOBJ); iNFG = 0; iFGFarM = 120; iFGGridU = 8 * METER;
    r3d_memset(&iFlatHS, 0, sizeof(iFlatHS));
    iLazyList = (u16*)Alloc(MAXMV * 2);
    iSunVis = 0; iSunDayLev = 256; iCloud = 0; iCloudR = iCloudG = iCloudB = 240; iBakedSun = -1;
    GenTextures();
    BuildPalettes();
    BuildStaticMeshes();
    iOutMode = -1;
    SetOutputMode(0);
    }

// ---------------------------------------------------------------- output modes
void R3D::SetOutputMode(int aMode)
    {
    iBorderFrames = 3;
    iOutMode = aMode;
    iNative = aMode != 3;
    SetRes();
    }

void R3D::SetRes()
    {
    iW = iOutMode == 1 ? 400 : (iOutMode == 2 ? 640 : 320);
    iH = 200;
    // 90 degree horizontal FOV; native modes have square pixels, the doubled mode 2:1 pixels
    iFx = iW / 2;
    iFy = iNative ? iFx : iFx * 2;
    // keep the vertical view sensible on 640-wide native (otherwise only 35 degrees)
    iFx16 = iFx * 16; iFy16 = iFy * 16;
    iCx16 = iW * 8; iCy16 = iH * 8;
    iHW = iW / 2; iHH = iH / 2;
    iLenX = r3d_isqrt(iFx * iFx + iHW * iHW); iLenY = r3d_isqrt(iFy * iFy + iHH * iHH);
    }

void R3D::SetAtmosphere(int aLight, int aFogR, int aFogG, int aFogB, int aZenR, int aZenG, int aZenB, int aStars)
    {
    iLight = aLight;
    iFogR = aFogR; iFogG = aFogG; iFogB = aFogB;
    if (iShadePal && Abs(aFogR - iPalFogR) + Abs(aFogG - iPalFogG) + Abs(aFogB - iPalFogB) > 24) BuildPalettes();
    iZenR = aZenR; iZenG = aZenG; iZenB = aZenB;
    iStars = aStars;
    }

// ---------------------------------------------------------------- sun, terrain lighting, sky
// the land is lit chunk by chunk as it comes into view (Cached); a new sun only changes the key they are lit for
void R3D::BakeTerrainLight()
    {
    int day = iSunDayLev > 0 ? iSunDayLev : 256;
    iBakedSun = ((iSunX >> 9) & 0xFF) | (((iSunY >> 9) & 0xFF) << 8) | (((iSunZ >> 9) & 0xFF) << 16) | ((day >> 4) << 24);
    }

// the lit colour of a terrain vertex: its ground's colour (varied a little), shaded by the sun on its slope;
// and its light level for the textured mode
#ifdef R3D_STATS
int g_r3dVC;
#endif
void R3D::VertexColour(int x, int z, int& aR, int& aG, int& aB, int& aLev)
    {
#ifdef R3D_STATS
    g_r3dVC++;
#endif
    int W = iHS;
    int i = z * W + x;
    int hl, hr, hd, hu, ny = 2 * CELL, m0;
    if (InWin(x - 1, z - 1) && InWin(x + 1, z + 1))
        {
        hl = iHeight[i - (x > 0 ? 1 : 0)]; hr = iHeight[i + (x < iGX ? 1 : 0)];
        hd = iHeight[i - (z > 0 ? W : 0)]; hu = iHeight[i + (z < iGZ ? W : 0)];
        m0 = iMat[i];
        }
    else
        {
        // the coarse land (the far land, the map): its slope over its own points
        hl = iCH[CIdx(x - CSTEP, z)]; hr = iCH[CIdx(x + CSTEP, z)];
        hd = iCH[CIdx(x, z - CSTEP)]; hu = iCH[CIdx(x, z + CSTEP)];
        ny = 2 * CSTEP * CELL; m0 = iCM[CIdx(x, z)];
        }
    int nx = (hl - hr) * HS, nz = (hd - hu) * HS;
    int l = r3d_isqrt((nx >> 2) * (nx >> 2) + (ny >> 2) * (ny >> 2) + (nz >> 2) * (nz >> 2)) * 4;
    if (l < 4) l = 4;
    int dot = ((nx * 127 / l) * (iSunX >> 7) + (ny * 127 / l) * (iSunY >> 7) + (nz * 127 / l) * (iSunZ >> 7));
    if (dot < 0) dot = 0;
    int lev = 70 + (dot * 200 >> 14);
    int m = m0 & VM_MAT;
    u32 hv = Hash(x, z, 9);
    int var = 232 + (int)((hv >> 8) & 47);
    int r = KTerrRGB[m * 3] * var >> 8, g = KTerrRGB[m * 3 + 1] * var >> 8, b = KTerrRGB[m * 3 + 2] * var >> 8;
    aR = Clamp(r * lev >> 8, 2, 253); aG = Clamp(g * lev >> 8, 2, 253); aB = Clamp(b * lev >> 8, 2, 253);
    int day = iSunDayLev > 0 ? iSunDayLev : 256;
    aLev = Clamp(((lev * day >> 8) - 40) >> 4, 0, 15);
    }

// the sun's direction (towards it, any length) and the daylight level; terrain is re-lit when it has moved on
void R3D::SetSun(int aDx, int aDy, int aDz, int aDayLev)
    {
    int l = r3d_isqrt((aDx >> 2) * (aDx >> 2) + (aDy >> 2) * (aDy >> 2) + (aDz >> 2) * (aDz >> 2)) * 4;
    if (l < 4) return;
    iSunX = aDx * 16384 / l; iSunY = aDy * 16384 / l; iSunZ = aDz * 16384 / l;
    iSunDayLev = aDayLev;
    int key = ((iSunX >> 9) & 0xFF) | (((iSunY >> 9) & 0xFF) << 8) | (((iSunZ >> 9) & 0xFF) << 16) | ((aDayLev >> 4) << 24);
    if (key != iBakedSun) BakeTerrainLight();
    }

// ---------------------------------------------------------------- point lights and gamma
void R3D::AddLight(int aX, int aY, int aZ, int aRadius, int aStrength)
    {
    if (iNumLights >= MAXLIGHTS) return;
    int* l = iLW + iNumLights * 5;
    l[0] = aX; l[1] = aY; l[2] = aZ; l[3] = aRadius; l[4] = aStrength;
    int r16 = aRadius >> 4;
    iLInv[iNumLights] = r16 > 0 ? (256 << 16) / (r16 * r16) : 0;
    iNumLights++;
    }

// after the camera is set: the lights into camera space
void R3D::PrepareLights()
    {
    int i;
    iLightListMask = -1;                 // (v0.4.73: the compact list made again)
    for (i = 0; i < iNumLights; i++)
        {
        TV t;
        TransformPt(iLW[i * 5], iLW[i * 5 + 1], iLW[i * 5 + 2], t);
        iLC[i * 3] = t.x; iLC[i * 3 + 1] = t.y; iLC[i * 3 + 2] = t.z;
        }
    }

// does any light reach a sphere (camera space)? Meshes and terrain chunks out of every light's reach skip the work
// which lights reach a sphere (camera space): a bit each, kept for PointLight (only those are looked at)
int R3D::LightsNear(int cx, int cy, int cz, int rad)
    {
    int i, mask = 0;
    for (i = 0; i < iNumLights; i++)
        {
        const int* c = iLC + i * 3;
        int R = iLW[i * 5 + 3] + rad;
        int dx = cx - c[0], dy = cy - c[1], dz = cz - c[2];
        if (dx < R && dx > -R && dy < R && dy > -R && dz < R && dz > -R) mask |= 1 << i;
        }
    iLightMask = mask;
    return mask != 0;
    }

// extra light level at a camera-space point: each light falls off smoothly to nothing at its reach (the lights
// LightsNear found near what is drawn)
// (v0.4.73) the lights of iLightMask in a compact list, for point_light (spans.s)
void R3D::SetLightList()
    {
    int i, n = 0, mask = iLightMask;
    for (i = 0; mask; i++, mask >>= 1)
        {
        if (!(mask & 1)) continue;
        int* L = iLightList + n * 8;
        int R = iLW[i * 5 + 3], r16 = R >> 4;
        L[0] = iLC[i * 3]; L[1] = iLC[i * 3 + 1]; L[2] = iLC[i * 3 + 2];
        L[3] = R; L[4] = 2 * R; L[5] = r16 * r16; L[6] = iLInv[i]; L[7] = iLW[i * 5 + 4];
        n++;
        }
    iLightListN = n; iLightListMask = iLightMask; iLightListFrame = iFrameCount;
    }

int R3D::PointLight(int cx, int cy, int cz)
    {
#ifdef USE_ASM
    if (iLightListMask != iLightMask || iLightListFrame != iFrameCount) SetLightList();
    return point_light(iLightList, iLightListN, cx, cy, cz);
#endif
    int add = 0, i, mask = iLightMask;
    for (i = 0; mask; i++, mask >>= 1)
        {
        if (!(mask & 1)) continue;
        const int* c = iLC + i * 3;
        int R = iLW[i * 5 + 3];
        int dx = cx - c[0], dy = cy - c[1], dz = cz - c[2];
        if (dx > R || dx < -R || dy > R || dy < -R || dz > R || dz < -R) continue;
        int r16 = R >> 4;
        int d2 = (dx >> 4) * (dx >> 4) + (dy >> 4) * (dy >> 4) + (dz >> 4) * (dz >> 4);
        int R2 = r16 * r16;
        if (d2 >= R2) continue;
        int f = ((R2 - d2) * iLInv[i]) >> 16;          // 0..256
        add += (iLW[i * 5 + 4] * ((f * f) >> 8)) >> 8;
        }
    return add;
    }

// gamma: brighten the dark and middle tones of the finished frame through a lookup table
void R3D::SetGamma(int aG)
    {
    iGamma = aG < 0 ? 0 : (aG > 3 ? 3 : aG);
    if (!iGamma || !iGammaLut) return;
    // per channel curve: out = in^(1/g), g = 1.25, 1.5, 1.8 (integer approximation by square-root blending)
    u8 cur[64];
    int k;
    for (k = 0; k < 64; k++)
        {
        int x = k * 256 / 63;                           // 0..256
        int sq = 0;
        while ((sq + 1) * (sq + 1) <= x * 256) sq++;    // sqrt(x/256)*256
        int w = iGamma == 1 ? 90 : (iGamma == 2 ? 160 : 230);
        int y = x + (((sq - x) * w) >> 8);
        cur[k] = (u8)(y > 255 ? 255 : y);
        }
    int i;
    for (i = 0; i < 65536; i++)
        {
        int r = (i >> 11) & 31, g = (i >> 5) & 63, b = i & 31;
        int R = cur[r * 63 / 31], G = cur[g], B = cur[b * 63 / 31];
        iGammaLut[i] = (u16)(((R >> 3) << 11) | ((G >> 2) << 5) | (B >> 3));
        }
    }

void R3D::ApplyGamma(u16* aOut, int aStride)
    {
    if (!iGamma || !iGammaLut) return;
    int x0 = OutX0(), w = OutW(), y;
    const u16* L = iGammaLut;
    for (y = 0; y < OUT_H; y++)
        {
        u16* p = aOut + y * aStride + x0;
        int n = w;
        while (n >= 4) { p[0] = L[p[0]]; p[1] = L[p[1]]; p[2] = L[p[2]]; p[3] = L[p[3]]; p += 4; n -= 4; }
        while (n-- > 0) { *p = L[*p]; p++; }
        }
    }

// ---------------------------------------------------------------- world building
void R3D::SetWorldSize(int aGX, int aGZ)
    {
    if (aGX > MAXGX) aGX = MAXGX;
    if (aGZ > MAXGZ) aGZ = MAXGZ;
    iGX = aGX & ~(CHUNK - 1); iGZ = aGZ & ~(CHUNK - 1);
    iGVX = iGX + 1;
    iNCX = iGX / CHUNK; iNCZ = iGZ / CHUNK;
    iCW = iGX / CSTEP + 1; iCHt = iGZ / CSTEP + 1;
#ifdef R3D_FULLLAND
    iHS = iGVX; iWX0 = iWZ0 = 0; iWW = iGX + 1; iWH = iGZ + 1; iHeight = iWinH; iMat = iWinM;
#else
    iWW = iWH = 0;                                   // nothing loaded yet: FocusLand
#endif
    iFarColKey = -1; iFarColRow = 0;
    int k;
    for (k = 0; k < iNCX * iNCZ; k++) { iChunkSlot[k] = -1; iChunkObj[k] = -1; }
    for (k = 0; k < TCACHE; k++) iTC[k].chunk = -1;
    iTCNext = 0;
    }

void R3D::GenTerrain(int aSeed)
    {
    int x, z;
    if (iWW <= iGX) { FlatLand(); return; }
    for (z = 0; z <= iGZ; z++)
        for (x = 0; x <= iGX; x++)
            {
            int h = (Noise(x, z, 32, aSeed + 1) * 22 >> 8) + (Noise(x, z, 16, aSeed + 2) * 9 >> 8)
                  + (Noise(x, z, 8, aSeed + 3) * 3 >> 8) + (Noise(x, z, 4, aSeed + 4) >> 8);
            h += 3 * METER;
            iHeight[z * iHS + x] = (short)(h / HS);
            iMat[z * iHS + x] = 0;
            }
    for (x = 0; x < iNCX * iNCZ; x++) iChunkWater[x] = (short)(WATER / HS);
    iObjCount = 0; iLandLoaded = 0;
    }

// ---------------------------------------------------------------- Skyrim's land (tools/make_land.py)
int R3D::LandByte()
    {
    if (iLandPos >= iLandLen)
        {
        iLandLen = iLandRead(iLandCtx, iLandBuf, sizeof(iLandBuf));
        iLandPos = 0;
        if (iLandLen <= 0) { iLandLen = 0; return -1; }
        }
    return iLandBuf[iLandPos++];
    }

static int LandU32(R3D& r)
    {
    int a = r.LandByte(), b = r.LandByte(), c = r.LandByte(), d = r.LandByte();
    return a | (b << 8) | (c << 16) | (d << 24);
    }

int R3D::LoadLand(TLandRead aRead, TLandSeek aSeek, void* aCtx)
    {
    iLandRead = aRead; iLandSeek = aSeek; iLandCtx = aCtx; iLandPos = iLandLen = 0;
    if (LandByte() != 'H' || LandByte() != 'L' || LandByte() != 'N' || LandByte() != 'D') return 0;
    int ver = LandU32(*this);
    int gx = LandU32(*this), gz = LandU32(*this);
    int offCm = LandU32(*this);
    if (gx > MAXGX || gz > MAXGZ || (gx & (CHUNK - 1)) || (gz & (CHUNK - 1))) return 0;
    int i;
    if (ver == 2)
        {
        // baked and tiled (tools/bake_land): the coarse land, the water, where the tiles are; the tiles are read
        // as the player goes
        int cw = LandU32(*this), ch = LandU32(*this), ntx = LandU32(*this), ntz = LandU32(*this);
        if (cw != gx / CSTEP + 1 || ch != gz / CSTEP + 1 || ntx > MAXLTX || ntz > MAXLTZ || !aSeek) return 0;
        SetWorldSize(gx, gz);
        iLandOffset = offCm * METER / 100;
        for (i = 0; i < cw * ch; i++) { int lo = LandByte(), hi = LandByte(); if (hi < 0) return 0; iCH[i] = (short)(lo | (hi << 8)); }
        for (i = 0; i < cw * ch; i++) { int m = LandByte(); if (m < 0) return 0; iCM[i] = (u8)m; }
        for (i = 0; i < iNCX * iNCZ; i++) { int lo = LandByte(), hi = LandByte(); if (hi < 0) return 0; iChunkWater[i] = (short)(lo | (hi << 8)); }
        iNTX = ntx; iNTZ = ntz;
        for (i = 0; i <= ntx * ntz; i++) iTileOff[i] = (u32)LandU32(*this);
        iObjCount = 0;
        iLandLoaded = 1; iLandBaked = 1;
        return 1;
        }
#ifdef R3D_FULLLAND
    if (ver != 1) return 0;
    SetWorldSize(gx, gz);
    iLandOffset = offCm * METER / 100;
    // heights: the difference from left + below - below-left, a byte or -128 and a short
    LandU32(*this);
    int W = iHS, x, z;
    for (z = 0; z <= iGZ; z++)
        {
        short* row = iHeight + z * W;
        for (x = 0; x <= iGX; x++)
            {
            int v = iLandPos < iLandLen ? iLandBuf[iLandPos++] : LandByte();
            if (v < 0) return 0;
            if (v == 128) { int lo = LandByte(), hi = LandByte(); v = (short)(lo | (hi << 8)); }
            else if (v > 128) v -= 256;
            int p;
            if (z == 0) p = x ? row[x - 1] : 0;
            else if (x == 0) p = row[x - W];
            else p = row[x - 1] + row[x - W] - row[x - W - 1];
            row[x] = (short)(p + v);
            }
        }
    // ground, run-length
    LandU32(*this);
    int n = iGVX * (iGZ + 1);
    i = 0;
    while (i < n)
        {
        int c = LandByte(), m = LandByte();
        if (m < 0) return 0;
        c++;
        if ((m & VM_MAT) >= TM_COUNT) m = (m & ~VM_MAT) | TM_TUNDRA;
        while (c-- > 0 && i < n) { iMat[(i / iGVX) * W + i % iGVX] = (u8)m; i++; }
        }
    // water per chunk
    LandU32(*this);
    for (i = 0; i < iNCX * iNCZ; i++)
        {
        int lo = LandByte(), hi = LandByte();
        if (hi < 0) return 0;
        iChunkWater[i] = (short)(lo | (hi << 8));
        }
    iObjCount = 0;
    iLandLoaded = 1; iLandBaked = 0;
    return 1;
#else
    return 0;
#endif
    }

// no land file: level ground 3 m up, grass, no water
void R3D::FlatLand()
    {
    int i;
    if (!iGX) SetWorldSize(128, 128);
    iNTX = iNTZ = 0;
    for (i = 0; i < iCW * iCHt; i++) { iCH[i] = (short)(3 * METER / HS); iCM[i] = 0; }
    for (i = 0; i < iNCX * iNCZ; i++) iChunkWater[i] = NOWATER;
    iLandLoaded = 0; iLandBaked = 1;
    }

// tiles tx0..tx1, tz0..tz1 (inclusive) from the land file into the window (each LTILE x LTILE vertices: heights as
// the difference from left + below - below-left within the tile, then the ground run-length coded)
void R3D::LoadTiles(int aTX0, int aTZ0, int aTX1, int aTZ1)
    {
    int tx, tz;
    for (tz = aTZ0; tz <= aTZ1; tz++)
        for (tx = aTX0; tx <= aTX1; tx++)
            {
            int x0 = tx * LTILE, z0 = tz * LTILE, i, j;
            int ok = tx >= 0 && tz >= 0 && tx < iNTX && tz < iNTZ && iLandSeek && iLandSeek(iLandCtx, (int)iTileOff[tz * iNTX + tx]);
            iLandPos = iLandLen = 0;
            iLandLoads++;
            for (j = 0; j < LTILE; j++)
                {
                if (!InWin(x0, z0 + j)) continue;
                short* row = iHeight + (z0 + j) * iHS + x0;
                for (i = 0; i < LTILE; i++)
                    {
                    if (!ok) { row[i] = (short)(3 * METER / HS); continue; }
                    int v = iLandPos < iLandLen ? iLandBuf[iLandPos++] : LandByte();
                    if (v < 0) { ok = 0; row[i] = 0; continue; }
                    if (v == 128) { int lo = LandByte(), hi = LandByte(); v = (short)(lo | (hi << 8)); }
                    else if (v > 128) v -= 256;
                    int p;
                    if (j == 0) p = i ? row[i - 1] : 0;
                    else if (i == 0) p = row[i - iHS];
                    else p = row[i - 1] + row[i - iHS] - row[i - iHS - 1];
                    row[i] = (short)(p + v);
                    }
                }
            int n = 0;
            while (n < LTILE * LTILE)
                {
                int c = ok ? LandByte() : LTILE * LTILE - 1, m = ok ? LandByte() : 0;
                if (m < 0) { ok = 0; continue; }
                c++;
                while (c-- > 0 && n < LTILE * LTILE)
                    {
                    int x = x0 + (n & (LTILE - 1)), z = z0 + n / LTILE;
                    if (InWin(x, z)) iMat[z * iHS + x] = (u8)m;
                    n++;
                    }
                }
            LandSettled(x0, z0, x0 + LTILE - 1, z0 + LTILE - 1);
            }
    }

// the window round a point: its tiles, the point's in the middle (the fifth of eight across); what is already in
// the window is moved, the rest read
void R3D::FocusLand(int aX, int aZ)
    {
#ifndef R3D_FULLLAND
    const int NT = WINW / LTILE;
    int tx = aX / CELL / LTILE, tz = aZ / CELL / LTILE;
    int maxx = iNTX - NT, maxz = iNTZ - NT;
    int ox = tx - NT / 2, oz = tz - NT / 2;
    if (ox > maxx) ox = maxx;
    if (oz > maxz) oz = maxz;
    if (ox < 0) ox = 0;
    if (oz < 0) oz = 0;
    int nx0 = ox * LTILE, nz0 = oz * LTILE;
    int had = iWW > 0;
    if (had && nx0 == iWX0 && nz0 == iWZ0) return;
    int dx = had ? nx0 - iWX0 : WINW, dz = had ? nz0 - iWZ0 : WINH;
    if (Abs(dx) >= WINW || Abs(dz) >= WINH) had = 0;
    if (had)
        {
        // move what stays: row by row, in the order that never overwrites what is still to be moved
        int n = WINW - Abs(dx), r;
        int sx = dx > 0 ? dx : 0, tx2 = dx < 0 ? -dx : 0;
        for (int k = 0; k < WINH - Abs(dz); k++)
            {
            r = dz >= 0 ? k : WINH - 1 - k;
            int src = r + dz;
            r3d_memmove(iWinH + r * WINW + tx2, iWinH + src * WINW + sx, n * 2);
            r3d_memmove(iWinM + r * WINW + tx2, iWinM + src * WINW + sx, n);
            }
        }
    else
        {
        int k;
        for (k = 0; k < TCACHE; k++) if (iTC[k].chunk >= 0) { iChunkSlot[iTC[k].chunk] = -1; iTC[k].chunk = -1; }
        }
    int owx = iWX0, owz = iWZ0;
    iWX0 = nx0; iWZ0 = nz0; iWW = WINW; iWH = WINH; iHS = WINW;
    iHeight = iWinH - (iWZ0 * iHS + iWX0);
    iMat = iWinM - (iWZ0 * iHS + iWX0);
    // the tiles that were not there
    int a, b;
    for (b = oz; b < oz + NT; b++)
        for (a = ox; a < ox + NT; a++)
            {
            int vx = a * LTILE, vz = b * LTILE;
            if (had && vx >= owx && vx < owx + WINW && vz >= owz && vz < owz + WINH) continue;
            LoadTiles(a, b, a, b);
            }
#else
    (void)aX; (void)aZ;
#endif
    }

// as the player moves: when they come within two tiles (256 m) of the window's edge, it follows
int R3D::UpdateLandWindow(int aX, int aZ)
    {
#ifndef R3D_FULLLAND
    if (!iLandBaked) return 0;
    int tx = aX / CELL / LTILE - iWX0 / LTILE, tz = aZ / CELL / LTILE - iWZ0 / LTILE;
    const int NT = WINW / LTILE;
    int edgeX = (iWX0 == 0 && tx < 2) || (iWX0 / LTILE + NT >= iNTX && tx > NT - 3);
    int edgeZ = (iWZ0 == 0 && tz < 2) || (iWZ0 / LTILE + NT >= iNTZ && tz > NT - 3);
    if (iWW && ((tx >= 2 && tx <= NT - 3) || edgeX) && ((tz >= 2 && tz <= NT - 3) || edgeZ)) return 0;
    int before = iLandLoads;
    FocusLand(aX, aZ);
    return iLandLoads != before;
#else
    (void)aX; (void)aZ; return 0;
#endif
    }

// after a part of the land is loaded (vertices x0..x1, z0..z1): the chunks there (their heights, their cache),
// the things standing on the ground there, then the game's own
void R3D::LandSettled(int aX0, int aZ0, int aX1, int aZ1)
    {
    int cx0 = Clamp((aX0 - 1) / CHUNK, 0, iNCX - 1), cx1 = Clamp((aX1 + 1) / CHUNK, 0, iNCX - 1);
    int cz0 = Clamp((aZ0 - 1) / CHUNK, 0, iNCZ - 1), cz1 = Clamp((aZ1 + 1) / CHUNK, 0, iNCZ - 1);
    int cx, cz;
    for (cz = cz0; cz <= cz1; cz++)
        for (cx = cx0; cx <= cx1; cx++)
            {
            int ci = cz * iNCX + cx;
            if (iChunkSlot[ci] >= 0) { iTC[iChunkSlot[ci]].chunk = -1; iChunkSlot[ci] = -1; }
            if (!InWin(cx * CHUNK, cz * CHUNK) || !InWin(cx * CHUNK + CHUNK, cz * CHUNK + CHUNK)) continue;
            int mn = 1 << 30, mx = -(1 << 30), x, z;
            for (z = cz * CHUNK; z <= cz * CHUNK + CHUNK; z++)
                for (x = cx * CHUNK; x <= cx * CHUNK + CHUNK; x++)
                    {
                    int h = iHeight[z * iHS + x] * HS;
                    if (h < mn) mn = h;
                    if (h > mx) mx = h;
                    }
            iChunkMinH[ci] = mn; iChunkMaxH[ci] = mx + 10 * METER;
            for (int n = iChunkObj[ci]; n >= 0; n = iObj[n].next)
                {
                Obj& o = iObj[n];
                if (!(o.pad & 1)) continue;
                o.y = GroundAt(o.x, o.z);
                if (o.type == ST_HOUSE) o.y -= METER / 3;
                }
            }
    if (iLandHook) iLandHook(iLandHookCtx, aX0, aZ0, aX1, aZ1);
    }

int R3D::WaterAt(int aX, int aZ) const
    {
    int cx = aX / (CELL * CHUNK), cz = aZ / (CELL * CHUNK);
    if (cx < 0 || cz < 0 || cx >= iNCX || cz >= iNCZ) return -(1 << 28);
    return WaterOfChunk(cz * iNCX + cx);
    }

// like Flatten, but only ever digs down (stream beds, ditches)
void R3D::Lower(int aX, int aZ, int aR, int aH, int aEdge)
    {
    if (iLandBaked || iWW <= iGX) return;       // (the land file has it; only baking changes the land)
    int r2 = aR + aEdge;
    int x0 = Clamp((aX - r2) / CELL - 1, 0, iGX), x1 = Clamp((aX + r2) / CELL + 1, 0, iGX);
    int z0 = Clamp((aZ - r2) / CELL - 1, 0, iGZ), z1 = Clamp((aZ + r2) / CELL + 1, 0, iGZ);
    int x, z;
    for (z = z0; z <= z1; z++)
        for (x = x0; x <= x1; x++)
            {
            int dx = (x * CELL - aX) >> 4, dz = (z * CELL - aZ) >> 4;
            int d = r3d_isqrt(dx * dx + dz * dz) << 4;
            int t;
            if (d <= aR) t = 256;
            else if (d >= r2) continue;
            else t = 256 - (d - aR) * 256 / (aEdge ? aEdge : 1);
            short& hh = iHeight[z * iHS + x];
            int h = hh * HS;
            int nh = h + ((aH - h) * t >> 8);
            if (nh < h) hh = (short)(nh / HS);
            }
    }

void R3D::Flatten(int aX, int aZ, int aR, int aH, int aEdge)
    {
    if (iLandBaked || iWW <= iGX) return;       // (the land file has it; only baking changes the land)
    int r2 = aR + aEdge;
    int x0 = Clamp((aX - r2) / CELL - 1, 0, iGX), x1 = Clamp((aX + r2) / CELL + 1, 0, iGX);
    int z0 = Clamp((aZ - r2) / CELL - 1, 0, iGZ), z1 = Clamp((aZ + r2) / CELL + 1, 0, iGZ);
    int x, z;
    for (z = z0; z <= z1; z++)
        for (x = x0; x <= x1; x++)
            {
            int dx = (x * CELL - aX) >> 4, dz = (z * CELL - aZ) >> 4;
            int d = r3d_isqrt(dx * dx + dz * dz) << 4;
            int t;
            if (d <= aR) t = 256;
            else if (d >= r2) continue;
            else t = 256 - (d - aR) * 256 / (aEdge ? aEdge : 1);
            short& hh = iHeight[z * iHS + x];
            int h = hh * HS;
            h += (aH - h) * t >> 8;
            hh = (short)(h / HS);
            }
    }

void R3D::AddHeight(int aX, int aZ, int aR, int aH)
    {
    if (iLandBaked || iWW <= iGX) return;       // (the land file has it; only baking changes the land)
    int x0 = Clamp((aX - aR) / CELL - 1, 0, iGX), x1 = Clamp((aX + aR) / CELL + 1, 0, iGX);
    int z0 = Clamp((aZ - aR) / CELL - 1, 0, iGZ), z1 = Clamp((aZ + aR) / CELL + 1, 0, iGZ);
    int x, z;
    int rr = (aR >> 4) * (aR >> 4);
    for (z = z0; z <= z1; z++)
        for (x = x0; x <= x1; x++)
            {
            int dx = (x * CELL - aX) >> 4, dz = (z * CELL - aZ) >> 4;
            int d2 = dx * dx + dz * dz;
            if (d2 >= rr) continue;
            int f = rr < (1 << 22) ? ((rr - d2) << 8) / rr : (rr - d2) / ((rr >> 8) + 1);     // 0..256 (32-bit safe)
            f = f * f >> 8;                                  // smooth
            short& hh = iHeight[z * iHS + x];
            int h = hh * HS + (aH >> 2) * f / 64;
            hh = (short)Clamp(h / HS, -32000, 32000);
            }
    }

void R3D::PaintPath(int aX0, int aZ0, int aX1, int aZ1, int aHalfW, int aMat)
    {
    int dx = aX1 - aX0, dz = aZ1 - aZ0;
    int len = r3d_isqrt((dx >> 8) * (dx >> 8) + (dz >> 8) * (dz >> 8)) << 8;
    if (len <= 0) return;
    int steps = len / (CELL / 2) + 1;
    int i;
    for (i = 0; i <= steps; i++)
        {
        // gentle wiggle so roads are not ruler-straight
        int px = aX0 + dx / steps * i, pz = aZ0 + dz / steps * i;
        int w = (r3d_sin(i * 37) * (CELL / 2)) >> 14;
        PaintDisc(px + w, pz - w / 2, aHalfW, aMat);
        }
    }

void R3D::PaintDisc(int aX, int aZ, int aR, int aMat)
    {
    if (iLandBaked || iWW <= iGX) return;       // (the land file has it; only baking changes the land)
    int x0 = Clamp((aX - aR) / CELL, 0, iGX), x1 = Clamp((aX + aR) / CELL + 1, 0, iGX);
    int z0 = Clamp((aZ - aR) / CELL, 0, iGZ), z1 = Clamp((aZ + aR) / CELL + 1, 0, iGZ);
    int x, z;
    int rr = (aR >> 4) * (aR >> 4);
    for (z = z0; z <= z1; z++)
        for (x = x0; x <= x1; x++)
            {
            int ddx = (x * CELL - aX) >> 4, ddz = (z * CELL - aZ) >> 4;
            if (ddx * ddx + ddz * ddz <= rr) iMat[z * iHS + x] = (u8)((iMat[z * iHS + x] & VM_NOTREE) | VM_PAINT | aMat);
            }
    }

int R3D::AddStatic(int aType, int aX, int aZ, int aYaw, int aScale, int aVar)
    {
    if (iObjCount >= MAXOBJ || aType < 0) return -1;
    Obj& o = iObj[iObjCount];
    o.type = (u16)aType; o.x = aX; o.z = aZ; o.y = 0;
    o.yaw = (short)aYaw; o.scale = (short)aScale; o.var = (u8)aVar; o.flag = 0; o.pad = 0;
    return iObjCount++;
    }

// give back the unused room of a single-frame mesh that was the last thing taken from the arena
// (its arrays sized for the most it could hold): the face arrays move down to follow the vertices it has
void R3D::MeshTrim(Mesh& m)
    {
    if (m.nfr != 1 || m.vn || !m.v || m.nv <= 0 || m.nf <= 0) return;
    u8* end = (u8*)m.uv + ((m.fstride * 12 + 7) & ~7);
    if (end != iArena) return;
    u8* p = (u8*)m.v + ((m.nv * 6 + 7) & ~7);
    int fb = m.nf * 8, nb = m.nf * 6, ub = m.nf * 12;
    u8* nf = p; p += (fb + 7) & ~7;
    u8* nn = p; p += (nb + 7) & ~7;
    u8* nu = p; p += (ub + 7) & ~7;
    // each destination lies at or below its source, so copying forwards is safe
    r3d_memmove(nf, m.f, fb); m.f = (u16*)nf;
    r3d_memmove(nn, m.n, nb); m.n = (short*)nn;
    r3d_memmove(nu, m.uv, ub); m.uv = (u16*)nu;
    m.vstride = m.nv; m.fstride = m.nf;
    iArena = p;
    }

int R3D::RegisterStatic(const StaticDef& aDef)
    {
    if (iNumDefs >= MAXDEFS) return -1;
    if (aDef.lod) MeshTrim(*(Mesh*)aDef.lod);
    if (aDef.mesh) MeshTrim(*(Mesh*)aDef.mesh);
    iDefs[iNumDefs] = aDef;
    iDefs[iNumDefs].boxState = 0;                   // (v0.4.74: its box looked at when first far enough)
    return ST_CUSTOM + iNumDefs++;
    }

static int StaticRadius(const Obj& o)
    {
    switch (o.type)
        {
        case ST_TREE: return (230 * o.scale) >> 8;       // ~0.9 m trunk/lower branches
        case ST_ROCK: return (420 * o.scale) >> 8;
        case ST_STONE: return 240;
        case ST_FIRE: return 260;
        default: return 0;
        }
    }

// trees grow where the ground is grass or forest and nothing was painted or built: decided per vertex from
// hashes when a chunk is cached (FillChunk); here only the ground round what was built is kept clear
void R3D::ScatterTrees(int aSeed)
    {
    if (iLandBaked || iWW <= iGX) return;       // (the land file has it; only baking changes the land)
    (void)aSeed;
    int n;
    for (n = 0; n < iObjCount; n++)
        {
        const Obj& o = iObj[n];
        if (o.type == ST_TREE) continue;
        int lim = (o.type == ST_HOUSE ? 14 : (o.type >= ST_CUSTOM ? 12 : 6)) * METER;
        int x0 = Clamp((o.x - lim) / CELL, 0, iGX), x1 = Clamp((o.x + lim) / CELL + 1, 0, iGX);
        int z0 = Clamp((o.z - lim) / CELL, 0, iGZ), z1 = Clamp((o.z + lim) / CELL + 1, 0, iGZ);
        int x, z;
        for (z = z0; z <= z1; z++)
            for (x = x0; x <= x1; x++)
                {
                int dx = (x * CELL - o.x) >> 8, dz = (z * CELL - o.z) >> 8;
                if (dx * dx + dz * dz < (lim >> 8) * (lim >> 8)) iMat[z * iHS + x] |= VM_NOTREE;
                }
        }
    }

void R3D::FinishWorld()
    {
    int x, z;
    int W = iHS;
    if (iLandBaked || iWW <= iGX) { }           // (the land file has all this in it already)
    else if (iLandLoaded)
        {
        // Skyrim's ground (and paint over it); its cliffs are rock whatever is under them
        for (z = 1; z < iGZ; z++)
            {
            const short* h = iHeight + z * W;
            u8* mt = iMat + z * W;
            for (x = 1; x < iGX; x++)
                {
                int v = mt[x];
                if (v & VM_PAINT) continue;
                int slope = Abs(h[x - 1] - h[x + 1]) + Abs(h[x - W] - h[x + W]);
                if (slope > 7 * METER / HS) mt[x] = (u8)(TM_ROCK | (v & ~VM_MAT));
                }
            }
        }
    else
    for (z = 0; z <= iGZ; z++)
        for (x = 0; x <= iGX; x++)
            {
            int i = z * W + x;
            int hl = iHeight[i - (x > 0 ? 1 : 0)] * HS, hr = iHeight[i + (x < iGX ? 1 : 0)] * HS;
            int hd = iHeight[i - (z > 0 ? W : 0)] * HS, hu = iHeight[i + (z < iGZ ? W : 0)] * HS;
            int nx = hl - hr, nz = hd - hu;
            int h = iHeight[i] * HS;
            int slope = Abs(nx) + Abs(nz);
            u32 hv = Hash(x, z, 9);
            int m;
            int v = iMat[i];
            if (h < WATER + METER / 2) m = TM_SAND;
            else if (h > 54 * METER + (int)(hv & 1023) * 4) m = TM_SNOW;
            else if (slope > 7 * METER) m = TM_ROCK;
            else if (Noise(x, z, 16, 77) > 36000) m = TM_FOREST;
            else m = TM_GRASS;
            if ((v & VM_PAINT) && m != TM_SNOW && h >= WATER) m = v & VM_MAT;
            iMat[i] = (u8)(m | (v & (VM_PAINT | VM_NOTREE)));
            }
    if (!iLandBaked && iWW > iGX) ScatterTrees(77);
    iFarColKey = -1; iFarColRow = 0;
    BakeTerrainLight();
    int cx, cz;
    for (cz = 0; cz < iNCZ; cz++)
        for (cx = 0; cx < iNCX; cx++)
            {
            int mn = 1 << 30, mx = -(1 << 30);
            int full = InWin(cx * CHUNK, cz * CHUNK) && InWin(cx * CHUNK + CHUNK, cz * CHUNK + CHUNK);
            for (z = cz * CHUNK; z <= cz * CHUNK + CHUNK; z += full ? 1 : CSTEP)
                for (x = cx * CHUNK; x <= cx * CHUNK + CHUNK; x += full ? 1 : CSTEP)
                    {
                    int h = (full ? iHeight[z * W + x] : Hgt(x, z)) * HS;
                    if (h < mn) mn = h;
                    if (h > mx) mx = h;
                    }
            if (!full) { mn -= 4 * METER; mx += 4 * METER; }     // (between the coarse points)
            iChunkMinH[cz * iNCX + cx] = mn;
            iChunkMaxH[cz * iNCX + cx] = mx + 10 * METER;   // room for trees / buildings
            iChunkObj[cz * iNCX + cx] = -1;
            iChunkSlot[cz * iNCX + cx] = -1;
            }
    int k;
    for (k = 0; k < TCACHE; k++) iTC[k].chunk = -1;
    int n;
    for (n = 0; n < iObjCount; n++)
        {
        Obj& o = iObj[n];
        if (o.y == 0)                       // (a height set when placing is kept: water on a stream bed)
            {
            o.y = GroundAt(o.x, o.z);
            if (o.type == ST_HOUSE) o.y -= METER / 3;
            o.pad = 1;                      // on the ground: settled again when its land is loaded
            }
        int ci = Clamp(o.z / (CELL * CHUNK), 0, iNCZ - 1) * iNCX + Clamp(o.x / (CELL * CHUNK), 0, iNCX - 1);
        o.next = iChunkObj[ci];
        iChunkObj[ci] = n;
        }
    }

void R3D::RelinkStatics()
    {
    int i, n;
    for (i = 0; i < iNCX * iNCZ; i++) iChunkObj[i] = -1;
    for (n = 0; n < iObjCount; n++)
        {
        Obj& o = iObj[n];
        if (o.y == 0) { o.y = GroundAt(o.x, o.z); if (o.type == ST_HOUSE) o.y -= METER / 3; o.pad = 1; }
        int ci = Clamp(o.z / (CELL * CHUNK), 0, iNCZ - 1) * iNCX + Clamp(o.x / (CELL * CHUNK), 0, iNCX - 1);
        o.next = iChunkObj[ci];
        iChunkObj[ci] = n;
        }
    }

// ---------------------------------------------------------------- the chunk cache
// fill an entry: the vertex colours and light levels for the sun as it is, and (aTrees) the chunk's trees
void R3D::FillChunk(TChunk& t, int aCx, int aCz, int aTrees)
    {
    int gx0 = aCx * CHUNK, gz0 = aCz * CHUNK, i, j;
    for (j = 0; j <= CHUNK; j++)
        for (i = 0; i <= CHUNK; i++)
            {
            int r, g, b, l;
            VertexColour(gx0 + i, gz0 + j, r, g, b, l);
            int k = j * 9 + i;
            t.lit[k * 3] = (u8)r; t.lit[k * 3 + 1] = (u8)g; t.lit[k * 3 + 2] = (u8)b; t.lev[k] = (u8)l;
            }
    t.key = iBakedSun;
    if (!aTrees) return;
    t.ntree = 0;
    if (iLandLoaded)
        {
        // Skyrim's own: a tree or a rock where its land has one (VM_TREE / VM_ROCK), nudged off the grid
        for (j = 0; j < CHUNK; j++)
            for (i = 0; i < CHUNK; i++)
                {
                int x = gx0 + i, z = gz0 + j;
                int v = iMat[z * iHS + x];
                if (!(v & (VM_TREE | VM_ROCK)) || (v & (VM_PAINT | VM_NOTREE))) continue;
                if (t.ntree >= TMAXTREE) return;
                u32 h = Hash(x, z, 77 + 31);
                Obj& o = t.tree[t.ntree];
                o.x = x * CELL + (int)((h >> 16) & 1023) - 512;
                o.z = z * CELL + (int)((h >> 6) & 1023) - 512;
                int nv = ((o.z + CELL / 2) / CELL) * iHS + (o.x + CELL / 2) / CELL;
                if (iMat[nv] & (VM_PAINT | VM_NOTREE)) continue;
                t.ntree++;
                o.y = GroundAt(o.x, o.z);
                o.yaw = (short)((h >> 20) & 1023); o.flag = 0; o.next = -1;
                if (v & VM_TREE)
                    {
                    o.type = ST_TREE; o.scale = (short)(200 + (int)((h >> 11) & 127));
                    o.var = (u8)(((h >> 3) & 3) + ((v & VM_MAT) == TM_FALL ? 4 : 0));
                    }
                else
                    {
                    o.type = ST_ROCK; o.scale = (short)(110 + (int)((h >> 11) & 127)); o.var = (u8)((h >> 3) & 1);
                    o.y -= METER / 4;                   // bedded in
                    }
                }
        return;
        }
    for (j = 0; j < CHUNK; j++)
        for (i = 0; i < CHUNK; i++)
            {
            int x = gx0 + i, z = gz0 + j;
            if (x < 3 || z < 3 || x >= iGX - 3 || z >= iGZ - 3) continue;
            int v = iMat[z * iHS + x];
            if (v & (VM_PAINT | VM_NOTREE)) continue;
            int m = v & VM_MAT;
            int dense = m == TM_FOREST || m == TM_FALL;
            if (!dense && m != TM_GRASS && m != TM_TUNDRA && m != TM_REACH && m != TM_MARSH) continue;
            if (iHeight[z * iHS + x] * HS < WaterOfChunk((z / CHUNK) * iNCX + x / CHUNK) + METER) continue;
            int f = Noise(x, z, 16, 77);
            u32 h = Hash(x, z, 77 + 31);
            int lim = dense ? (f - 30000) * 2 : (f - 42000);
            if (m == TM_TUNDRA) lim = (f - 48000) / 2;       // Whiterun's plains: a lone pine here and there
            if ((int)(h & 0xFFFF) >= lim) continue;
            if (t.ntree >= TMAXTREE) return;
            Obj& o = t.tree[t.ntree++];
            o.x = x * CELL + (int)((h >> 16) & 1023) - 512;
            o.z = z * CELL + (int)((h >> 6) & 1023) - 512;
            // where it really stands: not on a path or by a building (the vertex it was grown from may not be)
            int nv = ((o.z + CELL / 2) / CELL) * iHS + (o.x + CELL / 2) / CELL;
            if (iMat[nv] & (VM_PAINT | VM_NOTREE)) { t.ntree--; continue; }
            o.y = GroundAt(o.x, o.z);
            o.type = ST_TREE; o.yaw = (short)((h >> 20) & 1023); o.scale = (short)(200 + (int)((h >> 11) & 127));
            o.var = (u8)(((h >> 3) & 3) + (m == TM_FALL ? 4 : 0)); o.flag = 0; o.next = -1;
            }
    }

// the cache entry of a chunk, filled if it was not there; re-lit when the sun has moved on (aLight; a few
// chunks a frame, the rest wait a frame or two)
R3D::TChunk* R3D::Cached(int aCx, int aCz, int aLight)
    {
    int ci = aCz * iNCX + aCx;
    if (!InWin(aCx * CHUNK - 1, aCz * CHUNK - 1) || !InWin(aCx * CHUNK + CHUNK + 1, aCz * CHUNK + CHUNK + 1)) return 0;
    int s = iChunkSlot[ci];
    if (s >= 0)
        {
        TChunk& t = iTC[s];
        t.used = iFrameCount;
        if (aLight && t.key != iBakedSun && iRelit < 8) { iRelit++; FillChunk(t, aCx, aCz, 0); }
        return &t;
        }
    // take the next entry not used this frame or the last
    int k;
    for (k = 0; k < TCACHE; k++)
        {
        TChunk& t = iTC[iTCNext];
        int here = iTCNext;
        iTCNext = (iTCNext + 1) % TCACHE;
        if (t.chunk >= 0 && t.used >= iFrameCount - 1) continue;
        if (t.chunk >= 0) iChunkSlot[t.chunk] = -1;
        t.chunk = ci; t.used = iFrameCount;
        iChunkSlot[ci] = (short)here;
        FillChunk(t, aCx, aCz, 1);
        return &t;
        }
    return 0;
    }

int R3D::GroundAt(int aX, int aZ)
    {
    int gx = aX / CELL, gz = aZ / CELL;
    if (gx < 0) gx = 0;
    if (gz < 0) gz = 0;
    if (gx > iGX - 1) gx = iGX - 1;
    if (gz > iGZ - 1) gz = iGZ - 1;
    int fx = Clamp(aX - gx * CELL, 0, CELL), fz = Clamp(aZ - gz * CELL, 0, CELL);
    if (!InWin(gx, gz) || !InWin(gx + 1, gz + 1))
        {
        // far from the player: the coarse land, between its four nearest points
        const int CC = CSTEP * CELL;
        int cx = Clamp(aX / CC, 0, iCW - 2), cz = Clamp(aZ / CC, 0, iCHt - 2);
        int ux = Clamp(aX - cx * CC, 0, CC), uz = Clamp(aZ - cz * CC, 0, CC);
        const short* c = iCH + cz * iCW + cx;
        int a = c[0] * HS, b = c[1] * HS, d = c[iCW] * HS, e = c[iCW + 1] * HS;
        int top = a + (b - a) * (ux >> 4) / (CC >> 4), bot = d + (e - d) * (ux >> 4) / (CC >> 4);
        return top + (bot - top) * (uz >> 4) / (CC >> 4);
        }
    const short* hp = iHeight + gz * iHS + gx;
    int h00 = hp[0] * HS, h10 = hp[1] * HS;
    int h01 = hp[iHS] * HS, h11 = hp[iHS + 1] * HS;
    if (fx >= fz) return h00 + ((h10 - h00) * fx >> 10) + ((h11 - h10) * fz >> 10);
    return h00 + ((h11 - h01) * fx >> 10) + ((h01 - h00) * fz >> 10);
    }

int R3D::MaterialAt(int aX, int aZ)
    {
    int gx = Clamp((aX + CELL / 2) / CELL, 0, iGX), gz = Clamp((aZ + CELL / 2) / CELL, 0, iGZ);
    return MatV(gx, gz) & VM_MAT;
    }

// collision parts of a static (stall posts and counter, well ring ...), in the object's frame
static int PartsHit(const StaticDef& d, const Obj& o, int dx, int dz, int aR, int aY0, int aY1)
    {
    int cs = r3d_cos(o.yaw), sn = r3d_sin(o.yaw);
    int lx = ((dx * cs - dz * sn) >> 14) * 256 / o.scale, lz = ((dx * sn + dz * cs) >> 14) * 256 / o.scale;
    int r = aR * 256 / o.scale;
    const int* p = d.parts;
    int i;
    for (i = 0; i < d.nparts; i++, p += 7)
        {
        int y0 = o.y + (p[5] * o.scale >> 8), y1 = o.y + (p[6] * o.scale >> 8);
        if (aY1 <= y0 || aY0 >= y1) continue;                  // passes over or under it
        if (p[0] == PART_BOX)
            {
            if (lx + r > p[1] && lx - r < p[3] && lz + r > p[2] && lz - r < p[4]) return 1;
            }
        else
            {
            int ri = p[1], ro = p[2] + r;
            if (Abs(lx) >= ro || Abs(lz) >= ro) continue;
            int d2 = (lx >> 2) * (lx >> 2) + (lz >> 2) * (lz >> 2);
            if (d2 >= (ro >> 2) * (ro >> 2)) continue;
            if (r > 48) return 1;                                 // anything wider than a dropped thing: solid
            int rin = ri - r;
            if (d2 > (rin >> 2) * (rin >> 2)) return 1;         // in the wall
            }
        }
    return 0;
    }

int R3D::Blocked(int aX, int aZ, int aR, int aY0, int aY1)
    {
    int cx0 = Clamp((aX - aR - 20 * METER) / (CELL * CHUNK), 0, iNCX - 1), cx1 = Clamp((aX + aR + 20 * METER) / (CELL * CHUNK), 0, iNCX - 1);
    int cz0 = Clamp((aZ - aR - 20 * METER) / (CELL * CHUNK), 0, iNCZ - 1), cz1 = Clamp((aZ + aR + 20 * METER) / (CELL * CHUNK), 0, iNCZ - 1);
    int cx, cz;
    // the trees (grown per chunk in the cache) of the chunks the circle can touch
    int tx0 = Clamp((aX - aR - 2 * METER) / (CELL * CHUNK), 0, iNCX - 1), tx1 = Clamp((aX + aR + 2 * METER) / (CELL * CHUNK), 0, iNCX - 1);
    int tz0 = Clamp((aZ - aR - 2 * METER) / (CELL * CHUNK), 0, iNCZ - 1), tz1 = Clamp((aZ + aR + 2 * METER) / (CELL * CHUNK), 0, iNCZ - 1);
    for (cz = tz0; cz <= tz1; cz++)
        for (cx = tx0; cx <= tx1; cx++)
            {
            const TChunk* t = Cached(cx, cz, 0);
            if (!t) continue;
            int k;
            for (k = 0; k < t->ntree; k++)
                {
                const Obj& o = t->tree[k];
                int dx = aX - o.x, dz = aZ - o.z;
                int r = StaticRadius(o) + aR;
                if (Abs(dx) < r && Abs(dz) < r && (dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4) < (r >> 4) * (r >> 4)) return 1;
                }
            }
    for (cz = cz0; cz <= cz1; cz++)
        for (cx = cx0; cx <= cx1; cx++)
            {
            int i = iChunkObj[cz * iNCX + cx];
            while (i >= 0)
                {
                const Obj& o = iObj[i];
                i = o.next;
                int dx = aX - o.x, dz = aZ - o.z;
                if (o.type >= ST_CUSTOM)
                    {
                    const StaticDef& d = iDefs[o.type - ST_CUSTOM];
                    if (d.parts)
                        {
                        if (Abs(dx) < 16 * METER && Abs(dz) < 16 * METER && PartsHit(d, o, dx, dz, aR, aY0, aY1)) return 1;
                        continue;
                        }
                    // (a thing with a height: only in the way of a body at its height - a barrel on the floor below)
                    if (d.hy && (aY1 < o.y || aY0 > o.y + (d.hy * o.scale >> 8))) continue;
                    if (d.hx)
                        {
                        int cs = r3d_cos(o.yaw), sn = r3d_sin(o.yaw);
                        int lx = (dx * cs - dz * sn) >> 14, lz = (dx * sn + dz * cs) >> 14;
                        int hx = (d.hx * o.scale >> 8) + aR, hz = (d.hz * o.scale >> 8) + aR;
                        if (Abs(lx) < hx && Abs(lz) < hz) return 1;
                        }
                    else if (d.rad)
                        {
                        int r = (d.rad * o.scale >> 8) + aR;
                        if (Abs(dx) < r && Abs(dz) < r && (dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4) < (r >> 4) * (r >> 4)) return 1;
                        }
                    continue;
                    }
                if (o.type == ST_HOUSE)
                    {
                    // longhouse footprint: 8 m x 4.5 m half sizes, along local x
                    int hx = 8 * METER + aR, hz = 9 * METER / 2 + aR;
                    if ((o.yaw & 511) == 256) { int t = hx; hx = hz; hz = t; }
                    if (Abs(dx) < hx && Abs(dz) < hz) return 1;
                    continue;
                    }
                int r = StaticRadius(o) + aR;
                if (Abs(dx) < r && Abs(dz) < r && (dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4) < (r >> 4) * (r >> 4)) return 1;
                }
            }
    return 0;
    }

// ---------------------------------------------------------------- textures, palettes, sprites
void R3D::GenTextures()
    {
    int t, x, y;
    for (t = 0; t < NPAT; t++)
        {
        u8* d = iTex + t * 4096;
        int row = t < NTEX ? t : KPatRow[t - NTEX];
        for (y = 0; y < 64; y++)
            for (x = 0; x < 64; x++)
                {
                u32 h = Hash(x, y, t * 13 + 1);
                int n = (Noise(x + 64, y + 64, 8, t + 20) >> 11) + (int)(h & 7);     // 0..39
                int v;
                switch (t)
                    {
                    case TX_GRASS: v = 10 + (n >> 1) + ((h >> 4) % 5 == 0 ? 8 : 0); break;
                    case TX_ROCK:
                        {
                        // rock: broad lumps of light and shade, a few dark cracks wandering through
                        v = 6 + (Noise(x + 64, y + 64, 16, 40) >> 11) + (int)(h & 3);
                        int cr = (Noise(x + 64, y + 64, 8, 43) >> 10) - 32;          // -32..31
                        if (cr > -2 && cr < 2) v -= 6;
                        break;
                        }
                    case TX_SNOW:  v = 20 + (n >> 3); break;
                    case TX_SAND:  v = 12 + (n >> 2) + (int)(h & 3); break;
                    case TX_FUR:
                        {
                        // animal fur: short strands lying one way, tufts of lighter guard hair, dark under-fur
                        int strand = (int)(Hash(x, y >> 2, 61) & 7), tuft = Noise(x + 64, y + 64, 8, 62) >> 12;
                        v = 9 + strand + tuft + ((y & 3) == 3 ? -4 : 0);
                        if ((Hash(x >> 1, y >> 3, 63) & 15) == 0) v += 8;
                        break;
                        }
                    case TX_NEEDLE:v = 6 + (n >> 1); break;
                    case TX_FURN:
                        {
                        // stained furniture wood: fine vertical grain, a panel line every 32 texels
                        v = 16 + ((y * 3 + (Noise(x, y, 8, 52) >> 12)) & 7) + (int)(Hash(x >> 2, 1, 23) % 4);
                        if ((x & 31) == 0 || (y & 31) == 0) v = 5;
                        break;
                        }
                    case TX_BLOCK:
                        {
                        // dressed stone blocks 16x8 texels, offset every row, dark mortar
                        int row = y >> 3, bx = (x + (row & 1) * 8) & 15, by = y & 7;
                        u32 hb = Hash((x + (row & 1) * 8) >> 4, row, 99);
                        v = 13 + (int)(hb % 9) + (n >> 3);
                        if (bx == 0 || by == 0) v = 2;
                        else if (bx == 1 || by == 1) v -= 4;
                        break;
                        }
                    case TX_DIRT:  v = 12 + (n >> 2) + (int)(h & 3); break;
                    case TX_LOGS:
                        {
                        // horizontal logs 32 texels thick (half a metre at the usual mapping): round shading across
                        // each log, bark grain, dark gaps
                        int ly = y & 31, li = y >> 5;
                        int c = ly - 16; c = c < 0 ? -c : c;          // 0 centre .. 16 edge
                        v = 24 - c + (int)((Hash(x >> 2, li, 71) & 3)) + (n >> 4);
                        if (((x + li * 23) & 63) == 0) v -= 6;        // knot / joint
                        if (ly <= 1 || ly == 31) v = 3;
                        break;
                        }
                    case TX_THATCH:
                        {
                        // bundles of straw running down the roof, rows of tied courses
                        int strand = (int)(Hash(x, (y >> 3) + 7, 31) & 7);
                        v = 14 + strand + (n >> 3);
                        if ((y & 7) == 7) v -= 7;
                        if ((x & 3) == 0) v -= 2;
                        break;
                        }
                    case TX_NORDIC:
                        {
                        // Nordic ruin stone (a 2 m tile on the barrow's walls and floors): two courses of big
                        // pillowed blocks of uneven length, their edges worn and wavy, mottled with damp and moss,
                        // now and then a block carved with a faint swirl
                        int wy = (Noise(x + 64, y + 64, 8, 219) >> 13) - 2;          // wavy joints (-2..1)
                        int yy = (y + wy) & 63, row = yy >> 5;
                        int ox = row ? 23 : 0;
                        int xs = (x + ox + ((Noise(x + 64, y + 64, 8, 223) >> 13) - 2)) & 63;
                        // block boundaries in this course: 0, a, 64 (a differs per course)
                        int a = row ? 26 : 38;
                        int bl = xs < a ? xs : xs - a, bw = xs < a ? a : 64 - a, bi = xs < a ? 0 : 1;
                        int cx = bl - bw / 2, cy = (yy & 31) - 16;
                        int ex = bw / 2 - (cx < 0 ? -cx : cx), ey = 16 - (cy < 0 ? -cy : cy);
                        int e = ex < ey ? ex : ey;
                        u32 hb = Hash(bi, row, 213);
                        v = 16 + (int)(hb % 4) + (n >> 3) - (Noise(x + 64, y + 64, 16, 227) >> 12);   // mottling
                        if (e < 7) v -= (7 - e);                                                    // pillowed
                        if (e <= 0) v = 3 + (int)(h & 1);
                        if (bi == 0 && row == 1 && e > 5)
                            {
                            int r2 = cx * cx + cy * cy * 3 / 2;
                            int r = 0; while (r * r < r2) r++;
                            if (((r + (cx > 0 ? 1 : 0)) % 5) == 0 && r < 13) v -= 3;
                            }
                        break;
                        }
                    case TX_PLANKS:
                        {
                        int py = y & 15;
                        v = 16 + (int)(Hash(y >> 4, (x + (y >> 4) * 37) >> 5, 5) % 7) + ((x * 3 + (Noise(x, y, 16, 51) >> 12)) & 7) / 2;
                        if (py == 0) v = 3;
                        if (((x + (y >> 4) * 37) & 31) == 0) v = 4;
                        break;
                        }
                    case TX_DSTAVE:
                        {
                        // staves: the planks' grain and joints standing up the barrel (a stave every 16 texels)
                        int px = x & 15;
                        v = 14 + (int)(Hash(x >> 4, (y + (x >> 4) * 37) >> 5, 5) % 7) + ((y * 3 + (Noise(x, y, 16, 51) >> 12)) & 7) / 2;
                        if (px == 0) v = 2;
                        break;
                        }
                    case TX_ROPE:
                        {
                        // twisted rope: strands lying slantwise, a dark line between them, fibres
                        int s = (x + y) & 7;
                        v = 13 - (s < 2 ? 7 : 0) + (s == 4 ? 3 : 0) + (int)(h & 3) - 1;
                        break;
                        }
                    case TX_SHINGLE:
                        {
                        int row = y >> 3, sx = (x + (row & 1) * 4) & 7;
                        v = 14 + (int)(Hash((x + (row & 1) * 4) >> 3, row, 9) % 8) + (7 - (y & 7));
                        if (sx == 0 || (y & 7) == 7) v = 4;
                        break;
                        }
                    case TX_FIELD:  v = 12 + (n >> 2) + ((x & 3) == 0 ? -5 : 0); break;
                    case TX_SLATE:
                        {
                        int row = y >> 3, sx = (x + (row & 1) * 6) % 12;
                        v = 12 + (int)(Hash((x + (row & 1) * 6) / 12, row, 19) % 8) + (n >> 4);
                        if (sx == 0 || (y & 7) == 7) v = 3;
                        break;
                        }
                    default:       v = 16 + (n >> 3); break;
                    }
                d[y * 64 + x] = (u8)(row * 16 + (Clamp(v, 0, 31) >> 1));
                }
        }
    }

void R3D::BuildPalettes()
    {
    int i, l, f;
    iPalFogR = iFogR; iPalFogG = iFogG; iPalFogB = iFogB;
    for (i = 0; i < 256; i++)
        {
        int m = i >> 4, d = i & 15;
        int k = 150 + d * 8;                         // 150..270 /256
        iBasePal[i] = Rgb(KMatRGB[m * 3] * k >> 8, KMatRGB[m * 3 + 1] * k >> 8, KMatRGB[m * 3 + 2] * k >> 8);
        }
    for (l = 0; l < 16; l++)
        for (f = 0; f < 8; f++)
            {
            u16* p = iShadePal + (l * 8 + f) * 256;
            int lk = 40 + l * 16;                    // 40..280 /256
            for (i = 0; i < 256; i++)
                {
                int m = i >> 4, d = i & 15;
                int k = (150 + d * 8) * lk >> 8;
                int r = KMatRGB[m * 3] * k >> 8, g = KMatRGB[m * 3 + 1] * k >> 8, b = KMatRGB[m * 3 + 2] * k >> 8;
                r = r * iLightR >> 8; g = g * iLightG >> 8; b = b * iLightB >> 8;
                r = Clamp(r, 0, 255); g = Clamp(g, 0, 255); b = Clamp(b, 0, 255);
                r += (iFogR - r) * f >> 3; g += (iFogG - g) * f >> 3; b += (iFogB - b) * f >> 3;
                p[i] = Rgb(r, g, b);
                }
            }
    BuildSkyShade();
    }

// Skyrim's textures' palette, lit (40..280 /256, as iShadePal) and fogged
static void ShadeOf(const u8* aRgb, u16* aOut, int aFogR, int aFogG, int aFogB, int aLR, int aLG, int aLB);
void R3D::BuildSkyShade()
    {
    if (iLibRgb && iLibShade) ShadeOf(iLibRgb, iLibShade, iFogR, iFogG, iFogB, iLightR, iLightG, iLightB);
    if (iSkyRgb && iSkyShade) ShadeOf(iSkyRgb, iSkyShade, iFogR, iFogG, iFogB, iLightR, iLightG, iLightB);
    }
// (v0.4.74) the light's colour: the palettes made again when it changes
void R3D::SetLightColour(int aR, int aG, int aB)
    {
    if (aR == iLightR && aG == iLightG && aB == iLightB) return;
    iLightR = aR; iLightG = aG; iLightB = aB;
    if (iShadePal) BuildPalettes();
    }
void R3D::SetLibTextures(const u8* aRgb, const u8* aTex, const u8* aTex1, int aN, const u8* aHi, int aHiN, const u8* aKey, const u8* aHi2, int aHi2N)
    {
    iLibRgb = aN > 0 ? aRgb : 0; iLibTex = aN > 0 ? aTex : 0; iLibTex1 = aN > 0 ? aTex1 : 0; iLibN = aN > 0 ? aN : 0;
    iLibHi = aN > 0 ? aHi : 0; iLibHiN = iLibHi ? aHiN : 0; iLibKey = aN > 0 ? aKey : 0;
    iLibHi2 = aN > 0 ? aHi2 : 0; iLibHi2N = iLibHi2 ? aHi2N : 0;
    BuildSkyShade();
    }
static void ShadeOf(const u8* iSkyRgb, u16* iSkyShade, int iFogR, int iFogG, int iFogB, int aLR, int aLG, int aLB)
    {
    int i, l, f;
    for (l = 0; l < 16; l++)
        for (f = 0; f < 8; f++)
            {
            u16* p = iSkyShade + (l * 8 + f) * 256;
            int lk = 40 + l * 16;
            for (i = 0; i < 256; i++)
                {
                int r = iSkyRgb[i * 3] * lk >> 8, g = iSkyRgb[i * 3 + 1] * lk >> 8, b = iSkyRgb[i * 3 + 2] * lk >> 8;
                r = r * aLR >> 8; g = g * aLG >> 8; b = b * aLB >> 8;
                r = Clamp(r, 0, 255); g = Clamp(g, 0, 255); b = Clamp(b, 0, 255);
                r += (iFogR - r) * f >> 3; g += (iFogG - g) * f >> 3; b += (iFogB - b) * f >> 3;
                p[i] = Rgb(r, g, b);
                }
            }
    }

void R3D::SetSkyTextures(const u8* aRgb, const u8* aTex, const u8* aTex1, int aN, const u8* aHi, int aHiN)
    {
    iSkyRgb = aN > 0 ? aRgb : 0; iSkyTex = aN > 0 ? aTex : 0; iSkyTex1 = aN > 0 ? aTex1 : 0; iSkyN = aN > 0 ? aN : 0;
    iSkyHi = aN > 0 && aHi ? aHi : 0; iSkyHiN = iSkyHi ? aHiN : 0;
    BuildSkyShade();
    }

// ---------------------------------------------------------------- camera + transform
void R3D::SetupCamera()
    {
    int sy = r3d_sin(iYaw), cy = r3d_cos(iYaw), sp = r3d_sin(iPitch), cp = r3d_cos(iPitch);
    // X row
    iM[0] = cy; iM[1] = 0; iM[2] = -sy;
    // Y row
    iM[3] = -(sy * sp >> 14); iM[4] = cp; iM[5] = -(cy * sp >> 14);
    // Z row
    iM[6] = sy * cp >> 14; iM[7] = sp; iM[8] = cy * cp >> 14;
    }

inline void R3D::TransformPt(int aX, int aY, int aZ, TV& v)
    {
    int dx = aX - iCamX, dy = aY - iCamY, dz = aZ - iCamZ;
    if (((dx < 0 ? -dx : dx) | (dy < 0 ? -dy : dy) | (dz < 0 ? -dz : dz)) < (1 << 16))
        {
        v.x = (iM[0] * dx + iM[2] * dz) >> 14;
        v.y = (iM[3] * dx + iM[4] * dy + iM[5] * dz) >> 14;
        v.z = (iM[6] * dx + iM[7] * dy + iM[8] * dz) >> 14;
        return;
        }
    // far away (the far land, kilometres off): the same in units of 16 so the products keep in 32 bits
    dx >>= 4; dy >>= 4; dz >>= 4;
    v.x = (iM[0] * dx + iM[2] * dz) >> 10;
    v.y = (iM[3] * dx + iM[4] * dy + iM[5] * dz) >> 10;
    v.z = (iM[6] * dx + iM[7] * dy + iM[8] * dz) >> 10;
    }

inline void R3D::Outcode(TV& v)
    {
    int c = 0;
    int zx = v.z * iHW, zy = v.z * iHH, xx = v.x * iFx, yy = v.y * iFy;
    if (v.z < NEAR) c |= 1;
    if (zx + xx < 0) c |= 2;
    if (zx - xx < 0) c |= 4;
    if (zy - yy < 0) c |= 8;
    if (zy + yy < 0) c |= 16;
    // off the screen to a side but within a band round it (and not too far): projected all the same, its triangles
    // drawn without cutting them up (the rasteriser keeps to the screen); beyond that band, or nearer than NEAR: 32,
    // cut against the planes (ClipAndDraw)
    if (c & 1) c |= 32;
    else if (c)
        {
        if (v.z >= (1 << 20)) c |= 32;
        else
            {
            int gx = v.z * (iHW + GUARD), gy = v.z * (iHH + GUARD);
            if (gx + xx < 0 || gx - xx < 0 || gy - yy < 0 || gy + yy < 0) c |= 32;
            else { v.code = c; Project(v); return; }
            }
        }
    v.code = c;
    }

inline void R3D::Project(TV& v)
    {
    int z = v.z < NEAR ? NEAR : v.z;
    if (z <= 65535)
        {
        int r = iRecip[z];
        v.sx = iCx16 + MulHi30(v.x * iFx16, r);
        v.sy = iCy16 - MulHi30(v.y * iFy16, r);
        int w = r >> 8;
        v.w = w > 65535 ? 65535 : w;
        return;
        }
    // beyond 256 m: scale x, y, z down together (the screen position is unchanged), w = 2^22 / z
    int s = 0;
    while ((z >> s) > 65535) s++;
    int r = iRecip[z >> s];
    v.sx = iCx16 + MulHi30((v.x >> s) * iFx16, r);
    v.sy = iCy16 - MulHi30((v.y >> s) * iFy16, r);
    int w = (r >> s) >> 8;
    v.w = w < 1 ? 1 : w;
    }

void R3D::FogColour(int aZ, int aR, int aG, int aB, TV& v)
    {
    aR = aR * iLight >> 8; aG = aG * iLight >> 8; aB = aB * iLight >> 8;
    int f = FogF(aZ);
    v.a[0] = (aR << 8) + (iFogR - aR) * f;
    v.a[1] = (aG << 8) + (iFogG - aG) * f;
    v.a[2] = (aB << 8) + (iFogB - aB) * f;
    }

// ---------------------------------------------------------------- rasteriser
struct Edge
    {
    int x, dx, y, yend;
    int a[4], da[4];
    };


static void SetupEdge(Edge& e, const TV* va, const TV* vb, int na, const int* rc)
    {
    e.y = (va->sy + 7) >> 4;
    e.yend = (vb->sy + 7) >> 4;
    int dy = vb->sy - va->sy;
    if (dy <= 0 || e.yend <= e.y) { e.yend = e.y; e.dx = 0; e.x = va->sx << 12; return; }
    int rdy = dy < 65536 ? rc[dy] : (1 << 30) / dy;
    e.dx = MulHi30((vb->sx - va->sx) << 16, rdy);
    int sub = (e.y << 4) + 8 - va->sy;
    e.x = (va->sx << 12) + (e.dx >> 4) * sub;
    e.a[1] = e.a[2] = e.a[3] = e.da[1] = e.da[2] = e.da[3] = 0;
    int k;
    for (k = 0; k < na; k++)
        {
        int aa = k == 0 ? va->w : va->a[k - 1];
        int ab = k == 0 ? vb->w : vb->a[k - 1];
        e.da[k] = MulHi30((ab - aa) << 12, rdy);
        e.a[k] = (aa << 8) + (e.da[k] >> 4) * sub;
        }
    }

// a flat triangle: Raster's arithmetic for depth only (the same pixels), its edges and the span state set up
// without the colour channels
void R3D::RasterFlat(TV* v0, TV* v1, TV* v2)
    {
    // (the state's fixed parts kept from triangle to triangle: set again only when the screen changes)
    HalfState& hs = iFlatHS;
    if (hs.W != iW || hs.cstride != iCStride * 2)
        {
        hs.a1 = hs.a2 = hs.a3 = hs.da1 = hs.da2 = hs.da3 = 0;
        hs.W = iW; hs.xlo = 0; hs.xhi = iW;
        hs.cstride = iCStride * 2;
        }
#ifdef USE_ASM
    if (!iDouble && !(iSkip & 4096))
        {
        // (v0.4.43: in ARM code - the C below kept for the triangles it leaves: 8 rows or more against the occlusion map)
        FlatCtx& c = iFlatCtx;
        c.rc = iRecip; c.H = iH; c.cb = iCB; c.cstride = iCStride; c.zb = iZB; c.W = iW; c.occ = iOccOn; c.col = iFlatCol;
        c.tris = 0; c.hs = &hs;
        if (tri_flat1(v0, v1, v2, &c) != 2) { iTris += c.tris; return; }
        }
#endif
    // (v0.4.43: without the arrays of edges - kept on the stack, read back and forth; and a triangle that crosses no
    //  row's middle left at once: most of the small ones)
    TV* t;
    if (v1->sy < v0->sy) { t = v0; v0 = v1; v1 = t; }
    if (v2->sy < v0->sy) { t = v0; v0 = v2; v2 = t; }
    if (v2->sy < v1->sy) { t = v1; v1 = v2; v2 = t; }
    int y0 = v0->sy, y2 = v2->sy;
    int ys = (y0 + 7) >> 4, ye = (y2 + 7) >> 4;
    int H = iH;
    if ((ys < 0 ? 0 : ys) >= (ye > H ? H : ye)) return;
    const int* rc = iRecip;
    int dy02 = y2 - y0;
    int r02 = dy02 < 65536 ? rc[dy02] : (1 << 30) / dy02;
    int tt = MulHi30((v1->sy - y0) << 14, r02);
    int xl = v0->sx + (((v2->sx - v0->sx) * tt) >> 14);
    int width = v1->sx - xl;
    if (width == 0) return;
    int aw = width < 0 ? -width : width;
    int rw = aw < 65536 ? rc[aw] : (1 << 30) / aw;
    if (width < 0) rw = -rw;
    int along = v0->w + ((((v2->w - v0->w) >> 2) * tt) >> 12);
    int dw = MulHi30((v1->w - along) << 12, rw);
    iTris++;
#ifdef R3D_STATS
    { extern int g_triId, g_triOcc, g_triOccN; g_triId++; g_triOcc = iOccOn; g_triOccN += iOccOn; }
#endif
    hs.dws = dw >> 8; hs.dw = dw;
    hs.x.col = iFlatCol;
    int wmax = v0->w > v1->w ? v0->w : v1->w;
    if (v2->w > wmax) wmax = v2->w;
    // the long edge (v0 to v2): x a row (16.16), and w (8 more bits) when it is the left one
    int ldx = MulHi30((v2->sx - v0->sx) << 16, r02);
    int lsub = (ys << 4) + 8 - y0;
    int lx = (v0->sx << 12) + (ldx >> 4) * lsub, lw = 0, ldw = 0;
    if (width > 0) { ldw = MulHi30((v2->w - v0->w) << 12, r02); lw = (v0->w << 8) + (ldw >> 4) * lsub; }
    int ly = ys;
    int ym = (v1->sy + 7) >> 4;
    int part;
    for (part = 0; part < 2; part++)
        {
        const TV* a = part ? v1 : v0;
        const TV* b = part ? v2 : v1;
        int ra = part ? ym : ys, rb = part ? ye : ym;
        int pa = ra < 0 ? 0 : ra, pb = rb > H ? H : rb;
        if (pa >= pb) continue;
        int dy = b->sy - a->sy;
        int r = dy < 65536 ? rc[dy] : (1 << 30) / dy;
        int sdx = MulHi30((b->sx - a->sx) << 16, r);
        int ssub = (ra << 4) + 8 - a->sy;
        int ns = pa - ra, nl = pa - ly;
        lx += ldx * nl; lw += ldw * nl;
        int sx = (a->sx << 12) + (sdx >> 4) * ssub + sdx * ns;
        if (width > 0) { hs.lx = lx; hs.ldx = ldx; hs.a0 = lw; hs.da0 = ldw; hs.rx = sx; hs.rdx = sdx; }
        else
            {
            int sdw = MulHi30((b->w - a->w) << 12, r);
            hs.lx = sx; hs.ldx = sdx; hs.a0 = (a->w << 8) + (sdw >> 4) * ssub + sdw * ns; hs.da0 = sdw;
            hs.rx = lx; hs.rdx = ldx;
            }
        hs.crow = iCB + pa * iCStride; hs.zrow = iZB + pa * hs.W; hs.count = pb - pa;
#ifdef USE_ASM
        if (!iOccOn && !iDouble) half_flat1(&hs); else
#endif
        RunBand(K_FLAT, 0, hs, pa, wmax);
        if (width > 0) { lx = hs.lx; lw = hs.a0; } else lx = hs.rx;
        ly = pb;
        }
    }

// (v0.4.71) a textured triangle: Raster's arithmetic (the same pixels), the edges worked out as RasterFlat does - the
// long one once, each short one as its half comes - with w, u, v by name and nothing copied
void R3D::RasterTex(TV* v0, TV* v1, TV* v2)
    {
#ifdef USE_ASM
    if (!(iSkip & 16384))
        {
        // (v0.4.73) set up in ARM code (tex_tri): both halves at once - the edges' values are straight lines down the
        // rows, so the second half's start is the first's carried on, as the walkers would leave it
        TexTri& T = iTexTri;
        T.v[0] = v0; T.v[1] = v1; T.v[2] = v2; T.rc = iRecip; T.H = iH;
        T.W = iW; T.cstride = iCStride * 2; T.pal = iCurPal; T.tex = iCurTex; T.cb = iCB; T.zb = iZB; T.cstrideE = iCStride;
        int r = tex_tri(&T);
        if (r == 0) return;
        if (r == 1)
            {
            iTris++;
#ifdef R3D_STATS
    { extern int g_triId, g_triOcc, g_triOccN; g_triId++; g_triOcc = iOccOn; g_triOccN += iOccOn; }
#endif
            if (T.pa0 < T.pb0) RunBand(K_TEX, T.persp, T.hs[0], T.pa0, T.wmax);
            if (T.pa1 < T.pb1) RunBand(K_TEX, T.persp, T.hs[1], T.pa1, T.wmax);
            return;
            }
        }
#endif
    TV* t;
    if (v1->sy < v0->sy) { t = v0; v0 = v1; v1 = t; }
    if (v2->sy < v0->sy) { t = v0; v0 = v2; v2 = t; }
    if (v2->sy < v1->sy) { t = v1; v1 = v2; v2 = t; }
    int y0 = v0->sy, y2 = v2->sy;
    if (y2 <= y0) return;
    int w0 = v0->w, w1 = v1->w, w2 = v2->w;
    int u0 = v0->a[0], u1 = v1->a[0], u2 = v2->a[0];
    int q0 = v0->a[1], q1 = v1->a[1], q2 = v2->a[1];
    int wmax = w0 > w1 ? w0 : w1, wmin = w0 < w1 ? w0 : w1;
    if (w2 > wmax) wmax = w2;
    if (w2 < wmin) wmin = w2;
    int persp = 0;
    if (wmax > (1 << 22) / (PERSP_M * METER) && wmin > 0 && wmax - wmin > (wmax >> PERSP_SH))
        {
        persp = 1;
        u0 = MulShift(u0, w0, 13); u1 = MulShift(u1, w1, 13); u2 = MulShift(u2, w2, 13);
        q0 = MulShift(q0, w0, 13); q1 = MulShift(q1, w1, 13); q2 = MulShift(q2, w2, 13);
        }
    const int* rc = iRecip;
    int dy02 = y2 - y0;
    int r02 = dy02 < 65536 ? rc[dy02] : (1 << 30) / dy02;
    int tt = MulHi30((v1->sy - y0) << 14, r02);
    int width = v1->sx - (v0->sx + (((v2->sx - v0->sx) * tt) >> 14));
    if (width == 0) return;
    int aw = width < 0 ? -width : width;
    int rw = aw < 65536 ? rc[aw] : (1 << 30) / aw;
    if (width < 0) rw = -rw;
    HalfState& hs = iTexHS;
    {
    int dw = MulHi30((w1 - (w0 + ((((w2 - w0) >> 2) * tt) >> 12))) << 12, rw);
    int du = MulHi30((u1 - (u0 + ((((u2 - u0) >> 2) * tt) >> 12))) << 12, rw);
    int dv = MulHi30((q1 - (q0 + ((((q2 - q0) >> 2) * tt) >> 12))) << 12, rw);
    hs.dws = dw >> 8; hs.d1s = du >> 8; hs.d2s = dv >> 8; hs.d3s = 0;
    hs.dw = dw; hs.d1 = du; hs.d2 = dv; hs.d3 = 0;
    }
    iTris++;
#ifdef R3D_STATS
    { extern int g_triId, g_triOcc, g_triOccN; g_triId++; g_triOcc = iOccOn; g_triOccN += iOccOn; }
#endif
    hs.W = iW; hs.xlo = 0; hs.xhi = iW;
    hs.cstride = iCStride * 2;
    hs.pal = iCurPal; hs.x.tex = iCurTex;
    hs.a3 = hs.da3 = 0;
    int ys = (y0 + 7) >> 4, ye = (y2 + 7) >> 4, ym = (v1->sy + 7) >> 4;
    int H = iH;
    // the long edge (v0 to v2): x a row, and w, u, v when it is the left one
    int ldx = MulHi30((v2->sx - v0->sx) << 16, r02);
    int lsub = (ys << 4) + 8 - y0;
    int lx = (v0->sx << 12) + (ldx >> 4) * lsub, lw = 0, ldw = 0, lu = 0, ldu = 0, lv = 0, ldv = 0;
    if (width > 0)
        {
        ldw = MulHi30((w2 - w0) << 12, r02); lw = (w0 << 8) + (ldw >> 4) * lsub;
        ldu = MulHi30((u2 - u0) << 12, r02); lu = (u0 << 8) + (ldu >> 4) * lsub;
        ldv = MulHi30((q2 - q0) << 12, r02); lv = (q0 << 8) + (ldv >> 4) * lsub;
        }
    int ly = ys, part;
    for (part = 0; part < 2; part++)
        {
        const TV* a = part ? v1 : v0;
        const TV* b = part ? v2 : v1;
        int ra = part ? ym : ys, rb = part ? ye : ym;
        int pa = ra < 0 ? 0 : ra, pb = rb > H ? H : rb;
        if (pa >= pb) continue;
        int dy = b->sy - a->sy;
        int r = dy < 65536 ? rc[dy] : (1 << 30) / dy;
        int sdx = MulHi30((b->sx - a->sx) << 16, r);
        int ssub = (ra << 4) + 8 - a->sy;
        int ns = pa - ra, nl = pa - ly;
        lx += ldx * nl;
        int sx = (a->sx << 12) + (sdx >> 4) * ssub + sdx * ns;
        if (width > 0)
            {
            lw += ldw * nl; lu += ldu * nl; lv += ldv * nl;
            hs.lx = lx; hs.ldx = ldx; hs.rx = sx; hs.rdx = sdx;
            hs.a0 = lw; hs.da0 = ldw; hs.a1 = lu; hs.da1 = ldu; hs.a2 = lv; hs.da2 = ldv;
            }
        else
            {
            int aw_ = part ? w1 : w0, bw = part ? w2 : w1, au = part ? u1 : u0, bu = part ? u2 : u1, av = part ? q1 : q0, bv = part ? q2 : q1;
            int sdw = MulHi30((bw - aw_) << 12, r), sdu = MulHi30((bu - au) << 12, r), sdv = MulHi30((bv - av) << 12, r);
            hs.lx = sx; hs.ldx = sdx;
            hs.a0 = (aw_ << 8) + (sdw >> 4) * ssub + sdw * ns; hs.da0 = sdw;
            hs.a1 = (au << 8) + (sdu >> 4) * ssub + sdu * ns; hs.da1 = sdu;
            hs.a2 = (av << 8) + (sdv >> 4) * ssub + sdv * ns; hs.da2 = sdv;
            hs.rx = lx; hs.rdx = ldx;
            }
        hs.crow = iCB + pa * iCStride; hs.zrow = iZB + pa * hs.W; hs.count = pb - pa;
        RunBand(K_TEX, persp, hs, pa, wmax);
        if (width > 0) { lx = hs.lx; lw = hs.a0; lu = hs.a1; lv = hs.a2; } else lx = hs.rx;
        ly = pb;
        }
    }

void R3D::Raster(TV* v0, TV* v1, TV* v2, int aKind)
    {
    if (aKind == K_FLAT) { RasterFlat(v0, v1, v2); return; }
    if (aKind == K_TEX && !(iSkip & 8192)) { RasterTex(v0, v1, v2); return; }
    TV* t;
    if (v1->sy < v0->sy) { t = v0; v0 = v1; v1 = t; }
    if (v2->sy < v0->sy) { t = v0; v0 = v2; v2 = t; }
    if (v2->sy < v1->sy) { t = v1; v1 = v2; v2 = t; }
    int y0 = v0->sy, y2 = v2->sy;
    if (y2 <= y0) return;
    // textured triangles that recede: interpolate u*w, v*w (linear on screen) and divide per scanline
    int persp = 0;
    TV q[3];
    if (aKind == K_TEX)
        {
        int wa = v0->w, wb = v1->w, wc = v2->w;
        int wmax = wa > wb ? wa : wb, wmin = wa < wb ? wa : wb;
        if (wc > wmax) wmax = wc;
        if (wc < wmin) wmin = wc;
        if (wmax > (1 << 22) / (PERSP_M * METER) && wmin > 0 && wmax - wmin > (wmax >> PERSP_SH))     // within 20 m, receding by an eighth or more
            {
            persp = 1;
            q[0] = *v0; q[1] = *v1; q[2] = *v2;
            int k2;
            for (k2 = 0; k2 < 3; k2++)
                {
                q[k2].a[0] = MulShift(q[k2].a[0], q[k2].w, 13);
                q[k2].a[1] = MulShift(q[k2].a[1], q[k2].w, 13);
                }
            v0 = &q[0]; v1 = &q[1]; v2 = &q[2];
            }
        }
    int na = aKind == K_FLAT ? 1 : (aKind == K_GOURAUD ? 4 : 3);
    int dy02 = y2 - y0;
    int tt = MulHi30((v1->sy - y0) << 14, dy02 < 65536 ? iRecip[dy02] : (1 << 30) / dy02);
    int xl = v0->sx + (((v2->sx - v0->sx) * tt) >> 14);
    int width = v1->sx - xl;
    if (width == 0) return;
    int aw = width < 0 ? -width : width;
    int rw = aw < 65536 ? iRecip[aw] : (1 << 30) / aw;
    if (width < 0) rw = -rw;
    int dAdx[4];
    dAdx[1] = dAdx[2] = dAdx[3] = 0;
    int k;
    for (k = 0; k < na; k++)
        {
        int a0 = k == 0 ? v0->w : v0->a[k - 1];
        int a1 = k == 0 ? v1->w : v1->a[k - 1];
        int a2 = k == 0 ? v2->w : v2->a[k - 1];
        int along = a0 + ((((a2 - a0) >> 2) * tt) >> 12);          // (>> 2 first: perspective u*w can pass 2^17)
        dAdx[k] = MulHi30((a1 - along) << 12, rw);
        }
    iTris++;
#ifdef R3D_STATS
    { extern int g_triId, g_triOcc, g_triOccN; g_triId++; g_triOcc = iOccOn; g_triOccN += iOccOn; }
#endif
    Edge L1, L2, R1, R2;
    Edge* L[2]; Edge* R[2];
    if (width > 0)
        {
        SetupEdge(L1, v0, v2, na, iRecip); L[0] = &L1; L[1] = &L1;
        SetupEdge(R1, v0, v1, 0, iRecip); SetupEdge(R2, v1, v2, 0, iRecip); R[0] = &R1; R[1] = &R2;
        }
    else
        {
        SetupEdge(L1, v0, v1, na, iRecip); SetupEdge(L2, v1, v2, na, iRecip); L[0] = &L1; L[1] = &L2;
        SetupEdge(R1, v0, v2, 0, iRecip); R[0] = &R1; R[1] = &R1;
        }
    int ymid = (v1->sy + 7) >> 4;
    int ystart = (y0 + 7) >> 4, yend = (y2 + 7) >> 4;
    int W = iW, H = iH;
    int dw = dAdx[0], d1 = dAdx[1], d2 = dAdx[2], d3 = dAdx[3];
    HalfState hs;
    hs.dws = dw >> 8; hs.d1s = d1 >> 8; hs.d2s = d2 >> 8; hs.d3s = d3 >> 8;
    hs.dw = dw; hs.d1 = d1; hs.d2 = d2; hs.d3 = d3;
    hs.W = W; hs.xlo = 0; hs.xhi = W;
    hs.cstride = iCStride * 2;
    hs.pal = iCurPal;
    int wmax = v0->w > v1->w ? v0->w : v1->w;
    if (v2->w > wmax) wmax = v2->w;
    if (aKind == K_FLAT) hs.x.col = iFlatCol;
    else hs.x.tex = iCurTex;
    int part;
    for (part = 0; part < 2; part++)
        {
        Edge* le = L[part];
        Edge* re = R[part];
        int ya = part == 0 ? ystart : ymid;
        int yb = part == 0 ? ymid : yend;
        if (ya < 0) ya = 0;
        if (yb > H) yb = H;
        if (ya >= yb) continue;
        if (le->y < ya)
            {
            int n = ya - le->y;
            le->x += le->dx * n;
            for (k = 0; k < na; k++) le->a[k] += le->da[k] * n;
            le->y = ya;
            }
        if (re->y < ya) { re->x += re->dx * (ya - re->y); re->y = ya; }
        hs.lx = le->x; hs.ldx = le->dx; hs.rx = re->x; hs.rdx = re->dx;
        hs.a0 = le->a[0]; hs.da0 = le->da[0]; hs.a1 = le->a[1]; hs.da1 = le->da[1];
        hs.a2 = le->a[2]; hs.da2 = le->da[2]; hs.a3 = le->a[3]; hs.da3 = le->da[3];
        hs.crow = iCB + ya * iCStride; hs.zrow = iZB + ya * W; hs.count = yb - ya;
#ifdef USE_ASM
        if (!iOccOn && !iDouble && !persp && aKind == K_GOURAUD) half_gouraud1(&hs); else
#endif
        RunBand(aKind, persp, hs, ya, wmax);
        le->x = hs.lx; re->x = hs.rx;
        le->a[0] = hs.a0; le->a[1] = hs.a1; le->a[2] = hs.a2; le->a[3] = hs.a3;
        le->y = yb; re->y = yb;
        }
    }

// (v0.4.43) a half triangle in bands of the occlusion map's rows: where the blocks a band's runs cross all hold
// something no farther than the triangle's nearest corner (aWmax), nothing of it can show - left out; the runs cut
// to the blocks between the first and the last that can. (The land behind the houses: most of its pixels)
void R3D::RunBand(int aKind, int aPersp, HalfState& h, int aY, int aWmax)
    {
    int rows = h.count;
    if (!iOccOn || iDouble || (iSkip & 2048) || rows < OCCB) { if (aPersp) RunHalfPersp(h); else RunHalf(aKind, h); return; }
    int y = aY, W = iW;
    int pend = 0;            // rows seen whole, not yet drawn (drawn together: one call)
    while (rows > 0)
        {
        int n = OCCB - ((y + pend) & (OCCB - 1));
        if (n > rows) n = rows;
        // (the band's runs: from the state pend rows on)
        int lx = h.lx + h.ldx * pend, rx = h.rx + h.rdx * pend;
        int l1 = lx + h.ldx * (n - 1), r1 = rx + h.rdx * (n - 1);
        int xa = ((lx < l1 ? lx : l1) + 0x7FFF) >> 16, xb = ((rx > r1 ? rx : r1) + 0x7FFF) >> 16;
        if (xa < 0) xa = 0;
        if (xb > W) xb = W;
        int b0 = 1, b1 = 0, c0 = 0, c1 = 0;
        if (xa < xb)
            {
            const u16* m = iOccMap + ((y + pend) >> 3) * OCCW;
            b0 = c0 = xa >> 3; b1 = c1 = (xb - 1) >> 3;
            while (b0 <= b1 && m[b0] >= aWmax) b0++;
            while (b1 >= b0 && m[b1] >= aWmax) b1--;
            }
        if (b0 <= b1 && b0 == c0 && b1 == c1) { pend += n; rows -= n; continue; }     // (all of it may show: on with the rest)
        if (pend) { h.count = pend; if (aPersp) RunHalfPersp(h); else RunHalf(aKind, h); y += pend; pend = 0; }
        if (b0 <= b1)
            {
            h.xlo = b0 << 3; h.xhi = (b1 + 1) << 3 < W ? (b1 + 1) << 3 : W;
            h.count = n;
            if (aPersp) RunHalfPersp(h); else RunHalf(aKind, h);
            h.xlo = 0; h.xhi = W;
            }
        else
            {
            // (all hidden, or nothing there: the state stepped on as the walker would)
            if (xa < xb) iBandSkip += n;
            h.lx += h.ldx * n; h.rx += h.rdx * n;
            h.a0 += h.da0 * n; h.a1 += h.da1 * n; h.a2 += h.da2 * n; h.a3 += h.da3 * n;
            h.crow = (u16*)((u8*)h.crow + h.cstride * n); h.zrow += W * n;
            }
        y += n; rows -= n;
        }
    if (pend) { h.count = pend; if (aPersp) RunHalfPersp(h); else RunHalf(aKind, h); }
    h.count = 0;
    }

// perspective-correct textured half: per row, recover u and v at both span ends from u*w, v*w and w,
// then let the span walker step affinely between them
void R3D::RunHalfPersp(HalfState& h)
    {
#ifdef USE_ASM
    if (!iDouble && !(iSkip & 8192) && PERSP_LIM == 2 && PERSP_SEG == 32)
        {
        // (v0.4.73) the rows in ARM code too: each row's set-up and its runs, without a call a row
        PerspCtx c;
        c.rc = iRecip;
        switch (iTexQ)
            {
            case 1: persp_rowsq1(&h, &c); break;
            case 2: persp_rowsk1(&h, &c); break;
            case 3: persp_rowsh1(&h, &c); break;
            case 4: persp_rowskq1(&h, &c); break;
            case 5: persp_rowskh1(&h, &c); break;
            default: persp_rows1(&h, &c); break;
            }
        h.count = 0;
        return;
        }
#endif
    // premultiplied u*w, v*w: per-pixel steps and the left edge's value and step per row.
    // Each row is cut into runs of 16 pixels with u, v exact at both ends of every run, so walls
    // seen at a slant (depth changing along the row) do not swim; the affine walker fills the runs.
    int D1 = h.d1, D2 = h.d2, D1s = h.d1s, D2s = h.d2s;
    int pu = h.a1, pv = h.a2, dpu = h.da1, dpv = h.da2;
    int ldx = h.ldx, rdx = h.rdx, da0 = h.da0, dw = h.dw, dws = h.dws, cst = h.cstride, W = h.W;
    int rows = h.count;
    h.ldx = 0; h.rdx = 0; h.da0 = 0; h.da1 = 0; h.da2 = 0; h.cstride = 0;
    while (rows-- > 0)
        {
        int lx = h.lx, rx = h.rx, a0 = h.a0;
        u16* crow = h.crow; u16* zrow = h.zrow;
        int xs = (lx + 0x7FFF) >> 16, xe = (rx + 0x7FFF) >> 16;
        if (xs < h.xlo) xs = h.xlo;
        if (xe > h.xhi) xe = h.xhi;
        int n = xe - xs;
        if (n > 0)
            {
            int sub = ((xs << 16) + 0x8000 - lx) >> 8;
            int wS = a0 + dws * sub;
            int u0 = pu + D1s * sub, v0 = pv + D2s * sub;
            int rA = iRecip[Clamp(wS >> 8, 1, 65535)];
            int uA = MulShift(u0, rA, 17), vA = MulShift(v0, rA, 17);
            // rows whose depth hardly changes (floors seen ahead, walls face on) need no cuts
            int adw = dw < 0 ? -dw : dw;
            // run length: the depth may change by up to ~1/4 along a run (the error stays about a texel of
            // Skyrim's small pictures); rows that hardly recede are not cut at all
            int lim = wS >> PERSP_LIM, seg = PERSP_SEG;
            if (adw * n < lim) seg = n;
            else if ((adw << 6) < lim) seg = 64;
            else if ((adw << 5) < lim) seg = 32;
#ifdef USE_ASM
            if (!iDouble && !(iSkip & 8192))
                {
                // (v0.4.71) the row's runs in ARM code
                PerspCtx c;
                c.cp = crow + xs; c.zp = zrow + xs; c.n = n; c.wS = wS; c.dw = dw; c.u0 = u0; c.v0 = v0; c.D1 = D1; c.D2 = D2;
                c.seg = seg; c.rc = iRecip; c.tex = h.x.tex; c.pal = h.pal; c.uA = uA; c.vA = vA;
                switch (iTexQ)
                    {
                    case 1: persp_texq1(&c); break;
                    case 2: persp_texk1(&c); break;
                    case 3: persp_texh1(&c); break;
                    case 4: persp_texkq1(&c); break;
                    case 5: persp_texkh1(&c); break;
                    default: persp_tex1(&c); break;
                    }
                n = 0;
                }
#endif
            int k = 0;
            while (k < n)
                {
                int m = n - k;
                if (m > seg) m = seg;
                int kn = k + m;
                int rB = iRecip[Clamp((wS + dw * kn) >> 8, 1, 65535)];
                int uB = MulShift(u0 + D1 * kn, rB, 17), vB = MulShift(v0 + D2 * kn, rB, 17);
                int rm = iRecip[m];
                int du = MulShift(uB - uA, rm, 30), dv = MulShift(vB - vA, rm, 30);
                int x = xs + k;
                h.lx = (x << 16) - 0x7FFF; h.rx = ((x + m) << 16) - 0x7FFF;
                h.a0 = wS + dw * k - dws * 255;
                h.d1 = du; h.d1s = du >> 8; h.a1 = uA - h.d1s * 255;
                h.d2 = dv; h.d2s = dv >> 8; h.a2 = vA - h.d2s * 255;
                h.crow = crow; h.zrow = zrow; h.count = 1;
                RunHalf(K_TEX, h);
                uA = uB; vA = vB; k = kn;
                }
            }
        h.lx = lx + ldx; h.rx = rx + rdx; h.a0 = a0 + da0;
        h.crow = (u16*)((u8*)crow + cst); h.zrow = zrow + W;
        pu += dpu; pv += dpv;
        }
    // leave the state as the caller's next half expects it: premultiplied left-edge values and the
    // per-pixel steps unchanged (the second half of the triangle reuses them)
    h.ldx = ldx; h.rdx = rdx; h.da0 = da0; h.cstride = cst;
    h.a1 = pu; h.a2 = pv; h.da1 = dpu; h.da2 = dpv;
    h.d1 = D1; h.d2 = D2; h.d1s = D1s; h.d2s = D2s;
    h.count = 0;
    }

#ifdef R3D_STATS
#define R3D_VIS { extern int g_triId, g_triVisMark, g_triVis, g_triOcc, g_triVisOcc; if (g_triVisMark != g_triId) { g_triVisMark = g_triId; g_triVis++; g_triVisOcc += g_triOcc; } }
#else
#define R3D_VIS
#endif
void R3D::RunHalf(int aKind, HalfState& h)
    {
#ifdef USE_ASM
    if (iDouble)
        {
        if (aKind == K_GOURAUD) half_gouraud2(&h);
        else if (aKind == K_TEX) { if (iTexQ == 2) half_texk2(&h); else if (iTexQ) half_texq2(&h); else half_tex2(&h); }
        else half_flat2(&h);
        }
    else
        {
        if (aKind == K_GOURAUD) half_gouraud1(&h);
        else if (aKind == K_TEX)
            {
            switch (iTexQ)
                {
                case 1: half_texq1(&h); break;
                case 2: half_texk1(&h); break;
                case 3: half_texh1(&h); break;
                case 4: half_texkq1(&h); break;
                case 5: half_texkh1(&h); break;
                default: half_tex1(&h); break;
                }
            }
        else half_flat1(&h);
        }
#else
    // C reference of the assembly walkers (same arithmetic)
    while (h.count-- > 0)
        {
        int lx = h.lx, rx = h.rx;
        h.lx += h.ldx; h.rx += h.rdx;
        int xs = (lx + 0x7FFF) >> 16, xe = (rx + 0x7FFF) >> 16;
        if (xs < h.xlo) xs = h.xlo;
        if (xe > h.xhi) xe = h.xhi;
        int n = xe - xs;
        if (n > 0)
            {
#ifdef R3D_STATS
            iPixels += n; if (aKind == K_FLAT) g_r3dPixFlat += n;
            g_r3dRows[aKind == K_FLAT ? 0 : aKind == K_GOURAUD ? 1 : 2]++;
#endif
            int sub = ((xs << 16) + 0x8000 - lx) >> 8;
            u16* zp = h.zrow + xs;
            u16* cp = h.crow + (xs << iDouble);
            int cs = 1 << iDouble;
            int w = h.a0 + h.dws * sub;
            if (aKind == K_FLAT)
                {
                do { int z = w >> 8; if (z > *zp) { *zp = (u16)z; R3D_VIS cp[0] = cp[cs - 1] = (u16)h.x.col; } zp++; cp += cs; w += h.dw; } while (--n);
                }
            else if (aKind == K_GOURAUD)
                {
                int r = h.a1 + h.d1s * sub, g = h.a2 + h.d2s * sub, b = h.a3 + h.d3s * sub;
                do
                    {
                    int z = w >> 8;
                    if (z > *zp) { *zp = (u16)z; R3D_VIS cp[0] = cp[cs - 1] = (u16)(((r & 0xF80000) >> 8) | ((g & 0xFC0000) >> 13) | ((unsigned)b >> 19)); }
                    zp++; cp += cs; w += h.dw; r += h.d1; g += h.d2; b += h.d3;
                    }
                while (--n);
                }
            else
                {
                int u = h.a1 + h.d1s * sub, v = h.a2 + h.d2s * sub;
                do
                    {
                    int z = w >> 8;
                    int t = h.x.tex[(iTexQ == 1 || iTexQ == 4) ? ((v & 0x7F0000) >> 9) | ((u & 0x7F0000) >> 16)
                                  : (iTexQ == 3 || iTexQ == 5) ? ((v & 0xFF0000) >> 8) | ((u & 0xFF0000) >> 16)
                                  : ((v & 0x3F0000) >> 10) | ((u & 0x3F0000) >> 16)];
                    if (z > *zp && (t || !(iTexQ == 2 || iTexQ >= 4))) { *zp = (u16)z; R3D_VIS cp[0] = cp[cs - 1] = h.pal[t]; }
                    zp++; cp += cs; w += h.dw; u += h.d1; v += h.d2;
                    }
                while (--n);
                }
            }
        h.a0 += h.da0; h.a1 += h.da1; h.a2 += h.da2; h.a3 += h.da3;
        h.crow = (u16*)((u8*)h.crow + h.cstride); h.zrow += h.W;
        }
#endif
    }

// clip polygon against one plane; returns new count
static int PlaneDist(const TV& v, int p, int hw, int hh, int fx, int fy)
    {
    switch (p)
        {
        case 0: return v.z - NEAR;
        case 1: return v.z * hw + v.x * fx;
        case 2: return v.z * hw - v.x * fx;
        case 3: return v.z * hh - v.y * fy;
        default: return v.z * hh + v.y * fy;
        }
    }

static inline int LerpI(int a, int b, int t)
    {
    int d = b - a;
    if (d < (1 << 18) && d > -(1 << 18)) return a + ((d * t) >> 12);
    return a + (((d >> 6) * t) >> 6);          // far land: kilometres between the ends
    }
static void Lerp(TV& o, const TV& a, const TV& b, int t)
    {
    o.x = LerpI(a.x, b.x, t);
    o.y = LerpI(a.y, b.y, t);
    o.z = LerpI(a.z, b.z, t);
    o.a[0] = a.a[0] + (((b.a[0] - a.a[0]) >> 2) * t >> 10);
    o.a[1] = a.a[1] + (((b.a[1] - a.a[1]) >> 2) * t >> 10);
    o.a[2] = a.a[2] + (((b.a[2] - a.a[2]) >> 2) * t >> 10);
    o.code = 0;
    }

void R3D::ClipAndDraw(TV* a, TV* b, TV* c, int aKind)
    {
    int codes = a->code | b->code | c->code;
    TV* in = iClipA;
    TV* out = iClipB;
    in[0] = *a; in[1] = *b; in[2] = *c;
    int n = 3, p;
    for (p = 0; p < 5 && n >= 3; p++)
        {
        if (!(codes & (1 << p))) continue;
        int m = 0, i;
        for (i = 0; i < n; i++)
            {
            TV& va = in[i];
            TV& vb = in[(i + 1) % n];
            int da = PlaneDist(va, p, iHW, iHH, iFx, iFy);
            int db = PlaneDist(vb, p, iHW, iHH, iFx, iFy);
            if (da >= 0) out[m++] = va;
            if ((da >= 0) != (db >= 0))
                {
                int num = da, den = da - db;
                while (den > 32767 || den < -32767) { num >>= 1; den >>= 1; }
                int t = 0;
                if (den > 0) t = MulHi30(num << 12, iRecip[den]);
                else if (den < 0) t = -MulHi30(num << 12, iRecip[-den]);
                t = Clamp(t, 0, 4096);
                Lerp(out[m++], va, vb, t);
                }
            if (m >= 11) break;
            }
        TV* s = in; in = out; out = s;
        n = m;
        }
    if (n < 3) return;
    int i;
    for (i = 0; i < n; i++) Project(in[i]);
    for (i = 1; i + 1 < n; i++)
        {
        TV* p0 = &in[0]; TV* p1 = &in[i]; TV* p2 = &in[i + 1];
        int cr = (p1->sx - p0->sx) * (p2->sy - p0->sy) - (p1->sy - p0->sy) * (p2->sx - p0->sx);
        if (cr <= 0) continue;
        Raster(p0, p1, p2, aKind);
        }
    }

void R3D::DrawTri(TV* a, TV* b, TV* c, int aKind)
    {
    if (a->code & b->code & c->code & 31) return;
    if (!((a->code | b->code | c->code) & 32))
        {
        int cr = (b->sx - a->sx) * (c->sy - a->sy) - (b->sy - a->sy) * (c->sx - a->sx);
        if (cr <= 0) return;
        int mn = a->sy, mx = a->sy;
        if (b->sy < mn) mn = b->sy; else mx = b->sy;
        if (c->sy < mn) mn = c->sy; else if (c->sy > mx) mx = c->sy;
        if (((mn + 7) >> 4) == ((mx + 7) >> 4)) return;          // (v0.4.43: no row's middle crossed - nothing drawn)
        if (iOccOn && OccHiddenTri(a, b, c)) { iOccTris++; return; }
        if (aKind == K_FLAT) RasterFlat(a, b, c); else Raster(a, b, c, aKind);
        return;
        }
    ClipAndDraw(a, b, c, aKind);
    }

// ---------------------------------------------------------------- scene drawing
void R3D::DrawSky()
    {
    int sp = r3d_sin(iPitch), cp = r3d_cos(iPitch);
    if (cp < 1000) cp = 1000;
    int yh = (iCy16 >> 4) - sp * iFy / cp;     // horizon row
    int span = iFy * 7 / 8;                    // rows from horizon to "zenith" colour
    int ks = (256 << 8) / (span > 0 ? span : 1);
    int y;
    for (y = 0; y < iH; y++)
        {
        int d = yh - y;
        int t = d <= 0 ? 0 : Clamp((d * ks) >> 8, 0, 256);
        int r = iFogR + ((iZenR - iFogR) * t >> 8);
        int g = iFogG + ((iZenG - iFogG) * t >> 8);
        int b = iFogB + ((iZenB - iFogB) * t >> 8);
        u32 c = Rgb(r, g, b);
        c |= c << 16;
        u32* row = (u32*)(iCB + y * iCStride);
        int nw = iDouble ? iW : iW / 2;
#ifdef USE_ASM
        fill32(row, c, nw);
#else
        while (nw--) *row++ = c;
#endif
        }
    ClearZ();
    (void)yh;           // (the sun and the clouds: after the world - RenderScene)
    if (iStars > 0)
        {
        // fixed star field on the sky sphere
        int i;
        int sc = iStars;
        for (i = 0; i < 90; i++)
            {
            u32 h = Hash(i, 3, 1234);
            int az = (int)(h & 1023), el = 30 + (int)((h >> 10) % 220);
            int ce = r3d_cos(el);
            int dx = (r3d_sin(az) * ce) >> 14, dz = (r3d_cos(az) * ce) >> 14, dy = r3d_sin(el);
            // rotate direction into camera space (no translation)
            int x = (iM[0] * dx + iM[2] * dz) >> 14;
            int yy = (iM[3] * dx + iM[4] * dy + iM[5] * dz) >> 14;
            int z = (iM[6] * dx + iM[7] * dy + iM[8] * dz) >> 14;
            if (z < 2000) continue;
            int sx = (iCx16 >> 4) + x * iFx / z;
            int sy = (iCy16 >> 4) - yy * iFy / z;
            if (sx < 0 || sx >= iW || sy < 0 || sy >= iH) continue;
            int b = sc * (120 + (int)((h >> 20) & 127)) >> 8;
            u16 col = Rgb(b, b, b + 20 > 255 ? 255 : b + 20);
            if (iDouble) { u16* p = iCB + sy * iCStride + sx * 2; p[0] = col; p[1] = col; }
            else iCB[sy * iCStride + sx] = col;
            }
        }
    }

// a horizontal run of sky pixels mixed with a colour: 0 = opaque, 1 = half, 2 = a quarter
static void SkyRun(u16* p, int n, u16 c, int mode)
    {
    if (mode == 0) { while (n--) *p++ = c; return; }
    if (mode == 1)
        {
        u16 h = (u16)((c >> 1) & 0x7BEF);
        while (n--) { *p = (u16)(((*p >> 1) & 0x7BEF) + h); p++; }
        return;
        }
    u16 q = (u16)((c >> 2) & 0x39E7);
    while (n--) { u16 v = *p; *p++ = (u16)(v - ((v >> 2) & 0x39E7) + q); }
    }
// (v0.4.43: painted after the world, on the pixels nothing was drawn on - the z buffer's 0s - not under it all)
// (occ: the occlusion map's row - an 8 x 8 block with something in all of it skipped whole)
static void SkySpan(u16* row, const u16* zrow, const u16* occ, int iDouble, int x0, int x1, u16 c, int mode)
    {
    if (x1 <= x0) return;
    if (!zrow) { SkyRun(row + (x0 << iDouble), (x1 - x0) << iDouble, c, mode); return; }
    int x = x0;
    while (x < x1)
        {
#ifdef USE_ASM
        x = z_skip(zrow, x, x1);          // (v0.4.49: two at a time)
#else
        while (x < x1 && zrow[x])
            {
            if (occ && !(x & 7) && occ[x >> 3]) x += 8;
            else x++;
            }
#endif
        if (x > x1) x = x1;
        int a = x;
        while (x < x1 && !zrow[x]) x++;
        if (x > a) SkyRun(row + (a << iDouble), (x - a) << iDouble, c, mode);
        }
    }

// a filled ellipse on the sky
static void SkyBlob(u16* cb, int stride, const u16* zb, const u16* occ, int W, int H, int iDouble, int cx, int cy, int rx, int ry, u16 c, int mode)
    {
    if (rx < 1 || ry < 1) return;
    int y0 = cy - ry, y1 = cy + ry, y;
    if (y0 < 0) y0 = 0;
    if (y1 > H - 1) y1 = H - 1;
    int iry = (1 << 16) / ry;
    for (y = y0; y <= y1; y++)
        {
        int dy = ((y - cy) * iry) >> 8;                    // -256..256
        int w2 = 65536 - dy * dy;
        if (w2 <= 0) continue;
        int w = r3d_isqrt(w2);                              // 0..256
        int hw = (rx * w) >> 8;
        int a = cx - hw, b = cx + hw + 1;
        if (a < 0) a = 0;
        if (b > W) b = W;
        SkySpan(cb + y * stride, zb ? zb + y * W : 0, occ ? occ + (y >> 3) * (640 / 8) : 0, iDouble, a, b, c, mode);
        }
    }

// the sun (a bright disc with a glow) and drifting clouds, painted on the sky before the land
void R3D::DrawSunAndClouds(int yh)
    {
    if (iCell) return;
    (void)yh;
    int W = iW, H = iH;
    // ---- sun
    if (iSunVis > 0)
        {
        int x = (iM[0] * iSunX + iM[1] * iSunY + iM[2] * iSunZ) >> 14;
        int y = (iM[3] * iSunX + iM[4] * iSunY + iM[5] * iSunZ) >> 14;
        int z = (iM[6] * iSunX + iM[7] * iSunY + iM[8] * iSunZ) >> 14;
        if (z > 3000)
            {
            int sx = (iCx16 >> 4) + x * iFx / z, sy = (iCy16 >> 4) - y * iFy / z;
            int R = iFx * 700 / z;                          // ~2.5 degrees across
            if (R < 3) R = 3;
            if (sx > -4 * R && sx < W + 4 * R && sy > -4 * R && sy < H + 4 * R)
                {
                int warm = iSunY < 4000 ? 1 : 0;            // low sun: orange
                u16 glow = warm ? Rgb(255, 170, 90) : Rgb(255, 236, 190);
                if (iSunVis > 128) SkyBlob(iCB, iCStride, iZB, iOccOn ? iOccMap : 0, W, H, iDouble, sx, sy, R * 3, R * 3, glow, 2);
                SkyBlob(iCB, iCStride, iZB, iOccOn ? iOccMap : 0, W, H, iDouble, sx, sy, R * 2, R * 2, glow, 2);
                SkyBlob(iCB, iCStride, iZB, iOccOn ? iOccMap : 0, W, H, iDouble, sx, sy, R, R, warm ? Rgb(255, 214, 150) : Rgb(255, 252, 236), iSunVis > 160 ? 0 : 1);
                }
            }
        }
    // ---- clouds: puffs of three blobs each, drifting slowly with the wind
    if (iCloud <= 0) return;
    int n = 8 + (iCloud * 8 >> 8), k;
    int drift = (iAnimMs / 600) & 1023;
    u16 top = Rgb(iCloudR, iCloudG, iCloudB);
    u16 under = Rgb(iCloudR * 13 / 16, iCloudG * 13 / 16, iCloudB * 13 / 16);
    for (k = 0; k < n; k++)
        {
        u32 h = Hash(k, 11, 4321);
        int az = ((int)(h & 1023) + drift * (1 + (int)(k & 1))) & 1023;
        int el = 14 + (int)((h >> 10) % 80);
        int ce = r3d_cos(el);
        int dx = (r3d_sin(az) * ce) >> 14, dz = (r3d_cos(az) * ce) >> 14, dy = r3d_sin(el);
        int x = (iM[0] * dx + iM[1] * dy + iM[2] * dz) >> 14;
        int y = (iM[3] * dx + iM[4] * dy + iM[5] * dz) >> 14;
        int z = (iM[6] * dx + iM[7] * dy + iM[8] * dz) >> 14;
        if (z < 3000) continue;
        int sx = (iCx16 >> 4) + x * iFx / z, sy = (iCy16 >> 4) - y * iFy / z;
        int size = 1600 + (int)((h >> 20) % 2000) + (iCloud * 6);          // angular radius * 16384
        int rx = iFx * size / z;
        int ry = rx * (90 + (dy >> 7)) >> 8;                // flatter towards the horizon
        if (sx + 2 * rx < 0 || sx - 2 * rx > W || sy + 2 * ry < 0 || sy - 2 * ry > H) continue;
        int dense = iCloud > 150 ? 1 : 2;
        // a cumulus: a flat-bottomed row of puffs, the middle ones taller; shaded underneath, bright on top
        int nb = 4 + (int)((h >> 5) & 1), b;
        for (b = 0; b < nb; b++)
            {
            u32 hb = Hash(k, b, 99);
            int bx = sx - rx + (2 * rx * b + rx) / nb + (int)(hb & 7) - 4;
            int mid = b == 0 || b == nb - 1 ? 0 : 1;
            int br = (rx * (mid ? 9 : 6)) / (nb * 2) + (int)((hb >> 4) % (unsigned)(rx / 4 + 1));
            int bry = br * ry / rx;
            if (bry < 2) bry = 2;
            SkyBlob(iCB, iCStride, iZB, iOccOn ? iOccMap : 0, W, H, iDouble, bx, sy + ry / 4, br, bry * 2 / 3, under, dense);
            SkyBlob(iCB, iCStride, iZB, iOccOn ? iOccMap : 0, W, H, iDouble, bx, sy - bry / 3, br * 4 / 5, bry, top, 2);
            }
        }
    }

// the detail of a chunk by its distance: 1 (every cell) near, coarser further, 0 = not drawn
int R3D::LodOf(int cx, int cz)
    {
    if (cx < 0 || cz < 0 || cx >= iNCX || cz >= iNCZ) return 0;
    int mx = ((cx * CHUNK + CHUNK / 2) * CELL - iCamX) >> 8, mz = ((cz * CHUNK + CHUNK / 2) * CELL - iCamZ) >> 8;
    if (mx > iVis + 23 || mx < -iVis - 23 || mz > iVis + 23 || mz < -iVis - 23) return 0;
    int d = r3d_isqrt(mx * mx + mz * mz);       // metres
    if (d > iVis + 23) return 0;
    if (d < 55) return 1;
    if (d < 100) return 2;
    if (d < 150) return 4;
    return 8;
    }

void R3D::DrawTerrain()
    {
    // near to far: rings of chunks round the camera's chunk, out to the view distance
    int ccx = iCamX / (CELL * CHUNK), ccz = iCamZ / (CELL * CHUNK);
    int R = (iVis + 23) / (CHUNK * CELL / METER) + 1;
    int r, cx, cz;
    for (r = 0; r <= R; r++)
        for (cz = ccz - r; cz <= ccz + r; cz++)
            {
            if (cz < 0 || cz >= iNCZ) continue;
            int step = (cz == ccz - r || cz == ccz + r) ? 1 : 2 * r;
            if (step == 0) step = 1;
            for (cx = ccx - r; cx <= ccx + r; cx += step)
                if (LodOf(cx, cz)) DrawChunk(cx, cz);
            }
    }

void R3D::DrawChunk(int cx, int cz)
    {
    int lod = LodOf(cx, cz);
    int ci = cz * iNCX + cx;
    // sphere visibility test
    int half = CHUNK * CELL / 2;
    int mnh = iChunkMinH[ci], mxh = iChunkMaxH[ci];
    int wl = WaterOfChunk(ci);
    if (mnh < wl) mnh = wl;
    if (mxh < wl) mxh = wl;
    TV c;
    TransformPt(cx * CHUNK * CELL + half, (mnh + mxh) >> 1, cz * CHUNK * CELL + half, c);
    int rad = half * 3 / 2 + ((mxh - mnh) >> 1);
    if (c.z < -rad) return;
    // side planes: normal (fx, 0, hw) length ~ sqrt(fx^2+hw^2)
    int lx = iLenX, ly = iLenY;
    if (c.z * iHW + c.x * iFx < -rad * lx) return;
    if (c.z * iHW - c.x * iFx < -rad * lx) return;
    if (c.z * iHH - c.y * iFy < -rad * ly) return;
    if (c.z * iHH + c.y * iFy < -rad * ly) return;
    if (iOccOn && OccHiddenSphere(c, rad)) { iOccSkipped++; return; }     // wholly behind the buildings
    const TChunk* tc = Cached(cx, cz, 1);
    if (!tc) return;

    int n = CHUNK / lod;
    int i, j;
    int gx0 = cx * CHUNK, gz0 = cz * CHUNK;
    int W = iHS;
    int lit = iNumLights && LightsNear(c.x, c.y, c.z, rad);
    // neighbour LODs for crack fixing
    int lw = LodOf(cx - 1, cz), le = LodOf(cx + 1, cz), ls = LodOf(cx, cz - 1), ln = LodOf(cx, cz + 1);
    int tex = iMode == MODE_TEX;
    for (j = 0; j <= n; j++)
        for (i = 0; i <= n; i++)
            {
            int gx = gx0 + i * lod, gz = gz0 + j * lod;
            int idx = gz * W + gx;
            int k3 = ((j * lod) * 9 + i * lod) * 3;
            int h = iHeight[idx] * HS;
            // snap edge vertices to the coarser neighbour's edge
            int nl = 0;
            if (i == 0 && lw > lod) nl = lw;
            if (i == n && le > lod) nl = le;
            if (nl && (gz % nl))
                {
                int z0 = gz - gz % nl;
                int ha = iHeight[z0 * W + gx] * HS, hb = iHeight[(z0 + nl) * W + gx] * HS;
                h = ha + (((hb - ha) * (gz - z0)) >> (nl == 2 ? 1 : nl == 4 ? 2 : 3));
                }
            nl = 0;
            if (j == 0 && ls > lod) nl = ls;
            if (j == n && ln > lod) nl = ln;
            if (nl && (gx % nl))
                {
                int x0 = gx - gx % nl;
                int ha = iHeight[gz * W + x0] * HS, hb = iHeight[gz * W + x0 + nl] * HS;
                h = ha + (((hb - ha) * (gx - x0)) >> (nl == 2 ? 1 : nl == 4 ? 2 : 3));
                }
            TV& v = iVerts[j * (n + 1) + i];
            TransformPt(gx * CELL, h, gz * CELL, v);
            Outcode(v);
            if (!v.code) Project(v);
            if (tex)
                {
                v.a[0] = i << 14;       // 64 texels per quad, 8.8
                v.a[1] = j << 14;
                }
            else if (lit)
                {
                int add = PointLight(v.x, v.y, v.z);
                const u8* L = tc->lit + k3;
                FogColour(v.z, Clamp(L[0] + (L[0] * add >> 8) + (add >> 3), 0, 255),
                          Clamp(L[1] + (L[1] * add >> 8), 0, 255), L[2], v);
                }
            else
                FogColour(v.z, tc->lit[k3], tc->lit[k3 + 1], tc->lit[k3 + 2], v);
            }
    for (j = 0; j < n; j++)
        for (i = 0; i < n; i++)
            {
            TV* v00 = &iVerts[j * (n + 1) + i];
            TV* v10 = v00 + 1;
            TV* v01 = v00 + n + 1;
            TV* v11 = v01 + 1;
            if (tex)
                {
                int gx = gx0 + i * lod, gz = gz0 + j * lod;
                int idx = gz * W + gx;
                int zz = (v00->z + v11->z) >> 1;
                int f = (FogF(zz) >> 5 > 7 ? 7 : FogF(zz) >> 5);
                int l = tc->lev[(j * lod) * 9 + i * lod];
                if (lit)
                    {
                    int add = PointLight((v00->x + v11->x) >> 1, (v00->y + v11->y) >> 1, (v00->z + v11->z) >> 1);
                    l = Clamp(l + (add >> 4), 0, 15);
                    }
                iCurPal = iShadePal + (l * 8 + f) * 256;
                iCurTex = iTex + KTerrTex[iMat[idx] & VM_MAT] * 4096;
                // keep uv small: rebase per quad
                int bu = i << 14, bv = j << 14;
                v00->a[0] -= bu; v10->a[0] -= bu; v01->a[0] -= bu; v11->a[0] -= bu;
                v00->a[1] -= bv; v10->a[1] -= bv; v01->a[1] -= bv; v11->a[1] -= bv;
                DrawTri(v00, v11, v10, K_TEX);
                DrawTri(v00, v01, v11, K_TEX);
                v00->a[0] += bu; v10->a[0] += bu; v01->a[0] += bu; v11->a[0] += bu;
                v00->a[1] += bv; v10->a[1] += bv; v01->a[1] += bv; v11->a[1] += bv;
                }
            else if (lod >= 2)
                {
                // (from 55 m on, each triangle flat in the average of its corners: a third of the cost, the same look)
                iFlatCol = ::Rgb(Clamp((v00->a[0] + v11->a[0] + v10->a[0]) / 768, 0, 255), Clamp((v00->a[1] + v11->a[1] + v10->a[1]) / 768, 0, 255),
                                 Clamp((v00->a[2] + v11->a[2] + v10->a[2]) / 768, 0, 255));
                DrawTri(v00, v11, v10, K_FLAT);
                iFlatCol = ::Rgb(Clamp((v00->a[0] + v01->a[0] + v11->a[0]) / 768, 0, 255), Clamp((v00->a[1] + v01->a[1] + v11->a[1]) / 768, 0, 255),
                                 Clamp((v00->a[2] + v01->a[2] + v11->a[2]) / 768, 0, 255));
                DrawTri(v00, v01, v11, K_FLAT);
                }
            else
                {
                // (near: where the corners' colours hardly differ, flat in their average - the same look, cheaper)
                LandTri(v00, v11, v10);
                LandTri(v00, v01, v11);
                }
            }
    // water
    if (iChunkMinH[ci] < wl)
        {
        TV w[4];
        int k;
        for (k = 0; k < 4; k++)
            {
            int x = (gx0 + ((k & 1) ? CHUNK : 0)) * CELL, z = (gz0 + ((k & 2) ? CHUNK : 0)) * CELL;
            TransformPt(x, wl, z, w[k]);
            Outcode(w[k]);
            if (!w[k].code) Project(w[k]);
            FogColour(w[k].z, 44, 84, 124, w[k]);
            }
        if (tex)
            {
            iCurTex = iTex + TX_SNOW * 4096;
            int f = (FogF(w[0].z) >> 5 > 7 ? 7 : FogF(w[0].z) >> 5);
            iCurPal = iShadePal + (3 * 8 + f) * 256;
            for (k = 0; k < 4; k++) { w[k].a[0] = (k & 1) << 14; w[k].a[1] = (k >> 1) << 14; }
            DrawTri(&w[0], &w[3], &w[1], K_GOURAUD);
            DrawTri(&w[0], &w[2], &w[3], K_GOURAUD);
            }
        else
            {
            DrawTri(&w[0], &w[3], &w[1], K_GOURAUD);
            DrawTri(&w[0], &w[2], &w[3], K_GOURAUD);
            }
        }
    }

// a near land triangle: gouraud, or flat when its corners are within 6 (of 255) of each other in every colour
void R3D::LandTri(TV* a, TV* b, TV* c)
    {
    int k;
    for (k = 0; k < 3 && !iFlatLand; k++)          // (v0.4.50: the setting - every one flat)
        {
        int x = a->a[k], y = b->a[k], z = c->a[k];
        int mx = x > y ? x : y, mn = x < y ? x : y;
        if (z > mx) mx = z;
        if (z < mn) mn = z;
        if (mx - mn > LANDFLAT * 256) { DrawTri(a, b, c, K_GOURAUD); return; }
        }
    iFlatCol = ::Rgb(Clamp((a->a[0] + b->a[0] + c->a[0]) / 768, 0, 255), Clamp((a->a[1] + b->a[1] + c->a[1]) / 768, 0, 255),
                     Clamp((a->a[2] + b->a[2] + c->a[2]) / 768, 0, 255));
    DrawTri(a, b, c, K_FLAT);
    }

// the far land: past the detailed land, a coarse sheet of the heights (a vertex every 64 or 128 m) out to the
// far view distance, lit like the land and hazed by the same fog - the mountains on the skyline
void R3D::DrawFarLand()
    {
    if (!iFarOn) return;
    if (iFarColKey != iBakedSun || iFarColRow > 0)
        {
        // the sun has moved on: re-light the far land's vertices - all at once the first time, then 6 rows a frame
        // (the change is slight; one frame doing it all would stall)
        int nx16 = iGX / 16, nz16 = iGZ / 16, x, z, l;
        if (iFarColRow <= 0) { iFarColRow = 0; iFarColNext = iBakedSun; }
        int z1 = iFarColKey == -1 ? nz16 + 1 : iFarColRow + 6;
        if (z1 > nz16 + 1) z1 = nz16 + 1;
        for (z = iFarColRow; z < z1; z++)
            for (x = 0; x <= nx16; x++)
                {
                int r, g, b;
                VertexColour(x * 16, z * 16, r, g, b, l);
                u8* fc = iFarCol + (z * (nx16 + 1) + x) * 3;
                fc[0] = (u8)r; fc[1] = (u8)g; fc[2] = (u8)b;
                }
        iFarColRow = z1;
        if (z1 > nz16) { iFarColKey = iFarColNext; iFarColRow = 0; }
        }
    int fs = iFarVis > 1600 ? 32 : 16;                 // cells per far quad
    int step = fs * CELL;
    int R = iFarVis * METER;
    int nx = iGX / fs, nz = iGZ / fs;                  // far vertices: 0..nx, 0..nz
    int gx0 = Clamp((iCamX - R) / step, 0, nx), gx1 = Clamp((iCamX + R) / step + 1, 0, nx);
    int gz0 = Clamp((iCamZ - R) / step, 0, nz), gz1 = Clamp((iCamZ + R) / step + 1, 0, nz);
    if (gx1 - gx0 + 1 > MAXFARV) { int c = (gx0 + gx1) / 2; gx0 = c - MAXFARV / 2 + 1; gx1 = c + MAXFARV / 2 - 1; }
    int nearR = (iVis - 6) * METER;                   // a quad wholly inside this is the detailed land's
    int W = iHS;
    TV* ra = iFarV;
    TV* rb = iFarV + MAXFARV;
    int gx, gz;
    // which vertices can be in a quad that is drawn (between the detailed land's reach and the far one): within
    // half a quad's diagonal more either way; the rest are not worked out
    int qm = (step * 3 / 2) >> 8;                                      // (m)
    int rin = (nearR >> 8) - qm, rout = iFarVis + qm;
    int rin2 = rin > 0 ? rin * rin : -1, rout2 = rout * rout;
    int cw = iGX / 16 + 1, fsc = fs / CSTEP, fs16 = fs / 16, fsk = fs / CHUNK;
    int ncx = iNCX - 1, ncz = iNCZ - 1;
    int lim2 = -2 * step * iLenX;
    int pa = 1, pb = 0;                                // (v0.4.71) the row before's vertices worked out: pa..pb
    for (gz = gz0; gz <= gz1; gz++)
        {
        int z = gz * step;
        int dzc = z - iCamZ, dzm = dzc >> 8, dz2 = dzm * dzm;
        // (v0.4.71) only the row's part within the ring's outer edge, less its ends wholly off a side or behind (the
        // tests as below): the rest of the square round the camera - most of it - not looked at, and no quad drawn
        // that has a corner there (each of those is out of sight whole, or past the far reach)
        int ka = 1, kb = 0;
        if (dz2 <= rout2)
            {
            int xr = r3d_isqrt(rout2 - dz2) + 1;
            ka = (iCamX - xr * METER) / step; kb = (iCamX + xr * METER) / step + 1;
            if (ka < gx0) ka = gx0;
            if (kb > gx1) kb = gx1;
            const short* chR = iCH + (gz * fsc) * iCW;
            int dx0_ = gx0 * step - iCamX, cy_ = iCamY + METER, m7_ = iM[7];
            int bzr0 = iM[6] * (dx0_ >> 4) + iM[8] * (dzc >> 4), bzs_ = iM[6] * (step >> 4);
            int bxr0 = iM[0] * (dx0_ >> 4) + iM[2] * (dzc >> 4), bxs_ = iM[0] * (step >> 4);
            int hw = iHW, fx_ = iFx;
            while (ka <= kb)
                {
                int bz = (bzr0 + bzs_ * (ka - gx0) + m7_ * ((chR[ka * fsc] * HS - cy_) >> 4)) >> 10, bx = (bxr0 + bxs_ * (ka - gx0)) >> 10;
                if (bz < -2 * step || bz * hw + bx * fx_ < lim2 || bz * hw - bx * fx_ < lim2) ka++; else break;
                }
            while (kb >= ka)
                {
                int bz = (bzr0 + bzs_ * (kb - gx0) + m7_ * ((chR[kb * fsc] * HS - cy_) >> 4)) >> 10, bx = (bxr0 + bxs_ * (kb - gx0)) >> 10;
                if (bz < -2 * step || bz * hw + bx * fx_ < lim2 || bz * hw - bx * fx_ < lim2) kb--; else break;
                }
            }
        const short* chRow = iCH + (gz * fsc) * iCW;
        const u8* fcRow = iFarCol + (gz * fs16) * cw * 3;
        int wrow = Clamp(gz * fsk, 0, ncz) * iNCX;
        // (the tests below stepped along the row: x grows by a quad each vertex)
        int dx0 = gx0 * step - iCamX;
        int bzr = iM[6] * (dx0 >> 4) + iM[8] * (dzc >> 4), bzs = iM[6] * (step >> 4);
        int bxr = iM[0] * (dx0 >> 4) + iM[2] * (dzc >> 4), bxs = iM[0] * (step >> 4);
        int m7 = iM[7], cy = iCamY + METER;
        if (ka > gx0) { bzr += bzs * (ka - gx0); bxr += bxs * (ka - gx0); }
        for (gx = ka; gx <= kb; gx++, bzr += bzs, bxr += bxs)
            {
            int x = gx * step;
            TV& v = rb[gx - gx0];
            int dxc = x - iCamX, dxm = dxc >> 8, d2 = dxm * dxm + dz2;
            if (d2 < rin2 || d2 > rout2) { v.code = 32; continue; }     // (in no quad drawn)
            int hh = chRow[gx * fsc] * HS;
            // well behind the camera or off to a side (more than two quads): every quad it is in is too, left out
            // whole; the planes it is outside of, nothing more worked out
            int bz = (bzr + m7 * ((hh - cy) >> 4)) >> 10;
            int bx = bxr >> 10;
            int pl = bz * iHW + bx * iFx, pr = bz * iHW - bx * iFx;
            if (bz < -2 * step || pl < lim2 || pr < lim2)
                {
                v.code = 32 | (bz < NEAR ? 1 : 0) | (pl < 0 ? 2 : 0) | (pr < 0 ? 4 : 0);
                v.z = bz;
                continue;
                }
            int h = hh - METER;                             // (a little low: the near land wins)
            const u8* fc = fcRow + gx * fs16 * 3;
            int r = fc[0], g = fc[1], b = fc[2];
            int wl = WaterOfChunk(wrow + Clamp(gx * fsk, 0, ncx));
            if (h < wl) { h = wl; r = 44; g = 84; b = 124; }
            TransformPt(x, h, z, v);
            Outcode(v);
            if (!v.code) Project(v);
            FogColour(v.z, r, g, b, v);
#ifdef R3D_STATS
            g_r3dStage[8]++;
#endif
            }
        int qa = pa > ka ? pa : ka, qb = pb < kb ? pb : kb;
        pa = ka; pb = kb;
        if (gz > gz0)
            for (gx = qa; gx < qb; gx++)
                {
                // skip what the detailed land covers: the quad's nearest point to the camera inside its reach
                int x0 = gx * step, x1 = x0 + step, z0 = (gz - 1) * step, z1 = z0 + step;
                int qx = iCamX < x0 ? x0 : (iCamX > x1 ? x1 : iCamX), qz = iCamZ < z0 ? z0 : (iCamZ > z1 ? z1 : iCamZ);
                int dx = (qx - iCamX) >> 8, dz = (qz - iCamZ) >> 8;                    // nearest point (m)
                if (dx * dx + dz * dz > iFarVis * iFarVis) continue;
                int ex = Abs(x0 - iCamX) > Abs(x1 - iCamX) ? Abs(x0 - iCamX) : Abs(x1 - iCamX);
                int ez = Abs(z0 - iCamZ) > Abs(z1 - iCamZ) ? Abs(z0 - iCamZ) : Abs(z1 - iCamZ);
                ex >>= 8; ez >>= 8;                                                      // farthest corner (m)
                if (ex * ex + ez * ez < (nearR >> 8) * (nearR >> 8)) continue;
                TV* v00 = &ra[gx - gx0]; TV* v10 = v00 + 1;
                TV* v01 = &rb[gx - gx0]; TV* v11 = v01 + 1;
                if (v00->code & v10->code & v01->code & v11->code & 31) continue;
                LandTri(v00, v11, v10);         // (flat where its corners hardly differ, as the near land)
                LandTri(v00, v01, v11);
                }
        TV* t = ra; ra = rb; rb = t;
        }
    }

// ---------------------------------------------------------------- pre-baked meshes
u8* R3D::Alloc(int aBytes)
    {
    if (iArena + aBytes > iArenaEnd) { iArenaOver += aBytes;
#ifdef R3D_STATS
        { extern int g_arenaOver1, g_arenaFree1; if (!g_arenaOver1) { g_arenaOver1 = aBytes; g_arenaFree1 = (int)(iArenaEnd - iArena); } }
#endif
        }
    u8* p = iArena;
    iArena += (aBytes + 7) & ~7;
    return p;
    }

void R3D::MeshInit(Mesh& m, int aMaxV, int aMaxF, int aFrames)
    {
    if (aMaxV > 512) aMaxV = 512;       // (meshes built here: 512 vertices at most, as before MAXMV grew)
    if (aMaxF < aMaxV * 2) aMaxF = aMaxV * 2;       // closed shapes need about two faces per vertex
    m.nv = 0; m.nf = 0; m.nfr = aFrames; m.vstride = aMaxV; m.fstride = aMaxF; m.radius = 0; m.vn = 0; m.oneN = 0; m.dropped = 0; m.cl = 0; m.ncl = 0; m.lazy = 0; m.uvsh = 0;
    m.v = (short*)Alloc(aMaxV * 3 * 2 * aFrames);
    m.f = (u16*)Alloc(aMaxF * 8);
    m.n = (short*)Alloc(aMaxF * 3 * 2 * aFrames);
    m.uv = (u16*)Alloc(aMaxF * 12);
    iMeshCursor = 0;
    }

// exact unit face normal (Q14) of triangle A,B,C; zero-area faces get (0,0,0)
static void FaceNormal(const short* A, const short* B, const short* C, int& nx, int& ny, int& nz)
    {
    int ex = B[0] - A[0], ey = B[1] - A[1], ez = B[2] - A[2];
    int fx = C[0] - A[0], fy = C[1] - A[1], fz = C[2] - A[2];
    // edges fit in 16 bits; pre-shift only when products could overflow
    int sh = 0;
    int mx = Abs(ex) | Abs(ey) | Abs(ez) | Abs(fx) | Abs(fy) | Abs(fz);
    while ((mx >> sh) > 16000) sh++;
    ex >>= sh; ey >>= sh; ez >>= sh; fx >>= sh; fy >>= sh; fz >>= sh;
    nx = ey * fz - ez * fy;
    ny = ez * fx - ex * fz;
    nz = ex * fy - ey * fx;
    // scale to < 2^15 per component without losing small values
    while (Abs(nx) > 23000 || Abs(ny) > 23000 || Abs(nz) > 23000) { nx >>= 1; ny >>= 1; nz >>= 1; }
    int l = r3d_isqrt(nx * nx + ny * ny + nz * nz);
    if (l == 0) { nx = ny = nz = 0; return; }
    nx = nx * 16384 / l; ny = ny * 16384 / l; nz = nz * 16384 / l;
    }

// adds a face (frame 0 only); winding fixed so that cross(b-a, c-a) points away from (cx,cy,cz)
static void AddFace(Mesh& m, int a, int b, int c, int slot, int cx, int cy, int cz,
                    int u0, int v0, int u1, int v1, int u2, int v2, int aIn = 0)
    {
    if (m.nf >= m.fstride || a >= m.vstride || b >= m.vstride || c >= m.vstride) { m.dropped++; return; }
    const short* A = m.v + a * 3; const short* B = m.v + b * 3; const short* C = m.v + c * 3;
    int nx, ny, nz;
    FaceNormal(A, B, C, nx, ny, nz);
    int ox = (A[0] + B[0] + C[0]) / 3 - cx, oy = (A[1] + B[1] + C[1]) / 3 - cy, oz = (A[2] + B[2] + C[2]) / 3 - cz;
    // nx..nz are unit (Q14); |o| < 2^16 -> products < 2^30 each, sum fits after >>2
    int dot = ((nx * ox) >> 2) + ((ny * oy) >> 2) + ((nz * oz) >> 2);
    u16* f = m.f + m.nf * 4;
    u16* uv = m.uv + m.nf * 6;
    if (aIn) dot = -dot;                    // faces turned towards the centre (the inside of a cup)
    if (dot < 0) { int t = b; b = c; c = t; t = u1; u1 = u2; u2 = t; t = v1; v1 = v2; v2 = t; }
    f[0] = (u16)a; f[1] = (u16)b; f[2] = (u16)c; f[3] = (u16)slot;
    uv[0] = (u16)u0; uv[1] = (u16)v0; uv[2] = (u16)u1; uv[3] = (u16)v1; uv[4] = (u16)u2; uv[5] = (u16)v2;
    m.nf++;
    }

static const signed char KBoxF[6][4] =
    {
    { 0, 2, 6, 4 }, { 1, 5, 7, 3 }, { 0, 4, 5, 1 }, { 2, 3, 7, 6 }, { 0, 1, 3, 2 }, { 4, 6, 7, 5 }
    };

void R3D::MeshBox(Mesh& m, int aFrame, int aSlot, int px, int py, int pz, int swing, int hx, int hy, int hz)
    {
    MeshBoxR(m, aFrame, aSlot, px, py, pz, swing, 0, hx, hy, hz);
    }

void R3D::MeshBoxR(Mesh& m, int aFrame, int aSlot, int px, int py, int pz, int swing, int roll, int hx, int hy, int hz)
    {
    int base = aFrame == 0 ? m.nv : iMeshCursor;
    if (base + 8 > m.vstride) { iMeshDropped++; m.dropped++; return; }
    if (aFrame == 0) m.nv += 8;
    short* out = m.v + (aFrame * m.vstride + base) * 3;
    int scs = r3d_cos(swing), ssn = r3d_sin(swing);
    int rcs = r3d_cos(roll), rsn = r3d_sin(roll);
    int k;
    int sx = 0, sy = 0, sz = 0;
    for (k = 0; k < 8; k++)
        {
        int lx = (k & 1) ? hx : -hx;
        int ly = (k & 2) ? 0 : -2 * hy;
        int lz = (k & 4) ? hz : -hz;
        // roll about local z
        int xx = (lx * rcs - ly * rsn) >> 14;
        int yy = (lx * rsn + ly * rcs) >> 14;
        // swing about local x
        int y2 = (yy * scs - lz * ssn) >> 14;
        int z2 = (yy * ssn + lz * scs) >> 14;
        out[k * 3] = (short)(xx + px);
        out[k * 3 + 1] = (short)(y2 + py);
        out[k * 3 + 2] = (short)(z2 + pz);
        sx += xx + px; sy += y2 + py; sz += z2 + pz;
        }
    if (aFrame == 0)
        {
        int f;
        for (f = 0; f < 6; f++)
            {
            const signed char* q = KBoxF[f];
            AddFace(m, base + q[0], base + q[1], base + q[2], aSlot, sx / 8, sy / 8, sz / 8, 0, 0, 64, 0, 64, 64);
            AddFace(m, base + q[0], base + q[2], base + q[3], aSlot, sx / 8, sy / 8, sz / 8, 0, 0, 64, 64, 0, 64);
            }
        }
    iMeshCursor = base + 8;
    }

void R3D::MeshCone(Mesh& m, int aFrame, int aSlot, int r0, int y0, int y1, int sides, int rot)
    {
    MeshConeAt(m, aFrame, aSlot, 0, 0, r0, y0, y1, sides, rot);
    }

void R3D::MeshConeAt(Mesh& m, int aFrame, int aSlot, int cx, int cz, int r0, int y0, int y1, int sides, int rot)
    {
    int base = aFrame == 0 ? m.nv : iMeshCursor;
    if (base + sides + 1 > m.vstride) { iMeshDropped++; m.dropped++; return; }
    if (aFrame == 0) m.nv += sides + 1;
    short* out = m.v + (aFrame * m.vstride + base) * 3;
    out[0] = (short)cx; out[1] = (short)y1; out[2] = (short)cz;
    int i, step = 1024 / sides;
    for (i = 0; i < sides; i++)
        {
        int a = i * step + rot;
        out[(i + 1) * 3] = (short)(cx + (r3d_cos(a) * r0 >> 14));
        out[(i + 1) * 3 + 1] = (short)y0;
        out[(i + 1) * 3 + 2] = (short)(cz + (r3d_sin(a) * r0 >> 14));
        }
    if (aFrame == 0)
        for (i = 0; i < sides; i++)
            AddFace(m, base, base + 1 + (i + 1) % sides, base + 1 + i, aSlot, cx, y1 >= y0 ? y0 - 64 : y0 + 64, cz, 32, 0, 64, 64, 0, 64);
    iMeshCursor = base + sides + 1;
    }

// gable roof over a hx * hz footprint (ridge along x), eaves at y0, ridge at y1, overhang aOver
void R3D::MeshRoof(Mesh& m, int aFrame, int aSlot, int hx, int hz, int y0, int y1, int aOver)
    {
    int base = aFrame == 0 ? m.nv : iMeshCursor;
    if (base + 6 > m.vstride) { iMeshDropped++; m.dropped++; return; }
    if (aFrame == 0) m.nv += 6;
    short* o = m.v + (aFrame * m.vstride + base) * 3;
    int X = hx + aOver, Z = hz + aOver;
    int p[6][3] = { { -X, y0, -Z }, { X, y0, -Z }, { X, y0, Z }, { -X, y0, Z }, { -X, y1, 0 }, { X, y1, 0 } };
    int i;
    for (i = 0; i < 6; i++) { o[i * 3] = (short)p[i][0]; o[i * 3 + 1] = (short)p[i][1]; o[i * 3 + 2] = (short)p[i][2]; }
    if (aFrame == 0)
        {
        int cy = y0 - 256;
        // two slopes
        AddFace(m, base + 0, base + 1, base + 5, aSlot, 0, cy, 0, 0, 64, 64, 64, 64, 0);
        AddFace(m, base + 0, base + 5, base + 4, aSlot, 0, cy, 0, 0, 64, 64, 0, 0, 0);
        AddFace(m, base + 3, base + 2, base + 5, aSlot, 0, cy, 0, 0, 64, 64, 64, 64, 0);
        AddFace(m, base + 3, base + 5, base + 4, aSlot, 0, cy, 0, 0, 64, 64, 0, 0, 0);
        // gables (use slot + 1)
        AddFace(m, base + 0, base + 3, base + 4, aSlot + 1, 0, cy, 0, 0, 64, 64, 64, 32, 0);
        AddFace(m, base + 1, base + 2, base + 5, aSlot + 1, 0, cy, 0, 0, 64, 64, 64, 32, 0);
        // underside of the overhang so it is not see-through from below
        AddFace(m, base + 0, base + 1, base + 2, aSlot, 0, y0 + 256, 0, 0, 0, 64, 0, 64, 64);
        AddFace(m, base + 0, base + 2, base + 3, aSlot, 0, y0 + 256, 0, 0, 0, 64, 64, 0, 64);
        }
    iMeshCursor = base + 6;
    }

void R3D::MeshQuad(Mesh& m, int aFrame, int aSlot, const int* P, int cx, int cy, int cz)
    {
    int base = aFrame == 0 ? m.nv : iMeshCursor;
    if (base + 4 > m.vstride) { iMeshDropped++; m.dropped++; return; }
    if (aFrame == 0) m.nv += 4;
    short* o = m.v + (aFrame * m.vstride + base) * 3;
    int i;
    for (i = 0; i < 12; i++) o[i] = (short)P[i];
    if (aFrame == 0)
        {
        AddFace(m, base, base + 1, base + 2, aSlot, cx, cy, cz, 0, 0, 64, 0, 64, 64);
        AddFace(m, base, base + 2, base + 3, aSlot, cx, cy, cz, 0, 0, 64, 64, 0, 64);
        }
    iMeshCursor = base + 4;
    }

void R3D::MeshTri(Mesh& m, int aFrame, int aSlot, const int* P, int cx, int cy, int cz)
    {
    int base = aFrame == 0 ? m.nv : iMeshCursor;
    if (base + 3 > m.vstride) { iMeshDropped++; m.dropped++; return; }
    if (aFrame == 0) m.nv += 3;
    short* o = m.v + (aFrame * m.vstride + base) * 3;
    int i;
    for (i = 0; i < 9; i++) o[i] = (short)P[i];
    if (aFrame == 0) AddFace(m, base, base + 1, base + 2, aSlot, cx, cy, cz, 0, 0, 64, 0, 32, 64);
    iMeshCursor = base + 3;
    }

void R3D::MeshFinish(Mesh& m)
    {
    int fr, i;
    for (fr = 0; fr < (m.oneN ? 1 : m.nfr); fr++)
        for (i = 0; i < m.nf; i++)
            {
            const u16* f = m.f + i * 4;
            const short* V = m.v + fr * m.vstride * 3;
            int nx, ny, nz;
            FaceNormal(V + f[0] * 3, V + f[1] * 3, V + f[2] * 3, nx, ny, nz);
            short* n = m.n + (fr * m.fstride + i) * 3;
            n[0] = (short)nx; n[1] = (short)ny; n[2] = (short)nz;
            }
    int r = 0;
    for (i = 0; i < m.nv; i++)
        {
        const short* v = m.v + i * 3;
        int d = Abs(v[0]) + Abs(v[1]) + Abs(v[2]);
        if (d > r) r = d;
        }
    m.radius = r;
    int mn[3], mx[3];
    mn[0] = mn[1] = mn[2] = 32767; mx[0] = mx[1] = mx[2] = -32767;
    for (i = 0; i < m.nv; i++)
        {
        int k;
        for (k = 0; k < 3; k++)
            {
            int v = m.v[i * 3 + k];
            if (v < mn[k]) mn[k] = v;
            if (v > mx[k]) mx[k] = v;
            }
        }
    m.cx = (mn[0] + mx[0]) / 2; m.cy = (mn[1] + mx[1]) / 2; m.cz = (mn[2] + mx[2]) / 2;
    int ex = (mx[0] - mn[0]) / 2, ey = (mx[1] - mn[1]) / 2, ez = (mx[2] - mn[2]) / 2;
    m.brad = r3d_isqrt(ex * ex + ey * ey + ez * ez);
    }

// ---- organic shapes ----
static int Norm14(int& x, int& y, int& z)
    {
    while (Abs(x) > 30000 || Abs(y) > 30000 || Abs(z) > 30000) { x >>= 1; y >>= 1; z >>= 1; }
    int l = r3d_isqrt(x * x + y * y + z * z);
    if (l == 0) { x = 0; y = 16384; z = 0; return 0; }
    x = x * 16384 / l; y = y * 16384 / l; z = z * 16384 / l;
    return l;
    }

void R3D::MeshLimb(Mesh& m, int aFrame, int aSlot, int x0, int y0, int z0, int x1, int y1, int z1,
                   int r0a, int r0b, int r1a, int r1b, int aSides, int aCaps)
    {
    int base = aFrame == 0 ? m.nv : iMeshCursor;
    if (base + aSides * 2 > m.vstride) { iMeshDropped++; m.dropped++; return; }
    if (aFrame == 0) m.nv += aSides * 2;
    short* out = m.v + (aFrame * m.vstride + base) * 3;
    int dx = x1 - x0, dy = y1 - y0, dz = z1 - z0;
    int nx = dx, ny = dy, nz = dz;
    Norm14(nx, ny, nz);
    // reference axis least parallel to the limb
    int rx = 0, ry = 0, rz = 0;
    if (Abs(ny) > 11000) rz = 16384; else ry = 16384;
    // a = normalise(ref - (ref.n) n) ; b = n x a
    int dot = (rx * nx + ry * ny + rz * nz) >> 14;
    int ax = rx - ((dot * nx) >> 14), ay = ry - ((dot * ny) >> 14), az = rz - ((dot * nz) >> 14);
    Norm14(ax, ay, az);
    int bx = (ny * az - nz * ay) >> 14, by = (nz * ax - nx * az) >> 14, bz = (nx * ay - ny * ax) >> 14;
    int i;
    for (i = 0; i < aSides; i++)
        {
        int t = i * 1024 / aSides;
        int c = r3d_cos(t), s = r3d_sin(t);
        int ox0 = (ax * ((c * r0a) >> 14) + bx * ((s * r0b) >> 14)) >> 14;
        int oy0 = (ay * ((c * r0a) >> 14) + by * ((s * r0b) >> 14)) >> 14;
        int oz0 = (az * ((c * r0a) >> 14) + bz * ((s * r0b) >> 14)) >> 14;
        int ox1 = (ax * ((c * r1a) >> 14) + bx * ((s * r1b) >> 14)) >> 14;
        int oy1 = (ay * ((c * r1a) >> 14) + by * ((s * r1b) >> 14)) >> 14;
        int oz1 = (az * ((c * r1a) >> 14) + bz * ((s * r1b) >> 14)) >> 14;
        out[i * 3] = (short)(x0 + ox0); out[i * 3 + 1] = (short)(y0 + oy0); out[i * 3 + 2] = (short)(z0 + oz0);
        out[(aSides + i) * 3] = (short)(x1 + ox1); out[(aSides + i) * 3 + 1] = (short)(y1 + oy1); out[(aSides + i) * 3 + 2] = (short)(z1 + oz1);
        }
    if (aFrame == 0)
        {
        int mx = (x0 + x1) / 2, my = (y0 + y1) / 2, mz = (z0 + z1) / 2;
        for (i = 0; i < aSides; i++)
            {
            int j = (i + 1) % aSides;
            AddFace(m, base + i, base + j, base + aSides + j, aSlot, mx, my, mz, 0, 0, 64, 0, 64, 64, aCaps & 4);
            AddFace(m, base + i, base + aSides + j, base + aSides + i, aSlot, mx, my, mz, 0, 0, 64, 64, 0, 64, aCaps & 4);
            }
        if (aCaps & 1) for (i = 1; i + 1 < aSides; i++) AddFace(m, base, base + i, base + i + 1, aSlot, mx, my, mz, 32, 0, 0, 64, 64, 64);
        if (aCaps & 2) for (i = 1; i + 1 < aSides; i++) AddFace(m, base + aSides, base + aSides + i, base + aSides + i + 1, aSlot, mx, my, mz, 32, 0, 0, 64, 64, 64);
        }
    iMeshCursor = base + aSides * 2;
    }

void R3D::MeshEllipsoid(Mesh& m, int aFrame, int aSlot, int cx, int cy, int cz, int rx, int ry, int rz, int aSides, int aRings)
    {
    int nv = aSides * aRings + 2;
    int base = aFrame == 0 ? m.nv : iMeshCursor;
    if (base + nv > m.vstride) { iMeshDropped++; m.dropped++; return; }
    if (aFrame == 0) m.nv += nv;
    short* out = m.v + (aFrame * m.vstride + base) * 3;
    out[0] = (short)cx; out[1] = (short)(cy + ry); out[2] = (short)cz;
    int k, i;
    for (k = 0; k < aRings; k++)
        {
        int phi = 256 - (k + 1) * 512 / (aRings + 1);     // latitude from +90 to -90
        int cp = r3d_cos(phi), sp = r3d_sin(phi);
        for (i = 0; i < aSides; i++)
            {
            int t = i * 1024 / aSides + (k & 1) * 512 / aSides;
            short* o = out + (1 + k * aSides + i) * 3;
            o[0] = (short)(cx + ((((r3d_cos(t) * cp) >> 14) * rx) >> 14));
            o[1] = (short)(cy + ((sp * ry) >> 14));
            o[2] = (short)(cz + ((((r3d_sin(t) * cp) >> 14) * rz) >> 14));
            }
        }
    short* bot = out + (nv - 1) * 3;
    bot[0] = (short)cx; bot[1] = (short)(cy - ry); bot[2] = (short)cz;
    if (aFrame == 0)
        {
        for (i = 0; i < aSides; i++)
            AddFace(m, base, base + 1 + i, base + 1 + (i + 1) % aSides, aSlot, cx, cy, cz, 32, 0, 0, 64, 64, 64);
        for (k = 0; k + 1 < aRings; k++)
            for (i = 0; i < aSides; i++)
                {
                int a = base + 1 + k * aSides + i, b = base + 1 + k * aSides + (i + 1) % aSides;
                int c = base + 1 + (k + 1) * aSides + i, d = base + 1 + (k + 1) * aSides + (i + 1) % aSides;
                AddFace(m, a, c, d, aSlot, cx, cy, cz, 0, 0, 0, 64, 64, 64);
                AddFace(m, a, d, b, aSlot, cx, cy, cz, 0, 0, 64, 64, 64, 0);
                }
        int last = base + 1 + (aRings - 1) * aSides;
        for (i = 0; i < aSides; i++)
            AddFace(m, base + nv - 1, last + (i + 1) % aSides, last + i, aSlot, cx, cy, cz, 32, 64, 64, 0, 0, 0);
        }
    iMeshCursor = base + nv;
    }

void R3D::MeshSmooth(Mesh& m)
    {
    m.vn = (short*)Alloc(m.nfr * m.vstride * 3 * 2);
    int fr, i;
    int* acc = iMeshLev;    // scratch is MAXMV ints; accumulate per component in three passes
    short* fn = (short*)iMeshTV;     // scratch for one frame of face normals (MAXMV*sizeof(TV) bytes)
    for (fr = 0; fr < m.nfr; fr++)
        {
        const short* V = m.v + fr * m.vstride * 3;
        for (i = 0; i < m.nf; i++)
            {
            const u16* f = m.f + i * 4;
            int nx, ny, nz;
            FaceNormal(V + f[0] * 3, V + f[1] * 3, V + f[2] * 3, nx, ny, nz);
            fn[i * 3] = (short)nx; fn[i * 3 + 1] = (short)ny; fn[i * 3 + 2] = (short)nz;
            }
        short* vn = m.vn + fr * m.vstride * 3;
        int comp;
        for (comp = 0; comp < 3; comp++)
            {
            for (i = 0; i < m.nv; i++) acc[i] = 0;
            for (i = 0; i < m.nf; i++)
                {
                const u16* f = m.f + i * 4;
                int v = fn[i * 3 + comp];
                acc[f[0]] += v; acc[f[1]] += v; acc[f[2]] += v;
                }
            for (i = 0; i < m.nv; i++) vn[i * 3 + comp] = (short)Clamp(acc[i] >> 2, -32767, 32767);
            }
        for (i = 0; i < m.nv; i++)
            {
            int x = vn[i * 3], y = vn[i * 3 + 1], z = vn[i * 3 + 2];
            Norm14(x, y, z);
            vn[i * 3] = (short)x; vn[i * 3 + 1] = (short)y; vn[i * 3 + 2] = (short)z;
            }
        }
    }

// planar texture mapping, 32 texels per metre... (8 world units per texel), per face along its dominant axis
// box-projected texture coordinates: each face takes the plane it faces most; 1 texel = 2^aShift world units
// (3: a 64-texel tile is 2 m, 2: 1 m). Coordinates are rebased per face and stored as 16 bits, so big faces
// tile properly instead of stretching.
// a model already made (game_kit.cpp: Skyrim's own, simplified): allocated to its exact size, faces as given
void R3D::MeshFromData(Mesh& m, const short* aV, int aNv, const u16* aF, int aNf)
    {
    m.nv = aNv; m.nf = aNf; m.nfr = 1; m.vstride = aNv; m.fstride = aNf; m.radius = 0; m.vn = 0; m.oneN = 0; m.dropped = 0; m.cl = 0; m.ncl = 0; m.lazy = 0; m.uvsh = 0;
    m.v = (short*)Alloc(aNv * 3 * 2);
    m.f = (u16*)Alloc(aNf * 8);
    m.n = (short*)Alloc(aNf * 3 * 2);
    m.uv = (u16*)Alloc(aNf * 12);
    int i;
    for (i = 0; i < aNv * 3; i++) m.v[i] = aV[i];
    for (i = 0; i < aNf * 4; i++) m.f[i] = aF[i];
    iMeshCursor = 0;
    MeshFinish(m);
    MeshPlanarUV(m, 2);
    }

void R3D::MeshFromConst(Mesh& m, const short* aV, int aNv, const u16* aF, int aNf, const short* aN, const u16* aUV)
    {
    m.nv = aNv; m.nf = aNf; m.nfr = 1; m.vstride = aNv; m.fstride = aNf; m.vn = 0; m.oneN = 0; m.dropped = 0; m.cl = 0; m.ncl = 0; m.lazy = 0; m.uvsh = 0;
    m.v = (short*)aV; m.f = (u16*)aF; m.n = (short*)aN; m.uv = (u16*)aUV;      // read only, never written
    int i, k, r = 0, mn[3], mx[3];
    mn[0] = mn[1] = mn[2] = 32767; mx[0] = mx[1] = mx[2] = -32767;
    for (i = 0; i < aNv; i++)
        {
        const short* v = aV + i * 3;
        int d = Abs(v[0]) + Abs(v[1]) + Abs(v[2]);
        if (d > r) r = d;
        for (k = 0; k < 3; k++) { if (v[k] < mn[k]) mn[k] = v[k]; if (v[k] > mx[k]) mx[k] = v[k]; }
        }
    m.radius = r;
    m.cx = (mn[0] + mx[0]) / 2; m.cy = (mn[1] + mx[1]) / 2; m.cz = (mn[2] + mx[2]) / 2;
    int ex = (mx[0] - mn[0]) / 2, ey = (mx[1] - mn[1]) / 2, ez = (mx[2] - mn[2]) / 2;
    m.brad = r3d_isqrt(ex * ex + ey * ey + ez * ez);
    }

// (v0.4.43) a big model's faces in clusters of CLS, as they come (Skyrim's shapes, in order): a sphere round each, the
// cone its faces' normals lie in, the vertices it uses. DrawMesh leaves out a cluster off the screen or turned away
// as a whole - neither its faces looked at nor its vertices worked out. 12 shorts a cluster: centre x y z, radius,
// the cone's axis x y z (Q14), its sin and cos (Q14; cos -32768: no cone), first and last vertex
void R3D::MeshClusters(Mesh& m)
    {
    m.cl = 0; m.ncl = 0; m.lazy = 0;
    if (m.nfr != 1 || m.vn || m.nf > MAXLF || m.nv > MAXMV || m.nf < LAZYMIN) return;
    // (looked at face first only if every face's normal is the way its corners turn: the turned-away test uses it)
    int c, i0;
    for (i0 = 0; i0 < m.nf; i0++)
        {
        const u16* F = m.f + i0 * 4;
        int nx, ny, nz;
        FaceNormal(m.v + F[0] * 3, m.v + F[1] * 3, m.v + F[2] * 3, nx, ny, nz);
        const short* N = m.n + i0 * 3;
        if ((nx | ny | nz) && nx * N[0] + ny * N[1] + nz * N[2] < (1 << 27)) return;      // (under ~60 degrees apart)
        }
    m.lazy = 1;
    if (m.nf < 2 * CLS) return;
    int n = (m.nf + CLS - 1) >> CLSH;
    short* cl = (short*)Alloc(n * CLW * 2);
    for (c = 0; c < n; c++)
        {
        int f0 = c * CLS, f1 = f0 + CLS < m.nf ? f0 + CLS : m.nf, i, k;
        int mn[3] = { 32767, 32767, 32767 }, mx[3] = { -32767, -32767, -32767 }, v0 = 65535, v1 = 0;
        int ax = 0, ay = 0, az = 0;
        for (i = f0; i < f1; i++)
            {
            const u16* F = m.f + i * 4;
            for (k = 0; k < 3; k++)
                {
                const short* V = m.v + F[k] * 3;
                int j;
                for (j = 0; j < 3; j++) { if (V[j] < mn[j]) mn[j] = V[j]; if (V[j] > mx[j]) mx[j] = V[j]; }
                if (F[k] < v0) v0 = F[k];
                if (F[k] > v1) v1 = F[k];
                }
            int nx, ny, nz;
            FaceNormal(m.v + F[0] * 3, m.v + F[1] * 3, m.v + F[2] * 3, nx, ny, nz);
            ax += nx; ay += ny; az += nz;
            }
        int cx = (mn[0] + mx[0]) >> 1, cy = (mn[1] + mx[1]) >> 1, cz = (mn[2] + mx[2]) >> 1, r = 0;
        for (i = f0; i < f1; i++)
            for (k = 0; k < 3; k++)
                {
                const short* V = m.v + m.f[i * 4 + k] * 3;
                int dx = V[0] - cx, dy = V[1] - cy, dz = V[2] - cz;
                int d = r3d_isqrt(dx * dx + dy * dy + dz * dz) + 1;
                if (d > r) r = d;
                }
        // the cone: the normals' mean, and the widest any face's is from it
        while (Abs(ax) > 30000 || Abs(ay) > 30000 || Abs(az) > 30000) { ax >>= 1; ay >>= 1; az >>= 1; }
        int l = r3d_isqrt(ax * ax + ay * ay + az * az), ca = -32768, sa = 0;
        if (l > 0)
            {
            ax = ax * 16384 / l; ay = ay * 16384 / l; az = az * 16384 / l;
            int mind = 16384;
            for (i = f0; i < f1; i++)
                {
                const u16* F = m.f + i * 4;
                int nx, ny, nz;
                FaceNormal(m.v + F[0] * 3, m.v + F[1] * 3, m.v + F[2] * 3, nx, ny, nz);
                if (!(nx | ny | nz)) continue;             // (no area: drawn as nothing, whichever way)
                int d = (nx * ax + ny * ay + nz * az) >> 14;
                if (d < mind) mind = d;
                }
            mind -= 160;                                     // (the normals' rounding: about half a degree wider)
            if (mind > 0) { ca = mind; sa = r3d_isqrt(16384 * 16384 - mind * mind); }
            }
        short* o = cl + c * CLW;
        o[0] = (short)cx; o[1] = (short)cy; o[2] = (short)cz; o[3] = (short)(r > 32767 ? 32767 : r);
        o[4] = (short)ax; o[5] = (short)ay; o[6] = (short)az; o[7] = (short)sa; o[8] = (short)ca;
        o[9] = (short)v0; o[10] = (short)v1; o[11] = 0;
        }
    m.cl = cl; m.ncl = n;
    }

void R3D::MeshPlanarUV(Mesh& m, int aShift)
    {
    int i, k;
    for (i = 0; i < m.nf; i++)
        {
        const u16* f = m.f + i * 4;
        const short* n = m.n + i * 3;
        int ax = Abs(n[0]), ay = Abs(n[1]), az = Abs(n[2]);
        int cu, cv;     // coordinate indices for u and v
        if (ay >= ax && ay >= az) { cu = 0; cv = 2; }
        else if (ax >= az) { cu = 2; cv = 1; }
        else { cu = 0; cv = 1; }
        int u[3], v[3];
        for (k = 0; k < 3; k++)
            {
            const short* p = m.v + f[k] * 3;
            u[k] = p[cu] >> aShift; v[k] = -(p[cv] >> aShift);
            }
        int mu = u[0], mv = v[0];
        for (k = 1; k < 3; k++) { if (u[k] < mu) mu = u[k]; if (v[k] < mv) mv = v[k]; }
        int bu = mu & ~63, bv = mv & ~63;
        u16* uv = m.uv + i * 6;
        for (k = 0; k < 3; k++)
            {
            uv[k * 2] = (u16)Clamp(u[k] - bu, 0, 4095);
            uv[k * 2 + 1] = (u16)Clamp(v[k] - bv, 0, 4095);
            }
        }
    }

// smooth-only mesh: face normals are stored for frame 0 only (MeshSmooth computes the rest)
void R3D::MeshInitSmooth(Mesh& m, int aMaxV, int aMaxF, int aFrames)
    {
    if (aMaxV > 512) aMaxV = 512;       // (meshes built here: 512 vertices at most, as before MAXMV grew)
    if (aMaxF < aMaxV * 2) aMaxF = aMaxV * 2;
    m.nv = 0; m.nf = 0; m.nfr = aFrames; m.vstride = aMaxV; m.fstride = aMaxF; m.radius = 0; m.vn = 0;
    m.v = (short*)Alloc(aMaxV * 3 * 2 * aFrames);
    m.f = (u16*)Alloc(aMaxF * 8);
    m.n = (short*)Alloc(aMaxF * 3 * 2);
    m.uv = (u16*)Alloc(aMaxF * 12);
    m.oneN = 1; m.dropped = 0; m.cl = 0; m.ncl = 0; m.lazy = 0; m.uvsh = 0;
    iMeshCursor = 0;
    }

// loft through rings: per ring centre, side axis a (Q14), front axis b (Q14), radii ra/rb, slot of the band above it
void R3D::MeshRings(Mesh& m, int aFrame, const int* R, int aRings, int aSides, int aCaps)
    {
    int base = aFrame == 0 ? m.nv : iMeshCursor;
    int nv = aRings * aSides;
    if (base + nv > m.vstride) { iMeshDropped++; m.dropped++; return; }
    if (aFrame == 0) m.nv += nv;
    short* out = m.v + (aFrame * m.vstride + base) * 3;
    int k, i;
    for (k = 0; k < aRings; k++)
        {
        const int* r = R + k * 12;
        for (i = 0; i < aSides; i++)
            {
            int t = i * 1024 / aSides;
            int c = (r3d_cos(t) * r[9]) >> 14, s = (r3d_sin(t) * r[10]) >> 14;
            short* o = out + (k * aSides + i) * 3;
            o[0] = (short)(r[0] + ((r[3] * c + r[6] * s) >> 14));
            o[1] = (short)(r[1] + ((r[4] * c + r[7] * s) >> 14));
            o[2] = (short)(r[2] + ((r[5] * c + r[8] * s) >> 14));
            }
        }
    if (aFrame == 0)
        {
        for (k = 0; k + 1 < aRings; k++)
            {
            const int* r0 = R + k * 12; const int* r1 = r0 + 12;
            int mx = (r0[0] + r1[0]) / 2, my = (r0[1] + r1[1]) / 2, mz = (r0[2] + r1[2]) / 2;
            int slot = r0[11];
            for (i = 0; i < aSides; i++)
                {
                int j = (i + 1) % aSides;
                int a = base + k * aSides;
                AddFace(m, a + i, a + j, a + aSides + j, slot, mx, my, mz, 0, 0, 64, 0, 64, 64);
                AddFace(m, a + i, a + aSides + j, a + aSides + i, slot, mx, my, mz, 0, 0, 64, 64, 0, 64);
                }
            }
        // caps: centre slightly inside the loft
        if (aCaps & 1)
            {
            const int* r1 = R + 12;
            for (i = 1; i + 1 < aSides; i++) AddFace(m, base, base + i, base + i + 1, R[11], r1[0], r1[1], r1[2], 32, 0, 0, 64, 64, 64);
            }
        if (aCaps & 2)
            {
            const int* r0 = R + (aRings - 2) * 12; const int* rl = R + (aRings - 1) * 12;
            int b2 = base + (aRings - 1) * aSides;
            for (i = 1; i + 1 < aSides; i++) AddFace(m, b2, b2 + i, b2 + i + 1, rl[11], r0[0], r0[1], r0[2], 32, 0, 0, 64, 64, 64);
            }
        }
    iMeshCursor = base + nv;
    }

// shared face loop: vertices already in iMeshTV (camera space, outcoded, projected)
#ifdef R3D_STATS
// (host builds: what each model puts on the screen - triangles drawn, pixels, times drawn)
int g_arenaOver1, g_arenaFree1; int g_clTot;
#ifdef R3D_STATS
int g_triId, g_triVisMark = -1, g_triVis, g_triOcc, g_triOccN, g_triVisOcc;     // (v0.4.75) triangles set up, and those with a pixel that showed
#endif
 R3DStat g_r3dStat[256]; int g_r3dPixFlat; int g_r3dStage[12]; int g_r3dRows[3]; int g_r3dStatN;
static R3DStat* StatOf(const Mesh* m)
    {
    int i;
    for (i = 0; i < g_r3dStatN; i++) if (g_r3dStat[i].m == m) return &g_r3dStat[i];
    if (g_r3dStatN >= 256) return &g_r3dStat[255];
    R3DStat& s = g_r3dStat[g_r3dStatN++]; s.m = m; s.tris = s.faces = s.calls = s.pix = 0; return &s;
    }
#endif
// (v0.4.71) a slot's picture byte: whose picture (the area's, the library's), whether there is one, a cut-out, its
// bigger ones - all DrawFaces asks of it before the face's own sizes come in. Bits: 0 sky (a Skyrim picture, loaded),
// 1 lib, 2 key (cut out), 3 tx_ok, 4 a 128 one, 5 a 256 one, 8..13 its number, 15 worked out
enum { SI_SKY = 1, SI_LIB = 2, SI_KEY = 4, SI_TXOK = 8, SI_HI = 16, SI_HI2 = 32, SI_DONE = 0x8000 };
int R3D::SlotInfo(int b)
    {
    int lib = (b & SL_SKY) == 0x40, ti = b & 63;
    int sky = (b & SL_SKY) == SL_SKY || lib;
    const u8* bKey = lib ? iLibKey : iSkyKey;
    if (sky && ti >= (lib ? iLibN : iSkyN)) sky = 0;
    int key = sky && bKey && bKey[ti];
    int txok = (b & 0x80) && !(b & 0x40);         // (a picture of the game's own)
    int hi = ti < (lib ? iLibHiN : iSkyHiN);
    int hi2 = (lib ? iLibHi2 != 0 : iSkyHi2 != 0) && ti < (lib ? iLibHi2N : iSkyHi2N);
    int v = SI_DONE | (ti << 8) | (sky ? SI_SKY : 0) | (lib ? SI_LIB : 0) | (key ? SI_KEY : 0) | (txok ? SI_TXOK : 0) | (hi ? SI_HI : 0) | (hi2 ? SI_HI2 : 0);
    iSlotInfo[b] = (u16)v;
    return v;
    }

// (v0.4.73) the table made again when the pictures it describes are not the ones now in use (the areas' pictures are
// swapped in and out while a frame is drawn: a table made for another's would choose pictures that are not there)
void R3D::SlotInfoCheck()
    {
    if (iSlotKeyP[0] == iSkyKey && iSlotKeyP[1] == iSkyHi2 && iSlotKeyP[2] == iLibKey && iSlotKeyP[3] == iLibHi2 && iSlotKeyP[4] == iSkyTex &&
        iSlotKeyN[0] == iSkyN && iSlotKeyN[1] == iSkyHiN && iSlotKeyN[2] == iSkyHi2N && iSlotKeyN[3] == iLibN && iSlotKeyN[4] == iLibHiN &&
        iSlotKeyN[5] == iLibHi2N && (iSlotInfo[0] & SI_DONE)) return;
    int b;
    for (b = 0; b < 256; b++) SlotInfo(b);
    iSlotKeyP[0] = iSkyKey; iSlotKeyP[1] = iSkyHi2; iSlotKeyP[2] = iLibKey; iSlotKeyP[3] = iLibHi2; iSlotKeyP[4] = iSkyTex;
    iSlotKeyN[0] = iSkyN; iSlotKeyN[1] = iSkyHiN; iSlotKeyN[2] = iSkyHi2N; iSlotKeyN[3] = iLibN; iSlotKeyN[4] = iLibHiN; iSlotKeyN[5] = iLibHi2N;
    }

void R3D::DrawFaces(const Mesh& m, int aFrame, int lx, int ly, int lz, const u8* aSlots, int aFlags)
    {
#ifdef R3D_STATS
    R3DStat* st = StatOf(&m); int t0 = iTris, p0 = iPixels; st->calls++; st->faces += m.nf;
    DrawFacesX(m, aFrame, lx, ly, lz, aSlots, aFlags);
    st->tris += iTris - t0; st->pix += iPixels - p0;
    }
void R3D::DrawFacesX(const Mesh& m, int aFrame, int lx, int ly, int lz, const u8* aSlots, int aFlags)
    {
#endif
    const short* N = m.n + (m.oneN ? 0 : aFrame * m.fstride * 3);
    const u16* F = m.f;
    const u16* UV = m.uv;
    TV* tv = iMeshTV;
    int tex = iMode == MODE_TEX;
    int pl = iMeshNearLight && !(aFlags & MF_UNLIT);
    int i;
    if (m.vn && !tex)
        {
        // smooth: light every vertex once, then Gouraud faces
        const short* VN = m.vn + aFrame * m.vstride * 3;
        for (i = 0; i < m.nv; i++, VN += 3)
            {
            int lev;
            if (aFlags & MF_UNLIT) lev = 256;
            else
                {
                int dot = (VN[0] * lx + VN[1] * ly + VN[2] * lz) >> 14;
                if (dot < 0) dot = 0;
                lev = (80 + (dot * 190 >> 14)) * iLight >> 8;
                if (lev < 34) lev = 34;
                lev += iAmbient;
                if (pl) lev += PointLight(tv[i].x, tv[i].y, tv[i].z);
                if (lev > 300) lev = 300;
                }
            iMeshLev[i] = lev;
            }
        for (i = 0; i < m.nf; i++, F += 4)
            {
            TV* v[3];
            v[0] = &tv[F[0]]; v[1] = &tv[F[1]]; v[2] = &tv[F[2]];
            if (v[0]->code & v[1]->code & v[2]->code & 31) continue;
            if (!((v[0]->code | v[1]->code | v[2]->code) & 32))
                {
                int cr = (v[1]->sx - v[0]->sx) * (v[2]->sy - v[0]->sy) - (v[1]->sy - v[0]->sy) * (v[2]->sx - v[0]->sx);
                if (cr <= 0) continue;
                }
            const u8* sl = aSlots + F[3] * 4;
            int k;
            for (k = 0; k < 3; k++)
                {
                int lev = iMeshLev[F[k]];
                int r = Clamp(sl[0] * lev >> 8, 2, 253), g = Clamp(sl[1] * lev >> 8, 2, 253), bb = Clamp(sl[2] * lev >> 8, 2, 253);
                r = r * iLightR >> 8; g = g * iLightG >> 8; bb = bb * iLightB >> 8;
                if (aFlags & MF_FLASH) { r = (r + 255) >> 1; g = (g + 220) >> 1; bb = (bb + 200) >> 1; }
                int fo = (aFlags & MF_NOFOG) ? 0 : FogF(v[k]->z);
                v[k]->a[0] = (r << 8) + (iFogR - r) * fo;
                v[k]->a[1] = (g << 8) + (iFogG - g) * fo;
                v[k]->a[2] = (bb << 8) + (iFogB - bb) * fo;
                }
            DrawTri(v[0], v[1], v[2], K_GOURAUD);
            }
        return;
        }
    const int us = m.uvsh;                        // (v0.4.46: Skyrim's models' coordinates in quarter texels)
    // (LazyFaces: only the faces in its list; the face's arrays found from its number - nothing kept stepping)
    SlotInfoCheck();
    const u16* FL = iClOn ? iFaceList : 0;
    const u16* F0 = F; const short* N0 = N; const u16* UV0 = UV;
    int nl = FL ? iFaceN : m.nf, j, pre = 0;
#ifdef USE_ASM
    // (v0.4.49) the first tests in ARM code, ahead: only the faces that pass come round
    if (FL || m.nf <= MAXLF) { nl = face_cull(F0, tv, FL, nl, iFaceList); FL = iFaceList; pre = 1; }
    if (pre && !tex && !(iSkip & 32768))
        {
        // (v0.4.73) each face's picture, sharpness, light, fog, palette worked out in ARM code (face_shade), in
        // batches of records; here only the triangles set up from them
        if (pl && iMeshLightAdd < 0 && (iLightListMask != iLightMask || iLightListFrame != iFrameCount)) SetLightList();
        FaceCtx& c = iFaceCtx;
        c.F = F0; c.N = N0; c.UV = UV0; c.tv = tv; c.FL = FL; c.nl = nl; c.j = 0;
        c.slots = aSlots; c.si = iSlotInfo; c.lx = lx; c.ly = ly; c.lz = lz; c.flags = aFlags; c.light = iLight; c.amb = iAmbient;
        c.pl = pl; c.mla = iMeshLightAdd; c.ll = iLightList; c.ln = iLightListN; c.us = us; c.texdist = iTexDist; c.notex = iNoTex;
        c.texmip = iTexMip; c.dbl = iDouble; c.fog0 = iFog0; c.fogK = iFogK; c.fogSq = iFogSq; c.fogR = iFogR; c.fogG = iFogG; c.fogB = iFogB;
        c.libShade = iLibShade; c.skyShade = iSkyShade; c.shadePal = iShadePal; c.libHi2 = iLibHi2; c.skyHi2 = iSkyHi2;
        c.libHi = iLibHi; c.skyHi = iSkyHi; c.libTex1 = iLibTex1; c.libTex = iLibTex; c.skyTex32 = iSkyTex32; c.skyTex16 = iSkyTex16;
        c.skyTex1 = iSkyTex1; c.skyTex = iSkyTex; c.tex = iTex; c.out = iFaceRec; c.outEnd = iFaceRec + 64 * 3;
        c.lr = iLightR; c.lg = iLightG; c.lb = iLightB;
        int nr;
        while ((nr = face_shade(&c)) > 0)
            {
            const int* R = iFaceRec;
            for (; nr > 0; nr--, R += 3)
                {
                int r0 = R[0];
                i = r0 & 0xFFFF;
                F = F0 + i * 4;
                TV* a = &tv[F[0]]; TV* b_ = &tv[F[1]]; TV* c_ = &tv[F[2]];
                int ft = (r0 >> 16) & 1;
                if (ft)
                    {
                    int sh = (r0 >> 24) & 15;
                    UV = UV0 + i * 6;
                    iCurPal = (u16*)R[1]; iCurTex = (const u8*)R[2]; iTexQ = (r0 >> 20) & 15;
                    a->a[0] = UV[0] << sh; a->a[1] = UV[1] << sh;
                    b_->a[0] = UV[2] << sh; b_->a[1] = UV[3] << sh;
                    c_->a[0] = UV[4] << sh; c_->a[1] = UV[5] << sh;
                    }
                else iFlatCol = R[1];
                if ((a->code | b_->code | c_->code) & 32) DrawTri(a, b_, c_, ft ? K_TEX : K_FLAT);
                else if (iOccOn && OccHiddenTri(a, b_, c_)) iOccTris++;
                else if (ft) Raster(a, b_, c_, K_TEX);
                else RasterFlat(a, b_, c_);
                iTexQ = 0;
                }
            }
        return;
        }
#endif
    for (j = 0; j < nl; j++)
        {
        i = FL ? FL[j] : j;
        F = F0 + i * 4; N = N0 + i * 3; UV = UV0 + i * 6;
        TV* a = &tv[F[0]]; TV* b = &tv[F[1]]; TV* c = &tv[F[2]];
        if (pre) { }
        else if (a->code & b->code & c->code & 31) continue;
        else if (!((a->code | b->code | c->code) & 32))
            {
            int cr = (b->sx - a->sx) * (c->sy - a->sy) - (b->sy - a->sy) * (c->sx - a->sx);
            if (cr <= 0) continue;
            // (under a fifth of a pixel: nothing to see, and its setting up costs as much as a big one's)
            if (cr < 100) continue;
            // (v0.4.43) no row's middle crossed: no pixel drawn - left before its colour is worked out
            int mn = a->sy, mx = a->sy;
            if (b->sy < mn) mn = b->sy; else mx = b->sy;
            if (c->sy < mn) mn = c->sy; else if (c->sy > mx) mx = c->sy;
            if (((mn + 7) >> 4) == ((mx + 7) >> 4)) continue;
            }
        const u8* sl = aSlots + F[3] * 4;
        // (v0.4.56) whose picture: the area's (0xC0 + n) or the shared library's (0x40 + n) - (v0.4.71) as SlotInfo found
        int si = iSlotInfo[sl[3]];
        if (!si) si = SlotInfo(sl[3]);
        int lib = si & SI_LIB, ti = (si >> 8) & 63;
        int sky = si & SI_SKY;
        int blur = 0, hiq = 0;
        int key = si & SI_KEY;                      // (a cut-out: always a sharp picture, near or far)
        if (sky && !tex && !iNoTex)
            {
            // how many texels to a pixel (texels: the texture coordinates' area, in a 64 picture's - the models'
            // coordinates are in quarter texels: uvsh; pixels: the screen's, 1/16 pixel): about one, sharp; up to 24,
            // the blurred picture; more, flat in its average colour (a texture shrunk that far only sparkles, and costs
            // as much as a big one); magnified, the 128 picture, and more, the 256 (v0.4.46: the cut-out ones too)
            int du1 = UV[2] - UV[0], dv1 = UV[3] - UV[1], du2 = UV[4] - UV[0], dv2 = UV[5] - UV[1];
            int tcr = du1 * dv2 - dv1 * du2; if (tcr < 0) tcr = -tcr;     // texel^2 (x2)
            tcr >>= us * 2;
            int scr = ((a->code | b->code | c->code) & 32) ? 0 : (b->sx - a->sx) * (c->sy - a->sy) - (b->sy - a->sy) * (c->sx - a->sx);   // 1/256 px^2 (x2)
            if (scr > 0)
                {
                if (key) { }
                else if (tcr > (scr * 3) >> 5) sky = 0;          // (24 texels a pixel: far, or seen edge on)
                else if (!lib && iTexMip >= 2 && iSkyTex16 && tcr > (scr * 3) >> 7) blur = 3;     // (6..24: the 16 x 16 picture)
                else if (tcr > (scr * 3) >> 9) blur = !lib && iTexMip >= 1 && iSkyTex32 ? 2 : 1;  // (1.5..6: the 32 x 32, or the blurred 64)
                if (!blur && sky && tcr < scr >> 9 && (si & SI_HI) && (UV[0] | UV[1] | UV[2] | UV[3] | UV[4] | UV[5]) < (512 << us) && !(key && iDouble))
                    {
                    hiq = 1;                                                       // (magnified: half a texel a pixel or less)
                    if ((si & SI_HI2) && !iDouble && tcr < scr >> 11) hiq = 2;    // (an eighth: the 256)
                    }
                }
            }
        int tx_ok = sky || (si & SI_TXOK);                             // (a Skyrim texture not loaded: flat)
        int zn = a->z < b->z ? a->z : b->z; if (c->z < zn) zn = c->z;      // (its nearest corner: a face turns flat as a whole)
        int ft = tex ? (!(sl[3] & 0x80) || tx_ok) : (tx_ok && zn < iTexDist && !iNoTex);     // far textured faces (or textures off): flat in the slot colour
        if (key && tx_ok) ft = 1;
        int lev;
        if (aFlags & MF_UNLIT) lev = 256;
        else
            {
            int dot = (N[0] * lx + N[1] * ly + N[2] * lz) >> 14;
            if (dot < 0) dot = 0;
            lev = 80 + (dot * 190 >> 14);
            lev = lev * iLight >> 8;
            if (lev < 34) lev = 34;
            lev += iAmbient;
            if (pl) lev += iMeshLightAdd >= 0 ? iMeshLightAdd : PointLight(((a->x + b->x + c->x) * 85) >> 8, ((a->y + b->y + c->y) * 85) >> 8, ((a->z + b->z + c->z) * 85) >> 8);
            if (lev > 300) lev = 300;
            }
        if (ft)
            {
            int f = (aFlags & MF_NOFOG) ? 0 : (FogF(a->z) >> 5 > 7 ? 7 : FogF(a->z) >> 5);
            int sh = 8 - us;
            iTexQ = 0;
            if (sky)
                {
                iCurPal = (lib ? iLibShade : iSkyShade) + (Clamp((lev - 40) >> 4, 0, 15) * 8 + f) * 256;
                // (iTexQ: 0 the 64, 1 the 128, 2 the 64 cut out, 3 the 256, 4 the 128 cut out, 5 the 256 cut out)
                if (hiq == 2) { iCurTex = (lib ? iLibHi2 : iSkyHi2) + ti * 65536; iTexQ = key ? 5 : 3; sh = 10 - us; }
                else if (hiq) { iCurTex = (lib ? iLibHi : iSkyHi) + ti * 16384; iTexQ = key ? 4 : 1; sh = 9 - us; }
                else if (lib) { iCurTex = (blur && iLibTex1 ? iLibTex1 : iLibTex) + ti * 4096; if (key) iTexQ = 2; }
                else
                    {
                    if (blur == 2) { iCurTex = iSkyTex32 + (sl[3] & 63) * 4096; sh = 7 - us; }     // (laid out 64 wide, tiled: the same mask)
                    else if (blur == 3) { iCurTex = iSkyTex16 + (sl[3] & 63) * 4096; sh = 6 - us; }
                    else iCurTex = (blur && iSkyTex1 ? iSkyTex1 : iSkyTex) + (sl[3] & 63) * 4096;
                    if (key) iTexQ = 2;
                    }
                }
            else
                {
                iCurPal = iShadePal + (Clamp((lev - 40) >> 4, 0, 15) * 8 + f) * 256;
                iCurTex = iTex + (sl[3] & 31) * 4096;
                }
            a->a[0] = UV[0] << sh; a->a[1] = UV[1] << sh;
            b->a[0] = UV[2] << sh; b->a[1] = UV[3] << sh;
            c->a[0] = UV[4] << sh; c->a[1] = UV[5] << sh;
            }
        else
            {
            int r = Clamp(sl[0] * lev >> 8, 2, 253), g = Clamp(sl[1] * lev >> 8, 2, 253), bb = Clamp(sl[2] * lev >> 8, 2, 253);
            r = r * iLightR >> 8; g = g * iLightG >> 8; bb = bb * iLightB >> 8;            // (v0.4.74: the light's colour)
            if (aFlags & MF_FLASH) { r = (r + 255) >> 1; g = (g + 220) >> 1; bb = (bb + 200) >> 1; }
            if (!(aFlags & MF_NOFOG))
                {
                int fo = FogF(a->z);
                r += (iFogR - r) * fo >> 8; g += (iFogG - g) * fo >> 8; bb += (iFogB - bb) * fo >> 8;
                }
            iFlatCol = ::Rgb(r, g, bb);
            }
        // (v0.4.43: on the screen whole - the tests DrawTri makes already made: straight to the rasteriser)
        if ((a->code | b->code | c->code) & 32) DrawTri(a, b, c, ft ? K_TEX : K_FLAT);
        else if (iOccOn && OccHiddenTri(a, b, c)) iOccTris++;
        else if (ft) Raster(a, b, c, K_TEX);
        else RasterFlat(a, b, c);
        iTexQ = 0;
        }
    }

// (v0.4.43) whether all a cluster's faces are turned away: for every normal n in its cone, n.(eye - p) < 0 for every p
// within its sphere (U: the model's turn into the camera's space, unscaled; p: its middle there, cr its radius)
static int ClusterAway(const short* C, const int* U, int px, int py, int pz, int cr)
    {
    if (C[8] == -32768) return 0;
    int dx = -px >> 4, dy = -py >> 4, dz = -pz >> 4, rr = cr >> 4;
    if (Abs(dx) >= 30000 || Abs(dy) >= 30000 || Abs(dz) >= 30000) return 0;
    int L2 = dx * dx + dy * dy + dz * dz;
    if (L2 <= rr * rr) return 0;
    int ax = C[4], ay = C[5], az = C[6];
    int bx = (U[0] * ax + U[2] * az) >> 14;
    int by = (U[3] * ax + U[4] * ay + U[5] * az) >> 14;
    int bz = (U[6] * ax + U[7] * ay + U[8] * az) >> 14;
    int ad = bx * dx + by * dy + bz * dz;                    // Q14 x units/16
    int q = r3d_isqrt(L2 - rr * rr);
    // (only while the cone and the sphere's angle together stay under a right angle: cos(a+b) > 0)
    if ((C[8] >> 2) * (q >> 2) <= (C[7] >> 2) * (rr >> 2) + 4096) return 0;
    return ad < -(C[7] * q + C[8] * rr) - 16384;
    }

// (v0.4.43) which of a big model's clusters are seen (iClVis), and how many: off the screen, behind the walls, or all
// their faces turned away - left out. K: model -> camera (scaled), t: the model's origin in the camera's space
int R3D::ClusterCull(const Mesh& m, const int* K, const TV& t, int aScale, int cs, int sn)
    {
    int U[9];
    U[0] = (iM[0] * cs - iM[2] * sn) >> 14; U[1] = 0; U[2] = (iM[0] * sn + iM[2] * cs) >> 14;
    U[3] = (iM[3] * cs - iM[5] * sn) >> 14; U[4] = iM[4]; U[5] = (iM[3] * sn + iM[5] * cs) >> 14;
    U[6] = (iM[6] * cs - iM[8] * sn) >> 14; U[7] = iM[7]; U[8] = (iM[6] * sn + iM[8] * cs) >> 14;
    const short* C = m.cl;
    int c, nvis = 0;
    for (c = 0; c < m.ncl; c++, C += CLW)
        {
        int x = C[0], y = C[1], z = C[2];
        TV p;
        p.x = ((K[0] * x + K[2] * z) >> 14) + t.x;
        p.y = ((K[3] * x + K[4] * y + K[5] * z) >> 14) + t.y;
        p.z = ((K[6] * x + K[7] * y + K[8] * z) >> 14) + t.z;
        int cr = (C[3] * aScale >> 8) + 1;
        int vis = 0;
        if (p.z >= -cr && !(p.z * iHW + p.x * iFx < -cr * iLenX) && !(p.z * iHW - p.x * iFx < -cr * iLenX)
            && !(p.z * iHH - p.y * iFy < -cr * iLenY) && !(p.z * iHH + p.y * iFy < -cr * iLenY))
            {
            vis = !ClusterAway(C, U, p.x, p.y, p.z, cr);
            if (vis && iOccOn && OccHiddenSphere(p, cr)) vis = 0;
            }
        iClVis[c] = (u8)vis;
        nvis += vis;
        }
    iClSkipped += m.ncl - nvis;
#ifdef R3D_STATS
    { extern int g_clTot; g_clTot += m.ncl; }
#endif
    return nvis;
    }

// (v0.4.43) LazyFaces' three loops. The faces not turned away (nor in a cluster left out): iFaceList
int R3D::LazyCull(const Mesh& m, int ex, int ey, int ez, int sh, int margin)
    {
    u16* L = iFaceList;
    const short* V = m.v;
    const u16* F = m.f;
    const short* N = m.n;
    const u8* cv = m.ncl ? iClVis : 0;
    int nf = m.nf, i, n = 0;
    for (i = 0; i < nf; i++, F += 4, N += 3)
        {
        if (cv && !cv[i >> CLSH]) { int s_ = CLS - 1 - (i & (CLS - 1)); i += s_; F += s_ * 4; N += s_ * 3; continue; }
        const short* A = V + F[0] * 3;
        int d = N[0] * ((A[0] - ex) >> sh) + N[1] * ((A[1] - ey) >> sh) + N[2] * ((A[2] - ez) >> sh);
        if (d <= margin) L[n++] = (u16)i;
        }
    iFaceN = n;
    return n;
    }
// the vertices the faces left use, each once
int R3D::LazyList(const Mesh& m, int st)
    {
    const u16* FL = iFaceList;
    const u16* F0 = m.f;
    int* stamp = iMeshLev;
    u16* L = iLazyList;
    int nf = iFaceN, i, n = 0;
    for (i = 0; i < nf; i++)
        {
        const u16* F = F0 + FL[i] * 4;
        int a = F[0], b = F[1], c = F[2];
        if (stamp[a] != st) { stamp[a] = st; L[n++] = (u16)a; }
        if (stamp[b] != st) { stamp[b] = st; L[n++] = (u16)b; }
        if (stamp[c] != st) { stamp[c] = st; L[n++] = (u16)c; }
        }
    return n;
    }
// (v0.4.49) xform_v's numbers (spans.s)
void R3D::XfCtx(int* X, int c0, int c2, int c3, int c4, int c5, int c6, int c7, int c8, const TV& t)
    {
    X[0] = c0; X[1] = c2; X[2] = c3; X[3] = c4; X[4] = c5; X[5] = c6; X[6] = c7; X[7] = c8;
    X[8] = t.x; X[9] = t.y; X[10] = t.z; X[11] = iHW; X[12] = iHH; X[13] = iFx; X[14] = iFy;
    X[15] = (int)(long)iRecip; X[16] = iCx16; X[17] = iCy16; X[18] = iFx16; X[19] = iFy16; X[20] = NEAR;
    }
void R3D::LazyXform(const Mesh& m, int aN, const int* K, const TV& t)
    {
    int c0 = K[0], c2 = K[2], c3 = K[3], c4 = K[4], c5 = K[5], c6 = K[6], c7 = K[7], c8 = K[8];
    TV* tv = iMeshTV;
    const short* V0 = m.v;
    const u16* L = iLazyList;
    int i;
#ifdef USE_ASM
    {
    int X[21];
    XfCtx(X, c0, c2, c3, c4, c5, c6, c7, c8, t);
    xform_v(V0, tv, aN, X, L);
    for (i = 0; i < aN; i++) { TV& v = tv[L[i]]; if (v.code == -1) { Outcode(v); if (!v.code) Project(v); } }
    return;
    }
#endif
    for (i = 0; i < aN; i++)
        {
        int j = L[i];
        const short* V = V0 + j * 3;
        int x = V[0], y = V[1], z = V[2];
        TV& v = tv[j];
        v.x = ((c0 * x + c2 * z) >> 14) + t.x;
        v.y = ((c3 * x + c4 * y + c5 * z) >> 14) + t.y;
        v.z = ((c6 * x + c7 * y + c8 * z) >> 14) + t.z;
        Outcode(v);
        if (!v.code) Project(v);
        }
    }

// (v0.4.43) a model whose normals agree with its windings: its faces looked at first - in clusters off the screen or
// turned away (ClusterCull), or turned away themselves (the eye taken into the model's own space: a face's normal
// against the eye from a corner) - left out (iFaceVis 0); only the vertices the rest use worked out (a stamp each, in
// iMeshLev: its flat faces never use it). How many faces are left (0: nothing to draw)
int R3D::LazyFaces(const Mesh& m, const int* K, const TV& t, int aScale, int cs, int sn)
    {
    int U[9];
    U[0] = (iM[0] * cs - iM[2] * sn) >> 14; U[1] = 0; U[2] = (iM[0] * sn + iM[2] * cs) >> 14;
    U[3] = (iM[3] * cs - iM[5] * sn) >> 14; U[4] = iM[4]; U[5] = (iM[3] * sn + iM[5] * cs) >> 14;
    U[6] = (iM[6] * cs - iM[8] * sn) >> 14; U[7] = iM[7]; U[8] = (iM[6] * sn + iM[8] * cs) >> 14;
    if (m.ncl && !ClusterCull(m, K, t, aScale, cs, sn)) return 0;
    // the eye in the model's space: R^T (-t) / scale
    long long ex_ = -((long long)U[0] * t.x + (long long)U[3] * t.y + (long long)U[6] * t.z);
    long long ey_ = -((long long)U[1] * t.x + (long long)U[4] * t.y + (long long)U[7] * t.z);
    long long ez_ = -((long long)U[2] * t.x + (long long)U[5] * t.y + (long long)U[8] * t.z);
    int ex = (int)(ex_ >> 14), ey = (int)(ey_ >> 14), ez = (int)(ez_ >> 14);
    if (aScale != 256) { ex = (ex << 8) / aScale; ey = (ey << 8) / aScale; ez = (ez << 8) / aScale; }
    int big = Abs(ex) > Abs(ey) ? Abs(ex) : Abs(ey);
    if (Abs(ez) > big) big = Abs(ez);
    big += m.radius;
    int sh = 0;
    while ((big >> sh) > 32000) sh++;
    const int margin = 1 << 17;              // (the rounding: turned away by more than this, for sure)
    // (three plain loops - GCC 2.95 keeps each one's few values in registers): the faces; the vertices they use, each
    // once (a stamp: iMeshLev); those worked out
    int left = LazyCull(m, ex, ey, ez, sh, margin);
    if (!left) return 0;
    int st = ++iStampN, i;
    if (st > -0x40000000) { for (i = 0; i < MAXMV; i++) iMeshLev[i] = 0; st = iStampN = -0x7FFFFFFF; }
    int nl = LazyList(m, st);
    LazyXform(m, nl, K, t);
    return left;
    }

void R3D::DrawMesh(const Mesh& m, int aFrame, int ox, int oy, int oz, int aYaw, int aScale, const u8* aSlots, int aFlags)
    {
    int cs = r3d_cos(aYaw), sn = r3d_sin(aYaw);
    int c0 = (iM[0] * cs - iM[2] * sn) >> 14, c2 = (iM[0] * sn + iM[2] * cs) >> 14;
    int c3 = (iM[3] * cs - iM[5] * sn) >> 14, c4 = iM[4], c5 = (iM[3] * sn + iM[5] * cs) >> 14;
    int c6 = (iM[6] * cs - iM[8] * sn) >> 14, c7 = iM[7], c8 = (iM[6] * sn + iM[8] * cs) >> 14;
    if (aScale != 256)
        {
        c0 = c0 * aScale >> 8; c2 = c2 * aScale >> 8;
        c3 = c3 * aScale >> 8; c4 = c4 * aScale >> 8; c5 = c5 * aScale >> 8;
        c6 = c6 * aScale >> 8; c7 = c7 * aScale >> 8; c8 = c8 * aScale >> 8;
        }
    TV t;
    TransformPt(ox, oy, oz, t);
    // whole-mesh frustum reject; and what would be a pixel or less across (a cup across the hall)
    int rad = (m.radius * aScale) >> 8;
    if (t.z < -rad) return;
    if (t.z > rad && rad * iFx < t.z) return;
    if (t.z * iHW + t.x * iFx < -rad * iLenX) return;
    if (t.z * iHW - t.x * iFx < -rad * iLenX) return;
    if (t.z * iHH - t.y * iFy < -rad * iLenY) return;
    if (t.z * iHH + t.y * iFy < -rad * iLenY) return;
    if (iOccOn && OccHiddenSphere(t, rad)) { iOccSkipped++; return; }      // behind the walls already drawn
    const short* V = m.v + aFrame * m.vstride * 3;
    TV* tv = iMeshTV;
    int i;
    if (m.lazy && !(iSkip & 512))          // (512: tests, without)
        {
        int K[9] = { c0, 0, c2, c3, c4, c5, c6, c7, c8 };
        if (!LazyFaces(m, K, t, aScale, cs, sn)) return;
        iClOn = 1;
        }
#ifdef USE_ASM
    if (!iClOn)
        {
        int X[21];
        XfCtx(X, c0, c2, c3, c4, c5, c6, c7, c8, t);
        xform_v(V, tv, m.nv, X, 0);
        for (i = 0; i < m.nv; i++) { TV& v = tv[i]; if (v.code == -1) { Outcode(v); if (!v.code) Project(v); } }
        }
#else
    if (!iClOn)
    for (i = 0; i < m.nv; i++, V += 3)
        {
        int x = V[0], y = V[1], z = V[2];
        TV& v = tv[i];
        v.x = ((c0 * x + c2 * z) >> 14) + t.x;
        v.y = ((c3 * x + c4 * y + c5 * z) >> 14) + t.y;
        v.z = ((c6 * x + c7 * y + c8 * z) >> 14) + t.z;
        Outcode(v);
        if (!v.code) Project(v);
        }
#endif
    int lx = (iSunX * cs - iSunZ * sn) >> 14, ly = iSunY, lz = (iSunX * sn + iSunZ * cs) >> 14;
    iMeshNearLight = iNumLights && LightsNear(t.x, t.y, t.z, rad);
    // (a small thing - under 1.2 m across - is lit as a whole, from its middle: one look at the lights, not one a face)
    iMeshLightAdd = iMeshNearLight && rad < 6 * METER / 10 ? PointLight(t.x, t.y, t.z) : -1;
    DrawFaces(m, aFrame, lx, ly, lz, aSlots, aFlags);
    iClOn = 0;
    }

// world mesh with pitch (about local x) then yaw: items lying on tables, tilted props
void R3D::DrawMeshRot(const Mesh& m, int aFrame, int ox, int oy, int oz, int aYaw, int aPitch, int aScale, const u8* aSlots, int aFlags)
    {
    int cs = r3d_cos(aYaw), sn = r3d_sin(aYaw), cp = r3d_cos(aPitch), sp = r3d_sin(aPitch);
    int A[9] = { cs, (sn * sp) >> 14, (sn * cp) >> 14,   0, cp, -sp,   -sn, (cs * sp) >> 14, (cs * cp) >> 14 };
    DrawMeshMat(m, aFrame, ox, oy, oz, A, aScale, aSlots, aFlags);
    }

// turned about the vertical (yaw) after a roll about the model's own z axis (sails, wheels)
void R3D::DrawMeshRoll(const Mesh& m, int aFrame, int ox, int oy, int oz, int aYaw, int aRoll, int aScale, const u8* aSlots, int aFlags)
    {
    int cs = r3d_cos(aYaw), sn = r3d_sin(aYaw), cr = r3d_cos(aRoll), sr = r3d_sin(aRoll);
    int A[9] = { (cs * cr) >> 14, -(cs * sr) >> 14, sn,   sr, cr, 0,   -(sn * cr) >> 14, (sn * sr) >> 14, cs };
    DrawMeshMat(m, aFrame, ox, oy, oz, A, aScale, aSlots, aFlags);
    }

void R3D::DrawMeshMat(const Mesh& m, int aFrame, int ox, int oy, int oz, const int* A, int aScale, const u8* aSlots, int aFlags)
    {
    int K[9], r, c;
    for (r = 0; r < 3; r++)
        for (c = 0; c < 3; c++)
            K[r * 3 + c] = ((iM[r * 3] * A[c] + iM[r * 3 + 1] * A[3 + c] + iM[r * 3 + 2] * A[6 + c]) >> 14) * aScale >> 8;
    TV t;
    TransformPt(ox, oy, oz, t);
    int rad = (m.radius * aScale) >> 8;
    if (t.z < -rad) return;
    if (t.z > rad && rad * iFx < t.z) return;
    if (t.z * iHW + t.x * iFx < -rad * iLenX) return;
    if (t.z * iHW - t.x * iFx < -rad * iLenX) return;
    if (t.z * iHH - t.y * iFy < -rad * iLenY) return;
    if (t.z * iHH + t.y * iFy < -rad * iLenY) return;
    const short* V = m.v + aFrame * m.vstride * 3;
    TV* tv = iMeshTV;
    int i;
    for (i = 0; i < m.nv; i++, V += 3)
        {
        int x = V[0], y = V[1], z = V[2];
        TV& v = tv[i];
        v.x = ((K[0] * x + K[1] * y + K[2] * z) >> 14) + t.x;
        v.y = ((K[3] * x + K[4] * y + K[5] * z) >> 14) + t.y;
        v.z = ((K[6] * x + K[7] * y + K[8] * z) >> 14) + t.z;
        Outcode(v);
        if (!v.code) Project(v);
        }
    // light into object space: A^T * sun
    int lx = (A[0] * iSunX + A[3] * iSunY + A[6] * iSunZ) >> 14;
    int ly = (A[1] * iSunX + A[4] * iSunY + A[7] * iSunZ) >> 14;
    int lz = (A[2] * iSunX + A[5] * iSunY + A[8] * iSunZ) >> 14;
    iMeshNearLight = iNumLights && LightsNear(t.x, t.y, t.z, rad);
    // (a small thing - under 1.2 m across - is lit as a whole, from its middle: one look at the lights, not one a face)
    iMeshLightAdd = iMeshNearLight && rad < 6 * METER / 10 ? PointLight(t.x, t.y, t.z) : -1;
    DrawFaces(m, aFrame, lx, ly, lz, aSlots, aFlags);
    }

void R3D::DrawMeshView(const Mesh& m, int aFrame, int tx, int ty, int tz, int aYaw, int aPitch, int aRoll, const u8* aSlots, int aFlags)
    {
    // R = Ry(yaw) * Rx(pitch) * Rz(roll) in camera space (x right, y up, z forward)
    int cy = r3d_cos(aYaw), sy = r3d_sin(aYaw), cp = r3d_cos(aPitch), sp = r3d_sin(aPitch), cr = r3d_cos(aRoll), sr = r3d_sin(aRoll);
    // Rx*Rz
    int a00 = cr, a01 = -sr, a02 = 0;
    int a10 = (cp * sr) >> 14, a11 = (cp * cr) >> 14, a12 = -sp;
    int a20 = (sp * sr) >> 14, a21 = (sp * cr) >> 14, a22 = cp;
    // Ry * (Rx*Rz)
    int m0 = (cy * a00 + sy * a20) >> 14, m1 = (cy * a01 + sy * a21) >> 14, m2 = (cy * a02 + sy * a22) >> 14;
    int m3 = a10, m4 = a11, m5 = a12;
    int m6 = (-sy * a00 + cy * a20) >> 14, m7 = (-sy * a01 + cy * a21) >> 14, m8 = (-sy * a02 + cy * a22) >> 14;
    const short* V = m.v + aFrame * m.vstride * 3;
    TV* tv = iMeshTV;
    int i;
    for (i = 0; i < m.nv; i++, V += 3)
        {
        int x = V[0] - iPivX, y = V[1] - iPivY, z = V[2] - iPivZ;
        if (iViewScale != 256) { x = x * iViewScale >> 8; y = y * iViewScale >> 8; z = z * iViewScale >> 8; }
        TV& v = tv[i];
        v.x = ((m0 * x + m1 * y + m2 * z) >> 14) + tx;
        v.y = ((m3 * x + m4 * y + m5 * z) >> 14) + ty;
        v.z = ((m6 * x + m7 * y + m8 * z) >> 14) + tz;
        Outcode(v);
        if (!v.code) Project(v);
        }
    // sun: world -> camera -> object (R transposed)
    int sx = (iM[0] * iSunX + iM[1] * iSunY + iM[2] * iSunZ) >> 14;
    int syy = (iM[3] * iSunX + iM[4] * iSunY + iM[5] * iSunZ) >> 14;
    int sz = (iM[6] * iSunX + iM[7] * iSunY + iM[8] * iSunZ) >> 14;
    int lx = (m0 * sx + m3 * syy + m6 * sz) >> 14;
    int ly = (m1 * sx + m4 * syy + m7 * sz) >> 14;
    int lz = (m2 * sx + m5 * syy + m8 * sz) >> 14;
    iMeshNearLight = iNumLights; iLightMask = (1 << iNumLights) - 1; iMeshLightAdd = -1;
    DrawFaces(m, aFrame, lx, ly, lz, aSlots, aFlags | MF_NOFOG);
    }

// ---------------------------------------------------------------- static world meshes
enum { SM_TREE = ST_TREE, SM_HOUSE = ST_HOUSE, SM_ROCK = ST_ROCK, SM_STONE = ST_STONE, SM_FIRE = ST_FIRE,
       SM_TREEFAR = ST_COUNT, SM_ROCK2 = ST_COUNT + 1, SM_TREEMID = ST_COUNT + 2 };

void R3D::BuildStaticMeshes()
    {
    const int M = METER;
    // pine (as Skyrim's forest pines: a tall trunk, tiers of drooping branches narrowing to the top, each closed
    // underneath; 14 m at scale 256): slots 0,1,2 needles, 3 bark. Near: seven tiers; middle distance: four; far: two
    Mesh& t = iMSt[SM_TREE];
    MeshInit(t, 120, 120, 1);
    MeshCone(t, 0, 3, M / 4, -M / 2, 12 * M, 5, 0);                              // trunk
    {
    int k;
    for (k = 0; k < 7; k++)
        {
        int rim = 5 * M / 2 + k * 3 * M / 2 - M / 2, top = 4 * M + k * 3 * M / 2 + (k == 6 ? M : 0);
        int r = (3 * M) * (8 - k) / 8 + M / 3;
        MeshCone(t, 0, k % 3, r, rim, top, 7, (k & 1) * 73);
        MeshCone(t, 0, k % 3, r, rim, rim - M / 3, 7, (k & 1) * 73);         // (underneath: a shallow droop)
        }
    }
    MeshFinish(t);
    Mesh& tm = iMSt[SM_TREEMID];
    MeshInit(tm, 40, 40, 1);
    MeshCone(tm, 0, 3, M / 4, 0, 10 * M, 4, 0);
    MeshCone(tm, 0, 0, 3 * M, 2 * M, 6 * M, 6, 0);
    MeshCone(tm, 0, 1, 5 * M / 2, 5 * M, 9 * M, 6, 85);
    MeshCone(tm, 0, 2, 2 * M, 8 * M, 12 * M, 5, 0);
    MeshCone(tm, 0, 0, 5 * M / 4, 11 * M, 15 * M, 5, 100);
    MeshFinish(tm);
    Mesh& tf = iMSt[SM_TREEFAR];
    MeshInit(tf, 12, 10, 1);
    MeshCone(tf, 0, 0, 3 * M, 2 * M, 9 * M, 5, 0);
    MeshCone(tf, 0, 2, 2 * M, 7 * M, 15 * M, 4, 64);
    MeshFinish(tf);
    // longhouse: walls (slot 0), roof (slot 1), gables (slot 2), door (slot 3), ridge beams (slot 0)
    Mesh& h = iMSt[SM_HOUSE];
    MeshInit(h, 40, 48, 1);
    MeshBox(h, 0, 0, 0, 5 * M / 2, 0, 0, 8 * M, 5 * M / 4, 9 * M / 2);
    MeshRoof(h, 0, 1, 8 * M, 9 * M / 2, 5 * M / 2, 7 * M, M / 2);
    MeshBox(h, 0, 3, 0, 2 * M, 9 * M / 2 + 20, 0, 3 * M / 4, M, M / 12);
    // crossed gable beams (dragon-head style posts)
    MeshBoxR(h, 0, 0, -8 * M - M / 3, 15 * M / 2, 0, 0, 0, M / 8, 3 * M / 4, M / 8);
    MeshBoxR(h, 0, 0, 8 * M + M / 3, 15 * M / 2, 0, 0, 0, M / 8, 3 * M / 4, M / 8);
    MeshFinish(h);
    // rocks: irregular squat cones
    Mesh& r = iMSt[SM_ROCK];
    MeshInit(r, 12, 12, 1);
    MeshCone(r, 0, 0, 2 * M, -M / 2, 3 * M / 4, 7, 40);
    MeshFinish(r);
    Mesh& r2 = iMSt[SM_ROCK2];
    MeshInit(r2, 16, 16, 1);
    MeshConeAt(r2, 0, 0, 0, 0, 3 * M / 2, -M / 2, 2 * M / 3, 6, 0);
    MeshConeAt(r2, 0, 0, M, M / 2, M, -M / 2, M, 5, 100);
    MeshFinish(r2);
    // standing stone (word stone): tall slab + rune panel (slot 1, can glow)
    Mesh& s = iMSt[SM_STONE];
    MeshInit(s, 24, 24, 1);
    MeshBoxR(s, 0, 0, 0, 4 * M, 0, 0, 20, 3 * M / 4, 2 * M, M / 3);
    MeshBox(s, 0, 1, 0, 7 * M / 2, M / 3 + 12, 0, M / 2, 5 * M / 4, 8);
    MeshCone(s, 0, 0, 3 * M / 2, -M / 4, M / 4, 6, 0);       // base mound
    MeshFinish(s);
    // bonfire: logs (slot 0) + layered flame tongues (slots 1 outer, 2 inner, 3 core; unlit), 8 flicker frames
    Mesh& f = iMSt[SM_FIRE];
    MeshInit(f, 140, 150, 8);
    int fr;
    for (fr = 0; fr < 8; fr++)
        {
        iMeshCursor = 0;
        int k;
        for (k = 0; k < 4; k++)
            {
            int a = k * 256 + 90;
            int ca = r3d_cos(a), sa = r3d_sin(a);
            MeshLimb(f, fr, 0, (ca * M * 3 / 5) >> 14, 10, (sa * M * 3 / 5) >> 14, (-ca * M / 5) >> 14, M / 2, (-sa * M / 5) >> 14, 24, 24, 18, 18, 4, 3);
            }
        MeshFlame(f, fr, M / 2, 6 * M / 5, 1, 2, 3, M / 8);
        }
    MeshFinish(f);
    }

// a fire: rings of tapering tongues that sway and leap frame by frame, with a bright core.
// aR = spread radius, aH = height, slots: outer (red-orange), inner (yellow), core (white-yellow)
void R3D::MeshFlame(Mesh& m, int aFrame, int aR, int aH, int aOuter, int aInner, int aCore, int aY0)
    {
    int k;
    int ph = aFrame * 128;                       // 8 frames = one flicker cycle
    for (k = 0; k < 6; k++)
        {
        int a = k * 1024 / 6 + 40;
        int bx = (r3d_cos(a) * aR * 3 / 5) >> 14, bz = (r3d_sin(a) * aR * 3 / 5) >> 14;
        int h = aH * (150 + ((r3d_sin(ph * (1 + (k & 1)) + k * 300) + 16384) * 70 >> 15)) / 256;
        int sw = (r3d_sin(ph + k * 170) * aR / 3) >> 14;
        MeshLimb(m, aFrame, aOuter, bx, aY0, bz, bx / 3 + sw, aY0 + h, bz / 3 - sw / 2, aR / 3, aR / 3, 2, 2, 4, 0);
        }
    for (k = 0; k < 4; k++)
        {
        int a = k * 256 + 128;
        int bx = (r3d_cos(a) * aR / 3) >> 14, bz = (r3d_sin(a) * aR / 3) >> 14;
        int h = aH * (190 + ((r3d_sin(ph + k * 400 + 200) + 16384) * 66 >> 15)) / 256;
        int sw = (r3d_sin(ph * 2 + k * 260) * aR / 4) >> 14;
        MeshLimb(m, aFrame, aInner, bx, aY0, bz, sw, aY0 + h, -sw, aR / 4, aR / 4, 2, 2, 4, 0);
        }
    int hc = aH * (110 + ((r3d_sin(ph + 512) + 16384) * 40 >> 15)) / 256;
    MeshLimb(m, aFrame, aCore, 0, aY0, 0, 0, aY0 + hc, 0, aR / 4, aR / 4, 2, 2, 4, 1);
    }

// is an object (bounding circle R round (ox, oz), top height aTop) wholly hidden behind the wall line aOc from
// a camera on its near (-z) side? Its nearest point must be past the wall, the sight line to its top must pass
// under the wall's top, and every sight line to it must cross the wall within its ends. (Units of 16.)
static int OccHidden(const int* aOc, int cx, int cy, int cz, int ox, int oz, int aTop, int R)
    {
    int wz = aOc[2];
    if (oz - R <= wz + 64) return 0;
    int dw = (wz - cz) >> 4, dn = (oz - R - cz) >> 4, df = (oz + R - cz) >> 4;
    if (dw <= 0 || dn <= 0) return 0;
    if (aTop > cy && cy + ((((aTop - cy) >> 4) * dw / dn) << 4) >= aOc[3]) return 0;
    int xa = (ox - R - cx) >> 4, xb = (ox + R - cx) >> 4;
    int x1 = xa * dw / dn, x2 = xb * dw / dn, x3 = xa * dw / df, x4 = xb * dw / df;
    int mn = x1 < x3 ? x1 : x3, mx = x2 > x4 ? x2 : x4;
    return cx + (mn << 4) >= aOc[0] && cx + (mx << 4) <= aOc[1];
    }

// x^2 + z^2 out of line (GCC 2.95 for ARM cannot always allocate a squaring multiply in a big loop)
static inline int Dist2(int x, int z) { return x * x + z * z; }

// the trees, grown per chunk in the cache, out to the object distance (its own function: GCC 2.95 runs out of
// registers otherwise)
void R3D::DrawTrees(int ccx, int ccz)
    {
    int cx, cz;
    int rt = iObjD / (CELL * CHUNK) + 1;
    int od = iObjD >> 8;
    int lim = Dist2(od + 23, 0);
    int od2 = Dist2(od, 0);
    if (od2 > iVis2) od2 = iVis2;
    for (cz = ccz - rt; cz <= ccz + rt; cz++)
        {
        if (cz < 0 || cz >= iNCZ) continue;
        for (cx = ccx - rt; cx <= ccx + rt; cx++)
            {
            if (cx < 0 || cx >= iNCX) continue;
            int mx = ((cx * CHUNK + CHUNK / 2) * CELL - iCamX) >> 8, mz = ((cz * CHUNK + CHUNK / 2) * CELL - iCamZ) >> 8;
            if (Dist2(mx, mz) > lim) continue;
            const TChunk* tc = Cached(cx, cz, 0);
            if (!tc) continue;
            int k;
            for (k = 0; k < tc->ntree; k++)
                {
                const Obj& o = tc->tree[k];
                int d2 = Dist2((o.x - iCamX) >> 8, (o.z - iCamZ) >> 8);
                if (d2 > od2) continue;
                u8 sl[16];
                if (o.type == ST_ROCK)
                    {
                    sl[0] = 116; sl[1] = 112; sl[2] = 106; sl[3] = TX_ROCK;
                    DrawMesh(iMSt[o.var & 1 ? SM_ROCK2 : SM_ROCK], 0, o.x, o.y, o.z, o.yaw, o.scale, sl, 0);
                    continue;
                    }
                if (o.var >= 4 && iFallTree[0])
                    {
                    // (v0.4.66) Skyrim's aspen (the flora's: game_kit.cpp), its far form past 16 m
                    int fl = iLodFar[LODC_TREE];
                    const Mesh* am = iFallTree[1] && (fl < 0 || d2 > 8 * 8) ? iFallTree[1] : iFallTree[0];
                    DrawMesh(*am, 0, o.x, o.y, o.z, o.yaw, o.scale * 5 / 2, iFallSl, 0);
                    continue;
                    }
                if (o.var >= 4)
                    {
                    // the Rift's birches and aspens in their autumn gold
                    int y = 120 + (o.var & 3) * 18;
                    sl[0] = (u8)(y + 30); sl[1] = (u8)(y - 20); sl[2] = 24; sl[3] = TX_NEEDLE;
                    sl[4] = (u8)(y + 50); sl[5] = (u8)(y - 4); sl[6] = 30; sl[7] = TX_NEEDLE;
                    sl[8] = (u8)(y + 70); sl[9] = (u8)(y + 16); sl[10] = 40; sl[11] = TX_NEEDLE;
                    sl[12] = 180; sl[13] = 172; sl[14] = 160; sl[15] = TX_FUR;
                    }
                else
                    {
                    int g = 62 + o.var * 7;
                    sl[0] = 30; sl[1] = (u8)g; sl[2] = 36; sl[3] = TX_NEEDLE;
                    sl[4] = 36; sl[5] = (u8)(g + 10); sl[6] = 40; sl[7] = TX_NEEDLE;
                    sl[8] = 44; sl[9] = (u8)(g + 22); sl[10] = 46; sl[11] = TX_NEEDLE;
                    sl[12] = 92; sl[13] = 62; sl[14] = 38; sl[15] = TX_FUR;
                    }
                int tm = d2 > 70 * 70 ? SM_TREEFAR : (d2 > 30 * 30 ? SM_TREEMID : SM_TREE);
                DrawMesh(iMSt[tm], 0, o.x, o.y, o.z, o.yaw, o.scale, sl, 0);
                }
            }
        }
    }

void R3D::DrawObjects(int aP0, int aP1)
    {
    int ccx = iCamX / (CELL * CHUNK), ccz = iCamZ / (CELL * CHUNK);
    // outside the city wall and below its top: walled-in low props cannot be seen
    int walled = 0;
    if (iWallR)
        {
        int wx = (iCamX - iWallX) >> 8, wz = (iCamZ - iWallZ) >> 8, wr = (iWallR >> 8) + 2;
        walled = wx * wx + wz * wz > wr * wr && iCamY < iWallTop;
        }
    // wall lines that hide what is behind them from here
    int occ[4], nocc = 0, k;
    for (k = 0; k < iOccN && !iCell; k++)
        if (iCamZ < iOcc[k * 4 + 2] - METER / 2 && iCamY < iOcc[k * 4 + 3]) occ[nocc++] = k;
    int cx, cz;
    if (aP1 >= 1) DrawTrees(ccx, ccz);
    int od2 = Dist2(iObjD >> 8, 0);
    if (od2 > iVis2) od2 = iVis2;
    // what was built: out to the landmark distance
    int rr = iMaxD / (CELL * CHUNK) + 1;
    int vis2 = iFarOn ? (iMaxD >> 8) * (iMaxD >> 8) : iVis2;
    // the buildings and big pieces first, then the rest (left out where the land or the buildings hide them)
    int pass;
    for (pass = aP0; pass <= aP1; pass++)
    {
    // (v0.4.70) the custom statics that pass the tests gathered first, then drawn nearest first (by the distance to
    // their nearest side): what is nearer is drawn before what it hides, whose pixels then fail the depth test early -
    // in the city most pixels were drawn over three or four times, back pieces first as the chunks came
    int nkey = 0;
    // (v0.4.74) the far groups far enough off: drawn whole (once, in the first pass), their pieces left out below
    int fgn = (iCell || (iSkip & 65536)) ? 0 : iNFG, fg;
    for (fg = 0; fg < fgn; fg++)
        {
        const FarGroup& g = iFG[fg];
        int ex = iCamX < g.x0 ? g.x0 - iCamX : (iCamX > g.x1 ? iCamX - g.x1 : 0);
        int ez = iCamZ < g.z0 ? g.z0 - iCamZ : (iCamZ > g.z1 ? iCamZ - g.z1 : 0);
        ex >>= 8; ez >>= 8;
        int fm = iFGFarM;
        iFGFar[fg] = (u8)(ex * ex + ez * ez > fm * fm);
        if (iFGFar[fg] && pass == 0) DrawMesh(g.m, 0, g.x, g.y, g.z, 0, 256, g.slots, 0);     // (the big pieces' pass: once a frame)
        }
    for (cz = ccz - rr; cz <= ccz + rr; cz++)
        {
        if (cz < 0 || cz >= iNCZ) continue;
        for (cx = ccx - rr; cx <= ccx + rr; cx++)
            {
            if (cx < 0 || cx >= iNCX) continue;
            int i = iChunkObj[cz * iNCX + cx];
            while (i >= 0)
                {
                Obj& o = iObj[i];
                unsigned pk = (unsigned)(i - iOutPvsFirst);
                int og = fgn ? iObjGrp[i] : 0;
                i = o.next;
                if (og && iFGFar[og - 1]) continue;                // (v0.4.74: in its far group's model)
                // (v0.4.65) hidden from here (the city's walls and houses: host/pvs)
                if (iOutPvsBits && pk < (unsigned)iOutPvsN && o.type >= ST_CUSTOM && !((iOutPvsBits[pk >> 3] >> (pk & 7)) & 1)) continue;
                int dx = (o.x - iCamX) >> 8, dz = (o.z - iCamZ) >> 8;
                int d2 = dx * dx + dz * dz;
                if (d2 > vis2) continue;
                if (o.type >= ST_CUSTOM)
                    {
                    const StaticDef& d = iDefs[o.type - ST_CUSTOM];
                    if (d2 > d.drawDist * d.drawDist) continue;
                    // (v0.4.50, the setting) small things - under 2 m across, not trees - only within 25 m
                    if (iClutterNear && d2 > 25 * 25 && ((d.mesh->radius * o.scale) >> 8) < 2 * METER && d.lodCat != LODC_TREE) continue;
                    if ((((d.mesh->radius * o.scale) >> 8) >= 4 * METER) != (pass == 0)) continue;
                    if (walled && (d.flags & SD_WALLED))
                        {
                        int ox = (o.x - iWallX) >> 8, oz = (o.z - iWallZ) >> 8, orr = (iWallR >> 8) - 2;
                        if (ox * ox + oz * oz < orr * orr) continue;
                        }
                    if (nocc)
                        {
                        const Mesh* mm = d.mesh;
                        int R = ((mm->brad + Abs(mm->cx) + Abs(mm->cz)) * o.scale >> 8) + 32;
                        int top = o.y + d.yOff + ((mm->cy + mm->brad) * o.scale >> 8);
                        int q, hid = 0;
                        for (q = 0; q < nocc && !hid; q++) hid = OccHidden(iOcc + occ[q] * 4, iCamX, iCamY, iCamZ, o.x, o.z, top, R);
                        if (hid) continue;
                        }
                    if (nkey < MAXOBJ)
                        {
                        int dm = r3d_isqrt(d2) - ((d.mesh->radius * o.scale) >> 16);
                        if (dm < 0) dm = 0;
                        if (dm > 511) dm = 511;
                        iSortKey[nkey++] = ((u32)dm << 20) | (u32)((&o - iObj) & 0xFFFFF);
                        continue;
                        }
                    DrawStatic(o);
                    continue;
                    }
                if (d2 > od2 || pass == 0) continue;
                DrawOldStatic(o, d2);
                }
            }
        }
    // nearest first: counted out by metre
    {
    int* cnt = iSortCnt;
    u32* out = iSortKey + MAXOBJ;
    int b, sum = 0, j, t0 = iTris;
    for (b = 0; b <= 512; b++) cnt[b] = 0;
    for (j = 0; j < nkey; j++) cnt[iSortKey[j] >> 20]++;
    for (b = 0; b < 512; b++) { int c_ = cnt[b]; cnt[b] = sum; sum += c_; }
    for (j = 0; j < nkey; j++) out[cnt[iSortKey[j] >> 20]++] = iSortKey[j];
    for (j = 0; j < nkey; j++)
        {
        // (the nearest drawn: what they hide known - the occlusion map from the depths so far, so the far pieces
        //  behind them are left out whole, cluster by cluster, triangle by triangle)
        if (!(iSkip & 1024) && OccMidOk() && (iOccEvery > 0 ? j >= 8 && iTris - t0 >= iOccEvery : (j == 16 || j == 48 || j == 128) && iTris - t0 >= 500 * (j == 16 ? 1 : 2))) { OccMidBuild(); t0 = iTris; }     // (only where much is drawn: a build costs about a quarter of a million instructions)
        DrawStatic(iObj[out[j] & 0xFFFFF]);
        }
    }
    }
    }

// (v0.4.74) a thing's box from afar: its far model's bounds, and its colours from its faces' (by their areas): the
// roof's those facing up, the sides' the rest. Only for the big ones (2.5 m and more across their radius, not trees)
void R3D::BoxInfo(StaticDef& d)
    {
    d.boxState = 2;
    const Mesh* m = d.lod;
    if (!m || m->nv < 8 || m->radius < 25 * METER / 10 || d.lodCat == LODC_TREE || (d.flags & (SD_ANIM | SD_SPINY | SD_SPINZ))) return;
    int i, k, lo[3] = { 32767, 32767, 32767 }, hi[3] = { -32768, -32768, -32768 };
    for (i = 0; i < m->nv; i++) for (k = 0; k < 3; k++) { int v = m->v[i * 3 + k]; if (v < lo[k]) lo[k] = v; if (v > hi[k]) hi[k] = v; }
    int sum[2][4]; r3d_memset(sum, 0, sizeof(sum));
    for (i = 0; i < m->nf; i++)
        {
        const u16* f = m->f + i * 4;
        const short* a = m->v + f[0] * 3; const short* b = m->v + f[1] * 3; const short* c = m->v + f[2] * 3;
        int ux = (b[0] - a[0]) >> 4, uy = (b[1] - a[1]) >> 4, uz = (b[2] - a[2]) >> 4, vx = (c[0] - a[0]) >> 4, vy = (c[1] - a[1]) >> 4, vz = (c[2] - a[2]) >> 4;
        int cx = uy * vz - uz * vy, cy = uz * vx - ux * vz, cz = ux * vy - uy * vx;
        int ar = (cx < 0 ? -cx : cx) + (cy < 0 ? -cy : cy) + (cz < 0 ? -cz : cz) + 1;
        if (ar > 1 << 20) ar = 1 << 20;
        int top = m->n[i * 3 + 1] > 9000;
        const u8* sl = d.slots + (f[3] & 7) * 4;
        int w = ar >> 6 ? ar >> 6 : 1;
        sum[top][0] += sl[0] * w; sum[top][1] += sl[1] * w; sum[top][2] += sl[2] * w; sum[top][3] += w;
        }
    for (k = 0; k < 2; k++)
        {
        int s = k == 0 ? 1 : 0;                         // boxCol: the roof's (top), then the sides'
        int t = sum[s][3] ? s : 1 - s;
        for (i = 0; i < 3; i++) d.boxCol[k * 3 + i] = (u8)(sum[t][3] ? sum[t][i] / sum[t][3] : 128);
        }
    for (k = 0; k < 3; k++) { d.bb[k] = (short)lo[k]; d.bb[3 + k] = (short)hi[k]; }
    d.boxState = 1;
    }

// (v0.4.74) far groups. A static goes in its 64 m square's group if it has a far model and is not a tree, a small
// thing or a turning one; a square with 6 or more makes a group. Its model: every piece's far model in the group's
// frame, each corner snapped to a grid (iFGGridU) and merged with the others there; faces that collapse dropped; the
// faces' colours their slots' (a texture's: its average) in a palette of 32. The temporary tables at the arena's top
// end; what does not fit (faces, the arena) leaves the group out - its pieces drawn as before
enum { FG_HASH = 4096, FG_MAXV = 2000, FG_MAXF = 2400, FG_FHASH = 8192 };
static int FgEligible(const StaticDef& d, const Obj& o)
    {
    if (!d.lod || d.lodCat == LODC_TREE || (d.flags & (SD_ANIM | SD_SPINY | SD_SPINZ))) return 0;
    return ((d.mesh->radius * o.scale) >> 8) >= 15 * METER / 10;
    }
void R3D::BuildFarGroups(int aFirst, int aLast)
    {
    int i, k;
    iNFG = 0;
    r3d_memset(iObjGrp, 0, MAXOBJ);
#ifdef FG_TEST_OFF
    return;
#endif
    if (aLast > iObjCount) aLast = iObjCount;
    const int G = 64 * METER;
    // the squares with enough pieces
    for (i = aFirst; i < aLast; i++)
        {
        const Obj& o = iObj[i];
        if (o.type < ST_CUSTOM || !FgEligible(iDefs[o.type - ST_CUSTOM], o)) continue;
        int gx = o.x / G, gz = o.z / G;
        for (k = 0; k < iNFG; k++) if (iFG[k].x == gx && iFG[k].z == gz) break;
        if (k == iNFG) { if (iNFG >= MAXFG) continue; iFG[k].x = gx; iFG[k].z = gz; iFG[k].nobj = 0; iNFG++; }
        iFG[k].nobj++;
        iObjGrp[i] = (u8)(k + 1);
        }
    int tmpN = FG_HASH * 8 + FG_MAXV * 6 + FG_MAXF * 8 + FG_FHASH * 4 + 64;
    u8* tmp = iArenaEnd - tmpN;
    int n = 0;
    for (k = 0; k < iNFG; k++)
        {
        FarGroup& g = iFG[k];
        int ok = g.nobj >= 6 && iArena + 96 * 1024 < tmp && FarGroupMesh(g, k, aFirst, aLast, tmp, tmpN);
        // (kept or not: the pieces point at the group's place in the list as it ends up)
        for (i = aFirst; i < aLast; i++) if (iObjGrp[i] == k + 1) iObjGrp[i] = ok ? (u8)(n + 1) : 0;
        if (ok) { if (n != k) iFG[n] = g; n++; }
        }
    iNFG = n;
    }
int R3D::FarGroupMesh(FarGroup& g, int gi, int aFirst, int aLast, u8* aTmp, int aTmpN)
    {
    const int G = 64 * METER, Q = iFGGridU;
    int ox = g.x * G + G / 2, oz = g.z * G + G / 2, oy = 0, ny = 0, i, j;
    u32* hk = (u32*)aTmp; u16* hv = (u16*)(hk + FG_HASH);
    short* V = (short*)(aTmp + FG_HASH * 6 + 8);
    u16* F = (u16*)((u8*)V + FG_MAXV * 6 + 8);
    u32* fh = (u32*)((u8*)F + FG_MAXF * 8);                 // (the faces kept, by their corners: no two on the same three)
    for (i = 0; i < FG_HASH; i++) hk[i] = 0xFFFFFFFF;
    for (i = 0; i < FG_FHASH; i++) fh[i] = 0xFFFFFFFF;
    int nv = 0, nf = 0, nsl = 0;
    g.x0 = g.z0 = 0x7FFFFFFF; g.x1 = g.z1 = -0x7FFFFFFF;
    for (i = aFirst; i < aLast; i++) if (iObjGrp[i] == gi + 1) { oy += iObj[i].y; ny++; }
    oy = ny ? oy / ny : 0;
    for (i = aFirst; i < aLast; i++)
        {
        if (iObjGrp[i] != gi + 1) continue;
        const Obj& o = iObj[i];
        const StaticDef& d = iDefs[o.type - ST_CUSTOM];
        const Mesh* m = d.lod;
        int cs = r3d_cos(o.yaw), sn = r3d_sin(o.yaw), sc = o.scale;
        int* map = iMeshLev;                                // (a frame's scratch: not drawing now)
        if (m->nv > MAXMV) return 0;
        for (j = 0; j < m->nv; j++)
            {
            const short* p = m->v + j * 3;
            int wx = (((p[0] * cs + p[2] * sn) >> 14) * sc >> 8) + o.x - ox;
            int wy = (p[1] * sc >> 8) + o.y + d.yOff - oy;
            int wz = (((-p[0] * sn + p[2] * cs) >> 14) * sc >> 8) + o.z - oz;
            int qx = (wx + (wx >= 0 ? Q / 2 : -Q / 2)) / Q, qy = (wy + (wy >= 0 ? Q / 2 : -Q / 2)) / Q, qz = (wz + (wz >= 0 ? Q / 2 : -Q / 2)) / Q;
            if (qx < -500 || qx > 500 || qy < -500 || qy > 500 || qz < -500 || qz > 500 || qx * Q > 32000 || qx * Q < -32000 || qy * Q > 32000 || qy * Q < -32000 || qz * Q > 32000 || qz * Q < -32000) return 0;
            u32 key = (u32)(qx + 512) | ((u32)(qy + 512) << 10) | ((u32)(qz + 512) << 20);
            u32 h = (key * 2654435761u) >> 20;
            while (hk[h] != 0xFFFFFFFF && hk[h] != key) h = (h + 1) & (FG_HASH - 1);
            if (hk[h] == 0xFFFFFFFF)
                {
                if (nv >= FG_MAXV) return 0;
                hk[h] = key; hv[h] = (u16)nv;
                V[nv * 3] = (short)(qx * Q); V[nv * 3 + 1] = (short)(qy * Q); V[nv * 3 + 2] = (short)(qz * Q);
                nv++;
                }
            map[j] = hv[h];
            }
        // its extent (for the distance: the group's box)
        int rr = (m->radius * sc) >> 8;
        if (o.x - rr < g.x0) g.x0 = o.x - rr; if (o.x + rr > g.x1) g.x1 = o.x + rr;
        if (o.z - rr < g.z0) g.z0 = o.z - rr; if (o.z + rr > g.z1) g.z1 = o.z + rr;
        for (j = 0; j < m->nf; j++)
            {
            const u16* f = m->f + j * 4;
            int a = map[f[0]], b = map[f[1]], c = map[f[2]];
            if (a == b || b == c || a == c) continue;
            // (undersides: seen only from below)
            {
            int nx_, ny_, nz_;
            FaceNormal(V + a * 3, V + b * 3, V + c * 3, nx_, ny_, nz_);
            if (ny_ < -11000) continue;
            }
            {
            // (the same three corners as a face kept already - a wall's two sides, pieces on one another: once)
            int s0 = a < b ? a : b, s2 = a < b ? b : a, s1;
            if (c < s0) { s1 = s0; s0 = c; } else if (c > s2) { s1 = s2; s2 = c; } else s1 = c;
            u32 key = (u32)s0 | ((u32)s1 << 11) | ((u32)s2 << 22);
            u32 h = (key * 2654435761u) >> 19;
            while (fh[h] != 0xFFFFFFFF && fh[h] != key) h = (h + 1) & (FG_FHASH - 1);
            if (fh[h] == key) continue;
            fh[h] = key;
            }
            if (nf >= FG_MAXF) return 0;
            // the colour: the palette's nearest, or a new one
            const u8* sl = d.slots + (f[3] & 7) * 4;
            int best = -1, bd = 1 << 30, s;
            for (s = 0; s < nsl; s++)
                {
                int e = Abs(g.slots[s * 4] - sl[0]) + Abs(g.slots[s * 4 + 1] - sl[1]) + Abs(g.slots[s * 4 + 2] - sl[2]);
                if (e < bd) { bd = e; best = s; }
                }
            if ((best < 0 || bd > 18) && nsl < FG_SLOTS) { best = nsl++; g.slots[best * 4] = sl[0]; g.slots[best * 4 + 1] = sl[1]; g.slots[best * 4 + 2] = sl[2]; g.slots[best * 4 + 3] = 0; }
            F[nf * 4] = (u16)a; F[nf * 4 + 1] = (u16)b; F[nf * 4 + 2] = (u16)c; F[nf * 4 + 3] = (u16)best;
            nf++;
            }
        }
    if (nf == 0) return 0;
    // the model: its own arrays in the arena
    int need = nv * 6 + nf * 8 + nf * 6 + 64;
    if (iArena + need + 64 * 1024 > aTmp) return 0;
    Mesh& M = g.m;
    r3d_memset(&M, 0, sizeof(M));
    M.nv = nv; M.nf = nf; M.nfr = 1; M.vstride = nv; M.fstride = nf;
    M.v = (short*)Alloc(nv * 6); M.f = (u16*)Alloc(nf * 8); M.n = (short*)Alloc(nf * 6); M.uv = M.f;
    int rad = 0;
    for (i = 0; i < nv * 3; i++) M.v[i] = V[i];
    for (i = 0; i < nv; i++) { int r_ = Abs(V[i * 3]) + Abs(V[i * 3 + 1]) + Abs(V[i * 3 + 2]); if (r_ > rad) rad = r_; }
    for (i = 0; i < nf * 4; i++) M.f[i] = F[i];
    for (i = 0; i < nf; i++)
        {
        int nx, ny_, nz;
        FaceNormal(M.v + M.f[i * 4] * 3, M.v + M.f[i * 4 + 1] * 3, M.v + M.f[i * 4 + 2] * 3, nx, ny_, nz);
        M.n[i * 3] = (short)nx; M.n[i * 3 + 1] = (short)ny_; M.n[i * 3 + 2] = (short)nz;
        }
    M.radius = rad; M.brad = rad;
    g.x = ox; g.y = oy; g.z = oz;
    return 1;
    }

// (v0.4.70) one custom static (DrawObjects: its tests passed): its model for the distance, its looks, drawn
void R3D::DrawStatic(Obj& o)
    {
    const StaticDef& d = iDefs[o.type - ST_CUSTOM];
    int dx = (o.x - iCamX) >> 8, dz = (o.z - iCamZ) >> 8;
    int d2 = dx * dx + dz * dz;
                    {
                    // (the plainer model past lodDist from the thing's nearest side, not its middle: a long house's porch
                    //  is close while its middle is not)
                    // (v0.4.46: and a middle one between, from midDist; the distances x the Model detail setting)
                    const Mesh* m = d.mesh;
                    if (d.lod)
                        {
                        int rr = (d.mesh->radius * o.scale) >> 16;
                        int fd = d.lodCat ? iLodFar[d.lodCat] : d.lodDist, ld = fd + rr;
                        int nd = d.lodCat == LODC_BUILDING ? iLodNear : d.midDist;
                        if (fd < 0 || d2 > ld * ld) m = d.lod;
                        else if (d.mid) { if (nd < 0) m = d.mid; else { int lm = nd + rr; if (d2 > lm * lm) m = d.mid; } }
                        // (v0.4.70) and by its size on the screen: a thing under 8 pixels across its radius its far model,
                        // under 20 its middle one - small things (a trellis, a stall, a cart) were drawn whole, a hundred
                        // triangles, at a few hundred pixels
                        if (m != d.lod && !(iSkip & 2048))
                            {
                            int rpx = (d.mesh->radius * o.scale) >> 8;              // (world units)
                            int du = r3d_isqrt(d2) << 8;                             // (its middle's distance, world units)
                            if (rpx * iFx < 8 * du) m = d.lod;
                            else if (d.mid && m == d.mesh && rpx * iFx < 20 * du) m = d.mid;
                            }
                        // (v0.4.74) a few pixels across: its box (a town across the plains - a few hundred things, each
                        // its far model of up to 160 faces, at a frame's cost; a box is 12)
                        if (m == d.lod && iBoxPx && !(iSkip & 4096))
                            {
                            StaticDef& dd = (StaticDef&)d;
                            if (!dd.boxState) BoxInfo(dd);
                            int rpx = (d.mesh->radius * o.scale) >> 8, du = r3d_isqrt(d2) << 8;
                            if (dd.boxState == 1 && rpx * iFx < (d.lodCat == LODC_BUILDING ? iBoxPx + 3 : iBoxPx) * du)
                                {
                                Mesh& b = iBoxMesh;
                                int k;
                                for (k = 0; k < 8; k++)
                                    {
                                    iBoxV[k * 3] = (k & 1) ? dd.bb[3] : dd.bb[0];
                                    iBoxV[k * 3 + 1] = (k & 2) ? dd.bb[4] : dd.bb[1];
                                    iBoxV[k * 3 + 2] = (k & 4) ? dd.bb[5] : dd.bb[2];
                                    }
                                b.radius = d.lod->radius; b.cx = d.lod->cx; b.cy = d.lod->cy; b.cz = d.lod->cz; b.brad = d.lod->brad;
                                u8 bs[8];
                                bs[0] = dd.boxCol[0]; bs[1] = dd.boxCol[1]; bs[2] = dd.boxCol[2]; bs[3] = 0;
                                bs[4] = dd.boxCol[3]; bs[5] = dd.boxCol[4]; bs[6] = dd.boxCol[5]; bs[7] = 0;
                                DrawMesh(b, 0, o.x, o.y + d.yOff, o.z, o.yaw, o.scale, bs, 0);
                                return;
                                }
                            }
                        }
                    int fr = (d.flags & SD_ANIM) ? (iAnimMs / (d.animMs ? d.animMs : 100) + o.var * 3) % m->nfr : (d.frame < m->nfr ? d.frame : 0);
                    const u8* sl = d.slots;
                    u8 tmp[32];
                    if (o.var && !(d.flags & SD_ANIM))
                        {
                        // variant: tint slot 1 (e.g. roof / cloth colour)
                        int k;
                        for (k = 0; k < 32; k++) tmp[k] = d.slots[k];
                        static const u8 KVar[8 * 3] = { 0,0,0, 150,40,36, 52,80,130, 170,140,60, 86,112,60, 120,104,62, 70,70,76, 140,90,50 };
                        tmp[4] = KVar[(o.var & 7) * 3]; tmp[5] = KVar[(o.var & 7) * 3 + 1]; tmp[6] = KVar[(o.var & 7) * 3 + 2];
                        // textured slot 1 (roofs): the variant picks thatch, slate or shingles instead
                        static const u8 KVarTex[8] = { 0, 0, 0, 0, 0, TX_THATCH, TX_SLATE, TX_SHINGLE };
                        if ((tmp[7] & SL_SKY) == SL_TEX && KVarTex[o.var & 7]) tmp[7] = (u8)(SL_TEX | KVarTex[o.var & 7]);
                        sl = tmp;
                        }
                    if (d.flags & (SD_SPINY | SD_SPINZ))
                        {
                        // smooth turning: the angle comes from the clock, not from frames
                        int sm = d.spinMs ? d.spinMs : 10000;
                        int ang = (iAnimMs % sm) * 1024 / sm;
                        if (d.flags & SD_SPINY) { DrawMesh(*m, fr, o.x, o.y + d.yOff, o.z, (o.yaw + ang) & 1023, o.scale, sl, 0); return; }
                        int pz = d.pivZ * o.scale >> 8;
                        DrawMeshRoll(*m, fr, o.x + ((r3d_sin(o.yaw) * pz) >> 14), o.y + d.yOff, o.z + ((r3d_cos(o.yaw) * pz) >> 14), o.yaw, ang, o.scale, sl, 0);
                        return;
                        }
                    DrawMesh(*m, fr, o.x, o.y + d.yOff, o.z, o.yaw, o.scale, sl, 0);
                    }
    }
// the engine's own statics (trees, houses, rocks, stones, fires)
void R3D::DrawOldStatic(Obj& o, int d2)
    {
                u8 sl[16];
                switch (o.type)
                    {
                    case ST_TREE:
                        {
                        int g = 62 + o.var * 7;
                        sl[0] = 30; sl[1] = (u8)g; sl[2] = 36; sl[3] = TX_NEEDLE;
                        sl[4] = 36; sl[5] = (u8)(g + 10); sl[6] = 40; sl[7] = TX_NEEDLE;
                        sl[8] = 44; sl[9] = (u8)(g + 22); sl[10] = 46; sl[11] = TX_NEEDLE;
                        sl[12] = 92; sl[13] = 62; sl[14] = 38; sl[15] = TX_FUR;
                        if (d2 > 70 * 70) DrawMesh(iMSt[SM_TREEFAR], 0, o.x, o.y, o.z, o.yaw, o.scale, sl, 0);
                        else if (d2 > 30 * 30) DrawMesh(iMSt[SM_TREEMID], 0, o.x, o.y, o.z, o.yaw, o.scale, sl, 0);
                        else DrawMesh(iMSt[SM_TREE], 0, o.x, o.y, o.z, o.yaw, o.scale, sl, 0);
                        break;
                        }
                    case ST_HOUSE:
                        sl[0] = 112; sl[1] = 80; sl[2] = 50; sl[3] = TX_FUR;
                        sl[4] = 120; sl[5] = 104; sl[6] = 62; sl[7] = TX_FUR;      // thatch / turf roof
                        sl[8] = 92; sl[9] = 64; sl[10] = 40; sl[11] = TX_FUR;
                        sl[12] = 40; sl[13] = 28; sl[14] = 20; sl[15] = TX_FUR;
                        if (o.var & 1) { sl[4] = 86; sl[5] = 112; sl[6] = 60; }     // turf roof
                        DrawMesh(iMSt[SM_HOUSE], 0, o.x, o.y, o.z, o.yaw, 256, sl, 0);
                        break;
                    case ST_ROCK:
                        sl[0] = 116; sl[1] = 112; sl[2] = 106; sl[3] = TX_ROCK;
                        DrawMesh(iMSt[o.var & 1 ? SM_ROCK2 : SM_ROCK], 0, o.x, o.y, o.z, o.yaw, o.scale, sl, 0);
                        break;
                    case ST_STONE:
                        sl[0] = 104; sl[1] = 108; sl[2] = 112; sl[3] = TX_ROCK;
                        if (o.flag) { sl[4] = 120; sl[5] = 200; sl[6] = 255; }
                        else { sl[4] = 70; sl[5] = 76; sl[6] = 84; }
                        sl[7] = TX_ROCK;
                        DrawMesh(iMSt[SM_STONE], 0, o.x, o.y, o.z, o.yaw, 256, sl, 0);
                        break;
                    case ST_FIRE:
                        {
                        sl[0] = 70; sl[1] = 46; sl[2] = 28; sl[3] = TX_FUR;
                        sl[4] = 255; sl[5] = 140; sl[6] = 30; sl[7] = TX_FUR;
                        sl[8] = 255; sl[9] = 210; sl[10] = 80; sl[11] = TX_FUR;
                        sl[12] = 255; sl[13] = 246; sl[14] = 200; sl[15] = TX_FUR;
                        sl[4] = 240; sl[5] = 96; sl[6] = 24;
                        int fr = (iAnimMs / 85 + (o.x >> 8)) & 7;
                        DrawMesh(iMSt[SM_FIRE], fr, o.x, o.y, o.z, o.yaw, 256, sl, 0);
                        break;
                        }
                    }
    }

// ---------------------------------------------------------------- frame + 2D
void R3D::SetCamera(int aX, int aY, int aZ, int aYaw, int aPitch)
    {
    iCamX = aX; iCamY = aY; iCamZ = aZ; iYaw = aYaw & 1023; iPitch = aPitch;
    }

void R3D::ClearBorders()
    {
    if (iBorderFrames <= 0 || !iNative) return;
    iBorderFrames--;
    int y, a = OutX0() / 2, b = (OutX0() + iW) / 2;
    for (y = 0; y < OUT_H; y++)
        {
        u32* l = (u32*)(iOut + y * iOutStride);
        int x;
        for (x = 0; x < a; x++) l[x] = 0;
        for (x = b; x < OUT_W / 2; x++) l[x] = 0;
        }
    }

int R3D::AddInterior(int aCell, int aType, int aX, int aY, int aZ, int aYaw, int aScale, int aVar)
    {
    if (iINum >= MAXIOBJ) return -1;
    Obj& o = iIObj[iINum];
    o.x = aX; o.y = aY; o.z = aZ; o.yaw = (short)(aYaw & 1023); o.scale = (short)aScale;
    o.type = (u16)aType; o.var = (u8)aVar; o.flag = (u8)aCell; o.pad = 0; o.next = -1;
    return iINum++;
    }

// the z buffer so far, block by block: the farthest thing in each (0 where nothing is drawn yet); from now until
// the next ClearZ, DrawMesh leaves out what is wholly behind it
void R3D::OccBuild()
    {
    int bw = (iW + OCCB - 1) / OCCB, bh = (iH + OCCB - 1) / OCCB;
    if (bw > OCCW) bw = OCCW;
    if (bh > OCCH) bh = OCCH;
    int bx, by = 0;
#ifdef USE_ASM
    // (v0.4.70) the whole blocks in ARM code (spans.s); a part row of blocks at the foot, and a part block at the
    // right, as before
    if (!(iW & 7) && !((unsigned long)iZB & 3))
        {
        int full = iH / OCCB;
        if (full > bh) full = bh;
        if (full > 0) occ_build(iZB, iW, iW / OCCB < bw ? iW / OCCB : bw, full, iOccMap, OCCW);
        by = full;
        }
#endif
    for (; by < bh; by++)
        {
        int y0 = by * OCCB, y1 = y0 + OCCB > iH ? iH : y0 + OCCB;
        for (bx = 0; bx < bw; bx++)
            {
            int x0 = bx * OCCB, x1 = x0 + OCCB > iW ? iW : x0 + OCCB;
            int mn = 65535, y;
            for (y = y0; y < y1 && mn; y++)
                {
                const u16* z = iZB + y * iW + x0;
                int n = x1 - x0;
                if (n == OCCB && !((unsigned long)z & 3))
                    {
                    // (two at a time: the four words of a block's row)
                    const u32* q = (const u32*)z;
                    u32 a = q[0], b = q[1], c = q[2], d = q[3];
                    int v;
                    v = a & 0xFFFF; if (v < mn) mn = v;
                    v = a >> 16; if (v < mn) mn = v;
                    v = b & 0xFFFF; if (v < mn) mn = v;
                    v = b >> 16; if (v < mn) mn = v;
                    v = c & 0xFFFF; if (v < mn) mn = v;
                    v = c >> 16; if (v < mn) mn = v;
                    v = d & 0xFFFF; if (v < mn) mn = v;
                    v = d >> 16; if (v < mn) mn = v;
                    }
                else while (n--) { int v = *z++; if (v < mn) mn = v; }
                }
            iOccMap[by * OCCW + bx] = (u16)mn;
            }
        }
    iOccOn = 1;
    }

// a triangle on the screen (all its corners in view) wholly behind what the blocks it covers hold: its nearest
// corner's w is no nearer than the farthest thing in each of them
int R3D::OccHiddenTri(const TV* a, const TV* b, const TV* c)
    {
    int w = a->w, x0 = a->sx, x1 = a->sx, y0 = a->sy, y1 = a->sy;
    if (b->w > w) w = b->w;
    if (c->w > w) w = c->w;
    if (b->sx < x0) x0 = b->sx; else if (b->sx > x1) x1 = b->sx;
    if (c->sx < x0) x0 = c->sx; else if (c->sx > x1) x1 = c->sx;
    if (b->sy < y0) y0 = b->sy; else if (b->sy > y1) y1 = b->sy;
    if (c->sy < y0) y0 = c->sy; else if (c->sy > y1) y1 = c->sy;
    // (blocks of 8 pixels: 1/16 pixel >> 7)
    int bx0 = x0 >> 7, bx1 = x1 >> 7, by0 = y0 >> 7, by1 = y1 >> 7, bx, by;
    if (bx0 < 0) bx0 = 0;
    if (by0 < 0) by0 = 0;
    int bw = (iW + OCCB - 1) / OCCB, bh = (iH + OCCB - 1) / OCCB;
    if (bx1 >= bw) bx1 = bw - 1;
    if (by1 >= bh) by1 = bh - 1;
    for (by = by0; by <= by1; by++)
        {
        const u16* m = iOccMap + by * OCCW;
        for (bx = bx0; bx <= bx1; bx++) if (m[bx] < w) return 0;
        }
    return 1;
    }

// a bounding sphere (its middle in camera space, its radius) wholly behind what the blocks it covers hold
int R3D::OccHiddenSphere(const TV& aT, int aRad)
    {
    int zn = aT.z - aRad;
    if (zn < NEAR * 4) return 0;
    if (aT.z > 65535) return 0;
    int r = iRecip[aT.z];
    int sx = (iCx16 + MulHi30(aT.x * iFx16, r)) >> 4, sy = (iCy16 - MulHi30(aT.y * iFy16, r)) >> 4;
    int wn = zn < 65536 ? iRecip[zn] >> 8 : 0;                 // its nearest point's w (as the z buffer holds)
    if (wn > 65535) wn = 65535;
    int px = aRad * (iFx16 >> 4) / zn + 1, py = aRad * (iFy16 >> 4) / zn + 1;
    int x0 = sx - px, x1 = sx + px, y0 = sy - py, y1 = sy + py;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > iW - 1) x1 = iW - 1;
    if (y1 > iH - 1) y1 = iH - 1;
    if (x0 > x1 || y0 > y1) return 0;
    int bx0 = x0 / OCCB, bx1 = x1 / OCCB, by0 = y0 / OCCB, by1 = y1 / OCCB, bx, by;
    if (bx1 >= OCCW) bx1 = OCCW - 1;
    if (by1 >= OCCH) by1 = OCCH - 1;
    for (by = by0; by <= by1; by++)
        {
        const u16* m = iOccMap + by * OCCW;
        for (bx = bx0; bx <= bx1; bx++) if (m[bx] <= wn) return 0;
        }
    return 1;
    }

void R3D::DrawInterior()
    {
    // the big pieces first (the walls, floors, the furniture you cannot see past), then what is behind them is
    // left out. (v0.4.75) Each pass nearest first, as outside: what a near wall hides is known (the occlusion map
    // made from the depths so far) before the pieces behind it come - left out whole, or triangle by triangle
    int i, pass;
    for (pass = 0; pass < 2; pass++)
    {
    if (pass == 1) OccBuild();
    int nkey = 0;
    for (i = 0; i < iINum; i++)
        {
        const Obj& o = iIObj[i];
        if (o.flag != iCell || o.type < ST_CUSTOM) continue;
        if (o.pad && !((iZoneMask >> o.pad) & 1)) continue;          // a room behind a shut door
        if (iPvsBits) { unsigned k = (unsigned)(i - iPvsFirst); if (k < (unsigned)iPvsN && !((iPvsBits[k >> 3] >> (k & 7)) & 1)) continue; }   // (hidden from here)
        const StaticDef& d = iDefs[o.type - ST_CUSTOM];
        const Mesh* m = d.mesh;
        if (!m) continue;
        int rr = (m->radius * o.scale) >> 8;
        if ((rr >= 2 * METER) != (pass == 0)) continue;
        int dx = (o.x - iCamX) >> 8, dz = (o.z - iCamZ) >> 8;
        if (d.drawDist && d.drawDist < 40)                              // small things only near (Skyrim's own pieces)
            {
            int dd = o.scale > 256 ? d.drawDist * o.scale >> 8 : d.drawDist;     // (a big one further: a hearth's fire)
            if (dx * dx + dz * dz > dd * dd) continue;
            }
        if (nkey < MAXOBJ && !(iSkip & 262144))
            {
            // (by its nearest side, in quarter metres: the halls are small)
            int dq = (r3d_isqrt(dx * dx + dz * dz) * 4) - (rr >> 6);
            if (dq < 0) dq = 0;
            if (dq > 511) dq = 511;
            iSortKey[nkey++] = ((u32)dq << 20) | (u32)i;
            continue;
            }
        DrawIStatic(o);
        }
    int* cnt = iSortCnt;
    u32* out = iSortKey + MAXOBJ;
    int b, sum = 0, j, t0 = iTris;
    for (b = 0; b <= 512; b++) cnt[b] = 0;
    for (j = 0; j < nkey; j++) cnt[iSortKey[j] >> 20]++;
    for (b = 0; b < 512; b++) { int c_ = cnt[b]; cnt[b] = sum; sum += c_; }
    for (j = 0; j < nkey; j++) out[cnt[iSortKey[j] >> 20]++] = iSortKey[j];
    for (j = 0; j < nkey; j++)
        {
        if (!(iSkip & 1024) && OccMidOk() && (iOccEvery > 0 ? j >= 8 && iTris - t0 >= iOccEvery : (j == 16 || j == 48 || j == 128) && iTris - t0 >= 500 * (j == 16 ? 1 : 2))) { OccMidBuild(); t0 = iTris; }
        DrawIStatic(iIObj[out[j] & 0xFFFFF]);
        }
    }
    }
// (v0.4.75) one of an interior's pieces: its model for how far it is, drawn
void R3D::DrawIStatic(const Obj& o)
    {
    const StaticDef& d = iDefs[o.type - ST_CUSTOM];
    const Mesh* m = d.mesh;
    if (d.lod)
        {
        // (a plainer one further - from the thing's nearest side, as outside; v0.4.61: and the middle one between,
        //  the barrow's halls and caves)
        int dx = (o.x - iCamX) >> 8, dz = (o.z - iCamZ) >> 8;
        int d2 = dx * dx + dz * dz, rr = (d.mesh->radius * o.scale) >> 16;
        int fd = d.lodCat ? iLodFar[d.lodCat] : d.lodDist, ld = fd + rr;
        int nd = d.lodCat == LODC_BUILDING ? iLodNear : d.midDist;
        if (fd < 0 || d2 > ld * ld) m = d.lod;
        else if (d.mid) { if (nd < 0) m = d.mid; else { int lm = nd + rr; if (d2 > lm * lm) m = d.mid; } }
        }
    int fr = (d.flags & SD_ANIM) ? (iAnimMs / (d.animMs ? d.animMs : 100) + o.var * 3) % m->nfr : (d.frame < m->nfr ? d.frame : 0);
    DrawMesh(*m, fr, o.x, o.y + d.yOff, o.z, o.yaw, o.scale, d.slots, (d.flags & SD_UNLIT) ? MF_UNLIT : 0);
    }

// top of the highest static with a set height (hy) whose footprint covers (x, z), not above aYMax;
// a very low value when there is none. Used to rest items on tables and counters.
static int FootIn(const StaticDef& d, const Obj& o, int aX, int aZ)
    {
    int dx = aX - o.x, dz = aZ - o.z;
    if (d.parts)
        {
        int cs = r3d_cos(o.yaw), sn = r3d_sin(o.yaw);
        int lx = ((dx * cs - dz * sn) >> 14) * 256 / o.scale, lz = ((dx * sn + dz * cs) >> 14) * 256 / o.scale;
        const int* p = d.parts;
        int i;
        for (i = 0; i < d.nparts; i++, p += 7)
            if (p[0] == PART_BOX && p[6] == d.hy && lx > p[1] && lx < p[3] && lz > p[2] && lz < p[4]) return 1;
        return 0;
        }
    if (d.hx)
        {
        int cs = r3d_cos(o.yaw), sn = r3d_sin(o.yaw);
        int lx = ((dx * cs - dz * sn) >> 14) - (d.bx * o.scale >> 8), lz = ((dx * sn + dz * cs) >> 14) - (d.bz * o.scale >> 8);
        return Abs(lx) < (d.hx * o.scale >> 8) && Abs(lz) < (d.hz * o.scale >> 8);
        }
    if (d.rad)
        {
        int r = d.rad * o.scale >> 8;
        return Abs(dx) < r && Abs(dz) < r && (dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4) < (r >> 4) * (r >> 4);
        }
    return 0;
    }

int R3D::TopAt(int aX, int aZ, int aYMax)
    {
    int best = -(1 << 30);
    if (iCell)
        {
        int i;
        for (i = 0; i < iINum; i++)
            {
            const Obj& o = iIObj[i];
            if (o.flag != iCell || o.type < ST_CUSTOM) continue;
            const StaticDef& d = iDefs[o.type - ST_CUSTOM];
            if (!d.hy) continue;
            int top = o.y + d.hy;
            if (top > aYMax || top <= best || !FootIn(d, o, aX, aZ)) continue;
            best = top;
            }
        return best;
        }
    int cx0 = Clamp((aX - 8 * METER) / (CELL * CHUNK), 0, iNCX - 1), cx1 = Clamp((aX + 8 * METER) / (CELL * CHUNK), 0, iNCX - 1);
    int cz0 = Clamp((aZ - 8 * METER) / (CELL * CHUNK), 0, iNCZ - 1), cz1 = Clamp((aZ + 8 * METER) / (CELL * CHUNK), 0, iNCZ - 1);
    int cx, cz;
    for (cz = cz0; cz <= cz1; cz++)
        for (cx = cx0; cx <= cx1; cx++)
            {
            int i = iChunkObj[cz * iNCX + cx];
            while (i >= 0)
                {
                const Obj& o = iObj[i];
                i = o.next;
                if (o.type < ST_CUSTOM) continue;
                const StaticDef& d = iDefs[o.type - ST_CUSTOM];
                if (!d.hy) continue;
                int top = o.y + d.yOff + d.hy;
                if (top > aYMax || top <= best || !FootIn(d, o, aX, aZ)) continue;
                best = top;
                }
            }
    return best;
    }

// collision inside a cell: oriented boxes / circles with a vertical extent (feet y .. head)
int R3D::BlockedIn(int aX, int aZ, int aR, int aFeetY)
    {
    int i;
    for (i = 0; i < iINum; i++)
        {
        const Obj& o = iIObj[i];
        if (o.flag != iCell || o.type < ST_CUSTOM) continue;
        const StaticDef& d = iDefs[o.type - ST_CUSTOM];
        if (d.hy && (aFeetY + 6 * METER / 4 < o.y || aFeetY + 40 > o.y + d.hy)) continue;
        int dx = aX - o.x, dz = aZ - o.z;
        if (d.hx)
            {
            int cs = r3d_cos(o.yaw), sn = r3d_sin(o.yaw);
            int lx = ((dx * cs - dz * sn) >> 14) - (d.bx * o.scale >> 8), lz = ((dx * sn + dz * cs) >> 14) - (d.bz * o.scale >> 8);
            int hx = (d.hx * o.scale >> 8) + aR, hz = (d.hz * o.scale >> 8) + aR;
            if (Abs(lx) < hx && Abs(lz) < hz) return 1;
            }
        else if (d.rad)
            {
            int r = (d.rad * o.scale >> 8) + aR;
            if (Abs(dx) < r && Abs(dz) < r && (dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4) < (r >> 4) * (r >> 4)) return 1;
            }
        }
    return 0;
    }

void R3D::BeginOverlay(u16* aOut, int aStride)
    {
    iTris = 0; iPixels = 0;
    iOut = aOut; iOutStride = aStride;
    iCB = aOut + OutX0();
    iCStride = aStride;
    iDouble = !iNative;
    SetupCamera();
    }

// (v0.4.75) the occlusion map made again among a pass's pieces (after the nearest are drawn) pays where near walls
// hide much (Forelhost's Stronghold: 9.7 -> 8.1 M instructions) and costs ~0.2 M where they hide little (a house):
// each frame's builds judged by what they left out after them (a triangle ~400 instructions saved, a whole piece
// ~16 triangles' worth) against what they cost; when they did not pay, the next frames go without (tried again
// every 8th frame)
void R3D::OccMidBuild()
    {
    if (!iOccMid) iOccMidGain0 = iOccTris + 16 * iOccSkipped;
    iOccMid++;
    OccBuild();
    }
void R3D::BeginFrame(u16* aOut, int aStride)
    {
    if (iOccMid) iOccMidOff = iOccTris + 16 * iOccSkipped - iOccMidGain0 < 500 * iOccMid;
    iOccMid = 0;
    iTris = 0; iPixels = 0; iOccSkipped = 0; iOccTris = 0;
    iFrameCount++;
    r3d_memset(iSlotInfo, 0, sizeof(iSlotInfo));
    iOut = aOut; iOutStride = aStride;
    iCB = aOut + OutX0();
    iCStride = aStride;
    iDouble = !iNative;
    ClearBorders();
    SetupCamera();
    iHalfOn = 0;
    if (iHalfRows)
        {
        // (v0.4.50) the world on 100 rows, on the even lines: the pixels' work halved; EndWorld shows each row twice
        iHalfOn = 1;
        iH = OUT_H / 2;
        iFy = (iNative ? iFx : iFx * 2) / 2; iFy16 = iFy * 16;
        iCy16 = iH * 8; iHH = iH / 2;
        iLenY = r3d_isqrt(iFy * iFy + iHH * iHH);
        iCStride = aStride * 2;
        }
    PrepareLights();
    if (iCell)
        {
        int zr = iZenR, zg = iZenG, zb = iZenB, st = iStars;
        iZenR = iFogR; iZenG = iFogG; iZenB = iFogB; iStars = 0;
        DrawSky();
        iZenR = zr; iZenG = zg; iZenB = zb; iStars = st;
        DrawInterior();
        return;
        }
    if (!(iSkip & 64)) DrawSky();
    iRelit = 0;
    // the buildings first; then the land, leaving out what they hide; then the far land behind both; then the rest
    // of the things, leaving out what all of that hides (whole things, and single triangles: DrawTri)
    if (!(iSkip & 4)) DrawObjects(0, 0);
#ifdef R3D_STATS
    g_r3dStage[0] = iPixels; g_r3dStage[4] = iTris;
#endif
    if (!(iSkip & 128)) OccBuild();
    if (!(iSkip & 2)) DrawTerrain();
#ifdef R3D_STATS
    g_r3dStage[1] = iPixels; g_r3dStage[5] = iTris;
#endif
    if (iSkip & 256) OccBuild();         // (tests: the land's too - it costs more than it saves)
    if (!(iSkip & 1)) DrawFarLand();
#ifdef R3D_STATS
    g_r3dStage[2] = iPixels; g_r3dStage[6] = iTris;
#endif
    if (!(iSkip & 4)) DrawObjects(1, 1);
#ifdef R3D_STATS
    g_r3dStage[3] = iPixels; g_r3dStage[7] = iTris;
#endif
    if (!(iSkip & 64)) DrawSunAndClouds(0);      // (v0.4.43: on what is still sky)
    }

// (v0.4.50) after the world (and the view model) on half rows: each row copied to the line below, the full rows' numbers
// back (the weather, the labels and the menus are drawn on all 200)
void R3D::EndWorld()
    {
    if (!iHalfOn) return;
    iHalfOn = 0;
    int y, w = OutW();
    u16* p = iOut + OutX0();
    for (y = 0; y < OUT_H / 2; y++) r3d_memmove(p + (2 * y + 1) * iOutStride, p + 2 * y * iOutStride, w * 2);
    iCStride = iOutStride;
    SetRes();
    }

// the weather's fog (a clear day: 45..170 m); the view distances stretch it (ApplyView)
void R3D::SetFog(int aStartM, int aEndM)
    {
    iFog0M = aStartM; iFogEndM = aEndM;
    ApplyView();
    }

// the view distance settings: the detailed land (metres), and the far land beyond it (0 = none)
void R3D::SetViewDistance(int aNearM, int aFarM)
    {
    iNearM = Clamp(aNearM, 60, FARD >> 8);
    iFarM = aFarM;
    ApplyView();
    }

void R3D::ApplyView()
    {
    int s = iFog0M, e = iFogEndM;
    iFarOn = 0;
    int fogEnd, vis;
    if (iCell)
        {
        // indoors: the cell's own fog, nothing stretched
        fogEnd = e; vis = e + 8;
        iObjD = 100 * METER; iMaxD = vis * METER;
        }
    else
        {
        // the weather's clearness (a clear day reaches 170 m) scales the chosen distances
        int nearVis = iNearM * e / 170;
        if (nearVis > iNearM) nearVis = iNearM;
        if (iFarM > iNearM && e >= 140)
            {
            iFarOn = 1;
            iFarVis = iFarM * e / 170;
            fogEnd = iFarVis;
            vis = iNearM + 24;                      // the detailed land overlaps the far land a little
            iMaxD = (iFarVis < 400 ? iFarVis : 400) * METER;
            }
        else
            {
            fogEnd = nearVis;
            vis = nearVis + 8;
            iMaxD = (vis + 5) * METER;
            }
        iObjD = (100 * iNearM / 170) * METER;
        if (iObjD > vis * METER) iObjD = vis * METER;
        }
    iFogSq = iFarOn;
    if (iFarOn) s = iNearM / 2;           // the far land: clear air near, a haze thickening towards the edge
    if (fogEnd < s + 4) fogEnd = s + 4;
    iFog0 = s * METER;
    iFogK = 256 * 65536 / ((fogEnd - s) * METER);
    if (vis > (FARD >> 8) + 24) vis = (FARD >> 8) + 24;
    iVis = vis;
    iVis2 = iVis * iVis;
    }

void R3D::ClearZ()
    {
    iOccOn = 0;
#ifdef USE_ASM
    fill32((u32*)iZB, 0, iW * iH / 2);
#else
    r3d_memset(iZB, 0, iW * iH * 2);
#endif
    }

int R3D::ProjectWorld(int aX, int aY, int aZ, int& aSx, int& aSy)
    {
    TV v;
    TransformPt(aX, aY, aZ, v);
    if (v.z < NEAR * 2) return 0;
    Outcode(v);
    Project(v);
    aSx = OutX0() + ((v.sx >> 4) << iDouble);
    aSy = v.sy >> 4;
    return v.code == 0 ? 1 : 2;
    }

void R3D::Text(int aX, int aY, const char* aS, u16 aCol)
    {
    for (; *aS; aS++, aX += 6)
        {
        int c = (u8)*aS;
        if (c < 32 || c > 126) continue;
        const u8* g = KFont + (c - 32) * 8;
        int y, x;
        for (y = 0; y < 8; y++)
            {
            int py = aY + y;
            if (py < 0 || py >= OUT_H) continue;
            u16* row = iOut + py * iOutStride;
            int bits = g[y];
            if (!bits) continue;
            for (x = 0; x < 5; x++)
                if (bits & (16 >> x))
                    {
                    int px = aX + x;
                    if (px >= 0 && px < OUT_W) row[px] = aCol;
                    }
            }
        }
    }

void R3D::TextShadow(int aX, int aY, const char* aS, u16 aCol)
    {
    Text(aX + 1, aY + 1, aS, 0);
    Text(aX, aY, aS, aCol);
    }

int R3D::TextW(const char* aS)
    {
    int n = 0;
    while (aS[n]) n++;
    return n * 6;
    }

void R3D::Shade(int aX, int aY, int aW, int aH)
    {
    int x, y;
    int x0 = Clamp(aX, 0, OUT_W), x1 = Clamp(aX + aW, 0, OUT_W);
    for (y = Clamp(aY, 0, OUT_H); y < Clamp(aY + aH, 0, OUT_H); y++)
        {
        u16* row = iOut + y * iOutStride;
        for (x = x0; x < x1; x++) row[x] = (u16)((row[x] >> 2) & 0x39E7);
        }
    }

void R3D::Rect(int aX, int aY, int aW, int aH, u16 aCol)
    {
    int x, y;
    int x0 = Clamp(aX, 0, OUT_W), x1 = Clamp(aX + aW, 0, OUT_W);
    for (y = Clamp(aY, 0, OUT_H); y < Clamp(aY + aH, 0, OUT_H); y++)
        {
        u16* row = iOut + y * iOutStride;
        for (x = x0; x < x1; x++) row[x] = aCol;
        }
    }

void R3D::Tint(int aX, int aY, int aW, int aH, u16 aCol)
    {
    int x, y;
    int x0 = Clamp(aX, 0, OUT_W), x1 = Clamp(aX + aW, 0, OUT_W);
    u16 c = (u16)((aCol >> 1) & 0x7BEF);
    for (y = Clamp(aY, 0, OUT_H); y < Clamp(aY + aH, 0, OUT_H); y++)
        {
        u16* row = iOut + y * iOutStride;
        for (x = x0; x < x1; x++) row[x] = (u16)(((row[x] >> 1) & 0x7BEF) + c);
        }
    }

const unsigned char* GameFont() { return KFont; }
