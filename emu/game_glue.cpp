// glue: run the whole game (phone GCC 2.9 -O2 build) inside Unicorn
#include "game.h"
int GameRwHousePos(int aKind, int* aX, int* aZ);
int GameBarrowOutX(int a, int b); int GameBarrowOutZ(int a, int b);
extern "C" void* memcpy(void* d, const void* s, unsigned int n)
    { u8* a = (u8*)d; const u8* b = (const u8*)s; while (n--) *a++ = *b++; return d; }
extern "C" void* memset(void* d, int v, unsigned int n) { r3d_memset(d, v, (int)n); return d; }
extern "C" int emu_memneeded() { return Game::MemNeeded() + sizeof(Game) + 64; }
// the land file, put in memory by the harness: ctx = { u8* next, int left, u8* start, int size, u8* area, int area size }
static int EmuLandRead(void* aCtx, u8* aBuf, int aLen)
    {
    int* c = (int*)aCtx;
    int n = aLen < c[1] ? aLen : c[1];
    const u8* p = (const u8*)c[0];
    int i;
    for (i = 0; i < n; i++) aBuf[i] = p[i];
    c[0] += n; c[1] -= n;
    return n;
    }
static int EmuLandSeek(void* aCtx, int aPos)
    {
    int* c = (int*)aCtx;
    if (aPos < 0 || aPos > c[3]) return 0;
    c[0] = c[2] + aPos; c[1] = c[3] - aPos;
    return 1;
    }
// the area's file (riverwood.har) after the land, and the items' models (items.hit) after it; nothing else
static int EmuDataRead(void* aCtx, const char* aName, int aPos, u8* aBuf, int aLen)
    {
    int* c = (int*)aCtx;
    int k = aName[0] == 'i' ? 6 : (aName[0] == 'r' ? 4 : (aName[0] == 'b' ? 8 : (aName[0] == 'c' ? 10 : (aName[0] == 'w' ? 12 : (aName[0] == 'f' ? 14 : -1)))));
    if (k < 0) return 0;
    if (aPos < 0 || aPos >= c[k + 1]) return 0;
    int n = aLen < c[k + 1] - aPos ? aLen : c[k + 1] - aPos, i;
    const u8* p = (const u8*)c[k] + aPos;
    for (i = 0; i < n; i++) aBuf[i] = p[i];
    return n;
    }
extern "C" void emu_init(u8* mem, int* land)
    {
    Game* g = (Game*)mem;
    if (land && land[1] > 0) { g->iLandRead = EmuLandRead; g->iLandSeek = EmuLandSeek; g->iLandCtx = land; }
    if (land && land[5] > 0) { g->iDataRead = EmuDataRead; g->iDataCtx = land; }
    g->Init(mem + ((sizeof(Game) + 63) & ~63)); g->iUi = UI_PLAY;
    }
