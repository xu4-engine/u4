/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "u4.h"
#include "context.h"
#include "savegame.h"
#include "person.h"
#include "u4file.h"
#include "names.h"

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

int personIsLordBritish(const Person *p);
int personIsHawkwind(const Person *p);
int personGetLBIntroduction(const Person *p, char **intro);
int personGetHWIntroduction(const Person *p, char **intro);
int personGetLBResponse(const Person *p, const char *inquiry, char **reply, int *askq);
int personGetHWResponse(const Person *p, const char *inquiry, char **reply, int *askq);

int personInit() {
    FILE *avatar;

    avatar = u4fopen("avatar.exe");
    if (!avatar)
        return 0;

    lbKeywords = u4read_stringtable(avatar, 87581, 24);
    lbText = u4read_stringtable(avatar, 87754, 24);

    hawkwindText = u4read_stringtable(avatar, 74729, 53);

    u4fclose(avatar);

    return 1;
}


/**
 * Get the introductory description and dialog shown when a
 * conversation is started.
 */
int personGetIntroduction(const Person *p, char **intro) {
    const char *fmt = "You meet\n%s\n\n%s says: I am %s\n\n%s\n";
    char *prompt;

    if (personIsLordBritish(p))
        return personGetLBIntroduction(p, intro);
    else if (personIsHawkwind(p))
        return personGetHWIntroduction(p, intro);
        
    personGetPrompt(p, &prompt);
    *intro = malloc(strlen(fmt) - 8 + strlen(p->description) + strlen(p->pronoun) + strlen(p->name) + strlen(prompt) + 1);

    sprintf(*intro, fmt, p->description, p->pronoun, p->name, prompt);
    if (isupper((*intro)[9]))
        (*intro)[9] = tolower((*intro)[9]);
    free(prompt);

    return 0;
}

/**
 * Get the prompt shown after each reply.
 */
int personGetPrompt(const Person *p, char **prompt) {
    if (personIsLordBritish(p))
        *prompt = strdup("What else?\n");
    else if (personIsHawkwind(p))
        *prompt = strdup(hawkwindText[HW_PROMPT]);
    else
        *prompt = strdup("Your Interest:\n");

    return 0;
}

/**
 * Get the reply for an inquiry.
 */
int personGetResponse(const Person *p, const char *inquiry, char **reply, int *askq) {

    /* check if it's Lord British */
    if (personIsLordBritish(p))
        return personGetLBResponse(p, inquiry, reply, askq);

    /* check if it's Hawkwind */
    else if (personIsHawkwind(p))
        return personGetHWResponse(p, inquiry, reply, askq);
        

    if (inquiry[0] == '\0' ||
        strcasecmp(inquiry, "bye") == 0) {
        *reply = strdup("Bye.");
        *askq = 0;
        return 1;
    }

    else if (strncasecmp(inquiry, "look", 4) == 0) {
        *reply = malloc(strlen(p->pronoun) + 7 + strlen(p->description) + 1);
        sprintf(*reply, "%s says: %s", p->pronoun, p->description);
        *askq = 0;
        return 0;
    }

    else if (strncasecmp(inquiry, "name", 4) == 0) {
        *reply = malloc(strlen(p->pronoun) + 12 + strlen(p->name) + 1);
        sprintf(*reply, "%s says: I am %s", p->pronoun, p->name);
        *askq = 0;
        return 0;
    }

    else if (strncasecmp(inquiry, "job", 4) == 0) {
        *reply = strdup(p->job);
        *askq = (p->questionTrigger == QTRIGGER_JOB);
        return 0;
    }

    else if (strncasecmp(inquiry, "heal", 4) == 0) {
        *reply = strdup(p->health);
        *askq = (p->questionTrigger == QTRIGGER_HEALTH);
        return 0;
    }

    else if (strncasecmp(inquiry, p->keyword1, 4) == 0) {
        *reply = strdup(p->response1);
        *askq = (p->questionTrigger == QTRIGGER_KEYWORD1);
        return 0;
    }

    else if (strncasecmp(inquiry, p->keyword2, 4) == 0) {
        *reply = strdup(p->response1);
        *askq = (p->questionTrigger == QTRIGGER_KEYWORD2);
        return 0;
    }

    else {
        *reply = strdup("That I cannot\nhelp thee with.");
        *askq = 0;
        return 0;
    }
}

