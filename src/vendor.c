/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "u4.h"
#include "context.h"
#include "savegame.h"
#include "person.h"
#include "u4file.h"
#include "names.h"
#include "io.h"
#include "stats.h"
#include "map.h"
#include "player.h"

/*
 * Warning -- this is not the nicest code; vendors are (so far) the
 * most complicated part of xu4, with lots of special cases.  Also,
 * I'm figuring out how they work as I go along which doesn't help.
 * It is slowly getting better.
 */

#define N_ARMS_VENDORS 6
#define ARMS_VENDOR_INVENTORY_SIZE 4

#define VCM_SIZE 16

/*
 * This structure hold information about each vendor type.  The
 * offsets are byte offsets into the AVATAR.EXE file where the vendor
 * dialog strings and other data are stored.
 */
typedef struct VendorTypeDesc {
    long cityMapOffset;        
    unsigned char n_shops;
    long shopNamesOffset;
    long shopkeeperNamesOffset;
    unsigned char n_items;
    long itemDescriptionsOffset;
    int n_text;
    long textOffset;
    int n_text2;
    long text2Offset;
} VendorTypeDesc;

/*
 * This structure hold the information loading in from AVATAR.EXE for
 * each vendor type.  Each VendorTypeInfo is loaded from a
 * VendorTypeDesc.
 */
typedef struct _VendorTypeInfo {
    unsigned char cityMap[VCM_SIZE]; /* a byte for each city with the index
                                        (starting at 1) of the vendor */
    int n_shops;                /* number of shops of this vendor type */
    char **shopNames;           /* the name of each shop */
    char **shopkeeperNames;     /* the name of each shopkeeper */
    int n_items;                /* the number of items each vendor sells */
    char **itemDescriptions;    /* a description of each item */
    int n_text;                 /* text and text2 are arrays of the */
    char **text;                /* remaining dialog strings */
    int n_text2;
    char **text2;
} VendorTypeInfo;

typedef struct ArmsVendorInfo {
    unsigned char vendorInventory[N_ARMS_VENDORS][ARMS_VENDOR_INVENTORY_SIZE];
    unsigned short prices[WEAP_MAX];
} ArmsVendorInfo;

#define N_REAG_VENDORS 4
#define N_FOOD_VENDORS 5
#define N_TAVERN_VENDORS 6

ArmsVendorInfo weaponVendorInfo;
ArmsVendorInfo armorVendorInfo;
unsigned char reagPrices[N_REAG_VENDORS][REAG_MAX];
unsigned short foodPrices[N_FOOD_VENDORS];
unsigned short tavernFoodPrices[N_TAVERN_VENDORS];

#define WV_NOTENOUGH 0
#define WV_FINECHOICE 1
#define WV_VERYGOOD 100
#define WV_WEHAVE 101
#define WV_YOURINTEREST 102
#define WV_HOWMANYTOBUY 104
#define WV_TOOBAD 105
#define WV_ANYTHINGELSE 106
#define WV_WHATWILL 107
#define WV_YOUSELL 108
#define WV_DONTOWN 109
#define WV_HOWMANY 110
#define WV_TOSELL 111
#define WV_DONTOWNENOUGH 113
#define WV_WELCOME 123
#define WV_SPACER 124
#define WV_BUYORSELL 125
#define WV_BYE 126

#define FV_WELCOME 0
#define FV_SPACER 1
#define FV_GOODDAY 2
#define FV_GOODBYE 3
#define FV_PRICE1 4
#define FV_PRICE2 5
#define FV_HOWMANY 6
#define FV_TOOBAD 7
#define FV_CANTAFFORD 8
#define FV_AFFORDONLY1 9
#define FV_AFFORDONLY2 10
#define FV_ANYMORE 11
#define FV_BYE 12

