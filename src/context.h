/*
 * context.h
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include "location.h"
#include "aura.h"
#include "names.h"
#include "person.h"
#include "types.h"
#include "savegame.h"
#include "shrine.h"

class Party;
class StatsArea;

typedef enum {
    TRANSPORT_FOOT      = 0x1,
    TRANSPORT_HORSE     = 0x2,
    TRANSPORT_SHIP      = 0x4,
    TRANSPORT_BALLOON   = 0x8,
    TRANSPORT_FOOT_OR_HORSE = TRANSPORT_FOOT | TRANSPORT_HORSE,
    TRANSPORT_ANY       = 0xffff
} TransportContext;

/**
 * The Context class holds the world simulation state.
 */
class Context {
public:
    Context();
    ~Context();

    Party *party;
    SaveGame *saveGame;
    Location *location;
    int line, col;
    StatsArea *stats;
    int moonPhase;
    int windDirection;
    int windCounter;
    bool windLock;
    Aura aura;
    int horseSpeed;
    int opacity;
    TransportContext transportContext;
    uint32_t lastCommandTime;
    uint32_t commandTimer;
    Object *lastShip;
    ShrineState shrineState;
};

extern Context *c;

#endif
