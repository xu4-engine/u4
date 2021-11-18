/*
 * aura.h
 */

#ifndef AURA_H
#define AURA_H

/**
 * Aura class
 */
class Aura {
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

    void set(Type, int d);
    void passTurn();

private:
    Type type;
    int duration;
};

#endif