#define TV_SPECIALTY 0
#define TV_COSTS 1
#define TV_HOWMANY 2
#define TV_CANTAFFORD 3
#define TV_AFFORDONLY1 4
#define TV_AFFORDONLY2 5
#define TV_BESTMUG 101
#define TV_WELCOME 112
#define TV_FOODORALE 113
#define TV_HEREYOUARE 114
#define TV_BYE 115

#define RV_WELCOME 0
#define RV_IAM 1
#define RV_NEEDREAGENTS 2
#define RV_VERYWELL 3
#define RV_MENU 4
#define RV_BYE 15

#define HV_WELCOME 114
#define HV_NEEDHELP 115
#define HV_CANPERFORM 116
#define HV_YOURNEED 117
#define HV_BYE 119

#define IV_WEHAVE 0
#define IV_MORNING 100
#define IV_HORSE 101
#define IV_WELCOME 102
#define IV_IAM 103
#define IV_NEEDLODGING 104
#define IV_BEDS 105
#define IV_TAKEIT 106
#define IV_NOBETTERDEAL 107
#define IV_CANTPAY 108
#define IV_GOODNIGHT 109
#define IV_RATS 110
#define IV_WRONGPLACE 111

VendorTypeInfo *vendorLoadTypeInfo(FILE *avatar, const VendorTypeDesc *desc);
const char *vendorGetName(const Person *v);
const char *vendorGetShop(const Person *v);
const char *vendorGetText(const Person *v, int textId);
const VendorTypeInfo *vendorGetInfo(const Person *v);
char *vendorGetArmsVendorMenu(const Person *v);
int armsVendorInfoRead(ArmsVendorInfo *info, int nprices, FILE *f);

const VendorTypeDesc vendorTypeDesc[] = {
    { 80237, 6, 78883, -1, 14, 79015, 2, 80077, 27, 80282 },    /* weapons */
    { 81511, 5, 80803, -1, 6, 80920, 2, 81374, 27, 81540 },     /* armor */
    { 87519, 5, 87063, -1, 0, 0, 13, 87168, 0, 0 },             /* food */
    { 86363, 6, 85385, -1, 0, 0, 6, 86235, 16, 86465 },         /* tavern */
    { 78827, 4, 78295, -1, 0, 0, 16, 78377, 0, 0 },             /* reagents */
    { 84475, 10, 84209, -1, 0, 0, 3, 84439, 20, 84531 },        /* healer */
    { 83703, 7, 83001, -1, 0, 0, 8, 83158, 12, 83785 }          /* inn */
};

#define N_VENDOR_TYPE_DESCS (sizeof(vendorTypeDesc) / sizeof(vendorTypeDesc[0]))

VendorTypeInfo **vendorTypeInfo;

/**
 * Loads in prices and conversation data for vendors from avatar.exe.
 */
int vendorInit() {
    int i, j;
    FILE *avatar;

    avatar = u4fopen("avatar.exe");
    if (!avatar)
        return 0;

    fseek(avatar, 78859, SEEK_SET);
    for (i = 0; i < N_REAG_VENDORS; i++) {
        for (j = 0; j < REAG_MAX - 2; j++) {
            if (!readChar(&(reagPrices[i][j]), avatar))
                return 0;
        }
        reagPrices[i][REAG_NIGHTSHADE] = 0;
        reagPrices[i][REAG_MANDRAKE] = 0;
    }

    fseek(avatar, 87535, SEEK_SET);
    for (i = 0; i < N_FOOD_VENDORS; i++) {
        if (!readShort(&(foodPrices[i]), avatar))
            return 0;
    }

    fseek(avatar, 86427, SEEK_SET);
    for (i = 0; i < N_TAVERN_VENDORS; i++) {
        if (!readShort(&(tavernFoodPrices[i]), avatar))
            return 0;
    }

    fseek(avatar, 80181, SEEK_SET);
    if (!armsVendorInfoRead(&weaponVendorInfo, WEAP_MAX, avatar))
        return 0;

    fseek(avatar, 81471, SEEK_SET);
    if (!armsVendorInfoRead(&armorVendorInfo, ARMR_MAX, avatar))
        return 0;

    vendorTypeInfo = (VendorTypeInfo **) malloc(sizeof(VendorTypeInfo *) * N_VENDOR_TYPE_DESCS);
    for (i = 0; i < N_VENDOR_TYPE_DESCS; i++)
        vendorTypeInfo[i] = vendorLoadTypeInfo(avatar, &(vendorTypeDesc[i]));

    u4fclose(avatar);

    return 1;
}

