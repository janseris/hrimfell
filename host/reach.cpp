#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
int GameKitOutFloor(const Game& g, int aX, int aZ, int aFeetY, int aOff, int* aFound);
// where you can walk outside from a Skyrim point (x, y, feet z): a grid of 50 cm over a box (Skyrim units round the
// start), each square reached at which height -> a ppm (x east, y north up) and a count
static int WorldX(double aX) { return (int)((aX + 233472) * 3.6576); }
static int WorldZ(double aY) { return (int)((aY + 176128) * 3.6576); }
int main(int argc, char** argv)
 { int n=Game::MemNeeded(); unsigned char* mem=(unsigned char*)calloc(1,n); Game* g=(Game*)calloc(1,sizeof(Game)); g->Init(mem); g->NewGame();
 double sx = atof(argv[1]), sy = atof(argv[2]), sz = atof(argv[3]);
 double bx0 = atof(argv[4]), by0 = atof(argv[5]), bx1 = atof(argv[6]), by1 = atof(argv[7]);
 R3D& r = *g->iR; int off = r.iLandOffset;
 const int S = getenv("RS") ? atoi(getenv("RS")) : METER / 2;
 int X0 = WorldX(bx0), Z0 = WorldZ(by0), NX = (WorldX(bx1) - X0) / S, NZ = (WorldZ(by1) - Z0) / S;
 std::vector<int> feet(NX * NZ, -1 << 30);
 std::vector<int> q;
 r.FocusLand(WorldX(sx), WorldZ(sy));
 int ix = (WorldX(sx) - X0) / S, iz = (WorldZ(sy) - Z0) / S;
 int f0 = g->FloorAt(0, X0 + ix * S, Z0 + iz * S, (int)(sz * 1.42875 * 256 / 100) + off + METER / 4);
 feet[iz * NX + ix] = f0; q.push_back(iz * NX + ix);
 const int R = 100, H = 7 * METER / 4;
 for (size_t k = 0; k < q.size(); k++)
  {
  int c = q[k], cx = c % NX, cz = c / NX, f = feet[c];
  for (int d = 0; d < 8; d++)
   {
   static const int DX[8] = { 1, -1, 0, 0, 1, 1, -1, -1 }, DZ[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };
   int nx = cx + DX[d], nz = cz + DZ[d];
   if (nx < 0 || nz < 0 || nx >= NX || nz >= NZ) continue;
   // in four steps, the feet following the floor as the game's movement does
   int x = X0 + cx * S, z = Z0 + cz * S, ff = f, ok = 1;
   for (int s = 1; s <= 4 && ok; s++)
    {
    int px = x + DX[d] * S * s / 4, pz = z + DZ[d] * S * s / 4;
    int nf = g->FloorAt(0, px, pz, ff), run = S / 4 * (DX[d] && DZ[d] ? 141 : 100) / 100;
    int climb = nf - ff < run * 6 / 5 || nf < ff;
    if (!climb && nf - ff < 45 * METER / 100) { int found = 0; GameKitOutFloor(*g, px, pz, ff, off, &found); if (found) climb = 1; }
    if (!climb || g->BlockedAt(0, px, pz, R, ff + 7 * METER / 25, H - 7 * METER / 25)) { ok = 0; break; }
    ff = nf;
    }
   if (!ok) continue;
   int& t = feet[nz * NX + nx];
   if (t > ff - METER) continue;
   if (t == (-1 << 30)) q.push_back(nz * NX + nx); else q.push_back(nz * NX + nx);
   t = ff;
   if (q.size() > 3000000) break;
   }
  }
 int cnt = 0; for (int i = 0; i < NX * NZ; i++) if (feet[i] != (-1 << 30)) cnt++;
 FILE* fo = fopen(argv[8], "wb"); fprintf(fo, "P6 %d %d 255\n", NX, NZ);
 double lo = atof(argv[9]), hi = atof(argv[10]);
 for (int zz = NZ - 1; zz >= 0; zz--) for (int xx = 0; xx < NX; xx++)
  {
  int fv = feet[zz * NX + xx]; unsigned char c3[3] = { 0, 0, 0 };
  if (fv != (-1 << 30)) { double z = (fv - off) / 256.0 * 100 / 1.42875; double v = (z - lo) / (hi - lo); if (v < 0) v = 0; if (v > 1) v = 1; c3[0] = (unsigned char)(60 + 195 * v); c3[1] = 200; c3[2] = (unsigned char)(255 - 195 * v); }
  else { int gg = r.GroundAt(X0 + xx * S, Z0 + zz * S); double z = (gg - off) / 256.0 * 100 / 1.42875; int v = (int)((z - lo) / (hi - lo) * 60); if (v < 0) v = 0; if (v > 60) v = 60; c3[0] = c3[1] = c3[2] = (unsigned char)(20 + v); }
  fwrite(c3, 1, 3, fo);
  }
 fclose(fo);
 printf("reached %d squares of %d (start feet %.0f)\n", cnt, NX * NZ, (f0 - off) / 256.0 * 100 / 1.42875);
 for (int a = 11; a + 1 < argc; a += 2) { int px = (WorldX(atof(argv[a])) - X0) / S, pz = (WorldZ(atof(argv[a + 1])) - Z0) / S; int fv = feet[pz * NX + px]; printf("  %s %s: %s %.0f\n", argv[a], argv[a + 1], fv != (-1 << 30) ? "reached, feet" : "NOT reached", fv != (-1 << 30) ? (fv - off) / 256.0 * 100 / 1.42875 : 0.0); }
 return 0; }
