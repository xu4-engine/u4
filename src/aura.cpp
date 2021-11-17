/*
 * $Id$
 */

#include "aura.h"
#include "xu4.h"

#define NOTIFY  gs_emitMessage(SENDER_AURA, this)

Aura::Aura() : type(NONE), duration(0) {}

void Aura::setDuration(int d) {
    duration = d;
    NOTIFY;
}

void Aura::set(Type t, int d) {
    type = t;
    duration = d;
    NOTIFY;
}

void Aura::setType(Type t) {
    type = t;
    NOTIFY;
}

void Aura::passTurn() {
    if (duration > 0) {
        duration--;

        if (duration == 0) {
            type = NONE;
            NOTIFY;
        }
    }
}
