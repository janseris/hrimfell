// bakeland: the land file the game reads (data/hrimfell.land, version 2) from tools/make_land.py's (version 1, kept
// as data/hrimfell_v1.land): the game builds its world on the whole land once, here, and what it changes (the city's
// hill and terraces, the barrow's platforms, Riverwood's ground, the roads, rock on the steep, no grown trees round
// what was built) is written into the file. The game then only reads it, a tile at a time round the player.
//   g++ -O1 -DR3D_FULLLAND -Iinc -o host/bakeland host/bakeland.cpp src/*.cpp (the game's own) && ./host/bakeland
//
// Version 2 (little-endian):
//   "HLND", u32 2, u32 gx, u32 gz (cells), i32 offset (cm), u32 cw, u32 ch (the coarse land: every 4th vertex),
//   u32 ntx, u32 ntz (tiles of 32 x 32 vertices; past the land's edge its last vertex again)
//   coarse heights: cw * ch int16 (engine steps, rows from the south); coarse ground: cw * ch bytes (VM_* bits)
//   water: per chunk of 8 x 8 cells, int16 (gz / 8 rows of gx / 8), -32768 none
//   tile offsets: ntx * ntz + 1 u32 (from the start of the file; the last: the end)
//   each tile: 1024 heights as the difference from left + below - below-left within the tile (a signed byte, or
//   -128 then an int16), then its ground run-length coded: (count - 1, value) byte pairs
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
static int Rd(void* c, u8* b, int n) { return (int)fread(b, 1, n, (FILE*)c); }
static int Sk(void* c, int p) { return fseek((FILE*)c, p, SEEK_SET) == 0; }
static void U32(std::vector<u8>& o, unsigned v) { for (int k = 0; k < 4; k++) o.push_back((u8)(v >> (8 * k))); }
static void S16(std::vector<u8>& o, int v) { o.push_back((u8)(v & 255)); o.push_back((u8)((v >> 8) & 255)); }
int main(int argc, char** argv)
    {
    const char* src = argc > 1 ? argv[1] : "data/hrimfell_v1.land";
    const char* dst = argc > 2 ? argv[2] : "data/hrimfell.land";
    FILE* f = fopen(src, "rb");
    if (!f) { printf("no %s\n", src); return 1; }
    u8 hdr[20]; fread(hdr, 1, 20, f); fseek(f, 0, SEEK_SET);
    int offCm = hdr[16] | (hdr[17] << 8) | (hdr[18] << 16) | (hdr[19] << 24);
    int n = Game::MemNeeded();
    u8* mem = (u8*)calloc(1, n);
    Game* g = (Game*)calloc(1, sizeof(Game));
    g->iLandRead = Rd; g->iLandSeek = Sk; g->iLandCtx = f;
    g->Init(mem);
    R3D& r = *g->iR;
    if (!r.iLandLoaded || r.iLandBaked) { printf("the land did not load as version 1\n"); return 1; }
    int gx = r.iGX, gz = r.iGZ, W = r.iHS;
    int cw = gx / CSTEP + 1, ch = gz / CSTEP + 1;
    int ntx = (gx + 1 + LTILE - 1) / LTILE, ntz = (gz + 1 + LTILE - 1) / LTILE;
    std::vector<u8> o;
    o.push_back('H'); o.push_back('L'); o.push_back('N'); o.push_back('D');
    U32(o, 2); U32(o, gx); U32(o, gz); U32(o, (unsigned)offCm); U32(o, cw); U32(o, ch); U32(o, ntx); U32(o, ntz);
    int x, z, i;
    for (z = 0; z < ch; z++) for (x = 0; x < cw; x++) S16(o, r.iHeight[(z * CSTEP) * W + x * CSTEP]);
    for (z = 0; z < ch; z++) for (x = 0; x < cw; x++) o.push_back(r.iMat[(z * CSTEP) * W + x * CSTEP]);
    for (i = 0; i < r.iNCX * r.iNCZ; i++) S16(o, r.iChunkWater[i]);
    size_t table = o.size();
    for (i = 0; i <= ntx * ntz; i++) U32(o, 0);
    long bytesH = 0, bytesM = 0;
    for (int tz = 0; tz < ntz; tz++)
        for (int tx = 0; tx < ntx; tx++)
            {
            unsigned at = (unsigned)o.size();
            int t = tz * ntx + tx;
            for (int k = 0; k < 4; k++) o[table + t * 4 + k] = (u8)(at >> (8 * k));
            short h[LTILE][LTILE]; u8 m[LTILE * LTILE];
            for (int j = 0; j < LTILE; j++)
                for (int ii = 0; ii < LTILE; ii++)
                    {
                    int vx = tx * LTILE + ii, vz = tz * LTILE + j;
                    if (vx > gx) vx = gx;
                    if (vz > gz) vz = gz;
                    h[j][ii] = r.iHeight[vz * W + vx]; m[j * LTILE + ii] = r.iMat[vz * W + vx];
                    }
            size_t s0 = o.size();
            for (int j = 0; j < LTILE; j++)
                for (int ii = 0; ii < LTILE; ii++)
                    {
                    int p;
                    if (j == 0) p = ii ? h[j][ii - 1] : 0;
                    else if (ii == 0) p = h[j - 1][ii];
                    else p = h[j][ii - 1] + h[j - 1][ii] - h[j - 1][ii - 1];
                    int d = h[j][ii] - p;
                    if (d >= -127 && d <= 127) o.push_back((u8)(d & 255));
                    else { o.push_back(128); S16(o, h[j][ii]  - p); }
                    }
            size_t s1 = o.size();
            for (int k = 0; k < LTILE * LTILE; )
                {
                int c = 1;
                while (k + c < LTILE * LTILE && c < 256 && m[k + c] == m[k]) c++;
                o.push_back((u8)(c - 1)); o.push_back(m[k]);
                k += c;
                }
            bytesH += (long)(s1 - s0); bytesM += (long)(o.size() - s1);
            }
    unsigned end = (unsigned)o.size();
    for (int k = 0; k < 4; k++) o[table + ntx * ntz * 4 + k] = (u8)(end >> (8 * k));
    FILE* w = fopen(dst, "wb");
    fwrite(&o[0], 1, o.size(), w); fclose(w);
    // reference samples for host/landcheck: the ground's height and kind at points all over the land
    FILE* sf = fopen("host/land_samples.txt", "w");
    for (i = 0; i < 20000; i++)
        {
        u32 hh = r3d_hash(i, 3, 99);
        int sx = (int)(hh % (unsigned)(gx * CELL)), sz = (int)((hh >> 7) * 2654435761u % (unsigned)(gz * CELL));
        fprintf(sf, "%d %d %d %d\n", sx, sz, r.GroundAt(sx, sz), r.MaterialAt(sx, sz));
        }
    fclose(sf);
    printf("baked %s: land %d x %d cells, coarse %d x %d, %d x %d tiles; %ld bytes (heights %ld, ground %ld, coarse %d)\n",
           dst, gx, gz, cw, ch, ntx, ntz, (long)o.size(), bytesH, bytesM, cw * ch * 3);
    return 0;
    }
