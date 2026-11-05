#include <stdio.h>
#include <stdlib.h>
#define memcpy glue_memcpy
#define memset glue_memset
#include "../emu/game_glue.cpp"
#undef memcpy
#undef memset
int GameKitOutFloor(const Game& g, int aX, int aZ, int aFeetY, int aOff, int* aFound);
int main(int argc, char** argv)
 {
 int n = emu_memneeded(); u8* mem = (u8*)calloc(1, n + 64); u8* m = (u8*)(((size_t)mem + 63) & ~(size_t)63);
 emu_init(m, 0); Game* g = (Game*)m; emu_mode(m, 0);
 double X = atof(argv[1]), Y = atof(argv[2]);
 int ex = (int)((X + 233472) * 0.0142875 * METER), ez = (int)((Y + 176128) * 0.0142875 * METER);
 g->EnterCell(0, ex, ez, 0);
 int off = g->iR->iLandLoaded ? g->iR->iLandOffset : 0, f;
 for (int up = -2; up <= 10; up += 2) { int y = GameKitOutFloor(*g, ex, ez, g->iR->GroundAt(ex, ez) + up * METER, off, &f); printf("up %d: found %d y %.0f units (ground %.0f) area %d cells %d\n", up, f, (y / 256.0 - 206) / 0.0142875, (g->iR->GroundAt(ex, ez) / 256.0 - 206) / 0.0142875, g->iAreaCur, g->iKitCellN); }
 for (int k = 0; k < g->iKitCellN; k++) { const KitCell& c = g->iKitCells[k]; printf("cell %d %s nfloor %d ox %d oy %d grid %d %d %d %d sq %d\n", c.cell, c.name, c.nfloor, c.ox, c.oy, c.grid[0], c.grid[1], c.grid[2], c.grid[3], c.grid[4]); }
 }
