/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
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

char **hawkwindText;
char **lbKeywords;
char **lbText;
char **weaponVendorText;
char **weaponVendorText2;
char **armorVendorText;
char **armorVendorText2;
ArmsVendorInfo weaponVendorInfo;
ArmsVendorInfo armorVendorInfo;
unsigned char reagPrices[N_REAG_VENDORS][REAG_MAX];

#define HW_WELCOME 43
#define HW_GREETING1 44
#define HW_GREETING2 45
#define HW_PROMPT 46
#define HW_DEFAULT 49
#define HW_ALREADYAVATAR 50
#define HW_GOTOSHRINE 51
#define HW_BYE 52

#define WV_SHOPNAME 0
#define WV_VENDORNAME 6

#define AV_SHOPNAME 0
#define AV_VENDORNAME 5

#define WV_VERYGOOD 0
#define WV_WEHAVE 1
#define WV_YOURINTEREST 2
#define WV_WHATWILL 7
#define WV_YOUSELL 8
#define WV_WELCOME 23
#define WV_SPACER 24
#define WV_BUYORSELL 25
#define WV_BYE 26

void personGetIntroduction(Conversation *cnv, char **intro);
void personGetResponse(Conversation *cnv, const char *inquiry, char **reply);
void personGetLBResponse(const Person *p, const char *inquiry, char **reply, int *state);
void personGetHWResponse(const Person *p, const char *inquiry, char **reply, int *state);
void personGetQuestionResponse(Conversation *cnv, const char *response, char **reply);
void personGetQuestion(const Person *p, char **question);
void personGetBuySellResponse(Conversation *cnv, const char *response, char **reply);
int armsVendorInfoRead(ArmsVendorInfo *info, int nprices, FILE *f);
char *concat(const char *str, ...);

/**
 * Loads in conversation data for special cases and vendors from
 * avatar.exe.
 */
