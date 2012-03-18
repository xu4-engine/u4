/*
 * $Id$
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include <vector>

#include "location.h"
#include "aura.h"
#include "names.h"
#include "person.h"
#include "script.h"
#include "types.h"
#include "savegame.h"

class Object;
class Party;
class Person;
class Script;
class StatsArea;

typedef enum {
    TRANSPORT_FOOT      = 0x1,
    TRANSPORT_HORSE     = 0x2,
    TRANSPORT_SHIP      = 0x4,
    TRANSPORT_BALLOON		= 0x8,
    TRANSPORT_FOOT_OR_HORSE	= TRANSPORT_FOOT | TRANSPORT_HORSE,
    TRANSPORT_ANY			= 0xffff
} TransportContext;

/**
 * Context class
 */
class Context : public Script::Provider {
public:
    Context();
    ~Context();

    Party *party;
    SaveGame *saveGame;
    class Location *location;
    int line, col;
    StatsArea *stats;
    int moonPhase;
    int windDirection;
    int windCounter;
    bool windLock;
    Aura *aura;    
    int horseSpeed;
    int opacity;
    TransportContext transportContext;
    long lastCommandTime;
    class Object *lastShip;

    /**
     * Provides scripts with information
     */
    virtual string translate(std::vector<string>& parts) {
        if (parts.size() == 1) {
            if (parts[0] == "wind")
                return getDirectionName(static_cast<Direction>(windDirection));
        }
        return "";
    }
};

extern Context *c;

#endif