/**
 * Gets the vendor number for the given vendor.  Vendor number is
 * derived from the vendor type and the city; it is looked up in a
 * vendor city map that gives the vendor number for each city.
 */
int vendorGetVendorNo(const Person *v) {
    int type;

    if (v->npcType < NPC_VENDOR_WEAPONS ||
        v->npcType > NPC_VENDOR_INN)
        return 0;

    type = v->npcType - NPC_VENDOR_WEAPONS;

    assert((c->map->id - 1) < VCM_SIZE);
    assert(vendorTypeInfo[type]->cityMap[c->map->id - 1] != 0);

    return vendorTypeInfo[type]->cityMap[c->map->id - 1] - 1;
}

/**
 * Returns a piece of text identified by textId for the given vendor.
 */
const char *vendorGetText(const Person *v, int textId) {
    if (textId >= 100)
        return vendorGetInfo(v)->text2[textId - 100];
    else
        return vendorGetInfo(v)->text[textId];
}

/**
 * Returns the name of the given vendor.
 */
const char *vendorGetName(const Person *v) {
    return vendorGetInfo(v)->shopkeeperNames[vendorGetVendorNo(v)];
}

/**
 * Returns the shop name of the given vendor.
 */
const char *vendorGetShop(const Person *v) {
    return vendorGetInfo(v)->shopNames[vendorGetVendorNo(v)];
}

/**
 * Returns the raw info struct of the given vendor.
 */
const VendorTypeInfo *vendorGetInfo(const Person *v) {
    int type;
    assert(v->npcType >= NPC_VENDOR_WEAPONS && v->npcType <= NPC_VENDOR_INN);

    type = v->npcType - NPC_VENDOR_WEAPONS;

    return vendorTypeInfo[type];
}

char *vendorGetArmsVendorMenu(const Person *v) {
    char *menu, *tmp, buffer[17];
    int i;
    const ArmsVendorInfo *info;

    if (v->npcType == NPC_VENDOR_WEAPONS)
        info = &weaponVendorInfo;
    else
        info = &armorVendorInfo;

    menu = strdup("");
    for (i = 0; i < ARMS_VENDOR_INVENTORY_SIZE; i++) {
        if (info->vendorInventory[vendorGetVendorNo(v)][i] != 0) {
            const char *name;

            if (v->npcType == NPC_VENDOR_WEAPONS)
                name = getWeaponName((WeaponType) info->vendorInventory[vendorGetVendorNo(v)][i]);
            else
                name = getArmorName((ArmorType) info->vendorInventory[vendorGetVendorNo(v)][i]);

            snprintf(buffer, sizeof(buffer), "%c-%s\n", 'A' + info->vendorInventory[vendorGetVendorNo(v)][i], name);
            tmp = concat(menu, buffer, NULL);
            free(menu);
            menu = tmp;
        }
    }

    return menu;
}

