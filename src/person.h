/*
 * $Id$
 */

#ifndef PERSON_H
#define PERSON_H

#include <string>
#include <list>

#include "monster.h"
#include "types.h"

struct _Conversation;

typedef std::list<const char *> Reply;

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

/*typedef struct _Person {
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
    int permanent; // if the person is naturally part of the map, if not, we need to know
} Person;*/

class Person : public Monster {
public:
    Person(MapTile tile = 0) : Monster(tile) {
        setType(OBJECT_PERSON);
    }

public:
    std::string name;
    std::string pronoun;
    std::string description;
    std::string job;
    std::string health;
    std::string response1;
    std::string response2;
    std::string question;
    std::string yesresp;
    std::string noresp;
    std::string keyword1;
    std::string keyword2;
    PersonQuestionTrigger questionTrigger;
    PersonQuestionType questionType;
    union {
        int turnAwayProb;
        int attackProb;
    };     
    MapCoords start;    
    //ObjectMovementBehavior movement_behavior;
    PersonNpcType npcType;
};

bool isPerson(Object *punknown);

Reply *replyNew(const std::string &text);
void replyDelete(Reply *reply);
int personInit(void);
Reply *personGetConversationText(struct _Conversation *cnv, const char *inquiry);
std::string personGetPrompt(const struct _Conversation *cnv);
ConversationInputType personGetInputRequired(const struct _Conversation *cnv, int *bufferlen);
const char *personGetChoices(const struct _Conversation *cnv);

#endif
