// game_organic.cpp - smooth, jointed character models (people, wolf, dragon) for close range
#include "game.h"

// a rotation in the y/z plane: angle a (forward positive) applied to a downward segment of length L
static void Seg(int aA, int aL, int& aDy, int& aDz)
    {
    aDy = (-aL * r3d_cos(aA)) >> 14;
    aDz = (aL * r3d_sin(aA)) >> 14;
    }

static int Sc(int v, int s) { return v * s >> 8; }

struct Body
    {
    short hs, ws, armL, lean, weapon, skirt, longHair, beard, helmet, headS;
    short hipW, waist, chestD, belly;      // torso shape adjustments (units)
    };
// kinds: 0 man, 1 elder, 2 barrow wight, 3 troll, 4 woman, 5 player (sword sheathed), 6 player (sword drawn),
// 7 cat-folk caravan trader (furred, pointed ears, muzzle, tail), 8 skeleton (bare bones, an ancient blade)
static const Body KBody[9] =
    {
    // hs   ws  arm lean wpn skirt hair beard helm head  hipW waist chestD belly
    { 256, 256, 256,  6, 0,  60,  0,  1,  0, 256,    0,   0,    0,    0 },
    { 250, 262, 250, 26, 3, 200,  0,  1,  0, 256,    2,   4,    0,    4 },
    { 262, 196, 280, 46, 1,  40,  0,  0,  1, 236,   -4,  -6,   -4,   -2 },
    { 330, 380, 360, 90, 2,  70,  0,  0,  0, 200,    2,  10,    6,   14 },
    { 244, 228, 240,  4, 0, 210,  1,  0,  0, 246,   10,  -7,    7,    0 },
    { 262, 270, 262,  4, 4,  50,  0,  1,  0, 256,    0,   0,    2,    0 },
    { 262, 270, 262,  4, 1,  50,  0,  1,  0, 256,    0,   0,    2,    0 },
    { 250, 236, 256, 10, 0, 190,  0,  0,  0, 250,    4,  -6,    2,    0 },
    { 256, 230, 256, 22, 1,   0,  0,  0,  0, 240,    0,   0,    0,    0 }
    };

enum { HF_ATTACK = 16, HF_DEAD = 24, HF_SIT = 25, HF_COUNT = 26 };

// colour the head faces: hair on top/back, beard on the lower front, helmet for wights
void Game::PaintHead(Mesh& m, int aF0, int aF1, int aHy, int aHz, int aHs, int aKind)
    {
    const Body& b = KBody[aKind];
    int i;
    for (i = aF0; i < aF1; i++)
        {
        u16* f = m.f + i * 4;
        const short* A = m.v + f[0] * 3; const short* B = m.v + f[1] * 3; const short* C = m.v + f[2] * 3;
        int ly = ((A[1] + B[1] + C[1]) / 3 - aHy) * 256 / aHs;     // head-local units (head radius ~25)
        int lz = ((A[2] + B[2] + C[2]) / 3 - aHz) * 256 / aHs;
        int hair;
        if (b.helmet) hair = ly > 2 || (lz < -4 && ly > -14);
        else hair = ly > 9 || (lz < -6 && ly > -16) || (b.longHair && lz < 2 && ly > -22);
        if (b.beard && ly < -6 && lz > 2) hair = 1;
        if (hair) f[3] = 4;
        }
    }

// ---- skeleton chains: joints rotate in the body's y/z plane, rings are lofted along the bones ----
struct Chain
    {
    int x, y[4], z[4], ang[3], nb, front;
    };

static void ChainInit(Chain& c, int x, int y, int z, int aFront)
    {
    c.x = x; c.y[0] = y; c.z[0] = z; c.nb = 0; c.front = aFront;
    }

static void ChainBone(Chain& c, int aAng, int aLen)
    {
    int dy, dz;
    Seg(aAng, aLen, dy, dz);
    c.ang[c.nb] = aAng;
    c.y[c.nb + 1] = c.y[c.nb] + dy; c.z[c.nb + 1] = c.z[c.nb] + dz;
    c.nb++;
    }

// ring at t (256 per bone), radii ra (sideways) rb (front/back), centre pushed 'off' towards the front
static int* Ring(int* r, const Chain& c, int t, int ra, int rb, int off, int slot)
    {
    int k = t >> 8;
    if (k >= c.nb) k = c.nb - 1;
    int f = t - k * 256;
    int a = c.ang[k];
    if (f < 24 && k > 0) a = (c.ang[k - 1] + c.ang[k]) / 2;
    int cy = c.y[k] + (((c.y[k + 1] - c.y[k]) * f) >> 8);
    int cz = c.z[k] + (((c.z[k + 1] - c.z[k]) * f) >> 8);
    int fy = r3d_sin(a) * c.front, fz = r3d_cos(a) * c.front;
    r[0] = c.x; r[1] = cy + ((fy * off) >> 14); r[2] = cz + ((fz * off) >> 14);
    r[3] = 16384; r[4] = 0; r[5] = 0;
    r[6] = 0; r[7] = fy; r[8] = fz;
    r[9] = ra < 2 ? 2 : ra; r[10] = rb < 2 ? 2 : rb; r[11] = slot;
    return r + 12;
    }

static void ChainPoint(const Chain& c, int t, int& y, int& z)
    {
    int k = t >> 8;
    if (k >= c.nb) k = c.nb - 1;
    int f = t - k * 256;
    y = c.y[k] + (((c.y[k + 1] - c.y[k]) * f) >> 8);
    z = c.z[k] + (((c.z[k + 1] - c.z[k]) * f) >> 8);
    }