char *vendorGetIntro(Conversation *cnv) {
    char *intro;

    switch (cnv->talker->npcType) {
    case NPC_VENDOR_WEAPONS:
    case NPC_VENDOR_ARMOR:
        c->statsItem = cnv->talker->npcType == NPC_VENDOR_WEAPONS ? STATS_WEAPONS : STATS_ARMOR;
        statsUpdate();
        intro = concat(vendorGetText(cnv->talker, WV_WELCOME), vendorGetShop(cnv->talker),
                       vendorGetText(cnv->talker, WV_SPACER), vendorGetName(cnv->talker),
                       vendorGetText(cnv->talker, WV_BUYORSELL),
                       NULL);
        cnv->state = CONV_VENDORQUESTION;
        break;

    case NPC_VENDOR_FOOD:
        intro = concat(vendorGetText(cnv->talker, FV_WELCOME), vendorGetShop(cnv->talker),
                       vendorGetText(cnv->talker, FV_SPACER), vendorGetName(cnv->talker),
                       vendorGetText(cnv->talker, FV_GOODDAY),
                       NULL);
        cnv->state = CONV_CONTINUEQUESTION;
        break;

    case NPC_VENDOR_TAVERN:
        intro = concat(vendorGetName(cnv->talker), vendorGetText(cnv->talker, TV_WELCOME), 
                       vendorGetShop(cnv->talker), "\n", vendorGetName(cnv->talker), 
                       vendorGetText(cnv->talker, TV_FOODORALE), 
                       NULL);
        cnv->state = CONV_VENDORQUESTION;
        break;

    case NPC_VENDOR_REAGENTS:
        c->statsItem = STATS_REAGENTS;
        intro = concat(vendorGetText(cnv->talker, RV_WELCOME), vendorGetShop(cnv->talker),
                       vendorGetText(cnv->talker, RV_IAM), vendorGetName(cnv->talker),
                       vendorGetText(cnv->talker, RV_NEEDREAGENTS),
                       NULL);
        cnv->state = CONV_CONTINUEQUESTION;
        break;

    case NPC_VENDOR_HEALER:
        intro = concat(vendorGetText(cnv->talker, HV_WELCOME), vendorGetShop(cnv->talker), "\n",
                       vendorGetName(cnv->talker), vendorGetText(cnv->talker, HV_NEEDHELP),
                       NULL);
        cnv->state = CONV_CONTINUEQUESTION;
        break;

    case NPC_VENDOR_INN:
        intro = concat(vendorGetText(cnv->talker, IV_WELCOME), vendorGetShop(cnv->talker), 
                       vendorGetText(cnv->talker, IV_IAM), vendorGetName(cnv->talker), 
                       vendorGetText(cnv->talker, IV_NEEDLODGING), NULL);
        cnv->state = CONV_CONTINUEQUESTION;
        break;

    case NPC_VENDOR_GUILD:
        intro = strdup("I am a guild vendor!\n");
        cnv->state = CONV_DONE;
        break;

    case NPC_VENDOR_STABLE:
        intro = strdup("I am a horse vendor!\n");
        cnv->state = CONV_DONE;
        break;

    default:
        assert(0);          /* shouldn't happen */
    }

    return intro;
}

char *vendorGetPrompt(const Conversation *cnv) {
    char *prompt;

    switch (cnv->state) {

    case CONV_VENDORQUESTION:
    case CONV_CONTINUEQUESTION:
        prompt = strdup("");
        break;

    case CONV_BUY_ITEM:
        prompt = strdup(vendorGetText(cnv->talker, WV_YOURINTEREST));
        break;
        
    case CONV_SELL_ITEM:
        prompt = strdup(vendorGetText(cnv->talker, WV_YOUSELL));
        break;

    case CONV_BUY_QUANTITY:
        switch (cnv->talker->npcType) {
        case NPC_VENDOR_WEAPONS:
        case NPC_VENDOR_ARMOR:
            prompt = strdup(vendorGetText(cnv->talker, WV_HOWMANYTOBUY));
            break;
        default:
            prompt = strdup("");
            break;
        }
        break;

    case CONV_SELL_QUANTITY:
        prompt = strdup("");
        break;

    default:
        assert(0);
    }

    return prompt;
}

