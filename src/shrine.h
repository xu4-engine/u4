/*
 * shrine.h
 */

#ifndef SHRINE_H
#define SHRINE_H

#include "map.h"
#include "savegame.h"

#define SHRINE_MEDITATION_INTERVAL  100
#define MEDITATION_MANTRAS_PER_CYCLE 16

class Shrine : public Map {
public:
    // Methods
    virtual const char* getName() const;
    const char* mantraStr() const;
    void enter();
    void enhancedSequence();
    void meditationCycle();
    void askMantra();
    void eject();
    void showVision(bool elevated);

    // Properties
    Symbol mantra;
    Virtue virtue;
};

class ShrineState {
public:
    int cycles;
    int completedCycles;
    std::vector<std::string> advice;
    std::string shrineName;     // Temporary storage for name.
};

bool shrineCanEnter(const Portal *p);

#endif
