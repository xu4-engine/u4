/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "u4.h"

#include "person.h"

#include "context.h"
#include "debug.h"
#include "io.h"
#include "location.h"
#include "music.h"
#include "names.h"
#include "player.h"
#include "savegame.h"
#include "screen.h"
#include "spell.h"
#include "stats.h"
#include "u4file.h"
#include "vendor.h"

char **hawkwindText;
char **lbKeywords;
char **lbText;

/* Hawkwind text indexes */
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
char *lordBritishGetHelp(const Conversation *cnv);
char *hawkwindGetIntro(Conversation *cnv);
char *hawkwindGetResponse(Conversation *cnv, const char *inquiry);
char *hawkwindGetPrompt(const Conversation *cnv);
int linecount(const char *s, int columnmax);
int chars_needed(const char *s, int columnmax, int linesdesired);


/**
 * Splits a piece of response text into screen-sized chunks.
 */
Reply *replyNew(const char *text) {
    static const char paragraphBreak[] = "\n\n";
    Reply *reply;
    const char *ptr;
    int i;
    int offset;

    reply = (Reply *) malloc(sizeof(Reply));

    /*
     * find the first paragraph break, after skipping over any initial
     * newlines
     */
    offset = strspn(text, paragraphBreak);
    ptr = strstr(text+offset, paragraphBreak);
    if (ptr == NULL)
    {
        /*
         * the text might still be text too large
         * to display; find a new break point!
         */
        ptr = text + offset;
        ptr += chars_needed(text + offset, 16, 11);
    }

    /*
     * don't split up reply if less than a screenful or can't find a
     * paragraph break
     */
    if (linecount(text, 16) < 12) {
        reply->nchunks = 1;
        reply->chunk = (char **) malloc(sizeof(char *) * reply->nchunks);
        reply->chunk[0] = strdup(text);
    }

    /*
     * recursively split up all paragraphs but the first, then prepend
     * the first
     */
    else {
        Reply *tmp = replyNew(ptr);

        /* +1 so that the cursor is a line below the continuous text */
        int len = ptr - text + 1;
        if (*(text+len)!='\n') len--; // If there is no break point, then just display normally

        reply->nchunks = tmp->nchunks + 1;
        reply->chunk = (char **) malloc(sizeof(char *) * reply->nchunks);
        reply->chunk[0] = malloc(len + 1);
        strncpy(reply->chunk[0], text, len);
        reply->chunk[0][len] = '\0';
        for (i = 1; i < reply->nchunks; i++)
            reply->chunk[i] = tmp->chunk[i - 1];
        free(tmp->chunk);
        free(tmp);
    }

    return reply;
}

/**
 * Frees a reply.
 */
void replyDelete(Reply *reply) {
    int i;

    for (i = 0; i < reply->nchunks; i++)
        free(reply->chunk[i]);
    free(reply->chunk);
    free(reply);
}

/**
 * Loads in conversation data for special cases and vendors from
 * avatar.exe.
 */