char *vendorGetVendorQuestionResponse(Conversation *cnv, const char *response) {
    char buffer[10];
    char *reply;
    
    switch (cnv->talker->npcType) {

    case NPC_VENDOR_WEAPONS:
    case NPC_VENDOR_ARMOR:
        if (tolower(response[0]) == 'b') {
            char *menu;
            menu = vendorGetArmsVendorMenu(cnv->talker);
            reply = concat(vendorGetText(cnv->talker, WV_VERYGOOD),
                           vendorGetText(cnv->talker, WV_WEHAVE),
                           menu, 
                           NULL);
            free(menu);

            cnv->state = CONV_BUY_ITEM;
        }    

        else if (tolower(response[0]) == 's') {
            reply = strdup(vendorGetText(cnv->talker, WV_WHATWILL));
            cnv->state = CONV_SELL_ITEM;
        }

        else {
            reply = strdup("");
            cnv->state = CONV_DONE;
        }

        break;

    case NPC_VENDOR_TAVERN:
        if (tolower(response[0]) == 'f') {
            snprintf(buffer, sizeof(buffer), "%d", tavernFoodPrices[vendorGetVendorNo(cnv->talker)]);
            reply = concat(vendorGetText(cnv->talker, TV_SPECIALTY), "FIXME",
                           vendorGetText(cnv->talker, TV_COSTS), buffer,
                           vendorGetText(cnv->talker, TV_HOWMANY),
                           NULL);
            cnv->state = CONV_BUY_QUANTITY;
        }
        else if (tolower(response[0]) == 'a') {
            reply = strdup(vendorGetText(cnv->talker, TV_BESTMUG));
            cnv->state = CONV_BUY_QUANTITY;
        } 
        else {
            reply = strdup("");
            cnv->state = CONV_DONE;
        }
        break;

    default:
        reply = strdup("");
        cnv->state = CONV_DONE;
        break;
    }
    return reply;
}

char *vendorGetBuyItemResponse(Conversation *cnv, const char *response) {
    char *reply;
    int i;
    const ArmsVendorInfo *info;

    cnv->item = -1;

    if (cnv->talker->npcType == NPC_VENDOR_WEAPONS)
        info = &weaponVendorInfo;
    else
        info = &armorVendorInfo;

    if (response[0] == '\033') {
        cnv->state = CONV_DONE;
        return concat(vendorGetName(cnv->talker), vendorGetText(cnv->talker, WV_BYE), NULL);
    }

    for (i = 0; i < ARMS_VENDOR_INVENTORY_SIZE; i++) {
        if (info->vendorInventory[vendorGetVendorNo(cnv->talker)][i] != 0 &&
            (tolower(response[0]) - 'a') == info->vendorInventory[vendorGetVendorNo(cnv->talker)][i])
            cnv->item = info->vendorInventory[vendorGetVendorNo(cnv->talker)][i];
    }

    if (cnv->item == -1)
        reply = strdup("");
    else {
        reply = strdup(vendorGetInfo(cnv->talker)->itemDescriptions[cnv->item - 1]);
        cnv->state = CONV_BUY_QUANTITY;
    }

    return reply;
}

char *vendorGetSellItemResponse(Conversation *cnv, const char *response) {
    char *reply;
    int itemMax;

    cnv->item = tolower(response[0]) - 'a';

    if (cnv->talker->npcType == NPC_VENDOR_WEAPONS)
        itemMax = WEAP_MAX;
    else
        itemMax = ARMR_MAX;

    if (cnv->item >= itemMax) {
        return strdup("");
    } 
    else if (response[0] == '\033') {
        cnv->state = CONV_DONE;
        return concat(vendorGetName(cnv->talker), vendorGetText(cnv->talker, WV_BYE), NULL);
    }

    if (cnv->item == 0 ||
        (cnv->talker->npcType == NPC_VENDOR_WEAPONS && c->saveGame->weapons[cnv->item] < 1) ||
        (cnv->talker->npcType == NPC_VENDOR_ARMOR && c->saveGame->armor[cnv->item] < 1)) {
        reply = strdup(vendorGetText(cnv->talker, WV_DONTOWN));
        return reply;
    }

    switch (cnv->talker->npcType) {
    case NPC_VENDOR_WEAPONS:
        reply = concat(vendorGetText(cnv->talker, WV_HOWMANY),
                       getWeaponName((WeaponType) cnv->item),
                       vendorGetText(cnv->talker, WV_TOSELL),
                       NULL);
        break;
    case NPC_VENDOR_ARMOR:
        reply = concat(vendorGetText(cnv->talker, WV_HOWMANY),
                       getArmorName((ArmorType) cnv->item), 
                       vendorGetText(cnv->talker, WV_TOSELL),
                       NULL);
        break;
    default:
        assert(0);              /* shouldn't happen */
    }
    cnv->state = CONV_SELL_QUANTITY;

    return reply;
}

