/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "vendor.h"

#include "armor.h"
#include "context.h"
#include "debug.h"
#include "location.h"
#include "music.h"
#include "names.h"
#include "person.h"
#include "player.h"
#include "savegame.h"
#include "stats.h"
#include "u4file.h"
#include "utils.h"
#include "weapon.h"

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
#define N_INN_VENDORS 7

#define ARMS_VENDOR_INVENTORY_SIZE 4
#define N_TAVERN_TOPICS 6 
#define N_GUILD_ITEMS 4

#define VCM_SIZE 16

const struct {
    char *(*getIntro)(Conversation *cnv);
    char *(*getVendorQuestionResponse)(Conversation *cnv, const char *inquiry);
    char *(*getBuyItemResponse)(Conversation *cnv, const char *inquiry);
    char *(*getSellItemResponse)(Conversation *cnv, const char *inquiry);
    char *(*getBuyQuantityResponse)(Conversation *cnv, const char *inquiry);
    char *(*getSellQuantityResponse)(Conversation *cnv, const char *inquiry);
    char *(*getBuyPriceResponse)(Conversation *cnv, const char *inquiry);
    char *(*getConfirmationResponse)(Conversation *cnv, const char *answer);
    char *(*getContinueQuestionResponse)(Conversation *cnv, const char *answer);
    char *(*getTopicResponse)(Conversation *cnv, const char *inquiry);
    char *(*getPlayerResponse)(Conversation *cnv, const char *inquiry);
    const char *vendorQuestionChoices;
} vendorType[] = {
    { &vendorGetIntro, &vendorGetArmsVendorQuestionResponse, &vendorGetArmsBuyItemResponse, &vendorGetSellItemResponse, 
      &vendorGetBuyQuantityResponse, &vendorGetSellQuantityResponse, NULL, 
      &vendorGetArmsConfirmationResponse, &vendorGetContinueQuestionResponse, 
      NULL, NULL, "bs\015 \033" }, /* NPC_VENDOR_WEAPONS */
    { &vendorGetIntro, &vendorGetArmsVendorQuestionResponse, &vendorGetArmsBuyItemResponse, &vendorGetSellItemResponse, 
      &vendorGetBuyQuantityResponse, &vendorGetSellQuantityResponse, NULL,
      &vendorGetArmsConfirmationResponse, &vendorGetContinueQuestionResponse, 
      NULL, NULL, "bs\015 \033" }, /* NPC_VENDOR_ARMOR */
    { &vendorGetIntro, NULL, NULL, NULL, 
      &vendorGetBuyQuantityResponse, NULL, NULL, 
      NULL, &vendorGetContinueQuestionResponse, 
      NULL, NULL, NULL }, /* NPC_VENDOR_FOOD */
    { &vendorGetIntro, &vendorGetTavernVendorQuestionResponse, NULL, NULL, 
      &vendorGetBuyQuantityResponse, NULL, &vendorGetTavernBuyPriceResponse,
      NULL, &vendorGetContinueQuestionResponse, 
      &vendorGetTavernTopicResponse, NULL, "af\015 \033" }, /* NPC_VENDOR_TAVERN */
    { &vendorGetIntro, NULL, &vendorGetReagentsBuyItemResponse, NULL,
      &vendorGetBuyQuantityResponse, NULL, &vendorGetReagentsBuyPriceResponse,
      NULL, &vendorGetContinueQuestionResponse, 
      NULL, NULL, NULL }, /* NPC_VENDOR_REAGENTS */
    { &vendorGetIntro, NULL, &vendorGetHealerBuyItemResponse, NULL,
      NULL, NULL, NULL, 
      &vendorGetHealerConfirmationResponse, &vendorGetContinueQuestionResponse,
      NULL, &vendorGetHealerPlayerResponse, NULL }, /* NPC_VENDOR_HEALER */
    { &vendorGetIntro, &vendorGetInnVendorQuestionResponse, NULL, NULL,
      NULL, NULL, NULL,
      &vendorGetInnConfirmationResponse, &vendorGetContinueQuestionResponse, 
      NULL, NULL, "123\015 \033" }, /* NPC_VENDOR_INN */
    { &vendorGetIntro, NULL, &vendorGetGuildBuyItemResponse, NULL,
      NULL, NULL, NULL, 
      &vendorGetGuildConfirmationResponse, &vendorGetContinueQuestionResponse, 
      NULL, NULL, "ny\015 \033" }, /* NPC_VENDOR_GUILD */
    { &vendorGetIntro, NULL, NULL, NULL,
      NULL, NULL, NULL, 
      &vendorGetStableConfirmationResponse, &vendorGetContinueQuestionResponse, NULL, 
      NULL, NULL }, /* NPC_VENDOR_STABLE */
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
    /* item      citymap  shops/names    itedescs   text1      text2  */
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

typedef struct InnVendorInfo {
    unsigned char room_x;
    unsigned char room_y;
    unsigned char price;
} InnVendorInfo;

ArmsVendorInfo weaponVendorInfo;
ArmsVendorInfo armorVendorInfo;
unsigned char reagPrices[N_REAG_VENDORS][REAG_MAX];
unsigned short foodPrices[N_FOOD_VENDORS];
char **tavernSpecialties;
char **tavernTopics;
char **tavernInfo;
unsigned short tavernInfoPrices[N_TAVERN_TOPICS];
unsigned short tavernFoodPrices[N_TAVERN_VENDORS];
InnVendorInfo innVendorInfo[N_INN_VENDORS];
unsigned short guildItemPrices[N_GUILD_ITEMS];
unsigned short guildItemQuantities[N_GUILD_ITEMS];

VendorTypeInfo **vendorTypeInfo;

#define WV_NOTENOUGH 0
#define WV_FINECHOICE 1
#define WV_VERYGOOD 100
#define WV_WEHAVE 101
#define WV_YOURINTEREST 102
#define WV_CANTAFFORDONE 103
#define WV_HOWMANYTOBUY 104
#define WV_TOOBAD 105
#define WV_ANYTHINGELSE 106
#define WV_WHATWILL 107
#define WV_YOUSELL 108
#define WV_DONTOWN 109
#define WV_HOWMANY 110
#define WV_TOSELL 111
#define WV_TOOBAD2 112
#define WV_DONTOWNENOUGH 113
#define WV_ILLGIVEYA 114
#define WV_GPFOR 115
#define WV_THEM 116
#define WV_IT 117
#define WV_ILLGIVEYA2 118
#define WV_GPFOR2 119
#define WV_DEAL 120
#define WV_HMMPH 121
#define WV_FINE 122
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

#define HV_ASKS 0
#define HV_WHONEEDS 1
#define HV_NOONE 2
#define HV_WILLPAY 100
#define HV_CANNOTAID 101
#define HV_CURINGCOSTS 103
#define HV_CANTAFFORDCURE 104
#define HV_FORFREE 105
#define HV_HEALINGCOSTS 107
#define HV_CANTAFFORD 108
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
#define SV_ASHAME2 4
#define SV_NOTENOUGH 5
#define SV_BESTBREED 6

InnHandlerCallback innHandlerCallback;

char *vendorGetFarewell(const Conversation *cnv, const char *prefix);
VendorTypeInfo *vendorLoadTypeInfo(U4FILE *avatar, const VendorTypeDesc *desc);
const char *vendorGetName(const Person *v);
const char *vendorGetShop(const Person *v);
const char *vendorGetText(const Person *v, int textId);
const VendorTypeInfo *vendorGetInfo(const Person *v);
char *vendorGetArmsVendorMenu(const Person *v);
char *vendorDoBuyTransaction(Conversation *cnv);
char *vendorDoSellTransaction(Conversation *cnv);
int armsVendorInfoRead(ArmsVendorInfo *info, int nprices, U4FILE *f);
int innVendorInfoRead(InnVendorInfo *info, U4FILE *f);



void vendorSetInnHandlerCallback(InnHandlerCallback callback) {
    innHandlerCallback = callback;
}

/**
 * Loads in prices and conversation data for vendors from avatar.exe.
 */
int vendorInit() {
    int i, j;
    U4FILE *avatar;

    avatar = u4fopen("avatar.exe");
    if (!avatar)
        return 0;

    u4fseek(avatar, 78859, SEEK_SET);
    for (i = 0; i < N_REAG_VENDORS; i++) {
        for (j = 0; j < REAG_MAX - 2; j++)
            reagPrices[i][j] = u4fgetc(avatar);
        reagPrices[i][REAG_NIGHTSHADE] = 0;
        reagPrices[i][REAG_MANDRAKE] = 0;
    }

    u4fseek(avatar, 87535, SEEK_SET);
    for (i = 0; i < N_FOOD_VENDORS; i++) {
        foodPrices[i] = u4fgetshort(avatar);
    }

    tavernSpecialties = u4read_stringtable(avatar, 85521, N_TAVERN_VENDORS);
    tavernTopics = u4read_stringtable(avatar, 85599, N_TAVERN_TOPICS);
    tavernInfo = u4read_stringtable(avatar, 85671, N_TAVERN_TOPICS);

    u4fseek(avatar, 86379, SEEK_SET);
    for (i = 0; i < N_TAVERN_TOPICS; i++)
        tavernInfoPrices[i] = u4fgetshort(avatar);

    u4fseek(avatar, 86427, SEEK_SET);
    for (i = 0; i < N_TAVERN_VENDORS; i++)
        tavernFoodPrices[i] = u4fgetshort(avatar);

    u4fseek(avatar, 83719, SEEK_SET);
    if (!innVendorInfoRead(&(innVendorInfo[0]), avatar))
        return 0;

    u4fseek(avatar, 82969, SEEK_SET);
    for (i = 0; i < N_GUILD_ITEMS; i++)
        guildItemPrices[i] = u4fgetshort(avatar);

    u4fseek(avatar, 82977, SEEK_SET);
    for (i = 0; i < N_GUILD_ITEMS; i++)
        guildItemQuantities[i] = u4fgetshort(avatar);

    u4fseek(avatar, 80181, SEEK_SET);
    if (!armsVendorInfoRead(&weaponVendorInfo, WEAP_MAX, avatar))
        return 0;

    u4fseek(avatar, 81471, SEEK_SET);
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
    case CONV_CONFIRMATION:
        *response = (*vendorType[cnv->talker->npcType - NPC_VENDOR_WEAPONS].getConfirmationResponse)(cnv, inquiry);
        return;
    case CONV_CONTINUEQUESTION:
        *response = (*vendorType[cnv->talker->npcType - NPC_VENDOR_WEAPONS].getContinueQuestionResponse)(cnv, inquiry);
        return;
    case CONV_TOPIC:
        *response = (*vendorType[cnv->talker->npcType - NPC_VENDOR_WEAPONS].getTopicResponse)(cnv, inquiry);
        return;
    case CONV_PLAYER:
        *response = (*vendorType[cnv->talker->npcType - NPC_VENDOR_WEAPONS].getPlayerResponse)(cnv, inquiry);
        return;
    default:
        ASSERT(0, "invalid state: %d", cnv->state);
    }
}

