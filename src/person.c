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
    const char *fmt = "%s\n\nYou say: ";

    *question = malloc(strlen(fmt) - 2 + strlen(p->question) + 1);

    sprintf(*question, fmt, p->question);

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
    struct {
        const char *q;
        const char *a;
    } lbQa[] = {
        { "look", 
          "Thou see the\n"
          "King with the\n"
          "Royal Sceptre."
        },
        { "name",
          "He says:\n"
          "My name is\n"
          "Lord British\n"
          "Sovereign of\n"
          "all Britannia!"
        },
        { "job",
          "He says:\n"
          "I rule all\n"
          "Britannia, and\n"
          "shall do my best\n"
          "to help thee!"
        },
        { "trut",
          "He says:\n"
          "Many truths can\n"
          "be learned at\n"
          "the Lycaeum.  It\n"
          "lies on the\n"
          "northwestern\n"
          "shore of Verity\n"
          "Isle!"
        },          
        { "love",
          "He says:\n"
          "Look for the\n"
          "meaning of Love\n"
          "at Empath Abbey.\n"
          "The Abbey sits\n"
          "on the western\n"
          "edge of the Deep\n"
          "Forest!"
        },
        { "love",
          "He says:\n"
          "Look for the\n"
          "meaning of Love\n"
          "at Empath Abbey.\n"
          "The Abbey sits\n"
          "on the western\n"
          "edge of the Deep\n"
          "Forest!"
        },
        { "cour",
          "He says:\n"
          "Serpent's Castle\n"
          "on the Isle of\n"
          "Deeds is where\n"
          "Courage should\n"
          "be sought!"
        },
        { "hone",
          "He says:\n"
          "The fair towne\n"
          "of Moonglow on\n"
          "Verity Isle is\n"
          "where the virtue\n"
          "of Honesty\n"
          "thrives!"
        },
        { "comp",
          "He says:\n"
          "The bards in the\n"
          "towne of Britain\n"
          "are well versed\n"
          "in the virtue of\n"
          "Compassion!"
        },
        { "valo",
          "He says:\n"
          "Many valiant\n"
          "fighters come\n"
          "from Jhelom\n"
          "in the Valarian\n"
          "Isles!"
        },
        { "just",
          "He says:\n"
          "In the city of\n"
          "Yew, in the Deep\n"
          "Forest, Justice\n"
          "is served!"
        },
        { "sacr",
          "He says:\n"
          "Minoc, towne of\n"
          "self-sacrifice,\n"
          "lies on the\n"
          "eastern shores\n"
          "of Lost Hope\n"
          "Bay!"
        },
        { "hono",
          "He says:\n"
          "The Paladins who\n"
          "strive for Honor\n"
          "are oft seen in\n"
          "Trinsic, north\n"
          "of the Cape of\n"
          "Heroes!"
        },
        { "spir",
          "He says:\n"
          "In Skara Brae\n"
          "the Spiritual\n"
          "path is taught.\n"
          "Find it on an\n"
          "isle near\n"
          "Spiritwood!"
        },
        { "humi",
          "He says:\n"
          "Humility is the\n"
          "foundation of\n"
          "Virtue!  The\n"
          "ruins of proud\n"
          "Magincia are a\n"
          "testimony unto\n"
          "the Virtue of\n"
          "Humility!\n"
          "\n"
          "Find the Ruins\n"
          "of Magincia far\n"
          "off the shores\n"
          "of Britannia,\n"
          "on a small isle\n"
          "in the vast\n"
          "Ocean!"
        },
        { "prid",
          "He says:\n"
          "Of the eight\n"
          "combinations of\n"
          "Truth, Love and\n"
          "Courage, that\n"
          "which contains\n"
          "neither Truth,\n"
          "Love nor Courage\n"
          "is Pride.\n"
          "\n"
          "Pride being not\n"
          "a Virtue must be\n"
          "shunned in favor\n"
          "of Humility, the\n"
          "Virtue which is\n"
          "the antithesis\n"
          "of Pride!"
        },
        { "avat",
          "Lord British\n"
          "says:\n"
          "To be an Avatar\n"
          "is to be the\n"
          "embodiment of\n"
          "the Eight\n"
          "Virtues.\n"
          "\n"
          "It is to live a\n"
          "life constantly\n"
          "and forever in\n"
          "the Quest to\n"
          "better thyself\n"
          "and the world in\n"
          "which we live."
        },
        { "ques",
          "Lord British\n"
          "says:\n"
          "The Quest of\n"
          "the Avatar is\n"
          "to know and\n"
          "become the\n"
          "embodiment of\n"
          "the Eight\n"
          "Virtues of\n"
          "Goodness!\n"
          "It is known that\n"
          "all who take on\n"
          "this Quest must\n"
          "prove themselves\n"
          "by conquering\n"
          "the Abyss and\n"
          "Viewing the\n"
          "Codex of\n"
          "Ultimate Wisdom!"
        },
        { "brit",
          "He says:\n"
          "Even though the\n"
          "Great Evil Lords\n"
          "have been routed\n"
          "evil yet remains\n"
          "in Britannia.\n"
          "\n"
          "If but one soul\n"
          "could complete\n"
          "the Quest of the\n"
          "Avatar, our\n"
          "people would\n"
          "have a new hope,\n"
          "a new goal for\n"
          "life.\n"
          "\n"
          "There would be a\n"
          "shining example\n"
          "that there is\n"
          "more to life\n"
          "than the endless\n"
          "struggle for\n"
          "possessions\n"
          "and gold!"
        },
        { "ankh",
          "He says:\n"
          "The Ankh is the\n"
          "symbol of one\n"
          "who strives for\n"
          "Virtue.  Keep it\n"
          "with thee at all\n"
          "times for by\n"
          "this mark thou\n"
          "shalt be known!"
        },
        { "abys",
          "He says:\n"
          "The Great\n"
          "Stygian Abyss\n"
          "is the darkest\n"
          "pocket of evil\n"
          "remaining in\n"
          "Britannia!\n"
          "\n"
          "It is said that\n"
          "in the deepest\n"
          "recesses of the\n"
          "Abyss is the\n"
          "Chamber of the\n"
          "Codex!\n"
          "\n"
          "It is also said\n"
          "that only one of\n"
          "highest Virtue\n"
          "may enter this\n"
          "Chamber, one\n"
          "such as an\n"
          "Avatar!!!"
        },
        { "mond",
          "He says:\n"
          "Mondain is dead!"
        },
        { "mina",
          "He says:\n"
          "Minax is dead!"
        },
        { "exod",
          "He says:\n"
          "Exodus is dead!"
        },
        { "virt",
          "He says:\n"
          "The Eight\n"
          "Virtues of the\n"
          "Avatar are:\n"
          "Honesty,\n"
          "Compassion,\n"
          "Valor,\n"
          "Justice,\n"
          "Sacrifice,\n"
          "Honor,\n"
          "Spirituality,\n"
          "and Humility!"
        }
    };

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
        for (i = 0; i < sizeof(lbQa) / sizeof(lbQa[0]); i++) {
            if (strncasecmp(inquiry, lbQa[i].q, 4) == 0) {
                *reply = strdup(lbQa[i].a);
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