char *vendorGetBuyQuantityResponse(Conversation *cnv, const char *response) {
    char *reply;
    int totalprice, success;
    InventoryItem item;
    
    cnv->quant = (int) strtol(response, NULL, 10);

    switch (cnv->talker->npcType) {
    case NPC_VENDOR_WEAPONS:
        totalprice = weaponVendorInfo.prices[cnv->item] * cnv->quant;
        item = INV_WEAPON;
        break;
    case NPC_VENDOR_ARMOR:
        totalprice = armorVendorInfo.prices[cnv->item] * cnv->quant;
        item = INV_ARMOR;
        break;
    case NPC_VENDOR_FOOD:
        totalprice = foodPrices[vendorGetVendorNo(cnv->talker)] * cnv->quant;
        cnv->quant *= 25;
        item = INV_FOOD;
        cnv->item = 0;
        break;
    case NPC_VENDOR_TAVERN:
        totalprice = tavernFoodPrices[vendorGetVendorNo(cnv->talker)] * cnv->quant;
        item = INV_FOOD;
        cnv->item = 0;
        break;

    default:
        assert(0);              /* shouldn't happen */
    }

    success = playerPurchase(c->saveGame, item, cnv->item, cnv->quant, totalprice);

    switch (cnv->talker->npcType) {

    case NPC_VENDOR_WEAPONS:
    case NPC_VENDOR_ARMOR:
        if (success)
            reply = concat(vendorGetName(cnv->talker), vendorGetText(cnv->talker, WV_FINECHOICE), 
                           vendorGetText(cnv->talker, WV_ANYTHINGELSE), NULL);
        else
            reply = strdup(vendorGetText(cnv->talker, WV_NOTENOUGH));
        cnv->state = CONV_CONTINUEQUESTION;
        break;

    case NPC_VENDOR_FOOD:
        if (success) {
            reply = strdup(vendorGetText(cnv->talker, FV_ANYMORE));
            cnv->state = CONV_CONTINUEQUESTION;
        } else {
            char buffer[10];

            if (c->saveGame->gold / foodPrices[vendorGetVendorNo(cnv->talker)] == 0) {
                reply = concat(vendorGetText(cnv->talker, FV_CANTAFFORD), 
                               vendorGetText(cnv->talker, FV_BYE), 
                               NULL);
                cnv->state = CONV_DONE;
            } else {
                snprintf(buffer, sizeof(buffer), "%d", c->saveGame->gold / foodPrices[vendorGetVendorNo(cnv->talker)]);
                reply = concat(vendorGetText(cnv->talker, FV_AFFORDONLY1),
                               buffer,
                               vendorGetText(cnv->talker, FV_AFFORDONLY2),
                               vendorGetText(cnv->talker, FV_HOWMANY),
                               NULL);
                cnv->state = CONV_BUY_QUANTITY;
            }
        }
        break;

    case NPC_VENDOR_TAVERN:
        if (success) {
            reply = strdup(vendorGetText(cnv->talker, TV_HEREYOUARE));
            cnv->state = CONV_CONTINUEQUESTION;
        } else {
            char buffer[10];

            if (c->saveGame->gold / tavernFoodPrices[vendorGetVendorNo(cnv->talker)] == 0) {
                reply = concat(vendorGetText(cnv->talker, TV_CANTAFFORD), 
                               vendorGetText(cnv->talker, TV_HEREYOUARE),
                               NULL);
                cnv->state = CONV_CONTINUEQUESTION;
            } else {
                snprintf(buffer, sizeof(buffer), "%d", c->saveGame->gold / tavernFoodPrices[vendorGetVendorNo(cnv->talker)]);
                reply = concat(vendorGetText(cnv->talker, TV_AFFORDONLY1),
                               buffer,
                               vendorGetText(cnv->talker, TV_AFFORDONLY2), "s.\n",
                               vendorGetText(cnv->talker, TV_HEREYOUARE),
                               NULL);
                cnv->state = CONV_CONTINUEQUESTION;
            }
        }
        break;

    default:
        assert(0);              /* shouldn't happen */
    }

    statsUpdate();
    return reply;
}

