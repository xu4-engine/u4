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
#include "vendor.h"
#include "music.h"
#include "player.h"
#include "map.h"
#include "game.h"

char **hawkwindText;
char **lbKeywords;
char **lbText;

#define HW_WELCOME 43
#define HW_GREETING1 44
#define HW_GREETING2 45
#define HW_PROMPT 46
#define HW_DEFAULT 49
#define HW_ALREADYAVATAR 50
#define HW_GOTOSHRINE 51
#define HW_BYE 52

void personGetQuestion(const Person *p, char **question);

char *emptyGetIntro(Conversation *cnv);
char *talkerGetIntro(Conversation *cnv);
char *talkerGetResponse(Conversation *cnv, const char *inquiry);
char *beggarGetResponse(Conversation *cnv, const char *inquiry);
char *talkerGetQuestionResponse(Conversation *cnv, const char *inquiry);
char *talkerGetPrompt(const Conversation *cnv);
char *beggarGetPrompt(const Conversation *cnv);
char *beggarGetQuantityResponse(Conversation *cnv, const char *response);
char *lordBritishGetIntro(Conversation *cnv);
char *lordBritishGetResponse(Conversation *cnv, const char *inquiry);
char *lordBritishGetQuestionResponse(Conversation *cnv, const char *answer);
char *lordBritishGetPrompt(const Conversation *cnv);
char *hawkwindGetIntro(Conversation *cnv);
char *hawkwindGetResponse(Conversation *cnv, const char *inquiry);
char *hawkwindGetPrompt(const Conversation *cnv);

const PersonType personType[NPC_MAX] = {
    { &emptyGetIntro, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }, /* NPC_EMPTY */
    { &talkerGetIntro, &talkerGetResponse, &talkerGetQuestionResponse, NULL, NULL, NULL, NULL, NULL, &talkerGetPrompt, NULL }, /* NPC_TALKER */
    { &talkerGetIntro, &beggarGetResponse, &talkerGetQuestionResponse, NULL, NULL, NULL, &beggarGetQuantityResponse, NULL, &beggarGetPrompt, NULL }, /* NPC_TALKER_BEGGAR */
    { &talkerGetIntro, &talkerGetResponse, &talkerGetQuestionResponse, NULL, NULL, NULL, NULL, NULL, &talkerGetPrompt, NULL }, /* NPC_TALKER_GUARD */
    { &talkerGetIntro, &talkerGetResponse, &talkerGetQuestionResponse, NULL, NULL, NULL, NULL, NULL, &talkerGetPrompt, NULL }, /* NPC_TALKER_COMPANION */
    { &vendorGetIntro, NULL, NULL, &vendorGetVendorQuestionResponse, &vendorGetBuyItemResponse, &vendorGetSellItemResponse, &vendorGetBuyQuantityResponse, &vendorGetSellQuantityResponse, &vendorGetPrompt, "bs\033" }, /* NPC_VENDOR_WEAPONS */
    { &vendorGetIntro, NULL, NULL, &vendorGetVendorQuestionResponse, &vendorGetBuyItemResponse, &vendorGetSellItemResponse, &vendorGetBuyQuantityResponse, &vendorGetSellQuantityResponse, &vendorGetPrompt, "bs\033" }, /* NPC_VENDOR_ARMOR */
    { &vendorGetIntro, NULL, NULL, &vendorGetVendorQuestionResponse, NULL, NULL, &vendorGetBuyQuantityResponse, NULL, &vendorGetPrompt, "yn\033" }, /* NPC_VENDOR_FOOD */
    { &vendorGetIntro, NULL, NULL, &vendorGetVendorQuestionResponse, NULL, NULL, NULL, NULL, &vendorGetPrompt, "af\033" }, /* NPC_VENDOR_TAVERN */
    { &vendorGetIntro, NULL, NULL, &vendorGetVendorQuestionResponse, NULL, NULL, NULL, NULL, &vendorGetPrompt, "yn\033" }, /* NPC_VENDOR_REAGENTS */
    { &vendorGetIntro, NULL, NULL, &vendorGetVendorQuestionResponse, NULL, NULL, NULL, NULL, &vendorGetPrompt, "yn\033" }, /* NPC_VENDOR_HEALER */
    { &vendorGetIntro, NULL, NULL, &vendorGetVendorQuestionResponse, NULL, NULL, NULL, NULL, &vendorGetPrompt, "yn\033" }, /* NPC_VENDOR_INN */
    { &vendorGetIntro, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &vendorGetPrompt, NULL }, /* NPC_VENDOR_GUILD */
    { &vendorGetIntro, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &vendorGetPrompt, NULL }, /* NPC_VENDOR_STABLE */
    { &lordBritishGetIntro, &lordBritishGetResponse, &lordBritishGetQuestionResponse, NULL, NULL, NULL, NULL, NULL, &lordBritishGetPrompt, NULL }, /* NPC_LORD_BRITISH */
    { &hawkwindGetIntro, &hawkwindGetResponse, NULL, NULL, NULL, NULL, NULL, NULL, &hawkwindGetPrompt, NULL } /* NPC_HAWKWIND */
};

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

    u4fclose(avatar);

    return vendorInit();
}

