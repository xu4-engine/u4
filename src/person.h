/*
 * $Id$
 */

#ifndef PERSON_H
#define PERSON_H

#include "object.h"

struct _Conversation;

typedef struct _Reply {
    char **chunk;
    int nchunks;
} Reply;

typedef enum {
    QTRIGGER_NONE = 0,
    QTRIGGER_JOB = 3,
    QTRIGGER_HEALTH = 4,
    QTRIGGER_KEYWORD1 = 5,
    QTRIGGER_KEYWORD2 = 6
} PersonQuestionTrigger;

typedef enum {
    QUESTION_NORMAL,
    QUESTION_HUMILITY_TEST
} PersonQuestionType;

typedef enum {
   NPC_EMPTY,
   NPC_TALKER,
   NPC_TALKER_BEGGAR,
   NPC_TALKER_GUARD,
   NPC_TALKER_COMPANION,
   NPC_VENDOR_WEAPONS,
   NPC_VENDOR_ARMOR,
   NPC_VENDOR_FOOD,
   NPC_VENDOR_TAVERN,
   NPC_VENDOR_REAGENTS,
   NPC_VENDOR_HEALER,
   NPC_VENDOR_INN,
   NPC_VENDOR_GUILD,
   NPC_VENDOR_STABLE,
   NPC_LORD_BRITISH,
   NPC_HAWKWIND,
   NPC_MAX
} PersonNpcType;

typedef enum {
    CONV_INTRO,
    CONV_TALK,
    CONV_ASK,
    CONV_ASKYESNO,
    CONV_VENDORQUESTION,
    CONV_BUY_ITEM,
    CONV_SELL_ITEM,
    CONV_BUY_QUANTITY,
    CONV_SELL_QUANTITY,
    CONV_BUY_PRICE,
    CONV_CONFIRMATION,
    CONV_CONTINUEQUESTION,
    CONV_TOPIC,
    CONV_PLAYER,
    CONV_FULLHEAL,
    CONV_ADVANCELEVELS,
    CONV_GIVEBEGGAR,
    CONV_DONE
} ConversationState;

typedef enum {
    CONVINPUT_STRING,
    CONVINPUT_CHARACTER,
    CONVINPUT_NONE
} ConversationInputType;

typedef struct _Person {
    char *name;
    char *pronoun;
    char *description;
    char *job;
    char *health;
    char *response1;
    char *response2;
    char *question;
    char *yesresp;
    char *noresp;
    char *keyword1;
    char *keyword2;
    PersonQuestionTrigger questionTrigger;
    PersonQuestionType questionType;
    int turnAwayProb;
    unsigned char tile0, tile1;
    unsigned int startx, starty, startz;
    ObjectMovementBehavior movement_behavior;
    PersonNpcType npcType;
    int vendorIndex;
    int permanent; /* if the person is naturally part of the map, if not, we need to know */
} Person;

Reply *replyNew(const char *text);
void replyDelete(Reply *reply);
int personInit(void);
Reply *personGetConversationText(struct _Conversation *cnv, const char *inquiry);
char *personGetPrompt(const struct _Conversation *cnv);
ConversationInputType personGetInputRequired(const struct _Conversation *cnv, int *bufferlen);
const char *personGetChoices(const struct _Conversation *cnv);

#endif
