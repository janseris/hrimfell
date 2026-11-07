// game.h - "Hrimfell": an open-world Nordic action RPG for the Nokia 9300, built on r3d
#ifndef GAME_H
#define GAME_H
#include "r3d.h"
#include "items.h"

// held-key bits (platform maps scan codes to these)
enum
    {
    KEY_LOOK_L = 1, KEY_LOOK_R = 2, KEY_LOOK_U = 4, KEY_LOOK_D = 8,
    KEY_FWD = 16, KEY_BACK = 32, KEY_STRAFE_L = 64, KEY_STRAFE_R = 128,
    KEY_RUN = 256, KEY_JUMP = 512, KEY_FIRE = 1024, KEY_CROUCH = 2048, KEY_USE = 4096
    };

// character codes the platform passes to KeyChar()
enum { CH_TAB = 9, CH_ENTER = 13, CH_ESC = 27 };

enum { MAXPROP = 1400, MAXCONT = 256, CONTSLOTS = 32 };     // (v0.4.74: 900, 192 - Forelhost's own goods and doors did not fit)
enum { WORLD_GX = 1744, WORLD_GZ = 1376 };   // cells of 4 m: Skyrim's 7 x 5.5 km (without the land file: a made-up land)
// Whiterun: the city plan's origin (cell), placed so that the plan's main gate stands on Skyrim's (the gate door
// in Tamriel at game units (19312, -7424), the city's WRWallMainGate01 at (19520, -7552): engine (3615, 2409) m)
enum { CITY_CX = 918, CITY_CZ = 605 };
int GameBarrowOutX(int a, int b); int GameBarrowOutZ(int a, int b);
enum { MAXE = 100, MAXP = 24, MAXINV = 48, MAXMSG = 4, NSTONES = 3, NCHESTS = 4 };

enum EntType { E_NONE = 0, E_VILLAGER, E_ELDER, E_SMITH, E_WOLF, E_WIGHT, E_SKELETON, E_BANDIT, E_SKEEVER, E_SPIDER, E_TROLL, E_DRAGON, E_CHEST,
               E_DEER, E_HARE, E_FOX, E_GOAT, E_MUDCRAB };     // (v0.4.39: the land's animals; E_DEER var 1: an elk)
inline int IsPrey(int t) { return t >= E_DEER && t <= E_GOAT; }
enum EntState { S_IDLE = 0, S_WANDER, S_CHASE, S_ATTACK, S_STAGGER, S_DEAD, S_RETURN,
                D_SLEEP, D_FLY, D_LAND, D_GROUND, D_TAKEOFF };

struct Ent
    {
    u8 type, state, alive, flash, var, looted, aggro, frameBase;
    u8 cell, role, pad1, pad2;    // cell: 0 outdoors, else interior id; role: ROLE_*
    short hp, maxhp;
    int x, y, z, yaw;
    int vx, vz, vy;
    int timer, atk, anim, cool;
    int hx, hz;
    int fly;            // dragon altitude above ground
    u8 grp, sid, pad3, pad4;    // (v0.4.67) its place's group (G_*: 0 always here) and its number in the group's spawning
    };

// a bandit's kind (Skyrim's LvlBandit lists: one-handed, two-handed, archer, heavy armour), kept in Ent::looted
// bits 1-2 (bit 0: looted)
enum { BK_1H = 0, BK_2H, BK_ARCHER, BK_HEAVY };
struct Proj
    {
    u8 active, owner, kind, pad;     // owner 0 = player, 1 = enemy; pad: what it is (3 frost, 4 venom, 5 arrow)
    int x, y, z, vx, vy, vz, life, dmg;
    };
inline int BanditKind(const Ent& e) { return (e.looted >> 1) & 3; }
inline void SetBanditKind(Ent& e, int k) { e.looted = (u8)((e.looted & ~6) | (k << 1)); }

// interactive things placed in the world: loose items, containers, chairs, harvestables, the cooking pot
struct Prop
    {
    u8 type, cell, state, flags;    // state: frame / harvest stage; flags PF_*
    short item, count;              // PT_ITEM: item id + count; containers: container index
    int x, y, z;
    short yaw, anim;                // anim: lid frame (containers)
    int vy;                         // falling items
    int vx, vz;                     // thrown items
    short model, pad3;              // PT_HARVEST: the area's model it is drawn with
    };
enum { PT_NONE = 0, PT_ITEM, PT_GARLIC, PT_FISH, PT_BARREL, PT_SACK, PT_CHAIR, PT_POT, PT_BENCH,
       PT_CHEST, PT_CASE, PT_DRESSER, PT_CUPBOARD, PT_BED, PT_DOOR, PT_BRIDGE, PT_LEVER, PT_CRATE, PT_ALCHEMY,
       PT_HARVEST,                  // Skyrim's flora that are not plants: hanging game, coin purses (taken whole)
       PT_PILLAR, PT_PULLCHAIN, PT_CLAWDOOR, PT_WORDWALL, PT_WELL, PT_COUNT };     // (v0.4.69: Forelhost's well)
// Skyrim's base object kinds for placed things, and furniture markers
enum { BO_NONE = 0, BO_ITEM, BO_FLORA, BO_CONT, BO_FURN, BO_DOOR, BO_ACTI, BO_STAT };
enum { FM_SIT = 1, FM_SLEEP = 2, FM_CRAFT = 4 };
enum { PF_OWNED = 1, PF_GONE = 2, PF_FALL = 4, PF_NODRAW = 8 };     // (NODRAW: a seat of a Skyrim model drawn whole)
struct Cont { u8 id[CONTSLOTS]; u8 n[CONTSLOTS]; };

struct InvItem { u8 id, equipped, fav, pad; short count, pad2; };

