// (v0.4.67) People and creatures by place. A place's group is out (in the people table, iS.e) only while the player
// is there - in its cell, or within its reach outside (a world point: the player's, or where he left the open world
// when inside). Away from it the group's are taken off; what changed is kept in the roster (the dead: where they lie,
// looted or not), and when the player comes back the group is spawned again from its place's own tables and the
// roster laid over it. So the people table holds what is near, however many places the world has, and the save
// grows by the dead, not by the living.
#include "game.h"

static inline int Abs(int v) { return v < 0 ? -v : v; }
void GameBfDoor(int* aX, int* aZ);

// a group: its cell (inside), or its reach outside (metres, from the place's point; out again past it and 40 m more)
struct GrpDef { u8 cell, reach; };
static const GrpDef KGrp[G_COUNT] =
    {
    { 0, 0 },                       // G_NONE: always out (the towns' people with their days, the merchants, the wolves)
    { CELL_BFTEMPLE, 0 },
    { CELL_BFCRYPT, 0 },
    { CELL_BFSANCTUM, 0 },
    { 0, 220 },                     // G_BFOUT: the barrow's bandits, round its doors and the tower
    { CELL_CATACOMBS, 0 },
    { 0, 250 },                     // G_WRFOLK: Whiterun's people, in the city or near it (or in its houses)
    { CELL_FHSTRONG, 0 },           // (v0.4.68) Forelhost's halls
    { CELL_FHCRYPT, 0 },
    { CELL_FHREFECT, 0 },
    { 0, 200 }                      // (v0.4.69) G_FHOUT: Valmir (and at the end his soldier), round Forelhost's door
    };
int GameGrpCell(int aGrp) { return aGrp > 0 && aGrp < G_COUNT ? KGrp[aGrp].cell : 0; }

// the place's point outside (engine units); 0 if the place is not in this world
static int GrpPoint(const Game& g, int aGrp, int* aX, int* aZ)
    {
    switch (aGrp)
        {
        case G_BFOUT: GameBfDoor(aX, aZ); return 1;
        case G_FHOUT:
            if (!g.iFhKit) return 0;
            *aX = (int)(((long long)182930 + 233472) * 36576 / 10000); *aZ = (int)(((long long)-113715 + 176128) * 36576 / 10000);
            return 1;
        case G_WRFOLK:
            if (!g.iWrKit) return 0;
            // the Gildergreen, about the city's middle (Skyrim 24800, -4300)
            *aX = (int)(((long long)24800 + 233472) * 36576 / 10000); *aZ = (int)(((long long)-4300 + 176128) * 36576 / 10000);
            return 1;
        }
    return 0;
    }

// whether a group should be out now (aWas: out now - the reach then has its 40 m more)
static int GrpWant(const Game& g, int aGrp, int aWas)
    {
    const GrpDef& d = KGrp[aGrp];
    if (d.cell) return g.iS.cell == d.cell;
    int x, z;
    if (!GrpPoint(g, aGrp, &x, &z)) return 0;
    int px = g.iS.cell ? g.iS.extX : g.iS.px, pz = g.iS.cell ? g.iS.extZ : g.iS.pz;
    int r = (d.reach + (aWas ? 40 : 0)) * METER;
    int dx = Abs(px - x), dz = Abs(pz - z);
    if (dx > r || dz > r) return 0;
    long long ddx = dx >> 8, ddz = dz >> 8, rr = r >> 8;
    return ddx * ddx + ddz * ddz <= rr * rr;
    }

