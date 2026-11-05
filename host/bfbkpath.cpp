// (v0.4.55) Skyrim's own barrow (data/bleakfalls.har): ways over its floors, round its walls (the game's FloorAt and
// BlockedAt), on a 40 cm grid - the temple's main door to its door down to the sanctum, the sanctum's door to the
// word wall and on to its way out, and Arvel's run from the web towards the claw's door (written for gen_bfb_kit.py)
//   host/bfbkpath [arvel_path.txt]
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <queue>
#include <unordered_set>
struct DunProp { int type, x, y, z, yaw, item; };
#include "bfb_kit.inc"
static Game* g;
enum { G = 25 * METER / 100 };
struct Node { int x, z, y; };
int GameKitFloor(const Game& g, int aCell, int aX, int aZ, int aFeetY, int* aFound);
static int Feet(int cell, int x, int z, int y) { int f; int r = GameKitFloor(*g, cell, x, z, y + METER / 2, &f); return f ? r : -1000000; }
// a way from (ax, az, ay) to within 1.5 m of (bx, bz): the cells' squares visited by their floor's height band
static std::vector<Node> Way(int cell, int ax, int az, int ay, int bx, int bz, int by, int* aLen)
    {
    int x0 = (ax < bx ? ax : bx) - 160 * METER, z0 = (az < bz ? az : bz) - 160 * METER;
    int W = (abs(ax - bx) + 320 * METER) / G + 1, H = (abs(az - bz) + 320 * METER) / G + 1;
    std::unordered_set<long long> seen;                    // a square at each height (by the metre: floors over one another)
    std::vector<Node> nodes; std::vector<int> from;
    std::queue<int> q;
    Node s = { ax, az, Feet(cell, ax, az, ay) };
    nodes.push_back(s); from.push_back(-1); q.push(0);
    int found = -1;
    while (!q.empty() && nodes.size() < 2000000)
        {
        int i = q.front(); q.pop();
        Node n = nodes[i];
        if (abs(n.x - bx) < 2 * METER && abs(n.z - bz) < 2 * METER && abs(n.y - by) < 2 * METER) { found = i; break; }
        static const int D[8][2] = { {1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1} };
        for (int k = 0; k < 8; k++)
            {
            int x = n.x + D[k][0] * G, z = n.z + D[k][1] * G;
            int gx = (x - x0) / G, gz = (z - z0) / G;
            if (gx < 0 || gz < 0 || gx >= W || gz >= H) continue;
            int y = Feet(cell, x, z, n.y);
            if (y < -900000 || y > n.y + 60 * METER / 100 || y < n.y - 4 * METER) continue;     // a step up, or a drop of up to 2 m
            if (g->BlockedAt(cell, x, z, 25 * METER / 100, y)) continue;
            long long si = ((long long)gz * W + gx) * 4096 + ((y / METER) & 4095);
            if (seen.count(si)) continue;
            seen.insert(si);
            Node m = { x, z, y };
            nodes.push_back(m); from.push_back(i); q.push((int)nodes.size() - 1);
            }
        }
    std::vector<Node> path;
    if (found < 0)
        {
        if (getenv("DUMP")) { FILE* f = fopen(getenv("DUMP"), "w"); for (size_t i = 0; i < nodes.size(); i++) fprintf(f, "%d %d %d\n", (int)((long long)(nodes[i].x - 100 * METER) * 100 / METER), (int)((long long)(nodes[i].z - 100 * METER) * 100 / METER), nodes[i].y); fclose(f); unsetenv("DUMP"); }
        int best = 1 << 30, bi = 0;
        for (size_t i = 0; i < nodes.size(); i++) { int d = abs(nodes[i].x - bx) + abs(nodes[i].z - bz); if (d < best) { best = d; bi = (int)i; } }
        printf("   start feet %d (asked %d) blocked %d; reached %d squares, nearest %d m off at (%d, %d cm, feet %d)\n", s.y, ay, g->BlockedAt(cell, ax, az, 30 * METER / 100, s.y), (int)nodes.size(), best / METER,
               (int)((long long)(nodes[bi].x - 100 * METER) * 100 / METER), (int)((long long)(nodes[bi].z - 100 * METER) * 100 / METER), nodes[bi].y);
        *aLen = -1; return path;
        }
    for (int i = found; i >= 0; i = from[i]) path.push_back(nodes[i]);
    std::vector<Node> r(path.rbegin(), path.rend());
    *aLen = (int)r.size();
    return r;
    }
static int Cm(int e);
static void Report(const char* what, int cell, const DoorLink* a, int ax, int az, int ay, int bx, int bz, int by, std::vector<Node>* aOut)
    {
    int n;
    std::vector<Node> p = Way(cell, ax, az, ay, bx, bz, by, &n);
    if (n < 0) { printf("%-44s NO WAY\n", what); return; }
    printf("%-44s %d squares (%d m)\n", what, n, n * 40 / 100);
    for (size_t i = 1; i < p.size(); i++)
        if (p[i].y < p[i - 1].y - 60 * METER / 100)
            printf("     drop %d cm at (%d, %d) cm -> skyrim (%d, %d, %d)\n", (p[i - 1].y - p[i].y) * 100 / METER, Cm(p[i].x), Cm(p[i].z), Cm(p[i].x) * 100 / 143, Cm(p[i].z) * 100 / 143, (p[i].y - 100 * METER) * 100 / METER * 100 / 143);
    if (aOut) *aOut = p;
    }
