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
char *lordBritishGetQuestionResponse(Conversation *cnv, const char *inquiry);
char *lordBritishGetPrompt(const Conversation *cnv);
char *hawkwindGetIntro(Conversation *cnv);
char *hawkwindGetResponse(Conversation *cnv, const char *inquiry);
char *hawkwindGetPrompt(const Conversation *cnv);

const PersonType personType[NPC_MAX] = {
    { &emptyGetIntro, NULL, NULL, NULL, NULL, NULL, NULL, NULL }, /* NPC_EMPTY */
    { &talkerGetIntro, &talkerGetResponse, &talkerGetQuestionResponse, NULL, NULL, NULL, NULL, &talkerGetPrompt }, /* NPC_TALKER */
    { &talkerGetIntro, &beggarGetResponse, &talkerGetQuestionResponse, NULL, NULL, NULL, &beggarGetQuantityResponse, &beggarGetPrompt }, /* NPC_TALKER_BEGGAR */
    { &talkerGetIntro, &talkerGetResponse, &talkerGetQuestionResponse, NULL, NULL, NULL, NULL, &talkerGetPrompt }, /* NPC_TALKER_GUARD */
    { &talkerGetIntro, &talkerGetResponse, &talkerGetQuestionResponse, NULL, NULL, NULL, NULL, &talkerGetPrompt }, /* NPC_TALKER_COMPANION */
    { &vendorGetIntro, NULL, NULL, &vendorGetBuySellResponse, &vendorGetBuyResponse, NULL, &vendorGetQuantityResponse, &vendorGetPrompt }, /* NPC_VENDOR_WEAPONS */
    { &vendorGetIntro, NULL, NULL, &vendorGetBuySellResponse, &vendorGetBuyResponse, NULL, &vendorGetQuantityResponse, &vendorGetPrompt }, /* NPC_VENDOR_ARMOR */
    { &vendorGetIntro, NULL, NULL, NULL, NULL, NULL, NULL, &vendorGetPrompt }, /* NPC_VENDOR_FOOD */
    { &vendorGetIntro, NULL, NULL, NULL, NULL, NULL, NULL, &vendorGetPrompt }, /* NPC_VENDOR_TAVERN */
    { &vendorGetIntro, NULL, NULL, NULL, NULL, NULL, NULL, &vendorGetPrompt }, /* NPC_VENDOR_REAGENTS */
    { &vendorGetIntro, NULL, NULL, NULL, NULL, NULL, NULL, &vendorGetPrompt }, /* NPC_VENDOR_HEALER */
    { &vendorGetIntro, NULL, NULL, NULL, NULL, NULL, NULL, &vendorGetPrompt }, /* NPC_VENDOR_INN */
    { &vendorGetIntro, NULL, NULL, NULL, NULL, NULL, NULL, &vendorGetPrompt }, /* NPC_VENDOR_GUILD */
    { &vendorGetIntro, NULL, NULL, NULL, NULL, NULL, NULL, &vendorGetPrompt }, /* NPC_VENDOR_STABLE */
    { &lordBritishGetIntro, &lordBritishGetResponse, &lordBritishGetQuestionResponse, NULL, NULL, NULL, NULL, &lordBritishGetPrompt }, /* NPC_LORD_BRITISH */
    { &hawkwindGetIntro, &hawkwindGetResponse, NULL, NULL, NULL, NULL, NULL, &hawkwindGetPrompt } /* NPC_HAWKWIND */
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

void personGetConversationText(Conversation *cnv, const char *inquiry, char **response) {
    switch (cnv->state) {
    case CONV_INTRO:
        if (personType[cnv->talker->npcType].getIntro)
            *response = (*personType[cnv->talker->npcType].getIntro)(cnv);
        else
            *response = strdup("BUG!!");
        return;

    case CONV_TALK:
        if (personType[cnv->talker->npcType].getResponse)
            *response = (*personType[cnv->talker->npcType].getResponse)(cnv, inquiry);
        else
            *response = strdup("BUG!!");
        return;

    case CONV_ASK:
        if (personType[cnv->talker->npcType].getQuestionResponse)
            *response = (*personType[cnv->talker->npcType].getQuestionResponse)(cnv, inquiry);
        else
            *response = strdup("BUG!!");
        break;
    
    case CONV_BUYSELL:
        if (personType[cnv->talker->npcType].getBuySellResponse)
            *response = (*personType[cnv->talker->npcType].getBuySellResponse)(cnv, inquiry);
        else
            *response = strdup("BUG!!");
        break;

    case CONV_BUY:
        if (personType[cnv->talker->npcType].getBuyResponse)
            *response = (*personType[cnv->talker->npcType].getBuyResponse)(cnv, inquiry);
        else
            *response = strdup("BUG!!");
        break;

    case CONV_SELL:
        if (personType[cnv->talker->npcType].getSellResponse)
            *response = (*personType[cnv->talker->npcType].getSellResponse)(cnv, inquiry);
        else
            *response = strdup("BUG!!");
        break;

    case CONV_QUANTITY:
        if (personType[cnv->talker->npcType].getQuantityResponse)
            *response = (*personType[cnv->talker->npcType].getQuantityResponse)(cnv, inquiry);
        else
            *response = strdup("BUG!!");
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

char *emptyGetIntro(Conversation *cnv) {
    cnv->state = CONV_DONE;
    return strdup("Funny, no response!\n");
}

char *talkerGetIntro(Conversation *cnv) {
    const char *fmt = "You meet\n%s\n\n%s says: I am %s\n\n%s";
    char *prompt, *intro;

    /* DEBUG */
    printf("person %s has PersonQuestionType = %d\n", cnv->talker->name, cnv->talker->questionType);
    /* DEBUG */

    personGetPrompt(cnv, &prompt);
    intro = malloc(strlen(fmt) - 8 + strlen(cnv->talker->description) + strlen(cnv->talker->pronoun) + strlen(cnv->talker->name) + strlen(prompt) + 1);

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
        reply = malloc(strlen(cnv->talker->pronoun) + 7 + strlen(cnv->talker->description) + 1);
        sprintf(reply, "%s says: %s", cnv->talker->pronoun, cnv->talker->description);
    }

    else if (strncasecmp(inquiry, "name", 4) == 0) {
        reply = malloc(strlen(cnv->talker->pronoun) + 12 + strlen(cnv->talker->name) + 1);
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

    else
        reply = strdup("That I cannot\nhelp thee with.");

    return reply;
}

char *beggarGetResponse(Conversation *cnv, const char *inquiry) {
    char *reply;

    if (strncasecmp(inquiry, "give", 4) == 0) {
        reply = strdup("");
        cnv->state = CONV_QUANTITY;
    }
    else
        reply = talkerGetResponse(cnv, inquiry);

    return reply;
}

char *talkerGetQuestionResponse(Conversation *cnv, const char *answer) {
    char *reply;

    cnv->state = CONV_TALK;

    if (answer[0] == 'y' || answer[0] == 'Y')
        reply = strdup(cnv->talker->yesresp);

    else if (answer[0] == 'n' || answer[0] == 'N')
        reply = strdup(cnv->talker->noresp);

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

    if (cnv->state == CONV_QUANTITY)
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

char *lordBritishGetIntro(Conversation *cnv) {
    int i;
    const char *lbFmt = "Lord British\nsays:  Welcome\n%s and thy\nworthy\nAdventurers!\nWhat would thou\nask of me?\n";
    char *intro;

    musicLordBritish();

    for (i = 0; i < c->saveGame->members; i++) {
        if (playerGetRealLevel(&c->saveGame->players[i]) < playerGetMaxLevel(&c->saveGame->players[i])) {
            playerAdvanceLevel(&c->saveGame->players[i]);
            /*screenMessage("%s Thou art now Level %d\n", c->saveGame->players[i].name, playerGetRealLevel(&c->saveGame->players[i]));*/
            statsUpdate();
        }
    }

    if (c->saveGame->lbintro) {
        intro = malloc(strlen(lbFmt) - 2 + strlen(c->saveGame->players[0].name) + 1);
        sprintf(intro, lbFmt, c->saveGame->players[0].name);
    } else {
        intro = strdup("Lord British rises and says: At long last!\n thou hast come!  We have waited such a long, long time...\n");
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
        /* FIXME: should ask/heal party */
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

char *lordBritishGetQuestionResponse(Conversation *cnv, const char *inquiry) {
    cnv->state = CONV_TALK;

    return strdup("foo\n");
}

char *lordBritishGetPrompt(const Conversation *cnv) {
    char *prompt;

    if (cnv->state == CONV_ASK) {
        /* FIXME: art thou well? */
        personGetQuestion(cnv->talker, &prompt);
    }
    else
        prompt = strdup("What else?\n");

    return prompt;
}

char *hawkwindGetIntro(Conversation *cnv) {
    char *intro;

    playerAdjustKarma(c->saveGame, KA_HAWKWIND);

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
        if (strncasecmp(inquiry, getVirtueName(v), 4) == 0) {
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
