// game_meshes.cpp - pre-baked low-poly models for Hrimfell (all animation frames built at start-up)
#include "game.h"

static const int M = METER;

// humanoid proportions (world units, 1 m = 256)
struct HSpec { short legLen, legW, torsoH, torsoW, torsoD, armLen, armW, headS, hunch, weapon, robe; };
static const HSpec KHSpec[5] =
    {
    // legLen legW torsoH torsoW torsoD armLen armW headS hunch weapon robe
    { 230, 19, 160, 52, 30, 165, 16, 30,  0, 0, 0 },     // villager
    { 230, 19, 165, 54, 32, 160, 16, 30, 24, 3, 1 },     // elder (robe, staff)
    { 240, 15, 165, 46, 26, 178, 13, 28, 44, 1, 0 },     // barrow wight (sword)
    { 300, 42, 250, 104, 70, 330, 36, 50, 96, 2, 0 },    // troll (club)
    { 225, 18, 155, 48, 30, 160, 15, 29,  0, 0, 1 }      // woman (dress)
    };

enum { HF_WALK = 0, HF_ATTACK = 16, HF_DEAD = 24, HF_COUNT = 25 };

void Game::RotateFrame(Mesh& m, int aSrc, int aDst, int aAngle, int aLift)
    {
    int c = r3d_cos(aAngle), s = r3d_sin(aAngle);
    const short* a = m.v + aSrc * m.vstride * 3;
    short* b = m.v + aDst * m.vstride * 3;
    int i;
    for (i = 0; i < m.nv; i++)
        {
        int x = a[i * 3], y = a[i * 3 + 1], z = a[i * 3 + 2];
        b[i * 3] = (short)x;
        b[i * 3 + 1] = (short)(((y * c - z * s) >> 14) + aLift);
        b[i * 3 + 2] = (short)((y * s + z * c) >> 14);
        }
    }


// wolf: slots 0 fur, 1 dark fur, 2 eyes/teeth
void Game::BuildWolfLod(Mesh& m)
    {
    iR->MeshInit(m, 96, 140, 17);
    int fr;
    for (fr = 0; fr < 17; fr++)
        {
        iR->iMeshCursor = 0;
        int p = fr * 64;
        int a = (r3d_sin(p) * 130) >> 14, b = (r3d_sin(p + 512) * 130) >> 14;
        int bob = (r3d_abs(r3d_sin(p)) * 14) >> 14;
        int y0 = 150 + bob;
        iR->MeshBox(m, fr, 0, 0, y0 + 74, 0, 0, 44, 32, 140);              // body
        iR->MeshBox(m, fr, 0, 0, y0 + 86, 120, 0, 50, 40, 46);             // chest/shoulders
        iR->MeshBox(m, fr, 0, 0, y0 + 112, 178, 0, 32, 30, 36);            // head
        iR->MeshBox(m, fr, 1, 0, y0 + 88, 222, 0, 17, 15, 26);             // snout
        iR->MeshBox(m, fr, 1, -18, y0 + 136, 168, 0, 7, 12, 6);            // ears
        iR->MeshBox(m, fr, 1, 18, y0 + 136, 168, 0, 7, 12, 6);
        iR->MeshBox(m, fr, 1, -30, y0 + 20, 110, a, 11, y0 / 2 - 4, 11);  // legs
        iR->MeshBox(m, fr, 1, 30, y0 + 20, 110, b, 11, y0 / 2 - 4, 11);
        iR->MeshBox(m, fr, 1, -30, y0 + 20, -110, b, 11, y0 / 2 - 4, 11);
        iR->MeshBox(m, fr, 1, 30, y0 + 20, -110, a, 11, y0 / 2 - 4, 11);
        iR->MeshBox(m, fr, 0, 0, y0 + 80, -136, 300 + a / 3, 12, 56, 12); // tail
        }
    // dead: lying on its side
    RotateFrame(m, 0, 16, 0, -100);
    iR->MeshFinish(m);
    }

// dragon: slots 0 scales, 1 belly/horns, 2 wing membrane, 3 dark (claws, mouth)
void Game::BuildDragonLod(Mesh& m)
    {
    iR->MeshInit(m, 160, 200, 25);
    int fr;
    for (fr = 0; fr < 25; fr++)
        {
        iR->iMeshCursor = 0;
        int flying = fr < 16;
        int p = (fr & 15) * 64;
        int flap = flying ? ((r3d_sin(p) * 150) >> 14) : 210;     // wing angle (up = positive)
        int span = flying ? 256 : 110;                            // wing reach %
        int legA = flying ? 220 : ((r3d_sin(p * 2) * 90) >> 14);
        int legB = flying ? 220 : -legA;
        int sway = flying ? ((r3d_sin(p + 256) * 60) >> 14) : ((r3d_sin(p * 2) * 30) >> 14);
        int by = 3 * M;                                            // body top height
        iR->MeshBox(m, fr, 0, 0, by, 0, 0, 230, 200, 700);        // body
        iR->MeshBox(m, fr, 1, 0, by - 150, 100, 0, 180, 70, 520);  // belly
        // neck: two segments up/forward, then head forward
        int ny = by - 60, nz = 600;
        int s1 = 640 - sway / 2;
        iR->MeshBox(m, fr, 0, 0, ny, nz, s1, 120, 230, 120);
        ny += (-460 * r3d_cos(s1)) >> 14; nz += (-460 * r3d_sin(s1)) >> 14;
        int s2 = 600 + sway / 2;
        iR->MeshBox(m, fr, 0, 0, ny, nz, s2, 100, 200, 100);
        ny += (-400 * r3d_cos(s2)) >> 14; nz += (-400 * r3d_sin(s2)) >> 14;
        int hs = 768 + 40;
        iR->MeshBox(m, fr, 0, 0, ny + 60, nz - 40, hs, 110, 190, 90);   // head (skull)
        iR->MeshBox(m, fr, 3, 0, ny - 40, nz - 20, hs + 40, 80, 170, 40); // jaw
        // horns
        iR->MeshConeAt(m, fr, 1, -70, nz - 60 - 40, 30, ny + 130, ny + 330, 4, 0);
        iR->MeshConeAt(m, fr, 1, 70, nz - 60 - 40, 30, ny + 130, ny + 330, 4, 0);
        // tail: four segments backwards with sway
        int ty = by - 60, tz = -680;
        int k;
        for (k = 0; k < 4; k++)
            {
            int ts = 280 + k * 12 + (flying ? sway * (k + 1) / 3 : 0);
            int len = 520 - k * 70;
            int w = 150 - k * 30;
            iR->MeshBox(m, fr, 0, 0, ty, tz, ts, w, len / 2, w);
            ty += (-len * r3d_cos(ts)) >> 14; tz += (-len * r3d_sin(ts)) >> 14;
            }
        // legs
        iR->MeshBox(m, fr, 3, -220, by - 280, 440, legA, 70, 190, 80);
        iR->MeshBox(m, fr, 3, 220, by - 280, 440, legB, 70, 190, 80);
        iR->MeshBox(m, fr, 3, -240, by - 280, -440, legB, 80, 190, 90);
        iR->MeshBox(m, fr, 3, 240, by - 280, -440, legA, 80, 190, 90);
        // wings: inner + outer panel, both sides visible
        int side;
        for (side = -1; side <= 1; side += 2)
            {
            int rootY = by - 40;
            int c1 = r3d_cos(flap), s1w = r3d_sin(flap);
            int ex = side * (230 + ((((900 * span) >> 8) * c1) >> 14));
            int ey = rootY + ((((900 * span) >> 8) * s1w) >> 14);
            int f2 = flap + (flying ? flap / 2 : 40);
            int tx = ex + side * ((((1100 * span) >> 8) * r3d_cos(f2)) >> 14);
            int tyy = ey + ((((1100 * span) >> 8) * r3d_sin(f2)) >> 14);
            int inner[12] = { side * 230, rootY, 380,  ex, ey, 200,  ex, ey - 30, -500,  side * 230, rootY, -380 };
            int outer[12] = { ex, ey, 200,  tx, tyy, -300,  tx - side * 200, tyy - 40, -760,  ex, ey - 30, -500 };
            iR->MeshQuad(m, fr, 2, inner, 0, rootY - 600, 0);
            iR->MeshQuad(m, fr, 2, inner, 0, rootY + 600, 0);
            iR->MeshQuad(m, fr, 2, outer, 0, rootY - 600, 0);
            iR->MeshQuad(m, fr, 2, outer, 0, rootY + 600, 0);
            }
        }
    // dead: collapsed ground pose, lowered
    RotateFrame(m, 16, 24, 30, -220);
    iR->MeshFinish(m);
    }



