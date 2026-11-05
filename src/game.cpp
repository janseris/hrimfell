// game.cpp - Hrimfell game logic: world layout, player, creatures, combat, quest, items, save
#include "game.h"
#ifndef __EPOC32__
#include <stdio.h>
// on the PC (tests): data/hrimfell.land from the working folder, if it is there
static int HostLandRead(void* aCtx, u8* aBuf, int aLen) { return (int)fread(aBuf, 1, aLen, (FILE*)aCtx); }
static int HostLandSeek(void* aCtx, int aPos) { return fseek((FILE*)aCtx, aPos, SEEK_SET) == 0; }
// the game's other data files: data/<name> from the working folder
static int HostDataRead(void*, const char* aName, int aPos, u8* aBuf, int aLen)
    {
    char path[256]; snprintf(path, sizeof(path), "data/%s", aName);
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, aPos, SEEK_SET);
    int n = (int)fread(aBuf, 1, aLen, f);
    fclose(f);
    return n;
    }
#endif

static const int M = METER;


static const ItemDef KItems[IT_COUNT] =
    {
    // Skyrim-style records: the form type decides the menu category, keywords decide who buys it and what it is
    // name                       cat         mesh         equip      effect       form     flags    pow wt10 value book  keywords
    { "Iron Sword",               IC_WEAPON,  IM_SWORD,    EQ_WEAPON, 0,           FT_WEAP, 0,         7,  90,  25, -1, KW_V_WEAPON | KW_WEAP_SWORD },
    { "Steel Sword",              IC_WEAPON,  IM_SWORD2,   EQ_WEAPON, 0,           FT_WEAP, 0,         9, 100,  45, -1, KW_V_WEAPON | KW_WEAP_SWORD },
    { "Iron War Axe",             IC_WEAPON,  IM_AXE,      EQ_WEAPON, 0,           FT_WEAP, 0,         8, 110,  30, -1, KW_V_WEAPON | KW_WEAP_AXE },
    { "Iron Dagger",              IC_WEAPON,  IM_DAGGER,   EQ_WEAPON, 0,           FT_WEAP, 0,         4,  20,  10, -1, KW_V_WEAPON | KW_WEAP_DAGGER },
    { "Ancient Nord Sword",       IC_WEAPON,  IM_SWORD2,   EQ_WEAPON, 0,           FT_WEAP, 0,        12, 110,  90, -1, KW_V_WEAPON | KW_WEAP_SWORD },
    { "Fur Armor",                IC_APPAREL, IM_CUIRASS,  EQ_BODY,   0,           FT_ARMO, 0,        18,  60,  20, -1, KW_V_ARMOR | KW_ARMOR_LIGHT },
    { "Iron Armor",               IC_APPAREL, IM_CUIRASS,  EQ_BODY,   0,           FT_ARMO, 0,        25, 300, 125, -1, KW_V_ARMOR | KW_ARMOR_HEAVY },
    { "Fur Helmet",               IC_APPAREL, IM_HELMET,   EQ_HEAD,   0,           FT_ARMO, 0,         8,  10,  10, -1, KW_V_ARMOR | KW_ARMOR_LIGHT },
    { "Iron Helmet",              IC_APPAREL, IM_HELMET,   EQ_HEAD,   0,           FT_ARMO, 0,        15,  50,  60, -1, KW_V_ARMOR | KW_ARMOR_HEAVY },
    { "Fur Boots",                IC_APPAREL, IM_BOOTS,    EQ_FEET,   0,           FT_ARMO, 0,         5,  10,   8, -1, KW_V_ARMOR | KW_ARMOR_LIGHT },
    { "Iron Boots",               IC_APPAREL, IM_BOOTS,    EQ_FEET,   0,           FT_ARMO, 0,        10,  60,  25, -1, KW_V_ARMOR | KW_ARMOR_HEAVY },
    { "Fur Gauntlets",            IC_APPAREL, IM_GLOVES,   EQ_HANDS,  0,           FT_ARMO, 0,         5,  10,   8, -1, KW_V_ARMOR | KW_ARMOR_LIGHT },
    { "Iron Gauntlets",           IC_APPAREL, IM_GLOVES,   EQ_HANDS,  0,           FT_ARMO, 0,        10,  50,  25, -1, KW_V_ARMOR | KW_ARMOR_HEAVY },
    { "Hide Shield",              IC_APPAREL, IM_SHIELD,   EQ_SHIELD, 0,           FT_ARMO, 0,         8,  40,  15, -1, KW_V_ARMOR | KW_ARMOR_LIGHT },
    { "Iron Shield",              IC_APPAREL, IM_SHIELD,   EQ_SHIELD, 0,           FT_ARMO, 0,        20, 120,  60, -1, KW_V_ARMOR | KW_ARMOR_HEAVY },
    { "Potion of Minor Healing",  IC_POTION,  IM_POTION,   0,         PW_HEAL,     FT_ALCH, 0,        25,   5,  17, -1, KW_V_POTION },
    { "Potion of Healing",        IC_POTION,  IM_POTION,   0,         PW_HEAL,     FT_ALCH, 0,        50,   5,  36, -1, KW_V_POTION },
    { "Potion of Minor Magicka",  IC_POTION,  IM_POTION,   0,         PW_MAGICKA,  FT_ALCH, 0,        25,   5,  20, -1, KW_V_POTION },
    { "Potion of Minor Stamina",  IC_POTION,  IM_POTION,   0,         PW_STAMINA,  FT_ALCH, 0,        25,   5,  20, -1, KW_V_POTION },
    { "Bread",                    IC_FOOD,    IM_BREAD,    0,         PW_HEAL,     FT_ALCH, IF_FOOD,   3,   2,   2, -1, KW_V_FOOD },
    { "Cheese Wedge",             IC_FOOD,    IM_CHEESE,   0,         PW_HEAL,     FT_ALCH, IF_FOOD,   3,   3,   3, -1, KW_V_FOOD },
    { "Red Apple",                IC_FOOD,    IM_APPLE,    0,         PW_HEAL,     FT_ALCH, IF_FOOD,   2,   1,   3, -1, KW_V_FOODRAW },
    { "Roast Venison",            IC_FOOD,    IM_MEAT,     0,         PW_HEAL,     FT_ALCH, IF_FOOD,   8,   5,   6, -1, KW_V_FOOD | KW_COOKED },
    { "Frost Mirriam",            IC_INGRED,  IM_HERB,     0,         PW_MAGICKA,  FT_INGR, 0,         2,   1,   4, -1, KW_V_INGR },
    { "Wolf Fang",                IC_INGRED,  IM_FANG,     0,         0,           FT_INGR, 0,         0,   1,   6, -1, KW_V_INGR | KW_V_ANIMALPART },
    { "Troll Tusk",               IC_INGRED,  IM_FANG,     0,         PW_STAMINA,  FT_INGR, 0,         4,   5,  25, -1, KW_V_INGR | KW_V_ANIMALPART },
    { "Wight Dust",               IC_INGRED,  IM_HERB,     0,         0,           FT_INGR, 0,         0,   1,  10, -1, KW_V_INGR },
    { "The Book of the Dragonborn", IC_BOOK,    IM_BOOK,     0,         0,           FT_BOOK, 0,         0,  10,  15,  0, KW_V_BOOK },
    { "Dragon Language: Myth No More", IC_BOOK,    IM_BOOK,     0,         0,           FT_BOOK, 0,         0,  10,  20,  1, KW_V_BOOK },
    { "Night of Tears",           IC_BOOK,    IM_BOOK,     0,         0,           FT_BOOK, 0,         0,  10,   5,  2, KW_V_BOOK },
    { "Wolf Pelt",                IC_MISC,    IM_PELT,     0,         0,           FT_MISC, 0,         0,  50,  12, -1, KW_V_HIDE },
    { "Troll Pelt",               IC_MISC,    IM_PELT,     0,         0,           FT_MISC, 0,         0, 120,  45, -1, KW_V_HIDE },
    { "Rusty Key",                IC_KEY,     IM_KEY,      0,         0,           FT_KEYM, 0,         0,   1,   0, -1, 0 },
    { "Silver Ring",              IC_APPAREL, IM_RING,     EQ_RING,   0,           FT_ARMO, 0,         0,   1,  60, -1, KW_V_JEWELRY | KW_RING },
    { "Dragon Scale",             IC_MISC,    IM_SCALE,    0,         0,           FT_MISC, 0,         0,  30, 250, -1, KW_V_ANIMALPART },
    { "Cabbage",                  IC_FOOD,    IM_CABBAGE,  0,         PW_HEAL,     FT_ALCH, IF_FOOD,   2,   3,   6, -1, KW_V_FOODRAW },
    { "Leek",                     IC_FOOD,    IM_LEEK,     0,         PW_HEAL,     FT_ALCH, IF_FOOD,   1,   1,   4, -1, KW_V_FOODRAW },
    { "Potato",                   IC_FOOD,    IM_POTATO,   0,         PW_HEAL,     FT_ALCH, IF_FOOD,   1,   1,   1, -1, KW_V_FOODRAW },
    { "Carrot",                   IC_FOOD,    IM_CARROT,   0,         PW_HEAL,     FT_ALCH, IF_FOOD,   1,   1,   4, -1, KW_V_FOODRAW },
    { "Garlic",                   IC_INGRED,  IM_GARLIC,   0,         PW_HEAL,     FT_INGR, 0,         1,   1,   1, -1, KW_V_INGR | KW_V_FOODRAW },
    { "Salt Pile",                IC_INGRED,  IM_SALT,     0,         0,           FT_INGR, 0,         0,   2,   2, -1, KW_V_INGR | KW_V_FOODRAW },
    { "Venison",                  IC_FOOD,    IM_MEAT,     0,         PW_HEAL,     FT_ALCH, IF_FOOD,   1,  10,   4, -1, KW_V_FOODRAW },
    { "Salmon Meat",              IC_FOOD,    IM_FISH,     0,         PW_HEAL,     FT_ALCH, IF_FOOD,   1,   1,   3, -1, KW_V_FOODRAW },
    { "Raw Beef",                 IC_FOOD,    IM_STEAK,    0,         PW_HEAL,     FT_ALCH, IF_FOOD,   1,  10,   4, -1, KW_V_FOODRAW },
    { "Salmon Steak",             IC_FOOD,    IM_FISH,     0,         PW_HEAL,     FT_ALCH, IF_FOOD,  10,   2,   7, -1, KW_V_FOOD | KW_COOKED },
    { "Cooked Beef",              IC_FOOD,    IM_STEAK,    0,         PW_HEAL,     FT_ALCH, IF_FOOD,  10,   5,   8, -1, KW_V_FOOD | KW_COOKED },
    { "Vegetable Soup",           IC_FOOD,    IM_BOWL,     0,         PW_HEAL,     FT_ALCH, IF_FOOD,  15,   5,  12, -1, KW_V_FOOD | KW_COOKED },
    { "Cabbage Potato Soup",      IC_FOOD,    IM_BOWL,     0,         PW_MAGICKA,  FT_ALCH, IF_FOOD,  15,   5,   8, -1, KW_V_FOOD | KW_COOKED },
    { "Apple Cabbage Stew",       IC_FOOD,    IM_BOWL,     0,         PW_STAMINA,  FT_ALCH, IF_FOOD,  15,   5,  10, -1, KW_V_FOOD | KW_COOKED },
    { "Venison Stew",             IC_FOOD,    IM_BOWL,     0,         PW_STAMINA,  FT_ALCH, IF_FOOD,  25,  10,  12, -1, KW_V_FOOD | KW_COOKED },
    { "Garlic Bread",             IC_FOOD,    IM_BREAD,    0,         PW_HEAL,     FT_ALCH, IF_FOOD,   6,   2,   5, -1, KW_V_FOOD | KW_COOKED },
    { "Gold Ring",                IC_APPAREL, IM_RING,     EQ_RING,   0,           FT_ARMO, 0,         0,   2,  75, -1, KW_V_JEWELRY | KW_RING },
    { "Silver Necklace",          IC_APPAREL, IM_NECKLACE, EQ_NECK,   0,           FT_ARMO, 0,         0,   3,  60, -1, KW_V_JEWELRY | KW_NECKLACE },
    { "Gold Necklace",            IC_APPAREL, IM_NECKLACE, EQ_NECK,   0,           FT_ARMO, 0,         0,   3, 120, -1, KW_V_JEWELRY | KW_NECKLACE },
    { "Garnet",                   IC_MISC,    IM_GEM,      0,         0,           FT_MISC, 0,         0,   1, 100, -1, KW_V_GEM },
    { "Amethyst",                 IC_MISC,    IM_GEM,      0,         0,           FT_MISC, 0,         0,   1, 120, -1, KW_V_GEM },
    { "Tankard",                  IC_MISC,    IM_MUG,      0,         0,           FT_MISC, 0,         0,  10,   1, -1, KW_V_CLUTTER },
    { "Clay Jug",                 IC_MISC,    IM_JUG,      0,         0,           FT_MISC, 0,         0,  20,   2, -1, KW_V_CLUTTER },
    { "Pewter Plate",             IC_MISC,    IM_PLATE,    0,         0,           FT_MISC, 0,         0,  10,   2, -1, KW_V_CLUTTER },
    { "Wooden Bowl",              IC_MISC,    IM_BOWL,     0,         0,           FT_MISC, 0,         0,  10,   1, -1, KW_V_CLUTTER },
    { "Spiced Berry Wine",        IC_POTION,  IM_FLAGON,   0,         PW_STAMINA,  FT_ALCH, 0,        25,   5,  25, -1, KW_V_FOOD | KW_DRINK | KW_ALCOHOL },
    { "Honey Cream Mead",         IC_POTION,  IM_FLAGON,   0,         PW_STAMINA,  FT_ALCH, 0,        25,   5,  25, -1, KW_V_FOOD | KW_DRINK | KW_ALCOHOL },
    { "Ember Brandy Draught",     IC_POTION,  IM_FLAGON,   0,         PW_STAMINA,  FT_ALCH, 0,        25,   5,  25, -1, KW_V_FOOD | KW_DRINK | KW_ALCOHOL },
    { "Clay Pot",                 IC_MISC,    IM_CLAYPOT,  0,         0,           FT_MISC, 0,         0,  20,   2, -1, KW_V_CLUTTER },
    { "Charred Hare Meat",        IC_FOOD,    IM_ROAST,    0,         PW_HEAL,     FT_ALCH, IF_FOOD,   5,   5,   3, -1, KW_V_FOOD | KW_COOKED },
    { "Torch",                    IC_MISC,    IM_TORCH,    EQ_SHIELD, 0,           FT_LIGH, 0,         0,  10,   2, -1, KW_V_CLUTTER },
    { "Bone Meal",                IC_INGRED,  IM_SALT,     0,         0,           FT_INGR, 0,         0,   5,   5, -1, KW_V_INGR },
    { "Blisterwort",              IC_INGRED,  IM_HERB,     0,         PW_HEAL,     FT_INGR, 0,         2,   2,  12, -1, KW_V_INGR },
    { "Lavender",                 IC_INGRED,  IM_HERB,     0,         0,           FT_INGR, 0,         0,   1,   3, -1, KW_V_INGR },
    { "Blue Mountain Flower",     IC_INGRED,  IM_HERB,     0,         PW_HEAL,     FT_INGR, 0,         2,   1,   2, -1, KW_V_INGR },
    { "Wheat",                    IC_INGRED,  IM_HERB,     0,         PW_HEAL,     FT_INGR, 0,         2,   1,   5, -1, KW_V_INGR },
    { "Potion of Cure Disease",   IC_POTION,  IM_POTION,   0,         0,           FT_ALCH, 0,         0,   5,  80, -1, KW_V_POTION },
    { "Weak Poison",              IC_POTION,  IM_POTION,   0,         0,           FT_ALCH, IF_POISON, 0,   5,  20, -1, KW_V_POISON },
    { "Herbalist's Guide to Skyrim",    IC_BOOK,    IM_BOOK,     0,         0,           FT_BOOK, 0,         0,  10,  50,  3, KW_V_BOOK },
    { "Deathbell",                IC_INGRED,  IM_HERB,     0,         0,           FT_INGR, 0,          0,   1,   4, -1, KW_V_INGR },
    { "White Cap",                IC_INGRED,  IM_MUSHROOM, 0,         0,           FT_INGR, 0,          0,   3,   1, -1, KW_V_INGR },
    { "Canis Root",                IC_INGRED,  IM_ROOT,     0,         0,           FT_INGR, 0,          0,   1,   5, -1, KW_V_INGR },
    { "Juniper Berries",          IC_INGRED,  IM_HERB,     0,         0,           FT_INGR, 0,          0,   1,   1, -1, KW_V_INGR },
    { "Fire Salts",               IC_INGRED,  IM_CRYSTAL,  0,         0,           FT_INGR, 0,          0,   2,  50, -1, KW_V_INGR },
    { "Void Salts",               IC_INGRED,  IM_CRYSTAL,  0,         0,           FT_INGR, 0,          0,   2, 125, -1, KW_V_INGR },
    { "Glow Dust",                IC_INGRED,  IM_SALT,     0,         PW_MAGICKA,  FT_INGR, 0,          2,   5,  20, -1, KW_V_INGR },
    { "Hagraven Feathers",           IC_INGRED,  IM_FEATHER,  0,         0,           FT_INGR, 0,          0,   1,  20, -1, KW_V_INGR },
    { "Troll Fat",                IC_INGRED,  IM_EGG,      0,         0,           FT_INGR, 0,          0,  10,  15, -1, KW_V_INGR | KW_V_ANIMALPART },
    { "Skeever Tail",                 IC_INGRED,  IM_ROOT,     0,         0,           FT_INGR, 0,          0,   2,   3, -1, KW_V_INGR | KW_V_ANIMALPART },
    { "Spider Egg",               IC_INGRED,  IM_EGG,      0,         0,           FT_INGR, 0,          0,   2,   5, -1, KW_V_INGR | KW_V_ANIMALPART },
    { "Orange Dartwing",          IC_INGRED,  IM_INSECT,   0,         0,           FT_INGR, 0,          0,   1,   1, -1, KW_V_INGR | KW_V_ANIMALPART },
    { "Blue Dartwing",            IC_INGRED,  IM_INSECT,   0,         0,           FT_INGR, 0,          0,   1,   1, -1, KW_V_INGR | KW_V_ANIMALPART },
    { "Imp Stool",                  IC_INGRED,  IM_MUSHROOM, 0,         0,           FT_INGR, 0,          0,   3,   0, -1, KW_V_INGR },
    { "Taproot",                  IC_INGRED,  IM_ROOT,     0,         PW_MAGICKA,  FT_INGR, 0,          2,   5,  15, -1, KW_V_INGR },
    { "Slaughterfish Scales",              IC_INGRED,  IM_SCALE,    0,         0,           FT_INGR, 0,          0,   1,   3, -1, KW_V_INGR | KW_V_ANIMALPART },
    { "Ice Wraith Teeth",         IC_INGRED,  IM_FANG,     0,         0,           FT_INGR, 0,          0,   3,  30, -1, KW_V_INGR },
    { "Warbler Egg",              IC_INGRED,  IM_EGG,      0,         0,           FT_INGR, 0,          0,   5,   2, -1, KW_V_INGR | KW_V_ANIMALPART },
    { "Ectoplasm",                IC_INGRED,  IM_POTION,   0,         0,           FT_INGR, 0,          0,   2,  25, -1, KW_V_INGR },
    { "Spriggan Sap",               IC_INGRED,  IM_POTION,   0,         PW_HEAL,     FT_INGR, 0,          3,   2,  15, -1, KW_V_INGR },
    { "Creep Cluster",                IC_INGRED,  IM_HERB,     0,         0,           FT_INGR, 0,          0,   2,   1, -1, KW_V_INGR },
    { "Potion of Resist Fire",    IC_POTION,  IM_POTION,   0,         0,           FT_ALCH, 0,          0,   5,  60, -1, KW_V_POTION },
    { "Frostbite Venom",          IC_POTION,  IM_POTION,   0,         0,           FT_ALCH, IF_POISON,   0,   5,  30, -1, KW_V_POISON },
    // Bleak Falls Barrow
    { "Golden Claw",              IC_MISC,    IM_FANG,     0,         0,           FT_MISC, IF_QUEST,   0,  10, 100, -1, 0 },
    { "Arvel's Journal",          IC_BOOK,    IM_BOOK,     0,         0,           FT_BOOK, IF_QUEST,   0,  10,   5,  4, KW_V_BOOK },
    { "Dragonstone",              IC_MISC,    IM_PLATE,    0,         0,           FT_MISC, IF_QUEST,   0, 250,   0, -1, 0 },
    { "Thief",                    IC_BOOK,    IM_BOOK,     0,         0,           FT_BOOK, 0,          0,  10,  50,  5, KW_V_BOOK },
    { "Potion of Resist Frost",   IC_POTION,  IM_POTION,   0,         0,           FT_ALCH, 0,          0,   5,  60, -1, KW_V_POTION },
    { "Ancient Nord War Axe of Frost", IC_WEAPON, IM_AXE,  EQ_WEAPON, 0,           FT_WEAP, 0,         12, 140, 120, -1, KW_V_WEAPON | KW_WEAP_AXE },
    { "Raw Rabbit Leg",           IC_FOOD,    IM_STEAK,    0,         PW_HEAL,     FT_ALCH, IF_FOOD,   1,   1,   2, -1, KW_V_FOODRAW },
    { "Pheasant Breast",          IC_FOOD,    IM_STEAK,    0,         PW_HEAL,     FT_ALCH, IF_FOOD,   1,   2,   3, -1, KW_V_FOODRAW },
    // Skyrim's own: damage, weight, value
    { "Iron Mace",                IC_WEAPON,  IM_MACE,     EQ_WEAPON, 0,           FT_WEAP, 0,         9, 130,  35, -1, KW_V_WEAPON | KW_WEAP_AXE },
    { "Iron Greatsword",          IC_WEAPON,  IM_SWORD2,   EQ_WEAPON, 0,           FT_WEAP, 0,        15, 160,  50, -1, KW_V_WEAPON | KW_WEAP_SWORD },
    { "Iron Battleaxe",           IC_WEAPON,  IM_AXE,      EQ_WEAPON, 0,           FT_WEAP, 0,        16, 200,  55, -1, KW_V_WEAPON | KW_WEAP_AXE },
    { "Iron Warhammer",           IC_WEAPON,  IM_MACE,     EQ_WEAPON, 0,           FT_WEAP, 0,        18, 240,  60, -1, KW_V_WEAPON | KW_WEAP_AXE },
    { "Hunting Bow",              IC_WEAPON,  IM_BOW,      EQ_WEAPON, 0,           FT_WEAP, 0,         7,  70,  50, -1, KW_V_WEAPON },
    { "Long Bow",                 IC_WEAPON,  IM_BOW,      EQ_WEAPON, 0,           FT_WEAP, 0,         6,  50,  30, -1, KW_V_WEAPON },
    { "Iron Arrow",               IC_WEAPON,  IM_ARROW,    0,         0,           FT_WEAP, 0,         8,   0,   1, -1, KW_V_WEAPON },
    { "Lockpick",                 IC_MISC,    IM_KEY,      0,         0,           FT_MISC, 0,         0,   0,   3, -1, 0 },
    { "Iron Ingot",               IC_MISC,    IM_INGOT,    0,         0,           FT_MISC, 0,         0,  10,   7, -1, KW_V_ORE },
    { "Steel Dagger",             IC_WEAPON,  IM_DAGGER,   EQ_WEAPON, 0,           FT_WEAP, 0,         5,  25,  18, -1, KW_V_WEAPON | KW_WEAP_DAGGER },
    // (Skyrim's: Purple Mountain Flower 0x0002F44C restores stamina, Red 0x0007E8C1 magicka; Thistle Branch, Tundra
    //  Cotton, Snowberries)
    { "Purple Mountain Flower",   IC_INGRED,  IM_HERB,     0,         PW_STAMINA,  FT_INGR, 0,         2,   1,   2, -1, KW_V_INGR },
    { "Red Mountain Flower",      IC_INGRED,  IM_HERB,     0,         PW_MAGICKA,  FT_INGR, 0,         2,   1,   2, -1, KW_V_INGR },
    { "Thistle Branch",           IC_INGRED,  IM_HERB,     0,         0,           FT_INGR, 0,         0,   1,   1, -1, KW_V_INGR },
    { "Tundra Cotton",            IC_INGRED,  IM_HERB,     0,         0,           FT_INGR, 0,         0,   1,   1, -1, KW_V_INGR },
    { "Snowberries",              IC_INGRED,  IM_HERB,     0,         0,           FT_INGR, 0,         0,   1,   4, -1, KW_V_INGR },
    { "Deer Hide",                IC_MISC,    IM_PELT,     0,         0,           FT_MISC, 0,         0,   5,  20, -1, KW_V_HIDE },
    { "Small Antlers",            IC_INGRED,  IM_FANG,     0,         0,           FT_INGR, 0,         0,   1,  10, -1, KW_V_INGR | KW_V_ANIMALPART },
    { "Large Antlers",            IC_INGRED,  IM_FANG,     0,         0,           FT_INGR, 0,         0,  10,  25, -1, KW_V_INGR | KW_V_ANIMALPART },
    { "Fox Pelt",                 IC_MISC,    IM_PELT,     0,         0,           FT_MISC, 0,         0,   5,  15, -1, KW_V_HIDE },
    { "Goat Hide",                IC_MISC,    IM_PELT,     0,         0,           FT_MISC, 0,         0,   5,  10, -1, KW_V_HIDE },
    { "Leg of Goat",              IC_FOOD,    IM_MEAT,     0,         PW_HEAL,     FT_ALCH, IF_FOOD,   2,   5,   3, -1, KW_V_FOODRAW },
    { "Mudcrab Chitin",           IC_INGRED,  IM_SCALE,    0,         0,           FT_INGR, 0,         0,   5,   2, -1, KW_V_INGR | KW_V_ANIMALPART },
    // (v0.4.68) Forelhost: the Glass Claw (its door's key: Fox, Owl, Snake); Rahgot, the dragon priest's mask (Skyrim:
    // heavy helmet, armour 23, weight 9, value 962; its +70 stamina not yet - the game's apparel has no enchantments)
    { "Glass Claw",               IC_MISC,    IM_FANG,     0,         0,           FT_MISC, IF_QUEST,   0,  10, 100, -1, 0 },
    { "Rahgot",                   IC_APPAREL, IM_HELMET,   EQ_HEAD,   0,           FT_ARMO, 0,        23,  90, 962, -1, KW_V_ARMOR | KW_ARMOR_HEAVY },
    { "Skorm Snow-Strider's Journal", IC_BOOK, IM_BOOK,    0,         0,           FT_BOOK, IF_QUEST,   0,  10,   5,  6, 0 },
    { "Forelhost Well Key",       IC_MISC,    IM_KEY,      0,         0,           FT_MISC, IF_QUEST,   0,   0,   0, -1, 0 },
    // (v0.4.71) Rahgot's: Skyrim's Dragon Priest Staff (fire) - damage 0 (a staff), weight 8, value 1570; held and
    // swung it sprays a Wall of Flames, 50 fire damage a second for 30 s, ten at most (game_fhquest.cpp)
    { "Dragon Priest Staff",      IC_WEAPON,  IM_TORCH,    EQ_WEAPON, 0,           FT_WEAP, 0,         0,  80, 1570, -1, KW_V_WEAPON },
    // v0.4.72: Forelhost's books and notes, where Skyrim leaves them (game_fhquest.cpp); texts 6.. (fh_books.inc)
    { "Alchemist's Note",         IC_BOOK,    IM_NOTE,     0,         0,           FT_BOOK, 0,          0,   0,   0,  7, 0 },
    { "Rahgot's Reply",           IC_BOOK,    IM_NOTE,     0,         0,           FT_BOOK, 0,          0,   0,   0,  8, 0 },
    { "Orders",                   IC_BOOK,    IM_NOTE,     0,         0,           FT_BOOK, 0,          0,   0,   0,  9, 0 },
    { "Note",                     IC_BOOK,    IM_NOTE,     0,         0,           FT_BOOK, 0,          0,   0,   0, 10, 0 },
    { "Note",                     IC_BOOK,    IM_NOTE,     0,         0,           FT_BOOK, 0,          0,   0,   0, 11, 0 },
    { "The Exodus",               IC_BOOK,    IM_BOOK,     0,         0,           FT_BOOK, 0,          0,  10,  55, 12, KW_V_BOOK },
    { "Spell Tome: Flames",       IC_BOOK,    IM_BOOK,     0,         0,           FT_BOOK, 0,          0,  10,  50, 13, KW_V_BOOK },
    };

