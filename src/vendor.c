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

#define N_ARMS_VENDORS 6
#define ARMS_VENDOR_INVENTORY_SIZE 4

typedef struct ArmsVendorInfo {
    unsigned char vendorInventory[N_ARMS_VENDORS][ARMS_VENDOR_INVENTORY_SIZE];
    unsigned short prices[WEAP_MAX];
} ArmsVendorInfo;

#define N_REAG_VENDORS 4

char **weaponVendorText;
char **weaponVendorText2;
char **armorVendorText;
char **armorVendorText2;
ArmsVendorInfo weaponVendorInfo;
ArmsVendorInfo armorVendorInfo;
unsigned char reagPrices[N_REAG_VENDORS][REAG_MAX];

#define WV_SHOPNAME 0
#define WV_VENDORNAME 6
#define WV_ITEMDESC 12
#define WV_NOTENOUGH 26
#define WV_FINECHOICE 27

#define AV_SHOPNAME 0
#define AV_VENDORNAME 5
#define AV_ITEMDESC 11
#define AV_NOTENOUGH 17
#define AV_FINECHOICE 18

#define WV_VERYGOOD 0
#define WV_WEHAVE 1
#define WV_YOURINTEREST 2
#define WV_HOWMANY 4
#define WV_WHATWILL 7
#define WV_YOUSELL 8
#define WV_WELCOME 23
#define WV_SPACER 24
#define WV_BUYORSELL 25
#define WV_BYE 26

int armsVendorInfoRead(ArmsVendorInfo *info, int nprices, FILE *f);

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

    weaponVendorText = u4read_stringtable(avatar, 78883, 28);

    fseek(avatar, 80181, SEEK_SET);
    if (!armsVendorInfoRead(&weaponVendorInfo, WEAP_MAX, avatar))
        return 0;

    weaponVendorText2 = u4read_stringtable(avatar, 80282, 27);

    armorVendorText = u4read_stringtable(avatar, 80803, 19);

    fseek(avatar, 81471, SEEK_SET);
    if (!armsVendorInfoRead(&armorVendorInfo, ARMR_MAX, avatar))
        return 0;

    armorVendorText2 = u4read_stringtable(avatar, 81540, 27);

    u4fclose(avatar);

    return 1;
}

char *vendorGetIntro(Conversation *cnv) {
    char *intro;

    switch (cnv->talker->npcType) {
    case NPC_VENDOR_WEAPONS:
        c->statsItem = STATS_WEAPONS;
        statsUpdate();
        intro = concat(weaponVendorText2[WV_WELCOME],
                       weaponVendorText[WV_SHOPNAME + cnv->talker->vendorIndex],
                       weaponVendorText2[WV_SPACER],
                       weaponVendorText[WV_VENDORNAME + cnv->talker->vendorIndex],
                       weaponVendorText2[WV_BUYORSELL],
                       NULL);
        cnv->state = CONV_BUYSELL;
        break;

    case NPC_VENDOR_ARMOR:
        c->statsItem = STATS_ARMOR;
        statsUpdate();
        intro = concat(armorVendorText2[WV_WELCOME],
                       armorVendorText[AV_SHOPNAME + cnv->talker->vendorIndex],
                       armorVendorText2[WV_SPACER],
                       armorVendorText[AV_VENDORNAME + cnv->talker->vendorIndex],
                       armorVendorText2[WV_BUYORSELL],
                       NULL);
        cnv->state = CONV_BUYSELL;
        break;

    case NPC_VENDOR_FOOD:
        intro = strdup("I am a food vendor!\n");
        cnv->state = CONV_DONE;
        break;

    case NPC_VENDOR_TAVERN:
        intro = strdup("I am a tavern keeper!\n");
        cnv->state = CONV_DONE;
        break;

    case NPC_VENDOR_REAGENTS:
        intro = strdup("I am a reagent vendor!\n");
        cnv->state = CONV_DONE;
        break;

    case NPC_VENDOR_HEALER:
        intro = strdup("I am a healer!\n");
        cnv->state = CONV_DONE;
        break;

    case NPC_VENDOR_INN:
        intro = strdup("I am a inn keeper!\n");
        cnv->state = CONV_DONE;
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

    case CONV_BUY:
    case CONV_SELL:
        switch (cnv->talker->npcType) {
        case NPC_VENDOR_WEAPONS:
            prompt = strdup(weaponVendorText2[WV_YOURINTEREST]);
            break;
        case NPC_VENDOR_ARMOR:
            prompt = strdup(armorVendorText2[WV_YOURINTEREST]);
            break;
        default:
            assert(0);
        }
        break;
        
    case CONV_QUANTITY:
        switch (cnv->talker->npcType) {
        case NPC_VENDOR_WEAPONS:
            prompt = strdup(weaponVendorText2[WV_HOWMANY]);
            break;
        case NPC_VENDOR_ARMOR:
            prompt = strdup(armorVendorText2[WV_HOWMANY]);
            break;
        default:
            assert(0);
        }
        break;

    default:
        assert(0);
    }

    return prompt;
}