int personGetQuestion(const Person *p, char **question) {
    const char *prompt = "\n\nYou say: ";

    *question = malloc(strlen(p->question) + strlen(prompt) + 1);

    strcpy(*question, p->question);
    strcat(*question, prompt);

    return 0;
}

int personGetQuestionResponse(const Person *p, const char *response, char **reply, int *askq) {

    if (response[0] == 'y' || response[0] == 'Y') {
        *reply = strdup(p->yesresp);
        *askq = 0;
        return 0;
    }

    else if (response[0] == 'n' || response[0] == 'N') {
        *reply = strdup(p->noresp);
        *askq = 0;
        return 0;
    }

    else {
        *reply = strdup("That I cannot\nhelp thee with.");
        *askq = 0;
        return 0;
    }

}

/**
 * Determine if the given person is Lord British.  This allows LB to
 * be special cased.
 */
int personIsLordBritish(const Person *p) {
    return (p->tile0 == 94 && p->startx == 19 && p->starty == 7);
}

/**
 * Determine if the given person is Hawkwind.  This allows Hawkwind to
 * be special cased.
 */
int personIsHawkwind(const Person *p) {
    return (p->name == NULL && p->tile0 == 82 && p->startx == 9 && p->starty == 27);
}

int personGetLBIntroduction(const Person *p, char **intro) {
    const char *fmt = "Lord British\nsays:  Welcome\n%s and thy\nworthy\nAdventurers!\nWhat would thou\nask of me?\n";

    *intro = malloc(strlen(fmt) - 2 + strlen(c->saveGame->players[0].name) + 1);
    sprintf(*intro, fmt, c->saveGame->players[0].name);

    return 0;
}

int personGetHWIntroduction(const Person *p, char **intro) {
    const char *fmt = "%s%s%s%s";

    *intro = malloc(strlen(fmt) - 8 + strlen(hawkwindText[HW_WELCOME]) + strlen(c->saveGame->players[0].name) + 
                                             strlen(hawkwindText[HW_GREETING1]) + strlen(hawkwindText[HW_GREETING2]) + 1);
    sprintf(*intro, fmt, hawkwindText[HW_WELCOME], c->saveGame->players[0].name, hawkwindText[HW_GREETING1], hawkwindText[HW_GREETING2]);

    return 0;
}

int personGetLBResponse(const Person *p, const char *inquiry, char **reply, int *askq) {
    int i;

    if (inquiry[0] == '\0' ||
        strcasecmp(inquiry, "bye") == 0) {
        *reply = strdup("Lord British\nsays: Fare thee\nwell my friends!");
        *askq = 0;
        return 1;
    }

    else if (strncasecmp(inquiry, "heal", 4) == 0) {
        /* FIXME: should ask/heal party */
        *reply = strdup("He says: I am\nwell, thank ye.");
        *askq = 1;
        return 0;
    }

    else {
        for (i = 0; i < 24; i++) {
            if (strncasecmp(inquiry, lbKeywords[i], 4) == 0) {
                *reply = strdup(lbText[i]);
                *askq = 0;
                return 0;
            }
        }
        *reply = strdup("He says: I\ncannot help thee\nwith that.");
        *askq = 0;
        return 0;
    }
}

int personGetHWResponse(const Person *p, const char *inquiry, char **reply, int *askq) {
    int v;
    int virtue = -1, virtueLevel = -1;

    if (inquiry[0] == '\0' ||
        strcasecmp(inquiry, "bye") == 0) {
        *reply = strdup(hawkwindText[HW_BYE]);
        *askq = 0;
        return 1;
    }
        
    /* check if asking about a virtue */
    for (v = VIRT_HONESTY; v <= VIRT_HUMILITY; v++) {
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
        *askq = 0;
        return 0;
    }

    *reply = strdup(hawkwindText[HW_DEFAULT]);
    *askq = 0;
    return 0;

    return 0;
}