char *vendorGetSellQuantityResponse(Conversation *cnv, const char *response) {
    char *reply;
    int totalprice, success;
    InventoryItem item;

    cnv->quant = (int) strtol(response, NULL, 10);

    switch (cnv->talker->npcType) {
    case NPC_VENDOR_WEAPONS:
        totalprice = weaponVendorInfo.prices[cnv->item] * cnv->quant / 2;
        item = INV_WEAPON;
        break;
    case NPC_VENDOR_ARMOR:
        totalprice = armorVendorInfo.prices[cnv->item] * cnv->quant / 2;
        item = INV_ARMOR;
        break;

    default:
        assert(0);              /* shouldn't happen */
    }

    success = playerSell(c->saveGame, item, cnv->item, cnv->quant, totalprice);

    switch (cnv->talker->npcType) {

    case NPC_VENDOR_WEAPONS:
    case NPC_VENDOR_ARMOR:
        if (success)
            reply = concat(vendorGetName(cnv->talker), vendorGetText(cnv->talker, WV_FINECHOICE), 
                           vendorGetText(cnv->talker, WV_ANYTHINGELSE), NULL);
        else
            reply = concat(vendorGetText(cnv->talker, WV_DONTOWNENOUGH));

        cnv->state = CONV_CONTINUEQUESTION;
        break;

    default:
        assert(0);              /* shouldn't happen */
    }

    statsUpdate();
    return reply;
}