// items use slot 0 = main material, 1 = secondary, 2 = accent
void Game::BuildItems()
    {
    int i;
    for (i = 0; i < IM_COUNT; i++)
        {
        Mesh& m = iMItem[i];
        iR->MeshInit(m, 96, 128, 1);
        switch (i)
            {
            case IM_SWORD:
            case IM_SWORD2:
                {
                int L = i == IM_SWORD ? 230 : 260;
                iR->MeshBox(m, 0, 0, 0, 60 + L, 0, 0, 16, L / 2, 4);       // blade
                iR->MeshCone(m, 0, 0, 16, 60 + L, 60 + L + 30, 4, 128);   // tip
                iR->MeshBox(m, 0, 2, 0, 66, 0, 0, 52, 6, 10);             // guard
                iR->MeshBox(m, 0, 1, 0, 60, 0, 0, 9, 30, 9);              // grip
                iR->MeshBox(m, 0, 2, 0, 0, 0, 0, 13, 7, 13);              // pommel
                break;
                }
            case IM_AXE:
                iR->MeshBox(m, 0, 1, 0, 300, 0, 0, 10, 150, 10);          // haft
                iR->MeshBox(m, 0, 0, 34, 300, 0, 0, 34, 34, 5);           // head
                iR->MeshBox(m, 0, 0, 70, 310, 0, 0, 8, 46, 6);            // edge
                break;
            case IM_MACE:
                // a flanged head on a haft (a warhammer: the same, heavier)
                iR->MeshBox(m, 0, 1, 0, 200, 0, 0, 9, 100, 9);            // haft
                iR->MeshBox(m, 0, 0, 0, 300, 0, 0, 26, 40, 26);           // head
                iR->MeshBox(m, 0, 0, 0, 290, 0, 0, 38, 28, 8);            // flanges
                iR->MeshBox(m, 0, 0, 0, 290, 0, 0, 8, 28, 38);
                iR->MeshBox(m, 0, 2, 0, 20, 0, 0, 12, 10, 12);            // pommel
                break;
            case IM_BOW:
                {
                // a bent stave and its string, 1.2 m (the grip at the middle)
                int k;
                for (k = 0; k < 6; k++)
                    {
                    int a0 = 300 + k * 424 / 6, a1 = 300 + (k + 1) * 424 / 6;          // the arc's angles
                    iR->MeshLimb(m, 0, 0, (r3d_cos(a0) * 180) >> 14, 154 + ((r3d_sin(a0) * 180) >> 14) - 154 * 0, 0,
                                 (r3d_cos(a1) * 180) >> 14, 154 + ((r3d_sin(a1) * 180) >> 14) - 154 * 0, 0, 6, 6, 6, 6, 4, 0);
                    }
                iR->MeshLimb(m, 0, 1, (r3d_cos(300) * 180) >> 14, 154 + ((r3d_sin(300) * 180) >> 14), 0,
                             (r3d_cos(724) * 180) >> 14, 154 + ((r3d_sin(724) * 180) >> 14), 0, 1, 1, 1, 1, 3, 0);
                break;
                }
            case IM_ARROW:
                iR->MeshLimb(m, 0, 0, 0, 0, 0, 0, 200, 0, 3, 3, 3, 3, 3, 0);    // shaft (78 cm)
                iR->MeshCone(m, 0, 1, 8, 200, 222, 4, 128);                     // head
                iR->MeshBox(m, 0, 2, 0, 24, 0, 0, 10, 20, 1);                   // fletching
                iR->MeshBox(m, 0, 2, 0, 24, 0, 0, 1, 20, 10);
                break;
            case IM_INGOT:
                iR->MeshBox(m, 0, 0, 0, 14, 0, 0, 44, 14, 18);
                break;
            case IM_NOTE:
                iR->MeshBox(m, 0, 0, 0, 1, 0, 0, 115, 1, 70);                  // (v0.4.72) a sheet of paper
                break;
            case IM_DAGGER:
                iR->MeshBox(m, 0, 0, 0, 160, 0, 0, 12, 50, 3);
                iR->MeshCone(m, 0, 0, 12, 160, 180, 4, 128);
                iR->MeshBox(m, 0, 2, 0, 62, 0, 0, 34, 5, 8);
                iR->MeshBox(m, 0, 1, 0, 57, 0, 0, 8, 26, 8);
                break;
            case IM_SHIELD:
                iR->MeshConeAt(m, 0, 0, 0, 0, 120, 0, 18, 10, 0);
                iR->MeshConeAt(m, 0, 0, 0, 0, 120, 0, -6, 10, 0);
                iR->MeshConeAt(m, 0, 2, 0, 0, 30, 16, 40, 6, 0);         // boss
                break;
            case IM_HELMET:
                iR->MeshCone(m, 0, 0, 62, 0, 70, 8, 0);                    // dome
                iR->MeshCone(m, 0, 0, 64, 0, -20, 8, 0);
                iR->MeshBox(m, 0, 1, 0, 6, 0, 0, 66, 8, 66);              // band
                iR->MeshBox(m, 0, 1, 0, 0, 60, 0, 8, 40, 6);              // nose guard
                break;
            case IM_CUIRASS:
                iR->MeshBox(m, 0, 0, 0, 160, 0, 0, 70, 80, 36);
                iR->MeshBox(m, 0, 1, -70, 170, 0, 0, 30, 18, 38);
                iR->MeshBox(m, 0, 1, 70, 170, 0, 0, 30, 18, 38);
                iR->MeshBox(m, 0, 2, 0, 20, 0, 0, 72, 8, 38);             // belt
                break;
            case IM_BOOTS:
                iR->MeshBox(m, 0, 0, -36, 90, 0, 0, 22, 45, 24);
                iR->MeshBox(m, 0, 0, 36, 90, 0, 0, 22, 45, 24);
                iR->MeshBox(m, 0, 1, -36, 18, 20, 0, 22, 9, 42);
                iR->MeshBox(m, 0, 1, 36, 18, 20, 0, 22, 9, 42);
                break;
            case IM_GLOVES:
                iR->MeshBox(m, 0, 0, -40, 100, 0, 0, 24, 40, 14);
                iR->MeshBox(m, 0, 0, 40, 100, 0, 0, 24, 40, 14);
                iR->MeshBox(m, 0, 1, -40, 20, 0, 0, 20, 10, 12);
                iR->MeshBox(m, 0, 1, 40, 20, 0, 0, 20, 10, 12);
                break;
            case IM_POTION:
                iR->MeshCone(m, 0, 0, 38, 50, 100, 6, 0);                  // shoulder
                iR->MeshCone(m, 0, 0, 38, 50, -10, 6, 0);                  // bowl
                iR->MeshBox(m, 0, 1, 0, 130, 0, 0, 10, 20, 10);           // neck
                iR->MeshBox(m, 0, 2, 0, 150, 0, 0, 12, 10, 12);           // cork
                break;
            case IM_BREAD:
                {
                // a short piece of a long loaf (as Skyrim's bread): the round crust end at +x, the cut face at x = 0
                static const int KLoaf[6 * 3] = { 0, 9, 10,  5, 10, 11,  16, 10, 11,  24, 9, 10,  29, 6, 7,  32, 2, 3 };
                int R[12 * 6], k;
                for (k = 0; k < 6; k++)
                    {
                    int* q = R + k * 12;
                    q[0] = KLoaf[k * 3]; q[1] = 9; q[2] = 0;
                    q[3] = 0; q[4] = 16384; q[5] = 0; q[6] = 0; q[7] = 0; q[8] = 16384;
                    q[9] = KLoaf[k * 3 + 1]; q[10] = KLoaf[k * 3 + 2]; q[11] = 0;
                    }
                iR->MeshRings(m, 0, R, 6, 8, 2);
                iR->MeshLimb(m, 0, 1, -1, 9, 0, 0, 9, 0, 9, 10, 9, 10, 8, 3);       // the cut face (crumb)
                for (k = 0; k < 2; k++)                                              // slanted scores on the crust
                    iR->MeshLimb(m, 0, 2, 4 + k * 12, 19, -5, 12 + k * 12, 19, 5, 2, 1, 2, 1, 3, 0);
                break;
                }
            case IM_CHEESE:
                iR->MeshConeAt(m, 0, 0, 0, 0, 90, 30, 50, 8, 0);
                iR->MeshConeAt(m, 0, 0, 0, 0, 90, 30, 10, 8, 0);
                iR->MeshBox(m, 0, 1, 0, 52, 0, 0, 60, 22, 8);
                break;
            case IM_APPLE:
                // round, slightly wider than tall, a dimple and a short brown stalk with a leaf
                iR->MeshEllipsoid(m, 0, 0, 0, 38, 0, 42, 38, 42, 6, 4);
                iR->MeshLimb(m, 0, 1, 0, 70, 0, 4, 92, 2, 4, 4, 3, 3, 4, 2);
                iR->MeshEllipsoid(m, 0, 2, 12, 84, 4, 12, 3, 6, 4, 2);
                break;
            case IM_MEAT:
                // a haunch: rounded muscle tapering to the shank bone with a knuckle
                iR->MeshEllipsoid(m, 0, 0, 0, 34, -6, 44, 32, 56, 5, 3);
                iR->MeshLimb(m, 0, 0, 0, 34, 40, 0, 34, 70, 22, 20, 12, 11, 6, 0);
                iR->MeshLimb(m, 0, 1, 0, 34, 66, 0, 36, 104, 8, 8, 7, 7, 5, 0);
                iR->MeshEllipsoid(m, 0, 1, 0, 36, 108, 12, 10, 10, 4, 2);
                break;
            case IM_HERB:
                {
                int a[12] = { 0, 0, 0,  30, 60, 0,  10, 140, 0,  -20, 70, 0 };
                int b[12] = { 0, 0, 0,  0, 60, 30,  0, 130, 10,  0, 70, -25 };
                iR->MeshQuad(m, 0, 0, a, 0, 60, 60);
                iR->MeshQuad(m, 0, 0, a, 0, 60, -60);
                iR->MeshQuad(m, 0, 0, b, 60, 60, 0);
                iR->MeshQuad(m, 0, 0, b, -60, 60, 0);
                iR->MeshCone(m, 0, 1, 20, 140, 160, 5, 0);                 // flower
                break;
                }
            case IM_PELT:
                iR->MeshBox(m, 0, 0, 0, 12, 0, 0, 100, 6, 150);
                iR->MeshBox(m, 0, 1, 0, 14, 170, 0, 40, 5, 30);
                iR->MeshBox(m, 0, 1, 0, 14, -170, 0, 20, 5, 40);
                break;
            case IM_BOOK:
                iR->MeshBox(m, 0, 0, 0, 30, 0, 0, 70, 15, 95);
                iR->MeshBox(m, 0, 1, 6, 28, 0, 0, 64, 12, 90);
                break;
            case IM_KEY:
                iR->MeshBox(m, 0, 0, 0, 60, 0, 0, 6, 50, 6);
                iR->MeshBox(m, 0, 0, 0, 130, 0, 0, 22, 18, 6);
                iR->MeshBox(m, 0, 0, 14, 22, 0, 0, 12, 10, 6);
                break;
            case IM_RING:
                {
                // a band standing on its edge with a small stone set on top; real size (~2 cm across)
                int k;
                for (k = 0; k < 10; k++)
                    {
                    int a0 = k * 1024 / 10, a1 = (k + 1) * 1024 / 10;
                    iR->MeshLimb(m, 0, 0, (r3d_cos(a0) * 5) >> 14, 6 + ((r3d_sin(a0) * 5) >> 14), 0, (r3d_cos(a1) * 5) >> 14, 6 + ((r3d_sin(a1) * 5) >> 14), 0, 1, 1, 1, 1, 3, 0);
                    }
                iR->MeshEllipsoid(m, 0, 2, 0, 12, 0, 2, 2, 2, 4, 2);
                break;
                }
            case IM_SCALE:
                {
                int a[12] = { -80, 0, 0,  80, 0, 0,  50, 120, 0,  -50, 120, 0 };
                int t[9] = { -50, 120, 0,  50, 120, 0,  0, 180, 0 };
                iR->MeshQuad(m, 0, 0, a, 0, 60, 60);
                iR->MeshQuad(m, 0, 0, a, 0, 60, -60);
                iR->MeshTri(m, 0, 1, t, 0, 150, 60);
                iR->MeshTri(m, 0, 1, t, 0, 150, -60);
                break;
                }
            // ---- food and goods at real size (1 m = 256)
            case IM_CABBAGE:
                iR->MeshEllipsoid(m, 0, 0, 0, 22, 0, 26, 22, 26, 4, 3);
                iR->MeshEllipsoid(m, 0, 1, 0, 26, 0, 30, 12, 30, 5, 2);      // loose outer leaves
                break;
            case IM_LEEK:
                iR->MeshLimb(m, 0, 0, 0, 4, -40, 0, 4, 10, 5, 5, 5, 5, 5, 3);
                iR->MeshLimb(m, 0, 1, 0, 4, 10, 6, 7, 52, 6, 4, 2, 2, 4, 2);
                iR->MeshLimb(m, 0, 1, 0, 4, 10, -6, 6, 46, 6, 4, 2, 2, 4, 2);
                break;
            case IM_POTATO:
                iR->MeshEllipsoid(m, 0, 0, 0, 14, 0, 24, 14, 17, 4, 2);
                break;
            case IM_CARROT:
                iR->MeshLimb(m, 0, 0, 0, 5, -24, 0, 4, 22, 6, 6, 1, 1, 5, 1);
                iR->MeshLimb(m, 0, 1, 0, 6, -24, 0, 12, -36, 3, 3, 1, 1, 3, 2);
                break;
            case IM_GARLIC:
                iR->MeshEllipsoid(m, 0, 0, 0, 10, 0, 12, 10, 12, 4, 2);
                iR->MeshLimb(m, 0, 1, 0, 18, 0, 0, 26, 0, 3, 3, 1, 1, 3, 2);
                break;
            case IM_SALT:
                iR->MeshConeAt(m, 0, 0, 0, 0, 24, 0, 12, 7, 0);
                iR->MeshConeAt(m, 0, 1, 0, 0, 24, 0, -1, 7, 0);
                break;
            case IM_FISH:
                iR->MeshLimb(m, 0, 0, 0, 10, -50, 0, 10, 30, 6, 12, 7, 13, 6, 1);
                iR->MeshLimb(m, 0, 0, 0, 10, 30, 0, 10, 54, 7, 13, 2, 4, 5, 0);
                iR->MeshLimb(m, 0, 2, 0, 10, -50, 0, 10, -64, 6, 12, 1, 3, 4, 2);    // head
                iR->MeshLimb(m, 0, 2, 0, 10, 54, 0, 10, 66, 1, 2, 2, 14, 4, 2);     // tail
                break;
            case IM_BOWL:
                iR->MeshLimb(m, 0, 0, 0, 0, 0, 0, 16, 0, 22, 22, 36, 36, 8, 1);
                iR->MeshLimb(m, 0, 0, 0, 3, 0, 0, 16, 0, 19, 19, 33, 33, 8, 4);    // the inside, seen over the rim
                iR->MeshConeAt(m, 0, 1, 0, 0, 27, 9, 9, 8, 0);              // the stew / the bottom
                break;
            case IM_NECKLACE:
                {
                int k;
                // a fine chain laid out as an oval, with a pendant at the bottom of the loop
                for (k = 0; k < 14; k++)
                    {
                    int a0 = k * 1024 / 14, a1 = (k + 1) * 1024 / 14;
                    iR->MeshLimb(m, 0, 0, (r3d_cos(a0) * 22) >> 14, 1, (r3d_sin(a0) * 38) >> 14, (r3d_cos(a1) * 22) >> 14, 1, (r3d_sin(a1) * 38) >> 14, 1, 1, 1, 1, 3, 0);
                    }
                iR->MeshEllipsoid(m, 0, 2, 0, 3, -42, 4, 3, 5, 4, 2);
                break;
                }
            case IM_GEM:
                iR->MeshConeAt(m, 0, 0, 0, 0, 9, 6, 16, 6, 0);
                iR->MeshConeAt(m, 0, 1, 0, 0, 9, 6, 0, 6, 0);
                break;
            case IM_MUG:
                iR->MeshLimb(m, 0, 0, 0, 0, 0, 0, 40, 0, 16, 16, 15, 15, 7, 1);
                iR->MeshLimb(m, 0, 0, 0, 33, 0, 0, 40, 0, 13, 13, 13, 13, 7, 4);  // inside of the rim
                iR->MeshConeAt(m, 0, 2, 0, 0, 14, 34, 34, 7, 0);                // the ale in it
                iR->MeshLimb(m, 0, 1, 15, 32, 0, 24, 20, 0, 3, 3, 3, 3, 3, 0);
                iR->MeshLimb(m, 0, 1, 24, 20, 0, 15, 8, 0, 3, 3, 3, 3, 3, 0);
                break;
            case IM_STEAK:
                // a slab of beef with a rim of fat
                iR->MeshEllipsoid(m, 0, 1, 0, 9, 0, 46, 9, 33, 6, 2);
                iR->MeshEllipsoid(m, 0, 0, 2, 11, 0, 42, 10, 29, 6, 2);
                iR->MeshEllipsoid(m, 0, 1, -14, 13, 6, 10, 4, 8, 4, 2);
                break;
            case IM_JUG:
                iR->MeshLimb(m, 0, 0, 0, 0, 0, 0, 34, 0, 20, 20, 28, 28, 8, 1);
                iR->MeshLimb(m, 0, 0, 0, 34, 0, 0, 58, 0, 28, 28, 12, 12, 8, 0);
                iR->MeshLimb(m, 0, 0, 0, 58, 0, 0, 70, 0, 12, 12, 14, 14, 8, 2);
                iR->MeshLimb(m, 0, 1, 22, 56, 0, 30, 34, 0, 4, 4, 4, 4, 3, 0);
                iR->MeshLimb(m, 0, 1, 30, 34, 0, 24, 18, 0, 4, 4, 4, 4, 3, 0);
                break;
            case IM_PLATE:
                iR->MeshLimb(m, 0, 0, 0, 0, 0, 0, 3, 0, 30, 30, 40, 40, 10, 3);
                iR->MeshConeAt(m, 0, 1, 0, 0, 30, 4, 4, 10, 0);
                break;
            case IM_FLAGON:
                // a tall metal serving flagon: tapered body, hinged lid with a knob, handle and spout
                iR->MeshLimb(m, 0, 0, 0, 0, 0, 0, 70, 0, 26, 26, 19, 19, 8, 1);
                iR->MeshLimb(m, 0, 0, 0, 70, 0, 0, 78, 0, 20, 20, 20, 20, 8, 2);
                iR->MeshEllipsoid(m, 0, 1, 0, 84, 0, 6, 6, 6, 4, 2);
                iR->MeshLimb(m, 0, 1, 18, 72, 0, 34, 60, 0, 4, 4, 4, 4, 3, 0);
                iR->MeshLimb(m, 0, 1, 34, 60, 0, 30, 20, 0, 4, 4, 4, 4, 3, 0);
                iR->MeshLimb(m, 0, 0, -18, 66, 0, -30, 74, 0, 5, 5, 3, 3, 4, 2);
                iR->MeshLimb(m, 0, 1, 0, 10, 0, 0, 14, 0, 27, 27, 27, 27, 8, 0);   // bands
                iR->MeshLimb(m, 0, 1, 0, 48, 0, 0, 52, 0, 23, 23, 23, 23, 8, 0);
                break;
            case IM_CLAYPOT:
                iR->MeshEllipsoid(m, 0, 0, 0, 24, 0, 28, 24, 28, 5, 3);
                iR->MeshLimb(m, 0, 0, 0, 44, 0, 0, 56, 0, 14, 14, 16, 16, 7, 2);
                iR->MeshLimb(m, 0, 1, 0, 30, 0, 0, 34, 0, 29, 29, 29, 29, 7, 0);
                break;
            case IM_ROAST:
                // a small animal on the spit: body along x, haunches, head end
                iR->MeshEllipsoid(m, 0, 0, 0, 16, 0, 34, 15, 16, 5, 3);
                iR->MeshEllipsoid(m, 0, 0, -26, 12, 8, 12, 10, 8, 4, 2);
                iR->MeshEllipsoid(m, 0, 0, -26, 12, -8, 12, 10, 8, 4, 2);
                iR->MeshEllipsoid(m, 0, 1, 34, 18, 0, 10, 8, 8, 4, 2);
                break;
            case IM_TORCH:
                // a wooden shaft with a pitch-soaked rag head (real size, ~50 cm)
                iR->MeshLimb(m, 0, 0, 0, 0, 0, 0, 118, 0, 7, 7, 9, 9, 6, 3);
                iR->MeshEllipsoid(m, 0, 1, 0, 124, 0, 12, 18, 12, 4, 2);
                break;
            case IM_MUSHROOM:
                {
                // three mushrooms grown together: stems and caps (real size)
                static const int KM[3 * 3] = { 0, 0, 30,  14, -8, 22,  -12, 6, 18 };
                int k;
                for (k = 0; k < 3; k++)
                    {
                    int x = KM[k * 3], z = KM[k * 3 + 1], h = KM[k * 3 + 2];
                    iR->MeshLimb(m, 0, 1, x, 0, z, x, h, z, 4, 4, 3, 3, 4, 0);
                    iR->MeshEllipsoid(m, 0, 0, x, h + 2, z, h / 2 + 4, h / 4 + 2, h / 2 + 4, 4, 2);
                    }
                break;
                }
            case IM_INSECT:
                {
                // a dragonfly: a long body, four wings
                iR->MeshLimb(m, 0, 0, -30, 6, 0, 34, 6, 0, 4, 4, 2, 2, 4, 3);
                iR->MeshEllipsoid(m, 0, 0, -32, 6, 0, 6, 5, 6, 4, 2);
                int k;
                for (k = 0; k < 4; k++)
                    {
                    int x = k < 2 ? -18 : -8, s = (k & 1) ? 1 : -1;
                    int q[12] = { x, 7, 0,  x + 6, 7, s * 34,  x + 14, 7, s * 34,  x + 10, 7, 0 };
                    iR->MeshQuad(m, 0, 1, q, x + 6, 0, s * 10);
                    iR->MeshQuad(m, 0, 1, q, x + 6, 20, s * 10);
                    }
                break;
                }
            case IM_EGG:
                iR->MeshEllipsoid(m, 0, 0, 0, 14, 0, 11, 14, 11, 4, 2);
                iR->MeshEllipsoid(m, 0, 1, 4, 20, 6, 3, 3, 3, 4, 2);             // a speckle or two
                iR->MeshEllipsoid(m, 0, 1, -6, 12, 4, 3, 3, 3, 4, 2);
                break;
            case IM_FEATHER:
                {
                int k;
                for (k = 0; k < 3; k++)
                    {
                    int z = (k - 1) * 10, x = (k - 1) * 6;
                    int q[12] = { x, 2, z,  x + 8, 2, z + 30,  x + 2, 2, z + 90,  x - 8, 2, z + 30 };
                    iR->MeshQuad(m, 0, 0, q, x, 30, z + 40);
                    iR->MeshQuad(m, 0, 0, q, x, -30, z + 40);
                    iR->MeshLimb(m, 0, 1, x, 3, z - 12, x + 2, 3, z + 88, 1, 1, 1, 1, 3, 0);
                    }
                break;
                }
            case IM_CRYSTAL:
                {
                // a heap of salt crystals: pointed shards on a little base
                iR->MeshConeAt(m, 0, 1, 0, 0, 22, 0, 6, 6, 0);
                static const int KC[4 * 3] = { 0, 0, 34,  10, 6, 24,  -9, 5, 22,  2, -10, 20 };
                int k;
                for (k = 0; k < 4; k++) iR->MeshConeAt(m, 0, 0, KC[k * 3], KC[k * 3 + 1], 7, 4, KC[k * 3 + 2], 4, 0);
                break;
                }
            case IM_ROOT:
                // a twisted root, its tendrils
                iR->MeshLimb(m, 0, 0, -40, 6, 0, 10, 8, 4, 5, 5, 7, 7, 5, 1);
                iR->MeshLimb(m, 0, 0, 10, 8, 4, 40, 6, -6, 7, 7, 3, 3, 5, 2);
                iR->MeshLimb(m, 0, 1, -40, 6, 0, -60, 4, 10, 3, 3, 1, 1, 3, 2);
                iR->MeshLimb(m, 0, 1, -30, 6, 0, -52, 4, -12, 2, 2, 1, 1, 3, 2);
                break;
            default:    // fang / tusk
                iR->MeshCone(m, 0, 0, 22, 0, 140, 5, 0);
                iR->MeshCone(m, 0, 1, 24, 0, -20, 5, 0);
                break;
            }
        iR->MeshFinish(m);
        // how the item lies in the world: blades and shields flat, everything else upright; real-size scale
        static const int KRest[IM_COUNT * 2] =
            {
            // pitch scale
            256, 256,  256, 256,  256, 256,  256, 256,  256, 200,  0, 200,  0, 220,  0, 200,  0, 120,
            0, 90,  0, 256,  0, 90,  0, 80,  0, 180,  0, 90,  0, 200,  0, 120,  256, 70,  0, 256,
            256, 150,  0, 80,
            0, 256,  0, 256,  0, 256,  0, 256,  0, 256,  0, 256,  0, 256,  0, 256,  0, 256,  0, 256,  0, 256,
            0, 256,  0, 256,  0, 256,
            0, 256,  0, 256,  0, 256,  256, 256,
            0, 256,  0, 256,  0, 256,  0, 256,  0, 256,  0, 256,
            256, 256,  256, 256,  256, 256,  0, 256          // mace, bow, arrow: lying flat; ingot
            };
        int pitch = KRest[i * 2], sc = KRest[i * 2 + 1];
        int cp = r3d_cos(pitch), sp = r3d_sin(pitch), lo = 1 << 30, k;
        for (k = 0; k < m.nv; k++)
            {
            int y = (m.v[k * 3 + 1] * cp - m.v[k * 3 + 2] * sp) >> 14;
            if (y < lo) lo = y;
            }
        iItemRest[i * 3] = (short)pitch;
        iItemRest[i * 3 + 1] = (short)(-lo * sc >> 8);
        iItemRest[i * 3 + 2] = (short)sc;
        }
    }

