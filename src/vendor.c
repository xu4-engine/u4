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

ArmsVendorInfo weaponVendorInfo;
ArmsVendorInfo armorVendorInfo;
unsigned char reagPrices[N_REAG_VENDORS][REAG_MAX];
unsigned short foodPrices[N_FOOD_VENDORS];

#define WV_NOTENOUGH 0
#define WV_FINECHOICE 1

#define WV_VERYGOOD 0
#define WV_WEHAVE 1
#define WV_YOURINTEREST 2
#define WV_HOWMANYTOBUY 4
#define WV_WHATWILL 7
#define WV_YOUSELL 8
#define WV_DONTOWN 9
#define WV_HOWMANY 10
#define WV_TOSELL 11
#define WV_DONTOWNENOUGH 13
#define WV_WELCOME 23
#define WV_SPACER 24
#define WV_BUYORSELL 25
#define WV_BYE 26

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

#define TV_WELCOME 12
#define TV_FOODORALE 13

#define RV_WELCOME 0
#define RV_IAM 1
#define RV_NEEDREAGENTS 2

#define HV_WELCOME 14
#define HV_NEEDHELP 15

VendorTypeInfo *vendorLoadTypeInfo(FILE *avatar, const VendorTypeDesc *desc);
const char *vendorGetName(const Person *v);
const char *vendorGetShop(const Person *v);
char *vendorGetArmsVendorMenu(const Person *v);
int armsVendorInfoRead(ArmsVendorInfo *info, int nprices, FILE *f);

const VendorTypeDesc vendorTypeDesc[] = {
    { 80237, 6, 78883, -1, 14, 79015, 2, 80077, 27, 80282 },    /* weapons */
    { 81511, 5, 80803, -1, 6, 80920, 2, 81374, 27, 81540 },     /* armor */
    { 87519, 5, 87063, -1, 0, 0, 13, 87168, 0, 0 },             /* food */
    { 86363, 6, 85385, -1, 0, 0, 16, 86465, 0, 0 },             /* tavern */
    { 78827, 4, 78295, -1, 0, 0, 16, 78377, 0, 0 },             /* reagents */
    { 84475, 10, 84209, -1, 0, 0, 3, 84439, 20, 84531 },        /* healer */
    { 83703, 7, 83001, -1, 0, 0, 0, 0, 0, 0 }                   /* inn */
};

#define N_VENDOR_TYPE_DESCS (sizeof(vendorTypeDesc) / sizeof(vendorTypeDesc[0]))

VendorTypeInfo **vendorTypeInfo;

/**
 * Loads in conversation data for special cases and vendors from
 * avatar.exe.
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

const char *vendorGetName(const Person *v) {
    int type;
    if (v->npcType < NPC_VENDOR_WEAPONS ||
        v->npcType > NPC_VENDOR_INN)
        assert(0);              /* shouldn't happen */

    type = v->npcType - NPC_VENDOR_WEAPONS;

    return vendorTypeInfo[type]->shopkeeperNames[vendorGetVendorNo(v)];
}

