/*
 * $Id$
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include "aura.h"
#include "person.h"
#include "types.h"
#include "savegame.h"

class CombatController;
class Conversation;
class Location;
class Object;
class Party;
class Person;
class Script;
class StatsArea;

typedef enum {
    TRANSPORT_FOOT      = 0x1,
    TRANSPORT_HORSE     = 0x2,
    TRANSPORT_SHIP      = 0x4,
    TRANSPORT_BALLOON   = 0x8
} TransportContext;

#define TRANSPORT_ANY               (TransportContext)(0xFFFF)
#define TRANSPORT_FOOT_OR_HORSE     (TransportContext)(TRANSPORT_FOOT | TRANSPORT_HORSE)

/**
 * Context class
 */
class Context {
public:
    Context() : saveGame(NULL), location(NULL) {}

    Party *party;
    SaveGame *saveGame;
    CombatController *combat;
    class Location *location;
    Conversation *conversation;
    int line, col;
    StatsArea *stats;
    int moonPhase;
    int windDirection;
    int windCounter;
    Aura *aura;    
    int horseSpeed;
    int opacity;
    TransportContext transportContext;
    long lastCommandTime;
    class Object *lastShip;
};

extern Context *c;

#endif