// a box from corner to corner (units, y up) for the chest, turned by `aAng` (1024 = a full turn) about its lid's
// hinge, the line along x at the back of the body's top (y 124, z -92): the front of the lid rises as it opens
static void ChestBox(R3D& r, Mesh& m, int fr, int slot, int x0, int y0, int z0, int x1, int y1, int z1, int aAng)
    {
    const int hy = 124, hz = -92;
    int cs = r3d_cos(aAng), sn = r3d_sin(aAng);
    int P[8 * 3], k;
    for (k = 0; k < 8; k++)
        {
        int y = (k & 2) ? y1 : y0, z = (k & 4) ? z1 : z0;
        int dy = y - hy, dz = z - hz;
        P[k * 3] = (k & 1) ? x1 : x0;
        P[k * 3 + 1] = hy + ((dy * cs + dz * sn) >> 14);
        P[k * 3 + 2] = hz + ((dz * cs - dy * sn) >> 14);
        }
    int cx = (x0 + x1) / 2, cy = 0, cz = 0;
    for (k = 0; k < 8; k++) { cy += P[k * 3 + 1]; cz += P[k * 3 + 2]; }
    cy /= 8; cz /= 8;
    static const signed char KF[6][4] = { { 0, 2, 6, 4 }, { 1, 5, 7, 3 }, { 0, 4, 5, 1 }, { 2, 3, 7, 6 }, { 0, 1, 3, 2 }, { 4, 6, 7, 5 } };
    int f, q[12];
    for (f = 0; f < 6; f++)
        {
        if (f == 2 && y0 == 0 && aAng == 0) continue;          // the bottom on the floor: never seen
        for (k = 0; k < 4; k++) { q[k * 3] = P[KF[f][k] * 3]; q[k * 3 + 1] = P[KF[f][k] * 3 + 1]; q[k * 3 + 2] = P[KF[f][k] * 3 + 2]; }
        r.MeshQuad(m, fr, slot, q, cx, cy, cz);
        }
    }