int personIsJoinable(const Person *p) {
    int i;

    if (!p || !p->name)
        return 0;
    for (i = 1; i < 8; i++) {
        if (strcmp(c->saveGame->players[i].name, p->name) == 0)
            return 1;
    }
    return 0;
}

int personIsJoined(const Person *p) {
    int i;

    if (!p || !p->name)
        return 0;
    for (i = 1; i < c->saveGame->members; i++) {
        if (strcmp(c->saveGame->players[i].name, p->name) == 0)
            return 1;
    }
    return 0;
}

void personGetConversationText(Conversation *cnv, const char *inquiry, char **response) {
    switch (cnv->state) {
    case CONV_INTRO:
        if (personType[cnv->talker->npcType].getIntro)
            *response = (*personType[cnv->talker->npcType].getIntro)(cnv);
        else
            *response = strdup("BUG: no intro handler for npc");
        return;

    case CONV_TALK:
        if (personType[cnv->talker->npcType].getResponse)
            *response = (*personType[cnv->talker->npcType].getResponse)(cnv, inquiry);
        else
            *response = strdup("BUG: no response handler for npc");
        return;

    case CONV_ASK:
        if (personType[cnv->talker->npcType].getQuestionResponse)
            *response = (*personType[cnv->talker->npcType].getQuestionResponse)(cnv, inquiry);
        else
            *response = strdup("BUG: no question response handler for npc");
        break;
    
    case CONV_VENDORQUESTION:
        if (personType[cnv->talker->npcType].getVendorQuestionResponse)
            *response = (*personType[cnv->talker->npcType].getVendorQuestionResponse)(cnv, inquiry);
        else
            *response = strdup("BUG: no vendor question handler for npc");
        break;

    case CONV_BUY_ITEM:
        if (personType[cnv->talker->npcType].getBuyItemResponse)
            *response = (*personType[cnv->talker->npcType].getBuyItemResponse)(cnv, inquiry);
        else
            *response = strdup("BUG: no buy item response handler for npc");
        break;

    case CONV_SELL_ITEM:
        if (personType[cnv->talker->npcType].getSellItemResponse)
            *response = (*personType[cnv->talker->npcType].getSellItemResponse)(cnv, inquiry);
        else
            *response = strdup("BUG: no sell item response handler for npc");
        break;

    case CONV_BUY_QUANTITY:
        if (personType[cnv->talker->npcType].getBuyQuantityResponse)
            *response = (*personType[cnv->talker->npcType].getBuyQuantityResponse)(cnv, inquiry);
        else
            *response = strdup("BUG: no buy quantity response handler for npc");
        break;

    case CONV_SELL_QUANTITY:
        if (personType[cnv->talker->npcType].getSellQuantityResponse)
            *response = (*personType[cnv->talker->npcType].getSellQuantityResponse)(cnv, inquiry);
        else
            *response = strdup("BUG: no sell quantity response handler for npc");
        break;

    case CONV_DONE:
    default:
        assert(0);              /* shouldn't happen */
    }
}

/**
 * Get the prompt shown after each reply.
 */
void personGetPrompt(const Conversation *cnv, char **prompt) {

    if (personType[cnv->talker->npcType].getPrompt)
        *prompt = (*personType[cnv->talker->npcType].getPrompt)(cnv);
    else
        *prompt = strdup("BUG!!");
}

