/*
 * $Id$
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include <string>
#include "person.h"
#include "types.h"
#include "observable.h"
#include "savegame.h"

class CombatController;
class Location;
class Object;
class Party;
class Person;
class Script;
class StatsArea;

#define CONV_BUFFERLEN 16

typedef enum {
    AURA_NONE,
    AURA_HORN,
    AURA_JINX,
    AURA_NEGATE,
    AURA_PROTECTION,
    AURA_QUICKNESS
} AuraType;

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

/**
 * Aura class
 */
class Aura : public Observable<string> {
public:
    Aura() : type(AURA_NONE), duration(0) {}

    int getDuration() const            { return duration; }
    AuraType getType() const        { return type; }
    bool isActive() const            { return duration > 0; }

    void setDuration(int d) {
        duration = d;
        setChanged();
        notifyObservers("Aura::setDuration");
    }

    void set(AuraType t = AURA_NONE, int d = 0) {
        type = t;
        duration = d;
        setChanged();
        notifyObservers("Aura::set");
    }

    void setType(AuraType t) {
        type = t;
        setChanged();
        notifyObservers("Aura::setType");
    }

    bool operator==(const AuraType &t) const    { return type == t; }
    bool operator!=(const AuraType &t) const    { return !operator==(t); }

    void passTurn() {
        if (--duration <= 0) {
            type = AURA_NONE;
            duration = 0;
        }
    }    

private:
    AuraType type;
    int duration;
};

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
    Conversation conversation;
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