void Game::BuildMeshes()
    {
    int k;
    for (k = 0; k < 5; k++) { BuildHumanoid(iMHuman[k], k, 0); BuildHumanoid(iMHumanLod[k], k, 1); }
    BuildHumanoid(iMHuman[5], 7, 0); BuildHumanoid(iMHumanLod[5], 7, 1);   // the caravan's cat-folk
    BuildHumanoid(iMHuman[6], 8, 0); BuildHumanoid(iMHumanLod[6], 8, 1);   // the catacombs' skeletons
    BuildHumanoid(iMPlayer[0], 5, 0); BuildHumanoid(iMPlayer[1], 6, 0);
    BuildWolf(iMWolf);
    BuildWolfLod(iMWolfLod);
    BuildDragon(iMDragon);
    BuildItems();
    BuildFlora();
    BuildAnimals();
    // fireball: octahedron, slots 0 core 1 glow
    iR->MeshInit(iMFireball, 12, 12, 1);
    iR->MeshCone(iMFireball, 0, 0, 70, 0, 90, 4, 0);
    iR->MeshCone(iMFireball, 0, 1, 70, 0, -90, 4, 128);
    iR->MeshFinish(iMFireball);
    int fr;
    // flame (view model hand, candles, hearths): 8 flicker frames; slots 0 inner, 1 outer, 2 core
    iR->MeshInit(iMFlame, 110, 110, 8);
    for (fr = 0; fr < 8; fr++)
        {
        iR->iMeshCursor = 0;
        iR->MeshFlame(iMFlame, fr, 22, 60, 1, 0, 2, 0);
        }
    iR->MeshFinish(iMFlame);
    // a candle's flame (a few pixels on the screen): a tapering flicker and its core, 12 triangles (the hearths' flame
    // above is ninety)
    iR->MeshInit(iMFlameSmall, 24, 16, 8);
    for (fr = 0; fr < 8; fr++)
        {
        iR->iMeshCursor = 0;
        int ph = fr * 128, h = 60 * (200 + ((r3d_sin(ph) + 16384) * 60 >> 15)) / 256, sw = (r3d_sin(ph + 300) * 7) >> 14;
        iR->MeshLimb(iMFlameSmall, fr, 1, 0, 0, 0, sw, h, -sw / 2, 13, 13, 2, 2, 3, 0);
        iR->MeshLimb(iMFlameSmall, fr, 2, 0, 0, 0, sw / 2, h * 3 / 5, 0, 7, 7, 1, 1, 3, 0);
        }
    iR->MeshFinish(iMFlameSmall);
    // chest: Skyrim's Chest01 (96 x 49 units, the body 34 high, the lid 14), 5 frames (closed .. open), its front +z.
    // Slots 0 the body's planks, 1 iron, 2 the lid's planks, 3 the dark inside. The lid is the whole top, hinged
    // along the back edge, and swings up and back 100 degrees; its straps and the latch go with it.
    {
    enum { W = 176, D = 90, HB = 124, HL = 51 };
    iR->MeshInit(iMChest, 360, 320, 5);          // (the boxes' faces have corners of their own: 24 each)
    for (fr = 0; fr < 5; fr++)
        {
        iR->iMeshCursor = 0;
        ChestBox(*iR, iMChest, fr, 0, -W, 0, -D, W, HB - 2, D, 0);                       // the body
        ChestBox(*iR, iMChest, fr, 3, -W + 8, HB - 2, -D + 8, W - 8, HB, D - 8, 0);       // its dark inside
        int s;
        for (s = -1; s <= 1; s += 2)
            {
            ChestBox(*iR, iMChest, fr, 1, s * W - 5, 0, D - 6, s * W + 5, HB, D + 2, 0);  // iron at the corners
            ChestBox(*iR, iMChest, fr, 1, s * W - 5, 0, -D - 2, s * W + 5, HB, -D + 6, 0);
            ChestBox(*iR, iMChest, fr, 1, s * 60 - 34, 36, D, s * 60 + 34, 44, D + 3, 0); // the front's straps
            }
        ChestBox(*iR, iMChest, fr, 1, -14, 74, D, 14, HB, D + 3, 0);                      // the lock plate
        int a = fr * 71;                                                                  // 0, 25 .. 100 degrees open
        ChestBox(*iR, iMChest, fr, 2, -W - 2, HB, -D - 2, W + 2, HB + HL, D + 2, a);       // the lid
        ChestBox(*iR, iMChest, fr, 1, -W - 3, HB, D + 2, W + 3, HB + 10, D + 5, a);       // its iron lip
        for (s = -1; s <= 1; s += 2)
            ChestBox(*iR, iMChest, fr, 1, s * 96 - 9, HB + HL, -D - 2, s * 96 + 9, HB + HL + 3, D + 4, a);   // straps
        ChestBox(*iR, iMChest, fr, 1, -9, HB - 30, D + 5, 9, HB + HL, D + 8, a);         // the latch
        }
    iR->MeshFinish(iMChest);
    iR->MeshPlanarUV(iMChest, 2);
    }
    // first-person hand (fist), slot 0 skin/glove, 1 sleeve
    // first-person hands: slot 0 skin or glove, slot 1 sleeve / armour cuff
    // gripping hand: wraps a vertical grip (y axis, radius ~9) between y = 10 and 58
    {
    Mesh& h = iMHand;
    iR->MeshInitSmooth(h, 300, 520, 1);
    iR->MeshLimb(h, 0, 1, -8, -170, -8, -6, -20, -4, 34, 32, 30, 28, 7, 1);         // forearm in the sleeve
    iR->MeshLimb(h, 0, 1, -6, -26, -4, -5, -6, -3, 32, 30, 32, 30, 7, 0);           // cuff
    iR->MeshLimb(h, 0, 0, -6, -10, -4, -12, 18, -2, 24, 20, 22, 19, 6, 0);          // wrist
    iR->MeshEllipsoid(h, 0, 0, -16, 34, -4, 13, 30, 22, 6, 4);                     // palm behind the grip
    int k;
    for (k = 0; k < 4; k++)
        {
        int y = 18 + k * 12 - (k == 3 ? 2 : 0);
        int r = k == 3 ? 5 : 6;
        iR->MeshLimb(h, 0, 0, -16, y, 14, 2, y, 16, r + 1, r + 1, r, r, 5, 0);           // proximal: across the front
        iR->MeshLimb(h, 0, 0, 2, y, 16, 13, y, 4, r, r, r - 1, r - 1, 5, 0);              // middle: round the grip
        iR->MeshLimb(h, 0, 0, 13, y, 4, 10, y, -8, r - 1, r - 1, r - 2, r - 2, 4, 2);    // tip tucked in
        iR->MeshEllipsoid(h, 0, 0, 2, y, 17, 7, 6, 6, 4, 2);                              // knuckle
        }
    iR->MeshLimb(h, 0, 0, -18, 54, -14, -6, 64, 2, 9, 9, 8, 8, 5, 0);               // thumb over the fingers
    iR->MeshLimb(h, 0, 0, -6, 64, 2, 6, 60, 14, 8, 8, 6, 6, 5, 2);
    iR->MeshFinish(h);
    iR->MeshSmooth(h);
    }
    // open hand (spell casting): palm up, fingers spread and slightly curled
    {
    Mesh& h = iMHandOpen;
    iR->MeshInitSmooth(h, 200, 330, 1);
    iR->MeshLimb(h, 0, 1, 0, -170, -10, 0, -20, -4, 34, 32, 30, 28, 7, 1);
    iR->MeshLimb(h, 0, 1, 0, -26, -4, 0, -6, -3, 32, 30, 32, 30, 7, 0);
    iR->MeshLimb(h, 0, 0, 0, -10, -4, 0, 16, -2, 24, 18, 22, 17, 6, 0);
    iR->MeshEllipsoid(h, 0, 0, 0, 40, 0, 26, 30, 11, 6, 4);                      // palm
    int k;
    for (k = 0; k < 4; k++)
        {
        int x = -18 + k * 12;
        int len = k == 0 || k == 3 ? 34 : 42;
        int sp = (k - 1) * 4 - 2;
        iR->MeshLimb(h, 0, 0, x, 64, 2, x + sp, 64 + len / 2, 6, 6, 6, 5, 5, 5, 0);
        iR->MeshLimb(h, 0, 0, x + sp, 64 + len / 2, 6, x + sp * 2, 64 + len, 16, 5, 5, 4, 4, 4, 2);
        }
    iR->MeshLimb(h, 0, 0, -24, 30, 4, -46, 52, 12, 9, 8, 7, 7, 5, 0);               // thumb
    iR->MeshLimb(h, 0, 0, -46, 52, 12, -52, 72, 20, 7, 7, 6, 6, 4, 2);
    iR->MeshFinish(h);
    iR->MeshSmooth(h);
    }
    }