extern "C" void emu_tick(u8* mem, int dt, int keys) { ((Game*)mem)->Tick(dt, keys); }
extern "C" void emu_render(u8* mem, u16* out) { ((Game*)mem)->Render(out, 640); }
extern "C" int emu_tris(u8* mem) { return ((Game*)mem)->iR->iTris; }
extern "C" void emu_mode(u8* mem, int m) { Game* g = (Game*)mem; g->iS.outMode = m; g->iR->SetOutputMode(m); }
// scenes: 0 village, 1 forest/wolves, 2 stone + wights, 3 dragon fight, 4 inventory
static int g_emuItem;
extern "C" void emu_scene(u8* mem, int s)
    {
    Game* g = (Game*)mem;
    int x = 62 * CELL, z = 38 * CELL, tx = 62 * CELL, tz = 46 * CELL, pitch = -20;
    if (s == 1) { x = 40 * CELL; z = 22 * CELL; tx = 40 * CELL; tz = 30 * CELL; }
    if (s == 2) { x = 100 * CELL - 8 * METER; z = 40 * CELL - 10 * METER; tx = 100 * CELL; tz = 40 * CELL; }
    if (s == 3) { x = 100 * CELL - 4 * METER; z = 98 * CELL - 6 * METER; tx = 100 * CELL; tz = 98 * CELL + 30 * METER; pitch = 80; g->iS.quest = 3; g->iS.bfb[BF_WORD] = 3; }
    int ccx = CITY_CX * CELL + CELL / 2, ccz = CITY_CZ * CELL + CELL / 2;
    if (s == 5) { x = ccx - 4 * METER; z = ccz - 22 * METER; tx = ccx - 4 * METER; tz = ccz + 40 * METER; pitch = 10; }   // market, looking up the city
    if (s == 6) { x = ccx - 86 * METER; z = ccz - 102 * METER; tx = ccx; tz = ccz; pitch = 20; }       // approach
    if (s == 7) { x = ccx - 2 * METER; z = ccz + 15 * METER; tx = ccx + 5 * METER; tz = ccz + 60 * METER; pitch = 20; }  // wind district
    if (s == 9) { x = ccx - 92 * METER; z = ccz - 46 * METER; tx = ccx - 75 * METER; tz = ccz - 58 * METER; pitch = 0; }   // stables yard
    if (s == 10) { x = ccx - 50 * METER; z = ccz - 12 * METER; tx = ccx; tz = ccz; pitch = 0; g->iS.third = 1; g->iS.weather = W_RAIN; g->iPrevWeather = W_RAIN; }
    if ((s == 5 || s == 6 || s == 7) && g->iWrKit)
        {
        // (v0.4.57) Skyrim's Whiterun (Skyrim units): 5 the market from its south end up to the Wind District, 6 the
        // approach on the road from the stables to the main gate, 7 the Wind District by the Gildergreen
        static const int KW[3 * 4] = { 25000, -7700, 25000, -6000,   17300, -9000, 19300, -7420,   24800, -2800, 23800, -2000 };
        const int* w = KW + (s - 5) * 4;
        x = (int)(((long long)w[0] + 233472) * 36576 / 10000); z = (int)(((long long)w[1] + 176128) * 36576 / 10000);
        tx = (int)(((long long)w[2] + 233472) * 36576 / 10000); tz = (int)(((long long)w[3] + 176128) * 36576 / 10000);
        g->EnterCell(0, x, z, r3d_atan2(tx - x, tz - z)); g->iFadeUs = 0;
        g->iS.py = g->FloorAt(0, x, z, g->iR->GroundAt(x, z) + 8 * METER) + 410;
        g->iS.pitch = 10; g->iUi = UI_PLAY;
        return;
        }
    if (s >= 37 && s <= 40)
        {
        // (v0.4.74) the plains south-west of Whiterun, looking at the city (the Gildergreen, Skyrim 24800, -4300) from
        // 250, 400 and 600 m
        static const int KD[4] = { 250, 400, 600, 250 };     // (40: at 250 m, the other way)
        int d = KD[s - 37] * 70;
        int cx = (int)(((long long)24800 + 233472) * 36576 / 10000), cz = (int)(((long long)-4300 + 176128) * 36576 / 10000);
        x = (int)(((long long)(24800 - d * 6 / 10) + 233472) * 36576 / 10000); z = (int)(((long long)(-4300 - d * 8 / 10) + 176128) * 36576 / 10000);
        g->EnterCell(0, x, z, (r3d_atan2(cx - x, cz - z) + (s == 40 ? 512 : 0)) & 1023); g->iFadeUs = 0;
        g->iS.pitch = 15; g->iUi = UI_PLAY;
        return;
        }
    if (s >= 34 && s <= 36)
        {
        // (v0.4.68) Forelhost's halls, 3 m in from each door: 34 the Stronghold, 35 the Crypt, 36 the Refectory
        const DoorLink& d = g->Door(26 + (s - 34));
        int fx = (r3d_sin(d.iyaw) * 3 * METER) >> 14, fz = (r3d_cos(d.iyaw) * 3 * METER) >> 14;
        g->EnterCell(d.cell, d.ix + fx, d.iz + fz, d.iyaw, d.iy); g->iFadeUs = 0;
        g->iS.pitch = 0; g->iUi = UI_PLAY;
        return;
        }
    if (s == 32 || s == 33)
        {
        // (v0.4.66) the Rift's autumn forest west of Riften (Skyrim's cell 37, -27, its middle): 32 north to the lake,
        // 33 east-south-east
        x = (int)(((long long)(37 * 4096 + 2048) + 233472) * 36576 / 10000); z = (int)(((long long)(-27 * 4096 + 2048) + 176128) * 36576 / 10000);
        g->EnterCell(0, x, z, s == 32 ? 0 : 300); g->iFadeUs = 0;
        g->iS.pitch = 0; g->iUi = UI_PLAY;
        return;
        }
    if (s == 16)
        {
        // Bleak Falls Barrow from the terrace below its doors (Skyrim -700, -44100), looking at them (-3032, -43341)
        x = (int)((-700LL + 233472) * 36576 / 10000); z = (int)((-44100LL + 176128) * 36576 / 10000);
        tx = (int)((-3032LL + 233472) * 36576 / 10000); tz = (int)((-43341LL + 176128) * 36576 / 10000);
        g->EnterCell(0, x, z, r3d_atan2(tx - x, tz - z)); g->iFadeUs = 0;
        g->iS.py = g->FloorAt(0, x, z, (int)(6100LL * 143 / 100 * METER / 100) + g->iR->iLandOffset) + 410;
        g->iS.pitch = 15; g->iS.hp = 100 << 8; g->iUi = UI_PLAY;
        return;
        }
    if (s >= 17 && s <= 20 && g->iBfbKit)
        {
        // (v0.4.55) Skyrim's own barrow (Skyrim units in each cell): the entry hall from the doors towards the bandits'
        // fire, the spider's lair at Arvel's web, the word wall's chamber, the cave below the temple
        static const short KK[4 * 6] = { CELL_BFTEMPLE, -7000, 979, -3825, 745, 573,
                                         CELL_BFTEMPLE, 3000, -3300, 3822, -3336, -1790,
                                         CELL_BFSANCTUM, -6500, -200, -7802, 100, 640,
                                         CELL_BFTEMPLE, -500, -4900, 300, -4500, -4100 };
        const short* b = KK + (s - 17) * 6;
        int bx = 100 * METER + b[1] * 143 / 100 * METER / 100, bz = 100 * METER + b[2] * 143 / 100 * METER / 100;
        int tx2 = 100 * METER + b[3] * 143 / 100 * METER / 100, tz2 = 100 * METER + b[4] * 143 / 100 * METER / 100;
        g->EnterCell(b[0], bx, bz, r3d_atan2(tx2 - bx, tz2 - bz), 100 * METER + b[5] * 143 / 100 * METER / 100);
        g->iS.pitch = 0; g->iFadeUs = 0;
        g->iUi = UI_PLAY;
        return;
        }
    if (s >= 17 && s <= 20)
        {
        // inside the barrow (cm in the cells' frames, from docs/maps/gen_bfb.py)
        static const short KB[4 * 6] = { CELL_BFTEMPLE, 4200, -690, 2250, -675, 0,          // the entry hall from the doors
                                         CELL_BFCRYPT, 1950, -2400, 450, -1875, 0,           // the spider's lair from its door
                                         CELL_BFSANCTUM, -900, -450, -3450, -525, -200,      // the main chamber and the word wall
                                         CELL_BFCRYPT, 1800, 0, 3600, -750, 100 };           // the sunlit cavern
        const short* b = KB + (s - 17) * 6;
        int bx = 100 * METER + b[1] * METER / 100, bz = 100 * METER + b[2] * METER / 100;
        g->EnterCell(b[0], bx, bz, r3d_atan2(b[3] - b[1], b[4] - b[2]));
        g->iS.py = 100 * METER + b[5] * METER / 100 + 410;
        g->iS.pitch = 0; g->iFadeUs = 0;
        g->iUi = UI_PLAY;
        return;
        }
    if (s == 23)
        {
        // the Sleeping Giant: down the hall from the east end to the bar, as the wiki's picture
        g->EnterCell(CELL_RWINN, 100 * METER + 286 * METER / 100, 100 * METER - 214 * METER / 100, 768 + 10);
        g->iS.pitch = 10; g->iFadeUs = 0; g->iUi = UI_PLAY;
        return;
        }
    if (s == 24)
        {
        // Riverwood's street before the inn's porch
        const DoorLink& dl = g->Door(12);
        int x = dl.ox + ((r3d_sin(dl.oyaw) * 14 * METER) >> 14), z = dl.oz + ((r3d_cos(dl.oyaw) * 14 * METER) >> 14);
        g->EnterCell(0, x, z, (dl.oyaw + 512) & 1023); g->iFadeUs = 0;
        g->iS.py = g->iR->GroundAt(x, z) + 410; g->iS.pitch = 10; g->iUi = UI_PLAY;
        return;
        }
    if (s >= 29 && s <= 31)
        {
        // 29 the bridge from its south-west end; 30 the mill and its water wheel from across the race; 31 Faendal at
        // his chopping block, from 6 m
        int x = 0, z = 0, tx = 0, tz = 0;
        if (s == 29) { int y = GameRwHousePos(9, &x, &z) & 1023; tx = x + ((r3d_sin(y) * 10 * METER) >> 14); tz = z + ((r3d_cos(y) * 10 * METER) >> 14); }   // on its deck, along it
        else if (s == 30) { GameRwHousePos(8, &tx, &tz); x = tx + 12 * METER; z = tz + 9 * METER; }
        else
            {
            int i;
            for (i = 0; i < g->iS.ne; i++) if (g->iS.e[i].role == ROLE_NAMED && g->iS.e[i].pad1 == 15) { tx = g->iS.e[i].x; tz = g->iS.e[i].z; }
            x = tx + 5 * METER; z = tz + 3 * METER;
            }
        g->EnterCell(0, x, z, r3d_atan2(tx - x, tz - z)); g->iFadeUs = 0;
        g->iS.py = g->FloorAt(0, x, z, g->iR->GroundAt(x, z) + (s == 29 ? 8 : 2) * METER) + 410; g->iS.pitch = 0; g->iUi = UI_PLAY;
        return;
        }
    if (s == 27 || s == 28)
        {
        // outside, before Alvor's forge and its raised deck (27), before the Riverwood Trader (28), 12 m off the door
        const DoorLink& dl = g->Door(s == 27 ? 13 : 11);
        int x = dl.ox + ((r3d_sin(dl.oyaw) * 12 * METER) >> 14), z = dl.oz + ((r3d_cos(dl.oyaw) * 12 * METER) >> 14);
        g->EnterCell(0, x, z, (dl.oyaw + 512) & 1023); g->iFadeUs = 0;
        g->iS.py = g->FloorAt(0, x, z, g->iR->GroundAt(x, z)) + 410; g->iS.pitch = 10; g->iUi = UI_PLAY;
        return;
        }
    if (s == 25 || s == 26)
        {
        // the houses built from Skyrim's pieces: Gerdur's (the biggest), Sven and Hilde's; from the door looking in
        const DoorLink& dl = g->Door(s == 25 ? 14 : 16);
        g->EnterCell(dl.cell, dl.ix + ((r3d_sin(dl.iyaw) * METER) >> 14), dl.iz + ((r3d_cos(dl.iyaw) * METER) >> 14), dl.iyaw);
        g->iS.pitch = 0; g->iFadeUs = 0; g->iUi = UI_PLAY;
        return;
        }
    if (s == 21 || s == 22)
        {
        // the Riverwood Trader: the shop as Skyrim shows it (hearth, counter, Lucan); upstairs from the stairs' head
        g->iS.bfb[BF_LUCAN] = 1;
        if (s == 21) g->EnterCell(CELL_RWTRADER, 100 * METER - 6 * METER, 100 * METER - 48 * METER / 10, r3d_atan2(450, 440));
        else { g->EnterCell(CELL_RWTRADER, 100 * METER - 6 * METER, 100 * METER - 35 * METER / 10, 256); g->iS.py = 100 * METER + 343 * METER / 100 + 410; }
        g->iS.pitch = -10; g->iFadeUs = 0;
        g->iUi = UI_PLAY;
        return;
        }
    if (s == 15)
        {
        // Highhall from the top of the great stairs: the fire, the feast tables, the throne
        g->EnterCell(CELL_HIGHHALL, 100 * METER, 100 * METER + 28 * METER, 0);
        g->iS.py = 100 * METER + 3 * METER + 410;
        g->iS.pitch = 15; g->iFadeUs = 0;
        g->iUi = UI_PLAY;
        return;
        }
    if (s == 14)
        {
        // the Gilded Stag from its front door: the fire pit, the regulars, the bar
        g->EnterCell(CELL_INN, 100 * METER + 12 * METER / 10, 100 * METER + 87 * METER / 10, 256);
        g->iS.pitch = 0; g->iFadeUs = 0;
        g->iUi = UI_PLAY;
        return;
        }
    if (s == 13)
        {
        // the Bitterroot from its front door: the lab, the desk, the gallery
        g->EnterCell(CELL_SHOP, 100 * METER + 3 * METER, 100 * METER + 82 * METER / 10, 600);
        g->iS.pitch = -20; g->iFadeUs = 0;
        g->iUi = UI_PLAY;
        return;
        }
    if (s == 11 || s == 12)
        {
        // the Hall of the Dead from its door; the catacombs' corridor with the first skeleton coming
        if (s == 11) g->EnterCell(CELL_HALLDEAD, 100 * METER, 100 * METER - 44 * METER / 10, 0);
        else g->EnterCell(CELL_CATACOMBS, 100 * METER, 100 * METER - 40 * METER / 100, 200);
        g->iS.pitch = -20; g->iFadeUs = 0;
        g->iUi = UI_PLAY;
        return;
        }
    if (s == 8)
        {
        // inside the stables residence, from the door
        g->EnterCell(CELL_STABLES, 100 * METER - 2 * METER, 100 * METER - 33 * METER / 10, 0);
        g->iS.pitch = -10; g->iFadeUs = 0;
        g->iUi = UI_PLAY;
        return;
        }
    g->EnterCell(0, x, z, 0);
    g->iFadeUs = 0;
    g->iS.px = x; g->iS.pz = z; g->iS.py = g->iR->GroundAt(x, z) + 410;
    g->iS.yaw = r3d_atan2(tx - x, tz - z); g->iS.pitch = pitch;
    g->iS.hp = 100 << 8;
    g->iUi = s == 4 ? UI_INV : UI_PLAY;
    // (the inventory: the item emu_item() chose, shown alone; the iron sword by default)
    if (s == 4) { g->iS.ninv = 0; g->AddItem(g_emuItem, 1); g->iInvCat = 1; g->iInvCol = 1; g->iInvSel = 0; g->iInvScroll = 0; }
    }