// three colours (slots 0..2) per item
static const u8 KItemCol[IT_COUNT * 9] =
    {
    150,152,158,  92,64,40,  110,108,104,     // iron sword
    186,192,204,  60,44,36,  150,150,160,     // steel sword
    140,142,146,  110,78,48,  140,142,146,    // axe
    150,152,158,  92,64,40,  110,108,104,     // dagger
    120,140,150,  50,40,40,  170,140,70,      // ancient Nord sword (old bronze fittings)
    120,88,58,  96,70,46,  70,52,36,          // fur armor
    128,130,136,  100,102,108,  80,60,40,     // iron armor
    128,96,62,  100,74,50,  90,90,90,         // fur helmet
    132,134,140,  100,102,108,  120,120,120,  // iron helmet
    116,86,58,  80,60,40,  0,0,0,             // fur boots
    126,128,134,  90,92,98,  0,0,0,           // iron boots
    116,86,58,  86,66,46,  0,0,0,             // fur gauntlets
    126,128,134,  96,98,104,  0,0,0,          // iron gauntlets
    120,90,60,  120,90,60,  150,150,150,      // hide shield
    126,128,134,  126,128,134,  90,70,50,     // iron shield
    200,40,40,  180,190,200,  120,90,60,      // minor healing (red)
    230,30,50,  180,190,200,  120,90,60,      // healing
    50,80,220,  180,190,200,  120,90,60,      // magicka (blue)
    60,180,70,  180,190,200,  120,90,60,      // stamina (green)
    200,148,82,  236,216,172,  236,206,150,   // bread (crust, crumb, the cuts)
    236,200,90,  220,190,100,  0,0,0,         // cheese
    204,40,34,  92,62,34,  90,150,60,         // apple
    130,60,40,  230,220,200,  0,0,0,          // venison
    150,190,200,  220,240,255,  0,0,0,        // frost lichen
    236,230,210,  200,190,170,  0,0,0,        // wolf fang
    220,214,190,  190,180,160,  0,0,0,        // troll tusk
    170,170,160,  210,210,200,  0,0,0,        // wight dust
    110,40,30,  230,220,190,  0,0,0,          // book
    40,60,110,  230,220,190,  0,0,0,          // book
    90,70,50,  230,220,190,  0,0,0,           // book
    120,120,124,  90,90,94,  0,0,0,           // wolf pelt
    110,120,100,  80,90,70,  0,0,0,           // troll pelt
    150,100,60,  0,0,0,  0,0,0,               // key
    210,214,224,  0,0,0,  80,160,255,         // silver ring
    120,40,30,  160,70,40,  0,0,0,            // dragon scale
    110,160,80,  150,196,110,  0,0,0,         // cabbage
    210,226,190,  80,150,64,  0,0,0,          // leek
    170,130,80,  140,100,60,  0,0,0,          // potato
    226,122,40,  80,150,60,  0,0,0,           // carrot
    236,230,212,  196,186,166,  0,0,0,        // garlic
    236,236,240,  210,210,214,  0,0,0,        // salt
    170,60,56,  230,210,200,  0,0,0,          // venison
    232,132,100,  236,206,186,  150,156,166,  // salmon meat
    180,50,50,  236,220,210,  0,0,0,          // raw beef
    196,110,70,  120,70,40,  150,156,166,     // salmon steak
    120,70,44,  90,50,30,  0,0,0,             // cooked beef
    140,100,60,  176,140,62,  0,0,0,          // vegetable soup
    140,100,60,  196,190,140,  0,0,0,         // cabbage potato soup
    140,100,60,  180,100,60,  0,0,0,          // apple cabbage stew
    140,100,60,  128,78,48,  0,0,0,           // venison stew
    204,150,80,  236,214,170,  226,200,120,   // garlic bread
    232,190,80,  0,0,0,  232,190,80,          // gold ring
    214,214,224,  0,0,0,  80,160,255,         // silver necklace
    232,190,80,  0,0,0,  200,40,40,           // gold necklace
    170,30,40,  220,80,90,  0,0,0,            // garnet
    140,70,190,  200,140,240,  0,0,0,         // amethyst
    150,150,156,  110,80,52,  170,120,50,     // tankard
    176,98,60,  140,76,46,  0,0,0,            // clay jug
    156,156,164,  130,130,138,  0,0,0,        // pewter plate
    126,90,58,  110,78,50,  0,0,0,            // wooden bowl
    150,154,160,  110,112,118,  0,0,0,        // spiced berry wine (flagon)
    176,170,150,  120,116,100,  0,0,0,        // honey cream mead (flagon)
    120,96,70,  90,80,70,  0,0,0,             // ember brandy draught (flagon)
    176,100,62,  120,70,44,  0,0,0,           // clay pot
    96,56,32,  70,40,24,  0,0,0,              // charred hare meat
    104,76,50,  40,30,22,  0,0,0,             // torch (shaft, pitch-soaked head)
    226,220,204,  196,188,170,  0,0,0,        // bone meal
    120,70,60,  86,110,60,  0,0,0,            // blisterwort (red-brown cap, green stem)
    150,120,200,  90,120,80,  0,0,0,          // lavender
    80,110,200,  80,120,70,  0,0,0,           // blue mountain flower
    210,180,100,  170,150,80,  0,0,0,         // wheat
    220,200,120,  120,90,60,  0,0,0,          // cure disease (yellow)
    80,140,60,  60,60,60,  0,0,0,             // weak poison (green, dark stopper)
    70,90,60,  220,210,180,  0,0,0,           // the herbwife's handbook (green binding)
    70,80,60,  150,170,190,  0,0,0,   // deathbell
    226,220,206,  200,190,170,  0,0,0,   // white cap
    150,110,70,  120,90,60,  0,0,0,   // canis root
    70,100,60,  60,70,130,  0,0,0,   // juniper berries
    232,104,48,  120,80,60,  0,0,0,   // fire salts
    84,52,116,  50,40,60,  0,0,0,   // void salts
    196,224,240,  160,190,220,  0,0,0,   // glow dust
    52,48,46,  110,100,90,  0,0,0,   // hagraven feathers
    212,200,150,  180,160,110,  0,0,0,   // troll fat
    176,124,112,  150,100,90,  0,0,0,   // skeever tail
    206,204,176,  120,110,80,  0,0,0,   // spider egg
    214,110,40,  230,200,170,  0,0,0,   // orange dartwing
    60,110,200,  200,220,240,  0,0,0,   // blue dartwing
    196,112,60,  200,184,150,  0,0,0,   // imp stool
    140,118,86,  110,92,66,  0,0,0,   // taproot
    150,170,176,  120,140,150,  0,0,0,   // slaughterfish scales
    222,232,242,  190,210,230,  0,0,0,   // ice wraith teeth
    190,204,214,  90,80,70,  0,0,0,   // warbler egg
    200,226,236,  150,170,180,  100,80,60,   // ectoplasm
    120,170,70,  90,120,60,  100,80,60,   // spriggan sap
    110,96,70,  170,90,80,  0,0,0,   // creep cluster
    220,120,60,  110,80,60,  0,0,0,   // potion of resist fire
    170,210,226,  60,60,70,  0,0,0,   // frostbite venom
    218,176,60,  190,150,50,  0,0,0,     // golden claw
    120,92,60,  210,196,160,  0,0,0,     // Arvel's journal
    104,104,98,  80,80,76,  60,60,58,    // the Dragonstone
    90,60,40,  210,196,160,  0,0,0,      // Thief
    120,180,230,  110,92,70,  0,0,0,     // potion of resist frost
    140,150,150,  80,60,44,  150,200,230, // ancient Nord war axe of frost
    176,96,84,  232,214,196,  0,0,0,     // raw rabbit leg
    196,120,100,  236,220,200,  0,0,0,     // pheasant breast
    140,142,148,  92,64,40,  120,118,114,     // iron mace
    150,152,158,  92,64,40,  110,108,104,     // iron greatsword
    140,142,148,  96,68,42,  120,118,114,     // iron battleaxe
    132,134,140,  96,68,42,  110,108,104,     // iron warhammer
    120,86,52,  200,190,170,  60,44,30,       // hunting bow
    150,112,70,  200,190,170,  80,60,40,      // long bow
    150,120,80,  140,142,148,  210,210,200,   // iron arrow
    120,122,128,  0,0,0,  0,0,0,              // lockpick
    110,108,112,  0,0,0,  0,0,0,              // iron ingot
    186,192,204,  60,44,36,  150,150,160,     // steel dagger
    150,80,170,  70,110,50,  200,190,120,     // purple mountain flower
    200,50,50,  70,110,50,  220,200,120,      // red mountain flower
    170,90,170,  90,110,60,  120,130,80,      // thistle branch
    230,230,220,  110,120,80,  200,200,190,   // tundra cotton
    200,40,40,  220,225,230,  90,110,70,      // snowberries
    150,110,70,  120,86,56,  200,180,150,     // deer hide
    196,180,150,  150,130,100,  0,0,0,        // small antlers
    196,180,150,  150,130,100,  0,0,0,        // large antlers
    196,110,50,  230,220,200,  80,50,30,      // fox pelt
    176,166,150,  130,120,106,  90,80,70,     // goat hide
    170,80,70,  220,200,180,  0,0,0,          // leg of goat
    120,96,70,  80,64,50,  0,0,0,             // mudcrab chitin
    170,200,190,  120,150,140,  0,0,0,        // glass claw (malachite green glass)
    120,112,96,  70,64,56,  150,130,80,       // Rahgot (the mask: dark bronze)
    96,70,46,  200,190,160,  0,0,0,           // Skorm's journal
    150,130,80,  110,96,60,  0,0,0,           // the well's key (iron)
    60,44,34,  200,110,40,  150,120,70,       // the dragon priest's staff (dark wood, its fire)
    206,192,160,  0,0,0,  0,0,0,              // Alchemist's Note (paper)
    200,186,152,  0,0,0,  0,0,0,              // Rahgot's Reply
    210,198,166,  0,0,0,  0,0,0,              // Orders
    204,190,156,  0,0,0,  0,0,0,              // Note (the well)
    198,184,150,  0,0,0,  0,0,0,              // Note (Alduin)
    150,150,140,  200,190,160,  0,0,0,        // The Exodus (Restoration's grey-white tome)
    150,56,36,  200,190,160,  0,0,0           // Spell Tome: Flames (Destruction's red)
    };

static const char* const KCatName[NCATS] = { "FAVORITES", "ALL", "WEAPONS", "APPAREL", "POTIONS", "FOOD", "INGREDIENTS", "BOOKS", "KEYS", "MISC" };

// books: pages separated by '|'
static const char* const KBooks[6] =
    {
    "THE BOOK OF THE DRAGONBORN|A short treatise on the prophecies of the Dragonborn: mortals born with the soul of a dragon, who take a slain dragon's power for their own.|The writer gathers what the temples and the old Nord tales agree on - that such a one appears when the dragons return, and that the Blades once served them.|Whether the coming of a new Dragonborn is a blessing or a warning, the writer leaves for the reader to decide.",
    "DRAGON LANGUAGE: MYTH NO MORE|A scholar's argument that the dragons' tongue was a true language, not a legend.|The old Nords carved its words into walls of stone in their tombs; a few sounds on them can still be read, and some say a word of it, spoken right, has power.|The author urges colleagues to copy the walls before the barrows crumble.",
    "NIGHT OF TEARS|An account of Saarthal, the first city of men in Skyrim, and of the night the elves fell upon it and put it to the torch.|Ysgramor alone of its leaders escaped with his sons, sailed home, and returned with his Five Hundred Companions to drive the elves out.|So began the Nord kingdom, the tale says, out of fire and grief.",
    "HERBALIST'S GUIDE TO SKYRIM|Every root and petal holds something of the land it grew in. Taste carefully, and remember what each one did to you.|Blue mountain flower and wheat mend the body; blisterwort mends it faster, but it bites.|Lavender steadies the breath. Frost mirriam clears the mind. Deathbell and imp stool are for your enemies, never your cup.|Two that share a virtue make a potion. Two that share nothing make a mess.",
    "ARVEL'S JOURNAL|by Arvel the Swift|My fingers are trembling. The Golden Claw is finally in my hands, and with it, the power of the ancient Nordic heroes. That fool Lucan Valerius had no idea that his favorite store decoration was actually the key to Bleak Falls Barrow.|Now I just need to get to the Hall of Stories and unlock the door. The legend says there is a test that the Nords put in place to keep the unworthy away, but that \"When you have the golden claw, the solution is in the palm of your hands.\"",
    "THIEF|A tale of a young thief in the Imperial City who learns the trade from an old master: to watch a mark until you know his habits better than he does, to move only when nobody is looking, and to take only what will not be missed until you are long gone.|The master's last lesson is the hardest one: the best thief is the one nobody ever suspects."
    };