char *vendorGetPrompt(const Conversation *cnv) {
    if (cnv->talker->npcType != NPC_VENDOR_WEAPONS &&
        cnv->talker->npcType != NPC_VENDOR_ARMOR)
        return strdup("");

    switch (cnv->state) {

    case CONV_VENDORQUESTION:
    case CONV_SELL_QUANTITY:
    case CONV_BUY_PRICE:
    case CONV_CONFIRMATION:
    case CONV_CONTINUEQUESTION:
    case CONV_TOPIC:
    case CONV_PLAYER:
        return strdup("");
        break;

    case CONV_BUY_ITEM:
        return strdup(vendorGetText(cnv->talker, WV_YOURINTEREST));
        break;
        
    case CONV_SELL_ITEM:
        return strdup(vendorGetText(cnv->talker, WV_YOUSELL));
        break;

    case CONV_BUY_QUANTITY:
        return strdup(vendorGetText(cnv->talker, WV_HOWMANYTOBUY));
        break;

    default:
        ASSERT(0, "invalid state: %d", cnv->state);
    }

    return strdup("");
}

char *vendorGetFarewell(const Conversation *cnv, const char *prefix) {
    if (!prefix)
        prefix = "";

    switch (cnv->talker->npcType) {

    case NPC_VENDOR_WEAPONS:
    case NPC_VENDOR_ARMOR:
        return concat(prefix, vendorGetName(cnv->talker), vendorGetText(cnv->talker, WV_BYE), NULL);

    case NPC_VENDOR_FOOD:
        return concat(prefix, vendorGetText(cnv->talker, FV_BYE), NULL);

    case NPC_VENDOR_TAVERN:
        return concat(prefix, vendorGetText(cnv->talker, TV_BYE), NULL);

    case NPC_VENDOR_REAGENTS:
        return concat(prefix, vendorGetName(cnv->talker), vendorGetText(cnv->talker, RV_BYE), NULL);

    case NPC_VENDOR_HEALER:
        return concat(prefix, vendorGetName(cnv->talker), vendorGetText(cnv->talker, HV_BYE), NULL);

    case NPC_VENDOR_INN:
        return concat(prefix, vendorGetName(cnv->talker), vendorGetText(cnv->talker, IV_WRONGPLACE), NULL);

    case NPC_VENDOR_GUILD:
        return concat(prefix, vendorGetName(cnv->talker), vendorGetText(cnv->talker, GV_SEEYA), NULL);

    case NPC_VENDOR_STABLE:
        return concat(prefix, vendorGetText(cnv->talker, SV_ASHAME), NULL);

    default:
        ASSERT(0, "invalid npc type: %d", cnv->talker->npcType);
    }

    return NULL;
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

    ASSERT(v->npcType >= NPC_VENDOR_WEAPONS && v->npcType <= NPC_VENDOR_STABLE, "invalid npc type: %d", v->npcType);

    if (v->npcType == NPC_VENDOR_GUILD) {
        if (c->location->map->id == 14)   /* Buccaneers Den */
            return 0;
        if (c->location->map->id == 15)   /* Vesper */
            return 1;
        ASSERT(0, "map doesn't have guild vendor: %d", c->location->map->id);
    }

    if (v->npcType == NPC_VENDOR_STABLE) {
        if (c->location->map->id == 13)   /* Paws */
            return 0;
        ASSERT(0, "map doesn't have stable: %d", c->location->map->id);
    }


    type = v->npcType - NPC_VENDOR_WEAPONS;

    ASSERT((c->location->map->id - 1) < VCM_SIZE, "map id out of range: %d", c->location->map->id);
    ASSERT(vendorTypeInfo[type]->cityMap[c->location->map->id - 1] != 0, "map doesn't have vendor: %d", c->location->map->id);

    return vendorTypeInfo[type]->cityMap[c->location->map->id - 1] - 1;
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
    ASSERT(v->npcType >= NPC_VENDOR_WEAPONS && v->npcType <= NPC_VENDOR_STABLE, "npc type out of range: %d", v->npcType);

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
                name = weaponGetName((WeaponType) info->vendorInventory[vendorGetVendorNo(v)][i]);
            else
                name = armorGetName((ArmorType) info->vendorInventory[vendorGetVendorNo(v)][i]);

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
        if (c->transportContext & TRANSPORT_HORSE) {
            intro = strdup("The Innkeeper says: Get that horse out of here!!!\n");
            cnv->state = CONV_DONE;
        }
        else {
            intro = concat(vendorGetText(cnv->talker, IV_WELCOME), vendorGetShop(cnv->talker), 
                           vendorGetText(cnv->talker, IV_IAM), vendorGetName(cnv->talker), 
                           vendorGetText(cnv->talker, IV_NEEDLODGING), NULL);
            cnv->state = CONV_CONTINUEQUESTION;
        }
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
        ASSERT(0, "invalid npc type: %d", cnv->talker->npcType);
    }

    if (cnv->state != CONV_DONE)
        musicShopping();

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