const char *vendorGetShop(const Person *v) {
    int type;
    if (v->npcType < NPC_VENDOR_WEAPONS ||
        v->npcType > NPC_VENDOR_INN)
        assert(0);              /* shouldn't happen */

    type = v->npcType - NPC_VENDOR_WEAPONS;

    return vendorTypeInfo[type]->shopNames[vendorGetVendorNo(v)];
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
    int type;
    char *intro;

    type = cnv->talker->npcType - NPC_VENDOR_WEAPONS;

    switch (cnv->talker->npcType) {
    case NPC_VENDOR_WEAPONS:
    case NPC_VENDOR_ARMOR:
        c->statsItem = cnv->talker->npcType == NPC_VENDOR_WEAPONS ? STATS_WEAPONS : STATS_ARMOR;
        statsUpdate();
        intro = concat(vendorTypeInfo[type]->text2[WV_WELCOME], vendorGetShop(cnv->talker),
                       vendorTypeInfo[type]->text2[WV_SPACER], vendorGetName(cnv->talker),
                       vendorTypeInfo[type]->text2[WV_BUYORSELL],
                       NULL);
        cnv->state = CONV_VENDORQUESTION;
        break;

    case NPC_VENDOR_FOOD:
        intro = concat(vendorTypeInfo[type]->text[FV_WELCOME], vendorGetShop(cnv->talker),
                       vendorTypeInfo[type]->text[FV_SPACER], vendorGetName(cnv->talker),
                       vendorTypeInfo[type]->text[FV_GOODDAY],
                       NULL);
        cnv->state = CONV_VENDORQUESTION;
        break;

    case NPC_VENDOR_TAVERN:
        intro = concat(vendorGetName(cnv->talker), vendorTypeInfo[type]->text[TV_WELCOME], vendorGetShop(cnv->talker), "\n",
                       vendorGetName(cnv->talker), vendorTypeInfo[type]->text[TV_FOODORALE], 
                       NULL);
        cnv->state = CONV_VENDORQUESTION;
        break;

    case NPC_VENDOR_REAGENTS:
        intro = concat(vendorTypeInfo[type]->text[RV_WELCOME], vendorGetShop(cnv->talker), 
                       vendorTypeInfo[type]->text[RV_IAM], vendorGetName(cnv->talker),
                       vendorTypeInfo[type]->text[RV_NEEDREAGENTS],
                       NULL);
        cnv->state = CONV_VENDORQUESTION;
        break;

    case NPC_VENDOR_HEALER:
        intro = concat(vendorTypeInfo[type]->text2[HV_WELCOME], vendorGetShop(cnv->talker), "\n",
                       vendorGetName(cnv->talker), vendorTypeInfo[type]->text2[HV_NEEDHELP],
                       NULL);
        cnv->state = CONV_VENDORQUESTION;
        break;

    case NPC_VENDOR_INN:
        intro = concat("name: ", vendorGetName(cnv->talker), ", shop: ", vendorGetShop(cnv->talker), "\n", NULL);
        cnv->state = CONV_VENDORQUESTION;
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
    int type;
    char *prompt;

    type = cnv->talker->npcType - NPC_VENDOR_WEAPONS;

    switch (cnv->state) {

    case CONV_VENDORQUESTION:
        prompt = strdup("");
        break;

    case CONV_BUY_ITEM:
        prompt = strdup(vendorTypeInfo[type]->text2[WV_YOURINTEREST]);
        break;
        
    case CONV_SELL_ITEM:
        prompt = strdup(vendorTypeInfo[type]->text2[WV_YOUSELL]);
        break;

    case CONV_BUY_QUANTITY:
        switch (cnv->talker->npcType) {
        case NPC_VENDOR_WEAPONS:
        case NPC_VENDOR_ARMOR:
            prompt = strdup(vendorTypeInfo[type]->text2[WV_HOWMANYTOBUY]);
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
    int type;
    
    type = cnv->talker->npcType - NPC_VENDOR_WEAPONS;

    switch (cnv->talker->npcType) {

    case NPC_VENDOR_WEAPONS:
    case NPC_VENDOR_ARMOR:
        if (tolower(response[0]) == 'b') {
            char *menu;
            menu = vendorGetArmsVendorMenu(cnv->talker);
            reply = concat(vendorTypeInfo[type]->text2[WV_VERYGOOD], 
                           vendorTypeInfo[type]->text2[WV_WEHAVE], 
                           menu, 
                           NULL);
            free(menu);

            cnv->state = CONV_BUY_ITEM;
        }    

        else if (tolower(response[0]) == 's') {
            reply = strdup(vendorTypeInfo[type]->text2[WV_WHATWILL]);
            cnv->state = CONV_SELL_ITEM;
        }

        else {
            reply = strdup("");
            cnv->state = CONV_DONE;
        }

        break;

    case NPC_VENDOR_FOOD:
        if (tolower(response[0]) == 'y') {
            sprintf(buffer, "%d", foodPrices[vendorGetVendorNo(cnv->talker)]);
            reply = concat(vendorTypeInfo[type]->text[FV_PRICE1], buffer,
                           vendorTypeInfo[type]->text[FV_PRICE2], 
                           vendorTypeInfo[type]->text[FV_HOWMANY],
                           NULL);
            cnv->state = CONV_BUY_QUANTITY;
        }
        else if (tolower(response[0]) == 'n') {
            reply = strdup(vendorTypeInfo[type]->text[FV_BYE]);
            cnv->state = CONV_DONE;
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
    int i, type;
    const ArmsVendorInfo *info;

    cnv->item = -1;

    type = cnv->talker->npcType - NPC_VENDOR_WEAPONS;

    if (cnv->talker->npcType == NPC_VENDOR_WEAPONS)
        info = &weaponVendorInfo;
    else
        info = &armorVendorInfo;

    if (response[0] == '\033') {
        cnv->state = CONV_DONE;
        return concat(vendorGetName(cnv->talker), vendorTypeInfo[type]->text2[WV_BYE], NULL);
    }

    for (i = 0; i < ARMS_VENDOR_INVENTORY_SIZE; i++) {
        if (info->vendorInventory[vendorGetVendorNo(cnv->talker)][i] != 0 &&
            (tolower(response[0]) - 'a') == info->vendorInventory[vendorGetVendorNo(cnv->talker)][i])
            cnv->item = info->vendorInventory[vendorGetVendorNo(cnv->talker)][i];
    }

    if (cnv->item == -1)
        reply = strdup("");
    else {
        int type = cnv->talker->npcType - NPC_VENDOR_WEAPONS;
        reply = strdup(vendorTypeInfo[type]->itemDescriptions[cnv->item - 1]);
        cnv->state = CONV_BUY_QUANTITY;
    }

    return reply;
}

char *vendorGetSellItemResponse(Conversation *cnv, const char *response) {
    char *reply;
    int type, itemMax;

    type = cnv->talker->npcType - NPC_VENDOR_WEAPONS;

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
        return concat(vendorGetName(cnv->talker), vendorTypeInfo[type]->text2[WV_BYE], NULL);
    }

    if (cnv->item == 0 ||
        (cnv->talker->npcType == NPC_VENDOR_WEAPONS && c->saveGame->weapons[cnv->item] < 1) ||
        (cnv->talker->npcType == NPC_VENDOR_ARMOR && c->saveGame->armor[cnv->item] < 1)) {
        reply = strdup(vendorTypeInfo[type]->text2[WV_DONTOWN]);
        return reply;
    }

    switch (cnv->talker->npcType) {
    case NPC_VENDOR_WEAPONS:
        reply = concat(vendorTypeInfo[type]->text2[WV_HOWMANY], getWeaponName((WeaponType) cnv->item), vendorTypeInfo[type]->text2[WV_TOSELL], NULL);
        break;
    case NPC_VENDOR_ARMOR:
        reply = concat(vendorTypeInfo[type]->text2[WV_HOWMANY], getArmorName((ArmorType) cnv->item), vendorTypeInfo[type]->text2[WV_TOSELL], NULL);
        break;
    default:
        assert(0);              /* shouldn't happen */
    }
    cnv->state = CONV_SELL_QUANTITY;

    return reply;
}

char *vendorGetBuyQuantityResponse(Conversation *cnv, const char *response) {
    char *reply;
    int type, totalprice, success;
    InventoryItem item;
    
    type = cnv->talker->npcType - NPC_VENDOR_WEAPONS;

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
    default:
        assert(0);              /* shouldn't happen */
    }

    success = playerPurchase(c->saveGame, item, cnv->item, cnv->quant, totalprice);

    switch (cnv->talker->npcType) {

    case NPC_VENDOR_WEAPONS:
        if (success)
            reply = concat(vendorGetName(cnv->talker), vendorTypeInfo[type]->text[WV_FINECHOICE], NULL);
        else
            reply = strdup(vendorTypeInfo[type]->text[WV_NOTENOUGH]);
        cnv->state = CONV_DONE;
        break;

    case NPC_VENDOR_ARMOR:
        if (success)
            reply = concat(vendorGetName(cnv->talker), vendorTypeInfo[type]->text[WV_FINECHOICE], NULL);
        else
            reply = strdup(vendorTypeInfo[type]->text[WV_NOTENOUGH]);
        cnv->state = CONV_DONE;
        break;
        
    case NPC_VENDOR_FOOD:
        if (success) {
            reply = concat(vendorTypeInfo[type]->text[FV_ANYMORE], NULL);
            cnv->state = CONV_VENDORQUESTION;
        } else {
            char buffer[10];

            if (c->saveGame->gold / foodPrices[vendorGetVendorNo(cnv->talker)] == 0) {
                reply = concat(vendorTypeInfo[type]->text[FV_CANTAFFORD], 
                               vendorTypeInfo[type]->text[FV_BYE], 
                               NULL);
                cnv->state = CONV_DONE;
            } else {
                sprintf(buffer, "%d", foodPrices[vendorGetVendorNo(cnv->talker)]);
                reply = concat(vendorTypeInfo[type]->text[FV_AFFORDONLY1], 
                               buffer,
                               vendorTypeInfo[type]->text[FV_AFFORDONLY2],
                               vendorTypeInfo[type]->text[FV_HOWMANY],
                               NULL);
                cnv->state = CONV_BUY_QUANTITY;
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
    int type, totalprice, success;
    const ArmsVendorInfo *info;

    type = cnv->talker->npcType - NPC_VENDOR_WEAPONS;

    if (cnv->talker->npcType == NPC_VENDOR_WEAPONS)
        info = &weaponVendorInfo;
    else
        info = &armorVendorInfo;

    cnv->quant = (int) strtol(response, NULL, 10);

    totalprice = cnv->quant * info->prices[cnv->item] / 2;
    if ((cnv->talker->npcType == NPC_VENDOR_WEAPONS && cnv->quant <= c->saveGame->weapons[cnv->item]) ||
        (cnv->talker->npcType == NPC_VENDOR_ARMOR && cnv->quant <= c->saveGame->armor[cnv->item])) {
        success = 1;
        c->saveGame->gold += totalprice;
    } else
        success = 0;

    switch (cnv->talker->npcType) {

    case NPC_VENDOR_WEAPONS:
        if (success) {
            reply = concat(vendorGetName(cnv->talker), vendorTypeInfo[type]->text[WV_FINECHOICE], NULL);
            c->saveGame->weapons[cnv->item] -= cnv->quant;
            statsUpdate();
        } else
            reply = concat(vendorTypeInfo[type]->text2[WV_DONTOWNENOUGH]);
            
        cnv->state = CONV_DONE;
        break;

    case NPC_VENDOR_ARMOR:
        if (success) {
            reply = concat(vendorGetName(cnv->talker), vendorTypeInfo[type]->text[WV_FINECHOICE], NULL);
            c->saveGame->armor[cnv->item] -= cnv->quant;
            statsUpdate();
        } else
            reply = strdup(vendorTypeInfo[type]->text2[WV_DONTOWNENOUGH]);

        cnv->state = CONV_DONE;
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