int personInit() {
    U4FILE *avatar;

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

Reply *personGetConversationText(Conversation *cnv, const char *inquiry) {
    char *text;
    Reply *reply;

    /*
     * a convsation with a vendor
     */
    if (personIsVendor(cnv->talker)) {
        if (cnv->state == CONV_INTRO)
            musicShopping();
        vendorGetConversationText(cnv, inquiry, &text);
        if (cnv->state == CONV_DONE)
            musicPlay();
    }

    /*
     * a conversation with a non-vendor
     */
    else {
        switch (cnv->state) {
        case CONV_INTRO:
            if (cnv->talker->npcType == NPC_EMPTY)
                text = emptyGetIntro(cnv);
            else if (cnv->talker->npcType == NPC_LORD_BRITISH)
                text = lordBritishGetIntro(cnv);
            else if (cnv->talker->npcType == NPC_HAWKWIND)
                text = hawkwindGetIntro(cnv);
            else
                text = talkerGetIntro(cnv);

            break;

        case CONV_TALK:
            if (cnv->talker->npcType == NPC_LORD_BRITISH)
                text = concat("\n", lordBritishGetResponse(cnv, inquiry), NULL);
            else if (cnv->talker->npcType == NPC_HAWKWIND)
                text = hawkwindGetResponse(cnv, inquiry);
            else
                text = concat("\n\n", talkerGetResponse(cnv, inquiry), "\n", NULL);
            break;

        case CONV_ASK:
            ASSERT(cnv->talker->npcType != NPC_HAWKWIND, "invalid state for hawkwind conversation");
            if (cnv->talker->npcType == NPC_LORD_BRITISH)
                text = lordBritishGetQuestionResponse(cnv, inquiry);
            else
                text = concat("\n\n", talkerGetQuestionResponse(cnv, inquiry), "\n", NULL);
            break;

        case CONV_BUY_QUANTITY:
            ASSERT(cnv->talker->npcType == NPC_TALKER_BEGGAR, "invalid npc type: %d", cnv->talker->npcType);
            text = beggarGetQuantityResponse(cnv, inquiry);
            break;

        default:
            ASSERT(0, "invalid state: %d", cnv->state);
        }
    }

    reply = replyNew(text);
    free(text);
    return reply;
}

/**
 * Get the prompt shown after each reply.
 */
char *personGetPrompt(const Conversation *cnv) {
    if (cnv->talker->npcType == NPC_LORD_BRITISH)
        return lordBritishGetPrompt(cnv);
    else if (cnv->talker->npcType == NPC_HAWKWIND)
        return hawkwindGetPrompt(cnv);
    else if (personIsVendor(cnv->talker))
        return vendorGetPrompt(cnv);
    else
        return talkerGetPrompt(cnv);
}

ConversationInputType personGetInputRequired(const struct _Conversation *cnv) {
    switch (cnv->state) {
    case CONV_TALK:
    case CONV_ASK:
    case CONV_BUY_QUANTITY:
    case CONV_SELL_QUANTITY:
    case CONV_BUY_PRICE:
    case CONV_TOPIC:
        return CONVINPUT_STRING;

    case CONV_VENDORQUESTION:
    case CONV_BUY_ITEM:
    case CONV_SELL_ITEM:
    case CONV_CONFIRMATION:
    case CONV_CONTINUEQUESTION:
    case CONV_PLAYER:
        return CONVINPUT_CHARACTER;

    case CONV_DONE:
        return CONVINPUT_NONE;
    }

    ASSERT(0, "invalid state: %d", cnv->state);
    return CONVINPUT_NONE;
}

/**
 * Returns the valid keyboard choices for a given conversation.
 */
const char *personGetChoices(const struct _Conversation *cnv) {
    switch (cnv->state) {
    case CONV_VENDORQUESTION:
        ASSERT(personIsVendor(cnv->talker), "person must be vendor");
        return vendorGetVendorQuestionChoices(cnv);

    case CONV_BUY_ITEM:
    case CONV_SELL_ITEM:
        return "abcdefghijklmnopqrstuvwxyz\033";

    case CONV_CONFIRMATION:
    case CONV_CONTINUEQUESTION:
        return "ny\033";

    case CONV_PLAYER:
        return "012345678\033";

    default:
        ASSERT(0, "invalid state: %d", cnv->state);
    }

    return NULL;
}

char *emptyGetIntro(Conversation *cnv) {
    cnv->state = CONV_DONE;
    return strdup("Funny, no response!\n");
}

char *talkerGetIntro(Conversation *cnv) {
    const char *fmt1 = "\nYou meet\n%s\n\n%s says: I am %s\n%s";
    const char *fmt2 = "\nYou meet\n%s\n%s";
    char *prompt, *intro;

    prompt = personGetPrompt(cnv);
    intro = (char *) malloc(strlen(fmt1) - 8 +
                            strlen(cnv->talker->description) +
                            strlen(cnv->talker->pronoun) +
                            strlen(cnv->talker->name) +
                            strlen(prompt) + 1);

    // As far as I can tell, about 50% of the time they tell you their name
    if (rand()%2==1)
        sprintf(intro, fmt1, cnv->talker->description, cnv->talker->pronoun, cnv->talker->name, prompt);
    else sprintf(intro, fmt2, cnv->talker->description, prompt);

    if (isupper(intro[9]))
        intro[9] = tolower(intro[9]);
    free(prompt);
    cnv->state = CONV_TALK;

    return intro;
}

char *talkerGetResponse(Conversation *cnv, const char *inquiry) {
    char *reply;
    int testLen = 4;
    
    if (strlen(inquiry) < 4)
        testLen = strlen(inquiry);

    /* Does the person turn away from you? */
    if (rand()%0xFF < cnv->talker->turnAwayProb)
    {
        reply = concat(cnv->talker->pronoun, " turns away!\n", NULL);
        cnv->state = CONV_DONE;
    }

    else if (inquiry[0] == '\0' ||
        strcasecmp(inquiry, "bye") == 0) {
        reply = strdup("Bye.");
        cnv->state = CONV_DONE;
    }

    else if (strncasecmp(inquiry, "look", 4) == 0) {
        reply = concat("You see ", cnv->talker->description, NULL);
        if (isupper(reply[8]))
            reply[8] = tolower(reply[8]);
    }

    else if (strncasecmp(inquiry, "name", 4) == 0)
        reply = concat(cnv->talker->pronoun, " says: I am ", cnv->talker->name, NULL);

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

    else if (strncasecmp(inquiry, cnv->talker->keyword1, testLen) == 0) {
        reply = strdup(cnv->talker->response1);
        if (cnv->talker->questionTrigger == QTRIGGER_KEYWORD1)
            cnv->state = CONV_ASK;
    }

    else if (strncasecmp(inquiry, cnv->talker->keyword2, testLen) == 0) {
        reply = strdup(cnv->talker->response2);
        if (cnv->talker->questionTrigger == QTRIGGER_KEYWORD2)
            cnv->state = CONV_ASK;
    }

    else if (strncasecmp(inquiry, "give", 4) == 0) {
        if (cnv->talker->npcType == NPC_TALKER_BEGGAR) {
            reply = strdup("");
            cnv->state = CONV_BUY_QUANTITY;
        } else
            reply = concat(cnv->talker->pronoun, " says: I do not need thy gold.  Keep it!", NULL);
    }

    else if (strncasecmp(inquiry, "join", 4) == 0) {
        Virtue v;
        if (playerCanPersonJoin(c->saveGame, cnv->talker->name, &v)) {
            if (playerJoin(c->saveGame, cnv->talker->name)) {
                reply = strdup("I am honored to join thee!");
                statsUpdate();
                mapRemovePerson(c->location->map, cnv->talker);
                cnv->state = CONV_DONE;
            } else
                reply = concat("Thou art not ",
                               getVirtueAdjective(v), /* fixme */
                               " enough for me to join thee.",
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
        prompt = strdup("\nHow much? ");
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
            reply = concat("\n\n", cnv->talker->pronoun,
                           " says: Oh Thank thee! I shall never forget thy kindness!\n",
                           NULL);
        }

        else
            reply = strdup("\n\nThou hast not that much gold!\n");
    } else
        reply = strdup("\n");

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

    if (c->saveGame->lbintro) {
        if (c->saveGame->members == 1)
            screenMessage("\n\n\nLord British\nsays:  Welcome\n%s", c->saveGame->players[0].name);
        else if (c->saveGame->members == 2)
            screenMessage("\n\n\nLord British\nsays:  Welcome\n%s and thee also %s!",
                           c->saveGame->players[0].name,
                           c->saveGame->players[1].name);
        else
            screenMessage("\n\n\nLord British\nsays:  Welcome\n%s and thy\nworthy\nAdventurers!",
                           c->saveGame->players[0].name);

        // Check levels here, just like the original!
        lordBritishCheckLevels(cnv);

        intro = strdup("\nWhat would thou\nask of me?\n");
    }

    else {
        intro = concat("\n\n\nLord British rises and says: At long last!\n",
                       c->saveGame->players[0].name,
                       " thou hast come!  We have waited such a long, long time...\n\n",
                       "\n\nLord British sits and says: A new age is upon Britannia. The great evil Lords are gone but our people lack direction and purpose in their lives...\n\n\n",
                       "A champion of virtue is called for. Thou may be this champion, but only time shall tell.  I will aid thee any way that I can!\n\n"
                       "How may I help thee?\n",
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
        reply = strdup("\nLord British\nsays: Fare thee\nwell my friends!");
        cnv->state = CONV_DONE;
        musicPlay();
    }

    else if (strncasecmp(inquiry, "heal", 4) == 0) {
        reply = strdup("\n\n\n\n\n\nHe says: I am\nwell, thank ye.");
        cnv->state = CONV_ASK;
    }

    else if (strncasecmp(inquiry, "help", 4) == 0) {
        reply = lordBritishGetHelp(cnv);
    }

    else {
        for (i = 0; i < 24; i++) {
            if (strncasecmp(inquiry, lbKeywords[i], 4) == 0)
                return strdup(lbText[i]);
        }
        reply = strdup("\nHe says: I\ncannot help thee\nwith that.\n");
    }

    return reply;
}

char *lordBritishGetQuestionResponse(Conversation *cnv, const char *answer) {
    int i;    
    char *reply;

    cnv->state = CONV_TALK;

    if (tolower(answer[0]) == 'y') {
        reply = strdup("\n\nHe says: That is good.\n");
    }

    else if (tolower(answer[0]) == 'n') {        
        reply = strdup("\n\nHe says: Let me heal thy wounds!\n");
        for (i = 0; i < c->saveGame->members; i++) {
            c->saveGame->players[i].status = STAT_GOOD; // res. and cure the party            
            playerHeal(c->saveGame, HT_FULLHEAL, i); // heal the party
        }        
        (*spellCallback)('r', -1); // Same effect as resurrection spell

        statsUpdate();
    }

    else
        reply = strdup("\n\nThat I cannot\nhelp thee with.\n");

    return reply;
}

char *lordBritishGetPrompt(const Conversation *cnv) {
    char *prompt;

    if (cnv->state == CONV_ASK)
        prompt = strdup("\n\nHe asks: Art thou well?");
    else
        prompt = strdup("\nWhat else?\n");

    return prompt;
}

/**
 * Generate the appropriate response when the player asks Lord British
 * for help.  The help text depends on the current party status; when
 * one quest item is complete, Lord British provides some direction to
 * the next one.
 */
char *lordBritishGetHelp(const Conversation *cnv) {
    int v;
    int fullAvatar, partialAvatar;

    /*
     * check whether player is full avatar (in all virtues) or partial
     * avatar (in at least one virtue)
     */
    fullAvatar = 1;
    partialAvatar = 0;
    for (v = 0; v < VIRT_MAX; v++) {
        fullAvatar &= (c->saveGame->karma[v] == 0);
        partialAvatar |= (c->saveGame->karma[v] == 0);
    }

    if (c->saveGame->moves <= 1000) {
        return strdup("To survive in this hostile land thou must first know thyself! Seek ye to master thy weapons and thy magical ability!\n"
                      "\nTake great care in these thy first travels in Britannia.\n"
                      "\nUntil thou dost well know thyself, travel not far from the safety of the townes!\n");
    }

    else if (c->saveGame->members == 1) {
        return strdup("Travel not the open lands alone. There are many worthy people in the diverse townes whom it would be wise to ask to Join thee!\n"
                      "\nBuild thy party unto eight travellers, for only a true leader can win the Quest!\n");
    }

    else if (c->saveGame->runes == 0) {
        return strdup("Learn ye the paths of virtue. Seek to gain entry unto the eight shrines!\n"
                      "\nFind ye the Runes, needed for entry into each shrine, and learn each chant or \"Mantra\" used to focus thy meditations.\n"
                      "\nWithin the Shrines thou shalt learn of the deeds which show thy inner virtue or vice!\n"
                      "\nChoose thy path wisely for all thy deeds of good and evil are remembered and can return to hinder thee!\n");
    }

    else if (!partialAvatar) {
        return strdup("Visit the Seer Hawkwind often and use his wisdom to help thee prove thy virtue.\n"
                      "\nWhen thou art ready, Hawkwind will advise thee to seek the Elevation unto partial Avatarhood in a virtue.\n"
                      "\nSeek ye to become a partial Avatar in all eight virtues, for only then shalt thou be ready to seek the codex!\n");
    }

    else if (c->saveGame->stones == 0) {
        return strdup("Go ye now into the depths of the dungeons. Therein recover the 8 colored stones from the altar pedestals in the halls of the dungeons.\n"
                      "\nFind the uses of these stones for they can help thee in the Abyss!\n");
    }

    else if (!fullAvatar) {
        return strdup("Thou art doing very well indeed on the path to Avatarhood! Strive ye to achieve the Elevation in all eight virtues!\n");
    }

    else if ((c->saveGame->items & ITEM_BELL) == 0 ||
             (c->saveGame->items & ITEM_BOOK) == 0 ||
             (c->saveGame->items & ITEM_CANDLE) == 0) {
        return strdup("Find ye the Bell, Book and Candle!  With these three things, one may enter the Great Stygian Abyss!\n");
    }

    else if ((c->saveGame->items & ITEM_KEY_C) == 0 ||
             (c->saveGame->items & ITEM_KEY_L) == 0 ||
             (c->saveGame->items & ITEM_KEY_T) == 0) {
        return strdup("Before thou dost enter the Abyss thou shalt need the Key of Three Parts, and the Word of Passage.\n"
                      "\nThen might thou enter the Chamber of the Codex of Ultimate Wisdom!\n");
    }

    else {
        return strdup("Thou dost now seem ready to make the final journey into the dark Abyss! Go only with a party of eight!\n"
                      "\nGood Luck, and may the powers of good watch over thee on this thy most perilous endeavor!\n"
                      "\nThe hearts and souls of all Britannia go with thee now. Take care, my friend.\n");
    }
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
            reply = concat("\n\n", hawkwindText[HW_ALREADYAVATAR], "\n", NULL);
        else if (virtueLevel < 20)
            reply = concat("\n\n", hawkwindText[0 * 8 + virtue], "\n", NULL);
        else if (virtueLevel < 40)
            reply = concat("\n\n", hawkwindText[1 * 8 + virtue], "\n", NULL);
        else if (virtueLevel < 60)
            reply = concat("\n\n", hawkwindText[2 * 8 + virtue], "\n", NULL);
        else if (virtueLevel < 99)
            reply = concat("\n\n", hawkwindText[3 * 8 + virtue], "\n", NULL);
        else /* virtueLevel >= 99 */
            reply = concat("\n\n", hawkwindText[4 * 8 + virtue], hawkwindText[HW_GOTOSHRINE], "\n", NULL);

        return reply;
    }
        
    return concat("\n", hawkwindText[HW_DEFAULT], NULL);
}

char *hawkwindGetPrompt(const Conversation *cnv) {
    return concat(hawkwindText[HW_PROMPT], NULL);
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

/**
 * Counts the number of lines (of the maximum width given by
 * columnmax) in the string.
 */
int linecount(const char *s, int columnmax) {
    int lines = 1;
    int col;

    col = 0;
    for (; *s; s++) {
        if (*s == '\n' || col >= columnmax) {
            lines++;
            col = 0;
        } else
            col++;
    }

    return lines;
}

/**
 * Returns the number of characters needed to produce a
 * valid screen of text (given a column width)
 */
int chars_needed(const char *s, int columnmax, int linesdesired) {
    int lines = 1;
    int col;
    int charCount = 0;

    col = 0;
    for (; *s; s++) {
        if (*s == '\n' || col >= columnmax) {
            charCount++;
            lines++;
            col = 0;
            if (lines >= linesdesired)
                return charCount;
        }
        else {
            col++;
            charCount++;
        }
    }    

    return charCount;
}