// dialogues: lines "Speaker|text"
#define GAME_LINES_DLG
#include "game_lines.inc"      // (v0.4.75) KDlgLines: the people's lines (see game_lines_stub.inc)
// the named people of the inn (ROLE_NAMED): their names and their line
enum { NNAMED = 29 };
static const char* const KNamedName[NNAMED] = { "Saadia", "Mikael", "Uthgerd the Unbroken", "Sinmir", "Vilja",
                                            "Balgruuf the Greater", "Irileth", "Whiterun Guard", "Farengar Secret-Fire", "Fianna", "Gerda",
                                            "Alvor", "Sigrid", "Hod", "Gerdur", "Faendal", "Sven", "Hilde", "Camilla Valerius",
                                            "Delphine", "Embry", "Dorthe", "Frodnar",
                                            "Skulvar Sable-Hilt", "Jervar", "Severio Pelagia", "Nimriel", "Wilmuth", "Bjorlam" };
const char* GameWrFolkName(int aNamed);
const char* GameNamedName(int i) { const char* n = GameWrFolkName(i); if (i == 53) return "Valmir"; if (i == 54) return "Stormcloak Soldier"; return n ? n : KNamedName[i % NNAMED]; }
// (v0.4.53) the named people's own greetings from Skyrim (gen_kit.py, the export's dialogue.csv): how many a person
// has, and the k-th
#include "kit_dialogue.inc"
int GameKitSayN(int aNamed);
int GameKitSayN(int aNamed) { return aNamed >= 0 && aNamed * 2 + 1 < (int)(sizeof(KKitSayIx) / sizeof(KKitSayIx[0])) ? KKitSayIx[aNamed * 2 + 1] : 0; }
const char* GameKitSay(int aNamed, int k) { int n = GameKitSayN(aNamed); return n ? KKitSay[KKitSayIx[aNamed * 2] + k % n] : ""; }

struct DlgRange { short first, count; };
enum { DLG_ELDER_INTRO = 0, DLG_ELDER_REMIND, DLG_ELDER_WORDS, DLG_ELDER_DRAGON, DLG_ELDER_WIN, DLG_ELDER_AFTER,
       DLG_SMITH_GIFT, DLG_SMITH, DLG_VILLAGER };
static const DlgRange KDlg[] =
    {
    { 0, 3 }, { 10, 1 }, { 10, 1 }, { 10, 1 }, { 10, 1 }, { 10, 1 }, { 11, 2 }, { 13, 1 }, { 14, 1 }
    };

static const char* const KWords[3] = { "FUS", "RO", "DAH" };

// ---------------------------------------------------------------- world layout: CITY_CX / CITY_CZ (game.h)
static const int KWolfPack[3 * 2] = { -300, -160,  260, -220,  -170, 300 };      // metres from Whiterun, on the plains

static int Cw(int c) { return c * CELL + CELL / 2; }
// a point near Vindgard, metres from its centre (the city's own frame)
static int Vx(int mx) { return Cw(CITY_CX) + mx * METER; }
static int Vz(int mz) { return Cw(CITY_CZ) + mz * METER; }
static inline int Abs(int v) { return v < 0 ? -v : v; }
static int Clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// ---------------------------------------------------------------- memory / init
int Game::MemNeeded()
    {
    return R3D::MemNeeded() + ((sizeof(R3D) + 63) & ~63) + MAPW * MAPW * 2 + AREA_KB * 1024 + LIB_KB * 1024 + OUT_W * OUT_H * 2 + DET_ROWS * DET_MAXW + FLORA_MAX / 8 + ITEMBUF_KB * 1024 + ITEMSHADE_KB * 1024 + 64;
    }

void Game::Init(u8* aMem)
    {
    int k;
    for (k = 0; k < INIT_STEPS; k++) InitStep(aMem, k);
    }

// start-up in stages, so the platform can show progress (and the last stage reached if anything fails)
static const char* const KInitName[Game::INIT_STEPS] =
    { "engine and textures", "people and creatures", "city buildings", "interiors", "the land", "map and new game" };
const char* Game::InitStepName(int k) { return k >= 0 && k < INIT_STEPS ? KInitName[k] : ""; }

void Game::InitStep(u8* aMem, int k)
    {
    switch (k)
        {
        case 0:
            {
            iR = (R3D*)aMem;
            u8* p = aMem + ((sizeof(R3D) + 63) & ~63);
            iMapImg = (u16*)p; p += MAPW * MAPW * 2;
            iAreaMem = p; p += AREA_KB * 1024;
            iLibMem = p; p += LIB_KB * 1024; iLibMeshes = 0; iLibMeshN = 0; iKitMeshN = 0; iLidType = -1;
            iFrozen = (u16*)p; p += OUT_W * OUT_H * 2; iFrozenOk = 0;
            iAutoLevel = iAutoAccUs = iAutoN = iAutoGood = 0; iTalks = 0;
            iDet = p; p += DET_ROWS * DET_MAXW; iDetRow0 = -1; iDetOk = 0;
            iFloraTaken = p; p += FLORA_MAX / 8; iFloraN = 0;
            iItemBuf = p; p += ITEMBUF_KB * 1024; iItemShown = -1; iItemTab = 0;
            iItemShade = (u16*)p; p += ITEMSHADE_KB * 1024;
#ifndef __EPOC32__
            if (!iDataRead) iDataRead = HostDataRead;
#endif
            iR->Init(p);
            iCellFloorY = 100 * METER;
            break;
            }
        case 1: BuildMeshes(); break;
        case 2: BuildCityMeshes(); break;
        case 3: BuildInteriors(); break;
        case 4:
            {
#ifndef __EPOC32__
            FILE* f = 0;
            // (kept open: the land is read as the player goes)
            if (!iLandRead && (f = fopen("data/hrimfell.land", "rb")) != 0) { iLandRead = HostLandRead; iLandSeek = HostLandSeek; iLandCtx = f; }
            BuildWorld();
#else
            BuildWorld();
#endif
            break;
            }
        case 5:
            BuildMapImage();
            ShowFps = 0;
            iDirect = 1;
            NewGame();
            iUi = UI_TITLE;
            break;
        }
    }

// the land round the player was loaded (vertices x0..x1, z0..z1): who stands there outside is put on it (they took
// their heights from the coarse land while it was far)
static void GameLandHook(void* aCtx, int aX0, int aZ0, int aX1, int aZ1)
    {
    Game* g = (Game*)aCtx;
    int i;
    for (i = 0; i < g->iS.ne; i++)
        {
        Ent& e = g->iS.e[i];
        if (e.cell != 0 || e.type == E_DRAGON) continue;
        int vx = e.x / CELL, vz = e.z / CELL;
        if (vx < aX0 || vx > aX1 || vz < aZ0 || vz > aZ1) continue;
        // (on what it stood on: a deck, a platform of the barrow high over the land)
        int gr = g->iR->GroundAt(e.x, e.z);
        e.y = g->FloorAt(0, e.x, e.z, e.y + METER / 2 > gr + 2 * METER ? e.y + METER / 2 : gr + 2 * METER);
        }
    }

void Game::BuildWorld()
    {
    R3D& r = *iR;
    int i;
    // Skyrim's own land (data/hrimfell.land, from tools/make_land.py); without it, a made-up one
    r.iLandHook = GameLandHook; r.iLandHookCtx = this;
    if (!iLandRead || !r.LoadLand(iLandRead, iLandSeek, iLandCtx))
        {
        r.SetWorldSize(WORLD_GX, WORLD_GZ);
        r.GenTerrain(1);
        static const short KMount[6 * 4] = { 900, 700, 260, 520,   1500, 1100, 300, 700,   300, 1100, 220, 480,
                                             1500, 300, 260, 600,   1100, 1300, 200, 450,   600, 420, 140, 380 };
        for (i = 0; i < 6; i++) r.AddHeight(KMount[i * 4] * CELL, KMount[i * 4 + 1] * CELL, KMount[i * 4 + 2] * CELL, KMount[i * 4 + 3] * M);
        }
    iNumRamps = 0;
    // each part of the world is built with the land round it in the window (its things take their heights from it)
    r.FocusLand(Vx(0), Vz(0));
    if (!iWrKit) BuildCityTerrain();       // (v0.4.57: Skyrim's Whiterun - its own land is in the land file)
    // rocks scattered on the plains round Whiterun
    for (i = 0; i < 140; i++)
        {
        u32 h = r3d_hash(i, 5, 777);
        int x = Vx(-420 + (int)(h % 840)), z = Vz(-420 + (int)((h >> 16) % 840));
        int dx = (x - Vx(-20)) >> 8, dz = (z - Vz(0)) >> 8;
        if (dx * dx + dz * dz < 140 * 140) continue;      // not in or round the city
        if (r.GroundAt(x, z) < r.WaterAt(x, z) + M) continue;
        r.AddStatic(ST_ROCK, x, z, (int)(h & 1023), 120 + (int)((h >> 24) & 127), (int)(h >> 7) & 1);
        }
    BuildRiverwood();
    r.FocusLand(Vx(0), Vz(0));
    if (!iWrKit) BuildCity();              // (v0.4.57: Skyrim's Whiterun, from data/whiterun.har)
    r.FinishWorld();
    }

// the world map: MAPW pixels across the whole land, as many down as its shape needs, north up
int Game::MapImgH() const { return iR->iGZ * MAPW / iR->iGX; }
void Game::BuildMapImage()
    {
    R3D& r = *iR;
    int mh = MapImgH();
    int x, z;
    for (z = 0; z < mh; z++)
        for (x = 0; x < MAPW; x++)
            {
            int gx = x * r.iGX / MAPW + r.iGX / (2 * MAPW), gz = (mh - 1 - z) * r.iGZ / mh + r.iGZ / (2 * mh);
            int cr, cg, cb, lev;
            r.VertexColour(gx, gz, cr, cg, cb, lev);
            // lit from the north-west along the slopes (the mountains read as relief), then a parchment tint
            int s = r.iGX / MAPW + 1;
            int sl = (r.Hgt(gx - s, gz + s) - r.Hgt(gx + s, gz - s)) * HS * 3 / (4 * M);
            if (sl > 40) sl = 40;
            if (sl < -40) sl = -40;
            cr += sl; cg += sl; cb += sl;
            cr = (cr * 3 + 200) >> 2; cg = (cg * 3 + 180) >> 2; cb = (cb * 3 + 140) >> 2;
            if (cr < 0) cr = 0; if (cg < 0) cg = 0; if (cb < 0) cb = 0;
            if (cr > 255) cr = 255; if (cg > 255) cg = 255; if (cb > 255) cb = 255;
            iMapImg[z * MAPW + x] = R3D::Rgb(cr, cg, cb);
            }
    }

int Game::SpawnEnt(int aType, int aX, int aZ, int aYaw)
    {
    if (iS.ne >= MAXE) return -1;
    Ent& e = iS.e[iS.ne];
    r3d_memset(&e, 0, sizeof(Ent));
    e.type = (u8)aType; e.x = aX; e.z = aZ; e.hx = aX; e.hz = aZ; e.yaw = aYaw;
    e.y = iR->GroundAt(aX, aZ);
    e.alive = 1;
    e.state = S_IDLE;
    int hp = 30;
    switch (aType)
        {
        case E_WOLF: hp = 40; break;
        case E_WIGHT: hp = 70; break;
        case E_SKELETON: hp = 30; break;
        case E_BANDIT: hp = 50; break;
        case E_SKEEVER: hp = 15; break;
        case E_SPIDER: hp = 120; break;
        case E_TROLL: hp = 200; break;
        case E_DRAGON: hp = 650; e.state = D_SLEEP; break;
        case E_CHEST: hp = 1; break;
        case E_DEER: hp = 30; break;
        case E_HARE: hp = 4; break;
        case E_FOX: hp = 15; break;
        case E_GOAT: hp = 20; break;
        case E_MUDCRAB: hp = 25; break;
        default: hp = 60; break;
        }
    e.hp = e.maxhp = (short)hp;
    e.anim = (int)(r3d_hash(aX, aZ, 1) & 0xFFFF);
    e.timer = (int)(r3d_hash(aX, aZ, 2) & 0x3FFFFF);
    return iS.ne++;
    }

void Game::NewGame()
    {
    UseArea(0);                             // (v0.4.55: the main area's file - its houses' things are put out)
    int gamma = iS.gamma, view = iS.view;   // display settings: kept across new games
    r3d_memset(&iS, 0, sizeof(iS));
    r3d_memset(iP, 0, sizeof(iP));
    iS.magic = 0x48524D46; iS.version = SAVE_VERSION;
    iS.gamma = gamma & 3; iS.view = view;
    iS.px = Vx(-86); iS.pz = Vz(-102);
    iR->FocusLand(iS.px, iS.pz);
    iS.py = iR->GroundAt(iS.px, iS.pz) + 410;
    iS.yaw = 0; iS.pitch = 0;
    iS.hpMax = 100; iS.mpMax = 100; iS.spMax = 100;
    iS.hp = 100 << 8; iS.mp = 100 << 8; iS.sp = 100 << 8;
    iS.gold = 25;
    iS.timeOfDay = 9 * 3600 * 1000 / 60;   // game time in units of 60 ms (1 unit = 1 game "minute/1000")
    iS.outMode = 0;
    iS.cell = 0; iS.day = 0; iS.weather = W_CLEAR; iS.weatherHours = 0; iS.third = 0; iS.drawn = 0;
    iR->iCell = 0;
    ApplyViewSetting();
    iPrevWeather = W_CLEAR; iWBlend = 256;
    iDrawUs = 0; iCamDist = 0; iWaitSel = 1; iWaitLeft = 0; iSleeping = 0; iFadeUs = 0;
    iUsePending = -1; iGrab = -1; iGrabArmed = 0; iFireLock = 0; iOpenWaitUs = 0; iPropTickUs = 0; iSitting = -1; iOpenProp = -1; iTradeEnt = -1; iCookMode = 0;
    iS.nprop = 0; iS.ncont = 0;
    int i;
    // Hrolf the steward before Highhall, Sigrun at her forge in the Plains District, townsfolk about the streets
    // (v0.4.65: in Skyrim's Whiterun, Proventus Avenicci at the Jarl's side in Dragonsreach, Adrianne at her forge by
    // Warmaiden's - placed with the area, below - and the city's own people instead of Vindgard's five)
    {
    int st = SpawnEnt(E_ELDER, Vx(20), Vz(76), 640);
    if (iWrKit && st >= 0)
        {
        Ent& e = iS.e[st];
        e.cell = CELL_HIGHHALL; e.x = e.hx = 100 * M - 273 * M / 100; e.z = e.hz = 100 * M + 4560 * M / 100; e.yaw = 512;
        e.y = FloorAt(CELL_HIGHHALL, e.x, e.z, 100 * M + 390 * M / 100 + M / 2);
        }
    }
    SpawnEnt(E_SMITH, Vx(-51), Vz(-15), 768);
    static const int KFolk[5 * 2] = { -35, -10,  -8, -10,  -10, 14,  40, 30,  0, 52 };
    for (i = 0; i < (iWrKit ? 0 : 5); i++)
        {
        int idx = SpawnEnt(E_VILLAGER, Vx(KFolk[i * 2]), Vz(KFolk[i * 2 + 1]), i * 205);
        if (idx >= 0) iS.e[idx].var = (u8)i;
        }
    // Arnvid the stablemaster, at home in the stables residence (v0.4.48: only without Skyrim's own Stables - their
    // Skulvar Sable-Hilt and Jervar instead)
    if (!iWrKit && !GameIsKitCell(*this, CELL_STABLES))
    {
    int idx = SpawnEnt(E_VILLAGER, 100 * M - 3 * M, 100 * M + M, 300);
    if (idx >= 0)
        {
        Ent& e = iS.e[idx];
        e.cell = CELL_STABLES; e.role = ROLE_STABLE; e.var = 2;
        e.y = iCellFloorY;
        }
    }
    // (v0.4.67: the catacombs' skeletons, the barrow's people and creatures and the city's people come out with
    //  their places: game_stage.cpp)
    // Highhall's court and furnishings
    SpawnHighhall();
    // Bleak Falls Barrow: its bandits, creatures, dead, traps and treasure
    SpawnDungeons();
    // market merchants, goods, barrels, chairs, harvestables, the cooking pot
    SpawnMerchants();
    SpawnProps();
    SpawnRiverwood();
    SpawnWhiterun();
    SpawnRwTrader();
    SpawnSgInn();
    SpawnKit();
    { void GameSgInnRentBed(Game& g); GameSgInnRentBed(*this); }
    UpdateStage();                             // (v0.4.67: the groups where the player starts)
    RiverwoodSchedule();
    if (iWrKit)
        {
        UseArea(2); SpawnWhiterun();                   // (v0.4.57: Whiterun's own area: its houses' things, the outskirts' folk)
        // Adrianne Avenicci at her forge (WhiterunAdrianneSandboxForgePackage6x11: by the grindstone, Skyrim 20302, -8080)
        for (i = 0; i < iS.ne; i++)
            {
            Ent& e = iS.e[i];
            if (e.type != E_SMITH) continue;
            e.x = e.hx = (int)(((long long)20302 + 233472) * 36576 / 10000); e.z = e.hz = (int)(((long long)-8080 + 176128) * 36576 / 10000);
            e.y = FloorAt(0, e.x, e.z, iR->GroundAt(e.x, e.z) + 3 * M / 2); e.yaw = 768;
            }
        }
    WhiterunSchedule(1);                       // (the outskirts' people where the hour has them, from the start)
    // wolves
    for (i = 0; i < 3; i++)
        {
        int k;
        for (k = 0; k < 3; k++)
            SpawnEnt(E_WOLF, Vx(KWolfPack[i * 2]) + (k - 1) * 3 * M, Vz(KWolfPack[i * 2 + 1]) + (k & 1) * 3 * M, k * 300);
        }
    // starting inventory
    iS.ninv = 0;
    AddItem(IT_IRON_SWORD, 1);
    AddItem(IT_FUR_ARMOR, 1);
    AddItem(IT_FUR_BOOTS, 1);
    for (i = 0; i < iS.ninv; i++) iS.inv[i].equipped = 1;
    AddItem(IT_POT_MHEAL, 3);
    AddItem(IT_POT_MMAG, 1);
    AddItem(IT_BREAD, 2);
    AddItem(IT_APPLE, 1);
    AddItem(IT_TORCH, 2);
    iDragonIdx = -1;
    iUi = UI_PLAY;
    iDlgId = -1;
    iTarget = -1;
    iInvCat = 1;
    for (i = 0; i < MAXMSG; i++) iMsgUs[i] = 0;
    }

// ---------------------------------------------------------------- inventory helpers
int Game::AddItem(int aId, int aCount)
    {
    int i;
    for (i = 0; i < iS.ninv; i++)
        if (iS.inv[i].id == aId && !(KItems[aId].equip && iS.inv[i].equipped))
            {
            iS.inv[i].count = (short)(iS.inv[i].count + aCount);
            return i;
            }
    if (iS.ninv >= MAXINV) return -1;
    InvItem& it = iS.inv[iS.ninv];
    it.id = (u8)aId; it.count = (short)aCount; it.equipped = 0; it.fav = 0;
    return iS.ninv++;
    }

void Game::RemoveItem(int aIdx, int aCount)
    {
    if (aIdx < 0 || aIdx >= iS.ninv) return;
    iS.inv[aIdx].count = (short)(iS.inv[aIdx].count - aCount);
    if (iS.inv[aIdx].count <= 0)
        {
        int i;
        for (i = aIdx; i < iS.ninv - 1; i++) iS.inv[i] = iS.inv[i + 1];
        iS.ninv--;
        }
    }

int Game::EquippedId(int aSlot)
    {
    int i;
    for (i = 0; i < iS.ninv; i++)
        if (iS.inv[i].equipped && KItems[iS.inv[i].id].equip == aSlot) return iS.inv[i].id;
    return -1;
    }

int Game::WeaponDamage()
    {
    int w = EquippedId(EQ_WEAPON);
    return w >= 0 ? KItems[w].power * 2 : 4;      // fists
    }

int Game::ArmorRating()
    {
    int i, a = 0;
    for (i = 0; i < iS.ninv; i++)
        if (iS.inv[i].equipped && KItems[iS.inv[i].id].equip >= EQ_BODY) a += KItems[iS.inv[i].id].power;
    return a;
    }

