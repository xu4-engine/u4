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
class Response;
class ResponsePart;

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
    Person(const MapTile& tile);
    Person(const Person *p);

    bool canConverse() const;
    bool isVendor() const;
    virtual string getName() const;
    void setDialogue(Dialogue *d);
    MapCoords &getStart() { return start; }
    PersonNpcType getNpcType() const { return npcType; }
    void setNpcType(PersonNpcType t);

    list<string> getConversationText(Conversation *cnv, const char *inquiry);
    string getPrompt(Conversation *cnv);

    string getIntro(Conversation *cnv);
    string processResponse(Conversation *cnv, Response *response);
    void runCommand(Conversation *cnv, int command);
    string getResponse(Conversation *cnv, const char *inquiry);
    string talkerGetQuestionResponse(Conversation *cnv, const char *inquiry);
    string beggarGetQuantityResponse(Conversation *cnv, const char *response);
    string lordBritishGetQuestionResponse(Conversation *cnv, const char *answer);
    string getQuestion(Conversation *cnv);

private:
    Dialogue* dialogue;
    MapCoords start;
    PersonNpcType npcType;
};

bool isPerson(const Object *punknown);

list<string> replySplit(const string &text);
int linecount(const string &s, int columnmax);

#endif