// ---------------------------------------------------------------- the land's plants (v0.4.39)
// Skyrim's bushes, ferns, flowers and crops as small models of the engine's own (theirs are cut-out cards): a kind
// each (gen_kit.py's FLORA), slot 0 its colour, slot 1 the green of its stems
void Game::BuildFlora()
    {
    R3D& r = *iR;
    int k, i;
    for (k = 0; k < FK_COUNT; k++)
        {
        Mesh& m = iMFlora[k];
        r.MeshInit(m, 64, 64, 1);
        r.iMeshCursor = 0;
        switch (k)
            {
            case 0: case 1: case 2: case 17: case 18: case 19:
                {
                // a bush: a dome and a smaller one beside it (thicket, yellow, tundra, vine maple, snowy, autumn)
                int s = k == 2 ? 70 : (k == 0 ? 110 : 90);
                r.MeshEllipsoid(m, 0, 0, 0, s * 4 / 10, 0, s * 6 / 10, s * 45 / 100, s * 6 / 10, 5, 2);
                r.MeshEllipsoid(m, 0, 0, s * 4 / 10, s * 25 / 100, -s * 3 / 10, s * 35 / 100, s * 25 / 100, s * 35 / 100, 4, 1);
                break;
                }
            case 3: case 20:
                // dead twigs: thin sticks leaning out (a dead shrub; driftwood taller)
                for (i = 0; i < 4; i++)
                    {
                    int a = i * 256 + 40, h = k == 20 ? 150 : 70;
                    r.MeshConeAt(m, 0, 0, (r3d_sin(a) * 20) >> 14, (r3d_cos(a) * 20) >> 14, 5, 0, h - i * 8, 3, a);
                    }
                break;
            case 4:
                // a fern: six fronds arching out, seen from above and below
                for (i = 0; i < 6; i++)
                    {
                    int a = i * 171, c = r3d_cos(a), s = r3d_sin(a);
                    int P[9] = { 0, 8, 0, (s * 80) >> 14, 34, (c * 80) >> 14, ((s * 40) >> 14) + ((c * 14) >> 14), 40, ((c * 40) >> 14) - ((s * 14) >> 14) };
                    r.MeshTri(m, 0, 0, P, 0, -100, 0);
                    r.MeshTri(m, 0, 0, P, 0, 200, 0);
                    }
                break;
            case 5: case 6: case 7: case 9: case 10:
                // flowers (purple, blue, red), a thistle, tundra cotton: stems with a head each
                for (i = 0; i < 3; i++)
                    {
                    int a = i * 341 + 60, x = (r3d_sin(a) * 12) >> 14, z = (r3d_cos(a) * 12) >> 14, h = 34 + i * 8;
                    r.MeshConeAt(m, 0, 1, x, z, 3, 0, h, 3, a);
                    if (k == 10) r.MeshEllipsoid(m, 0, 0, x, h + 4, z, 7, 6, 7, 4, 1);
                    else { r.MeshConeAt(m, 0, 0, x, z, k == 9 ? 6 : 9, h - 4, h + (k == 9 ? 10 : 5), 4, a); r.MeshConeAt(m, 0, 0, x, z, k == 9 ? 6 : 9, h - 4, h - 9, 4, a); }
                    }
                break;
            case 8:
                // lavender: a low green tuft, purple spikes out of it
                r.MeshEllipsoid(m, 0, 1, 0, 10, 0, 22, 12, 22, 4, 1);
                for (i = 0; i < 4; i++)
                    {
                    int a = i * 256 + 128;
                    r.MeshConeAt(m, 0, 0, (r3d_sin(a) * 10) >> 14, (r3d_cos(a) * 10) >> 14, 4, 18, 50 + (i & 1) * 8, 3, a);
                    }
                break;
            case 11:
                // clover: a low green mat
                r.MeshEllipsoid(m, 0, 0, 0, 2, 0, 40, 8, 34, 5, 1);
                break;
            case 12:
                // snowberries: a pale bush, red berries on it
                r.MeshEllipsoid(m, 0, 1, 0, 35, 0, 40, 35, 40, 5, 2);
                r.MeshBox(m, 0, 0, 18, 60, 12, 0, 5, 5, 5);
                r.MeshBox(m, 0, 0, -16, 54, -10, 0, 5, 5, 5);
                break;
            case 13:
                // a cabbage: a green ball in its leaves
                r.MeshEllipsoid(m, 0, 0, 0, 16, 0, 20, 16, 20, 5, 2);
                r.MeshEllipsoid(m, 0, 1, 0, 4, 0, 30, 5, 30, 5, 1);
                break;
            case 14:
                // potatoes: a leafy low plant
                r.MeshEllipsoid(m, 0, 0, 0, 14, 0, 32, 16, 32, 5, 1);
                break;
            case 15: case 16:
                // leeks (three stalks), wheat (five, tall and golden)
                for (i = 0; i < (k == 16 ? 5 : 3); i++)
                    {
                    int a = i * (k == 16 ? 205 : 341), x = (r3d_sin(a) * 14) >> 14, z = (r3d_cos(a) * 14) >> 14;
                    r.MeshConeAt(m, 0, 0, x, z, k == 16 ? 4 : 6, 0, k == 16 ? 90 + i * 6 : 45, 3, a);
                    }
                break;
            }
        r.MeshFinish(m);
        r.MeshTrim(m);
        }
    // a tuft of grass: three blades leaning out, seen from both sides
    r.MeshInit(iMTuft, 18, 8, 1);       // (v0.4.51: three blades, both sides - 18; it had room for 12, the third blade left out)
    r.iMeshCursor = 0;
    for (i = 0; i < 3; i++)
        {
        int a = i * 341 + 90, c = r3d_cos(a), s = r3d_sin(a);
        int P[9] = { (c * 8) >> 14, 0, -(s * 8) >> 14, -(c * 8) >> 14, 0, (s * 8) >> 14, (s * 22) >> 14, 46 + i * 6, (c * 22) >> 14 };
        r.MeshTri(iMTuft, 0, i == 1 ? 1 : 0, P, (s * 100) >> 14, 20, (c * 100) >> 14);
        r.MeshTri(iMTuft, 0, i == 1 ? 1 : 0, P, -(s * 100) >> 14, 20, -(c * 100) >> 14);
        }
    r.MeshFinish(iMTuft);
    r.MeshTrim(iMTuft);
    }

