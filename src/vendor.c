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
#include "vendor.h"
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
#define N_REAG_VENDORS 4
#define N_FOOD_VENDORS 5
#define N_TAVERN_VENDORS 6

#define ARMS_VENDOR_INVENTORY_SIZE 4
#define N_TAVERN_TOPICS 6 
#define N_GUILD_ITEMS 4

#define VCM_SIZE 16

const struct {
    char *(*getIntro)(struct _Conversation *cnv);
    char *(*getVendorQuestionResponse)(struct _Conversation *cnv, const char *inquiry);
    char *(*getBuyItemResponse)(struct _Conversation *cnv, const char *inquiry);
    char *(*getSellItemResponse)(struct _Conversation *cnv, const char *inquiry);
    char *(*getBuyQuantityResponse)(struct _Conversation *cnv, const char *inquiry);
    char *(*getSellQuantityResponse)(struct _Conversation *cnv, const char *inquiry);
    char *(*getBuyPriceResponse)(struct _Conversation *cnv, const char *inquiry);
    char *(*getContinueQuestionResponse)(struct _Conversation *cnv, const char *answer);
    char *(*getTopicResponse)(struct _Conversation *cnv, const char *inquiry);
    const char *vendorQuestionChoices;
} vendorType[] = {
    { &vendorGetIntro, &vendorGetArmsVendorQuestionResponse, &vendorGetArmsBuyItemResponse, &vendorGetSellItemResponse, 
      &vendorGetBuyQuantityResponse, &vendorGetSellQuantityResponse, NULL, 
      &vendorGetContinueQuestionResponse, NULL, "bs\033" }, /* NPC_VENDOR_WEAPONS */
    { &vendorGetIntro, &vendorGetArmsVendorQuestionResponse, &vendorGetArmsBuyItemResponse, &vendorGetSellItemResponse, 
      &vendorGetBuyQuantityResponse, &vendorGetSellQuantityResponse, NULL,
      &vendorGetContinueQuestionResponse, NULL, "bs\033" }, /* NPC_VENDOR_ARMOR */
    { &vendorGetIntro, NULL, NULL, NULL, 
      &vendorGetBuyQuantityResponse, NULL, NULL, 
      &vendorGetContinueQuestionResponse, NULL, NULL }, /* NPC_VENDOR_FOOD */
    { &vendorGetIntro, &vendorGetTavernVendorQuestionResponse, NULL, NULL, 
      &vendorGetBuyQuantityResponse, NULL, &vendorGetTavernBuyPriceResponse,
      &vendorGetContinueQuestionResponse, &vendorGetTavernTopicResponse, "af\033" }, /* NPC_VENDOR_TAVERN */
    { &vendorGetIntro, NULL, &vendorGetReagentsBuyItemResponse, NULL,
      &vendorGetBuyQuantityResponse, NULL, &vendorGetReagentsBuyPriceResponse,
      &vendorGetContinueQuestionResponse, NULL, NULL }, /* NPC_VENDOR_REAGENTS */
    { &vendorGetIntro, &vendorGetHealerVendorQuestionResponse, &vendorGetHealerBuyItemResponse, NULL,
      NULL, NULL, NULL, 
      &vendorGetContinueQuestionResponse, NULL, "ny\033" }, /* NPC_VENDOR_HEALER */
    { &vendorGetIntro, NULL, NULL, NULL,
      NULL, NULL, NULL,
      &vendorGetContinueQuestionResponse, NULL, NULL }, /* NPC_VENDOR_INN */
    { &vendorGetIntro, &vendorGetGuildVendorQuestionResponse, &vendorGetGuildBuyItemResponse, NULL,
      NULL, NULL, NULL, 
      &vendorGetContinueQuestionResponse, NULL, "ny\033" }, /* NPC_VENDOR_GUILD */
    { &vendorGetIntro, NULL, NULL, NULL,
      NULL, NULL, NULL, 
      &vendorGetContinueQuestionResponse, NULL, NULL }, /* NPC_VENDOR_STABLE */
};

/*
 * This structure hold information about each vendor type.  The
 * offsets are byte offsets into the AVATAR.EXE file where the vendor
 * dialog strings and other data are stored.
 */
