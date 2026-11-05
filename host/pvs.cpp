// what can be seen from where, inside (v0.4.55): for each kit cell of an area, from points on its floor (Skyrim's
// navmesh, the kit's floor triangles) the pieces seen, looked at all round (a cube of six pictures, every piece's
// triangles drawn with a depth buffer: a piece is seen when one of its pixels is nearest; cut-out ones - roots, webs -
// hide nothing). Gathered by region (squares of RS units, the feet in bands of BH), each region also taking its
// neighbours' (walking a step across it), and written to the end of the area's file:
//   pvs AREA file.har        (AREA 0 the main one, 1 the barrow)
// "HPVS" n, per cell: cell, pieces, regions, RS, BH, the regions (x, y, band, 0: shorts, sorted), their bits; then
// the HPVS's offset and "PVS1" (game_kit.cpp LoadArea, GameKitPvs)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <map>
#include <tuple>
#include <algorithm>
#include <thread>
#include <atomic>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset

enum { RS = 256, BH = 160, RES = 128 };
static const float NEAR_ = 6.0f;               // engine units (2.3 cm)
struct Tri { float p[9]; int obj; int key; };
static std::vector<Tri> T;
static thread_local float Z[RES * RES]; static thread_local int ID[RES * RES];     // (v0.4.74: one each a thread)

// one face of the cube: the camera's axes (right, up, forward)
static void Face(int f, float* R, float* U, float* F)
    {
    static const float A[6][9] = {
        { 0,0,-1, 0,1,0, 1,0,0 }, { 0,0,1, 0,1,0, -1,0,0 }, { 1,0,0, 0,0,-1, 0,1,0 },
        { 1,0,0, 0,0,1, 0,-1,0 }, { 1,0,0, 0,1,0, 0,0,1 }, { -1,0,0, 0,1,0, 0,0,-1 } };
    for (int i = 0; i < 3; i++) { R[i] = A[f][i]; U[i] = A[f][3 + i]; F[i] = A[f][6 + i]; }
    }
struct V3 { float x, y, z; };
static void RasterTri(V3 a, V3 b, V3 c, int id, int writeZ, std::vector<char>& seen)
    {
    // screen: x/z, y/z in -1..1 -> 0..RES; depth 1/z
    float h = RES * 0.5f;
    float ax = h + a.x / a.z * h, ay = h - a.y / a.z * h, az = 1.0f / a.z;
    float bx = h + b.x / b.z * h, by = h - b.y / b.z * h, bz = 1.0f / b.z;
    float cx = h + c.x / c.z * h, cy = h - c.y / c.z * h, cz = 1.0f / c.z;
    float d = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
    if (fabsf(d) < 1e-6f) return;
    int x0 = (int)floorf(fminf(ax, fminf(bx, cx))), x1 = (int)ceilf(fmaxf(ax, fmaxf(bx, cx)));
    int y0 = (int)floorf(fminf(ay, fminf(by, cy))), y1 = (int)ceilf(fmaxf(ay, fmaxf(by, cy)));
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0; if (x1 > RES) x1 = RES; if (y1 > RES) y1 = RES;
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++)
            {
            float px = x + 0.5f, py = y + 0.5f;
            float w0 = ((bx - px) * (cy - py) - (by - py) * (cx - px)) / d;
            float w1 = ((cx - px) * (ay - py) - (cy - py) * (ax - px)) / d;
            float w2 = 1 - w0 - w1;
            if (w0 < 0 || w1 < 0 || w2 < 0) continue;
            float z = w0 * az + w1 * bz + w2 * cz;          // (1/z: bigger is nearer)
            int k = y * RES + x;
            if (writeZ) { if (z > Z[k]) { Z[k] = z; ID[k] = id; } }
            else if (z >= Z[k] * 0.999f) { seen[id] = 1; return; }
            }
    }
static int Clip(V3* in, int n, V3* out)
    {
    int m = 0;
    for (int i = 0; i < n; i++)
        {
        V3 a = in[i], b = in[(i + 1) % n];
        int ia = a.z >= NEAR_, ib = b.z >= NEAR_;
        if (ia) out[m++] = a;
        if (ia != ib) { float t = (NEAR_ - a.z) / (b.z - a.z); out[m++] = { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, NEAR_ }; }
        }
    return m;
    }
