/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u4.h"
#include "person.h"

int personGetResponse(const Person *p, const char *inquiry, char **reply, int *askq) {

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