char *vendorGetInnVendorQuestionResponse(Conversation *cnv, const char *response) {
    if (response[0] < '1' || response[0] > '3') {
        cnv->state = CONV_DONE;
        return strdup("");
    }

    cnv->quant = response[0] - '0';
    cnv->price = innVendorInfo[vendorGetVendorNo(cnv->talker)].price * cnv->quant;

    return vendorDoBuyTransaction(cnv);
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
        ASSERT(0, "invalid npc type: %d", cnv->talker->npcType);

    if (response[0] == '\033' || response[0] == '\015' || response[0] == ' ') {
        cnv->state = CONV_DONE;
        return vendorGetFarewell(cnv, NULL);
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
    else if (playerCanAfford(c->saveGame, info->prices[cnv->itemSubtype]) == 0) {
        reply = concat(vendorGetText(cnv->talker, WV_CANTAFFORDONE), 
                       vendorGetText(cnv->talker, WV_ANYTHINGELSE), 
                       NULL);
        cnv->state = CONV_CONTINUEQUESTION;
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

    if (response[0] == '\033' || response[0] == '\015' || response[0] == ' ') {
        cnv->state = CONV_DONE;
        return vendorGetFarewell(cnv, NULL);
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

    if (response[0] == '\033' || response[0] == '\015' || response[0] == ' ') {
        cnv->state = CONV_DONE;
        return vendorGetFarewell(cnv, NULL);
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
        else if (cnv->itemSubtype == HT_FULLHEAL) {
            msg = HV_HEALINGCOSTS;
            cnv->price = 200;
        }
        else if (cnv->itemSubtype == HT_RESURRECT) {
            msg = HV_REZCOSTS;
            cnv->price = 300;
        }
        if (c->saveGame->members == 1) {
            cnv->player = 0;
            reply = concat(vendorGetText(cnv->talker, msg), 
                           vendorGetText(cnv->talker, HV_WILLPAY), 
                           NULL);
            cnv->state = CONV_CONFIRMATION;
        } else {
            reply = concat(vendorGetName(cnv->talker),
                           vendorGetText(cnv->talker, HV_ASKS), 
                           vendorGetText(cnv->talker, HV_WHONEEDS), 
                           NULL);
            cnv->state = CONV_PLAYER;
        }
    }

    return reply;
}

char *vendorGetGuildBuyItemResponse(Conversation *cnv, const char *response) {
    char *reply;

    cnv->itemSubtype = -1;

    if (response[0] == '\033' || response[0] == '\015' || response[0] == ' ') {
        cnv->state = CONV_DONE;
        return vendorGetFarewell(cnv, NULL);
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
        cnv->state = CONV_CONFIRMATION;
    }

    return reply;
}

char *vendorGetSellItemResponse(Conversation *cnv, const char *response) {
    char buffer[10];
    char *reply;
    int itemMax;
    const char *itemName;
    int owned;

    cnv->itemSubtype = tolower(response[0]) - 'a';

    if (cnv->talker->npcType == NPC_VENDOR_WEAPONS)
        itemMax = WEAP_MAX;
    else
        itemMax = ARMR_MAX;

    if (cnv->itemSubtype >= itemMax) {
        return strdup("");
    } 
    else if (response[0] == '\033' || response[0] == '\015' || response[0] == ' ') {
        cnv->state = CONV_DONE;
        return vendorGetFarewell(cnv, NULL);
    }

    if (cnv->talker->npcType == NPC_VENDOR_WEAPONS)
        owned = c->saveGame->weapons[cnv->itemSubtype];
    else /* cnv->talker->npcType == NPC_VENDOR_ARMOR */
        owned = c->saveGame->armor[cnv->itemSubtype];


    if (cnv->itemSubtype == 0 || owned < 1) {
        reply = strdup(vendorGetText(cnv->talker, WV_DONTOWN));
        return reply;
    }

    if (cnv->talker->npcType == NPC_VENDOR_WEAPONS) {
        cnv->price = weaponVendorInfo.prices[cnv->itemSubtype] / 2;
        itemName = weaponGetName((WeaponType) cnv->itemSubtype);
    }
    else { /* cnv->talker->npcType == NPC_VENDOR_ARMOR */
        cnv->price = armorVendorInfo.prices[cnv->itemSubtype] / 2;
        itemName = armorGetName((ArmorType) cnv->itemSubtype);
    }

    if (owned == 1) {
        cnv->quant = 1;

        snprintf(buffer, sizeof(buffer), "%d", cnv->price);
        reply = concat(vendorGetText(cnv->talker, WV_ILLGIVEYA2),
                       buffer,
                       vendorGetText(cnv->talker, WV_GPFOR2),
                       itemName,
                       vendorGetText(cnv->talker, WV_DEAL),
                       NULL);
        cnv->state = CONV_CONFIRMATION;
    }

    else {
        reply = concat(vendorGetText(cnv->talker, WV_HOWMANY),
                       itemName,
                       vendorGetText(cnv->talker, WV_TOSELL),
                       NULL);

        cnv->state = CONV_SELL_QUANTITY;
    }

    return reply;
}

char *vendorGetBuyQuantityResponse(Conversation *cnv, const char *response) {
    char buffer[10];

    cnv->quant = (int) strtol(response, NULL, 10);

    if (cnv->quant == 0) {
        switch (cnv->talker->npcType) {
        case NPC_VENDOR_WEAPONS:
        case NPC_VENDOR_ARMOR:
            cnv->state = CONV_CONTINUEQUESTION;
            return concat(vendorGetText(cnv->talker, WV_TOOBAD),
                          vendorGetText(cnv->talker, WV_ANYTHINGELSE), 
                          NULL);
        case NPC_VENDOR_FOOD:
            cnv->state = CONV_DONE;
            return vendorGetFarewell(cnv, vendorGetText(cnv->talker, FV_TOOBAD));
            
        case NPC_VENDOR_TAVERN:
            cnv->state = CONV_DONE;
            return vendorGetFarewell(cnv, NULL);

        case NPC_VENDOR_REAGENTS:
            cnv->state = CONV_CONTINUEQUESTION;
            return concat(vendorGetText(cnv->talker, RV_ISEE),
                          vendorGetText(cnv->talker, RV_ANYTHINGELSE), 
                          NULL);

        default:
            ASSERT(0, "invalid npc type: %d", cnv->talker->npcType);
        }
    }

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
        ASSERT(0, "invalid npc type: %d", cnv->talker->npcType);
    }
    return NULL;
}

char *vendorGetSellQuantityResponse(Conversation *cnv, const char *response) {
    char buffer[10];
    char *reply;

    ASSERT(cnv->talker->npcType == NPC_VENDOR_WEAPONS ||
           cnv->talker->npcType == NPC_VENDOR_ARMOR,
           "invalid npc type: %d", cnv->talker->npcType);

    cnv->quant = (int) strtol(response, NULL, 10);

    /* zero: say too bad and ask again */
    if (cnv->quant == 0) {
        cnv->state = CONV_CONTINUEQUESTION;
        return concat(vendorGetText(cnv->talker, WV_TOOBAD2), 
                      vendorGetText(cnv->talker, WV_ANYTHINGELSE), NULL);
    }

    if (!playerCanSell(c->saveGame, cnv->itemType, cnv->itemSubtype, cnv->quant)) {
        cnv->state = CONV_DONE;
        return vendorGetFarewell(cnv, vendorGetText(cnv->talker, WV_DONTOWNENOUGH)); 
    }

    /* user entered a valid quantity */

    cnv->price *= cnv->quant;

    snprintf(buffer, sizeof(buffer), "%d", cnv->price);
    reply = concat(vendorGetText(cnv->talker, WV_ILLGIVEYA),
                   buffer,
                   vendorGetText(cnv->talker, WV_GPFOR),
                   cnv->quant == 1 ? vendorGetText(cnv->talker, WV_IT) : vendorGetText(cnv->talker, WV_THEM),
                   vendorGetText(cnv->talker, WV_DEAL),
                   NULL);
    cnv->state = CONV_CONFIRMATION;

    return reply;
}

char *vendorGetTavernBuyPriceResponse(Conversation *cnv, const char *response) {
    char *reply;
    
    cnv->price = (int) strtol(response, NULL, 10);

    if (playerCanAfford(c->saveGame, cnv->price) == 0) {
        reply = vendorGetFarewell(cnv, vendorGetText(cnv->talker, TV_NOTENOUGH));
        cnv->state = CONV_DONE;
    }

    if (cnv->price < 2) {
        reply = vendorGetFarewell(cnv, vendorGetText(cnv->talker, TV_WONTPAY));
        cnv->state = CONV_DONE;
    } else if (cnv->price == 2) {
        reply = strdup(vendorGetText(cnv->talker, TV_ANYTHINGELSE));
        cnv->state = CONV_CONTINUEQUESTION;
        playerPurchase(c->saveGame, INV_NONE, 0, 0, cnv->price);
    } else {
        reply = strdup(vendorGetText(cnv->talker, TV_WHATINFO));
        cnv->state = CONV_TOPIC;
        playerPurchase(c->saveGame, INV_NONE, 0, 0, cnv->price);
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
            reply = concat(vendorGetName(cnv->talker), 
                           vendorGetText(cnv->talker, WV_FINECHOICE), 
                           vendorGetText(cnv->talker, WV_ANYTHINGELSE), 
                           NULL);
        else
            reply = concat(vendorGetText(cnv->talker, WV_NOTENOUGH),
                           vendorGetText(cnv->talker, WV_ANYTHINGELSE), 
                           NULL);
        break;

    case NPC_VENDOR_FOOD:
        if (success)
            reply = strdup(vendorGetText(cnv->talker, FV_ANYMORE));

        else {
            char buffer[10];
            int afford;

            afford = playerCanAfford(c->saveGame, foodPrices[vendorGetVendorNo(cnv->talker)]);
            if (afford == 0) {
                reply = vendorGetFarewell(cnv, vendorGetText(cnv->talker, FV_CANTAFFORD));
                cnv->state = CONV_DONE;
            } else {
                snprintf(buffer, sizeof(buffer), "%d", afford);
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
            int afford;

            afford = playerCanAfford(c->saveGame, tavernFoodPrices[vendorGetVendorNo(cnv->talker)]);
            if (afford == 0) {
                reply = concat(vendorGetText(cnv->talker, TV_CANTAFFORD), 
                               vendorGetText(cnv->talker, TV_HEREYOUARE),
                               NULL);
            } else {
                snprintf(buffer, sizeof(buffer), "%d", afford);
                reply = concat(vendorGetText(cnv->talker, TV_AFFORDONLY1),
                               buffer,
                               vendorGetText(cnv->talker, TV_AFFORDONLY2), "s.\n",
                               vendorGetText(cnv->talker, TV_HEREYOUARE),
                               NULL);
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

    case NPC_VENDOR_HEALER:
        if (success) {
            playerHeal(c->saveGame, cnv->itemSubtype, cnv->player);
            reply = concat(vendorGetName(cnv->talker), 
                           vendorGetText(cnv->talker, HV_MOREHELP),
                           NULL);
        }
        else {
            reply = concat(vendorGetText(cnv->talker, HV_CANTAFFORD),
                           vendorGetName(cnv->talker), 
                           vendorGetText(cnv->talker, HV_MOREHELP), 
                           NULL);
        }
        break;

    case NPC_VENDOR_INN:
        if (success) {
            if (vendorGetVendorNo(cnv->talker) == 3) {
                switch (cnv->quant) {
                case 1:
                    c->location->x = 2;
                    c->location->y = 6;
                    break;
                case 2:
                    c->location->x = 2;
                    c->location->y = 2;
                    break;
                case 3:
                    c->location->x = 8;
                    c->location->y = 2;
                    break;
                }
            } 
            else {
                c->location->x = innVendorInfo[vendorGetVendorNo(cnv->talker)].room_x;
                c->location->y = innVendorInfo[vendorGetVendorNo(cnv->talker)].room_y;
            }
            if ((rand() % 4) == 0)
                reply = concat(vendorGetText(cnv->talker, IV_GOODNIGHT), 
                               vendorGetText(cnv->talker, IV_RATS),
                               NULL);
            else 
                reply = strdup(vendorGetText(cnv->talker, IV_GOODNIGHT));
            cnv->state = CONV_DONE;

            (*innHandlerCallback)();
        }
        else {
            reply = strdup(vendorGetText(cnv->talker, IV_CANTPAY));
            cnv->state = CONV_DONE;
        }
        break;

    case NPC_VENDOR_GUILD:
        if (success)
            reply = concat(vendorGetText(cnv->talker, GV_FINEFINE),
                           vendorGetName(cnv->talker),
                           vendorGetText(cnv->talker, GV_SEEMORE),
                           NULL);
        else {
            reply = strdup(vendorGetText(cnv->talker, GV_BUZZOFF));
            cnv->state = CONV_DONE;
        }
        break;

    case NPC_VENDOR_STABLE:
        if (success) {
            reply = strdup(vendorGetText(cnv->talker, SV_BESTBREED));
            cnv->state = CONV_DONE;
        } else {
            reply = strdup(vendorGetText(cnv->talker, SV_NOTENOUGH));
            cnv->state = CONV_DONE;
        }
        break;

    default:
        ASSERT(0, "invalid npc type: %d", cnv->talker->npcType);
    }

    return reply;
}

char *vendorDoSellTransaction(Conversation *cnv) {
    char *reply;
    int success;

    ASSERT(cnv->talker->npcType == NPC_VENDOR_WEAPONS ||
           cnv->talker->npcType == NPC_VENDOR_ARMOR, 
           "invalid npc type: %d", cnv->talker->npcType);

    success = playerSell(c->saveGame, cnv->itemType, cnv->itemSubtype, cnv->quant, cnv->price);

    if (success) {
        reply = strdup(vendorGetText(cnv->talker, WV_FINE));
        cnv->state = CONV_SELL_ITEM;
    }
    else {
        reply = vendorGetFarewell(cnv, vendorGetText(cnv->talker, WV_DONTOWNENOUGH));
        cnv->state = CONV_DONE;
    }

    return reply;
}

char *vendorGetArmsConfirmationResponse(Conversation *cnv, const char *response) {
    char *reply;

    if (tolower(response[0]) == 'y') {
        return vendorDoSellTransaction(cnv);
    }
    else if (tolower(response[0]) == 'n') {
        reply = strdup(vendorGetText(cnv->talker, WV_HMMPH));
        cnv->state = CONV_SELL_ITEM;
    }
    else {
        reply = strdup("");
        cnv->state = CONV_DONE;
    }

    return reply;
}

char *vendorGetHealerConfirmationResponse(Conversation *cnv, const char *response) {
    char *reply;
    
    if (tolower(response[0]) == 'y') {
        return vendorDoBuyTransaction(cnv);
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

char *vendorGetInnConfirmationResponse(Conversation *cnv, const char *response) {
    char *reply;
    
    if (tolower(response[0]) == 'y') {
        cnv->price = innVendorInfo[vendorGetVendorNo(cnv->talker)].price;
        cnv->quant = 1;
        return vendorDoBuyTransaction(cnv);
    }
    else if (tolower(response[0]) == 'n') {
        reply = strdup(vendorGetText(cnv->talker, IV_NOBETTERDEAL));
        cnv->state = CONV_DONE;
    } 
    else {
        reply = strdup("");
        cnv->state = CONV_DONE;
    }

    return reply;
}

char *vendorGetGuildConfirmationResponse(Conversation *cnv, const char *response) {
    char *reply;
    
    if (tolower(response[0]) == 'y') {
        return vendorDoBuyTransaction(cnv);
    }
    else if (tolower(response[0]) == 'n') {
        reply = vendorGetFarewell(cnv, vendorGetText(cnv->talker, GV_GRMBL));
        cnv->state = CONV_DONE;
    } 
    else {
        reply = strdup("");
        cnv->state = CONV_DONE;
    }

    return reply;
}

char *vendorGetStableConfirmationResponse(Conversation *cnv, const char *response) {
    char *reply;
    
    if (tolower(response[0]) == 'y') {
        return vendorDoBuyTransaction(cnv);
    }
    else if (tolower(response[0]) == 'n') {
        reply = vendorGetFarewell(cnv, NULL);
        cnv->state = CONV_DONE;
    } 
    else {
        reply = strdup("");
        cnv->state = CONV_DONE;
    }

    return reply;
}

char *vendorGetContinueQuestionResponse(Conversation *cnv, const char *answer) {
    char buffer[10];
    char *reply;
    char *menu;
    
    if (tolower(answer[0]) == 'n') {
        cnv->state = CONV_DONE;
        return vendorGetFarewell(cnv, NULL);
    }
    else if (tolower(answer[0]) != 'y') {
        cnv->state = CONV_DONE;
        return strdup("");
    }

    /* response was 'y' */

    switch (cnv->talker->npcType) {

    case NPC_VENDOR_WEAPONS:
    case NPC_VENDOR_ARMOR:
        menu = vendorGetArmsVendorMenu(cnv->talker);
        reply = concat(vendorGetText(cnv->talker, WV_WEHAVE),
                       menu, 
                       NULL);
        free(menu);
        cnv->state = CONV_BUY_ITEM;
        break;

    case NPC_VENDOR_FOOD:
        snprintf(buffer, sizeof(buffer), "%d", foodPrices[vendorGetVendorNo(cnv->talker)]);
        reply = concat(vendorGetText(cnv->talker, FV_PRICE1), buffer,
                       vendorGetText(cnv->talker, FV_PRICE2), 
                       vendorGetText(cnv->talker, FV_HOWMANY),
                       NULL);
        cnv->state = CONV_BUY_QUANTITY;
        break;

    case NPC_VENDOR_TAVERN:
        reply = concat(vendorGetName(cnv->talker), vendorGetText(cnv->talker, TV_FOODORALE), NULL);
        cnv->state = CONV_VENDORQUESTION;
        break;

    case NPC_VENDOR_REAGENTS:
        reply = concat(vendorGetText(cnv->talker, RV_VERYWELL),
                       vendorGetText(cnv->talker, RV_MENU),
                       vendorGetText(cnv->talker, RV_YOURINTEREST),
                       NULL);
        cnv->state = CONV_BUY_ITEM;
        break;

    case NPC_VENDOR_HEALER:
        reply = concat(vendorGetName(cnv->talker),
                       vendorGetText(cnv->talker, HV_CANPERFORM),
                       vendorGetText(cnv->talker, HV_YOURNEED),
                       NULL);
        cnv->state = CONV_BUY_ITEM;
        break;

    case NPC_VENDOR_INN:
        /* vendor #3 in minoc is special; gives the choice of 1, 2, or 3 beds */
        if (vendorGetVendorNo(cnv->talker) == 3) {
            reply = concat(vendorGetText(cnv->talker, IV_WEHAVE + vendorGetVendorNo(cnv->talker)), 
                           "\n",
                           vendorGetText(cnv->talker, IV_BEDS),
                           NULL);
            cnv->state = CONV_VENDORQUESTION;
        }
        else {
            reply = concat(vendorGetText(cnv->talker, IV_WEHAVE + vendorGetVendorNo(cnv->talker)), 
                           vendorGetText(cnv->talker, IV_TAKEIT),
                           NULL);
            cnv->state = CONV_CONFIRMATION;
        }
        break;

    case NPC_VENDOR_GUILD:
        reply = concat(vendorGetName(cnv->talker),
                       vendorGetText(cnv->talker, GV_IGOT), 
                       vendorGetText(cnv->talker, GV_WHATLLITBE),
                       NULL);
        cnv->state = CONV_BUY_ITEM;
        break;

    case NPC_VENDOR_STABLE:
        cnv->price = c->saveGame->members * 100;
        snprintf(buffer, sizeof(buffer), "%d", c->saveGame->members);
        reply = concat(vendorGetText(cnv->talker, SV_FORONLY), 
                       buffer,
                       vendorGetText(cnv->talker, SV_WILLBUY),
                       NULL);
        cnv->state = CONV_CONFIRMATION;
        break;

    default:
        ASSERT(0, "invalid npc type: %d", cnv->talker->npcType);
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

    /* FIXME: The above statement is incorrect.  There are 6 topics.
       I'm not sure which topics are which -- to find out check for
       the bug report which corresponds to this issue, but here is 
       the pattern:
               
               1 2 3 4 5 6
               x
               x x
               x x x
               x x x x
               x x x x x
               x x x x x x

       Where 1-6 are the different taverns, and the x's represent the
       different topics that they'll discuss.
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

char *vendorGetHealerPlayerResponse(Conversation *cnv, const char *response) {
    int msg;
    if (response[0] < '1' || response[0] > '8') {
        cnv->state = CONV_CONTINUEQUESTION;
        return concat(vendorGetText(cnv->talker, HV_NOONE),
                      vendorGetName(cnv->talker), 
                      vendorGetText(cnv->talker, HV_MOREHELP),
                      NULL);
    }

    cnv->player = response[0] - '1';
    cnv->state = CONV_CONFIRMATION;

    if (cnv->itemSubtype == HT_CURE)
        msg = HV_CURINGCOSTS;
    else if (cnv->itemSubtype == HT_FULLHEAL)
        msg = HV_HEALINGCOSTS;
    else if (cnv->itemSubtype == HT_RESURRECT)
        msg = HV_REZCOSTS;
    return concat(vendorGetText(cnv->talker, msg), 
                  vendorGetText(cnv->talker, HV_WILLPAY), 
                  NULL);
}

int armsVendorInfoRead(ArmsVendorInfo *info, int nprices, U4FILE *f) {
    int i, j;

    for (i = 0; i < N_ARMS_VENDORS; i++) {
        for (j = 0; j < ARMS_VENDOR_INVENTORY_SIZE; j++)
            info->vendorInventory[i][j] = u4fgetc(f);
    }

    for (i = 0; i < nprices; i++)
        info->prices[i] = u4fgetshort(f);

    return 1;
}

int innVendorInfoRead(InnVendorInfo *info, U4FILE *f) {
    int i;

    for (i = 0; i < N_INN_VENDORS; i++)
        info[i].room_x = u4fgetc(f);

    /* read pad byte */
    u4fgetc(f);

    for (i = 0; i < N_INN_VENDORS; i++)
        info[i].room_y = u4fgetc(f);

    /* read pad byte */
    u4fgetc(f);

    for (i = 0; i < N_INN_VENDORS; i++)
        info[i].price = u4fgetc(f);

    return 1;
}

VendorTypeInfo *vendorLoadTypeInfo(U4FILE *avatar, const VendorTypeDesc *desc) {
    int i;
    VendorTypeInfo *info = (VendorTypeInfo *) malloc(sizeof(VendorTypeInfo));

    memset(info, 0, sizeof(VendorTypeInfo));

    info->item = desc->item;

    if (desc->cityMapOffset != -1) {
        u4fseek(avatar, desc->cityMapOffset, SEEK_SET);
        for (i = 0; i < VCM_SIZE; i++)
            info->cityMap[i] = u4fgetc(avatar);
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
