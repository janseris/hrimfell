// host harness: renders benchmark frames to PPM and measures host time per mode
#include "r3d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static double now() { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
static void save(const char* fn, u16* px)
    {
    FILE* f = fopen(fn, "wb"); fprintf(f, "P6 640 200 255\n");
    for (int i = 0; i < 640 * 200; i++) { u16 p = px[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
    fclose(f);
    }
int main(int argc, char** argv)
    {
    int n = R3D::MemNeeded();
    unsigned char* mem = (unsigned char*)calloc(1, n);
    R3D* r = (R3D*)calloc(1, sizeof(R3D));
    r->Init(mem);
    static u16 out[640 * 200];
    printf("mem %d bytes, objects %d\n", n, r->iObjCount);
    const char* only = argc > 1 ? argv[1] : 0;
    for (int m = 0; m < 3; m++)
        {
        r->StartBenchmark();
        r->iBenchRun = m; r->iMode = m;
        // walk along the benchmark path, step 100 ms; screenshots at fixed times
        double t0 = now(); int frames = 0; long tris = 0, pix = 0;
        for (int step = 0; step < 90; step++)
            {
            r->Tick(100000, 0);
            r->Render(out, 640);
            frames++; tris += r->iTris; pix += r->iPixels;
            if (step == 10 || step == 45 || step == 80)
                {
                char fn[64]; sprintf(fn, "/home/claude/bench/shots/m%d_%02d.ppm", m, step); save(fn, out);
                }
            }
        double t = now() - t0;
        printf("mode %d: host %.2f ms/frame, avg tris %ld, avg pixels %ld\n", m, t * 1000 / frames, tris / frames, pix / frames);
        }
    return 0;
    }