// slots: 0 legs, 1 tunic, 2 skin, 3 metal, 4 hair, 5 boots/leather, 6 wood/belt
// aLod: 0 = smooth close-range model, 1 = light flat model for distance
void Game::BuildHumanoid(Mesh& m, int aKind, int aLod)
    {
    const Body& b = KBody[aKind];
    int bones = aKind == 8;
    if (aLod) iR->MeshInit(m, bones ? 180 : 150, bones ? 240 : 200, HF_COUNT);
    else if (bones) iR->MeshInitSmooth(m, 320, 480, HF_COUNT);
    else iR->MeshInitSmooth(m, 500, 820, HF_COUNT);
    int R[12 * 10];
    int fr;
    for (fr = 0; fr < HF_COUNT; fr++)
        {
        iR->iMeshCursor = 0;
        // ---- pose ----
        int hipA[2], knee[2], shA[2], elb[2];
        int lean = b.lean, bob = 0;
        int armed = b.weapon == 1 || b.weapon == 2;
        if (fr < HF_ATTACK)
            {
            int p = fr * 64;
            int s = r3d_sin(p);
            hipA[0] = (s * 96) >> 14; hipA[1] = -hipA[0];
            knee[0] = 24 + (((r3d_sin(p - 300) + 16384) * 70) >> 15);
            knee[1] = 24 + (((r3d_sin(p + 512 - 300) + 16384) * 70) >> 15);
            shA[0] = -hipA[0] * 3 / 4; shA[1] = -hipA[1] * 3 / 4;
            elb[0] = 40 + (shA[0] > 0 ? shA[0] / 2 : 0); elb[1] = 40 + (shA[1] > 0 ? shA[1] / 2 : 0);
            if (armed) { shA[1] = 70 + hipA[0] / 4; elb[1] = 90; }
            if (b.weapon == 3) { shA[1] = 40; elb[1] = 160; }
            bob = (r3d_abs(r3d_cos(p)) * 8) >> 14;
            }
        else if (fr < HF_DEAD)
            {
            int t = fr - HF_ATTACK;
            hipA[0] = 60; hipA[1] = -40; knee[0] = 70; knee[1] = 40;
            shA[0] = 40; elb[0] = 80;
            if (t < 4) { shA[1] = 70 + 110 * t; elb[1] = 90 + 20 * t; }    // raise overhead
            else { shA[1] = 400 - 90 * (t - 4); elb[1] = 150 - 35 * (t - 4); }  // chop down
            lean += t < 4 ? -t * 6 : (t - 4) * 14;
            }
        else if (fr == HF_SIT)
            {
            // seated: thighs level, shins down, hands resting on the knees
            hipA[0] = hipA[1] = 250; knee[0] = knee[1] = 250; shA[0] = shA[1] = 70; elb[0] = elb[1] = 110;
            }
        else
            {
            hipA[0] = 10; hipA[1] = -10; knee[0] = knee[1] = 20; shA[0] = shA[1] = 20; elb[0] = elb[1] = 30;
            }
        // ---- skeleton ----
        int T = Sc(113, b.hs), SH = Sc(108, b.hs), AN = Sc(18, b.hs);
        int hipY = T + SH + AN + bob;
        if (fr == HF_SIT) hipY = SH + AN + Sc(4, b.hs);            // on a 45 cm seat
        int HW = Sc(26, b.ws), SW = Sc(56, b.ws);
        int s;
        // spine: pelvis -> lumbar -> thoracic (front = -1 because the bones point up)
        Chain sp;
        ChainInit(sp, 0, hipY - Sc(16, b.hs), 0, -1);
        ChainBone(sp, 512 - lean / 2, Sc(84, b.hs));
        ChainBone(sp, 512 - lean, Sc(98, b.hs));
        int topY = sp.y[2], topZ = sp.z[2];
        // legs (front = +1)
        for (s = 0; s < 2; s++)
            {
            Chain c;
            ChainInit(c, s ? HW : -HW, hipY, 0, 1);
            ChainBone(c, hipA[s], T);
            ChainBone(c, hipA[s] - knee[s], SH);
            int* r = R;
            if (bones)
                {
                // femur, knee cap, shin bone, the small bones of the foot
                iR->MeshLimb(m, fr, 0, c.x, c.y[0], c.z[0], c.x, c.y[1], c.z[1], 7, 7, 5, 5, aLod ? 3 : 4, 0);
                if (!aLod) iR->MeshEllipsoid(m, fr, 2, c.x, c.y[1], c.z[1] + 3, 8, 8, 8, 4, 2);
                iR->MeshLimb(m, fr, 0, c.x, c.y[1], c.z[1], c.x, c.y[2], c.z[2], 6, 6, 4, 4, aLod ? 3 : 4, 0);
                int fdy, fdz;
                Seg(hipA[s] - knee[s] + 256, Sc(40, b.hs), fdy, fdz);
                iR->MeshLimb(m, fr, 5, c.x, c.y[2], c.z[2] - 3, c.x, c.y[2] + fdy / 3 - 3, c.z[2] + fdz, 8, 5, 5, 3, aLod ? 3 : 4, 2);
                continue;
                }
            if (aLod)
                {
                r = Ring(r, c, 0, Sc(20, b.ws), Sc(21, b.ws), 0, 0);
                r = Ring(r, c, 256, Sc(13, b.ws), Sc(14, b.ws), 0, 5);
                r = Ring(r, c, 512, Sc(9, b.ws), Sc(10, b.ws), 0, 5);
                iR->MeshRings(m, fr, R, 3, 4, 0);
                }
            else
                {
                r = Ring(r, c, 0, Sc(22, b.ws), Sc(23, b.ws), 0, 0);
                r = Ring(r, c, 70, Sc(20, b.ws), Sc(21, b.ws), 2, 0);       // quadriceps
                r = Ring(r, c, 200, Sc(15, b.ws), Sc(15, b.ws), 0, 0);
                r = Ring(r, c, 256, Sc(13, b.ws), Sc(14, b.ws), 1, 5);      // knee, boot top
                r = Ring(r, c, 310, Sc(14, b.ws), Sc(16, b.ws), -3, 5);     // calf
                r = Ring(r, c, 430, Sc(10, b.ws), Sc(10, b.ws), 0, 5);
                r = Ring(r, c, 512, Sc(9, b.ws), Sc(10, b.ws), 0, 5);       // ankle
                iR->MeshRings(m, fr, R, 7, 6, 2);
                }
            int aY = c.y[2], aZ = c.z[2];
            int fdy, fdz;
            Seg(hipA[s] - knee[s] + 256, Sc(44, b.hs), fdy, fdz);        // foot points forward
            iR->MeshLimb(m, fr, 5, c.x, aY, aZ - 4, c.x, aY + fdy / 3 - 4, aZ + fdz, Sc(11, b.ws), Sc(10, b.ws), Sc(6, b.ws), Sc(9, b.ws), aLod ? 3 : 4, 2);
            }
        // torso
        if (bones)
            {
            // pelvis, spine, a ribcage (its hollows painted dark between the ribs), collar bones
            int py, pz, my, mz, cy, cz;
            ChainPoint(sp, 20, py, pz); ChainPoint(sp, 256, my, mz); ChainPoint(sp, 370, cy, cz);
            iR->MeshEllipsoid(m, fr, 0, 0, py + 2, pz, Sc(34, b.ws), 15, 16, aLod ? 4 : 6, aLod ? 2 : 3);
            iR->MeshLimb(m, fr, 0, 0, py, pz - 8, 0, my, mz - 8, 6, 6, 6, 6, aLod ? 3 : 4, 0);
            iR->MeshLimb(m, fr, 0, 0, my, mz - 8, 0, topY, topZ - 4, 6, 6, 5, 5, aLod ? 3 : 4, 0);
            int f0 = m.nf;
            iR->MeshEllipsoid(m, fr, 0, 0, cy, cz + 2, Sc(44, b.ws), Sc(50, b.hs), 28, aLod ? 5 : 8, aLod ? 3 : 7);
            if (fr == 0 && !aLod)
                {
                int i;
                for (i = f0; i < m.nf; i++)
                    {
                    u16* f = m.f + i * 4;
                    const short* A = m.v + f[0] * 3; const short* B = m.v + f[1] * 3; const short* C = m.v + f[2] * 3;
                    int ly = (A[1] + B[1] + C[1]) / 3 - cy, lz = (A[2] + B[2] + C[2]) / 3 - cz;
                    int band = ((ly + 60) * 8 / 100) & 1;
                    if ((band && ly < 30) || (lz > 8 && ly < -10 && ly > -40 && ((A[0] + B[0] + C[0]) / 3) * ((A[0] + B[0] + C[0]) / 3) < 100)) f[3] = 1;
                    }
                }
            int shy, shz;
            ChainPoint(sp, 440, shy, shz);
            iR->MeshLimb(m, fr, 0, -SW, shy, shz, SW, shy, shz, 4, 4, 4, 4, 3, 3);
            }
        else
            {
            int hw = b.hipW, wa = b.waist, cd = b.chestD, be = b.belly;
            int* r = R;
            if (aLod)
                {
                r = Ring(r, sp, 0, Sc(30 + hw, b.ws), Sc(22, b.ws), 0, 0);
                r = Ring(r, sp, 60, Sc(45 + hw, b.ws), Sc(28 + be / 2, b.ws), -2, 1);
                r = Ring(r, sp, 340, Sc(48, b.ws), Sc(31 + cd, b.ws), 3, 1);
                r = Ring(r, sp, 450, Sc(56, b.ws), Sc(27, b.ws), 0, 1);
                r = Ring(r, sp, 512, Sc(22, b.ws), Sc(17, b.ws), -2, 1);
                iR->MeshRings(m, fr, R, 5, 6, 3);
                }
            else
                {
                r = Ring(r, sp, 0, Sc(30 + hw, b.ws), Sc(22, b.ws), 0, 0);              // crotch
                r = Ring(r, sp, 55, Sc(45 + hw, b.ws), Sc(28, b.ws), -3, 1);            // hips / seat
                r = Ring(r, sp, 135, Sc(40 + wa + hw / 2, b.ws), Sc(26 + be, b.ws), 1 + be / 2, 6);   // belt
                r = Ring(r, sp, 170, Sc(39 + wa, b.ws), Sc(26 + be, b.ws), 1 + be / 2, 1);   // waist
                r = Ring(r, sp, 260, Sc(42 + wa / 2, b.ws), Sc(28 + be / 2, b.ws), 2, 1);    // lower ribs
                r = Ring(r, sp, 360, Sc(48, b.ws), Sc(31 + cd, b.ws), 4 + cd / 2, 1);      // chest
                r = Ring(r, sp, 445, Sc(56, b.ws), Sc(28, b.ws), 1, 1);                   // shoulders
                r = Ring(r, sp, 495, Sc(42, b.ws), Sc(22, b.ws), -2, 1);                  // trapezius
                r = Ring(r, sp, 512, Sc(18, b.ws), Sc(15, b.ws), -2, 1);                  // neck base
                iR->MeshRings(m, fr, R, 9, 8, 3);
                }
            }
        int waistY, waistZ;
        ChainPoint(sp, 150, waistY, waistZ);
        if (b.skirt)
            {
            int hemY = waistY - Sc(b.skirt + 20, b.hs);
            if (hemY < 20) hemY = 20;
            int lon = b.skirt > 150;
            iR->MeshLimb(m, fr, 1, 0, waistY - 10, waistZ, 0, hemY, lon ? 6 : 2, Sc(31 + b.belly, b.ws), Sc(47 + b.hipW, b.ws),
                         Sc(lon ? 46 : 36, b.ws), Sc(lon ? 56 + b.hipW : 50 + b.hipW, b.ws), aLod ? 6 : 8, 0);
            }
        if (aKind == 7)
            {
            // a long tail curving out from under the robe, swaying as they walk
            int sw = (r3d_sin(fr * 64) * 18) >> 14;
            iR->MeshLimb(m, fr, 4, 0, waistY - Sc(30, b.hs), waistZ - Sc(22, b.ws), sw, waistY - Sc(80, b.hs), waistZ - Sc(60, b.ws),
                         Sc(8, b.ws), Sc(8, b.ws), Sc(6, b.ws), Sc(6, b.ws), aLod ? 3 : 5, 0);
            iR->MeshLimb(m, fr, 4, sw, waistY - Sc(80, b.hs), waistZ - Sc(60, b.ws), sw * 2, waistY - Sc(60, b.hs), waistZ - Sc(100, b.ws),
                         Sc(6, b.ws), Sc(6, b.ws), Sc(3, b.ws), Sc(3, b.ws), aLod ? 3 : 5, 2);
            }
        // neck + head
        int ndy, ndz;
        Seg(512 - lean / 2, Sc(22, b.hs), ndy, ndz);
        int neckTopY = topY + ndy, neckTopZ = topZ + ndz;
        int hs = b.headS;
        int hY = neckTopY + Sc(22, hs), hZ = neckTopZ + Sc(4, hs);
        if (bones)
            {
            // neck bones, the skull with deep eye sockets and a nose hole, the jaw hanging a little open
            iR->MeshLimb(m, fr, 0, 0, topY - 12, topZ - 2, 0, neckTopY, neckTopZ, 5, 5, 5, 5, 3, 0);
            iR->MeshEllipsoid(m, fr, 0, 0, hY + 2, hZ - 2, Sc(21, hs), Sc(24, hs), Sc(25, hs), aLod ? 4 : 7, aLod ? 2 : 4);
            if (!aLod)
                {
                iR->MeshEllipsoid(m, fr, 1, -Sc(9, hs), hY + Sc(2, hs), hZ + Sc(19, hs), Sc(7, hs), Sc(7, hs), Sc(4, hs), 5, 2);
                iR->MeshEllipsoid(m, fr, 1, Sc(9, hs), hY + Sc(2, hs), hZ + Sc(19, hs), Sc(7, hs), Sc(7, hs), Sc(4, hs), 5, 2);
                iR->MeshEllipsoid(m, fr, 1, 0, hY - Sc(8, hs), hZ + Sc(22, hs), Sc(3, hs), Sc(4, hs), Sc(3, hs), 4, 2);
                iR->MeshLimb(m, fr, 4, 0, hY - Sc(12, hs), hZ + Sc(6, hs), 0, hY - Sc(24, hs), hZ + Sc(20, hs), Sc(15, hs), Sc(8, hs), Sc(10, hs), Sc(4, hs), 5, 2);
                }
            }
        if (!aLod && !bones)
            iR->MeshLimb(m, fr, 2, 0, topY - 12, topZ - 2, 0, neckTopY, neckTopZ, Sc(15, b.ws), Sc(14, b.ws), Sc(12, b.ws), Sc(12, b.ws), 5, 0);
        // one head shape; hair / beard / helmet are painted onto its faces (no overlapping shells -> no flicker)
        int f0 = m.nf;
        int hb = fr == 0 ? m.nv : iR->iMeshCursor;          // first head vertex of this frame
        if (bones) {}
        else if (aLod) iR->MeshEllipsoid(m, fr, 2, 0, hY - 4, hZ, Sc(22, hs), Sc(30, hs), Sc(25, hs), 5, 3);
        else iR->MeshEllipsoid(m, fr, 2, 0, hY, hZ, Sc(22, hs), Sc(28, hs), Sc(25, hs), 8, 5);
        if (fr == 0 && !bones) PaintHead(m, f0, m.nf, hY, hZ, hs, aKind);
        if (!aLod && !bones)
            {
            // sculpt the head: hair stands off the skull, the jaw narrows, the chin comes forward
            short* hv = m.v + (fr * m.vstride + hb) * 3;
            int k, hn = 8 * 5 + 2;
            for (k = 0; k < hn; k++)
                {
                short* q = hv + k * 3;
                int dx = q[0], dy = q[1] - hY, dz = q[2] - hZ;
                int ly = dy * 256 / hs, lz = dz * 256 / hs;
                int hair = b.helmet ? (ly > 2 || (lz < -4 && ly > -14)) : (ly > 6 || (lz < -6 && ly > -18) || (b.longHair && lz < 0 && ly > -24));
                if (hair) { q[0] = (short)(dx * 9 / 8); q[1] = (short)(hY + dy * 9 / 8 + 2); q[2] = (short)(hZ + dz * 9 / 8 - (lz < 0 ? 3 : 0)); }
                else if (ly < -6)
                    {
                    q[0] = (short)(dx * 13 / 16);                                  // jaw
                    if (lz > 8) q[2] = (short)(hZ + dz + Sc(3, hs));               // chin
                    }
                else if (ly < 6 && r3d_abs(dx) > Sc(14, hs) && lz > 0) q[0] = (short)(dx * 17 / 16);   // cheekbones
                }
            // nose with a bridge, eyes, brows, ears (cat-folk: a short muzzle and tall pointed ears)
            if (aKind == 7)
                {
                iR->MeshEllipsoid(m, fr, 2, 0, hY - Sc(6, hs), hZ + Sc(20, hs), Sc(12, hs), Sc(9, hs), Sc(11, hs), 6, 3);
                iR->MeshEllipsoid(m, fr, 5, 0, hY - Sc(2, hs), hZ + Sc(31, hs), Sc(4, hs), Sc(3, hs), Sc(3, hs), 4, 2);
                int e;
                for (e = -1; e <= 1; e += 2)
                    iR->MeshLimb(m, fr, 4, e * Sc(14, hs), hY + Sc(20, hs), hZ - Sc(3, hs), e * Sc(18, hs), hY + Sc(44, hs), hZ - Sc(6, hs),
                                 Sc(8, hs), Sc(4, hs), 1, 1, 4, 1);
                }
            else
            iR->MeshLimb(m, fr, 2, 0, hY + Sc(6, hs), hZ + Sc(22, hs), 0, hY - Sc(6, hs), hZ + Sc(31, hs), Sc(4, hs), Sc(4, hs), Sc(5, hs), Sc(4, hs), 4, 2);
            iR->MeshLimb(m, fr, 5, -Sc(9, hs), hY + Sc(5, hs), hZ + Sc(19, hs), -Sc(9, hs), hY + Sc(5, hs), hZ + Sc(25, hs), Sc(4, hs), Sc(3, hs), Sc(3, hs), Sc(2, hs), 3, 2);
            iR->MeshLimb(m, fr, 5, Sc(9, hs), hY + Sc(5, hs), hZ + Sc(19, hs), Sc(9, hs), hY + Sc(5, hs), hZ + Sc(25, hs), Sc(4, hs), Sc(3, hs), Sc(3, hs), Sc(2, hs), 3, 2);
            iR->MeshLimb(m, fr, 4, -Sc(15, hs), hY + Sc(11, hs), hZ + Sc(20, hs), -Sc(3, hs), hY + Sc(10, hs), hZ + Sc(24, hs), Sc(2, hs), Sc(2, hs), Sc(2, hs), Sc(2, hs), 3, 3);
            iR->MeshLimb(m, fr, 4, Sc(15, hs), hY + Sc(11, hs), hZ + Sc(20, hs), Sc(3, hs), hY + Sc(10, hs), hZ + Sc(24, hs), Sc(2, hs), Sc(2, hs), Sc(2, hs), Sc(2, hs), 3, 3);
            if (!b.helmet && aKind != 7)
                {
                iR->MeshEllipsoid(m, fr, 2, -Sc(22, hs), hY + Sc(1, hs), hZ - Sc(2, hs), Sc(4, hs), Sc(9, hs), Sc(6, hs), 4, 2);
                iR->MeshEllipsoid(m, fr, 2, Sc(22, hs), hY + Sc(1, hs), hZ - Sc(2, hs), Sc(4, hs), Sc(9, hs), Sc(6, hs), 4, 2);
                }
            if (b.beard)
                {
                // full beard under the jaw and a moustache
                iR->MeshLimb(m, fr, 4, 0, hY - Sc(12, hs), hZ + Sc(16, hs), 0, hY - Sc(40, hs), hZ + Sc(22, hs), Sc(17, hs), Sc(12, hs), Sc(7, hs), Sc(5, hs), 6, 2);
                iR->MeshLimb(m, fr, 4, -Sc(11, hs), hY - Sc(9, hs), hZ + Sc(27, hs), Sc(11, hs), hY - Sc(9, hs), hZ + Sc(27, hs), Sc(3, hs), Sc(3, hs), Sc(3, hs), Sc(3, hs), 3, 3);
                }
            if (b.longHair)
                iR->MeshLimb(m, fr, 4, 0, hY - Sc(4, hs), hZ - Sc(19, hs), 0, hY - Sc(64, hs), hZ - Sc(28, hs), Sc(11, hs), Sc(21, hs), Sc(8, hs), Sc(22, hs), 6, 2);
            }
        // arms: shoulder -> elbow -> wrist, lofted with deltoid / biceps / forearm shape
        int shY, shZ;
        ChainPoint(sp, 430, shY, shZ);
        int armSkin = aKind == 2 ? 3 : 2;
        for (s = 0; s < 2; s++)
            {
            int UA = Sc(76, b.armL), FA = Sc(66, b.armL);
            Chain c;
            ChainInit(c, s ? SW : -SW, shY, shZ, 1);
            ChainBone(c, shA[s], UA);
            ChainBone(c, shA[s] + elb[s], FA);
            int* r = R;
            if (bones)
                {
                iR->MeshLimb(m, fr, 0, c.x, c.y[0], c.z[0], c.x, c.y[1], c.z[1], 6, 6, 4, 4, aLod ? 3 : 4, 0);
                iR->MeshLimb(m, fr, 0, c.x, c.y[1], c.z[1], c.x, c.y[2], c.z[2], 4, 4, 3, 3, aLod ? 3 : 4, 0);
                }
            else if (aLod)
                {
                r = Ring(r, c, 0, Sc(14, b.ws), Sc(15, b.ws), 0, 1);
                r = Ring(r, c, 256, Sc(9, b.ws), Sc(10, b.ws), 0, armSkin);
                r = Ring(r, c, 512, Sc(7, b.ws), Sc(6, b.ws), 0, armSkin);
                iR->MeshRings(m, fr, R, 3, 4, 0);
                }
            else
                {
                r = Ring(r, c, 0, Sc(13, b.ws), Sc(14, b.ws), 0, 1);
                r = Ring(r, c, 45, Sc(15, b.ws), Sc(16, b.ws), 1, 1);       // deltoid
                r = Ring(r, c, 140, Sc(11, b.ws), Sc(13, b.ws), 1, 1);      // biceps
                r = Ring(r, c, 236, Sc(10, b.ws), Sc(10, b.ws), 0, armSkin);    // sleeve end
                r = Ring(r, c, 320, Sc(10, b.ws), Sc(11, b.ws), 0, armSkin);    // forearm
                r = Ring(r, c, 420, Sc(8, b.ws), Sc(8, b.ws), 0, armSkin);
                r = Ring(r, c, 505, Sc(7, b.ws), Sc(6, b.ws), 0, armSkin);      // wrist
                iR->MeshRings(m, fr, R, 7, 5, 3);
                }
            int wY = c.y[2], wZ = c.z[2];
            int hdy, hdz;
            Seg(shA[s] + elb[s], Sc(22, b.armL), hdy, hdz);
            if (bones) iR->MeshLimb(m, fr, 5, c.x, wY, wZ, c.x, wY + hdy, wZ + hdz, 6, 3, 5, 2, 3, 2);      // bony fingers
            else iR->MeshLimb(m, fr, 2, c.x, wY, wZ, c.x, wY + hdy, wZ + hdz, Sc(9, b.ws), Sc(6, b.ws), Sc(7, b.ws), Sc(4, b.ws), aLod ? 3 : 4, 2);
            if (!aLod && !bones)
                {
                // thumb, towards the body and forward
                int tdy, tdz;
                Seg(shA[s] + elb[s] + 110, Sc(13, b.armL), tdy, tdz);
                int tx = s ? c.x - 6 : c.x + 6;
                iR->MeshLimb(m, fr, 2, tx, wY + hdy / 4, wZ + hdz / 4, tx + (s ? -3 : 3), wY + hdy / 4 + tdy, wZ + hdz / 4 + tdz, 4, 3, 3, 2, 3, 2);
                }
            if (s == 1 && armed)
                {
                int gx = c.x, gy = wY + hdy / 2, gz = wZ + hdz / 2;
                if (b.weapon == 1)
                    {
                    int bdy, bdz;
                    Seg(shA[s] + elb[s] + 256 + 20, 250, bdy, bdz);
                    iR->MeshLimb(m, fr, 3, gx, gy, gz, gx, gy + bdy, gz + bdz, 7, 2, 4, 1, 4, 2);
                    iR->MeshLimb(m, fr, 6, gx - 22, gy, gz, gx + 22, gy, gz, 5, 5, 5, 5, 4, 3);
                    }
                else
                    {
                    int bdy, bdz;
                    Seg(shA[s] + elb[s] + 220, 300, bdy, bdz);
                    iR->MeshLimb(m, fr, 6, gx, gy, gz, gx, gy + bdy, gz + bdz, 14, 14, 34, 30, 6, 2);
                    }
                }
            if (s == 1 && b.weapon == 3)
                {
                int gx = c.x, gz = wZ + hdz / 2;
                iR->MeshLimb(m, fr, 6, gx, 0, gz, gx, wY + hdy / 2 + 140, gz, 7, 7, 6, 6, 4, 2);
                }
            }
        if (b.weapon == 4)
            {
            // sword sheathed on the left hip: scabbard, guard, grip
            int x = -Sc(46, b.ws), y = waistY - 6, z = waistZ + 10;
            iR->MeshLimb(m, fr, 5, x, y, z, x - 6, y - 165, z - 78, 6, 9, 4, 6, 4, 2);
            iR->MeshLimb(m, fr, 3, x, y, z - 12, x, y + 4, z + 12, 4, 4, 4, 4, 3, 3);
            iR->MeshLimb(m, fr, 6, x, y, z, x + 1, y + 40, z + 14, 4, 4, 4, 4, 3, 2);
            }
        }
    RotateFrame(m, 0, HF_DEAD, -256, Sc(32, KBody[aKind].ws));
    if (aKind == 8)
        {
        // a skeleton does not fall like a man: it collapses into a heap of bones where it stood
        const short* a = m.v;
        short* d = m.v + HF_DEAD * m.vstride * 3;
        int i;
        for (i = 0; i < m.nv; i++)
            {
            int x = a[i * 3], y = a[i * 3 + 1], z = a[i * 3 + 2];
            int fall = y > 250 ? (y - 250) / 2 : 0;                     // the upper body tips forward onto the rest
            d[i * 3] = (short)(x * 3 / 2 + ((y * 7) & 31) - 16);
            d[i * 3 + 1] = (short)(5 + y / 9 + (x > 0 ? 2 : 0));
            d[i * 3 + 2] = (short)(z + fall - 40);
            }
        }
    iR->MeshFinish(m);
    if (!aLod) iR->MeshSmooth(m);
    }

