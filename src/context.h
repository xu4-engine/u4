/*
 * $Id$
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include "person.h"
#include "types.h"
#include "savegame.h"

class CombatController;
struct _Location;
class Object;
class Party;
class Person;
class Script;

#define CONV_BUFFERLEN 16

typedef enum {
    STATS_PARTY_OVERVIEW,
    STATS_CHAR1,
    STATS_CHAR2,
    STATS_CHAR3,
    STATS_CHAR4,
    STATS_CHAR5,
    STATS_CHAR6,
    STATS_CHAR7,
    STATS_CHAR8,
    STATS_WEAPONS,
    STATS_ARMOR,
    STATS_EQUIPMENT,
    STATS_ITEMS,
    STATS_REAGENTS,
    STATS_MIXTURES
} StatsView;

typedef enum {
    AURA_NONE,
    AURA_HORN,
    AURA_JINX,
    AURA_NEGATE,
    AURA_PROTECTION,
    AURA_QUICKNESS
} Aura;

typedef enum {
    TRANSPORT_FOOT      = 0x1,
    TRANSPORT_HORSE     = 0x2,
    TRANSPORT_SHIP      = 0x4,
    TRANSPORT_BALLOON   = 0x8
} TransportContext;

#define TRANSPORT_ANY               (TransportContext)(0xFFFF)
#define TRANSPORT_FOOT_OR_HORSE     (TransportContext)(TRANSPORT_FOOT | TRANSPORT_HORSE)

typedef struct _Conversation {
    const class Person *talker;
    int state;
    string playerInquiryBuffer;
    Reply *reply;
    /* for vendor conversations */
    class Script *script;
    int quant;
    int player;
    int price;
} Conversation;

class Context {
public:
    Context() : saveGame(NULL), location(NULL) {}

    Party *party;
    SaveGame *saveGame;
    CombatController *combat;
    struct _Location *location;
    Conversation conversation;
    int line, col;
    StatsView statsView;
    int moonPhase;
    int windDirection;
    int windCounter;
    Aura aura;
    int auraDuration;
    int horseSpeed;
    int opacity;
    TransportContext transportContext;
    long lastCommandTime;
    class Object *lastShip;
};

extern Context *c;

#endif
