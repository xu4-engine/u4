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

char **hawkwindText;
char **lbKeywords;
char **lbText;

#define HW_SPEAKONLYWITH 40
#define HW_RETURNWHEN 41
#define HW_ISREVIVED 42
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
char *talkerGetQuestionResponse(Conversation *cnv, const char *inquiry);
char *talkerGetPrompt(const Conversation *cnv);
char *beggarGetQuantityResponse(Conversation *cnv, const char *response);
char *lordBritishGetIntro(Conversation *cnv);
char *lordBritishGetResponse(Conversation *cnv, const char *inquiry);
char *lordBritishGetQuestionResponse(Conversation *cnv, const char *answer);
char *lordBritishGetPrompt(const Conversation *cnv);
char *hawkwindGetIntro(Conversation *cnv);
char *hawkwindGetResponse(Conversation *cnv, const char *inquiry);
char *hawkwindGetPrompt(const Conversation *cnv);

typedef struct _VendorType {
    char *(*getIntro)(struct _Conversation *cnv);
    char *(*getVendorQuestionResponse)(struct _Conversation *cnv, const char *inquiry);
    char *(*getBuyItemResponse)(struct _Conversation *cnv, const char *inquiry);
    char *(*getSellItemResponse)(struct _Conversation *cnv, const char *inquiry);
    char *(*getBuyQuantityResponse)(struct _Conversation *cnv, const char *inquiry);
    char *(*getSellQuantityResponse)(struct _Conversation *cnv, const char *inquiry);
    char *(*getBuyPriceResponse)(struct _Conversation *cnv, const char *inquiry);
    char *(*getContinueQuestionResponse)(struct _Conversation *cnv, const char *answer);
    char *(*getPrompt)(const struct _Conversation *cnv);
    const char *vendorQuestionChoices;
} VendorType;

const VendorType vendorType[] = {
    { &vendorGetIntro, &vendorGetArmsVendorQuestionResponse, &vendorGetArmsBuyItemResponse, &vendorGetSellItemResponse, 
      &vendorGetBuyQuantityResponse, &vendorGetSellQuantityResponse, NULL, 
      &vendorGetContinueQuestionResponse, &vendorGetPrompt, "bs\033" }, /* NPC_VENDOR_WEAPONS */
    { &vendorGetIntro, &vendorGetArmsVendorQuestionResponse, &vendorGetArmsBuyItemResponse, &vendorGetSellItemResponse, 
      &vendorGetBuyQuantityResponse, &vendorGetSellQuantityResponse, NULL, &vendorGetContinueQuestionResponse,
      &vendorGetPrompt, "bs\033" }, /* NPC_VENDOR_ARMOR */
    { &vendorGetIntro, NULL, NULL, NULL, 
      &vendorGetBuyQuantityResponse, NULL, NULL, 
      &vendorGetContinueQuestionResponse, &vendorGetPrompt, NULL }, /* NPC_VENDOR_FOOD */
    { &vendorGetIntro, &vendorGetTavernVendorQuestionResponse, NULL, NULL, 
      &vendorGetBuyQuantityResponse, NULL, NULL,
      &vendorGetContinueQuestionResponse, &vendorGetPrompt, "af\033" }, /* NPC_VENDOR_TAVERN */
    { &vendorGetIntro, NULL, &vendorGetReagentsBuyItemResponse, NULL,
      &vendorGetBuyQuantityResponse, NULL, &vendorGetBuyPriceResponse,
      &vendorGetContinueQuestionResponse, &vendorGetPrompt, NULL }, /* NPC_VENDOR_REAGENTS */
    { &vendorGetIntro, &vendorGetHealerVendorQuestionResponse, &vendorGetHealerBuyItemResponse, NULL,
      NULL, NULL, NULL, 
      &vendorGetContinueQuestionResponse, &vendorGetPrompt, "ny\033" }, /* NPC_VENDOR_HEALER */
    { &vendorGetIntro, NULL, NULL, NULL,
      NULL, NULL, NULL,
      &vendorGetContinueQuestionResponse, &vendorGetPrompt, NULL }, /* NPC_VENDOR_INN */
    { &vendorGetIntro, NULL, NULL, NULL,
      NULL, NULL, NULL, 
      NULL, &vendorGetPrompt, NULL }, /* NPC_VENDOR_GUILD */
    { &vendorGetIntro, NULL, NULL, NULL,
      NULL, NULL, NULL, 
      NULL, &vendorGetPrompt, NULL }, /* NPC_VENDOR_STABLE */
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

int personIsVendor(const Person *person) {
    return 
        person->npcType >= NPC_VENDOR_WEAPONS &&
        person->npcType <= NPC_VENDOR_STABLE;
}

void personGetConversationText(Conversation *cnv, const char *inquiry, char **response) {

    /*
     * a convsation with a vendor
     */
    if (personIsVendor(cnv->talker)) {
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
        default:
            assert(0);          /* shouldn't happen */

        }
    } 

    /*
     * a conversation with a non-vendor
     */
    else {
        switch (cnv->state) {
        case CONV_INTRO:
            if (cnv->talker->npcType == NPC_EMPTY)
                *response = emptyGetIntro(cnv);
            else if (cnv->talker->npcType == NPC_LORD_BRITISH)
                *response = lordBritishGetIntro(cnv);
            else if (cnv->talker->npcType == NPC_HAWKWIND)
                *response = hawkwindGetIntro(cnv);
            else 
                *response = talkerGetIntro(cnv);
            return;

        case CONV_TALK:
            if (cnv->talker->npcType == NPC_LORD_BRITISH)
                *response = lordBritishGetResponse(cnv, inquiry);
            else if (cnv->talker->npcType == NPC_HAWKWIND)
                *response = hawkwindGetResponse(cnv, inquiry);
            else 
                *response = talkerGetResponse(cnv, inquiry);
            return;

        case CONV_ASK:
            assert(cnv->talker->npcType != NPC_HAWKWIND);
            if (cnv->talker->npcType == NPC_LORD_BRITISH)
                *response = lordBritishGetQuestionResponse(cnv, inquiry);
            else 
                *response = talkerGetQuestionResponse(cnv, inquiry);
            return;
    
        case CONV_BUY_QUANTITY:
            assert(cnv->talker->npcType == NPC_TALKER_BEGGAR);
            *response = beggarGetQuantityResponse(cnv, inquiry);
            return;

        default:
            assert(0);          /* shouldn't happen */
        }
    }
}