// wolf: slots 0 fur, 1 dark fur, 2 light (muzzle/belly)
void Game::BuildWolf(Mesh& m)
    {
    iR->MeshInit(m, 200, 250, 17);
    int fr;
    for (fr = 0; fr < 17; fr++)
        {
        iR->iMeshCursor = 0;
        int p = (fr & 15) * 64;
        int bob = (r3d_abs(r3d_sin(p)) * 10) >> 14;
        int y = 126 + bob;
        iR->MeshEllipsoid(m, fr, 0, 0, y, -16, 38, 38, 92, 6, 3);          // body
        iR->MeshEllipsoid(m, fr, 0, 0, y + 8, 58, 44, 48, 46, 6, 3);       // chest/shoulders (thick ruff)
        iR->MeshLimb(m, fr, 0, 0, y + 16, 84, 0, y + 44, 120, 28, 26, 22, 20, 5, 0);   // neck
        iR->MeshEllipsoid(m, fr, 0, 0, y + 50, 130, 27, 24, 28, 6, 3);      // head
        iR->MeshLimb(m, fr, 2, 0, y + 44, 148, 0, y + 34, 194, 16, 14, 8, 7, 5, 2);    // snout
        iR->MeshLimb(m, fr, 1, -13, y + 66, 124, -17, y + 90, 120, 8, 7, 1, 1, 3, 0);  // ears
        iR->MeshLimb(m, fr, 1, 13, y + 66, 124, 17, y + 90, 120, 8, 7, 1, 1, 3, 0);
        // legs: front pair and back pair, diagonal gait
        int k;
        for (k = 0; k < 4; k++)
            {
            int front = k < 2;
            int side = (k & 1) ? 1 : -1;
            int ph = (front ? 0 : 512) + (side > 0 ? 512 : 0);
            int a = (r3d_sin(p + ph) * 100) >> 14;
            int lift = ((r3d_sin(p + ph + 256) + 16384) * 50) >> 15;
            int up = front ? a - 10 : a + 60;
            int lo = front ? a + 10 + lift : a - 80 + lift;
            int x = side * 22, z = front ? 66 : -76, top = front ? y - 8 : y - 4;
            int dy, dz;
            int L1 = front ? 52 : 56, L2 = 62;
            Seg(up, L1, dy, dz);
            int ky = top + dy, kz = z + dz;
            Seg(lo, L2, dy, dz);
            int py = ky + dy, pz = kz + dz;
            if (py < 6) py = 6;
            iR->MeshLimb(m, fr, 0, x, top, z, x, ky, kz, 18, 16, 12, 11, 5, 0);
            iR->MeshLimb(m, fr, 1, x, ky, kz, x, py, pz, 11, 10, 8, 8, 5, 2);
            }
        int sw = (r3d_sin(p * 2) * 30) >> 14;
        iR->MeshLimb(m, fr, 0, 0, y + 14, -100, sw, y - 30, -170, 15, 14, 12, 11, 5, 2);   // bushy tail
        }
    RotateFrame(m, 0, 16, 0, -96);
    iR->MeshFinish(m);
    iR->MeshSmooth(m);
    }