// ---------------------------------------------------------------- the land's animals (v0.4.39)
// a four-legged animal, walking (frames 0..15: a stride), dead (16: down on the ground). Its body aLen long (half),
// aH deep (half), legs aLeg long, a neck aNeck long rising in front, a head aHead long, horns or antlers aHorn high
// (negative: a goat's, swept back), ears aEar, a tail aTail. Slots: 0 coat, 1 legs and dark, 2 light (belly,
// tail), 3 horn
void Game::BuildQuadruped(Mesh& m, int aLen, int aH, int aLeg, int aNeck, int aHead, int aHorn, int aEar, int aTail)
    {
    R3D& r = *iR;
    r.MeshInit(m, aHorn > 0 ? 160 : 140, 140, 17);     // (v0.4.51: antlers - the deer's and the elk's last tine had no room)
    int fr, w = aH * 7 / 10 + 6;
    for (fr = 0; fr < 16; fr++)
        {
        r.iMeshCursor = 0;
        int p = fr * 64;
        int a = (r3d_sin(p) * 70) >> 14, b = (r3d_sin(p + 512) * 70) >> 14;          // (a stride: 25 degrees each way)
        int bob = (r3d_abs(r3d_sin(p)) * (aLeg / 12)) >> 14;
        int y0 = aLeg + bob, top = y0 + aH * 2;
        r.MeshBox(m, fr, 0, 0, y0 + 2 * aH, 0, 0, w, aH, aLen);                                // body (MeshBox: from its top down)
        r.MeshBox(m, fr, 2, 0, y0 + aH * 2 / 3, 0, 0, w * 8 / 10, aH / 3 + 1, aLen * 7 / 10);   // belly
        int nz = aLen * 9 / 10, ny = top + aNeck / 2 - aH / 2;
        r.MeshBox(m, fr, 0, 0, ny + aNeck / 2 + 4, nz, 120, w * 55 / 100, aNeck / 2 + 4, w * 55 / 100);   // neck, leaning on
        int hy = top + aNeck - aH / 3, hz = nz + aNeck / 3 + aHead / 2;
        r.MeshBox(m, fr, 0, 0, hy + w * 45 / 100, hz, 0, w * 5 / 10, w * 45 / 100, aHead / 2);   // head
        r.MeshBox(m, fr, 1, 0, hy - w / 8 + w * 3 / 10, hz + aHead / 2, 0, w * 3 / 10, w * 3 / 10, aHead / 6 + 2);   // muzzle
        if (aEar)
            {
            r.MeshBox(m, fr, 0, -w * 4 / 10, hy + w * 4 / 10 + aEar, hz - aHead / 4, 0, 5, aEar / 2, 4);
            r.MeshBox(m, fr, 0, w * 4 / 10, hy + w * 4 / 10 + aEar, hz - aHead / 4, 0, 5, aEar / 2, 4);
            }
        if (aHorn > 0)
            {
            // antlers: a beam up from each side of the head, branching out
            int k;
            for (k = -1; k <= 1; k += 2)
                {
                int bx = k * w * 35 / 100, by = hy + w * 4 / 10;
                r.MeshBox(m, fr, 3, bx + k * aHorn / 6, by + aHorn, hz - aHead / 3, 0, 4, aHorn / 2, 4);
                r.MeshBox(m, fr, 3, bx + k * aHorn / 3, by + aHorn * 7 / 10 + 3, hz - aHead / 3 + aHorn / 8, 0, aHorn / 6, 3, 3);
                r.MeshBox(m, fr, 3, bx + k * aHorn / 4, by + aHorn + 3, hz - aHead / 3 - aHorn / 8, 0, 3, 3, aHorn / 6);
                }
            }
        else if (aHorn < 0)
            {
            // a goat's horns, swept back over its neck
            r.MeshBox(m, fr, 3, -w * 3 / 10, hy + w * 45 / 100 + 4, hz - aHead / 2, -200, 5, -aHorn / 2, 5);
            r.MeshBox(m, fr, 3, w * 3 / 10, hy + w * 45 / 100 + 4, hz - aHead / 2, -200, 5, -aHorn / 2, 5);
            }
        int lw = w / 4 + 3, lx = w * 7 / 10, lz = aLen * 8 / 10;
        r.MeshBox(m, fr, 1, -lx, y0 + 10, lz, a, lw, (y0 + 10) / 2, lw);                         // legs
        r.MeshBox(m, fr, 1, lx, y0 + 10, lz, b, lw, (y0 + 10) / 2, lw);
        r.MeshBox(m, fr, 1, -lx, y0 + 10, -lz, b, lw, (y0 + 10) / 2, lw);
        r.MeshBox(m, fr, 1, lx, y0 + 10, -lz, a, lw, (y0 + 10) / 2, lw);
        if (aTail) r.MeshBox(m, fr, 2, 0, y0 + aH * 2 - 4, -aLen, 300, w / 4 + 2, aTail / 2, w / 4 + 2);
        }
    RotateFrame(m, 0, 16, 0, -aLeg * 9 / 10);
    r.MeshFinish(m);
    }

