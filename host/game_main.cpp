// host harness: scripted scenes -> screenshots
#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static u16 out[640 * 200];
static Game* g;
static void save(const char* name)
    {
    char fn[128]; sprintf(fn, "/home/claude/game/shots/%s.ppm", name);
    FILE* f = fopen(fn, "wb"); fprintf(f, "P6 640 200 255\n");
    for (int i = 0; i < 640 * 200; i++) { u16 p = out[i]; unsigned char c[3] = { (unsigned char)((p >> 8) & 0xF8), (unsigned char)((p >> 3) & 0xFC), (unsigned char)((p << 3) & 0xF8) }; fwrite(c, 1, 3, f); }
    fclose(f);
    }
static void run(int frames, u32 keys) { for (int i = 0; i < frames; i++) { g->Tick(66000, keys); g->Render(out, 640); } }
static void place(int x, int z, int yawTowardX, int yawTowardZ, int pitch)
    {
    g->iS.px = x; g->iS.pz = z; g->iS.py = g->iR->GroundAt(x, z) + 410;
    g->iS.yaw = r3d_atan2(yawTowardX - x, yawTowardZ - z); g->iS.pitch = pitch;
    }
int main(int argc, char** argv)
    {
    int n = Game::MemNeeded();
    unsigned char* mem = (unsigned char*)calloc(1, n);
    g = (Game*)calloc(1, sizeof(Game));
    g->Init(mem);
    printf("mem %d + game %d bytes, statics %d, ents %d\n", n, (int)sizeof(Game), g->iR->iObjCount, g->iS.ne);
    run(3, 0); save("01_title");
    g->KeyChar('e'); run(3, 0); save("02_start");
    // face the elder (entity 0) and talk
    Ent& el = g->iS.e[0];
    place(el.x + 2 * METER, el.z - 2 * METER, el.x, el.z, -20);
    run(2, 0); printf("focus kind %d idx %d\n", g->iFocusKind, g->iFocus);
    g->KeyChar('e'); run(1, 0); save("03_dialog");
    for (int i = 0; i < 4; i++) { g->KeyChar('e'); run(1, 0); }
    printf("quest %d ui %d\n", g->iS.quest, g->iUi);
    save("04_after_dialog");
    g->KeyChar('i'); run(1, 0); save("05_inventory");
    g->KeyChar('e'); run(1, 0); run(1, KEY_LOOK_D); run(1, 0); save("06_inventory_item");
    for (int i = 0; i < 2; i++) { run(1, 0); run(1, KEY_LOOK_D); }
    run(1, 0); save("06b_inventory_item");
    g->KeyChar('i'); g->KeyChar('m'); run(1, 0); save("07_map");
    g->KeyChar('m'); run(1, 0);
    printf("ui %d\n", g->iUi);
    // village look
    place(62 * CELL, 40 * CELL, 62 * CELL, 46 * CELL, -30); run(2, 0); save("08_village");
    // wolves
    place(40 * CELL, 24 * CELL, 40 * CELL, 30 * CELL, -20);
    run(20, 0); save("09_wolves");
    run(4, KEY_FIRE); save("10_swing");
    run(20, KEY_FIRE);
    printf("hp after wolves %d\n", g->iS.hp >> 8);
    // word stone
    place(100 * CELL - 6 * METER, 40 * CELL - 8 * METER, 100 * CELL, 40 * CELL, -40);
    g->iS.hp = 100 << 8;
    run(3, 0); save("11_stone");
    // troll
    place(82 * CELL + 512, 72 * CELL, 82 * CELL, 78 * CELL, -20); run(6, 0); save("12_troll");
    // dragon
    g->iS.quest = 3; g->iS.bfb[BF_WORD] = 3; g->iS.hp = 100 << 8;
    place(92 * CELL, 88 * CELL, 100 * CELL, 98 * CELL, 40);
    run(30, 0); save("13_dragon_wake");
    run(30, 0);
    { Ent& d = g->iS.e[g->iDragonIdx]; g->iS.yaw = r3d_atan2(d.x - g->iS.px, d.z - g->iS.pz); g->iS.pitch = 100; printf("dragon state %d fly %d\n", d.state, d.fly >> 8); }
    run(1, 0); save("14_dragon_fly");
    g->iS.timeOfDay = 23 * 60 * 1000; g->iS.pitch = 120; run(2, 0); save("15_night");
    g->iS.timeOfDay = 19 * 60 * 1000; place(62 * CELL, 36 * CELL, 62 * CELL, 46 * CELL, 0); run(2, 0); save("16_dusk");
    g->KeyChar(27); run(1, 0); save("17_menu");
    printf("done\n");
    return 0;
    }
