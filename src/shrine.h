/*
 * $Id$
 */

#ifndef SHRINE_H
#define SHRINE_H

#include "map.h"
#include "savegame.h"

#define SHRINE_MEDITATION_INTERVAL  100
#define MEDITATION_MANTRAS_PER_CYCLE 16

/*typedef struct _Shrine {
    Virtue virtue;
    const char *mantra;
} Shrine;*/

class Shrine : public Map {
public:
    Shrine();

    // Methods
    virtual string  getName();
    Virtue          getVirtue() const;
    string          getMantra() const;

    void            setVirtue(Virtue v);
    void            setMantra(string mantra);

    void enter();
    void enhancedSequence();
    void meditationCycle();
    void askMantra();
    void eject();
    void showVision(bool elevated);

    // Properties
private:
    string name;
    Virtue virtue;
    string mantra;
};

bool shrineCanEnter(const struct _Portal *p);
bool isShrine(Map *punknown);

#endif