ConversationInputType personGetInputRequired(const struct _Conversation *cnv) {
    switch (cnv->state) {
    case CONV_TALK:
    case CONV_ASK:
    case CONV_BUY_QUANTITY:
    case CONV_SELL_QUANTITY:
        return CONVINPUT_STRING;
    
    case CONV_VENDORQUESTION:
    case CONV_BUY_ITEM:
    case CONV_SELL_ITEM:
        return CONVINPUT_CHARACTER;
        
    case CONV_DONE:
        return CONVINPUT_NONE;
    }

    assert(0);                  /* shouldn't happen */
    return CONVINPUT_NONE;
}

/**
 * Returns the valid keyboard choices for a given conversation.
 */
const char *personGetChoices(const struct _Conversation *cnv) {
    switch (cnv->state) {
    case CONV_VENDORQUESTION:
        return personType[cnv->talker->npcType].vendorQuestionChoices;

    case CONV_BUY_ITEM:
    case CONV_SELL_ITEM:
        return "bcdefghijklmnopqrstuvwxyz\033";

    default:
        assert(0);              /* shouldn't happen */
    }

    return NULL;
}

char *emptyGetIntro(Conversation *cnv) {
    cnv->state = CONV_DONE;
    return strdup("Funny, no response!\n");
}

char *talkerGetIntro(Conversation *cnv) {
    const char *fmt = "You meet\n%s\n\n%s says: I am %s\n\n%s";
    char *prompt, *intro;

    personGetPrompt(cnv, &prompt);
    intro = (char *) malloc(strlen(fmt) - 8 + 
                            strlen(cnv->talker->description) + 
                            strlen(cnv->talker->pronoun) + 
                            strlen(cnv->talker->name) + 
                            strlen(prompt) + 1);

    sprintf(intro, fmt, cnv->talker->description, cnv->talker->pronoun, cnv->talker->name, prompt);
    if (isupper(intro[9]))
        intro[9] = tolower(intro[9]);
    free(prompt);
    cnv->state = CONV_TALK;

    return intro;
}

char *talkerGetResponse(Conversation *cnv, const char *inquiry) {
    char *reply;

    if (inquiry[0] == '\0' ||
        strcasecmp(inquiry, "bye") == 0) {
        reply = strdup("Bye.");
        cnv->state = CONV_DONE;
    }

    else if (strncasecmp(inquiry, "look", 4) == 0) {
        reply = (char *) malloc(strlen(cnv->talker->pronoun) + 7 + strlen(cnv->talker->description) + 1);
        sprintf(reply, "%s says: %s", cnv->talker->pronoun, cnv->talker->description);
    }

    else if (strncasecmp(inquiry, "name", 4) == 0) {
        reply = (char *) malloc(strlen(cnv->talker->pronoun) + 12 + strlen(cnv->talker->name) + 1);
        sprintf(reply, "%s says: I am %s", cnv->talker->pronoun, cnv->talker->name);
    }

    else if (strncasecmp(inquiry, "job", 4) == 0) {
        reply = strdup(cnv->talker->job);
        if (cnv->talker->questionTrigger == QTRIGGER_JOB)
            cnv->state = CONV_ASK;
    }

    else if (strncasecmp(inquiry, "heal", 4) == 0) {
        reply = strdup(cnv->talker->health);
        if (cnv->talker->questionTrigger == QTRIGGER_HEALTH)
            cnv->state = CONV_ASK;
    }

    else if (strncasecmp(inquiry, cnv->talker->keyword1, 4) == 0) {
        reply = strdup(cnv->talker->response1);
        if (cnv->talker->questionTrigger == QTRIGGER_KEYWORD1)
            cnv->state = CONV_ASK;
    }

    else if (strncasecmp(inquiry, cnv->talker->keyword2, 4) == 0) {
        reply = strdup(cnv->talker->response2);
        if (cnv->talker->questionTrigger == QTRIGGER_KEYWORD2)
            cnv->state = CONV_ASK;
    }

    else if (strncasecmp(inquiry, "give", 4) == 0) {
        reply = concat(cnv->talker->pronoun, 
                       " says: I do not need thy gold.  Keep it!",
                       NULL);
    }

    else if (strncasecmp(inquiry, "join", 4) == 0) {
        if (personIsJoinable(cnv->talker)) {
            playerJoin(c->saveGame, cnv->talker->name);
            reply = strdup("I am honored to join thee!");
            statsUpdate();
            mapRemovePerson(c->map, cnv->talker);
            cnv->state = CONV_DONE;
        } else 
            reply = concat(cnv->talker->pronoun, 
                           " says: I cannot join thee.",
                           NULL);
    }

    else
        reply = strdup("That I cannot\nhelp thee with.");

    return reply;
}