int Game::CarryWeight10()
    {
    int i, w = 0;
    for (i = 0; i < iS.ninv; i++) w += KItems[iS.inv[i].id].weight10 * iS.inv[i].count;
    return w;
    }

void Game::UseItem(int aIdx)
    {
    if (aIdx < 0 || aIdx >= iS.ninv) return;
    InvItem& it = iS.inv[aIdx];
    const ItemDef& d = KItems[it.id];
    if (d.equip)
        {
        if (it.equipped) { it.equipped = 0; Msg2(d.name, " unequipped"); return; }
        int i;
        for (i = 0; i < iS.ninv; i++)
            if (iS.inv[i].equipped && KItems[iS.inv[i].id].equip == d.equip) iS.inv[i].equipped = 0;
        if (it.count > 1)
            {
            // split one off so a single copy is equipped
            it.count--;
            if (iS.ninv < MAXINV)
                {
                InvItem n = it;
                n.count = 1; n.equipped = 1;
                iS.inv[iS.ninv++] = n;
                }
            }
        else it.equipped = 1;
        Msg2(d.name, " equipped");
        return;
        }
    if (d.cat == IC_BOOK)
        {
        iBookId = d.book; iBookPage = 0; iBookProp = -1; iUi = UI_BOOK;
        return;
        }
    if (d.effect)
        {
        int amt = d.power << 8;
        if (d.effect == PW_HEAL) iS.hp = Clamp(iS.hp + amt, 0, iS.hpMax << 8);
        if (d.effect == PW_MAGICKA) iS.mp = Clamp(iS.mp + amt, 0, iS.mpMax << 8);
        if (d.effect == PW_STAMINA) iS.sp = Clamp(iS.sp + amt, 0, iS.spMax << 8);
        Msg2(d.cat == IC_POTION ? "Drank " : "Ate ", d.name);
        RemoveItem(aIdx, 1);
        return;
        }
    Msg("Nothing happens.");
    }

void Game::DrinkPotion(int aEffect)
    {
    // best matching potion (largest restore) - like the favourites hotkey
    int i, best = -1, bp = 0;
    for (i = 0; i < iS.ninv; i++)
        {
        const ItemDef& d = KItems[iS.inv[i].id];
        if (d.cat == IC_POTION && d.effect == aEffect && d.power > bp) { bp = d.power; best = i; }
        }
    if (best < 0) { Msg("No healing potions."); return; }
    UseItem(best);
    }

int GameDunLoot(Game& g, Ent& e);
void Game::GiveLoot(Ent& e)
    {
    if (e.looted & 1) { Msg("Nothing more to take."); return; }
    e.looted |= 1;
    if (GameDunLoot(*this, e)) return;
    u32 h = r3d_hash(e.hx, e.hz, e.type);
    switch (e.type)
        {
        case E_WOLF:
            AddItem(IT_WOLF_PELT, 1); Msg("Wolf Pelt added");
            break;
        // the land's animals, as Skyrim's: venison and a hide (a buck's antlers), a hare's leg, a fox's pelt, a goat's
        // hide and leg, a mudcrab's chitin
        case E_DEER:
            AddItem(IT_RAW_VENISON, 1); AddItem(IT_DEER_HIDE, 1); Msg("Venison added"); Msg("Deer Hide added");
            if (e.var || (h & 1)) { AddItem(e.var ? IT_LARGE_ANTLERS : IT_SMALL_ANTLERS, 1); Msg(e.var ? "Large Antlers added" : "Small Antlers added"); }
            break;
        case E_HARE: AddItem(IT_RAW_RABBIT, 1); Msg("Raw Rabbit Leg added"); break;
        case E_FOX: AddItem(IT_FOX_PELT, 1); Msg("Fox Pelt added"); break;
        case E_GOAT: AddItem(IT_GOAT_HIDE, 1); AddItem(IT_GOAT_LEG, 1); Msg("Goat Hide added"); Msg("Leg of Goat added"); break;
        case E_MUDCRAB: AddItem(IT_MUDCRAB_CHITIN, 1 + (int)(h & 1)); Msg("Mudcrab Chitin added"); break;
        case E_WIGHT:
            {
            int g = 5 + (int)(h % 20);
            iS.gold += g;
            Msg("Gold added");
            if ((h >> 4) % 3 == 0) { AddItem(IT_BARROW_BLADE, 1); Msg("Ancient Nord Sword added"); }
            else if ((h >> 4) % 3 == 1) { AddItem(IT_POT_MHEAL, 1); Msg("Potion of Minor Healing added"); }
            break;
            }
        case E_SKELETON:
            {
            // bone meal, and now and then the old blade it carried (different each time it rises again)
            u32 k = r3d_hash(e.hx, e.cool, 77);
            if (k & 1) { AddItem(IT_BONE_MEAL, 2); Msg("Bone Meal (2) added"); }
            else { AddItem(IT_BONE_MEAL, 1); Msg("Bone Meal added"); }
            if ((k >> 3) % 4 == 0) { AddItem(IT_BARROW_BLADE, 1); Msg("Ancient Nord Sword added"); }
            break;
            }
        case E_TROLL:
            AddItem(IT_TROLL_FAT, 2); Msg("Troll Fat (2) added");
            break;
        case E_DRAGON:
            AddItem(IT_DRAGON_SCALE, 2); iS.gold += 200;
            Msg("Dragon Scale (2) added"); Msg("200 Gold added");
            break;
        }
    }

void Game::OpenChest(Ent& e)
    {
    if (e.looted) { Msg("The chest is empty."); return; }
    e.looted = 1;
    iS.chestOpen[e.var & 3] = 1;
    switch (e.var)
        {
        case 0: AddItem(IT_STEEL_SWORD, 1); AddItem(IT_POT_HEAL, 1); AddItem(IT_LICHEN, 2); iS.gold += 40;
                Msg("Steel Sword added"); Msg("Potion of Healing added"); Msg("40 Gold added"); break;
        case 1: AddItem(IT_IRON_HELMET, 1); AddItem(IT_POT_MMAG, 2); AddItem(IT_SILVER_RING, 1); iS.gold += 35;
                Msg("Iron Helmet added"); Msg("Potion of Minor Magicka (2) added"); Msg("Silver Ring added"); break;
        case 2: AddItem(IT_IRON_ARMOR, 1); AddItem(IT_IRON_GAUNTLETS, 1); AddItem(IT_POT_MSTAM, 2); iS.gold += 50;
                Msg("Iron Armor added"); Msg("Iron Gauntlets added"); Msg("50 Gold added"); break;
        default: AddItem(IT_IRON_SHIELD, 1); AddItem(IT_IRON_BOOTS, 1); AddItem(IT_POT_HEAL, 2); iS.gold += 150;
                Msg("Iron Shield added"); Msg("Iron Boots added"); Msg("150 Gold added"); break;
        }
    }

// ---------------------------------------------------------------- messages / dialogue
void Game::Msg(const char* aS)
    {
    int i;
    for (i = MAXMSG - 1; i > 0; i--)
        {
        int k;
        for (k = 0; k < 56; k++) iMsg[i][k] = iMsg[i - 1][k];
        iMsgUs[i] = iMsgUs[i - 1];
        }
    int n = 0;
    while (aS[n] && n < 55) { iMsg[0][n] = aS[n]; n++; }
    iMsg[0][n] = 0;
    iMsgUs[0] = 4000000;
    }

void Game::Msg2(const char* aA, const char* aB)
    {
    char b[56];
    int n = 0;
    while (*aA && n < 55) b[n++] = *aA++;
    while (*aB && n < 55) b[n++] = *aB++;
    b[n] = 0;
    Msg(b);
    }

void Game::StartDialog(int aId)
    {
    iDlgId = aId; iDlgPage = 0; iUi = UI_DIALOG;
    }

int GameDunDlgCount(int aId);
void Game::DialogNext()
    {
    iDlgPage++;
    if (iDlgId >= 100)
        {
        if (iDlgPage < GameDunDlgCount(iDlgId - 100)) return;
        int id = iDlgId - 100;
        iUi = UI_PLAY; iDlgId = -1;
        DunDialogDone(id);
        return;
        }
    if (iDlgPage < KDlg[iDlgId].count) return;
    // dialogue finished: consequences
    switch (iDlgId)
        {
        case DLG_SMITH_GIFT:
            if (!iS.smithGift)
                {
                iS.smithGift = 1;
                AddItem(IT_FUR_HELMET, 1); AddItem(IT_HIDE_SHIELD, 1);
                Msg("Fur Helmet added"); Msg("Hide Shield added");
                }
            break;
        }
    iUi = UI_PLAY;
    iDlgId = -1;
    }

// ---------------------------------------------------------------- quest
int GameBarrowX();
int GameBarrowZ();
const char* GameFhObjective(const Game& g);
const char* Game::QuestText()
    {
    // (v0.4.69) Siege on the Dragon Cult, while in or near Forelhost
    {
    const char* f = GameFhObjective(*this);
    int px = iS.cell ? iS.extX : iS.px, pz = iS.cell ? iS.extZ : iS.pz;
    if (f && ((iS.cell >= CELL_FHSTRONG && iS.cell <= CELL_FHREFECT) || (Abs(px - (int)(((long long)182930 + 233472) * 36576 / 10000)) < 600 * M && Abs(pz - (int)(((long long)-113715 + 176128) * 36576 / 10000)) < 600 * M))) return f;
    }
    // Skyrim's quests to Bleak Falls Barrow, while they are open
    int qb = iS.bfb[BF_QBLEAK];
    if (qb >= 10 && qb < 180) return "Bleak Falls Barrow: retrieve the Dragonstone";
    if (qb == 180) return "Bleak Falls Barrow: bring the Dragonstone to Farengar";
    if (iS.bfb[BF_QCLAW] == 12) return "The Golden Claw: find the claw in Bleak Falls Barrow";
    if (iS.bfb[BF_QCLAW] == 10) return "The Golden Claw: retrieve Lucan's claw from Bleak Falls Barrow";
    if (iS.bfb[BF_QCLAW] == 50 && iS.bfb[BF_CLAWDOOR]) return "The Golden Claw: return the claw to Lucan in Riverwood";
    if (qb == 0) return "Speak to Farengar in Dragonsreach";
    return "Explore Skyrim";
    }

void Game::QuestTargets(int* aX, int* aZ, int& aN)
    {
    aN = 0;
    int qb = iS.bfb[BF_QBLEAK];
    if (iS.bfb[BF_QCLAW] == 50 && iS.bfb[BF_CLAWDOOR] && !(qb >= 10 && qb < 180))
        {
        // Lucan: outside, the Trader's door; inside, the man himself
        if (iS.cell == 0) { const DoorLink& dr = Door(11); aX[0] = dr.ox; aZ[0] = dr.oz; aN = 1; return; }
        int i;
        for (i = 0; i < iS.ne; i++)
            if (iS.e[i].cell == iS.cell && iS.e[i].role == ROLE_MERCHANT && iS.e[i].pad1 == MER_RWTRADER) { aX[0] = iS.e[i].x; aZ[0] = iS.e[i].z; aN = 1; }
        return;
        }
    if ((qb >= 10 && qb < 180) || iS.bfb[BF_QCLAW] == 12 || iS.bfb[BF_QCLAW] == 10)
        {
        if (iS.cell == 0) { aX[0] = GameBarrowX(); aZ[0] = GameBarrowZ(); aN = 1; }
        return;
        }
    if (qb == 180)
        {
        // Farengar in Dragonsreach: outside, its great doors; inside, the man himself
        if (iS.cell == 0) { const DoorLink& dr = Door(6); aX[0] = dr.ox; aZ[0] = dr.oz; aN = 1; return; }
        int i;
        for (i = 0; i < iS.ne; i++)
            if (iS.e[i].cell == iS.cell && iS.e[i].role == ROLE_NAMED && iS.e[i].pad1 == 8 && iS.e[i].type == E_VILLAGER) { aX[0] = iS.e[i].x; aZ[0] = iS.e[i].z; aN = 1; }
        return;
        }
    if (qb == 0)
        {
        // Farengar: outside, Dragonsreach's doors; inside, the man himself
        if (iS.cell == 0) { const DoorLink& dr = Door(6); aX[0] = dr.ox; aZ[0] = dr.oz; aN = 1; return; }
        int i;
        for (i = 0; i < iS.ne; i++)
            if (iS.e[i].cell == iS.cell && iS.e[i].role == ROLE_NAMED && iS.e[i].pad1 == 8 && iS.e[i].type == E_VILLAGER) { aX[0] = iS.e[i].x; aZ[0] = iS.e[i].z; aN = 1; }
        }
    }

// ---------------------------------------------------------------- atmosphere (day / night)
void Game::UpdateAtmosphere()
    {
    // timeOfDay: 0..1 440 000 (minutes * 1000)
    int t = iS.timeOfDay / 1000;                // minutes since midnight
    // daylight 0..256: full from 7:00 to 18:00, ramps at dawn/dusk, night floor
    int d;
    if (t < 5 * 60 || t > 21 * 60) d = 0;
    else if (t < 7 * 60) d = (t - 5 * 60) * 256 / 120;
    else if (t < 18 * 60) d = 256;
    else d = (21 * 60 - t) * 256 / 180;
    // dusk/dawn warmth
    int warm = 0;
    if (t > 5 * 60 && t < 8 * 60) warm = 256 - Abs(t - (6 * 60 + 30)) * 256 / 90;
    if (t > 17 * 60 && t < 21 * 60) warm = 256 - Abs(t - 19 * 60) * 256 / 120;
    if (warm < 0) warm = 0;
    int light = 92 + (d * 164 >> 8);
    int fr = 24 + ((178 - 24) * d >> 8), fg = 30 + ((196 - 30) * d >> 8), fb = 52 + ((218 - 52) * d >> 8);
    fr += (210 - fr) * warm >> 9; fg += (140 - fg) * warm >> 9; fb += (110 - fb) * warm >> 9;
    int zr = 6 + ((70 - 6) * d >> 8), zg = 8 + ((110 - 8) * d >> 8), zb = 22 + ((190 - 22) * d >> 8);
    int stars = 255 - d * 2;
    if (stars < 0) stars = 0;
    if (iS.cell)
        {
        // indoors: firelit, the far corners fade into warm darkness
        iR->SetAtmosphere(240, 58, 44, 32, 58, 44, 32, 0);
        // (v0.4.74) a house's light is its hearth's and candles' - Skyrim's cool plaster and grey wood warm, as there;
        // the ruins and catacombs keep their cold stone light
        if (IsDungeon(iS.cell) || iS.cell == CELL_CATACOMBS) iR->SetLightColour(256, 256, 256);
        else iR->SetLightColour(256, 214, 164);
        iR->SetFog(14, 44);
        return;
        }
    // weather: dim the light, grey the sky, pull the fog in
    int wl, grey, f0, f1;
    WeatherParams(wl, grey, f0, f1);
    light = light * wl >> 8;
    int lum = (fr * 77 + fg * 150 + fb * 29) >> 8;
    lum = lum * (180 + (wl >> 2)) >> 8;
    fr += (lum - fr) * grey >> 8; fg += (lum - fg) * grey >> 8; fb += (lum + 6 - fb) * grey >> 8;
    zr += (fr - zr) * grey >> 8; zg += (fg - zg) * grey >> 8; zb += (fb - zb) * grey >> 8;
    stars = stars * (256 - grey) >> 8;
    iR->SetLightColour(256, 256, 256);
    iR->SetAtmosphere(light, fr, fg, fb, zr, zg, zb, stars);
    iR->SetFog(f0, f1);
    // the sun's path: rises in the east at 6:00, highest in the south at noon (55 degrees), sets in the west
    int th = (t - 6 * 60) * 512 / (12 * 60);
    int up = th > 0 && th < 512;
    int ths = Clamp(th, 40, 472);                       // at night the land keeps a soft low light
    int se = r3d_sin(156), ce = r3d_cos(156);           // 55 degrees
    int sdx = r3d_cos(ths), sdy = (r3d_sin(ths) * se) >> 14, sdz = -((r3d_sin(ths) * ce) >> 14);
    iR->SetSun(sdx, sdy, sdz, 140 + (light * 116 >> 8));
    iR->iSunVis = up ? (256 - grey) * Clamp(th < 256 ? th * 4 : (512 - th) * 4, 0, 256) >> 8 : 0;
    // clouds by the weather (blended through a change), coloured by the time of day
    static const int KCover[W_COUNT] = { 80, 200, 0, 240, 230 };
    int ca = KCover[iPrevWeather % W_COUNT], cb = KCover[iS.weather % W_COUNT];
    int cover = ca + ((cb - ca) * iWBlend >> 8);
    int cr = 236, cg = 238, cbl = 244;
    cr += (250 - cr) * warm >> 8; cg += (190 - cg) * warm >> 8; cbl += (160 - cbl) * warm >> 8;
    cr = cr * (60 + (d * 196 >> 8)) >> 8; cg = cg * (64 + (d * 192 >> 8)) >> 8; cbl = cbl * (80 + (d * 176 >> 8)) >> 8;
    int gl = (cr + cg + cbl) / 3;
    cr += (gl * 7 / 10 - cr) * grey >> 8; cg += (gl * 7 / 10 - cg) * grey >> 8; cbl += (gl * 3 / 4 - cbl) * grey >> 8;
    iR->iCloud = cover; iR->iCloudR = Clamp(cr, 0, 255); iR->iCloudG = Clamp(cg, 0, 255); iR->iCloudB = Clamp(cbl, 0, 255);
    }

// ---------------------------------------------------------------- player
void Game::HurtPlayer(int aDmg, int aFromX, int aFromZ)
    {
    if (iUi == UI_DEAD) return;
    int ar = ArmorRating();
    int red = ar * 12 / 10;                      // % reduction (cap 80)
    if (red > 80) red = 80;
    int dmg = aDmg * (100 - red) / 100;
    if (dmg < 1) dmg = 1;
    iS.hp -= dmg << 8;
    iHurtUs = 700000;
    iHurtFrom = r3d_atan2(aFromX - iS.px, aFromZ - iS.pz);          // (the red edge shows where it came from)
    iShakeUs = 200000;
    // knock the player back a little
    int dx = iS.px - aFromX, dz = iS.pz - aFromZ;
    int l = Abs(dx) + Abs(dz);
    if (l > 0)
        {
        int nx = iS.px + dx * (M / 3) / l, nz = iS.pz + dz * (M / 3) / l;
        if (!BlockedAt(iS.cell, nx, nz, 90, iS.py - 410)) { iS.px = nx; iS.pz = nz; }
        }
    if (iS.hp <= 0)
        {
        iS.hp = 0;
        iS.deaths++;
        iUi = UI_DEAD;
        iSwingUs = 0;
        }
    }

void Game::Respawn()
    {
    iS.cell = 0; iR->iCell = 0;
    iS.px = Vx(-86); iS.pz = Vz(-102); iS.yaw = 0;
    iS.py = iR->GroundAt(iS.px, iS.pz) + 410;
    iS.hp = iS.hpMax << 8; iS.mp = iS.mpMax << 8; iS.sp = iS.spMax << 8;
    iS.gold -= iS.gold / 10;
    int i;
    for (i = 0; i < iS.ne; i++)
        {
        Ent& e = iS.e[i];
        if (e.alive && e.type >= E_WOLF && e.type <= E_TROLL)
            {
            e.state = S_RETURN; e.aggro = 0;
            e.hp = e.maxhp;
            }
        if (e.type == E_DRAGON && e.alive && e.state != D_SLEEP) { e.state = D_FLY; e.hp = (short)(e.hp + 100 > e.maxhp ? e.maxhp : e.hp + 100); }
        }
    for (i = 0; i < MAXP; i++) iP[i].active = 0;
    iUi = UI_PLAY;
    Msg("You wake on the road below Whiterun.");
    }

