// the world map at each zoom, centred on Riverwood: mapshot out_prefix
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
static u16 out[640 * 200];
int main(int argc, char** argv)
    {
    int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64);
    u8* m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
    emu_init(m, 0);
    Game* g = (Game*)m;
    emu_mode(m, 0);
    g->EnterCell(0, (int)((21900 + 233472) * 0.0142875 * METER), (int)((-45500 + 176128) * 0.0142875 * METER), 0);
    for (int s = 0; s < 5; s++) emu_tick(m, 33000, 0);
    for (int z = 0; z <= 4; z++)
        {
        g->iUi = UI_MAP; g->iMapLocal = 0; g->iMapWZ = z; g->iMapCX = g->iS.px; g->iMapCZ = g->iS.pz;
        emu_render(m, out);
        char fn[256]; sprintf(fn, "%s%d.ppm", argv[1], z);
        FILE* f = fopen(fn, "wb"); fprintf(f, "P6 640 200 255\n");
        for (int i = 0; i < 640 * 200; i++) { u16 q = out[i]; unsigned char c[3] = { (unsigned char)((q >> 8) & 0xF8), (unsigned char)((q >> 3) & 0xFC), (unsigned char)((q << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
        fclose(f);
        }
    return 0;
    }
