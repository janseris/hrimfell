#include <vector>
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
// Bleak Falls Barrow end to end: the path up, the doors, the passages, the puzzles, the traps, the quests
static Game* g;
static int CI(int c) { return c == CELL_BFTEMPLE ? 62 : (c == CELL_BFCRYPT ? 60 : 106); }
static int CJ(int c) { return c == CELL_BFTEMPLE ? 25 : (c == CELL_BFCRYPT ? 50 : 30); }
static int WX(double c) { return 100 * METER + (int)((c * 2 - CI(g->iS.cell)) * 75) * METER / 100; }
static int WZ(double r) { return 100 * METER + (int)((CJ(g->iS.cell) - r * 2) * 75) * METER / 100; }
static int fails = 0;
// walk through points (map squares in the current cell, or metres outside)
int GameBarrowDoorYaw(); int GameTowerX(); int GameTowerZ();
// breadth-first over a 1 m grid round the two points: a step is allowed where the game's walking allows it
// (up no more than 1.1 m a metre, down no more than 2 m, nothing in the way); returns waypoints in metres
int GameKitOutFloor(const Game& g, int aX, int aZ, int aFeetY, int aOff, int* aFound);
static int route(int x0, int z0, int x1, int z1, double* out, int maxPts)
    {
    // (the feet carried from square to square as the game carries them: up onto the barrow's platforms and flights,
    // which stand over the land; the higher way kept where there are two)
    const int S = METER / 2;
    int ox = x0 - ((x0 - (x0 < x1 ? x0 : x1)) / S + 80) * S, oz = z0 - ((z0 - (z0 < z1 ? z0 : z1)) / S + 80) * S;   // (the start on the grid)
    int W = (abs(x1 - x0) / S) + 161, H = (abs(z1 - z0) / S) + 161;
    int off = g->iR->iLandLoaded ? g->iR->iLandOffset : 0;
    // states: a square at a height (one square can be on the land and on a platform over it)
    struct St { int c, feet, parent; };
    std::vector<St> st; std::vector<std::vector<int> > at(W * H);
    auto idx = [&](int x, int z) { return ((z - oz + S / 2) / S) * W + (x - ox + S / 2) / S; };
    int s0 = idx(x0, z0), s1 = idx(x1, z1);
    St a0 = { s0, g->iS.py - 410, -1 }; st.push_back(a0); at[s0].push_back(0);
    for (size_t h = 0; h < st.size() && st.size() < 400000; h++)
        {
        int c = st[h].c, ci = c % W, cj = c / W;
        for (int d = 0; d < 8; d++)
            {
            static const int D[16] = { 1,0, -1,0, 0,1, 0,-1, 1,1, 1,-1, -1,1, -1,-1 };
            int ni = ci + D[d * 2], nj = cj + D[d * 2 + 1];
            if (ni < 0 || nj < 0 || ni >= W || nj >= H) continue;
            int n = nj * W + ni;
            int x = ox + ci * S, z = oz + cj * S, ff = st[h].feet, ok = 1;
            for (int q = 1; q <= 4 && ok; q++)
                {
                int px = x + D[d * 2] * S * q / 4, pz = z + D[d * 2 + 1] * S * q / 4;
                int nf = g->FloorAt(0, px, pz, ff), run = S / 4 * (d >= 4 ? 141 : 100) / 100;
                int climb = nf - ff < run * 6 / 5 || nf < ff;
                if (!climb && nf - ff < 45 * METER / 100) { int found = 0; GameKitOutFloor(*g, px, pz, ff, off, &found); if (found) climb = 1; }
                if (!climb || nf < ff - 2 * METER || g->BlockedAt(0, px, pz, 100, ff + 7 * METER / 25, 7 * METER / 4 - 7 * METER / 25)) { ok = 0; break; }
                ff = nf;
                }
            if (!ok) continue;
            int seen = 0;
            for (size_t k = 0; k < at[n].size() && !seen; k++) if (abs(st[at[n][k]].feet - ff) < METER) seen = 1;
            if (seen) continue;
            St a = { n, ff, (int)h }; at[n].push_back((int)st.size()); st.push_back(a);
            }
        }
    // at the end: the highest way there (the doors stand on the terrace)
    int best = -1;
    for (size_t k = 0; k < at[s1].size(); k++) if (best < 0 || st[at[s1][k]].feet > st[best].feet) best = at[s1][k];
    if (best < 0) { printf("  route: no way found\n"); return 0; }
    std::vector<int> path; for (int k = best; k >= 0; k = st[k].parent) path.push_back(st[k].c);
    int n = 0;
    for (int k = (int)path.size() - 1; k >= 0 && n < maxPts; k--)
        { out[n * 2] = (ox + (path[k] % W) * S) / (double)METER; out[n * 2 + 1] = (oz + (path[k] / W) * S) / (double)METER; n++; }
    return n;
    }