// (v0.4.74) whether a stride from (aX, aZ) to (aNx, aNz) may climb what is there (the walker's rule, and WaysOut's): no
// face steeper than ~50 degrees; a step of Skyrim's stairs indoors; outside up onto a deck, a porch, a boardwalk, a
// stone, a log or a root; a lip in rough land when the ground half a metre on is no steeper. aMx, aMz: the way (Q14)
int Game::ClimbOk(int cell, int aX, int aZ, int nx, int nz, int feet, int ground, int mx, int mz)
    {
    int g2 = FloorAt(cell, nx, nz, feet);
    int run = Abs(nx - aX) + Abs(nz - aZ) + 1;
    int climbOk = (g2 - ground) < run * 6 / 5 || g2 < ground;
    if (!climbOk && g2 - ground < 45 * M / 100)
        {
        // a step of Skyrim's stairs (20 units), indoors; outside, up onto a deck, a porch or a boardwalk from the
        // land (their edges stand up to 45 cm over it)
        int found = 0;
        if (!cell) GameKitOutFloor(*this, nx, nz, feet, iR->iLandLoaded ? iR->iLandOffset : 0, &found);
        if ((GameIsKitCell(*this, cell) && g2 - ground < 33 * M / 100) || found) climbOk = 1;
        // outdoors: up onto a stone, a log or a root (whatever's top is the new floor), as one steps over them
        if (!cell && !climbOk && iR->TopAt(nx, nz, feet + 45 * M / 100) >= g2 - 8) climbOk = 1;
        }
    if (!climbOk && !cell)
        {
        // a lip in rough land: fine if the ground half a metre on is no steeper than you can climb
        int ax = aX + (mx * (M / 2) >> 14), az = aZ + (mz * (M / 2) >> 14);
        climbOk = FloorAt(cell, ax, az, feet) - ground < (M / 2) * 6 / 5 && g2 - ground < M / 3;
        }
    return climbOk;
    }

void Game::UpdatePlayer(int dt)
    {
    int dtMs = dt / 1000;
    u32 k = iKeys;
    // look: joystick acts like a mouse - starts gentle, accelerates while held
    if (k & (KEY_LOOK_L | KEY_LOOK_R | KEY_LOOK_U | KEY_LOOK_D)) iLookHoldUs += dt;
    else iLookHoldUs = 0;
    int accel = 110 + (iLookHoldUs > 500000 ? 146 : iLookHoldUs * 146 / 500000);   // /256
    int yawSp = 330 * accel >> 8;           // units per second (1024 = 360 deg)
    int pitSp = 220 * accel >> 8;
    if (k & KEY_LOOK_L) iS.yaw -= yawSp * dtMs / 1000;
    if (k & KEY_LOOK_R) iS.yaw += yawSp * dtMs / 1000;
    if (k & KEY_LOOK_U) iS.pitch += pitSp * dtMs / 1000;
    if (k & KEY_LOOK_D) iS.pitch -= pitSp * dtMs / 1000;
    iS.yaw &= 1023;
    iS.pitch = Clamp(iS.pitch, -230, 230);
    if (iSitting >= 0) return;                  // seated: look around only
    // movement
    int cell = iS.cell;
    int feet = iS.py - 410;
    int ground = FloorAt(cell, iS.px, iS.pz, feet);
    int wl = cell == 0 ? iR->WaterAt(iS.px, iS.pz) : -(1 << 28);
    int swimming = cell == 0 && ground < wl - M;
    int heavy = CarryWeight10() > 3000;
    int crouch = (iCrouch || (k & KEY_CROUCH)) && !swimming;
    int running = (k & KEY_RUN) && iS.sp > (2 << 8) && !swimming && !heavy && !crouch;
    int spd = running ? 13 * M / 2 : (crouch ? 2 * M : 7 * M / 2);      // per second (crouching: slow and quiet)
    {
    int want = crouch ? 150 : 0;                    // (the eyes 60 cm lower)
    int st = 600 * dt / 1000000 + 1;
    iCrouchY += iCrouchY < want ? (want - iCrouchY < st ? want - iCrouchY : st) : -(iCrouchY - want < st ? iCrouchY - want : st);
    }
    if (swimming) spd = 2 * M;
    if (iSwingUs > 0) spd = spd * 3 / 4;
    int fx = r3d_sin(iS.yaw), fz = r3d_cos(iS.yaw);
    int mx = 0, mz = 0;
    if (k & KEY_FWD) { mx += fx; mz += fz; }
    if (k & KEY_BACK) { mx -= fx * 3 / 4; mz -= fz * 3 / 4; }
    if (k & KEY_STRAFE_R) { mx += fz; mz -= fx; }
    if (k & KEY_STRAFE_L) { mx -= fz; mz += fx; }
    int moving = mx || mz;
    if (running && moving) { iS.sp -= 10 * 256 * dtMs / 1000; iStaminaWaitUs = 1200000; }
    if (moving)
        {
        // normalise diagonal
        int l = r3d_isqrt((mx >> 4) * (mx >> 4) + (mz >> 4) * (mz >> 4)) << 4;
        if (l > 16384) { mx = mx * 16384 / l; mz = mz * 16384 / l; }
        int step = spd * dtMs / 1000;
        // in strides of 25 cm at most: at the phone's 3 to 5 frames a second a frame's step is about a metre, a rise
        // of 60 units on Skyrim's stairs, more than a floor is looked for above the feet (the foot of a flight stopped
        // the walker)
        int nSub = step / (M / 4) + 1, sub;
#ifdef NOSUB
        nSub = 1;
#endif
        for (sub = 0; sub < nSub; sub++)
        {
        int dx = (mx * step / nSub) >> 14, dz = (mz * step / nSub) >> 14;
        int nx = Clamp(iS.px + dx, 6 * CELL, (iR->iGX - 6) * CELL), nz = Clamp(iS.pz + dz, 6 * CELL, (iR->iGZ - 6) * CELL);
        int opx = iS.px, opz = iS.pz;
        int run = Abs(dx) + Abs(dz) + 1;
        int climbOk = ClimbOk(cell, iS.px, iS.pz, nx, nz, feet, ground, mx, mz);
        // (things under 28 cm above the feet are stepped onto: deck edges, the top of a flight; outdoors 45 cm:
        //  the stones and roots one steps over in Skyrim)
        int sh = cell ? 7 * M / 25 : 45 * M / 100;
        if (climbOk && !BlockedAt(cell, nx, nz, 100, feet + sh, 7 * M / 4 - sh)) { iS.px = nx; iS.pz = nz; }
        else if (dx && !BlockedAt(cell, nx, iS.pz, 100, feet + sh, 7 * M / 4 - sh) && (FloorAt(cell, nx, iS.pz, feet) - ground) < run) iS.px = nx;
        else if (dz && !BlockedAt(cell, iS.px, nz, 100, feet + sh, 7 * M / 4 - sh) && (FloorAt(cell, iS.px, nz, feet) - ground) < run) iS.pz = nz;
        else if (!cell)
            {
            // on rough land: slip round the steep bit, half a step turned 45 degrees either way
            int s2;
            for (s2 = -1; s2 <= 1; s2 += 2)
                {
                int ax = iS.px + ((dx - s2 * dz) * 181 >> 9), az = iS.pz + ((dz + s2 * dx) * 181 >> 9);
                int ga = FloorAt(cell, ax, az, feet);
                int ra = (Abs(dx) + Abs(dz)) / 2 + 1;
                if ((ga - ground) < ra * 6 / 5 && !BlockedAt(cell, ax, az, 100, feet + 7 * M / 25, 7 * M / 4 - 7 * M / 25)) { iS.px = ax; iS.pz = az; break; }
                }
            }
        // (v0.4.74) not moved, and closed in all round (inside a piece's walls - a rock stepped onto from its low side, a
        // landing over a threshold): the walls let the player out; only the climb still counts
        if (iS.px == opx && iS.pz == opz && (dx || dz) && WaysOut(cell, iS.px, iS.pz, feet) == 0)
            {
            if (climbOk) { iS.px = nx; iS.pz = nz; }
            else if (WaysOut(cell, iS.px, iS.pz, feet, 0) == 0) { iS.px = nx; iS.pz = nz; }      // (a pit with no way up at all)
            }
        // the next stride from where this one ended: the feet up onto a floor that rose under them (down is gravity's)
#ifndef NOSUB
        ground = FloorAt(cell, iS.px, iS.pz, feet);
        if (ground > feet) { feet = ground; if (iS.py < feet + 410) iS.py = feet + 410; }
#endif
        }
        iPlayerAnim += (running ? spd * 3 / 4 : spd) * dtMs * 1024 / (360 * 1000);
        // push out of creatures
        int i;
        for (i = 0; i < iS.ne; i++)
            {
            Ent& e = iS.e[i];
            if (!e.alive || e.type == E_DRAGON || e.cell != cell) continue;
            int ex = iS.px - e.x, ez = iS.pz - e.z;
            int r = e.type == E_TROLL ? 260 : (e.type == E_CHEST ? 200 : 140);
            if (Abs(ex) < r && Abs(ez) < r)
                {
                int d = Abs(ex) + Abs(ez) + 1;
                int px2 = ex * (r - d / 2) / d / 4, pz2 = ez * (r - d / 2) / d / 4;
                // townsfolk give way (step back from the player) where they can; others push the player, never into a wall
                if (e.type <= E_SMITH && !BlockedAt(cell, e.x - px2, e.z - pz2, 90, e.y + 7 * M / 25, 7 * M / 4 - 7 * M / 25)) { e.x -= px2; e.z -= pz2; e.hx = e.x; e.hz = e.z; }
                else if (!BlockedAt(cell, iS.px + px2, iS.pz + pz2, 100, feet + 7 * M / 25, 7 * M / 4 - 7 * M / 25)) { iS.px += px2; iS.pz += pz2; }
                }
            }
        }
    // vertical: jump / gravity / swim
    ground = FloorAt(cell, iS.px, iS.pz, iS.py - 410);
    int eye = ground + 410;
    if (swimming) eye = wl + 64;
    if (iUi == UI_PLAY && (k & KEY_JUMP) && iS.py <= eye + 16 && !swimming && iS.sp > (5 << 8))
        {
        iS.vy = 1150;   // ~4.5 m/s
        iS.sp -= 5 << 8; iStaminaWaitUs = 1000000;
        }
    iS.py += iS.vy * dtMs / 1000;
    iS.vy -= 3070 * dtMs / 1000;     // 12 m/s^2
    if (iS.py <= eye) { iS.py = eye; iS.vy = 0; }
    // regeneration
    if (iStaminaWaitUs > 0) iStaminaWaitUs -= dt;
    else iS.sp += 9 * 256 * dtMs / 1000;
    if (iMagickaWaitUs > 0) iMagickaWaitUs -= dt;
    else iS.mp += 4 * 256 * dtMs / 1000;
    iS.hp += 256 * dtMs / 2000;
    iS.hp = Clamp(iS.hp, 0, iS.hpMax << 8);
    iS.mp = Clamp(iS.mp, 0, iS.mpMax << 8);
    iS.sp = Clamp(iS.sp, 0, iS.spMax << 8);
    // attack
    if (iSwingUs > 0)
        {
        iSwingUs -= dt;
        if (!iSwingHitDone && iSwingUs < 260000)
            {
            iSwingHitDone = 1;
            if (EquippedId(EQ_WEAPON) == IT_RAHGOT_STAFF) { void GameStaffCast(Game& g); GameStaffCast(*this); }     // (v0.4.71) a Wall of Flames
            else PlayerAttack();
            }
        if (iSwingUs < 0) iSwingUs = 0;
        }
    if (iGrab >= 0 || iFireLock) { }                   // the press throws what is carried
    else if (iUi == UI_PLAY && Pressed(KEY_FIRE) && !iS.drawn) ToggleWeapon();
    else if (iUi == UI_PLAY && (k & KEY_FIRE) && iS.drawn && iDrawUs == 0 && iSwingUs == 0 && iS.sp > (3 << 8))
        {
        iSwingUs = 450000; iSwingHitDone = 0;
        iS.sp -= 6 << 8; iStaminaWaitUs = 900000;
        }
    if (iCastCoolUs > 0) iCastCoolUs -= dt;
    if (iShoutCoolUs > 0) iShoutCoolUs -= dt;
    if (iShoutFxUs > 0) iShoutFxUs -= dt;
    if (iHurtUs > 0) iHurtUs -= dt;
    // the bars: shown a while after they change (and always while not full: DrawBars)
    {
    int cur[3] = { iS.hp >> 8, iS.mp >> 8, iS.sp >> 8 }, k;
    for (k = 0; k < 3; k++)
        {
        if (cur[k] != iBarLast[k]) { iBarUs[k] = 3000000; iBarLast[k] = cur[k]; }
        else if (iBarUs[k] > 0) iBarUs[k] -= dt;
        }
    }
    if (iShakeUs > 0) iShakeUs -= dt;
    }

void Game::PlayerAttack()
    {
    int fx = r3d_sin(iS.yaw), fz = r3d_cos(iS.yaw);
    int dmg = WeaponDamage();
    int i, hit = 0;
    for (i = 0; i < iS.ne; i++)
        {
        Ent& e = iS.e[i];
        if (!e.alive || e.type < E_WOLF || e.type == E_CHEST || e.cell != iS.cell) continue;
        if ((e.type == E_SPIDER || e.type == E_WIGHT) && (e.pad2 & 72)) continue;
        int dx = e.x - iS.px, dz = e.z - iS.pz;
        int reach = 3 * M + (e.type == E_TROLL ? M : 0) + (e.type == E_DRAGON ? 4 * M : 0);
        if (Abs(dx) > reach || Abs(dz) > reach) continue;
        int d = r3d_isqrt((dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4)) << 4;
        if (d > reach) continue;
        int ey = e.y + (e.type == E_DRAGON ? e.fly : 0);
        if (Abs(ey + M - (iS.py - 410)) > 3 * M && e.type != E_DRAGON) continue;
        if (e.type == E_DRAGON && e.fly > 4 * M) continue;
        // in front: dot > cos(55 deg)
        int dot = d ? ((dx * fx + dz * fz) / d) : 16384;
        if (dot < 9400 && d > M) continue;
        int crit = (r3d_hash(iTimeUs, i, 3) & 7) == 0;
        HurtEnt(e, dmg + (crit ? dmg / 2 : 0), 2 * M, iS.px, iS.pz);
        hit = 1;
        }
    if (hit) iShakeUs = 80000;
    }

// nothing solid between two points (a wall, a floor, the land): every half metre along the line
int Game::LineClear(int aCell, int x0, int y0, int z0, int x1, int y1, int z1)
    {
    int dx = x1 - x0, dy = y1 - y0, dz = z1 - z0;
    int L = r3d_isqrt((dx >> 4) * (dx >> 4) + (dy >> 4) * (dy >> 4) + (dz >> 4) * (dz >> 4)) << 4;
    int n = L / (M / 2), k;
    for (k = 1; k < n; k++)
        {
        int x = x0 + (int)((long long)dx * k / n), y = y0 + (int)((long long)dy * k / n), z = z0 + (int)((long long)dz * k / n);
        if (y < FloorAt(aCell, x, z, y) || BlockedAt(aCell, x, z, 20, y - 100)) return 0;
        }
    return 1;
    }
// whether an archer sees the player: looked again every 0.4 s (Ent::vy: bit 0 the answer, the rest the ms to wait)
int Game::ArcherSees(Ent& e, int dt)
    {
    int left = (e.vy >> 1) - dt / 1000;
    if (left > 0) { e.vy = (left << 1) | (e.vy & 1); return e.vy & 1; }
    int los = LineClear(e.cell, e.x, e.y + 3 * M / 2, e.z, iS.px, iS.py - 150, iS.pz);
    e.vy = (400 << 1) | los;
    return los;
    }
// a bandit archer's arrow at the player (a little ahead of where it aims, falling a little)
void Game::ShootArrow(Ent& e)
    {
    int i;
    for (i = 0; i < MAXP; i++) if (!iP[i].active) break;
    if (i == MAXP) return;
    Proj& p = iP[i];
    p.active = 1; p.owner = 1; p.kind = 0; p.pad = 5;
    p.x = e.x + ((r3d_sin(e.yaw) * (M / 2)) >> 14); p.z = e.z + ((r3d_cos(e.yaw) * (M / 2)) >> 14); p.y = e.y + 3 * M / 2;
    // (not every arrow true: up to 80 cm off at the player, more from further)
    u32 hh = r3d_hash(e.x + iTimeUs, e.z, 99);
    int L0 = Abs(iS.px - p.x) + Abs(iS.pz - p.z), spread = 40 + L0 / (6 * M) * 6;
    int tx = iS.px - p.x + ((int)(hh % (2 * spread + 1)) - spread) * M / 100, ty = (iS.py - 150) - p.y + ((int)((hh >> 10) % 61) - 30) * M / 100;
    int tz = iS.pz - p.z + ((int)((hh >> 18) % (2 * spread + 1)) - spread) * M / 100;
    int L = r3d_isqrt((tx >> 4) * (tx >> 4) + (ty >> 4) * (ty >> 4) + (tz >> 4) * (tz >> 4)) << 4;
    if (L < 1) L = 1;
    int sp = 32 * M;
    p.vx = tx * (sp >> 4) / (L >> 4); p.vz = tz * (sp >> 4) / (L >> 4);
    p.vy = ty * (sp >> 4) / (L >> 4) + L / 16;                         // (aimed a little high for the drop: g t / 2)
    p.life = 2500000; p.dmg = 9;
    }

void Game::CastSpell()
    {
    if (iCastCoolUs > 0) return;
    if (iS.mp < (20 << 8)) { Msg("Not enough magicka."); return; }
    int i;
    for (i = 0; i < MAXP; i++) if (!iP[i].active) break;
    if (i == MAXP) return;
    iS.mp -= 20 << 8;
    iMagickaWaitUs = 1500000;
    iCastCoolUs = 600000;
    Proj& p = iP[i];
    int cp = r3d_cos(iS.pitch), sp = r3d_sin(iS.pitch);
    int dx = (r3d_sin(iS.yaw) * cp) >> 14, dz = (r3d_cos(iS.yaw) * cp) >> 14, dy = sp;
    p.active = 1; p.owner = 0; p.kind = 0;
    p.x = iS.px + (dx >> 6) - (r3d_cos(iS.yaw) >> 7); p.z = iS.pz + (dz >> 6) + (r3d_sin(iS.yaw) >> 7);
    p.y = iS.py - 60;
    int speed = 26 * M;
    p.vx = (dx * (speed >> 6)) >> 8; p.vy = (dy * (speed >> 6)) >> 8; p.vz = (dz * (speed >> 6)) >> 8;
    p.life = 3000000; p.dmg = 34;
    }

void Game::Shout()
    {
    int words = iS.bfb[BF_WORD];
    if (words <= 0) { Msg("You know no words of the Voice."); return; }
    if (iShoutCoolUs > 0) { Msg("Your voice has not recovered."); return; }
    iShoutCoolUs = 20000000 - words * 4000000;
    iShoutFxUs = 900000;
    iShakeUs = 300000;
    int fx = r3d_sin(iS.yaw), fz = r3d_cos(iS.yaw);
    int range = (8 + words * 5) * M;
    int i;
    for (i = 0; i < iS.ne; i++)
        {
        Ent& e = iS.e[i];
        if (!e.alive || e.type < E_WOLF || e.type == E_CHEST || e.cell != iS.cell || ((e.type == E_SPIDER || e.type == E_WIGHT) && (e.pad2 & 72))) continue;
        int dx = e.x - iS.px, dz = e.z - iS.pz;
        int lim = e.type == E_DRAGON ? range * 3 : range;
        if (Abs(dx) > lim || Abs(dz) > lim) continue;
        int d = r3d_isqrt((dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4)) << 4;
        if (d > lim) continue;
        int dot = d ? ((dx * fx + dz * fz) / d) : 16384;
        if (dot < 11500) continue;          // ~45 degree cone
        if (e.type == E_DRAGON)
            {
            if (words >= 3 && (e.state == D_FLY || e.state == D_TAKEOFF))
                {
                e.state = D_LAND; e.timer = 0;
                Msg("The dragon is dragged out of the sky!");
                }
            HurtEnt(e, 10 * words, 0, iS.px, iS.pz);
            continue;
            }
        HurtEnt(e, 8 * words, (3 + words * 3) * M, iS.px, iS.pz);
        if (e.alive) { e.state = S_STAGGER; e.timer = 1200000 + words * 300000; }
        }
    }

// ---------------------------------------------------------------- creatures
void Game::HurtEnt(Ent& e, int aDmg, int aKnock, int aFromX, int aFromZ)
    {
    if (!e.alive) return;
    if ((e.type == E_SPIDER || e.type == E_WIGHT) && (e.pad2 & 72)) return;     // hidden: in the dark above, in the sarcophagus
    e.hp = (short)(e.hp - aDmg);
    e.flash = 4;
    e.aggro = 1;
    iTarget = (int)(&e - iS.e);
    iTargetUs = 5000000;
    if (aKnock && e.type != E_DRAGON)
        {
        int dx = e.x - aFromX, dz = e.z - aFromZ;
        int l = Abs(dx) + Abs(dz) + 1;
        int k = e.type == E_TROLL ? aKnock / 3 : aKnock;
        e.vx = dx * k / l; e.vz = dz * k / l;
        if (e.state != S_ATTACK || aKnock > 3 * M) { e.state = S_STAGGER; e.timer = 300000; }
        }
    if (e.type == E_DRAGON && e.state == D_SLEEP) { e.state = D_TAKEOFF; e.timer = 0; }
    if (e.hp <= 0) KillEnt(e);
    }

