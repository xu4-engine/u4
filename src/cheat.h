/*
 * $Id$
 */

#ifndef CHEAT_H
#define CHEAT_H

#include "controller.h"

class CheatMenuController : public WaitableController<void *> {
public:
    bool keyPressed(int key);
};

/**
 * This class controls the wind option from the cheat menu.  It
 * handles setting the wind direction as well as locking/unlocking.
 * The value field of WaitableController isn't used.
 */
class WindCmdController : public WaitableController<void *> {
public:
    bool keyPressed(int key);
};

#endif /* CHEAT_H */