char *vendorGetBuySellResponse(Conversation *cnv, const char *response) {
    char *reply;
    int i;
    char **vendorText;
    const ArmsVendorInfo *info;
    
    if (cnv->talker->npcType == NPC_VENDOR_WEAPONS) {
        vendorText = weaponVendorText2;
        info = &weaponVendorInfo;
    }
    else {
        vendorText = armorVendorText2;
        info = &armorVendorInfo;
    }

    if (tolower(response[0]) == 'b') {
        reply = concat(vendorText[WV_VERYGOOD], vendorText[WV_WEHAVE], NULL);
        for (i = 0; i < ARMS_VENDOR_INVENTORY_SIZE; i++) {
            char *newreply, buffer[17];
            if (info->vendorInventory[cnv->talker->vendorIndex][i] != 0) {
                const char *name;

                if (cnv->talker->npcType == NPC_VENDOR_WEAPONS)
                    name = getWeaponName(info->vendorInventory[cnv->talker->vendorIndex][i]);
                else
                    name = getArmorName(info->vendorInventory[cnv->talker->vendorIndex][i]);

                snprintf(buffer, sizeof(buffer), "%c-%s\n", 'A' + info->vendorInventory[cnv->talker->vendorIndex][i], name);
                newreply = concat(reply, buffer, NULL);
                free(reply);
                reply = newreply;
            }
        }
        cnv->state = CONV_BUY;
    }    

    else /* tolower(response[0]) == 's' */ {

        reply = concat(vendorText[WV_WHATWILL], vendorText[WV_YOUSELL], NULL);
        cnv->state = CONV_SELL;
    }

    return reply;
}

char *vendorGetBuyResponse(Conversation *cnv, const char *response) {
    char *reply;
    int i;
    const ArmsVendorInfo *info;

    cnv->item = -1;

    if (cnv->talker->npcType == NPC_VENDOR_WEAPONS) {
        info = &weaponVendorInfo;
    }
    else {
        info = &armorVendorInfo;
    }

    for (i = 0; i < ARMS_VENDOR_INVENTORY_SIZE; i++) {
        if (info->vendorInventory[cnv->talker->vendorIndex][i] != 0 &&
            (tolower(response[0]) - 'a') == info->vendorInventory[cnv->talker->vendorIndex][i])
            cnv->item = info->vendorInventory[cnv->talker->vendorIndex][i];
    }

    if (cnv->item == -1)
        reply = strdup("");
    else {
        switch (cnv->talker->npcType) {
        case NPC_VENDOR_WEAPONS:
            reply = strdup(weaponVendorText[WV_ITEMDESC + cnv->item - 1]);
            break;
        case NPC_VENDOR_ARMOR:
            reply = strdup(armorVendorText[AV_ITEMDESC + cnv->item - 1]);
            break;
        default:
            assert(0);              /* shouldn't happen */
        }
        cnv->state = CONV_QUANTITY;
    }

    return reply;
}

char *vendorGetQuantityResponse(Conversation *cnv, const char *response) {
    char *reply;
    int totalprice, success;
    const ArmsVendorInfo *info;

    if (cnv->talker->npcType == NPC_VENDOR_WEAPONS) {
        info = &weaponVendorInfo;
    }
    else {
        info = &armorVendorInfo;
    }

    cnv->quant = (int) strtol(response, NULL, 10);

    totalprice = cnv->quant * info->prices[cnv->item];
    if (totalprice <= c->saveGame->gold) {
        success = 1;
        c->saveGame->gold -= totalprice;
    } else
        success = 0;

    switch (cnv->talker->npcType) {

    case NPC_VENDOR_WEAPONS:
        if (success) {
            reply = concat(weaponVendorText[WV_VENDORNAME + cnv->talker->vendorIndex], weaponVendorText[WV_FINECHOICE], NULL);
            c->saveGame->weapons[cnv->item] += cnv->quant;
            statsUpdate();
        } else
            reply = strdup(weaponVendorText[WV_NOTENOUGH]);
            
        cnv->state = CONV_DONE;
        break;

    case NPC_VENDOR_ARMOR:
        if (success) {
            reply = concat(armorVendorText[AV_VENDORNAME + cnv->talker->vendorIndex], armorVendorText[AV_FINECHOICE], NULL);
            c->saveGame->armor[cnv->item] += cnv->quant;
            statsUpdate();
        } else
            reply = strdup(weaponVendorText[AV_NOTENOUGH]);
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