void Game::KillEnt(Ent& e)
    {
    e.hp = 0; e.alive = 0; e.state = S_DEAD; e.timer = 0; e.atk = 0;
    e.vx = e.vz = 0;
    if (e.type == E_SKELETON) e.cool = iS.day;          // it rises again some days later
    DunKilled(e);
    if (e.type == E_DRAGON)
        {
        Msg("The dragon is slain!");
        }
    }

void Game::MoveEnt(Ent& e, int aDx, int aDz, int aRadius)
    {
    int nx = e.x + aDx, nz = e.z + aDz;
    if (nx < 8 * CELL || nx > (iR->iGX - 8) * CELL || nz < 8 * CELL || nz > (iR->iGZ - 8) * CELL) return;
    int g0 = e.y, g1 = FloorAt(e.cell, nx, nz, e.y);
    int swim = e.cell == 0 && g1 < iR->WaterAt(nx, nz) - M / 2;
    if (!swim && !BlockedAt(e.cell, nx, nz, aRadius, e.y) && g1 - g0 < (Abs(aDx) + Abs(aDz)) * 2 + 32) { e.x = nx; e.z = nz; return; }
    // slide: try rotated directions
    int k;
    for (k = 0; k < 2; k++)
        {
        int rx = k ? -aDz : aDz, rz = k ? aDx : -aDx;
        int ax = e.x + (aDx + rx) / 2, az = e.z + (aDz + rz) / 2;
        int g = FloorAt(e.cell, ax, az, e.y);
        if ((e.cell || g >= iR->WaterAt(ax, az) - M / 2) && !BlockedAt(e.cell, ax, az, aRadius, e.y) && g - g0 < (Abs(aDx) + Abs(aDz)) * 2 + 32) { e.x = ax; e.z = az; return; }
        }
    }

void Game::UpdateEnt(Ent& e, int dt)
    {
    int dtMs = dt / 1000;
    if (e.flash) e.flash--;
    int dx = iS.px - e.x, dz = iS.pz - e.z;
    int far = Abs(dx) > 120 * M || Abs(dz) > 120 * M || e.cell != iS.cell;
    int d = far ? 120 * M : (r3d_isqrt((dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4)) << 4);
    if (e.state == S_DEAD) { e.y = FloorAt(e.cell, e.x, e.z, e.y); return; }
    if (e.type == E_CHEST) return;
    if (e.type == E_DRAGON) { UpdateDragon(e, dt); return; }
    int speed = 0, radius = 100, aggroR = 0, reach = 2 * M, dmg = 0, atkUs = 700000, leash = 60 * M;
    switch (e.type)
        {
        case E_WOLF:  speed = 13 * M / 2; radius = 120; aggroR = 24 * M; reach = 2 * M; dmg = 8; atkUs = 600000; leash = 70 * M; break;
        case E_WIGHT: speed = 3 * M; radius = 100; aggroR = 15 * M; reach = 5 * M / 2; dmg = 13; atkUs = 900000; leash = 32 * M; break;
        case E_SKELETON: speed = 7 * M / 2; radius = 90; aggroR = 11 * M; reach = 9 * M / 4; dmg = 7; atkUs = 800000; leash = 30 * M; break;
        case E_TROLL: speed = 4 * M; radius = 220; aggroR = 20 * M; reach = 4 * M; dmg = 26; atkUs = 1200000; leash = 45 * M; break;
        case E_BANDIT:
            speed = 4 * M; radius = 100; aggroR = 14 * M; reach = 9 * M / 4; dmg = 10; atkUs = 800000; leash = 40 * M;
            if (BanditKind(e) == BK_2H) { dmg = 15; atkUs = 1100000; reach = 5 * M / 2; }
            else if (BanditKind(e) == BK_HEAVY) { dmg = 11; speed = 7 * M / 2; }
            else if (BanditKind(e) == BK_ARCHER) { dmg = 6; aggroR = 30 * M; leash = 50 * M; }      // (a dagger near)
            break;
        case E_SKEEVER: speed = 5 * M; radius = 70; aggroR = 12 * M; reach = 8 * M / 5; dmg = 4; atkUs = 600000; leash = 40 * M; break;
        case E_SPIDER: speed = 4 * M; radius = 160; aggroR = 14 * M; reach = 13 * M / 5; dmg = 12; atkUs = 900000; leash = 30 * M; break;
        case E_MUDCRAB: speed = 2 * M; radius = 110; aggroR = 6 * M; reach = 2 * M; dmg = 4; atkUs = 800000; leash = 20 * M; break;
        case E_DEER: speed = e.var ? 8 * M : 9 * M; radius = 120; break;
        case E_HARE: speed = 7 * M; radius = 50; break;
        case E_FOX: speed = 8 * M; radius = 70; break;
        case E_GOAT: speed = 6 * M; radius = 100; break;
        default:      speed = 6 * M / 5; radius = 90; break;
        }
    int moveX = 0, moveZ = 0, animSp = 0;
    int hdx = e.hx - e.x, hdz = e.hz - e.z;
    int homeD = Abs(hdx) + Abs(hdz);
    if (IsPrey(e.type))
        {
        // the land's game: grazing about where Skyrim puts it, off at a run from the player (nearer than it can bear,
        // or when hurt), and some seconds more once out of reach
        int fear = e.type == E_DEER ? 22 * M : (e.type == E_GOAT ? 16 * M : 13 * M);
        if (e.state == S_STAGGER || e.flash || (d < fear && iUi != UI_DEAD)) { e.state = S_CHASE; e.timer = 5000000; }
        if (e.state == S_CHASE)
            {
            e.timer -= dt;
            if (e.timer <= 0 && d > fear) { e.state = S_IDLE; e.hx = e.x; e.hz = e.z; }        // (grazes where it stopped)
            else
                {
                int wig = (int)((r3d_hash(e.x >> 10, e.z >> 10, 3) & 255) - 128);
                e.yaw = (r3d_atan2(-dx, -dz) + wig) & 1023;
                int st = speed * dtMs / 1000;
                moveX = (r3d_sin(e.yaw) * st) >> 14; moveZ = (r3d_cos(e.yaw) * st) >> 14;
                animSp = speed;
                }
            }
        else
            {
            e.timer -= dt;
            if (e.timer <= 0)
                {
                u32 h = r3d_hash(e.x, iTimeUs >> 10, e.type);
                e.timer = 3000000 + (int)(h % 6000000);
                int a = (int)(h >> 8) & 1023, rr = 10 * M;
                e.vx = e.hx + ((r3d_sin(a) * (rr >> 4)) >> 10) - e.x; e.vz = e.hz + ((r3d_cos(a) * (rr >> 4)) >> 10) - e.z;
                e.state = (h >> 20) & 1 ? S_WANDER : S_IDLE;
                }
            if (e.state == S_WANDER)
                {
                int l = Abs(e.vx) + Abs(e.vz);
                if (l < M / 2) e.state = S_IDLE;
                else
                    {
                    int st = M * dtMs / 1000;                                 // (a walk, grazing as it goes)
                    moveX = e.vx * st / l; moveZ = e.vz * st / l;
                    e.vx -= moveX; e.vz -= moveZ;
                    e.yaw = r3d_atan2(moveX, moveZ);
                    animSp = M;
                    }
                }
            }
        }
    else if (e.type <= E_SMITH)
        {
        // villagers: stroll around home, stop and face the player when close
        if (e.role == ROLE_MERCHANT || e.role == ROLE_TRADER || (e.role == ROLE_NAMED && !(e.pad2 & 1)))
            {
            e.state = S_IDLE;
            if (d < 4 * M) e.yaw = r3d_atan2(dx, dz);
            else if (GameChopping(*this, e)) e.yaw = 0;                    // back to the block (it is north of him)
            }
        else if (d < 3 * M) { e.yaw = r3d_atan2(dx, dz); }
        else
            {
            e.timer -= dt;
            if (e.timer <= 0)
                {
                u32 h = r3d_hash(e.x, iTimeUs >> 10, e.type);
                e.timer = 3000000 + (int)(h % 5000000);
                int r = e.cell ? 2 * M : (e.role == ROLE_CARAVAN ? 3 * M : (e.type == E_VILLAGER ? 14 * M : 3 * M));
                int a = (int)(h >> 8) & 1023;
                e.vx = e.hx + ((r3d_sin(a) * (r >> 4)) >> 10) - e.x;     // stored as target offset
                e.vz = e.hz + ((r3d_cos(a) * (r >> 4)) >> 10) - e.z;
                e.state = (h >> 20) & 1 ? S_WANDER : S_IDLE;
                }
            if (e.state == S_WANDER)
                {
                int l = Abs(e.vx) + Abs(e.vz);
                if (l < M / 2) e.state = S_IDLE;
                else
                    {
                    int st = speed * dtMs / 1000;
                    moveX = e.vx * st / l; moveZ = e.vz * st / l;
                    e.vx -= moveX; e.vz -= moveZ;
                    e.yaw = r3d_atan2(moveX, moveZ);
                    animSp = speed;
                    }
                }
            }
        }
    else
        {
        // hostile (the barrow's own: asleep, hanging, stuck in a web, scripted, casting)
        if (DunEntUpdate(e, dt, d)) return;
        if (e.type == E_WIGHT && IsDungeon(e.cell))
            {
            if (e.pad1 == 2) { dmg = 22; atkUs = 1100000; reach = 11 * M / 4; }
            else if (e.pad1 == 4) { dmg = 26; atkUs = 1200000; reach = 11 * M / 4; }      // (v0.4.68) Rahgot, his staff
            else if (e.pad1 == 5) dmg = 18;                                               // a ruin's boss
            else if (e.pad1 == 1) dmg = 14;
            else dmg = 10;
            leash = 45 * M;
            }
        if (e.state == S_STAGGER)
            {
            e.timer -= dt;
            moveX = e.vx * dtMs / 1000; moveZ = e.vz * dtMs / 1000;
            e.vx = e.vx * 7 / 8; e.vz = e.vz * 7 / 8;
            if (e.timer <= 0) e.state = S_CHASE;
            }
        else
            {
            if (!e.aggro && d < aggroR && iUi != UI_DEAD) e.aggro = 1;
            if (e.aggro && (homeD > leash || iUi == UI_DEAD)) { e.aggro = 0; e.state = S_RETURN; }
            // (a blow reaches only someone at its own height: not through a floor, from the storey below)
            int level = Abs(e.y - (iS.py - 410)) < 3 * M / 2;
            if (e.atk > 0)
                {
                int before = e.atk;
                e.atk -= dt;
                int mid = atkUs / 2;
                if (before > mid && e.atk <= mid && d < reach + M / 2 && level) HurtPlayer(dmg, e.x, e.z);
                if (e.atk <= 0) e.atk = 0;
                e.yaw = r3d_atan2(dx, dz);
                }
            else if (e.aggro && e.type == E_BANDIT && BanditKind(e) == BK_ARCHER && d > 4 * M && d < 35 * M && ArcherSees(e, dt))
                {
                // an archer: stands and shoots while it can see you (Skyrim's bandit archers keep their distance)
                e.state = S_IDLE;
                e.yaw = r3d_atan2(dx, dz);
                if (e.cool <= 0) { ShootArrow(e); e.cool = 2400000 + (int)(r3d_hash(e.x, iTimeUs >> 14, 5) % 900000); }
                }
            else if (e.aggro)
                {
                e.state = S_CHASE;
                e.yaw = r3d_atan2(dx, dz);
                if (d < reach && level) { e.atk = atkUs; }
                else
                    {
                    int st = speed * dtMs / 1000;
                    moveX = (r3d_sin(e.yaw) * st) >> 14; moveZ = (r3d_cos(e.yaw) * st) >> 14;
                    animSp = speed;
                    }
                }
            else if (e.state == S_RETURN || homeD > 4 * M)
                {
                if (homeD < 2 * M) e.state = S_IDLE;
                else
                    {
                    e.yaw = r3d_atan2(hdx, hdz);
                    int st = speed / 2 * dtMs / 1000;
                    moveX = (r3d_sin(e.yaw) * st) >> 14; moveZ = (r3d_cos(e.yaw) * st) >> 14;
                    animSp = speed / 2;
                    if (e.hp < e.maxhp) e.hp++;
                    }
                }
            else
                {
                // idle: occasional turn
                e.timer -= dt;
                if (e.timer <= 0) { e.timer = 2000000 + (int)(r3d_hash(e.x, iTimeUs >> 12, 7) % 4000000); e.yaw = (e.yaw + 200) & 1023; }
                }
            }
        }
    // (the floor under it looked up only when it moved: standing still it stays where it is)
    if (moveX || moveZ) { MoveEnt(e, moveX, moveZ, radius); e.y = FloorAt(e.cell, e.x, e.z, e.y); }
    else if (d < 30 * M) e.y = FloorAt(e.cell, e.x, e.z, e.y);                     // (near: all the same)
    // one walk cycle (1024 anim units) per stride length so the feet match the ground speed
    int stride = e.type == E_WOLF ? 560 : (e.type == E_TROLL ? 640 : (e.type == E_DEER ? 700 : (e.type == E_HARE ? 200 : 360)));    // world units per cycle
    e.anim += animSp * dtMs * 1024 / (stride * 1000);
    }

void Game::UpdateDragon(Ent& e, int dt)
    {
    int dtMs = dt / 1000;
    int dx = iS.px - e.x, dz = iS.pz - e.z;
    int far = Abs(dx) > 120 * M || Abs(dz) > 120 * M || iS.cell != 0;
    int d = far ? 120 * M : (r3d_isqrt((dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4)) << 4);
    int ground = iR->GroundAt(e.x, e.z);
    e.anim += 900 * dtMs / 60;
    e.timer += dt;
    switch (e.state)
        {
        case D_SLEEP:
            e.y = ground; e.fly = 0;
            if (d < 70 * M) { e.state = D_TAKEOFF; e.timer = 0; Msg("A dragon rises!"); }
            break;
        case D_TAKEOFF:
            e.fly += 6 * M * dtMs / 1000;
            if (e.fly > 22 * M) { e.state = D_FLY; e.timer = 0; }
            break;
        case D_FLY:
            {
            // circle the player at ~30 m, climbing/descending to 22 m above ground
            int a = r3d_atan2(e.x - iS.px, e.z - iS.pz) + 40 * dtMs / 100;
            int tx = iS.px + ((r3d_sin(a) * 30) >> 14) * M, tz = iS.pz + ((r3d_cos(a) * 30) >> 14) * M;
            int mx = tx - e.x, mz = tz - e.z;
            int l = Abs(mx) + Abs(mz) + 1;
            int st = 15 * M * dtMs / 1000;
            if (l > st) { e.x += mx * st / l; e.z += mz * st / l; }
            e.yaw = r3d_atan2(mx, mz);
            int want = 22 * M;
            e.fly += (want - e.fly) * dtMs / 1000;
            e.x = Clamp(e.x, 10 * CELL, (iR->iGX - 10) * CELL); e.z = Clamp(e.z, 10 * CELL, (iR->iGZ - 10) * CELL);
            // fireballs every 3.5 s
            e.cool -= dt;
            if (e.cool <= 0 && d < 70 * M)
                {
                e.cool = 3500000;
                int i;
                for (i = 0; i < MAXP; i++) if (!iP[i].active) break;
                if (i < MAXP)
                    {
                    Proj& p = iP[i];
                    p.active = 1; p.owner = 1; p.kind = 0;
                    p.x = e.x; p.z = e.z; p.y = ground + e.fly + 2 * M;
                    int tx2 = iS.px - p.x, ty2 = (iS.py - 200) - p.y, tz2 = iS.pz - p.z;
                    int L = r3d_isqrt((tx2 >> 4) * (tx2 >> 4) + (ty2 >> 4) * (ty2 >> 4) + (tz2 >> 4) * (tz2 >> 4)) << 4;
                    if (L < 1) L = 1;
                    int sp = 20 * M;
                    p.vx = tx2 * (sp >> 4) / (L >> 4); p.vy = ty2 * (sp >> 4) / (L >> 4); p.vz = tz2 * (sp >> 4) / (L >> 4);
                    p.life = 4000000; p.dmg = 18;
                    }
                }
            // land on its own now and then
            if (e.timer > 22000000) { e.state = D_LAND; e.timer = 0; }
            break;
            }
        case D_LAND:
            e.fly -= 9 * M * dtMs / 1000;
            if (d > 14 * M)
                {
                int st = 8 * M * dtMs / 1000;
                e.x += (dx >> 6) * (st >> 4) / ((d >> 10) + 1) / 1; e.z += (dz >> 6) * (st >> 4) / ((d >> 10) + 1);
                }
            if (e.fly <= 0) { e.fly = 0; e.state = D_GROUND; e.timer = 0; iShakeUs = 400000; }
            break;
        case D_GROUND:
            {
            e.fly = 0;
            e.yaw = r3d_atan2(dx, dz);
            if (e.atk > 0)
                {
                int before = e.atk;
                e.atk -= dt;
                if (before > 500000 && e.atk <= 500000 && d < 7 * M) HurtPlayer(28, e.x, e.z);
                if (e.atk < 0) e.atk = 0;
                }
            else if (d < 6 * M) e.atk = 1000000;
            else if (d < 40 * M)
                {
                int st = 4 * M * dtMs / 1000;
                MoveEnt(e, (r3d_sin(e.yaw) * st) >> 14, (r3d_cos(e.yaw) * st) >> 14, 300);
                }
            // breathe fire at mid range
            e.cool -= dt;
            if (e.cool <= 0 && d < 25 * M && d > 6 * M)
                {
                e.cool = 2800000;
                int i;
                for (i = 0; i < MAXP; i++) if (!iP[i].active) break;
                if (i < MAXP)
                    {
                    Proj& p = iP[i];
                    p.active = 1; p.owner = 1; p.kind = 0;
                    p.x = e.x + ((r3d_sin(e.yaw) * 4) >> 14) * M; p.z = e.z + ((r3d_cos(e.yaw) * 4) >> 14) * M; p.y = ground + 4 * M;
                    int sp = 18 * M;
                    p.vx = (dx >> 4) * (sp >> 4) / ((d >> 8) + 1) ; p.vz = (dz >> 4) * (sp >> 4) / ((d >> 8) + 1);
                    p.vx = p.vx * 16 / 16; p.vz = p.vz;
                    p.vy = -M;
                    p.life = 2500000; p.dmg = 16;
                    }
                }
            if (e.timer > 16000000 && e.hp > 0) { e.state = D_TAKEOFF; e.timer = 0; }
            break;
            }
        }
    if (e.fly < 0) e.fly = 0;
    e.y = iR->GroundAt(e.x, e.z);
    }

// one gone (the last moved into its place)
void Game::RemoveEnt(int i)
    {
    if (i < 0 || i >= iS.ne) return;
    int last = iS.ne - 1;
    if (i != last) iS.e[i] = iS.e[last];
    iS.ne--;
    if (iDragonIdx == last) iDragonIdx = i;
    if (iTradeEnt == last) iTradeEnt = i; else if (iTradeEnt == i) iTradeEnt = -1;
    if (iFocusKind == 1) { iFocusKind = 0; iFocus = -1; }
    }

// the land's animals (Skyrim's creature markers, gen_kit.py): each, while the player is within 110 m of it, has its
// animal out (wolves for the predators' markers); one further than 170 m from the player is gone again (dead or
// alive: it comes back when the player does, as Skyrim's respawn). Marker k's animal: pad1, pad2 = k + 1.
void Game::UpdateAnimals(int aDt)
    {
    iAnimTickUs -= aDt;
    if (iAnimTickUs > 0 || !iAnimN) return;
    iAnimTickUs = 1500000;
    int i;
    // out of reach: gone; the rest marked out (from the animals themselves: a loaded game's too)
    r3d_memset(iAnimOut, 0, sizeof(iAnimOut));
    for (i = iS.ne - 1; i >= 0; i--)
        {
        Ent& e = iS.e[i];
        int k = (e.pad1 | (e.pad2 << 8)) - 1;
        if (e.cell || k < 0 || k >= iAnimN || !(e.type == E_WOLF || (e.type >= E_DEER && e.type <= E_MUDCRAB))) continue;
        if (Abs(e.x - iS.px) < 170 * M && Abs(e.z - iS.pz) < 170 * M && iS.cell == 0) { iAnimOut[k >> 3] |= (u8)(1 << (k & 7)); continue; }
        RemoveEnt(i);
        }
    if (iS.cell) return;
    for (i = 0; i < iAnimN && iS.ne < MAXE - 24; i++)
        {
        if ((iAnimOut[i >> 3] >> (i & 7)) & 1) continue;
        const u8* a = iAnimEnt + i * 12;
        int x = a[0] | (a[1] << 8) | (a[2] << 16) | (a[3] << 24), z = a[4] | (a[5] << 8) | (a[6] << 16) | (a[7] << 24);
        if (Abs(x - iS.px) > 110 * M || Abs(z - iS.pz) > 110 * M) continue;
        static const u8 KType[7] = { E_WOLF, E_DEER, E_DEER, E_HARE, E_FOX, E_GOAT, E_MUDCRAB };
        int kind = a[8] < 7 ? a[8] : 0;
        int idx = SpawnEnt(KType[kind], x, z, (int)(r3d_hash(x, z, 9) & 1023));
        if (idx < 0) break;
        Ent& e = iS.e[idx];
        e.var = (u8)(kind == 2);
        e.pad1 = (u8)((i + 1) & 255); e.pad2 = (u8)((i + 1) >> 8);
        iAnimOut[i >> 3] |= (u8)(1 << (i & 7));
        }
    }