extern "C" void emu_item(u8* mem, int aId) { g_emuItem = aId; }
// close-up of the villagers for head debugging
extern "C" void emu_heads(u8* mem, u16* out)
    {
    Game* g = (Game*)mem;
    R3D& r = *g->iR;
    int cx = 62 * CELL, cz = 30 * CELL; int gy = r.GroundAt(cx, cz);
    u8 s[32];
    int i;
    static const u8 c[32] = { 86,70,52,6, 120,40,36,6, 222,180,146,7, 120,124,128,1, 80,56,36,4, 50,36,28,4, 110,80,50,4, 0,0,0,0 };
    for (i = 0; i < 32; i++) s[i] = c[i];
    r.SetCamera(cx, gy + 430, cz - 2 * METER, 0, -2);
    r.BeginFrame(out, 640);
    for (i = 0; i < 3; i++)
        {
        int x = cx + (i - 1) * METER;
        int yaw = i == 0 ? 512 : (i == 1 ? 512 + 120 : 512 - 150);
        r.DrawMesh(g->iMHuman[i == 2 ? 4 : (i == 1 ? 1 : 0)], 0, x, r.GroundAt(x, cz), cz, yaw, 256, s, 0);
        }
    }
extern "C" void emu_title(u8* mem) { Game* g = (Game*)mem; g->iUi = UI_TITLE; }
extern "C" void emu_step(u8* mem, int k, int* land)
    {
    Game* g = (Game*)mem;
    if (k == 0 && land && land[1] > 0) { g->iLandRead = EmuLandRead; g->iLandSeek = EmuLandSeek; g->iLandCtx = land; }
    if (k == 0 && land && land[5] > 0) { g->iDataRead = EmuDataRead; g->iDataCtx = land; }
    g->InitStep(mem + ((sizeof(Game) + 63) & ~63), k);
    }