// item categories / equip slots
enum { IC_WEAPON = 0, IC_APPAREL, IC_POTION, IC_FOOD, IC_INGRED, IC_BOOK, IC_KEY, IC_MISC, IC_COUNT };
enum { NCATS = IC_COUNT + 2 };          // menu rows: favourites, all, then one per category
// Skyrim's record types for things you can carry (WEAP, ARMO, ALCH, INGR, BOOK, KEYM, MISC, ...)
enum { FT_WEAP = 0, FT_ARMO, FT_AMMO, FT_ALCH, FT_INGR, FT_BOOK, FT_SCRL, FT_KEYM, FT_SLGM, FT_MISC, FT_LIGH };
enum { IF_FOOD = 1, IF_POISON = 2, IF_QUEST = 4 };      // ALCH flags as in Skyrim (food / poison), quest items
// keywords: the VendorItem* family decides which merchant deals in a thing; the others describe it
enum
    {
    KW_V_FOOD = 1 << 0, KW_V_FOODRAW = 1 << 1, KW_V_INGR = 1 << 2, KW_V_POTION = 1 << 3, KW_V_POISON = 1 << 4,
    KW_V_WEAPON = 1 << 5, KW_V_ARMOR = 1 << 6, KW_V_CLOTHING = 1 << 7, KW_V_JEWELRY = 1 << 8, KW_V_GEM = 1 << 9,
    KW_V_BOOK = 1 << 10, KW_V_CLUTTER = 1 << 11, KW_V_HIDE = 1 << 12, KW_V_ANIMALPART = 1 << 13, KW_V_TOOL = 1 << 14,
    KW_V_ORE = 1 << 15,
    KW_WEAP_SWORD = 1 << 16, KW_WEAP_DAGGER = 1 << 17, KW_WEAP_AXE = 1 << 18,
    KW_ARMOR_LIGHT = 1 << 19, KW_ARMOR_HEAVY = 1 << 20, KW_RING = 1 << 21, KW_NECKLACE = 1 << 22,
    KW_DRINK = 1 << 23, KW_ALCOHOL = 1 << 24, KW_COOKED = 1 << 25
    };
enum { EQ_NONE = 0, EQ_WEAPON, EQ_BODY, EQ_HEAD, EQ_HANDS, EQ_FEET, EQ_SHIELD, EQ_RING, EQ_NECK };
enum { PW_HEAL = 1, PW_MAGICKA, PW_STAMINA };

struct ItemDef
    {
    const char* name;
    u8 cat, mesh, equip, effect;
    u8 form, flags;     // FT_* record type, IF_* flags
    short power;        // damage / armour / restore amount
    short weight10;     // weight * 10
    short value;
    short book;         // book text index (-1 none)
    u32 kw;             // KW_* keywords
    };

// everything that is saved
struct GameState
    {
    int magic, version;
    int px, py, pz, yaw, pitch, vy;
    int hp, mp, sp;                 // * 256
    int hpMax, mpMax, spMax;        // plain
    int gold, view, quest, deaths;  // view: draw distance settings (Game::ApplyViewSetting)
    int unused1[NSTONES];
    int chestOpen[NCHESTS];
    int smithGift, elderGift;
    int timeOfDay;                  // milliseconds of game time in a day (0..86 400 000 / 60)
    int playTimeS;
    Ent e[MAXE];
    int ne;
    InvItem inv[MAXINV];
    int ninv;
    int outMode;
    int gamma;                      // display brightness curve 0..3
    // v2
    int cell;                       // 0 outdoors, else interior cell id
    int day;                        // days since the start
    int weather, weatherHours;      // W_* and hours it has lasted
    int third, drawn;               // camera mode, weapon drawn
    int extX, extZ, extYaw;         // where the player left the open world (for teleport back)
    int spare[8];
    // v3
    Prop prop[MAXPROP];
    int nprop;
    Cont cont[MAXCONT];
    int ncont;
    int mgold[8];                   // merchants' purses
    // v9: Bleak Falls Barrow (BF_* below) and its quests
    int bfb[32];
    // v14 (v0.4.40): the land's plants picked, each with the game hour its Skyrim cell was last loaded: Skyrim's cells
    // come back as they were 10 days (240 hours) after the player last had them loaded (the 5 x 5 cells round him,
    // outdoors); being near again starts the wait anew
    enum { FLORA_PICKS = 256 };
    struct { u16 idx, pad; int hour; } pick[FLORA_PICKS];
    int npick;
    // v16 (v0.4.67): people and creatures by place - a group's are out only while the player is there (its cell, or
    // near its place outside); away, only the changes are kept: the dead (where they lie, looted or not)
    enum { ROSTER_MAX = 256 };
    u32 grpIn;                      // the groups whose people are out now (bit per G_*)
    struct { u8 grp, sid, flags, cell; int x, y, z; short yaw, pad; } roster[ROSTER_MAX];     // flags 1 dead, 2 looted
    int nroster;
    // v17 (v0.4.68): the other Nordic ruins' state (Forelhost: FH_*)
    u8 dun[64];
    };
enum { SAVE_VERSION = 19 };
#define HRIMFELL_VERSION "0.4.84"
// the world map's width in pixels (its picture, Game::BuildMapImage)
enum { MAPW = 256 };
// the places with a marker on the map and compass (Skyrim's map markers): their kinds
enum { LOC_CITY = 0, LOC_TOWN, LOC_RUIN };
enum { DISC_US = 4200000 };     // the "discovered" banner: in 0.9 s, held, out in 0.9 s
struct LocInfo { const char* name; int x, z, r, kind; };
int GameLocN();
class Game;
void GameLoc(const Game& g, int i, LocInfo& aOut);
enum { ROLE_MERCHANT = 4, ROLE_CARAVAN = 5, ROLE_TRADER = 6, ROLE_NAMED = 7 };   // 7: someone with a name of their own (pad1 = which; pad2 bit 0 = walks about)   // 5: cat-folk of the caravan, 6: its trader (a merchant too)          // Ent::role; Ent::pad1 = merchant id, Ent::pad2 = container (stock)
enum { MER_GROCER = 0, MER_BUTCHER, MER_JEWELER, MER_CARAVAN, MER_APOTHECARY, MER_INN, MER_RWTRADER, MER_RWINN, MER_COUNT };
enum { W_CLEAR = 0, W_CLOUDY, W_FOG, W_RAIN, W_SNOW, W_COUNT };
enum { ROLE_NONE = 0, ROLE_STABLE, ROLE_SHOP, ROLE_INN };
enum { CELL_NONE = 0, CELL_STABLES = 1, CELL_HALLDEAD = 2, CELL_CATACOMBS = 3, CELL_SHOP = 4, CELL_INN = 5, CELL_HIGHHALL = 6,
       CELL_BFTEMPLE = 7, CELL_BFCRYPT = 8, CELL_BFSANCTUM = 9, CELL_RWTRADER = 10, CELL_RWINN = 11,
       CELL_RWALVOR = 12, CELL_RWGERDUR = 13, CELL_RWFAENDAL = 14, CELL_RWSVEN = 15,
       CELL_PELAGIA = 16, CELL_CHILLFURROW = 17, CELL_BATTLEBORN = 18,
       CELL_FHSTRONG = 19, CELL_FHCRYPT = 20, CELL_FHREFECT = 21, CELL_COUNT = 22 };   // (v0.4.68: Forelhost's halls)   // 12..: built from Skyrim's pieces (game_kit.cpp; v0.4.48: CELL_STABLES too, and Whiterun's farms)