/**
 * Get the prompt shown after each reply.
 */
void personGetPrompt(const Conversation *cnv, char **prompt) {
    if (cnv->talker->npcType == NPC_LORD_BRITISH)
        *prompt = lordBritishGetPrompt(cnv);
    else if (cnv->talker->npcType == NPC_HAWKWIND)
        *prompt = hawkwindGetPrompt(cnv);
    else if (personIsVendor(cnv->talker))
        *prompt = (*vendorType[cnv->talker->npcType - NPC_VENDOR_WEAPONS].getPrompt)(cnv);
    else 
        *prompt = talkerGetPrompt(cnv);
}

ConversationInputType personGetInputRequired(const struct _Conversation *cnv) {
    switch (cnv->state) {
    case CONV_TALK:
    case CONV_ASK:
    case CONV_BUY_QUANTITY:
    case CONV_SELL_QUANTITY:
    case CONV_BUY_PRICE:
        return CONVINPUT_STRING;
    
    case CONV_VENDORQUESTION:
    case CONV_BUY_ITEM:
    case CONV_SELL_ITEM:
    case CONV_CONTINUEQUESTION:
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
        assert(personIsVendor(cnv->talker));
        return vendorType[cnv->talker->npcType - NPC_VENDOR_WEAPONS].vendorQuestionChoices;

    case CONV_BUY_ITEM:
    case CONV_SELL_ITEM:
        return "abcdefghijklmnopqrstuvwxyz\033";

    case CONV_CONTINUEQUESTION:
        return "ny\033";

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
        if (cnv->talker->npcType == NPC_TALKER_BEGGAR) {
            reply = strdup("");
            cnv->state = CONV_BUY_QUANTITY;
        } else
            reply = concat(cnv->talker->pronoun, 
                           " says: I do not need thy gold.  Keep it!",
                           NULL);
    }

    else if (strncasecmp(inquiry, "join", 4) == 0) {
        Virtue v;
        if (playerCanPersonJoin(c->saveGame, cnv->talker->name, &v)) {
            if (playerJoin(c->saveGame, cnv->talker->name)) {
                reply = strdup("I am honored to join thee!");
                statsUpdate();
                mapRemovePerson(c->map, cnv->talker);
                cnv->state = CONV_DONE;
            } else
                reply = concat("Thou art not ",
                               getVirtueAdjective(v), /* fixme */
                               " enough for me to join thee.\n",
                               NULL);
        } else 
            reply = concat(cnv->talker->pronoun, 
                           " says: I cannot join thee.",
                           NULL);
    }

    else
        reply = strdup("That I cannot\nhelp thee with.");

    return reply;
}