int personInit() {
    int i, j;
    FILE *avatar;

    avatar = u4fopen("avatar.exe");
    if (!avatar)
        return 0;

    lbKeywords = u4read_stringtable(avatar, 87581, 24);
    lbText = u4read_stringtable(avatar, 87754, 24);

    hawkwindText = u4read_stringtable(avatar, 74729, 53);

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

void personGetConversationText(Conversation *cnv, const char *inquiry, char **response) {
    switch (cnv->state) {
    case CONV_INTRO:
        personGetIntroduction(cnv, response);
        return;

    case CONV_TALK:
        personGetResponse(cnv, inquiry, response);
        return;

    case CONV_ASK:
        personGetQuestionResponse(cnv, inquiry, response);
        break;
    
    case CONV_BUYSELL:
        personGetBuySellResponse(cnv, inquiry, response);
        break;

    case CONV_DONE:
    default:
        assert(0);              /* shouldn't happen */
    }
}

/**
 * Get the introductory description and dialog shown when a
 * conversation is started.
 */
void personGetIntroduction(Conversation *cnv, char **intro) {
    const char *lbFmt = "Lord British\nsays:  Welcome\n%s and thy\nworthy\nAdventurers!\nWhat would thou\nask of me?\n";
    const char *fmt = "You meet\n%s\n\n%s says: I am %s\n\n%s";
    char *prompt;

    switch (cnv->talker->npcType) {

    case NPC_EMPTY:
        *intro = strdup("Funny, no response!");
        cnv->state = CONV_DONE;
        break;

    case NPC_VENDOR_WEAPONS:
        c->statsItem = STATS_WEAPONS;
        statsUpdate();
        *intro = concat(weaponVendorText2[WV_WELCOME],
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
        *intro = concat(armorVendorText2[WV_WELCOME],
                        armorVendorText[AV_SHOPNAME + cnv->talker->vendorIndex],
                        armorVendorText2[WV_SPACER],
                        armorVendorText[AV_VENDORNAME + cnv->talker->vendorIndex],
                        armorVendorText2[WV_BUYORSELL],
                        NULL);
        cnv->state = CONV_BUYSELL;
        break;

    case NPC_VENDOR_FOOD:
        *intro = strdup("I am a food vendor!\n");
        cnv->state = CONV_DONE;
        break;

    case NPC_VENDOR_TAVERN:
        *intro = strdup("I am a tavern keeper!\n");
        cnv->state = CONV_DONE;
        break;

    case NPC_VENDOR_REAGENTS:
        *intro = strdup("I am a reagent vendor!\n");
        cnv->state = CONV_DONE;
        break;

    case NPC_VENDOR_HEALER:
        *intro = strdup("I am a healer!\n");
        cnv->state = CONV_DONE;
        break;

    case NPC_VENDOR_INN:
        *intro = strdup("I am a inn keeper!\n");
        cnv->state = CONV_DONE;
        break;

    case NPC_VENDOR_GUILD:
        *intro = strdup("I am a guild vendor!\n");
        cnv->state = CONV_DONE;
        break;

    case NPC_VENDOR_STABLE:
        *intro = strdup("I am a horse vendor!\n");
        cnv->state = CONV_DONE;
        break;

    case NPC_TALKER:
    case NPC_TALKER_BEGGAR:
    case NPC_TALKER_GUARD:
    case NPC_TALKER_COMPANION:
        personGetPrompt(cnv, &prompt);
        *intro = malloc(strlen(fmt) - 8 + strlen(cnv->talker->description) + strlen(cnv->talker->pronoun) + strlen(cnv->talker->name) + strlen(prompt) + 1);

        sprintf(*intro, fmt, cnv->talker->description, cnv->talker->pronoun, cnv->talker->name, prompt);
        if (isupper((*intro)[9]))
            (*intro)[9] = tolower((*intro)[9]);
        free(prompt);
        cnv->state = CONV_TALK;
        break;

    case NPC_LORD_BRITISH:
        *intro = malloc(strlen(lbFmt) - 2 + strlen(c->saveGame->players[0].name) + 1);
        sprintf(*intro, lbFmt, c->saveGame->players[0].name);
        cnv->state = CONV_TALK;
        break;

    case NPC_HAWKWIND:
        *intro = concat(hawkwindText[HW_WELCOME], 
                        c->saveGame->players[0].name,
                        hawkwindText[HW_GREETING1], 
                        hawkwindText[HW_GREETING2],
                        NULL);
        cnv->state = CONV_TALK;
        break;

    default:
        assert(0);
    }
}

/**
 * Get the prompt shown after each reply.
 */
void personGetPrompt(const Conversation *cnv, char **prompt) {

    switch (cnv->state) {

    case CONV_ASK:
        personGetQuestion(cnv->talker, prompt);
        break;

    case CONV_BUYSELL:
        switch (cnv->talker->npcType) {
        case NPC_VENDOR_WEAPONS:
            *prompt = strdup(weaponVendorText2[WV_YOURINTEREST]);
            break;
        case NPC_VENDOR_ARMOR:
            *prompt = strdup(armorVendorText2[WV_YOURINTEREST]);
            break;
        default:
            assert(0);
        }
        break;
        
    default:
        switch (cnv->talker->npcType) {
        case NPC_LORD_BRITISH:
            *prompt = strdup("What else?\n");
            break;
        case NPC_HAWKWIND:
            *prompt = strdup(hawkwindText[HW_PROMPT]);
            break;
        default:
            *prompt = strdup("\nYour Interest:\n");
            break;
        }
        break;
    }
}

/**
 * Get the reply for an inquiry.
 */
void personGetResponse(Conversation *cnv, const char *inquiry, char **reply) {

    switch (cnv->talker->npcType) {

    case NPC_LORD_BRITISH:
        personGetLBResponse(cnv->talker, inquiry, reply, &cnv->state);
        break;
        
    case NPC_HAWKWIND:
         personGetHWResponse(cnv->talker, inquiry, reply, &cnv->state);
         break;

    case NPC_EMPTY:
        *reply = strdup("");
        cnv->state = CONV_DONE;
        break;

    case NPC_TALKER:
    case NPC_TALKER_COMPANION:
    case NPC_TALKER_BEGGAR:
        if (inquiry[0] == '\0' ||
            strcasecmp(inquiry, "bye") == 0) {
            *reply = strdup("Bye.");
            cnv->state = CONV_DONE;
        }

        else if (strncasecmp(inquiry, "look", 4) == 0) {
            *reply = malloc(strlen(cnv->talker->pronoun) + 7 + strlen(cnv->talker->description) + 1);
            sprintf(*reply, "%s says: %s", cnv->talker->pronoun, cnv->talker->description);
        }

        else if (strncasecmp(inquiry, "name", 4) == 0) {
            *reply = malloc(strlen(cnv->talker->pronoun) + 12 + strlen(cnv->talker->name) + 1);
            sprintf(*reply, "%s says: I am %s", cnv->talker->pronoun, cnv->talker->name);
        }

        else if (strncasecmp(inquiry, "job", 4) == 0) {
            *reply = strdup(cnv->talker->job);
            if (cnv->talker->questionTrigger == QTRIGGER_JOB)
                cnv->state = CONV_ASK;
        }

        else if (strncasecmp(inquiry, "heal", 4) == 0) {
            *reply = strdup(cnv->talker->health);
            if (cnv->talker->questionTrigger == QTRIGGER_HEALTH)
                cnv->state = CONV_ASK;
        }

        else if (strncasecmp(inquiry, cnv->talker->keyword1, 4) == 0) {
            *reply = strdup(cnv->talker->response1);
            if (cnv->talker->questionTrigger == QTRIGGER_KEYWORD1)
                cnv->state = CONV_ASK;
        }

        else if (strncasecmp(inquiry, cnv->talker->keyword2, 4) == 0) {
            *reply = strdup(cnv->talker->response1);
            if (cnv->talker->questionTrigger == QTRIGGER_KEYWORD2)
                cnv->state = CONV_ASK;
        }

        else
            *reply = strdup("That I cannot\nhelp thee with.");
        break;

    default:
        assert(0);
    }
}

void personGetQuestion(const Person *p, char **question) {
    const char *prompt = "\n\nYou say: ";

    *question = concat(p->question, prompt, NULL);
}

void personGetQuestionResponse(Conversation *cnv, const char *response, char **reply) {
    cnv->state = CONV_TALK;

    if (response[0] == 'y' || response[0] == 'Y')
        *reply = strdup(cnv->talker->yesresp);

    else if (response[0] == 'n' || response[0] == 'N')
        *reply = strdup(cnv->talker->noresp);

    else
        *reply = strdup("That I cannot\nhelp thee with.");
}

void personGetLBResponse(const Person *p, const char *inquiry, char **reply, int *state) {
    int i;

    if (inquiry[0] == '\0' ||
        strcasecmp(inquiry, "bye") == 0) {
        *reply = strdup("Lord British\nsays: Fare thee\nwell my friends!");
        *state = CONV_DONE;
    }

    else if (strncasecmp(inquiry, "heal", 4) == 0) {
        /* FIXME: should ask/heal party */
        *reply = strdup("He says: I am\nwell, thank ye.");
        *state = CONV_ASK;
    }

    else {
        for (i = 0; i < 24; i++) {
            if (strncasecmp(inquiry, lbKeywords[i], 4) == 0) {
                *reply = strdup(lbText[i]);
                return;
            }
        }
        *reply = strdup("He says: I\ncannot help thee\nwith that.");
    }
}

void personGetHWResponse(const Person *p, const char *inquiry, char **reply, int *state) {
    int v;
    int virtue = -1, virtueLevel = -1;

    if (inquiry[0] == '\0' ||
        strcasecmp(inquiry, "bye") == 0) {
        *reply = strdup(hawkwindText[HW_BYE]);
        *state = CONV_DONE;
        return;
    }
        
    /* check if asking about a virtue */
    for (v = 0; v < VIRT_MAX; v++) {
        if (strncasecmp(inquiry, getVirtueName(v), 4) == 0) {
            virtue = v;
            virtueLevel = c->saveGame->karma[v];
        }
    }
    if (virtue != -1) {
        if (virtueLevel == 0)
            *reply = strdup(hawkwindText[HW_ALREADYAVATAR]);
        else if (virtueLevel < 20)
            *reply = strdup(hawkwindText[0 * 8 + virtue]);
        else if (virtueLevel < 40)
            *reply = strdup(hawkwindText[1 * 8 + virtue]);
        else if (virtueLevel < 60)
            *reply = strdup(hawkwindText[2 * 8 + virtue]);
        else if (virtueLevel < 99)
            *reply = strdup(hawkwindText[3 * 8 + virtue]);
        else /* virtueLevel >= 99 */
            *reply = concat(hawkwindText[4 * 8 + virtue], hawkwindText[HW_GOTOSHRINE], NULL);

        return;
    }

    *reply = strdup(hawkwindText[HW_DEFAULT]);
}

void personGetBuySellResponse(Conversation *cnv, const char *response, char **reply) {
    int i;

    if (tolower(response[0]) == 'b') {
        switch (cnv->talker->npcType) {
        case NPC_VENDOR_WEAPONS:
            *reply = concat(weaponVendorText2[WV_VERYGOOD], weaponVendorText2[WV_WEHAVE], NULL);
            for (i = 0; i < ARMS_VENDOR_INVENTORY_SIZE; i++) {
                char *newreply, buffer[17];
                if (weaponVendorInfo.vendorInventory[0][i] != 0) {
                    snprintf(buffer, sizeof(buffer), "%c-%s\n", 
                            'A' + weaponVendorInfo.vendorInventory[0][i],
                            getWeaponName(weaponVendorInfo.vendorInventory[0][i]));
                    newreply = concat(*reply, buffer, NULL);
                    free(*reply);
                    *reply = newreply;
                }
            }
            break;

        case NPC_VENDOR_ARMOR:
            *reply = concat(armorVendorText2[WV_VERYGOOD], armorVendorText2[WV_WEHAVE], NULL);
            for (i = 0; i < ARMS_VENDOR_INVENTORY_SIZE; i++) {
                char *newreply, buffer[17];
                if (armorVendorInfo.vendorInventory[0][i] != 0) {
                    snprintf(buffer, sizeof(buffer), "%c-%s\n", 
                            'A' + armorVendorInfo.vendorInventory[0][i],
                            getArmorName(armorVendorInfo.vendorInventory[0][i]));
                    newreply = concat(*reply, buffer, NULL);
                    free(*reply);
                    *reply = newreply;
                }
            }
            break;
            
        default:
            assert(0);          /* shouldn't happen */
        }

        /*cnv->state = CONV_BUY;*/
        cnv->state = CONV_DONE;
        c->statsItem = STATS_PARTY_OVERVIEW;
        statsUpdate();
    }    

    else /* tolower(response[0]) == 's' */ {

        switch (cnv->talker->npcType) {

        case NPC_VENDOR_WEAPONS:
            *reply = concat(weaponVendorText2[WV_WHATWILL], weaponVendorText2[WV_YOUSELL], NULL);
            break;

        case NPC_VENDOR_ARMOR:
            *reply = concat(armorVendorText2[WV_WHATWILL], armorVendorText2[WV_YOUSELL], NULL);
            break;

        default:
            assert(0);          /* shouldn't happen */
        }

        /*cnv->state = CONV_SELL;*/
        cnv->state = CONV_DONE;
        c->statsItem = STATS_PARTY_OVERVIEW;
        statsUpdate();
    }
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

/**
 * Utility function to concatenate a NULL terminated list of strings
 * together into a newly allocated string.  Derived from glibc
 * documention, "Copying and Concatenation" section.
 */
char *concat(const char *str, ...) {
    va_list ap;
    unsigned int allocated = 1;
    char *result, *p;

    result = malloc(allocated);
    if (allocated) {
        const char *s;
        char *newp;

        va_start(ap, str);

        p = result;
        for (s = str; s; s = va_arg(ap, const char *)) {
            unsigned int len = strlen(s);

            /* resize the allocated memory if necessary.  */
            if (p + len + 1 > result + allocated) {
                allocated = (allocated + len) * 2;
                newp = realloc(result, allocated);
                if (!newp) {
                    free(result);
                    return NULL;
                }
                p = newp + (p - result);
                result = newp;
            }

            memcpy(p, s, len);
            p += len;
        }

        /* terminate the result */
        *p++ = '\0';

        /* resize memory to the optimal size.  */
        newp = realloc(result, p - result);
        if (newp)
            result = newp;

        va_end (ap);
    }

    return result;
}