// (v0.4.68) Forelhost's state (GameState::dun): its portcullises raised (0..7), the claw door open, its rings, the word
// learned (Storm Call's first: STRUN), Rahgot fallen, its things put out
enum { FH_GATE0 = 0, FH_CLAWDOOR = 8, FH_RING0, FH_RING1, FH_RING2, FH_WORD, FH_RAHGOT, FH_KITSPAWN, FH_QUEST, FH_JOURNAL, FH_GREETED, FH_COUNT };   // (v0.4.69: Siege on the Dragon Cult's stage, the journal shown)
// Bleak Falls Barrow's state (GameState::bfb): puzzles, traps, what has happened, the quests' stages
enum { BF_PILLAR0 = 0, BF_PILLAR1, BF_PILLAR2, BF_GATE0, BF_GATE1, BF_BLADES0, BF_BLADES1, BF_CLAWDOOR, BF_WORD, BF_LORD,
       BF_ARVEL, BF_TALK, BF_LEVERMAN, BF_SPIDER, BF_OIL0, BF_OIL1, BF_QBLEAK, BF_QCLAW, BF_RING0, BF_RING1, BF_RING2,
       BF_PLATE0, BF_DARTS, BF_TOWER, BF_LUCAN, BF_KITSPAWN, BF_WRSPAWN, BF_COUNT };    // (BF_KITSPAWN, v0.4.55: Skyrim's barrow's own things put out, on first going in)

// a door between the open world and an interior cell (world units, yaw 0..1023 = facing after passing)
// interiors built from Skyrim's own pieces (docs/maps/gen_kit.py -> src/kit_ext.inc, game_kit.cpp)
struct KitMesh { const short* v; int nv; const unsigned short* f; int nf; const short* n; const unsigned short* uv; unsigned char slots[32]; short hx, hz, hy, bx, bz; int lod; int spin, pivz, yoff; const char* name; };   // lod: its far model, or -1; spin: turns about its z (the water wheel), its hub pivz along it and yoff up from the origin
struct KitCell
    {
    int cell; const char* name;
    const int* stat; int nstat;         // model, x, y, z, rotation, scale (Skyrim units, degrees, 256 = 1), always drawn
    const int* prop; int nprop;         // kind, x, y, z, rotation, item
    const int* item; int nitem;         // item, x, y, z
    const int* flame; int nflame;       // x, y, z, scale
    const int* light; int nlight;       // x, y, z, fire
    const int* people; int npeople;     // named, x, y, z, rotation
    const short* floor; int nfloor;     // floor triangles (x, y, z each corner)
    const short* wall; int nwall;       // walls: x0, y0, x1, y1, z0, z1
    const int* door;                    // inside: door x, y, where you come out x, y (Tamriel); outside: door x, y, where you come in x, y; z inside
    const int* grid;                    // x0, y0, columns, rows, the squares' size (a power of two); then per square the start of its floor
                                        // list and of its wall list (columns * rows + 1 each)
    const unsigned short* glist;        // the floors' and walls' indices, square by square
    int ox, oy;                         // outside (cell CELL_NONE): its origin in Skyrim's world units
    // (v0.4.55) what can be seen from where (host/pvs.cpp, the area file's end): the floor's regions - squares of
    // pvsRS units, feet in bands of pvsBH - sorted (x, y, band, 0 each), and for each a bit per placed piece (nstat)
    const short* pvsReg; const unsigned char* pvs; int pvsN, pvsRS, pvsBH;
    };
// (v0.4.67) the groups of people and creatures that are out only while the player is at their place (game_stage.cpp)
enum { G_NONE = 0, G_BFTEMPLE, G_BFCRYPT, G_BFSANCTUM, G_BFOUT, G_CATACOMBS, G_WRFOLK, G_FHSTRONG, G_FHCRYPT, G_FHREFECT, G_FHOUT, G_COUNT };
int GameGrpCell(int aGrp);                      // a cell's group: its cell, else 0
class Game;
int GameChopping(const Game& g, const Ent& e);   // Faendal at his chopping block, by day (game_riverwood.cpp)
int GameKitOutFloor(const Game& g, int aX, int aZ, int aFeetY, int aOff, int* aFound);   // Riverwood's decks, porches and steps (outside)
int GameKitOutBlocked(const Game& g, int aX, int aZ, int aR, int aFeetY, int aOff);      // Riverwood's buildings' walls (outside)
int GameIsKitCell(const Game& g, int aCell);
int GameKitPersonSpot(const Game& g, int aWho, int* aCell, int* aX, int* aY, int* aZ);   // where Skyrim places a named person inside
int GameKitEntryFeet(const Game& g, int aCell);       // the floor height at the door inside (engine units), for arriving

struct DoorLink
    {
    int cell;
    int ocell;              // the cell on the outer side (0: the open world)
    int ox, oz, oyaw;       // outdoors: door position, facing when you come out
    int ix, iz, iyaw;       // inside: door position, facing when you come in
    const char* name;
    int hk;                 // which floor heights are known: 1 inside (iy), 2 outside (oy)
    int iy, oy;             // the floor at the door: inside (engine), outside (engine, over the land's offset)
    };

