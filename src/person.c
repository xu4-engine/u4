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

#define N_WEAPON_VENDORS 6
#define WEAPON_VENDOR_INVENTORY_SIZE 4

typedef struct WeaponVendorInfo {
    WeaponType vendorInventory[N_WEAPON_VENDORS][WEAPON_VENDOR_INVENTORY_SIZE];
    unsigned short prices[WEAP_MAX];
} WeaponVendorInfo;

char **hawkwindText;
char **lbKeywords;
char **lbText;
char **weaponVendorText;
char **weaponVendorText2;
WeaponVendorInfo weaponVendorInfo;

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

#define WV_VERYGOOD 0
#define WV_WEHAVE 1
#define WV_WELCOME 17
#define WV_SPACER 18
#define WV_BUYORSELL 19

void personGetIntroduction(Conversation *cnv, char **intro);
void personGetResponse(Conversation *cnv, const char *inquiry, char **reply);
void personGetLBResponse(const Person *p, const char *inquiry, char **reply, int *state);
void personGetHWResponse(const Person *p, const char *inquiry, char **reply, int *state);
void personGetQuestionResponse(Conversation *cnv, const char *response, char **reply);
void personGetQuestion(const Person *p, char **question);
void personGetBuySellResponse(Conversation *cnv, const char *response, char **reply);
int weaponVendorInfoRead(WeaponVendorInfo *info, FILE *f);

/**
 * Loads in conversation data for special cases and vendors from
 * avatar.exe.
 */
int personInit() {
    FILE *avatar;

    avatar = u4fopen("avatar.exe");
    if (!avatar)
        return 0;

    lbKeywords = u4read_stringtable(avatar, 87581, 24);
    lbText = u4read_stringtable(avatar, 87754, 24);

    hawkwindText = u4read_stringtable(avatar, 74729, 53);

    weaponVendorText = u4read_stringtable(avatar, 78883, 28);

    fseek(avatar, 80181, SEEK_SET);
    if (!weaponVendorInfoRead(&weaponVendorInfo, avatar))
        return 0;

    weaponVendorText2 = u4read_stringtable(avatar, 80282, 20);

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
    const char *hwFmt = "%s%s%s%s";
    const char *fmt = "You meet\n%s\n\n%s says: I am %s\n\n%s";
    char *prompt;

    switch (cnv->talker->npcType) {

    case NPC_EMPTY:
        *intro = strdup("Funny, no response!");
        cnv->state = CONV_DONE;
        break;

    case NPC_VENDOR_WEAPONS:
        *intro = malloc(strlen(weaponVendorText2[WV_WELCOME]) + strlen(weaponVendorText[WV_SHOPNAME + cnv->talker->vendorIndex]) + 
                        strlen(weaponVendorText2[WV_SPACER]) + strlen(weaponVendorText[WV_VENDORNAME + cnv->talker->vendorIndex]) + 
                        strlen(weaponVendorText2[WV_BUYORSELL]) + 1);
        strcpy(*intro, weaponVendorText2[WV_WELCOME]);
        strcat(*intro, weaponVendorText[WV_SHOPNAME + cnv->talker->vendorIndex]);
        strcat(*intro, weaponVendorText2[WV_SPACER]);
        strcat(*intro, weaponVendorText[WV_VENDORNAME + cnv->talker->vendorIndex]);
        strcat(*intro, weaponVendorText2[WV_BUYORSELL]);
        cnv->state = CONV_BUYSELL;
        break;

    case NPC_VENDOR_ARMOR:
        *intro = strdup("I am an armor vendor!\n");
        cnv->state = CONV_DONE;
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
        *intro = malloc(strlen(hwFmt) - 8 + strlen(hawkwindText[HW_WELCOME]) + strlen(c->saveGame->players[0].name) + 
                        strlen(hawkwindText[HW_GREETING1]) + strlen(hawkwindText[HW_GREETING2]) + 1);
        sprintf(*intro, hwFmt, hawkwindText[HW_WELCOME], c->saveGame->players[0].name, hawkwindText[HW_GREETING1], hawkwindText[HW_GREETING2]);
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
            *prompt = strdup("Your\nInterest?");
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

    *question = malloc(strlen(p->question) + strlen(prompt) + 1);

    strcpy(*question, p->question);
    strcat(*question, prompt);
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
        else /* virtueLevel >= 99 */ {
            *reply = malloc(strlen(hawkwindText[4 * 8 + virtue]) + strlen(hawkwindText[HW_GOTOSHRINE]) + 1);
            strcpy(*reply, hawkwindText[4 * 8 + virtue]);
            strcat(*reply, hawkwindText[HW_GOTOSHRINE]);
        }
        return;
    }

    *reply = strdup(hawkwindText[HW_DEFAULT]);
}

void personGetBuySellResponse(Conversation *cnv, const char *response, char **reply) {
    const char *fmt = "%s%s%c-%s\n%c-%s\n%c-%s\n%c-%s\n";

    if (tolower(response[0]) == 'b') {
        *reply = malloc(strlen(fmt) + strlen(weaponVendorText2[WV_VERYGOOD]) + strlen(weaponVendorText2[WV_WEHAVE]) +
                        strlen(getWeaponName(weaponVendorInfo.vendorInventory[0][0])) +
                        strlen(getWeaponName(weaponVendorInfo.vendorInventory[0][1])) +
                        strlen(getWeaponName(weaponVendorInfo.vendorInventory[0][2])) +
                        strlen(getWeaponName(weaponVendorInfo.vendorInventory[0][3])));

        sprintf(*reply, fmt, weaponVendorText2[WV_VERYGOOD], weaponVendorText2[WV_WEHAVE], 
                'A' + weaponVendorInfo.vendorInventory[0][0], getWeaponName(weaponVendorInfo.vendorInventory[0][0]),
                'A' + weaponVendorInfo.vendorInventory[0][1], getWeaponName(weaponVendorInfo.vendorInventory[0][1]),
                'A' + weaponVendorInfo.vendorInventory[0][2], getWeaponName(weaponVendorInfo.vendorInventory[0][2]),
                'A' + weaponVendorInfo.vendorInventory[0][3], getWeaponName(weaponVendorInfo.vendorInventory[0][3]));

        cnv->state = CONV_BUY;
    }
}

int weaponVendorInfoRead(WeaponVendorInfo *info, FILE *f) {
    int i, j;
    char c;

    for (i = 0; i < N_WEAPON_VENDORS; i++) {
        for (j = 0; j < WEAPON_VENDOR_INVENTORY_SIZE; j++) {
            if (!readChar(&c, f))
                return 0;
            info->vendorInventory[i][j] = c;
        }
    }

    for (i = 0; i < WEAP_MAX; i++) {
        if (!readShort(&(info->prices[i]), f))
            return 0;
    }
    
    return 1;
}