char *talkerGetQuestionResponse(Conversation *cnv, const char *answer) {
    char *reply;

    cnv->state = CONV_TALK;

    if (tolower(answer[0]) == 'y') {
        reply = strdup(cnv->talker->yesresp);
        if (cnv->talker->questionType == QUESTION_HUMILITY_TEST)
            playerAdjustKarma(c->saveGame, KA_BRAGGED);
    }

    else if (tolower(answer[0]) == 'n') {
        reply = strdup(cnv->talker->noresp);
        if (cnv->talker->questionType == QUESTION_HUMILITY_TEST)
            playerAdjustKarma(c->saveGame, KA_HUMBLE);
    }

    else
        reply = strdup("That I cannot\nhelp thee with.");

    return reply;
}

char *talkerGetPrompt(const Conversation *cnv) {
    char *prompt;

    if (cnv->state == CONV_ASK)
        personGetQuestion(cnv->talker, &prompt);
    else if (cnv->state == CONV_BUY_QUANTITY)
        prompt = strdup("How much? ");
    else
        prompt = strdup("\nYour Interest:\n");

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

void lordBritishCheckLevels(Conversation *cnv) {
    int i;

    for (i = 0; i < c->saveGame->members; i++) {
        if (playerGetRealLevel(&c->saveGame->players[i]) < 
            playerGetMaxLevel(&c->saveGame->players[i]))
            playerAdvanceLevel(&c->saveGame->players[i]);
    }
}

char *lordBritishGetIntro(Conversation *cnv) {
    char *intro;

    musicLordBritish();

    lordBritishCheckLevels(cnv);

    if (c->saveGame->lbintro) {
        if (c->saveGame->members == 1)
            intro = concat("Lord British\nsays:  Welcome\n",
                           c->saveGame->players[0].name,
                           "\nWhat would thou\nask of me?\n",
                           NULL);
        else if (c->saveGame->members == 2)
            intro = concat("Lord British\nsays:  Welcome\n",
                           c->saveGame->players[0].name,
                           " and thee also ",
                           c->saveGame->players[1].name,
                           "!\nWhat would thou\nask of me?\n",
                           NULL);
        else
            intro = concat("Lord British\nsays:  Welcome\n",
                           c->saveGame->players[0].name,
                           " and thy\nworthy\nAdventurers!\nWhat would thou\nask of me?\n",
                           NULL);
            
    }
    else {
        intro = concat("Lord British rises and says: At long last!\n thou hast come!  We have waited such a long, long time...\n",
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
            playerHeal(c->saveGame, HT_HEAL, i);
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

    if (c->saveGame->players[0].status == STAT_SLEEPING ||
        c->saveGame->players[0].status == STAT_DEAD) {
        intro = concat(hawkwindText[HW_SPEAKONLYWITH],
                       c->saveGame->players[0].name,
                       hawkwindText[HW_RETURNWHEN],
                       c->saveGame->players[0].name,
                       hawkwindText[HW_ISREVIVED],
                       NULL);
        cnv->state = CONV_DONE;
    }

    else {
        playerAdjustKarma(c->saveGame, KA_HAWKWIND);

        intro = concat(hawkwindText[HW_WELCOME], 
                       c->saveGame->players[0].name,
                       hawkwindText[HW_GREETING1], 
                       hawkwindText[HW_GREETING2],
                       NULL);
        cnv->state = CONV_TALK;
    }

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