static int walk(const char* what, const double* p, int n, int maxTicks)
    {
    int k = 0, t, stuck = 0, lastX = g->iS.px, lastZ = g->iS.pz;
    for (t = 0; t < maxTicks && k < n; t++)
        {
        int tx = g->iS.cell ? WX(p[k * 2]) : (int)(p[k * 2] * METER), tz = g->iS.cell ? WZ(p[k * 2 + 1]) : (int)(p[k * 2 + 1] * METER);
        int dx = tx - g->iS.px, dz = tz - g->iS.pz;
        if ((dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4) < (METER / 3 >> 4) * (METER / 3 >> 4)) { k++; continue; }
        g->iS.yaw = r3d_atan2(dx, dz);
        g->iUi = UI_PLAY;
        g->Tick(16000, KEY_FWD);
        if (g->iS.hp < (40 << 8)) g->iS.hp = 100 << 8;
        if ((t & 63) == 63)
            {
            if (abs(g->iS.px - lastX) + abs(g->iS.pz - lastZ) < METER / 8) stuck++; else stuck = 0;
            lastX = g->iS.px; lastZ = g->iS.pz;
            if (getenv("WDBG")) printf("    t%d k%d (%.2f,%.2f) feet %d ui %d hp %d\n", t, k, ((g->iS.px - 100 * METER) * 100.0 / METER / 75 + CI(g->iS.cell)) / 2, (CJ(g->iS.cell) - (g->iS.pz - 100 * METER) * 100.0 / METER / 75) / 2, (g->iS.py - 410 - 100 * METER) * 100 / METER, g->iUi, g->iS.hp >> 8);
            if (getenv("ADBG")) for (int q = 0; q < g->iS.ne; q++) { Ent& e = g->iS.e[q]; if (e.type == E_BANDIT && e.pad1 == 1) printf("      arvel x %d z %d alive %d hp %d cool %d state %d vx %d cell %d\n", e.x, e.z, e.alive, e.hp, e.cool, e.state, e.vx, e.cell); }
            if (getenv("WDBG")) for (int q = 0; q < g->iS.ne; q++) { Ent& e = g->iS.e[q]; if (e.cell == g->iS.cell && abs(e.x - g->iS.px) < 3 * METER && abs(e.z - g->iS.pz) < 3 * METER) printf("      ent %d type %d hp %d dx %d dz %d\n", q, e.type, e.hp, (e.x - g->iS.px) * 100 / METER, (e.z - g->iS.pz) * 100 / METER); }
            if (stuck > 3 && getenv("WDBG"))
                {
                int fe = g->iS.py - 410, tx2 = g->iS.cell ? WX(p[k * 2]) : (int)(p[k * 2] * METER), tz2 = g->iS.cell ? WZ(p[k * 2 + 1]) : (int)(p[k * 2 + 1] * METER);
                printf("    stuck: feet %.2f, floor here %.2f, target floor %.2f, blocked at target %d (feet+28cm), props %d\n", fe / 256.0, g->FloorAt(g->iS.cell, g->iS.px, g->iS.pz, fe) / 256.0,
                       g->FloorAt(g->iS.cell, tx2, tz2, fe) / 256.0, g->BlockedAt(g->iS.cell, tx2, tz2, 100, fe + 7 * METER / 25, 7 * METER / 4 - 7 * METER / 25), g->PropBlocked(g->iS.cell, tx2, tz2, 100, fe + 7 * METER / 25));
                for (int q = 0; q < g->iS.nprop; q++) { const Prop& pp = g->iS.prop[q]; if (pp.cell == g->iS.cell && abs(pp.x - g->iS.px) < 3 * METER && abs(pp.z - g->iS.pz) < 3 * METER) printf("      prop %d type %d dx %d dz %d yaw %d\n", q, pp.type, (pp.x - g->iS.px) * 100 / METER, (pp.z - g->iS.pz) * 100 / METER, pp.yaw); }
                if (getenv("MAPDBG")) for (int j = -16; j <= 16; j++) { printf("      "); for (int i = -16; i <= 16; i++) { int qx = g->iS.px + i * METER / 4, qz = g->iS.pz - j * METER / 4; int pb = g->PropBlocked(g->iS.cell, qx, qz, 100, fe + 7 * METER / 25), ab = g->BlockedAt(g->iS.cell, qx, qz, 100, fe + 7 * METER / 25, 7 * METER / 4 - 7 * METER / 25); int fl = (g->FloorAt(g->iS.cell, qx, qz, fe) - fe) * 100 / METER; putchar(i == 0 && j == 0 ? '@' : pb ? 'C' : ab ? '#' : fl < -150 ? 'V' : fl < -30 ? 'v' : fl > 30 ? '^' : '.'); } putchar(10); }
                for (int q = 1; q <= 12; q++) { int qx = g->iS.px + (tx2 - g->iS.px) * q / 12, qz = g->iS.pz + (tz2 - g->iS.pz) * q / 12;
                    printf("      %d: floor %.3f blocked %d (dun %d statics %d)\n", q, g->FloorAt(g->iS.cell, qx, qz, fe) / 256.0, g->BlockedAt(g->iS.cell, qx, qz, 100, fe + 7 * METER / 25, 7 * METER / 4 - 7 * METER / 25), g->DunBlocked(g->iS.cell, qx, qz, 100, fe + 7 * METER / 25), (g->iR->iCell = g->iS.cell, g->iR->BlockedIn(qx, qz, 100, fe + 7 * METER / 25))); }
                }
            if (stuck > 3) break;
            }
        }
    int ok = k >= n;
    if (!ok) fails++;
    if (g->iS.cell)
        printf("%-52s %d/%d at (%.1f, %.1f) sq, floor %+d cm %s\n", what, k, n, ((g->iS.px - 100 * METER) * 100.0 / METER / 75 + CI(g->iS.cell)) / 2,
               (CJ(g->iS.cell) - (g->iS.pz - 100 * METER) * 100.0 / METER / 75) / 2, (g->iS.py - 410 - 100 * METER) * 100 / METER, ok ? "OK" : "STUCK");
    else printf("%-52s %d/%d at (%.1f, %.1f) m, height %.1f m %s\n", what, k, n, g->iS.px / (double)METER, g->iS.pz / (double)METER, (g->iS.py - 410) / (double)METER, ok ? "OK" : "STUCK");
    return ok;
    }