// exercise the interaction screens: container, cooking, barter, sitting, carrying
extern "C" void emu_interact(u8* mem, u16* out, int k)
    {
    Game* g = (Game*)mem;
    int i, n = g->iS.nprop;
    static const int types[11] = { PT_BARREL, PT_POT, PT_CHAIR, PT_GARLIC, PT_ITEM, PT_CHEST, PT_CASE, PT_DRESSER, PT_CUPBOARD, PT_BED, PT_DOOR };
    if (k < 11)
        {
        for (i = 0; i < n; i++) if (g->iS.prop[i].type == types[k] && !(g->iS.prop[i].flags & PF_GONE)) break;
        if (i == n) return;
        Prop& p = g->iS.prop[i];
        g->EnterCell(p.cell, p.x - 200, p.z, 256);
        g->iUi = UI_PLAY;
        g->UseProp(i);
        g->Tick(66000, k == 4 ? KEY_USE : 0);
        g->Tick(66000, k == 4 ? KEY_USE : 0);
        int j;
        for (j = 0; j < 6; j++) g->Tick(66000, 0);
        if (k == 4) { g->KeyChar('e'); g->Tick(66000, 0); }      // drop the carried item
        }
    else if (k == 12)
        {
        // torch in hand at night outdoors, brightness curve, the drawbridge lever, a throw
        g->iUi = UI_PLAY;
        g->iS.timeOfDay = 22 * 60000;
        g->ToggleTorch();
        g->iR->SetGamma(2);
        for (i = 0; i < n; i++) if (g->iS.prop[i].type == PT_LEVER) { g->EnterCell(0, g->iS.prop[i].x - 300, g->iS.prop[i].z, 256); g->UseProp(i); }
        int j;
        for (j = 0; j < 8; j++) g->Tick(66000, 0);
        g->Render(out, 640);
        g->iS.third = 1;
        for (j = 0; j < 4; j++) g->Tick(66000, 0);
        g->Render(out, 640);
        g->iS.third = 0;
        g->iR->SetGamma(0);
        g->iS.timeOfDay = 12 * 60000;
        }
    else if (k == 13)
        {
        // the local map, both zooms, panned; a house door's name; the world map
        g->EnterCell(0, CITY_CX * CELL + CELL / 2 - 4 * METER, CITY_CZ * CELL + CELL / 2 - 12 * METER, 0);
        g->iUi = UI_PLAY; g->KeyChar('m');
        g->Tick(66000, 0); g->Render(out, 640);
        g->KeyChar('z'); g->Tick(66000, KEY_FWD); g->Render(out, 640);
        g->KeyChar('z'); g->KeyChar(' '); g->Render(out, 640);
        g->KeyChar('m'); g->iUi = UI_PLAY;
        g->EnterCell(0, CITY_CX * CELL + CELL / 2 - 283 * METER / 10, CITY_CZ * CELL + CELL / 2 - 19 * METER, 512);
        g->Tick(66000, 0); g->Render(out, 640);
        }
    else
        {
        for (i = 0; i < g->iS.ne; i++) if (g->iS.e[i].role == ROLE_MERCHANT) break;
        g->iTradeEnt = i; g->iUi = UI_TRADE; g->iListSel = 0; g->iListCol = 0;
        g->KeyChar('e'); g->iListCol = 1; g->KeyChar('e');
        }
    g->Render(out, 640);
    g->KeyChar('e'); g->KeyChar(27);
    g->Tick(66000, 0);
    }