void Game::UpdateEnts(int dt)
    {
    int i;
    UpdateAnimals(dt);
    UpdatePicks();
    for (i = 0; i < iS.ne; i++) UpdateEnt(iS.e[i], dt);
    // separation between hostile creatures
    for (i = 0; i < iS.ne; i++)
        {
        Ent& a = iS.e[i];
        if (!a.alive || a.type < E_WOLF || a.type > E_TROLL) continue;
        int j;
        for (j = i + 1; j < iS.ne; j++)
            {
            Ent& b = iS.e[j];
            if (!b.alive || b.type < E_WOLF || b.type > E_TROLL) continue;
            int dx = a.x - b.x, dz = a.z - b.z;
            if (Abs(dx) < 220 && Abs(dz) < 220)
                {
                int s = dx >= 0 ? 8 : -8, t = dz >= 0 ? 8 : -8;
                a.x += s; a.z += t; b.x -= s; b.z -= t;
                }
            }
        }
    }

void Game::UpdateProj(int dt)
    {
    int dtMs = dt / 1000;
    int i;
    for (i = 0; i < MAXP; i++)
        {
        Proj& p = iP[i];
        if (!p.active) continue;
        p.life -= dt;
        if (p.kind == 2)
            {
            if (p.life <= 0) p.active = 0;
            continue;
            }
        p.x += p.vx * dtMs / 1000; p.y += p.vy * dtMs / 1000; p.z += p.vz * dtMs / 1000;
        if (p.pad == 5) p.vy -= 4 * M * dtMs / 1000;                   // an arrow falls
        int hit = p.life <= 0 || p.y < FloorAt(iS.cell, p.x, p.z, p.y) || BlockedAt(iS.cell, p.x, p.z, 20, p.y - 100)
                  || (iS.cell && p.y > iCellFloorY + 3 * M);
        if (p.owner == 0)
            {
            int k;
            for (k = 0; k < iS.ne && !hit; k++)
                {
                Ent& e = iS.e[k];
                if (!e.alive || e.type < E_WOLF || e.type == E_CHEST) continue;
                int r = e.type == E_DRAGON ? 5 * M : (e.type == E_TROLL ? 3 * M / 2 : M);
                int ey = e.y + e.fly + (e.type == E_DRAGON ? 2 * M : M);
                if (Abs(p.x - e.x) < r && Abs(p.z - e.z) < r && Abs(p.y - ey) < r + M)
                    {
                    HurtEnt(e, p.dmg, M, p.x - p.vx / 8, p.z - p.vz / 8);
                    hit = 1;
                    }
                }
            }
        else
            {
            if (Abs(p.x - iS.px) < 280 && Abs(p.z - iS.pz) < 280 && Abs(p.y - (iS.py - 200)) < 400)
                {
                HurtPlayer(p.dmg, p.x - p.vx, p.z - p.vz);
                if (p.pad == 5) Msg("Hit by an arrow");
                hit = 1;
                }
            }
        if (hit) { p.kind = 2; p.life = 350000; }
        }
    }

// ---------------------------------------------------------------- activation
void Game::FindFocus()
    {
    iFocus = -1; iFocusKind = 0;
    int fx = r3d_sin(iS.yaw), fz = r3d_cos(iS.yaw);
    int best = 4 * M;
    int i;
    for (i = 0; i < iS.ne; i++)
        {
        Ent& e = iS.e[i];
        if (e.type == E_DRAGON && e.alive) continue;
        if (e.alive && e.type >= E_WOLF && e.type <= E_TROLL && !(e.type == E_BANDIT && (e.pad2 & 16))) continue;
        if (e.cell != iS.cell) continue;
        int dx = e.x - iS.px, dz = e.z - iS.pz;
        if (Abs(dx) > best || Abs(dz) > best) continue;
        // (not through a floor: someone downstairs, or up on a gallery, is not to be talked to from here)
        if (e.type != E_DRAGON && Abs(e.y - (iS.py - 410)) > 16 * M / 10) continue;
        int d = r3d_isqrt((dx >> 4) * (dx >> 4) + (dz >> 4) * (dz >> 4)) << 4;
        int lim = e.type == E_DRAGON ? 9 * M : 3 * M;
        if (d > lim || d > best) continue;
        int dot = d ? (dx * fx + dz * fz) / d : 16384;
        if (dot < 11000) continue;
        best = d; iFocus = i; iFocusKind = 1;
        }
    }

void Game::Activate()
    {
    if (iFocusKind != 1) return;
    Ent& e = iS.e[iFocus];
    switch (e.type)
        {
        case E_ELDER:
            if (!iS.quest) { iS.quest = 1; StartDialog(DLG_ELDER_INTRO); }      // (quest: met him)
            else StartDialog(DLG_ELDER_AFTER);
            break;
        case E_SMITH:
            StartDialog(iS.smithGift ? DLG_SMITH : DLG_SMITH_GIFT);
            break;
        case E_VILLAGER:
            if (e.role == ROLE_MERCHANT && e.pad1 == MER_RWTRADER && LucanTalk(iFocus)) break;
            if (e.role == ROLE_MERCHANT && e.pad1 == MER_RWINN) { Msg("Orgnar: We got rooms and food. Drink, too."); Msg("I cook. Ain't much else to tell."); }
            if (e.role == ROLE_NAMED && e.pad1 == 18 && e.cell == CELL_RWTRADER && CamillaTalk()) break;
            if (e.role == ROLE_NAMED && e.pad1 == 53 && FhValmirTalk()) break;          // (v0.4.69) Valmir
            if (e.role == ROLE_MERCHANT || e.role == ROLE_TRADER)
                {
                int hour = iS.timeOfDay / 60000;
                // the market keeps hours; the caravan trades day and night, as in Skyrim; the Riverwood Trader stays
                // open day and night until the claw is home
                int allDay = e.pad1 == MER_INN || e.pad1 == MER_RWINN || (e.pad1 == MER_RWTRADER && iS.bfb[BF_QCLAW] < 200);
                if (e.role == ROLE_MERCHANT && !allDay && (hour < 8 || hour >= 20)) { Msg("\"We're closed. Come back after sunrise.\""); break; }
                iTradeEnt = iFocus;
                OpenItemMenu(UI_TRADE);
                break;
                }
            if (e.role == ROLE_NAMED && e.pad1 == 8 && FarengarTalk()) break;
            iDlgId = DLG_VILLAGER; iDlgPage = 0; iUi = UI_DIALOG;
            iDlgPage = 0;
            // pick one of the villager lines (negative page = line index)
            if (e.role == ROLE_STABLE) iDlgPage = -4 - (int)((iTimeUs >> 22) & 1);
            else if (e.role == ROLE_NAMED && GameKitSayN(e.pad1))
                iDlgPage = -1000 - (int)(e.pad1 * 64 + (((iTimeUs >> 18) + iTalks++) % GameKitSayN(e.pad1)));   // (v0.4.53: one of their own lines)
            else if (e.role == ROLE_NAMED) iDlgPage = -6 - (int)(e.pad1 % NNAMED);
            else iDlgPage = -(int)(e.var % 3) - 1;
            break;
        case E_CHEST:
            OpenChest(e);
            break;
        default:
            if (e.alive && DunTalk(e)) break;
            if (!e.alive) GiveLoot(e);
            break;
        }
    }

// ---------------------------------------------------------------- tick
void Game::Tick(int aDtUs, u32 aKeys)
    {
    if (aDtUs > 100000) aDtUs = 100000;
    if (aDtUs < 1000) aDtUs = 1000;
    // (v0.4.57) outside, crossing between Riverwood's and Whiterun's areas: the other area's file read in
    // (v0.4.74) a load put off a frame (the loading screen shown): done now; till it is shown, nothing moves
    if (iLoadPhase == 2) DoPendingLoad();
    if (iLoadPhase) return;
    if (iWrKit && !iS.cell && iUi == UI_PLAY) { int a = AreaOutside(iS.px, iS.pz); if (a != iAreaCur && !DeferLoad(2, a)) UseArea(a); }
    if (iLoadPhase) return;
    iPrevKeys = iKeys;
    iKeys = aKeys;
    // this frame's presses: what went down since the last (a tap between frames too); in the menus a held direction
    // repeats, after a moment, a few times a second
    iHit = (iKeys & ~iPrevKeys) | iHitIn; iHitIn = 0;
    {
    const u32 nav = KEY_LOOK_U | KEY_LOOK_D | KEY_LOOK_L | KEY_LOOK_R | KEY_FWD | KEY_BACK | KEY_STRAFE_L | KEY_STRAFE_R;
    u32 held = iKeys & nav;
    if (iUi != UI_PLAY && held && !(iHit & nav))
        {
        iRepUs += aDtUs;
        if (iRepUs >= 380000) { iHit |= held; iRepUs -= 120000; }
        }
    else iRepUs = 0;
    }
    iTimeUs += aDtUs;
    if (iS.cell == 0) iR->UpdateLandWindow(iS.px, iS.pz);      // the land round the player: read as they go
    if (iUi != UI_TITLE) UpdateStage();                         // (v0.4.67) people and creatures with their places
    int i;
    for (i = 0; i < MAXMSG; i++) if (iMsgUs[i] > 0) iMsgUs[i] -= aDtUs;
    if (iTargetUs > 0) iTargetUs -= aDtUs;
    if (iVictoryUs > 0) iVictoryUs -= aDtUs;
    iPreviewAngle = (iPreviewAngle + aDtUs / 3000) & 1023;
    if (iUi == UI_TITLE)
        {
        iTitleUs += aDtUs;
        if (Pressed(KEY_FIRE)) iUi = UI_PLAY;
        UpdateAtmosphere();
        return;
        }
    if (iFadeUs > 0) iFadeUs -= aDtUs;
    if (iDiscUs > 0) iDiscUs -= aDtUs;
    if (iUi == UI_PLAY) Discover();
    if (iWBlend < 256) { iWBlend += aDtUs / 31250; if (iWBlend > 256) iWBlend = 256; }     // ~8 s cross-fade
    if (iUi == UI_WAIT) { WaitInput(aDtUs); UpdateCamera(aDtUs); UpdateAtmosphere(); return; }
    if (iUi == UI_TELE) { TeleInput(); UpdateAtmosphere(); return; }
    if (iUi == UI_CONT || iUi == UI_COOK || iUi == UI_TRADE)
        {
        if (iUi == UI_CONT) ContInput();
        else if (iUi == UI_COOK) CookInput();
        else TradeInput();
        UpdateProps(aDtUs);
        UpdateAtmosphere();
        return;
        }
    if (iUi == UI_PUZZLE) { PuzzleInput(); UpdateAtmosphere(); return; }
    if (iUi != UI_PLAY && iUi != UI_DEAD) { UiInput(); UpdateAtmosphere(); return; }
    // world time: 1 real second = 2 game minutes; the weather may turn every game hour
    int hourBefore = iS.timeOfDay / 60000, tenBefore = iS.timeOfDay / 10000;
    iS.timeOfDay += aDtUs / 500;
    if (iS.timeOfDay >= 1440000) { iS.timeOfDay -= 1440000; iS.day++; }
    if (iS.timeOfDay / 60000 != hourBefore) StepWeather();
    if (iS.timeOfDay / 10000 != tenBefore) RiverwoodSchedule();     // every ten game minutes: whoever could not go unseen
    iS.playTimeS = iTimeUs / 1000000;
    if (iDrawUs > 0) { iDrawUs -= aDtUs; if (iDrawUs < 0) iDrawUs = 0; }
    if (iUi == UI_PLAY) { UpdateProps(aDtUs); UpdatePlayer(aDtUs); }
    else if (Pressed(KEY_FIRE)) Respawn();
    UpdateEnts(aDtUs);
    UpdateProj(aDtUs);
    if (iUi == UI_PLAY) UpdateDungeon(aDtUs);
    FindFocus();
    FindPropFocus();
    FindFloraFocus();
    FindDoor();
    UpdateCamera(aDtUs);
    UpdateAtmosphere();
    }

void Game::KeyChar(int c)
    {
    if (iUi == UI_TITLE) { if (c == 'e' || c == 'E' || c == ' ' || c == CH_ENTER) iUi = UI_PLAY; return; }
    if (c == 'n' || c == 'N') { iS.outMode = (iS.outMode + 1) & 3; iR->SetOutputMode(iS.outMode); return; }
    if (iUi == UI_DEAD) { if (c == 'e' || c == 'E') Respawn(); return; }
    if (iUi == UI_PLAY)
        {
        switch (c)
            {
            case 'e': case 'E':
                if (iUsePending >= 0) break;                       // key repeat while holding E
                if (iGrab >= 0)
                    {
                    // carried: the next press of E lets it go (repeats of the press that picked it up are ignored)
                    if (iGrabArmed) { Prop& p = iS.prop[iGrab]; p.flags |= PF_FALL; p.vy = 0; iGrab = -1; }
                    break;
                    }
                if (iSitting >= 0) { StandUp(); break; }
                if (iFocusKind == 3) UseProp(iFocus);
                else if (iFocusKind == 5) TakeFlora(iFocus);
                else if (iDoorFocus >= 0 && iFocusKind == 0) UseDoor(iDoorFocus);
                else Activate();
                break;
            case 'r': case 'R': ToggleWeapon(); break;
            case 'c': case 'C': iCrouch = !iCrouch; break;
            case 'q': case 'Q': if (!iS.drawn) ToggleWeapon(); else if (iDrawUs == 0) CastSpell(); break;
            case 'x': case 'X': iShowModel = !iShowModel; Msg(iShowModel ? "Model names shown (X)" : "Model names hidden"); break;
            case 't': case 'T': StartWait(); break;
            case 'z': case 'Z': case 'y': case 'Y': Shout(); break;
            case 'i': case 'I': iUi = UI_INV; iInvCol = 0; iInvSel = 0; iInvScroll = 0; break;
            case 'm': case 'M': case CH_TAB: iUi = UI_MAP; iMapLocal = iMapPref && iS.cell == 0; iMapCX = iS.cell ? iS.extX : iS.px; iMapCZ = iS.cell ? iS.extZ : iS.pz; break;
            case 'j': case 'J': Msg2("Quest: ", QuestText()); break;
            case 'f': case 'F': iS.third = !iS.third; iCamDist = 0; break;
            case 'l': case 'L': ToggleTorch(); break;
            case 'o': case 'O': iDirect = !iDirect; break;
            case CH_ESC: iUi = UI_MENU; iMenuSel = 0; break;
            }
        return;
        }
    if (iUi == UI_CONT || iUi == UI_COOK || iUi == UI_TRADE) { ListKey(c); return; }
    if (iUi == UI_PUZZLE)
        {
        if (c == CH_ESC || c == CH_TAB) iUi = UI_PLAY;
        else if (c == 'e' || c == 'E' || c == CH_ENTER)
            {
            int r0 = iPuzDoor ? iS.dun[FH_RING0] : iS.bfb[BF_RING0], r1 = iPuzDoor ? iS.dun[FH_RING1] : iS.bfb[BF_RING1], r2 = iPuzDoor ? iS.dun[FH_RING2] : iS.bfb[BF_RING2];
            if (r0 == 0 && r1 == 1 && r2 == 2)
                {
                if (iPuzDoor) iS.dun[FH_CLAWDOOR] = 1; else iS.bfb[BF_CLAWDOOR] = 1;
                iUi = UI_PLAY; iShakeUs = 600000; Msg("The claw turns. The door rumbles open.");
                }
            else Msg("The claw turns, but the door does not move.");
            }
        return;
        }
    if (iUi == UI_WAIT)
        {
        if (c == CH_ESC || c == CH_TAB || c == 't' || c == 'T') { iUi = UI_PLAY; iWaitLeft = 0; iSleeping = 0; }
        else if ((c == 'e' || c == 'E' || c == CH_ENTER) && iWaitLeft == 0) { iWaitLeft = iWaitSel; iWaitAccUs = 0; }
        return;
        }
    if (iUi == UI_HELP)
        {
        // the controls page: any of Esc, Tab, E or the joystick press goes back to the menu
        if (c == CH_ESC || c == CH_TAB || c == 'e' || c == 'E' || c == CH_ENTER) iUi = UI_MENU;
        return;
        }
    if (iUi == UI_TELE)
        {
        if (c == CH_ESC) iUi = UI_MENU;
        else if (c == CH_TAB) iUi = UI_PLAY;
        else if (c == 'e' || c == 'E' || c == CH_ENTER) Teleport(iTeleSel);
        return;
        }
    if (iUi == UI_DIALOG)
        {
        if (c == CH_TAB) { iUi = UI_PLAY; iDlgId = -1; return; }
        if (c == 'e' || c == 'E' || c == ' ' || c == CH_ESC)
            {
            if (iDlgPage < 0) { iUi = UI_PLAY; iDlgId = -1; }
            else DialogNext();
            }
        return;
        }
    if (iUi == UI_BOOK)
        {
        if (c == CH_ESC || c == 'i' || c == 'I') { iUi = iBookProp >= 0 ? UI_PLAY : UI_INV; iBookProp = -1; return; }
        if (c == CH_TAB) { iUi = UI_PLAY; iBookProp = -1; return; }
        // E: take the book lying open in front of you (and close it, as in Skyrim); a book from the
        // inventory has nothing to take, there E turns the page. Space turns the page too.
        if ((c == 'e' || c == 'E') && iBookProp >= 0)
            {
            int p = iBookProp;
            iBookProp = -1; iUi = UI_PLAY;
            if (p < iS.nprop && !(iS.prop[p].flags & PF_GONE)) TakeProp(p);
            return;
            }
        if (c == 'e' || c == 'E' || c == ' ') iBookPage++;
        return;
        }
    if (iUi == UI_MAP)
        {
        if (c == CH_ESC || c == 'm' || c == 'M' || c == CH_TAB) iUi = UI_PLAY;
        // L (or Space / E): the local map and the world map in turn (the local one only outdoors)
        if (c == 'l' || c == 'L' || c == ' ' || c == 'e' || c == 'E')
            {
            if (iS.cell == 0) { iMapLocal = !iMapLocal; iMapPref = iMapLocal; iMapCX = iS.px; iMapCZ = iS.pz; }
            else Msg("No local map here");
            }
        // Z / Enter: closer, X: further (the world map: 3 steps; the local map: 2)
        if (c == CH_ENTER || c == 'z' || c == 'Z' || c == '+') { if (iMapLocal) iMapZoom = 0; else if (iMapWZ < 4) iMapWZ++; }
        if (c == 'x' || c == 'X' || c == '-') { if (iMapLocal) iMapZoom = 1; else if (iMapWZ > 0) iMapWZ--; }
        // (v0.4.74) T: to the place under the cross, anywhere on the land - a found place or not, a town, a forest
        if (c == 't' || c == 'T')
            {
            int x = Clamp(iMapCX, 8 * CELL, (iR->iGX - 8) * CELL), z = Clamp(iMapCZ, 8 * CELL, (iR->iGZ - 8) * CELL);
            iUi = UI_PLAY;
            Go(0, x, z, iS.yaw);
            Msg("You travel there.");
            }
        return;
        }
    if (iUi == UI_INV)
        {
        int list[MAXINV];
        int n = InvList(iInvCat, list, MAXINV);
        switch (c)
            {
            case CH_ESC: case 'i': case 'I': case CH_TAB: iUi = UI_PLAY; break;
            case 'e': case 'E':
                if (iInvCol == 0) { iInvCol = 1; iInvSel = 0; }
                else if (iInvSel < n) UseItem(list[iInvSel]);
                break;
            case 'r': case 'R':
                if (iInvCol == 1 && iInvSel < n)
                    {
                    InvItem& it = iS.inv[list[iInvSel]];
                    if (it.equipped) Msg("Unequip it first.");
                    else { Msg2(KItems[it.id].name, " dropped"); RemoveItem(list[iInvSel], 1); }
                    }
                break;
            case 'f': case 'F':
                if (iInvCol == 1 && iInvSel < n) iS.inv[list[iInvSel]].fav ^= 1;
                break;
            }
        return;
        }
    if (iUi == UI_MENU)
        {
        if (c == CH_ESC || c == CH_TAB) iUi = UI_PLAY;
        else if (c == 'e' || c == 'E') { u32 h = iHit; iHit |= KEY_FIRE; MenuInput(); iHit = h; }
        return;
        }
    }