static void use(const char* what)
    {
    g->iUi = UI_PLAY;
    g->Tick(16000, 0);
    g->FindFocus(); g->FindPropFocus(); g->FindDoor();
    printf("  %s: focus kind %d idx %d door %d\n", what, g->iFocusKind, g->iFocus, g->iDoorFocus);
    g->KeyChar('e');
    for (int k = 0; k < 4; k++) g->Tick(16000, 0);
    }
static void face(double c, double r) { g->iS.yaw = r3d_atan2(WX(c) - g->iS.px, WZ(r) - g->iS.pz); g->iS.pitch = 0; }
static void ticks(int n) { for (int k = 0; k < n; k++) { g->iUi = g->iUi == UI_DIALOG ? UI_DIALOG : g->iUi; g->Tick(16000, 0); } }
static void talkThrough() { for (int k = 0; k < 12 && g->iUi == UI_DIALOG; k++) { g->KeyChar('e'); g->Tick(16000, 0); } }
int main(){ int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame(); g->iUi=UI_PLAY;
 // the quest from Farengar
 for (int i = 0; i < g->iS.ne; i++) if (g->iS.e[i].role == ROLE_NAMED && g->iS.e[i].pad1 == 8) { g->EnterCell(g->iS.e[i].cell, g->iS.e[i].x - 2 * METER, g->iS.e[i].z, 256); g->iS.yaw = r3d_atan2(g->iS.e[i].x - g->iS.px, g->iS.e[i].z - g->iS.pz); }
 g->Tick(16000, 0); g->FindFocus(); g->Activate(); printf("Farengar: ui %d dlg %d\n", g->iUi, g->iDlgId); talkThrough();
 printf("quest stage %d: %s\n", g->iS.bfb[BF_QBLEAK], g->QuestText());
 // keep the barrow's creatures out of the way while walking (Arvel and the spider stay)
 for (int i = 0; i < g->iS.ne; i++) { Ent& e = g->iS.e[i]; if (e.type >= E_WOLF && e.type < E_DRAGON && !(e.type == E_BANDIT && e.pad1 == 1) && e.type != E_SPIDER && !(e.type == E_WIGHT && e.pad1 == 2)) e.cell = 99; }
 // outside: from the road below the tower up to the doors
 // a way from below the tower up to the doors, found over the land as the game lets you walk it (1 m steps)
 // (from the foot of the flights below the barrow, Skyrim's (6000, -44000), on the land)
 int sx = (int)((6000LL + 233472) * 36576 / 10000), sz = (int)((-44000LL + 176128) * 36576 / 10000);
 g->EnterCell(0, sx, sz, 0); g->iFadeUs = 0;
 static double KUp[1200 * 2]; int nu = route(sx, sz, GameBarrowOutX(0, 1), GameBarrowOutZ(0, 1), KUp, 1199);
 printf("route from the tower to the doors: %d points\n", nu);
 if (getenv("RDBG")) for (int k = 0; k < nu; k++) printf("  wp %d (%.1f, %.1f) floor %.2f\n", k, KUp[k*2], KUp[k*2+1], g->FloorAt(0,(int)(KUp[k*2]*METER),(int)(KUp[k*2+1]*METER),1<<29)/256.0);
 g->EnterCell(0, sx, sz, 0); g->iFadeUs = 0;
 walk("from below the flights up the platforms to the doors", KUp, nu, 30000);
 g->iS.yaw = (GameBarrowDoorYaw() + 512) & 1023;
 { const DoorLink& dl = g->Door(7); printf("  feet %d, the door's floor %d, %d cm from it\n", g->iS.py - 410, g->DoorOutY(dl), (int)(sqrt((double)(dl.ox - g->iS.px) * (dl.ox - g->iS.px) + (double)(dl.oz - g->iS.pz) * (dl.oz - g->iS.pz)) * 100 / METER)); }
 use("the barrow's doors");
 printf("in cell %d\n", g->iS.cell);
 // the entry hall to the pillar room
 static const double KIn[] = { 55, 17,  52.5, 15.3,  48.5, 15.6,  45, 17,  41, 17,  36, 17,  31.5, 17,  31.5, 21,  26, 21,  24, 21,  24, 18.5,  20, 17,  12, 17,  9, 17,  9, 14,  9, 11 };
 walk("the hall, the passages, up into the pillar room", KIn, 16, 6000);
 printf("darts fired: %d, the lever-man %d\n", g->iS.bfb[BF_LEVERMAN], g->iS.bfb[BF_DARTS]);
 // the pillars: snake, snake, whale from the left (south to north: 2, 1, 0)
 static const double KP[] = { 5, 12 }; walk("to the south pillar", KP, 1, 600); face(3.85, 12); use("pillar 2"); ticks(45);
 for (int k = 0; k < 5 && g->iS.bfb[BF_PILLAR2] != 0; k++) { use("pillar 2 again"); ticks(45); }
 static const double KP1[] = { 5, 10.5 }; walk("to the middle pillar", KP1, 1, 600); face(3.85, 10.5);
 for (int k = 0; k < 5 && g->iS.bfb[BF_PILLAR1] != 0; k++) { use("pillar 1"); ticks(45); }
 static const double KP0[] = { 5, 9 }; walk("to the north pillar", KP0, 1, 600); face(3.85, 9);
 for (int k = 0; k < 5 && g->iS.bfb[BF_PILLAR0] != 2; k++) { use("pillar 0"); ticks(45); }
 printf("pillars %d %d %d\n", g->iS.bfb[BF_PILLAR0], g->iS.bfb[BF_PILLAR1], g->iS.bfb[BF_PILLAR2]);
 static const double KL[] = { 7, 9.6 }; walk("to the lever", KL, 1, 600); face(7, 8.5); g->iS.pitch = -60; use("lever");
 ticks(200); printf("gate 0 %d lift %d\n", g->iS.bfb[BF_GATE0], g->iBfTimer[8]);
 static const double KB[] = { 10.3, 12,  10.3, 10.5,  10.3, 8,  10, 6 }; walk("up to the balcony", KB, 4, 1200);
 static const double KG[] = { 10.3, 8,  10.3, 11,  7, 9,  7, 6.5,  6, 3.6,  5.2, 3 }; walk("down, through the gate, to the spiral stairs", KG, 6, 2000);
 face(3.8, 2.8); use("the stairs down");
 printf("in cell %d\n", g->iS.cell);
 // below: the skeever cave, the spider, Arvel
 static const double KC[] = { 22, 45.5,  30, 45.3,  38, 45.3,  43, 44.3,  43, 41,  40, 38 };
 walk("the skeever cave into the spider's lair", KC, 6, 4000);
 ticks(120);
 int sp = -1, ar = -1; for (int i = 0; i < g->iS.ne; i++) { if (g->iS.e[i].type == E_SPIDER) sp = i; if (g->iS.e[i].type == E_BANDIT && g->iS.e[i].pad1 == 1) ar = i; }
 printf("spider dropped %d, fly %d\n", g->iS.bfb[BF_SPIDER], sp >= 0 ? g->iS.e[sp].fly : -1);
 if (sp >= 0) g->HurtEnt(g->iS.e[sp], 999, 0, g->iS.px, g->iS.pz);
 static const double KA[] = { 33.5, 38.0 }; walk("to Arvel", KA, 1, 1000); face(31.8, 37.6);
 use("Arvel"); printf("dialog %d\n", g->iDlgId); talkThrough();
 printf("Arvel freed %d\n", g->iS.bfb[BF_ARVEL]);
 ticks(400);
 printf("Arvel: alive %d at (%d,%d) path %d, state %d\n", g->iS.e[ar].alive, g->iS.e[ar].x, g->iS.e[ar].z, g->iS.e[ar].cool, g->iS.bfb[BF_ARVEL]);
 static const double KR[] = { 30, 37.5,  25, 36.5,  21, 36.2,  17, 35.6,  15, 35.2 }; walk("after him", KR, 5, 3000);
 g->EnterCell(g->iS.cell, g->iS.e[ar].x - METER, g->iS.e[ar].z, 256); if (getenv("WDBG")) printf("  after enter px %d pz %d\n", g->iS.px, g->iS.pz); g->iS.yaw = 256; g->Tick(16000, 0); g->FindFocus(); g->Activate(); if (getenv("WDBG")) printf("  after activate px %d pz %d ui %d\n", g->iS.px, g->iS.pz, g->iUi);
 printf("claw %d journal %d qclaw %d\n", g->CountItem(IT_GOLDEN_CLAW), g->CountItem(IT_ARVEL_JOURNAL), g->iS.bfb[BF_QCLAW]);
 static const double K5[] = { 8, 33.6,  7, 30,  7, 25.3,  4, 25.3,  4, 21.5,  8, 20.8,  12, 20.7,  15.5, 20.7,  19.8, 20.8,  20.3, 19,  17, 19 };
 walk("the burial passage, up the stairs, into the blades", K5, 11, 4000);
 printf("hp %d (blades hit?)\n", g->iS.hp >> 8);
 static const double K6[] = { 10, 19,  10, 17.4 }; walk("past the blades to the chain", K6, 2, 2000); face(9.6, 17.0); g->iS.pitch = -60; use("chain 0");
 printf("blades 0 stopped %d\n", g->iS.bfb[BF_BLADES0]);
 static const double K7[] = { 10, 17,  13, 17,  13, 15,  14, 13,  17, 14.5,  25, 14.5,  31, 14,  34, 12,  34, 13.8 }; walk("the room, the cave, the waterfall cavern", K7, 9, 4000);
 face(33.25, 13.8); g->iS.pitch = -60; use("chain 1");
 ticks(200); printf("gate 1 %d lift %d\n", g->iS.bfb[BF_GATE1], g->iBfTimer[9]);
 static const double K8[] = { 35.8, 15.4,  35.8, 18,  35.8, 21,  35.5, 26,  40, 26,  50, 26.3,  54.3, 24.5,  54.3, 19,  54, 17.2,  50, 17.2,  50, 13,  49.2, 3 };
 walk("down to the stream, round to the sanctum's door", K8, 12, 6000);
 g->iS.yaw = 0; use("door up");
 printf("in cell %d\n", g->iS.cell);
 static const double KS[] = { 100, 6.6,  99.5, 5,  92, 4.3,  89, 3.6,  86, 3.8,  86, 9.5,  83, 11.5,  83, 13.8,  80, 14.6,  78.5, 18.2,  76, 18.2,  70, 17.9,  68.5, 17.65 };
 walk("down to the walkway, the antechamber, the blades", KS, 13, 5000);
 printf("blades 1 state %d hp %d\n", g->iS.bfb[BF_BLADES1], g->iS.hp >> 8);
 face(67.9, 16.2); g->iS.pitch = -60; use("chain 2");
 { static const double KD[] = { 67.9, 17.65 }; walk("up to the claw door", KD, 1, 400); } face(66.9, 17.65); use("claw door"); printf("ui %d (puzzle %d)\n", g->iUi, UI_PUZZLE);
 // the rings: bear, moth, owl
 for (int ring = 0; ring < 3; ring++) { while (g->iS.bfb[BF_RING0 + ring] != ring) g->iS.bfb[BF_RING0 + ring] = (g->iS.bfb[BF_RING0 + ring] + 1) % 3; }
 g->KeyChar('e'); printf("claw door open %d\n", g->iS.bfb[BF_CLAWDOOR]);
 ticks(250);
 static const double KM[] = { 64, 17.65,  52, 17.6,  45, 17.8,  42, 17.8,  38.5, 18.5,  36, 18.6,  33.5, 18.6 }; walk("the corridor, the chamber, the word wall's platform", KM, 7, 4000);
 ticks(150); printf("word %d lord %d\n", g->iS.bfb[BF_WORD], g->iS.bfb[BF_LORD]);
 int lord = -1; for (int i = 0; i < g->iS.ne; i++) if (g->iS.e[i].type == E_WIGHT && g->iS.e[i].pad1 == 2) lord = i;
 if (lord >= 0) { printf("Overlord hidden %d aggro %d\n", g->iS.e[lord].pad2 & 64, g->iS.e[lord].aggro); g->HurtEnt(g->iS.e[lord], 999, 0, g->iS.px, g->iS.pz);
   g->EnterCell(g->iS.cell, g->iS.e[lord].x + 2 * METER, g->iS.e[lord].z, 768); g->iS.yaw = 768; g->Tick(16000, 0); g->FindFocus(); g->Activate(); }
 printf("dragonstone %d stage %d\n", g->CountItem(IT_DRAGONSTONE), g->iS.bfb[BF_QBLEAK]);
 static const double KX[] = { 32.1, 20.9,  32.1, 22.1,  33.8, 22.1,  34, 22.8,  32.5, 24,  28, 24,  27.6, 20,  27.4, 18.4,  25, 18.2,  20, 17.7,  13.5, 17.7,  11, 17,  5, 16.8,  2.2, 16.8 };
 walk("down the steps, round the wall, up the way out", KX, 14, 5000);
 g->iS.yaw = 768; use("the way out");
 printf("in cell %d at (%.1f, %.1f) m\n", g->iS.cell, g->iS.px / (double)METER, g->iS.pz / (double)METER);
 // back to Farengar with the stone
 int gold0 = g->iS.gold;
 for (int i = 0; i < g->iS.ne; i++) if (g->iS.e[i].role == ROLE_NAMED && g->iS.e[i].pad1 == 8) { g->EnterCell(g->iS.e[i].cell, g->iS.e[i].x - 2 * METER, g->iS.e[i].z, 256); g->iS.yaw = r3d_atan2(g->iS.e[i].x - g->iS.px, g->iS.e[i].z - g->iS.pz); }
 g->iUi = UI_PLAY; g->Tick(16000, 0); g->FindFocus(); g->Activate(); printf("Farengar again: ui %d dlg %d\n", g->iUi, g->iDlgId); talkThrough();
 printf("quest stage %d, dragonstone %d, gold +%d, word %d\n", g->iS.bfb[BF_QBLEAK], g->CountItem(IT_DRAGONSTONE), g->iS.gold - gold0, g->iS.bfb[BF_WORD]);
 if (g->iS.bfb[BF_QBLEAK] != 190) fails++;
 printf("FAILS %d\n", fails);
}