char *beggarGetResponse(Conversation *cnv, const char *inquiry) {
    char *reply;

    if (strncasecmp(inquiry, "give", 4) == 0) {
        reply = strdup("");
        cnv->state = CONV_BUY_QUANTITY;
    }
    else
        reply = talkerGetResponse(cnv, inquiry);

    return reply;
}

char *talkerGetQuestionResponse(Conversation *cnv, const char *answer) {
    char *reply;

    cnv->state = CONV_TALK;

    if (tolower(answer[0]) == 'y') {
        reply = strdup(cnv->talker->yesresp);
        if (cnv->talker->questionType == QUESTION_HUMILITY_TEST)
            gameLostEighth(playerAdjustKarma(c->saveGame, KA_BRAGGED));
    }

    else if (tolower(answer[0]) == 'n') {
        reply = strdup(cnv->talker->noresp);
        if (cnv->talker->questionType == QUESTION_HUMILITY_TEST)
            gameLostEighth(playerAdjustKarma(c->saveGame, KA_HUMBLE));
    }

    else
        reply = strdup("That I cannot\nhelp thee with.");

    return reply;
}

char *talkerGetPrompt(const Conversation *cnv) {
    char *prompt;

    if (cnv->state == CONV_ASK)
        personGetQuestion(cnv->talker, &prompt);
    else
        prompt = strdup("\nYour Interest:\n");

    return prompt;
}

char *beggarGetPrompt(const Conversation *cnv) {
    char *prompt;

    if (cnv->state == CONV_BUY_QUANTITY)
        prompt = strdup("How much? ");
    else
        prompt = talkerGetPrompt(cnv);

    return prompt;
}

char *beggarGetQuantityResponse(Conversation *cnv, const char *response) {
    char *reply;

    cnv->quant = (int) strtol(response, NULL, 10);
    cnv->state = CONV_TALK;

    if (cnv->quant > 0) {
        if (playerDonate(c->saveGame, cnv->quant)) {
            reply = concat(cnv->talker->pronoun, 
                           " says: Oh Thank thee! I shall never forget thy kindness!",
                           NULL);
            statsUpdate();
        }
        
        else
            reply = strdup("Thou hast not that much gold!\n");
    }

    return reply;
}

char *lordBritishCheckLevels(Conversation *cnv) {
    int i;
    char buffer[10];
    char *msg, *tmp;

    msg = strdup("");
    for (i = 0; i < c->saveGame->members; i++) {
        if (playerGetRealLevel(&c->saveGame->players[i]) < playerGetMaxLevel(&c->saveGame->players[i])) {
            playerAdvanceLevel(&c->saveGame->players[i]);
            tmp = msg;
            sprintf(buffer, "%d", playerGetRealLevel(&c->saveGame->players[i]));
            msg = concat(tmp, c->saveGame->players[i].name, " Thou art now Level ", buffer, "\n", NULL);
            statsUpdate();
        }
    }

    return msg;
}

char *lordBritishGetIntro(Conversation *cnv) {
    char *leveled, *intro;

    musicLordBritish();

    leveled = lordBritishCheckLevels(cnv);

    if (c->saveGame->lbintro)
        intro = concat(leveled, 
                       "Lord British\nsays:  Welcome\n",
                       c->saveGame->players[0].name,
                       "and thy\nworthy\nAdventurers!\nWhat would thou\nask of me?\n",
                       NULL);
    else {
        intro = concat(leveled, 
                       "Lord British rises and says: At long last!\n thou hast come!  We have waited such a long, long time...\n",
                       NULL);
        c->saveGame->lbintro = 1;
    }
    cnv->state = CONV_TALK;

    return intro;
}