void Game::StageIn(int aGrp)
    {
    if ((iS.grpIn >> aGrp) & 1) return;
    iS.grpIn |= 1u << aGrp;
    int n0, i, k;
    // room made: the land's animals give way (they come and go with the player anyway)
    for (i = iS.ne - 1; i >= 0 && iS.ne > MAXE - 48; i--)
        {
        const Ent& e = iS.e[i];
        if (!e.grp && !e.cell && (e.type == E_WOLF || (e.type >= E_DEER && e.type <= E_MUDCRAB)) && (e.pad1 | e.pad2)) RemoveEnt(i);
        }
    n0 = iS.ne;
    switch (aGrp)
        {
        case G_BFTEMPLE: case G_BFCRYPT: case G_BFSANCTUM: case G_BFOUT:
        case G_FHSTRONG: case G_FHCRYPT: case G_FHREFECT: SpawnDunEnts(aGrp); break;
        case G_CATACOMBS:
            {
            // the Hall of the Dead's catacombs: three skeletons (right of the entrance, at the foot of the east stairs,
            // behind the closed door of the side room), as in Whiterun's
            static const int KSkel[3 * 3] = { 600, 100, 768,   680, 1010, 0,   -760, 60, 256 };
            for (i = 0; i < 3; i++)
                {
                int x = 100 * METER + KSkel[i * 3] * METER / 100, z = 100 * METER + KSkel[i * 3 + 1] * METER / 100;
                int idx = SpawnEnt(E_SKELETON, x, z, KSkel[i * 3 + 2]);
                if (idx < 0) continue;
                Ent& e = iS.e[idx];
                e.cell = CELL_CATACOMBS; e.var = (u8)i;
                e.y = FloorAt(CELL_CATACOMBS, x, z, 0);
                }
            break;
            }
        case G_WRFOLK: SpawnWrFolk(); break;
        case G_FHOUT: SpawnFhOut(); break;
        }
    // numbered in the order spawned (the place's tables' order), then the roster's changes laid over them
    for (i = n0; i < iS.ne; i++) { iS.e[i].grp = (u8)aGrp; iS.e[i].sid = (u8)(i - n0); }
    for (k = 0; k < iS.nroster; k++)
        {
        if (iS.roster[k].grp != aGrp) continue;
        for (i = n0; i < iS.ne; i++)
            {
            Ent& e = iS.e[i];
            if (e.sid != iS.roster[k].sid) continue;
            if (iS.roster[k].flags & 1)
                {
                e.alive = 0; e.state = S_DEAD; e.hp = 0; e.aggro = 0; e.vx = e.vz = e.vy = 0;
                e.cell = iS.roster[k].cell; e.x = iS.roster[k].x; e.y = iS.roster[k].y; e.z = iS.roster[k].z; e.yaw = iS.roster[k].yaw;
                e.pad2 &= (u8)~(8 | 32 | 128);              // (not asleep in its alcove, not hidden: where it fell)
                }
            if (iS.roster[k].flags & 2) e.looted |= 1;
            break;
            }
        }
    if (aGrp == G_WRFOLK) WhiterunSchedule(0);
    }

void Game::StageOut(int aGrp)
    {
    if (!((iS.grpIn >> aGrp) & 1)) return;
    iS.grpIn &= ~(1u << aGrp);
    int i, k;
    for (i = iS.ne - 1; i >= 0; i--)
        {
        Ent& e = iS.e[i];
        if (e.grp != aGrp) continue;
        if (!e.alive && !((e.pad2 & 2) && !(e.looted & 1)))         // (the place's own dead, untouched: theirs again)
            {
            // the dead kept: where they lie, looted or not
            for (k = 0; k < iS.nroster; k++) if (iS.roster[k].grp == aGrp && iS.roster[k].sid == e.sid) break;
            if (k == iS.nroster)
                {
                if (iS.nroster >= GameState::ROSTER_MAX)
                    {
                    // full: the oldest let go (that one is back, as a place is in Skyrim when it resets)
                    for (k = 1; k < iS.nroster; k++) iS.roster[k - 1] = iS.roster[k];
                    iS.nroster--;
                    }
                k = iS.nroster++;
                }
            iS.roster[k].grp = (u8)aGrp; iS.roster[k].sid = e.sid; iS.roster[k].cell = e.cell;
            iS.roster[k].flags = (u8)(1 | ((e.looted & 1) ? 2 : 0));
            iS.roster[k].x = e.x; iS.roster[k].y = e.y; iS.roster[k].z = e.z; iS.roster[k].yaw = (short)e.yaw; iS.roster[k].pad = 0;
            }
        RemoveEnt(i);
        }
    }

// each tick (and on going through a door): the groups out that should be, the others in
void Game::UpdateStage(int aForceOut)
    {
    int g;
    for (g = 1; g < G_COUNT; g++)
        {
        int was = (iS.grpIn >> g) & 1;
        int want = !aForceOut && GrpWant(*this, g, was);
        if (want && !was) StageIn(g);
        else if (!want && was) StageOut(g);
        }
    }