// the pieces seen from (ex, ey, ez)
static void Look(float ex, float ey, float ez, std::vector<char>& seen)
    {
    for (int f = 0; f < 6; f++)
        {
        float R[3], U[3], F[3];
        Face(f, R, U, F);
        for (int k = 0; k < RES * RES; k++) { Z[k] = 0; ID[k] = -1; }
        for (int pass = 0; pass < 2; pass++)
            for (size_t i = 0; i < T.size(); i++)
                {
                const Tri& t = T[i];
                if ((pass == 0) == (t.key != 0)) continue;
                if (pass == 1 && seen[t.obj]) continue;
                V3 v[3];
                int out = 0, behind = 0;
                for (int j = 0; j < 3; j++)
                    {
                    float dx = t.p[j * 3] - ex, dy = t.p[j * 3 + 1] - ey, dz = t.p[j * 3 + 2] - ez;
                    v[j].x = dx * R[0] + dy * R[1] + dz * R[2]; v[j].y = dx * U[0] + dy * U[1] + dz * U[2]; v[j].z = dx * F[0] + dy * F[1] + dz * F[2];
                    if (v[j].z < NEAR_) behind++;
                    if (v[j].x > v[j].z) out |= 1; else if (v[j].x < -v[j].z) out |= 2; else out |= 64;
                    }
                if (behind == 3) continue;
                // all three off one side
                int ox = 0, oxn = 0, oy = 0, oyn = 0;
                for (int j = 0; j < 3; j++) { ox += v[j].x > v[j].z; oxn += v[j].x < -v[j].z; oy += v[j].y > v[j].z; oyn += v[j].y < -v[j].z; }
                if (ox == 3 || oxn == 3 || oy == 3 || oyn == 3) continue;
                V3 c[8]; int n = 3;
                if (behind) n = Clip(v, 3, c); else { c[0] = v[0]; c[1] = v[1]; c[2] = v[2]; }
                for (int j = 1; j + 1 < n; j++) RasterTri(c[0], c[j], c[j + 1], t.obj, pass == 0, seen);
                }
        for (int k = 0; k < RES * RES; k++) if (ID[k] >= 0) seen[ID[k]] = 1;
        }
    }