// the animals' models: deer, elk, hare, fox, goat (four-legged), mudcrab
void Game::BuildAnimals()
    {
    R3D& r = *iR;
    BuildQuadruped(iMAnimal[0], 150, 52, 230, 120, 70, 70, 30, 30);      // deer: 1.2 m long, 0.9 m at the shoulder, small antlers
    BuildQuadruped(iMAnimal[1], 200, 72, 290, 150, 90, 190, 36, 24);     // elk: bigger, great antlers
    BuildQuadruped(iMAnimal[2], 50, 22, 40, 20, 34, 0, 46, 14);          // hare: small, long ears
    BuildQuadruped(iMAnimal[3], 90, 28, 110, 50, 60, 0, 24, 110);        // fox: low, a long brush of a tail
    BuildQuadruped(iMAnimal[4], 130, 50, 160, 70, 60, -90, 18, 20);      // goat: stocky, horns swept back
    // the mudcrab: a broad flat shell, six legs, two claws; walking (0..15), dead on its back (16)
    Mesh& m = iMAnimal[5];
    r.MeshInit(m, 120, 120, 17);
    int fr;
    for (fr = 0; fr < 16; fr++)
        {
        r.iMeshCursor = 0;
        int p = fr * 64, a = (r3d_sin(p) * 160) >> 14, b = -a;
        r.MeshEllipsoid(m, fr, 0, 0, 46, 0, 92, 34, 76, 6, 2);                    // shell
        int k;
        for (k = 0; k < 3; k++)
            {
            int z = -40 + k * 40, sw = (k & 1) ? a : b;
            r.MeshBox(m, fr, 1, -100, 50, z, sw, 6, 25, 6);
            r.MeshBox(m, fr, 1, 100, 50, z, -sw, 6, 25, 6);
            }
        r.MeshBox(m, fr, 2, -46, 54, 96, a / 2, 20, 14, 30);                       // claws
        r.MeshBox(m, fr, 2, 46, 54, 96, b / 2, 20, 14, 30);
        }
    RotateFrame(m, 0, 16, 512, 70);
    r.MeshFinish(m);
    }