typedef struct VendorTypeDesc {
    InventoryItem item;
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

const VendorTypeDesc vendorTypeDesc[] = {
    { INV_WEAPON,  80237, 6,  78883, -1, 14, 79015, 2,  80077, 27, 80282 },  /* weapons */
    { INV_ARMOR,   81511, 5,  80803, -1, 6,  80920, 2,  81374, 27, 81540 },  /* armor */
    { INV_FOOD,    87519, 5,  87063, -1, 0,  0,     13, 87168, 0,  0 },      /* food */
    { INV_FOOD,    86363, 6,  85385, -1, 0,  0,     6,  86235, 16, 86465 },  /* tavern */
    { INV_REAGENT, 78827, 4,  78295, -1, 0,  0,     16, 78377, 0,  0 },      /* reagents */
    { INV_NONE,    84475, 10, 84209, -1, 0,  0,     3,  84439, 20, 84531 },  /* healer */
    { INV_NONE,    83703, 7,  83001, -1, 0,  0,     8,  83158, 12, 83785 },  /* inn */
    { INV_GUILDITEM, -1,  2,  82341, -1, 4,  82403, 12, 82641, 0,  0 },      /* guild */
    { INV_HORSE,   -1,    0,  0, 0,      0,  0,     7,  82023, 0,  0 }       /* stables */
};

#define N_VENDOR_TYPE_DESCS (sizeof(vendorTypeDesc) / sizeof(vendorTypeDesc[0]))

/*
 * This structure hold the information loading in from AVATAR.EXE for
 * each vendor type.  Each VendorTypeInfo is loaded from a
 * VendorTypeDesc.
 */
typedef struct _VendorTypeInfo {
    InventoryItem item;
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

ArmsVendorInfo weaponVendorInfo;
ArmsVendorInfo armorVendorInfo;
unsigned char reagPrices[N_REAG_VENDORS][REAG_MAX];
unsigned short foodPrices[N_FOOD_VENDORS];
char **tavernSpecialties;
char **tavernTopics;
char **tavernInfo;
unsigned short tavernInfoPrices[N_TAVERN_TOPICS];
unsigned short tavernFoodPrices[N_TAVERN_VENDORS];
unsigned short guildItemPrices[N_GUILD_ITEMS];
unsigned short guildItemQuantities[N_GUILD_ITEMS];

VendorTypeInfo **vendorTypeInfo;

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
#define TV_WONTPAY 102
#define TV_NOTENOUGH 103
#define TV_WHATINFO 104
#define TV_DONTKNOW 105
#define TV_MOREGOLD 106
#define TV_SAYS 110
#define TV_ANYTHINGELSE 111
#define TV_WELCOME 112
#define TV_FOODORALE 113
#define TV_HEREYOUARE 114
#define TV_BYE 115

#define RV_WELCOME 0
#define RV_IAM 1
#define RV_NEEDREAGENTS 2
#define RV_VERYWELL 3
#define RV_MENU 4
#define RV_YOURINTEREST 5
#define RV_WESELL 6
#define RV_FOR 7
#define RV_HOWMANY 8
#define RV_THATWILLBE 9
#define RV_YOUPAY 10
#define RV_CANTAFFORD 11
#define RV_VERYGOOD 12
#define RV_ISEE 13
#define RV_ANYTHINGELSE 14
#define RV_BYE 15

#define HV_WILLPAY 100
#define HV_CANNOTAID 101
#define HV_CURINGCOSTS 103
#define HV_HEALINGCOSTS 107
#define HV_REZCOSTS 110
#define HV_WELCOME 114
#define HV_NEEDHELP 115
#define HV_CANPERFORM 116
#define HV_YOURNEED 117
#define HV_MOREHELP 118
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

#define GV_WELCOMETO 2
#define GV_SEEGOODS 3
#define GV_IGOT 4
#define GV_WHATLLITBE 5
#define GV_WILLYABUY 6
#define GV_GRMBL 7
#define GV_BUZZOFF 8
#define GV_FINEFINE 9
#define GV_SEEMORE 10
#define GV_SEEYA 11 

#define SV_NEEDHORSE 0
#define SV_ASHAME 1
#define SV_FORONLY 2
#define SV_WILLBUY 3

VendorTypeInfo *vendorLoadTypeInfo(FILE *avatar, const VendorTypeDesc *desc);
const char *vendorGetName(const Person *v);
const char *vendorGetShop(const Person *v);
const char *vendorGetText(const Person *v, int textId);
const VendorTypeInfo *vendorGetInfo(const Person *v);
char *vendorGetArmsVendorMenu(const Person *v);
char *vendorDoBuyTransaction(Conversation *cnv);
char *vendorDoSellTransaction(Conversation *cnv);
int armsVendorInfoRead(ArmsVendorInfo *info, int nprices, FILE *f);

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

    tavernSpecialties = u4read_stringtable(avatar, 85521, N_TAVERN_VENDORS);
    tavernTopics = u4read_stringtable(avatar, 85599, N_TAVERN_TOPICS);
    tavernInfo = u4read_stringtable(avatar, 85671, N_TAVERN_TOPICS);

    fseek(avatar, 86379, SEEK_SET);
    for (i = 0; i < N_TAVERN_TOPICS; i++) {
        if (!readShort(&(tavernInfoPrices[i]), avatar))
            return 0;
    }

    fseek(avatar, 86427, SEEK_SET);
    for (i = 0; i < N_TAVERN_VENDORS; i++) {
        if (!readShort(&(tavernFoodPrices[i]), avatar))
            return 0;
    }

    fseek(avatar, 82969, SEEK_SET);
    for (i = 0; i < N_GUILD_ITEMS; i++) {
        if (!readShort(&(guildItemPrices[i]), avatar))
            return 0;
    }

    fseek(avatar, 82977, SEEK_SET);
    for (i = 0; i < N_GUILD_ITEMS; i++) {
        if (!readShort(&(guildItemQuantities[i]), avatar))
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

void vendorGetConversationText(Conversation *cnv, const char *inquiry, char **response) {
    switch (cnv->state) {
    case CONV_INTRO:
        *response = (*vendorType[cnv->talker->npcType - NPC_VENDOR_WEAPONS].getIntro)(cnv);
        return;
    case CONV_VENDORQUESTION:
        *response = (*vendorType[cnv->talker->npcType - NPC_VENDOR_WEAPONS].getVendorQuestionResponse)(cnv, inquiry);
        return;
    case CONV_BUY_ITEM:
        *response = (*vendorType[cnv->talker->npcType - NPC_VENDOR_WEAPONS].getBuyItemResponse)(cnv, inquiry);
        return;
    case CONV_SELL_ITEM:
        *response = (*vendorType[cnv->talker->npcType - NPC_VENDOR_WEAPONS].getSellItemResponse)(cnv, inquiry);
        return;
    case CONV_BUY_QUANTITY:
        *response = (*vendorType[cnv->talker->npcType - NPC_VENDOR_WEAPONS].getBuyQuantityResponse)(cnv, inquiry);
        return;
    case CONV_SELL_QUANTITY:
        *response = (*vendorType[cnv->talker->npcType - NPC_VENDOR_WEAPONS].getSellQuantityResponse)(cnv, inquiry);
        return;
    case CONV_BUY_PRICE:
        *response = (*vendorType[cnv->talker->npcType - NPC_VENDOR_WEAPONS].getBuyPriceResponse)(cnv, inquiry);
        return;
    case CONV_CONTINUEQUESTION:
        *response = (*vendorType[cnv->talker->npcType - NPC_VENDOR_WEAPONS].getContinueQuestionResponse)(cnv, inquiry);
        return;
    case CONV_TOPIC:
        *response = (*vendorType[cnv->talker->npcType - NPC_VENDOR_WEAPONS].getTopicResponse)(cnv, inquiry);
        return;
    default:
        assert(0);          /* shouldn't happen */

    }
}

char *vendorGetPrompt(const Conversation *cnv) {
    char *prompt;

    switch (cnv->state) {

    case CONV_VENDORQUESTION:
    case CONV_SELL_QUANTITY:
    case CONV_BUY_PRICE:
    case CONV_CONTINUEQUESTION:
    case CONV_TOPIC:
        prompt = strdup("");
        break;

    case CONV_BUY_ITEM:
        switch (cnv->talker->npcType) {
        case NPC_VENDOR_WEAPONS:
        case NPC_VENDOR_ARMOR:
            prompt = strdup(vendorGetText(cnv->talker, WV_YOURINTEREST));
            break;
        default:
            prompt = strdup("");
            break;
        }
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

    default:
        assert(0);
    }

    return prompt;
}

const char *vendorGetVendorQuestionChoices(const Conversation *cnv) {
    return vendorType[cnv->talker->npcType - NPC_VENDOR_WEAPONS].vendorQuestionChoices;
}

/**
 * Gets the vendor number for the given vendor.  Vendor number is
 * derived from the vendor type and the city; it is looked up in a
 * vendor city map that gives the vendor number for each city.  Guild
 * and stables don't have a vendor city map, so they are handled as
 * special cases.
 */
int vendorGetVendorNo(const Person *v) {
    int type;

    assert(v->npcType >= NPC_VENDOR_WEAPONS && v->npcType <= NPC_VENDOR_STABLE);

    if (v->npcType == NPC_VENDOR_GUILD) {
        if (c->map->id == 14)   /* Buccaneers Den */
            return 0;
        if (c->map->id == 15)   /* Vesper */
            return 1;
        assert(0);              /* shouldn't happen */
    }

    if (v->npcType == NPC_VENDOR_STABLE) {
        if (c->map->id == 13)   /* Paws */
            return 0;
        assert(0);              /* shouldn't happen */
    }


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
    assert(v->npcType >= NPC_VENDOR_WEAPONS && v->npcType <= NPC_VENDOR_STABLE);

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

    cnv->itemType = vendorGetInfo(cnv->talker)->item;

    switch (cnv->talker->npcType) {
    case NPC_VENDOR_WEAPONS:
    case NPC_VENDOR_ARMOR:
        c->statsItem = cnv->talker->npcType == NPC_VENDOR_WEAPONS ? STATS_WEAPONS : STATS_ARMOR;
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
        c->statsItem = STATS_EQUIPMENT;
        intro = concat(vendorGetName(cnv->talker),
                       vendorGetText(cnv->talker, GV_WELCOMETO),
                       vendorGetShop(cnv->talker),
                       vendorGetText(cnv->talker, GV_SEEGOODS),
                       NULL);
        cnv->state = CONV_CONTINUEQUESTION;
        break;

    case NPC_VENDOR_STABLE:
        intro = strdup(vendorGetText(cnv->talker, SV_NEEDHORSE));
        cnv->state = CONV_CONTINUEQUESTION;
        break;

    default:
        assert(0);          /* shouldn't happen */
    }

    statsUpdate();
    return intro;
}

char *vendorGetArmsVendorQuestionResponse(Conversation *cnv, const char *response) {
    char *reply;
    
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

    return reply;
}

char *vendorGetTavernVendorQuestionResponse(Conversation *cnv, const char *response) {
    char buffer[10];
    char *reply;
    
    if (tolower(response[0]) == 'f') {
        snprintf(buffer, sizeof(buffer), "%d", tavernFoodPrices[vendorGetVendorNo(cnv->talker)]);
        reply = concat(vendorGetText(cnv->talker, TV_SPECIALTY), tavernSpecialties[vendorGetVendorNo(cnv->talker)],
                       vendorGetText(cnv->talker, TV_COSTS), buffer,
                       vendorGetText(cnv->talker, TV_HOWMANY),
                       NULL);
        cnv->state = CONV_BUY_QUANTITY;
    }
    else if (tolower(response[0]) == 'a') {
        reply = strdup(vendorGetText(cnv->talker, TV_BESTMUG));
        cnv->state = CONV_BUY_PRICE;
    } 
    else {
        reply = strdup("");
        cnv->state = CONV_DONE;
    }

    return reply;
}

char *vendorGetHealerVendorQuestionResponse(Conversation *cnv, const char *response) {
    char *reply;
    
    if (tolower(response[0]) == 'y') {
        playerHeal(c->saveGame, cnv->itemSubtype, 0); /* FIXME: get real player number */
        statsUpdate();
        reply = concat(vendorGetName(cnv->talker), 
                       vendorGetText(cnv->talker, HV_MOREHELP), 
                       NULL);
        cnv->state = CONV_CONTINUEQUESTION;
    }
    else if (tolower(response[0]) == 'n') {
        reply = concat(vendorGetText(cnv->talker, HV_CANNOTAID), 
                       vendorGetName(cnv->talker), 
                       vendorGetText(cnv->talker, HV_MOREHELP), 
                       NULL);
        cnv->state = CONV_CONTINUEQUESTION;
    } 
    else {
        reply = strdup("");
        cnv->state = CONV_DONE;
    }

    return reply;
}

char *vendorGetGuildVendorQuestionResponse(Conversation *cnv, const char *response) {
    char *reply;
    
    if (tolower(response[0]) == 'y') {
        return vendorDoBuyTransaction(cnv);
    }
    else if (tolower(response[0]) == 'n') {
        reply = concat(vendorGetText(cnv->talker, GV_GRMBL), "\n",
                       vendorGetName(cnv->talker), 
                       vendorGetText(cnv->talker, GV_SEEYA), 
                       NULL);
        cnv->state = CONV_DONE;
    } 
    else {
        reply = strdup("");
        cnv->state = CONV_DONE;
    }

    return reply;
}

char *vendorGetArmsBuyItemResponse(Conversation *cnv, const char *response) {
    char *reply;
    int i;
    const ArmsVendorInfo *info = NULL;

    cnv->itemSubtype = -1;

    if (cnv->talker->npcType == NPC_VENDOR_WEAPONS)
        info = &weaponVendorInfo;
    else if (cnv->talker->npcType == NPC_VENDOR_ARMOR)
        info = &armorVendorInfo;
    else
        assert(0);              /* shouldn't happen */

    if (response[0] == '\033') {
        cnv->state = CONV_DONE;
        return concat(vendorGetName(cnv->talker), vendorGetText(cnv->talker, WV_BYE), NULL);
    }

    for (i = 0; i < ARMS_VENDOR_INVENTORY_SIZE; i++) {
        if (info->vendorInventory[vendorGetVendorNo(cnv->talker)][i] != 0 &&
            (tolower(response[0]) - 'a') == info->vendorInventory[vendorGetVendorNo(cnv->talker)][i])
            cnv->itemSubtype = info->vendorInventory[vendorGetVendorNo(cnv->talker)][i];
    }

    if (cnv->itemSubtype == -1) {
        reply = strdup("");
        cnv->state = CONV_DONE;
    }
    else {
        reply = strdup(vendorGetInfo(cnv->talker)->itemDescriptions[cnv->itemSubtype - 1]);
        cnv->state = CONV_BUY_QUANTITY;
    }

    return reply;
}

char *vendorGetReagentsBuyItemResponse(Conversation *cnv, const char *response) {
    char *reply;

    cnv->itemSubtype = -1;

    if (response[0] == '\033') {
        cnv->state = CONV_DONE;
        return concat(vendorGetName(cnv->talker), vendorGetText(cnv->talker, RV_BYE), NULL);
    }

    cnv->itemSubtype = tolower(response[0]) - 'a';
    if (cnv->itemSubtype < 0 || cnv->itemSubtype >= REAG_MAX || 
        reagPrices[vendorGetVendorNo(cnv->talker)][cnv->itemSubtype] == 0) {
        reply = strdup("");
        cnv->state = CONV_DONE;
    }
    else {
        char buffer[10];

        snprintf(buffer, sizeof(buffer), "%d", reagPrices[vendorGetVendorNo(cnv->talker)][cnv->itemSubtype]);
        reply = concat(vendorGetText(cnv->talker, RV_WESELL), 
                       getReagentName(cnv->itemSubtype),
                       vendorGetText(cnv->talker, RV_FOR), 
                       buffer,
                       vendorGetText(cnv->talker, RV_HOWMANY), 
                       NULL);
        cnv->state = CONV_BUY_QUANTITY;
    }

    return reply;
}

char *vendorGetHealerBuyItemResponse(Conversation *cnv, const char *response) {
    char *reply;

    cnv->itemSubtype = -1;

    if (response[0] == '\033') {
        cnv->state = CONV_DONE;
        return concat(vendorGetName(cnv->talker), vendorGetText(cnv->talker, HV_BYE), NULL);
    }

    cnv->itemSubtype = tolower(response[0]) - 'a' + HT_CURE;
    if (cnv->itemSubtype < HT_CURE || cnv->itemSubtype > HT_RESURRECT) {
        reply = strdup("");
        cnv->state = CONV_DONE;
    }
    else {
        int msg;
        if (cnv->itemSubtype == HT_CURE) {
            msg = HV_CURINGCOSTS;
            cnv->price = 100;
        }
        else if (cnv->itemSubtype == HT_HEAL) {
            msg = HV_HEALINGCOSTS;
            cnv->price = 200;
        }
        else if (cnv->itemSubtype == HT_RESURRECT) {
            msg = HV_REZCOSTS;
            cnv->price = 300;
        }
        reply = concat(vendorGetText(cnv->talker, msg), 
                       vendorGetText(cnv->talker, HV_WILLPAY), 
                       NULL);
        cnv->state = CONV_VENDORQUESTION;
    }

    return reply;
}

char *vendorGetGuildBuyItemResponse(Conversation *cnv, const char *response) {
    char *reply;

    cnv->itemSubtype = -1;

    if (response[0] == '\033') {
        cnv->state = CONV_DONE;
        return concat(vendorGetName(cnv->talker), vendorGetText(cnv->talker, GV_SEEYA), NULL);
    }

    cnv->itemSubtype = tolower(response[0]) - 'a';
    if (cnv->itemSubtype < 0 || cnv->itemSubtype >= 4) {
        reply = strdup("");
        cnv->state = CONV_DONE;
    }
    else {
        cnv->quant = guildItemQuantities[cnv->itemSubtype];
        cnv->price = guildItemPrices[cnv->itemSubtype];
        reply = concat(vendorGetInfo(cnv->talker)->itemDescriptions[cnv->itemSubtype],
                       vendorGetText(cnv->talker, GV_WILLYABUY),
                       NULL);
        cnv->state = CONV_VENDORQUESTION;
    }

    return reply;
}

char *vendorGetSellItemResponse(Conversation *cnv, const char *response) {
    char *reply;
    int itemMax;

    cnv->itemSubtype = tolower(response[0]) - 'a';

    if (cnv->talker->npcType == NPC_VENDOR_WEAPONS)
        itemMax = WEAP_MAX;
    else
        itemMax = ARMR_MAX;

    if (cnv->itemSubtype >= itemMax) {
        return strdup("");
    } 
    else if (response[0] == '\033') {
        cnv->state = CONV_DONE;
        return concat(vendorGetName(cnv->talker), vendorGetText(cnv->talker, WV_BYE), NULL);
    }

    if (cnv->itemSubtype == 0 ||
        (cnv->talker->npcType == NPC_VENDOR_WEAPONS && c->saveGame->weapons[cnv->itemSubtype] < 1) ||
        (cnv->talker->npcType == NPC_VENDOR_ARMOR && c->saveGame->armor[cnv->itemSubtype] < 1)) {
        reply = strdup(vendorGetText(cnv->talker, WV_DONTOWN));
        return reply;
    }

    switch (cnv->talker->npcType) {
    case NPC_VENDOR_WEAPONS:
        reply = concat(vendorGetText(cnv->talker, WV_HOWMANY),
                       getWeaponName((WeaponType) cnv->itemSubtype),
                       vendorGetText(cnv->talker, WV_TOSELL),
                       NULL);
        break;
    case NPC_VENDOR_ARMOR:
        reply = concat(vendorGetText(cnv->talker, WV_HOWMANY),
                       getArmorName((ArmorType) cnv->itemSubtype), 
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
    char buffer[10];

    cnv->quant = (int) strtol(response, NULL, 10);

    switch (cnv->talker->npcType) {
    case NPC_VENDOR_WEAPONS:
        cnv->price = weaponVendorInfo.prices[cnv->itemSubtype] * cnv->quant;
        return vendorDoBuyTransaction(cnv);

    case NPC_VENDOR_ARMOR:
        cnv->price = armorVendorInfo.prices[cnv->itemSubtype] * cnv->quant;
        return vendorDoBuyTransaction(cnv);

    case NPC_VENDOR_FOOD:
        cnv->price = foodPrices[vendorGetVendorNo(cnv->talker)] * cnv->quant;
        cnv->quant *= 25;
        cnv->itemSubtype = 0;
        return vendorDoBuyTransaction(cnv);

    case NPC_VENDOR_TAVERN:
        cnv->price = tavernFoodPrices[vendorGetVendorNo(cnv->talker)] * cnv->quant;
        cnv->itemSubtype = 0;
        return vendorDoBuyTransaction(cnv);

    case NPC_VENDOR_REAGENTS:
        cnv->state = CONV_BUY_PRICE;
        snprintf(buffer, sizeof(buffer), "%d", reagPrices[vendorGetVendorNo(cnv->talker)][cnv->itemSubtype] * cnv->quant);
        return concat(vendorGetText(cnv->talker, RV_THATWILLBE),
                      buffer,
                      vendorGetText(cnv->talker, RV_YOUPAY), 
                      NULL);

    default:
        assert(0);              /* shouldn't happen */
    }
}

char *vendorGetSellQuantityResponse(Conversation *cnv, const char *response) {
    assert (cnv->talker->npcType == NPC_VENDOR_WEAPONS ||
            cnv->talker->npcType == NPC_VENDOR_ARMOR);

    cnv->quant = (int) strtol(response, NULL, 10);

    if (cnv->talker->npcType == NPC_VENDOR_WEAPONS)
        cnv->price = weaponVendorInfo.prices[cnv->itemSubtype] * cnv->quant / 2;

    else /* cnv->talker->npcType == NPC_VENDOR_ARMOR */
        cnv->price = armorVendorInfo.prices[cnv->itemSubtype] * cnv->quant / 2;

    return vendorDoSellTransaction(cnv);
}

char *vendorGetTavernBuyPriceResponse(Conversation *cnv, const char *response) {
    char *reply;
    
    cnv->price = (int) strtol(response, NULL, 10);

    if (cnv->price > c->saveGame->gold) {
        reply = concat(vendorGetText(cnv->talker, TV_NOTENOUGH),
                       vendorGetText(cnv->talker, TV_BYE),
                       NULL);
        cnv->state = CONV_DONE;
    }

    if (cnv->price < 2) {
        reply = concat(vendorGetText(cnv->talker, TV_WONTPAY),
                       vendorGetText(cnv->talker, TV_BYE),
                       NULL);
        cnv->state = CONV_DONE;
    } else if (cnv->price == 2) {
        reply = strdup(vendorGetText(cnv->talker, TV_ANYTHINGELSE));
        cnv->state = CONV_CONTINUEQUESTION;
        c->saveGame->gold -= cnv->price;
        statsUpdate();
    } else {
        reply = strdup(vendorGetText(cnv->talker, TV_WHATINFO));
        cnv->state = CONV_TOPIC;
        c->saveGame->gold -= cnv->price;
        statsUpdate();
    }

    return reply;

}

char *vendorGetReagentsBuyPriceResponse(Conversation *cnv, const char *response) {
    cnv->price = (int) strtol(response, NULL, 10);

    return vendorDoBuyTransaction(cnv);
}

char *vendorDoBuyTransaction(Conversation *cnv) {
    char *reply;
    int success;

    success = playerPurchase(c->saveGame, cnv->itemType, cnv->itemSubtype, cnv->quant, cnv->price);

    /*
     * usual case is to ask if player wants to do another transaction
     * after a purchase
     */
    cnv->state = CONV_CONTINUEQUESTION;

    switch (cnv->talker->npcType) {

    case NPC_VENDOR_WEAPONS:
    case NPC_VENDOR_ARMOR:
        if (success)
            reply = concat(vendorGetName(cnv->talker), vendorGetText(cnv->talker, WV_FINECHOICE), 
                           vendorGetText(cnv->talker, WV_ANYTHINGELSE), NULL);
        else
            reply = strdup(vendorGetText(cnv->talker, WV_NOTENOUGH));
        break;

    case NPC_VENDOR_FOOD:
        if (success)
            reply = strdup(vendorGetText(cnv->talker, FV_ANYMORE));

        else {
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
        if (success)
            reply = strdup(vendorGetText(cnv->talker, TV_HEREYOUARE));

        else {
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

    case NPC_VENDOR_REAGENTS:
        if (success) {
            if (cnv->price >= (reagPrices[vendorGetVendorNo(cnv->talker)][cnv->itemSubtype] * cnv->quant)) {
                reply = concat(vendorGetText(cnv->talker, RV_VERYGOOD),
                               vendorGetText(cnv->talker, RV_ANYTHINGELSE),
                               NULL);
                playerAdjustKarma(c->saveGame, KA_DIDNT_CHEAT_REAGENTS);                
            }
            else {
                reply = concat(vendorGetText(cnv->talker, RV_ISEE),
                               vendorGetText(cnv->talker, RV_ANYTHINGELSE),
                               NULL);
                playerAdjustKarma(c->saveGame, KA_CHEAT_REAGENTS);                
            }
                
        }
        else
            reply = concat(vendorGetText(cnv->talker, RV_CANTAFFORD),
                           vendorGetText(cnv->talker, RV_ANYTHINGELSE),
                           NULL);
        break;

    case NPC_VENDOR_GUILD:
        if (success) {
            reply = concat(vendorGetText(cnv->talker, GV_FINEFINE),
                           vendorGetName(cnv->talker),
                           vendorGetText(cnv->talker, GV_SEEMORE),
                           NULL);
            cnv->state = CONV_CONTINUEQUESTION;
        }
        else {
            reply = strdup(vendorGetText(cnv->talker, GV_BUZZOFF));
            cnv->state = CONV_DONE;
        }
        break;

    default:
        assert(0);              /* shouldn't happen */
    }

    statsUpdate();
    return reply;
}

char *vendorDoSellTransaction(Conversation *cnv) {
    char *reply;
    int success;

    assert (cnv->talker->npcType == NPC_VENDOR_WEAPONS ||
            cnv->talker->npcType == NPC_VENDOR_ARMOR);

    success = playerSell(c->saveGame, cnv->itemType, cnv->itemSubtype, cnv->quant, cnv->price);

    if (success) {
        reply = concat(vendorGetName(cnv->talker), vendorGetText(cnv->talker, WV_FINECHOICE), 
                       vendorGetText(cnv->talker, WV_ANYTHINGELSE), NULL);
        cnv->state = CONV_CONTINUEQUESTION;
    }
    else {
        reply = concat(vendorGetText(cnv->talker, WV_DONTOWNENOUGH), 
                       vendorGetText(cnv->talker, WV_BYE), NULL);
        cnv->state = CONV_DONE;
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
                           vendorGetText(cnv->talker, RV_YOURINTEREST),
                           NULL);
            cnv->state = CONV_BUY_ITEM;
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
                           vendorGetText(cnv->talker, HV_YOURNEED),
                           NULL);
            cnv->state = CONV_BUY_ITEM;
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

    case NPC_VENDOR_GUILD:
        if (cont) {
            reply = concat(vendorGetName(cnv->talker),
                           vendorGetText(cnv->talker, GV_IGOT), 
                           vendorGetText(cnv->talker, GV_WHATLLITBE),
                           NULL);
            cnv->state = CONV_BUY_ITEM;
        } else {
            reply = concat(vendorGetName(cnv->talker),
                           vendorGetText(cnv->talker, GV_SEEYA), 
                           NULL);
            cnv->state = CONV_DONE;
        }
        break;

    case NPC_VENDOR_STABLE:
        if (cont) {
            reply = concat(vendorGetText(cnv->talker, SV_FORONLY), 
                           "1", /* FIXME */
                           vendorGetText(cnv->talker, SV_WILLBUY),
                           NULL);
            cnv->state = CONV_DONE;
        } else {
            reply = strdup(vendorGetText(cnv->talker, SV_ASHAME));
            cnv->state = CONV_DONE;
        }
        break;

    default:
        assert(0);              /* shouldn't happen */
    }
    return reply;
}

char *vendorGetTavernTopicResponse(Conversation *cnv, const char *response) {
    char *reply = NULL;
    int i;

    /*
     * Note: I initially thought that each vendor would respond to
     * only one topic; but it turns out that any tavern keeper will
     * provide information on any of the available topics.  It just
     * happens there are six vendors and six topics.
     */

    /* FIXME: check price */
    for (i = 0; i < N_TAVERN_TOPICS; i++) {
        if (strcasecmp(response, tavernTopics[i]) == 0)
            reply = concat(vendorGetName(cnv->talker), 
                           vendorGetText(cnv->talker, TV_SAYS), 
                           tavernInfo[i], 
                           vendorGetText(cnv->talker, TV_ANYTHINGELSE), 
                           NULL);
    }
    if (!reply)
        reply = concat(vendorGetText(cnv->talker, TV_DONTKNOW),
                       vendorGetText(cnv->talker, TV_ANYTHINGELSE),
                       NULL);

    cnv->state = CONV_CONTINUEQUESTION;

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

    info->item = desc->item;

    if (desc->cityMapOffset != -1) {
        fseek(avatar, desc->cityMapOffset, SEEK_SET);
        for (i = 0; i < VCM_SIZE; i++) {
            if (!readChar(&(info->cityMap[i]), avatar))
                return NULL;
        }
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