// L: light a torch in the left hand (it takes the shield's place), or put it out
void Game::ToggleTorch()
    {
    int i;
    for (i = 0; i < iS.ninv; i++)
        if (iS.inv[i].id == IT_TORCH && iS.inv[i].equipped) { iS.inv[i].equipped = 0; Msg("Torch put away"); return; }
    for (i = 0; i < iS.ninv; i++)
        if (iS.inv[i].id == IT_TORCH) { UseItem(i); return; }
    Msg("You have no torch.");
    }

int Game::InvList(int aCat, int* aOut, int aMax)
    {
    int n = 0, i;
    for (i = 0; i < iS.ninv && n < aMax; i++)
        {
        const ItemDef& d = KItems[iS.inv[i].id];
        int ok;
        if (aCat == 0) ok = iS.inv[i].fav;
        else if (aCat == 1) ok = 1;
        else ok = d.cat == aCat - 2;
        if (ok) aOut[n++] = i;
        }
    return n;
    }

void Game::UiInput()
    {
    if (iUi == UI_INV) InvInput();
    else if (iUi == UI_MENU) MenuInput();
    else if (iUi == UI_DIALOG)
        {
        if (Pressed(KEY_FIRE))
            {
            if (iDlgPage < 0) { iUi = UI_PLAY; iDlgId = -1; }
            else DialogNext();
            }
        }
    else if (iUi == UI_BOOK)
        {
        // the pages: right arrow / D / Enter on, left arrow / A back
        if (Pressed(KEY_FIRE) || Pressed(KEY_LOOK_R) || Pressed(KEY_STRAFE_R)) iBookPage++;
        if ((Pressed(KEY_LOOK_L) || Pressed(KEY_STRAFE_L)) && iBookPage > 0) iBookPage--;
        }
    else if (iUi == UI_HELP)
        {
        if (Pressed(KEY_FIRE)) iUi = UI_MENU;
        if ((Pressed(KEY_LOOK_D) || Pressed(KEY_BACK)) && iListScroll < 18) iListScroll++;
        if ((Pressed(KEY_LOOK_U) || Pressed(KEY_FWD)) && iListScroll > 0) iListScroll--;
        }
    else if (iUi == UI_MAP)
        {
        if (Pressed(KEY_FIRE)) iUi = UI_PLAY;
        // the map pans with the movement and look keys: 80 screen pixels a second, faster the longer they are held
        // (up to four times in three seconds; run: twice more)
        int dt = iTimeUs - iMapPrevUs;
        iMapPrevUs = iTimeUs;
        if (dt < 0 || dt > 300000) dt = 0;
        u32 mv = iKeys & (KEY_FWD | KEY_LOOK_U | KEY_BACK | KEY_LOOK_D | KEY_STRAFE_L | KEY_LOOK_L | KEY_STRAFE_R | KEY_LOOK_R);
        if (mv) iMapHoldUs += dt; else iMapHoldUs = 0;
        int pxs = 80 + (iMapHoldUs > 3000000 ? 240 : iMapHoldUs / 12500);
        if (iKeys & KEY_RUN) pxs *= 2;
        // world units a screen pixel: the local map 1 or 2 pixels a metre; the world map by its zoom (Game::DrawMap)
        int upx;
        if (iMapLocal && !iS.cell) upx = iMapZoom ? METER : METER / 2;
        else
            {
            int w = iR->OutW(), mh = MapImgH();
            int z16 = iMapWZ == 0 ? OUT_H * 16 / mh : (iMapWZ == 1 ? w * 32 / MAPW : w * 64 / MAPW);
            upx = (int)((long long)iR->WorldW() * 16 / MAPW / z16);
            if (iMapWZ >= 3) upx = iMapWZ == 3 ? CELL : CELL / 2;           // (the close-up: a land vertex a pixel, or two)
            }
        // (the remainder kept: slow frames do not lose the fractions)
        long long sst = (long long)pxs * upx * dt + iMapRem;
        int st = (int)(sst / 1000000); iMapRem = (int)(sst - (long long)st * 1000000);
        if (iKeys & (KEY_FWD | KEY_LOOK_U)) iMapCZ += st;
        if (iKeys & (KEY_BACK | KEY_LOOK_D)) iMapCZ -= st;
        if (iKeys & (KEY_STRAFE_L | KEY_LOOK_L)) iMapCX -= st;
        if (iKeys & (KEY_STRAFE_R | KEY_LOOK_R)) iMapCX += st;
        }
    }

// the categories the inventory lists: favourites and all always, the rest only when you carry such things
int Game::InvCats(int* aOut)
    {
    int list[MAXINV];
    int n = 0, c;
    for (c = 0; c < NCATS; c++) if (c < 2 || InvList(c, list, MAXINV) > 0) aOut[n++] = c;
    return n;
    }

void Game::InvInput()
    {
    int list[MAXINV], cats[NCATS];
    int nc = InvCats(cats), at = -1, k;
    for (k = 0; k < nc; k++) if (cats[k] == iInvCat) at = k;
    if (at < 0) { iInvCat = 1; at = 1; }
    int n = InvList(iInvCat, list, MAXINV);
    if (Pressed(KEY_LOOK_U) || Pressed(KEY_FWD))
        {
        if (iInvCol == 0) { iInvCat = cats[(at + nc - 1) % nc]; iInvSel = 0; iInvScroll = 0; }
        else if (iInvSel > 0) iInvSel--;
        }
    if (Pressed(KEY_LOOK_D) || Pressed(KEY_BACK))
        {
        if (iInvCol == 0) { iInvCat = cats[(at + 1) % nc]; iInvSel = 0; iInvScroll = 0; }
        else if (iInvSel < n - 1) iInvSel++;
        }
    if (Pressed(KEY_LOOK_R) || Pressed(KEY_STRAFE_R)) { if (iInvCol == 0 && n > 0) { iInvCol = 1; iInvSel = 0; } }
    if (Pressed(KEY_LOOK_L) || Pressed(KEY_STRAFE_L)) iInvCol = 0;
    if (Pressed(KEY_FIRE))
        {
        if (iInvCol == 0) { if (n > 0) { iInvCol = 1; iInvSel = 0; } }
        else if (iInvSel < n) UseItem(list[iInvSel]);
        }
    n = InvList(iInvCat, list, MAXINV);
    if (iInvSel >= n) iInvSel = n > 0 ? n - 1 : 0;
    if (n == 0 && iInvCol == 1) iInvCol = 0;
    }

enum { MENU_RESUME = 0, MENU_SAVE, MENU_LOAD, MENU_CONTROLS, MENU_DISPLAY, MENU_GAMMA, MENU_VIEW, MENU_FAR, MENU_TEX, MENU_PLANTS, MENU_FULL, MENU_MIDDLE, MENU_ROWS, MENU_LAND, MENU_CLUTTER, MENU_AUTO, MENU_FPS, MENU_TRAVEL, MENU_NEW, MENU_UPDATE, MENU_QUIT, MENU_COUNT };

// the view settings, kept in iS.view: bits 0-2 the detailed land, bits 3-5 the far land (each a step in the lists
// below, 0 the default: 170 m, 1.5 km), bits 6-8 textures (0 off, the default: textured models flat in their
// colours, a quarter to a half faster; else on, out to 50, 100, 200 or 25 m)
static const short KNearM[8] = { 60, 80, 100, 120, 140, 170, 250, 350 };
// (v0.4.74) the far land in 14 steps (bits 3-5 and 21; stored 0 the default 1.5 km): the steps between Off and 500 m
// choose how far the distant things are drawn (a town seen across the plains) - under the draw distance, as Off
enum { NFAR = 14 };
static const short KFarM[NFAR] = { 0, 50, 100, 150, 200, 300, 400, 500, 1000, 1500, 2000, 3000, 4000, 6000 };
static int FarIdx(int v) { return ((((v >> 3) & 7) | (((v >> 21) & 1) << 3)) + 9) % NFAR; }
static int FarSet(int v, int aIdx) { int s = (aIdx + NFAR - 9) % NFAR; return (v & ~((7 << 3) | (1 << 21))) | ((s & 7) << 3) | ((s >> 3) << 21); }
int GameViewNear(int v) { return KNearM[(v + 5) & 7]; }
int GameViewFar(int v) { return KFarM[FarIdx(v)]; }
static const short KTexM[5] = { 0, 50, 100, 200, 25 };
// (v0.4.46) bits 10-12: Full models - the buildings' full model to this many metres off their nearest side (the first:
// the default; 0: Off, never - their middle one even close, and everything else its plain one); the others' full
// models (furniture 5 m, trees 12, rocks 6, people 7 outside and 4 inside, plants 7 at 3) in the same proportion.
// Bits 13-15: Middle models - the buildings' middle model to this many metres, their far one beyond
static const short KFullM[5] = { 3, 5, 8, 2, 0 };
static const short KMidM[5] = { 10, 15, 20, 30, 6 };
int GameViewFull(int v) { int i = (v >> 10) & 7; return KFullM[i < 5 ? i : 0]; }
int GameViewMiddle(int v) { int i = (v >> 13) & 7; return KMidM[i < 5 ? i : 0]; }
// the full models' distance for a thing whose own is aBase metres at the default (-1: none)
int GameFullDist(int v, int aBase) { int f = GameViewFull(v); return f ? (aBase * f + 1) / 3 : -1; }
int GameViewTex(int v) { int t = (v >> 6) & 7; return t > 4 ? 0 : KTexM[t]; }
void Game::ApplyViewSetting()
    {
    iR->SetViewDistance(GameViewNear(iS.view), GameViewFar(iS.view));
    int t = GameViewTex(iS.view);
    iR->iNoTex = !t; iR->iTexDist = (t ? t : 50) * METER;
    int f = GameViewFull(iS.view);
    iR->iLodNear = f ? f : -1;
    iR->iLodFar[LODC_BUILDING] = GameViewMiddle(iS.view);
    iR->iLodFar[LODC_TREE] = GameFullDist(iS.view, 12);
    iR->iLodFar[LODC_FURN] = GameFullDist(iS.view, 5);
    iR->iLodFar[LODC_ROCK] = GameFullDist(iS.view, 6);
    ApplySpeed();
    }

// (v0.4.46) one of iS.view's settings a step on (aDir 1) or back (-1): aBits from aShift, aN choices
static int ViewStep(int aView, int aShift, int aBits, int aN, int aDir)
    {
    int m = (1 << aBits) - 1, v = (aView >> aShift) & m;
    v = (v + aDir + aN) % aN;
    return (aView & ~(m << aShift)) | (v << aShift);
    }

void Game::MenuInput()
    {
    if (Pressed(KEY_LOOK_U) || Pressed(KEY_FWD)) iMenuSel = (iMenuSel + MENU_COUNT - 1) % MENU_COUNT;
    if (Pressed(KEY_LOOK_D) || Pressed(KEY_BACK)) iMenuSel = (iMenuSel + 1) % MENU_COUNT;
    // (v0.4.46) a setting: the press or right a step on, left a step back; the rest only by the press
    int dir = Pressed(KEY_FIRE) || Pressed(KEY_LOOK_R) || Pressed(KEY_STRAFE_R) ? 1 : (Pressed(KEY_LOOK_L) || Pressed(KEY_STRAFE_L) ? -1 : 0);
    if (!dir) return;
    switch (iMenuSel)
        {
        case MENU_DISPLAY: iS.outMode = (iS.outMode + dir) & 3; iR->SetOutputMode(iS.outMode); return;
        case MENU_GAMMA: iS.gamma = (iS.gamma + dir) & 3; iR->SetGamma(iS.gamma); return;
        case MENU_VIEW: iS.view = ViewStep(iS.view, 0, 3, 8, dir); ApplyViewSetting(); return;
        case MENU_FAR: iS.view = FarSet(iS.view, (FarIdx(iS.view) + dir + NFAR) % NFAR); ApplyViewSetting(); return;
        case MENU_TEX: iS.view = ViewStep(iS.view, 6, 3, 5, dir); ApplyViewSetting(); return;
        case MENU_PLANTS: iS.view ^= 1 << 9; return;             // (bit 9: the land's bushes and flowers left out)
        case MENU_FULL: iS.view = ViewStep(iS.view, 10, 3, 5, dir); ApplyViewSetting(); return;
        case MENU_MIDDLE: iS.view = ViewStep(iS.view, 13, 3, 5, dir); ApplyViewSetting(); return;
        case MENU_ROWS: iS.view ^= 1 << 16; ApplyViewSetting(); return;       // (v0.4.50 bit 16: half rows)
        case MENU_LAND: iS.view ^= 1 << 17; ApplyViewSetting(); return;       // (bit 17: the near land flat)
        case MENU_CLUTTER: iS.view ^= 1 << 18; ApplyViewSetting(); return;    // (bit 18: small things only near)
        case MENU_AUTO: iS.view = ViewStep(iS.view, 19, 2, 3, dir); iAutoLevel = 0; iAutoGood = 0; ApplyViewSetting(); return;   // (bits 19-20: Off, 8, 10 fps)
        case MENU_FPS: ShowFps = !ShowFps; return;
        }
    if (!Pressed(KEY_FIRE)) return;
    switch (iMenuSel)
        {
        case MENU_RESUME: iUi = UI_PLAY; break;
        case MENU_SAVE: iRequest = 1; break;
        case MENU_LOAD: iRequest = 2; break;
        case MENU_TRAVEL: iUi = UI_TELE; break;
        case MENU_CONTROLS: iUi = UI_HELP; iListScroll = 0; break;
        case MENU_NEW:
            {
            NewGame();
            iR->SetOutputMode(iS.outMode);
            Msg("A new journey begins.");
            break;
            }
        case MENU_UPDATE: iRequest = 4; iUi = UI_PLAY; break;        // (v0.4.58: the app downloads skyrim.sis and installs it)
        case MENU_QUIT: iRequest = 3; break;
        }
    }

void Game::SaveDone(int aOk)
    {
    iRequest = 0;
    Msg(aOk ? "Game saved." : "Saving failed.");
    iUi = UI_PLAY;
    }

void Game::LoadDone(int aOk)
    {
    iRequest = 0;
    if (!aOk || iLoad.magic != 0x48524D46 || iLoad.version != SAVE_VERSION)
        {
        Msg(aOk ? "That save is from an older version." : "No saved game found.");
        return;
        }
    // accept: copy word by word (no C library)
    {
    const int* a = (const int*)&iLoad;
    int* b = (int*)&iS;
    int n = sizeof(GameState) / 4, k;
    for (k = 0; k < n; k++) b[k] = a[k];
    }
    if (!DeferLoad(3)) LoadFinish();            // (v0.4.74: the loading screen first)
    }

// the rest of a load: the save's area and land in, the game set going from it
void Game::LoadFinish()
    {
    UseArea(AreaOf(iS.cell, iS.cell ? iS.extX : iS.px, iS.cell ? iS.extZ : iS.pz));     // (v0.4.55: the area's file the save is in)
    iR->iCell = iS.cell;
    iR->FocusLand(iS.cell ? iS.extX : iS.px, iS.cell ? iS.extZ : iS.pz);    // the land round where the save was made
    RebuildFloraTaken();
    r3d_memset(iBfTimer, 0, sizeof(iBfTimer));
    iPrevWeather = iS.weather; iWBlend = 256;
    iDrawUs = 0; iCamDist = 0; iWaitLeft = 0;
    int i;
    for (i = 0; i < MAXP; i++) iP[i].active = 0;
    for (i = 0; i < iS.ne; i++) if (iS.e[i].type == E_DRAGON) iDragonIdx = i;
    iR->SetOutputMode(iS.outMode & 3);
    iR->SetGamma(iS.gamma & 3);
    iS.view &= 1023; ApplyViewSetting();
    iGrab = -1; iUsePending = -1; iSitting = -1; iOpenProp = -1; iOpenWaitUs = 0;
    UpdateAtmosphere();
    iUi = UI_PLAY;
    Msg("Game loaded.");
    }

void Game::ReportTimes(int aRenderUs, int aBlitUs, int aFrameUs)
    {
    iRenderUs = aRenderUs; iBlitUs = aBlitUs;
    // (v0.4.75) the frame rate shown: whole frames (the game's own work too, not only the drawing and the copy) over
    // about a second - the phone's clock ticks 64 times a second, so one frame's time is off by up to a tick (+-10 %
    // at 6 fps)
    if (aFrameUs > 0 && aFrameUs < 1000000)
        {
        iFrames++; iFpsAccUs += aFrameUs; iRenderAccUs += aRenderUs; iBlitAccUs += aBlitUs;
        if (iFpsAccUs >= 1000000)
            {
            iFps10 = iFrames * 10000000 / iFpsAccUs;
            iRenderAvgMs = iRenderAccUs / 1000 / iFrames; iBlitAvgMs = iBlitAccUs / 1000 / iFrames;
            iFrames = 0; iFpsAccUs = 0; iRenderAccUs = 0; iBlitAccUs = 0;
            }
        }
    // (v0.4.51) Auto speed (the menu: Off, 8 or 10 fps): the frames' time over a second and a half while playing (the
    // clock ticks 64 times a second - one frame's time is not to be trusted, their sum is); too slow, a step faster -
    // the small things only near, then the near land flat, then half rows; well inside the time (a step slower would
    // still fit: under 78% of it) twice running, a step back. The steps add to what the settings already ask for.
    int want = (iS.view >> 19) & 3;
    if (!want || iUi != UI_PLAY || aFrameUs <= 0 || aFrameUs > 1000000)
        {
        iAutoAccUs = iAutoN = 0;
        if (!want && iAutoLevel) { iAutoLevel = 0; ApplySpeed(); }
        return;
        }
    iAutoAccUs += aFrameUs; iAutoN++;
    if (iAutoAccUs < 1500000) return;
    int avg = iAutoAccUs / iAutoN, target = want == 1 ? 125000 : 100000;
    iAutoAccUs = iAutoN = 0;
    if (avg > target + target / 20) { iAutoGood = 0; if (iAutoLevel < 3) { iAutoLevel++; ApplySpeed(); } }
    else if (avg < target * 78 / 100 && iAutoLevel > 0) { if (++iAutoGood >= 2) { iAutoGood = 0; iAutoLevel--; ApplySpeed(); } }
    else iAutoGood = 0;
    }

void Game::ApplySpeed()
    {
    iR->iHalfRows = ((iS.view >> 16) & 1) || iAutoLevel >= 3;
    iR->iFlatLand = ((iS.view >> 17) & 1) || iAutoLevel >= 2;
    iR->iClutterNear = ((iS.view >> 18) & 1) || iAutoLevel >= 1;
    }

// helpers shared with game_ui.cpp
const ItemDef& GameItem(int aId) { return KItems[aId]; }
const u8* GameItemCol(int aId) { return KItemCol + aId * 9; }
const char* GameCatName(int aCat) { return KCatName[aCat]; }
const char* GameFhBook(int aId);
const char* GameBook(int aId) { return aId < 6 ? KBooks[aId] : GameFhBook(aId - 6); }     // (v0.4.72: 6.. Forelhost's)
const char* GameDunDlgLine(int aId, int aPage);
const char* GameDlgLine(int aDlg, int aPage)
    {
    if (aDlg >= 100) return GameDunDlgLine(aDlg - 100, aPage);
    if (aPage < 0) return KDlgLines[14 + Clamp(-aPage - 1, 0, (int)(sizeof(KDlgLines) / sizeof(KDlgLines[0])) - 15)];
    return KDlgLines[KDlg[aDlg].first + aPage];
    }
const char* GameWord(int i) { return KWords[i]; }
int GameVillageX() { return Vx(0); }          // Whiterun (map marker, title view)
int GameVillageZ() { return Vz(0); }
int GameHomeX() { return Vx(-86); }              // where you wake: the road below the stables
int GameHomeZ() { return Vz(-102); }