static int Cm(int e) { return (int)((long long)(e - 100 * METER) * 100 / METER); }
int main(int argc, char** argv)
    {
    int n = Game::MemNeeded(); unsigned char* mem = (unsigned char*)calloc(1, n); g = (Game*)calloc(1, sizeof(Game));
    g->Init(mem); g->NewGame();
    if (!g->iBfbKit) { printf("no data/bleakfalls.har\n"); return 1; }
    g->EnterCell(CELL_BFTEMPLE, g->Door(7).ix, g->Door(7).iz, 0, g->Door(7).iy);
    for (int k = 8; k <= 10; k++) g->iBfTimer[k] = 400;          // (the gates up, the claw door down)
    const DoorLink& d7 = g->Door(7); const DoorLink& d9 = g->Door(9); const DoorLink& d10 = g->Door(10);
    int ax = d7.ix + ((r3d_sin(d7.iyaw) * 6 * METER / 5) >> 14), az = d7.iz + ((r3d_cos(d7.iyaw) * 6 * METER / 5) >> 14);
    Report("temple: main door -> door down to the sanctum", CELL_BFTEMPLE, &d7, ax, az, d7.iy, d9.ox, d9.oz, d9.oy, 0);
    if (getenv("FROM"))
        {
        double fx, fy, fz; sscanf(getenv("FROM"), "%lf,%lf,%lf", &fx, &fy, &fz);
        int qx = 100 * METER + (int)(fx * 143 / 100) * METER / 100, qz = 100 * METER + (int)(fy * 143 / 100) * METER / 100, qy = 100 * METER + (int)(fz * 143 / 100) * METER / 100;
        Report("temple: FROM -> door down to the sanctum", CELL_BFTEMPLE, 0, qx, qz, qy, d9.ox, d9.oz, d9.oy, 0);
        std::vector<Node> back;
        Report("temple: FROM -> main door", CELL_BFTEMPLE, 0, qx, qz, qy, ax, az, d7.iy, &back);
        for (int i = (int)back.size() - 1; i > 0; i--)
            {
            const Node& a = back[i]; const Node& b = back[i - 1];
            int y = Feet(CELL_BFTEMPLE, b.x, b.z, a.y);
            int bl = g->BlockedAt(CELL_BFTEMPLE, b.x, b.z, 30 * METER / 100, y);
            if (y < -900000 || y > a.y + 60 * METER / 100 || y < a.y - 2 * METER || bl)
                { printf("   back the other way: stuck at (%d, %d) cm, skyrim (%d, %d): feet %d -> %d (path's %d) blocked %d\n", Cm(a.x), Cm(a.z), Cm(a.x) * 100 / 143, Cm(a.z) * 100 / 143, a.y, y, b.y, bl); break; }
            }
        }
    std::vector<Node> arv;
    int wx = 100 * METER + BFK_WEB_X * METER / 100, wz = 100 * METER + BFK_WEB_Z * METER / 100, wy = 100 * METER + BFK_WEB_Y * METER / 100;
    int px = 100 * METER + BFK_PLATE_X * METER / 100, pz = 100 * METER + BFK_PLATE_Z * METER / 100, py = 100 * METER + BFK_PLATE_Y * METER / 100;
    Report("temple: Arvel's web -> the pressure plate", CELL_BFTEMPLE, 0, wx, wz, wy, px, pz, py, &arv);
    g->EnterCell(CELL_BFSANCTUM, d9.ix, d9.iz, 0, d9.iy);
    int sx = d9.ix + ((r3d_sin(d9.iyaw) * 6 * METER / 5) >> 14), sz = d9.iz + ((r3d_cos(d9.iyaw) * 6 * METER / 5) >> 14);
    Report("sanctum: door from the temple -> way out", CELL_BFSANCTUM, 0, sx, sz, d9.iy, d10.ix, d10.iz, d10.iy, 0);
    if (argc > 1 && arv.size())
        {
        // simplified: a point where the way turns, at most every 4 m
        FILE* f = fopen(argv[1], "w");
        size_t last = 0;
        for (size_t i = 1; i < arv.size(); i++)
            {
            bool keep = i + 1 == arv.size() || i - last >= 10;
            if (!keep) { int dx0 = arv[i].x - arv[i - 1].x, dz0 = arv[i].z - arv[i - 1].z, dx1 = arv[i + 1].x - arv[i].x, dz1 = arv[i + 1].z - arv[i].z; keep = dx0 != dx1 || dz0 != dz1; }
            if (keep) { fprintf(f, "%d %d\n", Cm(arv[i].x), Cm(arv[i].z)); last = i; }
            }
        fclose(f);
        }
    return 0;
    }