// dragon: slots 0 scales, 1 belly/horn, 2 membrane, 3 dark (claws, jaw)
void Game::BuildDragon(Mesh& m)
    {
    iR->MeshInit(m, 250, 330, 25);
    int fr;
    for (fr = 0; fr < 25; fr++)
        {
        iR->iMeshCursor = 0;
        int flying = fr < 16;
        int p = (fr & 15) * 64;
        int flap = flying ? ((r3d_sin(p) * 110) >> 14) : 210;
        int span = flying ? 256 : 120;
        int sway = flying ? ((r3d_sin(p + 256) * 50) >> 14) : ((r3d_sin(p * 2) * 30) >> 14);
        int by = 300;
        iR->MeshEllipsoid(m, fr, 0, 0, by, 0, 270, 230, 580, 8, 3);       // body
        // neck chain
        int ny = by + 80, nz = 430;
        int k;
        int ang = 512 - 130 - sway;
        for (k = 0; k < 3; k++)
            {
            int dy, dz;
            int a = ang + k * 60;
            Seg(a, 240, dy, dz);
            int r0 = 150 - k * 22, r1 = 128 - k * 22;
            iR->MeshLimb(m, fr, 0, 0, ny, nz, 0, ny + dy, nz + dz, r0, r0, r1, r1, 5, 0);
            ny += dy; nz += dz;
            }
        // head, snout, jaw, horns
        iR->MeshEllipsoid(m, fr, 0, 0, ny + 20, nz + 30, 82, 72, 100, 6, 3);
        iR->MeshLimb(m, fr, 0, 0, ny + 30, nz + 90, 0, ny + 10, nz + 250, 56, 50, 30, 26, 5, 2);
        iR->MeshLimb(m, fr, 3, 0, ny - 30, nz + 80, 0, ny - 50 - (flying ? 0 : 30), nz + 230, 36, 30, 20, 16, 5, 2);
        iR->MeshLimb(m, fr, 1, -50, ny + 70, nz - 20, -80, ny + 170, nz - 150, 20, 18, 2, 2, 3, 0);
        iR->MeshLimb(m, fr, 1, 50, ny + 70, nz - 20, 80, ny + 170, nz - 150, 20, 18, 2, 2, 3, 0);
        // tail chain
        int ty = by + 20, tz = -480;
        for (k = 0; k < 4; k++)
            {
            int dy, dz;
            int a = -230 - k * 10 + (flying ? sway * (k + 1) / 2 : 0);
            Seg(a, 430, dy, dz);
            int r0 = 140 - k * 32, r1 = 110 - k * 30;
            if (r1 < 12) r1 = 12;
            iR->MeshLimb(m, fr, 0, 0, ty, tz, 0, ty + dy, tz + dz, r0, r0, r1, r1, 5, k == 3 ? 2 : 0);
            ty += dy; tz += dz;
            }
        // legs (one tapered segment each)
        for (k = 0; k < 4; k++)
            {
            int side = (k & 1) ? 1 : -1;
            int front = k < 2;
            int a = flying ? -200 : ((r3d_sin(p * 2 + (k * 256)) * 80) >> 14);
            int dy, dz;
            Seg(a, 300, dy, dz);
            int x = side * 170, y0 = by - 60, z0 = front ? 320 : -300;
            iR->MeshLimb(m, fr, 3, x, y0, z0, x + side * 20, y0 + dy, z0 + dz, 70, 64, 40, 36, 5, 2);
            }
        // wings: bone + two membrane panels per side, both faces visible
        int side;
        for (side = -1; side <= 1; side += 2)
            {
            int rootY = by + 80;
            int c1 = r3d_cos(flap), s1 = r3d_sin(flap);
            int L1 = (900 * span) >> 8, L2 = (1100 * span) >> 8;
            int ex = side * (200 + ((L1 * c1) >> 14)), ey = rootY + ((L1 * s1) >> 14);
            int f2 = flap + (flying ? flap / 4 : 40);
            int tx = ex + side * ((L2 * r3d_cos(f2)) >> 14), tyy = ey + ((L2 * r3d_sin(f2)) >> 14);
            iR->MeshLimb(m, fr, 0, side * 200, rootY, 260, ex, ey, 160, 34, 30, 22, 20, 5, 0);
            int mx = side * 200 + (ex - side * 200) * 3 / 5, my = rootY + (ey - rootY) * 3 / 5;
            int inner[12] = { side * 200, rootY, 300,  ex, ey, 160,  mx, my - 10, -480,  side * 200, rootY - 10, -380 };
            int outer[12] = { ex, ey, 160,  tx, tyy, -200,  ex + (tx - ex) * 2 / 3, ey + (tyy - ey) * 2 / 3 - 20, -640,  mx, my - 10, -480 };
            iR->MeshQuad(m, fr, 2, inner, 0, rootY - 600, 0);
            iR->MeshQuad(m, fr, 2, inner, 0, rootY + 600, 0);
            iR->MeshQuad(m, fr, 2, outer, 0, rootY - 600, 0);
            iR->MeshQuad(m, fr, 2, outer, 0, rootY + 600, 0);
            }
        }
    RotateFrame(m, 16, 24, 30, -230);
    iR->MeshFinish(m);
    iR->MeshSmooth(m);
    }