// shared furniture / interior models (iMIn, iInType)
enum
    {
    IN_SHELL = 0, IN_FIREPIT, IN_FLAME, IN_TABLE, IN_LONGTABLE, IN_BED2, IN_BED1, IN_CUPBOARD, IN_SHELF,
    IN_CHEST, IN_DRESSER, IN_ENDTABLE, IN_DISPLAY, IN_HANG, IN_BARREL, IN_RUG, IN_CANDLE, IN_SACKS,
    IN_CHAIR, IN_GARLIC, IN_FISH, IN_POT, IN_SACK, IN_HAM, IN_BENCH, IN_DOOR, IN_PELTBED, IN_WALLPELT, IN_WALLPELT2, IN_FURRUG, IN_WALLTORCH,
    IN_HALLSHELL, IN_CATASHELL, IN_URNS, IN_COFFIN, IN_SHRINE, IN_PLANTER, IN_BRAZIER, IN_PILLAR, IN_STEPS,
    IN_SHOPSHELL, IN_SHOPUPPER, IN_ALCHLAB, IN_COUNTER, IN_FIREPLACE, IN_HERBS, IN_SHOPSTAIRS, IN_INNSHELL, IN_INNUPPER,
    IN_FIREPIT2, IN_COUNTER2, IN_MATHEX, IN_MATROUND, IN_RUG2, IN_HUTCH, IN_DBLDOOR, IN_CAULDRON, IN_SHOPFRAME, IN_RAILING,
    IN_HHSHELL1, IN_HHSHELL2, IN_HHSTEPS, IN_HHROOF, IN_HHPILLAR, IN_CARPET, IN_FEAST, IN_THRONE, IN_BOOKCASE, IN_BANNER2,
    IN_CHANDELIER, IN_ENCHANTER, IN_WALLMAP, IN_MAPTABLE,
    // Bleak Falls Barrow: decor, the moving parts of its traps and puzzles
    IN_WEB, IN_EGGS, IN_PLAQUE, IN_WALLCOFFIN, IN_SPIRAL, IN_WATERFALL, IN_RUBBLE, IN_BONES, IN_LOGS,
    IN_GATE, IN_BLADE, IN_SPIKEWALL, IN_PUZPILLAR, IN_CLAWDOOR, IN_CLAWRING, IN_WORDWALL, IN_SARCOPH, IN_SARCLID,
    IN_LANTERN, IN_PLATE, IN_CHAIN, IN_OIL, IN_DART, IN_NORCOL, IN_ARCH, IN_MURAL,
    // the Riverwood Trader
    IN_RWTSHELL, IN_RWTUPPER, IN_RWTSTAIRS, IN_RWTHEARTH, IN_RWTCOUNTER, IN_ELKHEAD, IN_WALLFISH, IN_HORNCANDLE, IN_WALLHORN,
    IN_FIREWOOD, IN_RWTSHELF, IN_SQTABLE,
    // the Sleeping Giant Inn
    IN_SGHALL, IN_SGROOF, IN_SGPOSTS, IN_SGWINGS, IN_SGPIT, IN_SGBAR, IN_SGBASE, IN_FARMTABLE, IN_DUMMY, IN_WEAPRACK, IN_ENCHTOP,
    IN_KEG, IN_RUGLOW, IN_COUNT
    };

// a walkable flight of steps along +z (world units); base height filled in after the terrain is final
// a flight of steps' walking surface: from its foot (x, z0) uphill along (dx, dz) (Q14 unit vector) for len,
// hw either side, rising rise above base (base < 0: the ground at the foot, taken when first needed)
struct Ramp { int x, z0, dx, dz, len, hw, rise, base; };

// UI modes
enum { UI_PLAY = 0, UI_DIALOG, UI_MENU, UI_INV, UI_MAP, UI_BOOK, UI_DEAD, UI_TITLE, UI_WAIT, UI_TELE, UI_CONT, UI_COOK, UI_TRADE, UI_HELP, UI_PUZZLE };