// the view settings for the scenes (iS.view: Game::ApplyViewSetting)
// (v0.4.75) the renderer's big buffers, for the cache model's map: k -> address (0 past the last), its size, its name
extern "C" int emu_buf(u8* mem, int k, int* aSize)
    {
    Game* g = (Game*)mem; R3D& r = *g->iR;
    switch (k)
        {
        case 0: *aSize = r.iW * r.iH * 2; return (int)(long)r.iZB;
        case 1: *aSize = 640 * 200 * 2; return (int)(long)r.iCB;
        case 2: *aSize = 32 * 4096; return (int)(long)r.iTex;
        case 3: *aSize = r.iSkyN * 4096; return (int)(long)r.iSkyTex;
        case 4: *aSize = r.iLibN * 4096; return (int)(long)r.iLibTex;
        case 5: *aSize = 65536 * 4; return (int)(long)r.iRecip;
        case 6: *aSize = r.iObjCount * (int)sizeof(Obj); return (int)(long)r.iObj;
        case 7: *aSize = 256 * 64 * 2; return (int)(long)r.iShadePal;
        case 8: *aSize = r.iSkyN * 64 * 2 * 16; return (int)(long)r.iSkyShade;
        case 9: *aSize = r.iLibN * 64 * 2 * 16; return (int)(long)r.iLibShade;
        case 10: *aSize = (int)sizeof(R3D); return (int)(long)&r;
        case 11: *aSize = (int)sizeof(Game); return (int)(long)g;
        case 12: *aSize = MAXMV * (int)sizeof(TV); return (int)(long)r.iMeshTV;
        case 13: *aSize = MAXIOBJ * (int)sizeof(Obj); return (int)(long)r.iIObj;
        case 14: *aSize = MAXMV * 4; return (int)(long)r.iMeshLev;
        case 15: *aSize = MAXLF * 2; return (int)(long)r.iFaceList;
        case 16: *aSize = 4600 * 8; return (int)(long)r.iSortKey;      // (r3d.cpp: MAXOBJ 4600, the sort keys 2 words each)
        case 17: *aSize = MAXMV * 2; return (int)(long)r.iLazyList;
        case 18: *aSize = (int)((char*)r.iArenaEnd - (char*)g->iAreaMem); return (int)(long)g->iAreaMem;
        }
    return 0;
    }
extern "C" int emu_addr(u8* mem, int k) { Game* g = (Game*)mem; return k == 0 ? (int)(long)g->iR->iZB : (int)(long)g->iAreaMem; }
extern "C" void emu_texmip(u8* mem, int v) { Game* g = (Game*)mem; g->iR->iTexMip = v; }
extern "C" void emu_occ(u8* mem, int v) { Game* g = (Game*)mem; g->iR->iOccEvery = v; }
extern "C" void emu_skip(u8* mem, int v) { Game* g = (Game*)mem; g->iR->iSkip = v; }
extern "C" void emu_view(u8* mem, int v) { Game* g = (Game*)mem; g->iS.view = v; g->ApplyViewSetting(); }