// horse: slots 0 coat, 1 mane/tail, 2 hooves, 3 muzzle/blaze, 4 eyes; 16 idle frames (tail swish, head bob)
static int* HRing(int* r, int x, int y, int z, int ra, int rb, int slot)
    {
    // ring around the z axis (body runs along z): side axis x, second axis y
    r[0] = x; r[1] = y; r[2] = z; r[3] = 16384; r[4] = 0; r[5] = 0; r[6] = 0; r[7] = 16384; r[8] = 0;
    r[9] = ra; r[10] = rb; r[11] = slot;
    return r + 12;
    }

void Game::BuildHorse(Mesh& m)
    {
    const int C = METER / 100;         // ~2.56 units per cm
    (void)C;
    iR->MeshInitSmooth(m, 320, 640, 16);
    int fr;
    for (fr = 0; fr < 16; fr++)
        {
        iR->iMeshCursor = 0;
        int p = fr * 64;
        int sw = r3d_sin(p), cw = r3d_cos(p);
        int tail = (sw * 30) >> 14;                     // tail swish (units)
        int nod = ((r3d_sin(p * 2) + 16384) * 10) >> 15;   // head bob
        int R[12 * 8];
        int* r = R;
        // barrel of the body along z (rump at -z, chest at +z), withers ~1.55 m
        r = HRing(r, 0, 300, -250, 30, 40, 0);
        r = HRing(r, 0, 302, -215, 60, 64, 0);           // haunch
        r = HRing(r, 0, 300, -120, 64, 72, 0);
        r = HRing(r, 0, 296, 0, 62, 76, 0);              // belly
        r = HRing(r, 0, 310, 120, 58, 74, 0);            // chest
        r = HRing(r, 0, 322, 180, 44, 56, 0);
        r = HRing(r, 0, 330, 200, 20, 26, 0);
        iR->MeshRings(m, fr, R, 7, 8, 3);
        // neck rising forward, head angled down
        int hx = 0, hy = 470 - nod, hz = 300 + nod / 2;
        iR->MeshLimb(m, fr, 0, 0, 330, 170, hx, hy, hz - 20, 50, 34, 34, 24, 6, 0);
        iR->MeshLimb(m, fr, 1, 0, 380, 150, hx, hy + 18, hz - 40, 10, 8, 8, 6, 4, 0);     // mane
        iR->MeshLimb(m, fr, 0, hx, hy + 6, hz - 14, hx, hy - 100, hz + 76, 30, 26, 18, 16, 6, 2);   // head
        iR->MeshLimb(m, fr, 3, hx, hy - 70, hz + 52, hx, hy - 104, hz + 80, 17, 15, 15, 13, 5, 2);  // muzzle
        iR->MeshLimb(m, fr, 0, hx - 14, hy + 10, hz - 10, hx - 18, hy + 42, hz - 16, 7, 4, 3, 2, 3, 2);   // ears
        iR->MeshLimb(m, fr, 0, hx + 14, hy + 10, hz - 10, hx + 18, hy + 42, hz - 16, 7, 4, 3, 2, 3, 2);
        iR->MeshEllipsoid(m, fr, 4, hx - 24, hy - 20, hz + 14, 6, 6, 6, 4, 2);
        iR->MeshEllipsoid(m, fr, 4, hx + 24, hy - 20, hz + 14, 6, 6, 6, 4, 2);
        // legs: forearm/cannon/hoof in front, gaskin/hock/cannon/hoof behind; weight shifts slowly
        int s;
        for (s = 0; s < 2; s++)
            {
            int x = s ? 34 : -34;
            int lift = s ? (cw > 0 ? (cw * 6) >> 14 : 0) : 0;
            // front
            iR->MeshLimb(m, fr, 0, x, 280, 140, x, 150, 150, 26, 30, 15, 17, 6, 0);
            iR->MeshLimb(m, fr, 0, x, 150, 150, x, 30 + lift, 148, 13, 13, 11, 11, 5, 0);
            iR->MeshLimb(m, fr, 2, x, 30 + lift, 148, x, lift, 152, 14, 15, 17, 18, 5, 3);
            // hind
            iR->MeshLimb(m, fr, 0, x, 300, -190, x, 170, -230, 34, 40, 18, 20, 6, 0);
            iR->MeshLimb(m, fr, 0, x, 170, -230, x, 30, -205, 12, 13, 11, 11, 5, 0);
            iR->MeshLimb(m, fr, 2, x, 30, -205, x, 0, -202, 14, 15, 17, 18, 5, 3);
            }
        // tail
        iR->MeshLimb(m, fr, 1, 0, 320, -262, tail / 2, 250, -300, 12, 12, 10, 10, 4, 0);
        iR->MeshLimb(m, fr, 1, tail / 2, 250, -300, tail, 120, -296, 10, 10, 16, 14, 5, 2);
        }
    iR->MeshFinish(m);
    iR->MeshSmooth(m);
    }