char *lordBritishGetResponse(Conversation *cnv, const char *inquiry) {
    char *reply;
    int i;

    if (inquiry[0] == '\0' ||
        strcasecmp(inquiry, "bye") == 0) {
        reply = strdup("Lord British\nsays: Fare thee\nwell my friends!");
        cnv->state = CONV_DONE;
        musicPlay();
    }

    else if (strncasecmp(inquiry, "heal", 4) == 0) {
        reply = strdup("He says: I am\nwell, thank ye.");
        cnv->state = CONV_ASK;
    }

    else {
        for (i = 0; i < 24; i++) {
            if (strncasecmp(inquiry, lbKeywords[i], 4) == 0)
                return strdup(lbText[i]);
        }
        reply = strdup("He says: I\ncannot help thee\nwith that.");
    }

    return reply;
}

char *lordBritishGetQuestionResponse(Conversation *cnv, const char *answer) {
    int i;
    char *reply;
    cnv->state = CONV_TALK;


    if (tolower(answer[0]) == 'y') {
        reply = strdup("\nHe says: That is good.\n");
    }

    else if (tolower(answer[0]) == 'n') {
        reply = strdup("\nHe says: Let me heal thy wounds!\n");
        /* FIXME: special effect here */
        for (i = 0; i < c->saveGame->members; i++) {
            if (c->saveGame->players[i].status != STAT_DEAD)
                c->saveGame->players[i].hp = c->saveGame->players[i].hpMax;
        }
        statsUpdate();
    }

    else
        reply = strdup("That I cannot\nhelp thee with.");

    return reply;
}

char *lordBritishGetPrompt(const Conversation *cnv) {
    char *prompt;

    if (cnv->state == CONV_ASK)
        prompt = strdup("\n\nHe asks: Art thou well?");
    else
        prompt = strdup("What else?\n");

    return prompt;
}

char *hawkwindGetIntro(Conversation *cnv) {
    char *intro;

    gameLostEighth(playerAdjustKarma(c->saveGame, KA_HAWKWIND));

    intro = concat(hawkwindText[HW_WELCOME], 
                   c->saveGame->players[0].name,
                   hawkwindText[HW_GREETING1], 
                   hawkwindText[HW_GREETING2],
                   NULL);
    cnv->state = CONV_TALK;

    return intro;
}

char *hawkwindGetResponse(Conversation *cnv, const char *inquiry) {
    char *reply;
    int v;
    int virtue = -1, virtueLevel = -1;

    if (inquiry[0] == '\0' ||
        strcasecmp(inquiry, "bye") == 0) {
        reply = strdup(hawkwindText[HW_BYE]);
        cnv->state = CONV_DONE;
        return reply;
    }
        
    /* check if asking about a virtue */
    for (v = 0; v < VIRT_MAX; v++) {
        if (strncasecmp(inquiry, getVirtueName((Virtue) v), 4) == 0) {
            virtue = v;
            virtueLevel = c->saveGame->karma[v];
        }
    }
    if (virtue != -1) {
        if (virtueLevel == 0)
            reply = strdup(hawkwindText[HW_ALREADYAVATAR]);
        else if (virtueLevel < 20)
            reply = strdup(hawkwindText[0 * 8 + virtue]);
        else if (virtueLevel < 40)
            reply = strdup(hawkwindText[1 * 8 + virtue]);
        else if (virtueLevel < 60)
            reply = strdup(hawkwindText[2 * 8 + virtue]);
        else if (virtueLevel < 99)
            reply = strdup(hawkwindText[3 * 8 + virtue]);
        else /* virtueLevel >= 99 */
            reply = concat(hawkwindText[4 * 8 + virtue], hawkwindText[HW_GOTOSHRINE], NULL);

        return reply;
    }

    return strdup(hawkwindText[HW_DEFAULT]);
}

char *hawkwindGetPrompt(const Conversation *cnv) {
    return strdup(hawkwindText[HW_PROMPT]);
}

void personGetQuestion(const Person *p, char **question) {
    const char *prompt = "\n\nYou say: ";

    *question = concat(p->question, prompt, NULL);
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

    result = (char *) malloc(allocated);
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
                newp = (char *) realloc(result, allocated);
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
        newp = (char *) realloc(result, p - result);
        if (newp)
            result = newp;

        va_end (ap);
    }

    return result;
}