int main(int argc, char** argv)
    {
    if (argc < 3) { printf("pvs AREA file.har\n"); return 1; }
    int area = atoi(argv[1]);
    int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64);
    u8* m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
    emu_init(m, 0);
    Game* g = (Game*)m;
    emu_mode(m, 0);
    g->UseArea(area);
    R3D& r = *g->iR;
    std::vector<unsigned char> blob;
    auto put = [&](int v) { for (int i = 0; i < 4; i++) blob.push_back((v >> (8 * i)) & 255); };
    int ncells = 0;
    std::vector<unsigned char> body;
    // (v0.4.65) the jobs: each interior kit cell, then the outside (cell 0x7FFF: all the statics the area puts
    // outside, looked at from its navmesh floors - Whiterun's streets - in Skyrim's world units)
    int off = r.iLandLoaded ? r.iLandOffset : 0;
    for (int k = 0; k <= g->iKitCellN && k <= 32; k++)
        {
        int outside = k == g->iKitCellN || k == 32;
        const KitCell* cp = outside ? 0 : &g->iKitCells[k];
        std::vector<short> fl;                 // floors: x, y, z each corner (interiors: the cell's units; outside: the world's)
        int cellid, no;
        const Obj* objs;
        if (!outside)
            {
            const KitCell& c = *cp;
            if (c.cell == CELL_NONE || g->iKitObj0[k] < 0 || !c.nfloor) continue;
            if (getenv("CELL") && atoi(getenv("CELL")) != c.cell) continue;
            cellid = c.cell; no = c.nstat; objs = r.iIObj + g->iKitObj0[k];
            fl.assign(c.floor, c.floor + c.nfloor * 9);
            }
        else
            {
            if (getenv("CELL") && atoi(getenv("CELL")) != 0) continue;
            if (g->iOutObjMark < 0 || g->iOutObjN <= 0) continue;
            cellid = 0x7FFF; no = g->iOutObjN; objs = r.iObj + g->iOutObjMark;
            for (int j = 0; j < g->iKitCellN; j++)
                {
                const KitCell& c = g->iKitCells[j];
                if (c.cell != CELL_NONE || !c.nwall || c.wall[4] <= c.wall[5]) continue;     // (the navmesh's: Whiterun's)
                for (int f = 0; f < c.nfloor; f++)
                    for (int q = 0; q < 3; q++) { fl.push_back((short)0); }
                fl.resize(fl.size() - c.nfloor * 3);
                for (int f = 0; f < c.nfloor * 3; f++)
                    {
                    const short* v = c.floor + f * 3;
                    fl.push_back((short)((c.ox + v[0]) / 2)); fl.push_back((short)((c.oy + v[1]) / 2)); fl.push_back(v[2]);
                    }
                }
            if (fl.empty()) continue;
            }
        int nfl = (int)fl.size() / 9;
        // (outside the floors are kept halved in x, y - the world's units pass a short - and doubled again below)
        int half = outside ? 2 : 1;
        // every piece's triangles, in the world
        T.clear();
        for (int i = 0; i < no; i++)
            {
            const Obj& o = objs[i];
            if (o.type < ST_CUSTOM) continue;
            const StaticDef& d = r.iDefs[o.type - ST_CUSTOM];
            const Mesh* me = d.mesh;
            if (!me) continue;
            float cs = r3d_cos(o.yaw) / 16384.0f, sn = r3d_sin(o.yaw) / 16384.0f, sc = o.scale / 256.0f;
            for (int f = 0; f < me->nf; f++)
                {
                Tri t; t.obj = i;
                int sl = me->f[f * 4 + 3];
                int tb = d.slots[sl * 4 + 3];
                t.key = ((tb & 0xC0) == 0xC0 && r.iSkyKey && (tb & 63) < r.iSkyN && r.iSkyKey[tb & 63])
                     || ((tb & 0xC0) == 0x40 && r.iLibKey && (tb & 63) < r.iLibN && r.iLibKey[tb & 63]);   // (the library's cut-outs too)
                for (int j = 0; j < 3; j++)
                    {
                    const short* V = me->v + me->f[f * 4 + j] * 3;
                    float x = V[0] * sc, y = V[1] * sc, z = V[2] * sc;
                    t.p[j * 3] = o.x + cs * x + sn * z; t.p[j * 3 + 1] = o.y + d.yOff + y; t.p[j * 3 + 2] = o.z - sn * x + cs * z;
                    }
                T.push_back(t);
                }
            }
        // (v0.4.74) the ruin's hinged doors, shut: they hide what is behind them (no pieces of their own - the game adds
        // what can be seen from beyond a door while it is open: GameKitPvs)
        if (!outside)
            for (int i = 0; i < g->iS.nprop; i++)
                {
                const Prop& p = g->iS.prop[i];
                if (p.type != PT_DOOR || p.count <= 0 || p.cell != cp->cell) continue;
                if (p.model < 0 || p.model >= g->iKitMeshN || p.model >= Game::MAXKIT || g->iKitType[p.model] < ST_CUSTOM) continue;
                const Mesh* me = &g->iMKit[p.model];
                int cs = r3d_cos(p.yaw), sn = r3d_sin(p.yaw), lx = p.vx * METER / 100, lz = p.vz * METER / 100;
                float mx = p.x - ((lx * cs + lz * sn) >> 14), mz = p.z - ((-lx * sn + lz * cs) >> 14);
                float fc = cs / 16384.0f, fs = sn / 16384.0f;
                int nd = 0;
                for (int f = 0; f < me->nf; f++)
                    {
                    Tri t; t.obj = -1; t.key = 0;
                    for (int j = 0; j < 3; j++)
                        {
                        const short* V = me->v + me->f[f * 4 + j] * 3;
                        t.p[j * 3] = mx + fc * V[0] + fs * V[2]; t.p[j * 3 + 1] = p.y + V[1]; t.p[j * 3 + 2] = mz - fs * V[0] + fc * V[2];
                        }
                    T.push_back(t); nd++;
                    }
                }
        // each piece's box (for the near ones)
        std::vector<float> bb(no * 6);
        for (int o = 0; o < no; o++) { bb[o * 6] = bb[o * 6 + 1] = bb[o * 6 + 2] = 1e30f; bb[o * 6 + 3] = bb[o * 6 + 4] = bb[o * 6 + 5] = -1e30f; }
        for (auto& t : T)
            if (t.obj >= 0)
            for (int j = 0; j < 3; j++)
                for (int q = 0; q < 3; q++) { bb[t.obj * 6 + q] = std::min(bb[t.obj * 6 + q], t.p[j * 3 + q]); bb[t.obj * 6 + 3 + q] = std::max(bb[t.obj * 6 + 3 + q], t.p[j * 3 + q]); }
        // the points looked from: on a lattice of 64 units over the floor triangles (and each one's middle)
        std::map<std::tuple<int, int, int>, std::vector<char>> reg;
        auto rdiv = [](int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); };
        std::map<std::tuple<int, int, int>, int> samples;
        std::vector<std::tuple<int, int, int>> pts;
        int L = outside ? 128 : 64;            // (outside: a lattice of 128 units - the streets are wide, the job large)
        for (int f = 0; f < nfl; f++)
            {
            int t[9];
            for (int q = 0; q < 9; q++) t[q] = (q % 3 == 2) ? fl[f * 9 + q] : fl[f * 9 + q] * half;
            int x0 = std::min(t[0], std::min(t[3], t[6])), x1 = std::max(t[0], std::max(t[3], t[6]));
            int y0 = std::min(t[1], std::min(t[4], t[7])), y1 = std::max(t[1], std::max(t[4], t[7]));
            double d = (double)(t[4] - t[7]) * (t[0] - t[6]) + (double)(t[6] - t[3]) * (t[1] - t[7]);
            if (fabs(d) < 1) continue;
            std::vector<std::pair<int, int>> at;
            at.push_back({ (t[0] + t[3] + t[6]) / 3, (t[1] + t[4] + t[7]) / 3 });
            for (int y = (rdiv(y0, L)) * L; y <= y1; y += L)
                for (int x = (rdiv(x0, L)) * L; x <= x1; x += L) at.push_back({ x, y });
            for (auto& p : at)
                {
                int px = p.first, py = p.second;
                double w0 = ((double)(t[4] - t[7]) * (px - t[6]) + (double)(t[6] - t[3]) * (py - t[7])) / d;
                double w1 = ((double)(t[7] - t[1]) * (px - t[6]) + (double)(t[0] - t[6]) * (py - t[7])) / d;
                double w2 = 1 - w0 - w1;
                if (w0 < -0.01 || w1 < -0.01 || w2 < -0.01) continue;
                int z = (int)(w0 * t[2] + w1 * t[5] + w2 * t[8]);
                // one point to a lattice spot and height band
                auto key = std::make_tuple(rdiv(px, L), rdiv(py, L), rdiv(z, 80));
                if (samples.count(key)) continue;
                samples[key] = 1;
                pts.push_back(std::make_tuple(px, py, z));
                }
            }
        printf("cell %d (%s): %d pieces, %d triangles, %d points\n", cellid, outside ? "outside" : cp->name, no, (int)T.size(), (int)pts.size()); fflush(stdout);
        // (v0.4.74) the points shared out among threads (THREADS, else all the cores): each point's own set, then
        // joined into the regions in the points' order - the same tables as one thread makes
        std::vector<std::vector<char>> ps(pts.size());
        std::atomic<int> next(0), done(0);
        auto work = [&]()
            {
            for (;;)
                {
                int i = next++;
                if (i >= (int)pts.size()) return;
                int px = std::get<0>(pts[i]), py = std::get<1>(pts[i]), z = std::get<2>(pts[i]);
                std::vector<char>& s = ps[i];
                s.assign(no, 0);
                // the eyes: standing, and crouched (engine units)
                float ex = 100 * METER + px * METER / 70.0f, ez = 100 * METER + py * METER / 70.0f, fy = 100 * METER + z * METER / 70.0f;
                if (outside) { ex = (px + 233472) * 3.6576f; ez = (py + 176128) * 3.6576f; fy = z * 143 / 100 * 256 / 100.0f + off; }
                Look(ex, fy + 410, ez, s);
                Look(ex, fy + 200, ez, s);
                // (outside) and whatever is within 8 m of the eyes, seen or not from this one point: the lattice is coarse,
                // and a piece right before the eyes is clipped by the near plane here, not in the game
                if (outside)
                    for (int o = 0; o < no; o++)
                        {
                        if (objs[o].type < ST_CUSTOM) { s[o] = 1; continue; }        // (fires, the game's own: not judged here)
                        if (s[o] || bb[o * 6] > bb[o * 6 + 3]) continue;
                        float dx = std::max(0.0f, std::max(bb[o * 6] - ex, ex - bb[o * 6 + 3]));
                        float dy = std::max(0.0f, std::max(bb[o * 6 + 1] - (fy + 300), (fy + 300) - bb[o * 6 + 4]));
                        float dz = std::max(0.0f, std::max(bb[o * 6 + 2] - ez, ez - bb[o * 6 + 5]));
                        if (dx * dx + dy * dy + dz * dz < 8.0f * METER * 8.0f * METER) s[o] = 1;
                        }
                int d = ++done;
                if (d % 200 == 0) { printf("  %d / %d\n", d, (int)pts.size()); fflush(stdout); }
                }
            };
        int nth = getenv("THREADS") ? atoi(getenv("THREADS")) : (int)std::thread::hardware_concurrency();
        if (nth < 1) nth = 1;
        std::vector<std::thread> th;
        for (int t = 1; t < nth; t++) th.emplace_back(work);
        work();
        for (auto& t : th) t.join();
        for (size_t i = 0; i < pts.size(); i++)
            {
            auto key = std::make_tuple(rdiv(std::get<0>(pts[i]), RS), rdiv(std::get<1>(pts[i]), RS), rdiv(std::get<2>(pts[i]), BH));
            auto& s = reg[key];
            if (s.empty()) s.assign(no, 0);
            for (int o = 0; o < no; o++) s[o] |= ps[i][o];
            std::vector<char>().swap(ps[i]);
            }
        // each region with its neighbours' (the eight round it at its band, and the bands above and below there)
        std::vector<std::pair<std::tuple<int, int, int>, std::vector<char>>> outr;
        long tot = 0;
        for (auto& e : reg)
            {
            int sx, sy, b; std::tie(sx, sy, b) = e.first;
            std::vector<char> u = e.second;
            for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) for (int db = -1; db <= 1; db++)
                {
                if (!dx && !dy && !db) continue;
                if (db && (dx || dy)) continue;
                auto it = reg.find(std::make_tuple(sx + dx, sy + dy, b + db));
                if (it == reg.end()) continue;
                for (int i = 0; i < no; i++) u[i] |= it->second[i];
                }
            int cnt = 0; for (int i = 0; i < no; i++) cnt += u[i];
            tot += cnt;
            outr.push_back({ e.first, u });
            }
        printf("cell %d: %d regions, %.0f of %d pieces seen on average\n", cellid, (int)outr.size(), outr.empty() ? 0.0 : (double)tot / outr.size(), no);
        // (std::map's order: x, then y, then band - as GameKitPvs looks)
        auto putb = [&](int v) { for (int i = 0; i < 4; i++) body.push_back((v >> (8 * i)) & 255); };
        auto puts_ = [&](int v) { body.push_back(v & 255); body.push_back((v >> 8) & 255); };
        putb(cellid); putb(no); putb((int)outr.size()); putb(RS); putb(BH);
        for (auto& e : outr) { puts_(std::get<0>(e.first)); puts_(std::get<1>(e.first)); puts_(std::get<2>(e.first)); puts_(0); }
        int by = (no + 7) >> 3;
        for (auto& e : outr)
            for (int j = 0; j < by; j++)
                {
                int v = 0;
                for (int b = 0; b < 8; b++) if (j * 8 + b < no && e.second[j * 8 + b]) v |= 1 << b;
                body.push_back(v);
                }
        while (body.size() & 3) body.push_back(0);
        ncells++;
        }
    // onto the file: its old one (if any) off first
    FILE* f = fopen(argv[2], "rb");
    if (!f) { printf("no %s\n", argv[2]); return 1; }
    std::vector<unsigned char> file;
    { unsigned char buf[65536]; size_t k; while ((k = fread(buf, 1, sizeof buf, f)) > 0) file.insert(file.end(), buf, buf + k); }
    fclose(f);
    size_t L = file.size();
    if (L >= 8 && !memcmp(&file[L - 4], "PVS1", 4))
        {
        int q = file[L - 8] | (file[L - 7] << 8) | (file[L - 6] << 16) | (file[L - 5] << 24);
        // (v0.4.68) CELL=n: only that cell worked out - the others' records kept as they were
        if (getenv("CELL") && q + 8 <= (int)L && !memcmp(&file[q], "HPVS", 4))
            {
            auto rd = [&](int o) { return file[o] | (file[o + 1] << 8) | (file[o + 2] << 16) | (file[o + 3] << 24); };
            int nc = rd(q + 4), o = q + 8, want = atoi(getenv("CELL"));
            for (int c = 0; c < nc && o + 20 <= (int)L; c++)
                {
                int cid = rd(o), no = rd(o + 4), nr = rd(o + 8);
                int len = 20 + nr * 8 + nr * ((no + 7) >> 3);
                len = (len + 3) & ~3;
                if (cid != want) { body.insert(body.end(), file.begin() + o, file.begin() + o + len); ncells++; }
                o += len;
                }
            }
        file.resize(q);
        }
    while (file.size() & 3) file.push_back(0);
    int at = (int)file.size();
    const char* h = "HPVS"; file.insert(file.end(), h, h + 4);
    for (int i = 0; i < 4; i++) file.push_back((ncells >> (8 * i)) & 255);
    file.insert(file.end(), body.begin(), body.end());
    for (int i = 0; i < 4; i++) file.push_back((at >> (8 * i)) & 255);
    const char* t = "PVS1"; file.insert(file.end(), t, t + 4);
    f = fopen(argv[2], "wb"); fwrite(&file[0], 1, file.size(), f); fclose(f);
    printf("%s: %d bytes of what can be seen (%d cells), now %d bytes\n", argv[2], (int)(file.size() - at), ncells, (int)file.size());
    return 0;
    }
