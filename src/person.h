/*
 * $Id$
 */

#ifndef PERSON_H
#define PERSON_H

#include <list>
#include <string>

#include "creature.h"
#include "types.h"

using std::list;
using std::string;

class Conversation;
class Dialogue;

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
    string name;
    Dialogue* dialogue;
    MapCoords start;    
    PersonNpcType npcType;
};

bool isPerson(Object *punknown);

list<string> replySplit(const string &text);
int personInit(void);
list<string> personGetConversationText(class Conversation *cnv, const char *inquiry);
string personGetPrompt(class Conversation *cnv);
const char *personGetChoices(class Conversation *cnv);
int linecount(const string &s, int columnmax);

#endif
