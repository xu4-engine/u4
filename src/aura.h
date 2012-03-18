/*
 * $Id$
 */

#ifndef AURA_H
#define AURA_H

#include <string>

#include "observable.h"

using std::string;

/**
 * Aura class
 */
class Aura : public Observable<Aura *> {
public:
    enum Type {
        NONE,
        HORN,
        JINX,
        NEGATE,
        PROTECTION,
        QUICKNESS
    };

    Aura();

    int getDuration() const         { return duration; }
    Aura::Type getType() const      { return type; }
    bool isActive() const           { return duration > 0; }

    void setDuration(int d);
    void set(Type = NONE, int d = 0);
    void setType(Type t);

    bool operator==(const Type &t) const    { return type == t; }
    bool operator!=(const Type &t) const    { return !operator==(t); }

    void passTurn();

private:
    Type type;
    int duration;
};

#endif
