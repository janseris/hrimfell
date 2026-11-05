// the inventory preview of items: invshot out.ppm ID [ID ...] (items.h numbers), one picture of 640x200 per item: out_N.ppm
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
    g->iUi = UI_PLAY; g->iFadeUs = 0; g->ShowFps = 1;
    for (int s = 0; s < 5; s++) emu_tick(m, 33000, 0);
    for (int a = 2; a < argc; a++)
        {
        int id = atoi(argv[a]);
        g->iS.ninv = 0; g->AddItem(id, 1);
        g->iUi = UI_INV; g->iInvCat = 1; g->iInvCol = 1; g->iInvSel = 0; g->iInvScroll = 0; g->iPreviewAngle = 100;
        emu_render(m, out);
        char fn[256]; sprintf(fn, "%s_%d.ppm", argv[1], id);
        FILE* f = fopen(fn, "wb"); fprintf(f, "P6 640 200 255\n");
        for (int i = 0; i < 640 * 200; i++) { u16 q = out[i]; unsigned char c[3] = { (unsigned char)((q >> 8) & 0xF8), (unsigned char)((q >> 3) & 0xFC), (unsigned char)((q << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
        fclose(f);
        printf("%d: kit %d\n", id, g->iItemKit[id]);
        }
    return 0;
    }