class Game
    {
public:
    static int MemNeeded();
    void Init(u8* aMem);
    enum { INIT_STEPS = 6 };
    void InitStep(u8* aMem, int aStep);
    const char* InitStepName(int aStep);
    int  iRamFreeKB, iRamTotalKB, iRamUsedKB;    // filled in by the platform for the FPS display
    void Tick(int aDtUs, u32 aKeys);
    void KeyChar(int aCode);
    void Render(u16* aOut, int aStride);
    void RenderFrame(u16* aOut, int aStride);
    // save/load: the platform reads/writes iS (sizeof(GameState)) when iRequest is set
    int  iRequest;      // 0 none, 1 save, 2 load, 3 quit, 4 update, 5 the benchmark's log to save and send, 6 the settings to save
    void SaveDone(int aOk);
    void LoadDone(int aOk);
    int  ShowFps;
    void DrawFps(int x0, int w);
    void ReportTimes(int aRenderUs, int aBlitUs, int aFrameUs = 0);
    void ApplySpeed();                         // (v0.4.51) the speed settings and the automatic steps to the renderer
    int  WantDirect() const { return iDirect; }
    int  iDirect;

    // ---- state ----
    R3D* iR;
    GameState iS;
    GameState iLoad;               // the platform reads save files here first
    Proj iP[MAXP];
    // ui / transient
    int iUi, iUiPrev;
    u32 iKeys, iPrevKeys;
    u32 iHitIn;                 // keys pressed since the last frame (the platform sets these as the keys go down:
                                // a tap between two frames counts)
    u32 iHit; int iRepUs;       // this frame's presses (with a held direction repeating in the menus)
    int iCrouch, iCrouchY;      // crouching (C, or Ctrl held): the view lowered by iCrouchY as it goes down
    int iShowModel;             // X: the Skyrim model the crosshair is on, named in the top left corner
    const char* LookedAtModel();
    u16* iFrozen; int iFrozenOk;    // the world as it was when a menu opened (drawn under it, not again each frame)
    int iLookHoldUs;
    int iSwingUs, iSwingHitDone, iCastCoolUs, iShoutCoolUs, iShoutFxUs, iHurtUs, iShakeUs;
    int iHurtFrom;                     // the heading the last blow came from
    int iBarUs[3];                     // the health, magicka and stamina bars: shown while this runs (or not full)
    int iBarLast[3];
    int iStaminaWaitUs, iMagickaWaitUs;
    int iTarget, iTargetUs;        // last enemy hit/targeted (health bar)
    int iFocus;                    // entity or static index under the crosshair (activation)
    int iFocusKind;                // 0 none, 1 entity, 2 stone
    char iMsg[MAXMSG][56];
    int iMsgUs[MAXMSG];
    // dialogue
    int iDlgId, iDlgPage;
    // menus
    int iMenuSel;
    int iInvCat, iInvSel, iInvCol, iInvScroll;
    int iBookId, iBookPage;
    int iBookProp;                 // reading a book lying in the world (its prop), or -1: from the inventory
    int iPreviewAngle;
    int iTitleUs;
    int iTimeUs;
    int iFps10, iFrames, iFpsAccUs, iRenderUs, iBlitUs;
    int iRenderAccUs, iBlitAccUs, iRenderAvgMs, iBlitAvgMs;    // (v0.4.76) drawing and the copy to the screen, averaged (ms)
    int iAutoLevel, iAutoAccUs, iAutoN, iAutoGood;
    int iTalks;                                      // (v0.4.53) how many greetings: the next of a person's own lines   // (v0.4.51) Auto speed: steps taken (0..3), the frames measured
    int iDragonIdx;
    int iStoneObj[NSTONES];
    int iVictoryUs;
    // the land file's reader, set by the platform before Init (else a made-up land)
    R3D::TLandRead iLandRead; R3D::TLandSeek iLandSeek; void* iLandCtx;
    int iSlowFrames;
    // weapon / camera
    int iDrawUs;                   // draw or sheathe animation time left
    int iPlayerAnim;               // walk cycle for the third-person body
    int iCamX, iCamY, iCamZ, iCamDist;
    // wait
    int iWaitSel, iWaitLeft, iWaitAccUs, iSleeping;
    // weather particles
    int iRainSeed;
    int iPrevWeather, iWBlend;     // weather cross-fade 0..256
    // doors / teleport
    int iDoorFocus;                // door index under the crosshair or -1
    int iTeleSel, iTeleScroll;
    // props: use / grab / sit, container, cooking and trade screens
    int iUsePending, iUseHeldUs, iGrab, iGrabArmed, iSitting, iSitStandY, iFireLock, iOpenWaitUs, iPropTickUs;
    int iWellX, iWellZ;
    int iBridgeX, iBridgeZ, iBridgeY, iLeverX, iLeverZ, iBridgeProp;
    int iTentX[3], iTentZ[3], iTentYaw[3], iTentBig[3];      // the caravan's tents (for the props inside)
    int iSitX, iSitZ, iSitYaw, iSitY;   // where and which way the seat faces
    int iOpenCont, iOpenProp, iListSel, iListCol, iListScroll;
    int iTradeEnt;
    int iMenuSide;                 // item screens: 0 = barrel / merchant, 1 = player
    int iFadeUs;
    // interiors
    int iCellFloorY;               // floor height of interior cells (world units)
    DoorLink iDoors[32];
    Ramp iRamps[12];
    int iNumRamps;
    void AddRamp(int aX, int aZ0, int aLen, int aHw, int aRise);
    void AddRampYaw(int aX, int aZ, int aYaw, int aLen, int aHw, int aRise, int aBase);
    int iDoorsInit;
    Mesh iMIn[136];                // interior furniture (IN_COUNT of them)
    int iInType[136];
    u16* iMapImg;                  // 128x128 world map
    // the world map's close-up (data/hrimfell_map.bin: a byte a land vertex): DET_ROWS whole rows of it kept
    // (from row iDetRow0, -1 none yet), its width and height, its palette as the screen's colours
    enum { DET_ROWS = 240, DET_MAXW = 1760 };
    u8* iDet; int iDetRow0, iDetW, iDetH, iDetOk; u16 iDetPal[256];
    int DetailRows(int aRow0, int aRows);
    // the land's plants (the area file: gen_kit.py's kit_flora.bin): n of them in 32 m squares (gw x gh from the
    // corner ox, oz; first[k]: the first in square k), 6 bytes each; which have been picked (a bit each, till the
    // game is loaded again); their models, by kind
    enum { FLORA_MAX = 32768, FK_COUNT = 26, FK_TREE0 = 21 };     // (v0.4.66: 21.. the Rift's trees: aspens tall, middling, small; pines, snowy pines)
    int iFloraN, iFloraOX, iFloraOZ, iFloraSq, iFloraGW, iFloraGH;
    const u16* iFloraFirst; const u8* iFloraEnt; u8* iFloraTaken;
    Mesh iMFlora[FK_COUNT], iMTuft;
    // the land's animals (the area file: Skyrim's creature markers): n of them, 12 bytes each; which have one out now
    // (a bit each); their models (deer, elk, hare, fox, goat, mudcrab)
    enum { ANIM_MAX = 1024 };
    int iAnimN, iAnimTickUs; const u8* iAnimEnt; u8 iAnimOut[ANIM_MAX / 8];
    Mesh iMAnimal[6];
    u8 iFloraCol[FK_COUNT * 6];
    u8 iFloraTex[FK_COUNT];            // (v0.4.42) a kind drawn as Skyrim's leaf cards: its picture's number, else 255
    Mesh iMFloraFar[FK_COUNT]; u8 iFloraFar[FK_COUNT];     // a bush's plainer form past 10 m (HFM1), if it has one        // each plant kind's colours (its leaves', darker): the game's, or Skyrim's (HFM1)
    short iItemKit[IT_COUNT];          // the items' own Skyrim models (the area file): a kit model each, -1 none
    // the items' own Skyrim models as made, with their textures (data/items.hit, one read at a time: game_kit.cpp)
    enum { ITEMBUF_KB = 160, ITEMSHADE_KB = 64 };      // (the model; its colours lit 16 ways, laid out as R3D::iSkyShade)
    u8* iItemBuf; int iItemShown; int iItemTab;      // the buffer; the item in it (-1 none); the table read (1), absent (-1)
    u32 iItemOff[IT_COUNT]; u32 iItemLen[IT_COUNT];
    u16* iItemShade;
    Mesh iMItemView; const u8* iItemSlots; const u8* iItemTex; const u8* iItemTex1; const u8* iItemHi; int iItemTexN, iItemHiN;
    const Mesh* ItemModel(int aId);                  // its model (and iItemSlots, iItemTex ...), or 0: the game's own
    void DrawItemView(const Mesh& m, const u8* aSlots, int x, int y, int z, int aYaw, int aPitch, int aRoll);
    void BuildAnimals();
    void BuildQuadruped(Mesh& m, int aLen, int aH, int aLeg, int aNeck, int aHead, int aHorn, int aEar, int aTail);
    void UpdateAnimals(int aDt);
    void RemoveEnt(int i);
    void BuildFlora();
    void DrawFlora();
    void DrawGrass();
    void FindFloraFocus();
    void TakeFlora(int i);
    void UpdatePicks();                // the picked plants' cells: their waits, and those come back
    void RebuildFloraTaken();          // iFloraTaken from iS.pick (a game loaded, the area read)
    int  GameHour() const { return iS.day * 24 + iS.timeOfDay / 60000; }
    int FloraItem(int aKind);
    void FloraPos(int i, int& aX, int& aZ);
    struct MapView { int x0, w, detail, ppv, u0, v0, step, ww, wd, mh; };
    int iMapBest;
    void MapXY(const MapView& v, int wx, int wz, int& sx, int& sy);
    void DrawMapMarks(R3D& r, const MapView& v);
    void DrawMapCross(R3D& r, int x0, int w, int);
    // meshes
    Mesh iMHuman[7];               // man, elder, wight, troll, woman, cat-folk, skeleton (smooth, close range)
    Mesh iMHumanLod[7];            // light versions for distance
    Mesh iMPlayer[2];              // third-person body: sword sheathed / drawn
    Mesh iMWolfLod;
    Mesh iMWolf, iMDragon, iMFireball, iMChest, iMHand, iMHandOpen, iMFlame, iMFlameSmall;
    int iSmallFlameType;               // the candles' flame (IPlace: IN_FLAME at under 150)
    Mesh iMItem[IM_COUNT];
    short iItemRest[IM_COUNT * 3];   // world rest pose: pitch, lift (units at scale), scale
    Mesh iMCity[72];
    int iCityType[72];
    Mesh iMHorse;

    // ---- functions ----
    void BuildWorld();
    void BuildCityMeshes();
    void BuildCityTerrain();
    void BuildCity();
    void Place(int aCt, int aMx, int aMz, int aYaw, int aScale, int aVar);
    void WallRun(const int* aP, int aN, int aScale);
    void BuildMeshes();
    void BuildHumanoid(Mesh& m, int aKind, int aLod);
    void PaintHead(Mesh& m, int aF0, int aF1, int aHy, int aHz, int aHs, int aKind);
    void BuildWolf(Mesh& m);
    void BuildWolfLod(Mesh& m);
    void BuildDragon(Mesh& m);
    void BuildDragonLod(Mesh& m);
    void BuildItems();
    void RotateFrame(Mesh& m, int aSrc, int aDst, int aAngle, int aLift);
    void NewGame();
    int  SpawnEnt(int aType, int aX, int aZ, int aYaw);
    void UpdatePlayer(int dt);
    void UpdateEnts(int dt);
    void UpdateEnt(Ent& e, int dt);
    void UpdateDragon(Ent& e, int dt);
    void UpdateProj(int dt);
    void MoveEnt(Ent& e, int aDx, int aDz, int aRadius);
    void HurtPlayer(int aDmg, int aFromX, int aFromZ);
    void HurtEnt(Ent& e, int aDmg, int aKnock, int aFromX, int aFromZ);
    void KillEnt(Ent& e);
    void PlayerAttack();
    void CastSpell();
    void Shout();
    void DrinkPotion(int aEffect);
    void Activate();
    void FindFocus();
    void StartDialog(int aId);
    void DialogNext();
    void Msg(const char* aS);
    void Msg2(const char* aA, const char* aB);
    void Respawn();
    // camera, weapon, wait, weather, interiors
    void ToggleWeapon();
    void UpdateCamera(int dt);
    void DrawPlayerBody();
    void StartWait();
    void WaitInput(int aDtUs);
    void FindDoor();
    void UseDoor(int i);
    int LineClear(int aCell, int x0, int y0, int z0, int x1, int y1, int z1);
    void ShootArrow(Ent& e);
    int ArcherSees(Ent& e, int dt);
    int DoorOutY(const DoorLink& d) const;
    void WeatherParams(int& aLight, int& aGrey, int& aF0, int& aF1);
    int  DrawAmount();
    void WaitHour();
    void StepWeather();
    void DrawWeather();
    void DrawWait(int x0, int w);
    void DrawTele(int x0, int w);
    void TeleInput();
    void Teleport(int aIdx);
    void BuildInteriors();
    void BuildStablesInterior();
    void BuildHallDeadInterior();
    void BuildCryptMeshes();
    void BuildShopMeshes();
    void BuildShopInterior();
    void BuildInnMeshes();
    void BuildInnInterior();
    void BuildHighhallMeshes();
    void BuildHighhall();
    void SpawnHighhall();
    int iCookMode;                 // the crafting screen: 0 cooking pot, 1 alchemy lab
    void BuildCatacombs();
    // Bleak Falls Barrow (game_dungeon.cpp): the cells built from their grids, the outside, traps, puzzles, quests
    enum { MAXDUN = 128 };
    Mesh iMDun[MAXDUN];            // the dungeon cells' walls, floors and ceilings, a tile at a time
    int iDunCell[MAXDUN], iDunX[MAXDUN], iDunY[MAXDUN], iDunZ[MAXDUN], iNDun;
    u8 iDunSl[32];                 // their colours
    Mesh iMSpider;
    int iBfTimer[20];              // transient: trap animations, the darts, the oil fires, speech
    int iPuzRing;                  // the claw door's ring being turned (UI_PUZZLE)
    int iPuzDoor;                  // (v0.4.68) which claw door: 0 Bleak Falls' (the Golden Claw), 1 Forelhost's (the Glass Claw)
    int iFhGate[8], iFhClaw;
    int iFireWall[10 * 5];         // (v0.4.71) the staff's Walls of Flames: x, y, z, its turn, ms left (transient)       // (v0.4.68) Forelhost's portcullises' and claw door's heights now (cm), as iBfTimer[8..10]
    void BuildDungeonMeshes();
    void BuildDungeons();
    void BuildRiverwood();
    void SpawnSgInn();
    void BuildKit();
    void SpawnKit();
    enum { MAXKIT = 1500 };      // (v0.4.46: 800, was 720 - the buildings' middle models; v0.4.48: Whiterun's outskirts)
    Mesh iMKit[MAXKIT];                // Skyrim's own models, simplified (game_kit.cpp)
    int iKitType[MAXKIT];
    // the area's models and places, read from its file (data/riverwood.har: tools/kit_pack.py) into iAreaMem
    enum { AREA_KB = 7168 };     // (the area file and its tables of models and places after it; v0.4.46: 5120, was 4608 - the 256 pictures, the middle models; v0.4.48: Whiterun's outskirts)
    u8* iAreaMem;
    const KitMesh* iKitMeshes; int iKitMeshN;   // (iKitMeshN: the library's and the area's together)
    // (v0.4.56) the shared library (data/common.hlb, tools/lib_pack.py): its models first, kept for every area
    enum { LIB_KB = 2048 };
    u8* iLibMem; const KitMesh* iLibMeshes; int iLibMeshN;
    const KitMesh& KitM(int i) const { return i < iLibMeshN ? iLibMeshes[i] : iKitMeshes[i - iLibMeshN]; }
    void BuildKitMesh(int i);
    const KitCell* iKitCells; int iKitCellN;
    int  iKitObj0[32];           // (v0.4.55) each kit cell's first placed piece in R3D::iIObj (-1: not all in a row)
    int LoadArea(const char* aName, int aLib = 0);
    // (v0.4.55) the areas' files: 0 data/riverwood.har (Riverwood, the outskirts, the houses), 1 data/bleakfalls.har
    // (Bleak Falls Barrow's halls, Skyrim's own) - one at a time in iAreaMem, the other's models, places, lights gone
    int iAreaCur, iAreaDefs, iAreaIObj, iBfbKit;
    int iWrKit, iOutObjMark, iOutArea;
    // (v0.4.74) a loading screen: what needs an area's file read is put off a frame - the screen says so first (the
    // phone's app sets iDeferLoads; the host and the emulator read at once)
    int iDeferLoads, iLoadPhase, iLoadKind, iLoadA[5];
    void Go(int aCell, int aX, int aZ, int aYaw, int aFeet = 0x7fffffff);
    int  DeferLoad(int aKind, int a0 = 0, int a1 = 0, int a2 = 0, int a3 = 0, int a4 = 0);
    void DoPendingLoad();
    // (v0.4.84) the settings (Brightness, the view's, Display, Show FPS) kept in a file of their own by the platform:
    // read into iCfg before the start-up's last stage, written (iRequest 6) when they change and the menu is shut
    enum { CFG_MAGIC = 0x47464348 };
    int iCfg[4];                // magic, gamma, view, outMode | ShowFps << 4
    void CfgUse();
    void CfgCheck();
    // (v0.4.82) the benchmark (menu: Benchmark): every travel place in turn - loaded, left to settle, measured - one
    // line each into iBenchLog; when done (or stopped by Esc) iRequest 5: the platform saves the log and sends it to
    // the PC. The player doesn't move meanwhile, the weapon sheathed, Auto speed off (put back after)
    enum { BENCH_LOG = 16384 };
    int iBench, iBenchPhase, iBenchUs, iBenchLoadUs, iBenchLoaded, iBenchView;
    int iBenchN, iBenchTris, iBenchTrisMin, iBenchTrisMax, iBenchR, iBenchRMin, iBenchRMax, iBenchB, iBenchBMax, iBenchF, iBenchFMax;
    int iBenchLen, iBenchTotalUs;
    char iBenchLog[BENCH_LOG];
    void BenchStart();
    void BenchFrame(int aRenderUs, int aBlitUs, int aFrameUs);
    void BenchStop(const char* aWhy);
    void BenchPut(const char* aS);
    void BenchNum(int aV, int aWidth);
    int  BenchCount() const;
    const char* BenchName(int aStep, int* aTravel);
    void DrawLoading(u16* aOut, int aStride);
    void LoadFinish();
    int iFhKit;                        // (v0.4.68) Forelhost's file is there (data/forelhost.har)
    int iOutObjN;                // (v0.4.65) how many statics the area put outside (from iOutObjMark)
    const short* iOutPvsReg; const u8* iOutPvs; int iOutPvsRegN, iOutPvsRS, iOutPvsBH, iOutPvsObjN;     // (what can be seen from where outside: Skyrim's world units)
    int iLidType;                // (v0.4.62) Skyrim's sarcophagus lid (NorSarcophagusTop) as a kind of static, for the moving lids (-1: none)
    short iLidOpen[8];           // (and how far each moving lid of the cell is aside, 0..90)   // (v0.4.57) Whiterun's own area (data/whiterun.har); the outside statics' start; whose are out
    int AreaOutside(int aX, int aZ);
    int AreaOf(int aCell, int aX, int aZ);
    u8* iAreaArena;
    void UseArea(int aArea);
    // the platform's reader of the game's data files (by name: "riverwood.har"): the bytes it gave from aPos
    typedef int (*TDataRead)(void* aCtx, const char* aName, int aPos, u8* aBuf, int aLen);
    TDataRead iDataRead; void* iDataCtx;
    void SpawnRiverwood();
    void PlaceKitOutside();            // Riverwood's buildings from Skyrim's own models (game_kit.cpp)
    void PlaceKitOut(const KitCell* c, int off);
    void RiverwoodSchedule(int aForce = 0);   // Riverwood's people out by day, home by night
    void SpawnWhiterun();                      // (v0.4.48) Whiterun's outskirts' people
    void WhiterunSchedule(int aForce = 0);
    void BuildSpider(Mesh& m);
    void SpawnDungeons();
    void SpawnDunEnts(int aGrp);               // (v0.4.67) a group's people and creatures (game_stage.cpp's groups)
    void SpawnWrFolk();
    void SpawnFhOut();                         // (v0.4.69) Valmir, and at the end his soldier
    void FhQuestUpdate(int dt);
    int  FhValmirTalk();
    void FhDialogDone(int aId);
    void UpdateStage(int aForceOut = 0);       // groups in and out as the player moves
    void StageIn(int aGrp);
    void StageOut(int aGrp);
    int  DunFloor(int aCell, int aX, int aZ, int aFeetY);
    int  DunBlocked(int aCell, int aX, int aZ, int aR, int aFeetY);
    void DrawDungeon();
    void UpdateDungeon(int dt);
    int  UseDunProp(int i);
    int  DunPropText(int i, const char*& aVerb, const char*& aName);
    void DrawDunProp(const Prop& p);
    void PuzzleInput();
    void DrawPuzzle(int x0, int w);
    int  DunEntUpdate(Ent& e, int dt, int d);
    void DunDialogDone(int aId);
    int  DunTalk(Ent& e);
    void DunKilled(Ent& e);
    void DunSpellHit(int aX, int aY, int aZ);
    int  FarengarTalk();
    int  LucanTalk(int aEnt);
    int  CamillaTalk();
    void RwTraderEnter();
    void SpawnRwTrader();
    int  IsDungeon(int aCell) const { return (aCell >= CELL_BFTEMPLE && aCell <= CELL_BFSANCTUM) || (aCell >= CELL_FHSTRONG && aCell <= CELL_FHREFECT); }
    void ReviveSkeletons();
    void BuildHorse(Mesh& m);
    int  FloorAt(int aCell, int aX, int aZ, int aFeetY);
    int  BlockedAt(int aCell, int aX, int aZ, int aR, int aFeetY, int aHeight = 7 * METER / 4);
    void EnterCell(int aCell, int aX, int aZ, int aYaw, int aFeet = 0x7fffffff);
    int  NumDoors();
    const DoorLink& Door(int i);
    int  HostilesNear(int aDist);
    const char* WeatherName();
    void IPlace(int aCell, int aCt, int aMx, int aMy, int aMz, int aYaw, int aScale);
    // light sources (fires, candles, torches) and the per-frame choice of the nearest
    struct LightSrc { int cell, x, y, z, r, s; };
    enum { MAXLSRC = 240 };     // (v0.4.55: 240 - Skyrim's barrow, about 100 of its own)
    LightSrc iLSrc[MAXLSRC];
    int iNLSrc;
    void AddLightSrc(int aCell, int aX, int aY, int aZ, int aRadius, int aStrength);
    void ILight(int aCell, int aMx, int aMy, int aMz, int aRadCm, int aStrength);
    void WallTorch(int aCell, int aMx, int aMz, int aYaw);
    void SetupLights();
    void BuildApproach();
    void Brazier(int aX, int aZ, int aScale);
    int  BridgeFloor(int aX, int aZ, int aFeetY);
    // props
    int  AddProp(int aType, int aCell, int aX, int aY, int aZ, int aYaw, int aItem, int aCount, int aFlags);
    int  NewCont();
    void ContAdd(int aCont, int aId, int aCount);
    void SpawnProps();
    void SpawnMerchants();
    void UpdateProps(int dt);
    void DrawProps();
    void DrawCaseGoods(const Prop& p);
    void DrawCityProp(const Prop& p);
    void FindPropFocus();
    void UseProp(int i);
    void TakeProp(int i);
    void PropText(int i, const char*& aVerb, const char*& aName, char* aInfo, int& aRed);
    int  SurfaceAt(int aCell, int aX, int aZ, int aY);
    int  PropBlocked(int aCell, int aX, int aZ, int aR, int aFeetY = -0x7FFFFFFF);
    int  WaysOut(int aCell, int aX, int aZ, int aFeet, int aWalls = 1);    // (v0.4.74) of 8 ways, how many a step can be taken
    int  ClimbOk(int aCell, int aX, int aZ, int aNx, int aNz, int aFeet, int aGround, int aMx, int aMz);     // (v0.4.74) the walker's climb rule
    void FreeSpot();                                      // (v0.4.74) the player moved to the nearest spot with ways out
    void StandUp();
    void CloseScreens();
    void ContInput();
    int  MenuCats(int aSide, int* aOut);
    int  InvCats(int* aOut);
    int  MenuRows(int aSide, int aCat, int* aId, int* aCount, int* aRef);
    void ItemMenuInput();
    void MoveOne(int aId, int aRef);
    void OpenItemMenu(int aUi);
    void DrawItemMenu(int x0, int w);
    void CookInput();
    void TradeInput();
    void ListKey(int c);
    void DrawCont(int x0, int w);
    void DrawCook(int x0, int w);
    void DrawTrade(int x0, int w);
    int  CountItem(int aId);
    void TakeItems(int aId, int aCount);
    int  WeaponDamage();
    int  ArmorRating();
    int  CarryWeight10();
    int  EquippedId(int aSlot);
    int  AddItem(int aId, int aCount);
    void RemoveItem(int aIdx, int aCount);
    void UseItem(int aIdx);
    void ToggleTorch();
    void GiveLoot(Ent& e);
    void OpenChest(Ent& e);
    void UpdateAtmosphere();
    void QuestTargets(int* aX, int* aZ, int& aN);
    const char* QuestText();
    // input in menus
    int  Pressed(u32 aBit) const { return (iHit & aBit) != 0; }
    void UiInput();
    void InvInput();
    void MenuInput();
    // drawing
    void DrawEntities();
    void DrawViewModel();
    void ViewMesh(const Mesh& m, int aFrame, int x, int y, int z, int yaw, int pitch, int roll, const u8* sl, int fl);
    int iHideView;
    void DrawHud();
    void DrawCompass(int x0, int w);
    void DrawBars(int x0, int w);
    void DrawDialog(int x0, int w);
    void DrawMenu(int x0, int w);
    void DrawHelp(int x0, int w);
    void DrawInventory(int x0, int w);
    void DrawMap(int x0, int w);
    void DrawLocalMap(int x0, int w);
    int  MapShape(int aType, int& aHx, int& aHz, int& aOz);   // local map: how a static is drawn (0: not at all)
    int  iMapLocal, iMapCX, iMapCZ, iMapZoom;
    int  iMapPref, iMapWZ;
    int  iMapRem, iMapPrevUs, iMapHoldUs;                              // the map's panning: the last tick, how long a key is held                                    // the local map chosen last; the world map's zoom (0: all of it, 1: across the screen, 2: twice that)
    int  iDiscUs, iDiscLoc;                                   // the "discovered" banner: time left, which place
    void Discover();                 // local map shown, the point at its centre, zoom (0: 2 px a metre, 1: 1)
    void DrawBook(int x0, int w);
    void DrawWrapped(int aX, int aY, int aW, const char* aS, u16 aCol, int aMaxLines);
    void BuildMapImage();
    void ApplyViewSetting();
    int MapImgH() const;
    int  InvList(int aCat, int* aOut, int aMax);
    };

#endif
