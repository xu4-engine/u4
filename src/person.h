/*
 * $Id$
 */

#ifndef PERSON_H
#define PERSON_H

#include <string>
#include <list>

#include "conversation.h"
#include "creature.h"
#include "types.h"

typedef std::list<const char *> Reply;

class Conversation;

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

class Person : public Creature {
public:
    Person(MapTile tile = 0) : Creature(tile) {
        setType(Object::PERSON);
    }

public:
    std::string name;
    Dialogue* dialogue;
    MapCoords start;    
    PersonNpcType npcType;
};

bool isPerson(Object *punknown);

Reply *replyNew(const std::string &text);
void replyDelete(Reply *reply);
int personInit(void);
Reply *personGetConversationText(class Conversation *cnv, const char *inquiry);
std::string personGetPrompt(class Conversation *cnv);
const char *personGetChoices(class Conversation *cnv);

#endif