char *vendorGetContinueQuestionResponse(Conversation *cnv, const char *answer) {
    char buffer[10];
    char *reply;
    int cont;
    
    if (tolower(answer[0]) == 'y')
        cont = 1;
    else if (tolower(answer[0]) == 'n')
        cont = 0;
    else {
        cnv->state = CONV_DONE;
        return strdup("");
    }

    switch (cnv->talker->npcType) {

    case NPC_VENDOR_WEAPONS:
    case NPC_VENDOR_ARMOR:
        if (cont) {
            char *menu;
            menu = vendorGetArmsVendorMenu(cnv->talker);
            reply = concat(vendorGetText(cnv->talker, WV_WEHAVE),
                           menu, 
                           NULL);
            free(menu);

            cnv->state = CONV_BUY_ITEM;
        }
        else {
            reply = concat(vendorGetName(cnv->talker), vendorGetText(cnv->talker, WV_BYE), NULL);
            cnv->state = CONV_DONE;
        }
        break;

    case NPC_VENDOR_FOOD:
        if (cont) {
            snprintf(buffer, sizeof(buffer), "%d", foodPrices[vendorGetVendorNo(cnv->talker)]);
            reply = concat(vendorGetText(cnv->talker, FV_PRICE1), buffer,
                           vendorGetText(cnv->talker, FV_PRICE2), 
                           vendorGetText(cnv->talker, FV_HOWMANY),
                           NULL);
            cnv->state = CONV_BUY_QUANTITY;
        }
        else {
            reply = strdup(vendorGetText(cnv->talker, FV_BYE));
            cnv->state = CONV_DONE;
        } 
        break;

    case NPC_VENDOR_TAVERN:
        if (cont) {
            reply = concat(vendorGetName(cnv->talker), vendorGetText(cnv->talker, TV_FOODORALE), NULL);
            cnv->state = CONV_VENDORQUESTION;
        }
        else {
            reply = strdup(vendorGetText(cnv->talker, TV_BYE));
            cnv->state = CONV_DONE;
        } 
        break;

    case NPC_VENDOR_REAGENTS:
        if (cont) {
            reply = concat(vendorGetText(cnv->talker, RV_VERYWELL),
                           vendorGetText(cnv->talker, RV_MENU),
                           NULL);
            cnv->state = CONV_DONE;
        }
        else {
            reply = concat(vendorGetName(cnv->talker),
                           vendorGetText(cnv->talker, RV_BYE),
                           NULL);
            cnv->state = CONV_DONE;
        } 
        break;

    case NPC_VENDOR_HEALER:
        if (cont) {
            reply = concat(vendorGetName(cnv->talker),
                           vendorGetText(cnv->talker, HV_CANPERFORM),
                           NULL);
            cnv->state = CONV_DONE;
        }
        else {
            reply = concat(vendorGetName(cnv->talker),
                           vendorGetText(cnv->talker, HV_BYE),
                           NULL);
            cnv->state = CONV_DONE;
        } 
        break;

    case NPC_VENDOR_INN:
        if (cont) {
            reply = concat(vendorGetText(cnv->talker, IV_WEHAVE + vendorGetVendorNo(cnv->talker)), 
                           vendorGetText(cnv->talker, IV_TAKEIT),
                           NULL);
            cnv->state = CONV_DONE;
        }
        else {
            reply = concat(vendorGetName(cnv->talker),
                           vendorGetText(cnv->talker, IV_WRONGPLACE),
                           NULL);
            cnv->state = CONV_DONE;
        } 
        break;

    default:
        assert(0);              /* shouldn't happen */
    }
    return reply;
}

int armsVendorInfoRead(ArmsVendorInfo *info, int nprices, FILE *f) {
    int i, j;

    for (i = 0; i < N_ARMS_VENDORS; i++) {
        for (j = 0; j < ARMS_VENDOR_INVENTORY_SIZE; j++) {
            if (!readChar(&(info->vendorInventory[i][j]), f))
                return 0;
        }
    }

    for (i = 0; i < nprices; i++) {
        if (!readShort(&(info->prices[i]), f))
            return 0;
    }

    return 1;
}

VendorTypeInfo *vendorLoadTypeInfo(FILE *avatar, const VendorTypeDesc *desc) {
    int i;
    VendorTypeInfo *info = (VendorTypeInfo *) malloc(sizeof(VendorTypeInfo));

    memset(info, 0, sizeof(VendorTypeInfo));

    fseek(avatar, desc->cityMapOffset, SEEK_SET);
    for (i = 0; i < VCM_SIZE; i++) {
        if (!readChar(&(info->cityMap[i]), avatar))
            return NULL;
    }

    info->n_shops = desc->n_shops;
    if (info->n_shops) {
        info->shopNames = u4read_stringtable(avatar, desc->shopNamesOffset, info->n_shops);
        info->shopkeeperNames = u4read_stringtable(avatar, desc->shopkeeperNamesOffset, info->n_shops);
    }

    info->n_items = desc->n_items;
    if (info->n_items) {
        info->itemDescriptions = u4read_stringtable(avatar, desc->itemDescriptionsOffset, info->n_items);
    }

    info->n_text = desc->n_text;
    if (info->n_text) {
        info->text = u4read_stringtable(avatar, desc->textOffset, info->n_text);
    }

    info->n_text2 = desc->n_text2;
    if (info->n_text2) {
        info->text2 = u4read_stringtable(avatar, desc->text2Offset, info->n_text2);
    }

    return info;
}
