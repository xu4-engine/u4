/*
 * $Id$
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

/**
 * A generic controller base class.  Controllers are classes that
 * contain the logic for responding to external events (e.g. keyboard,
 * mouse, timers).
 */
class Controller {
public:
    Controller(int timerInterval = 1);
    virtual ~Controller();

    /* methods for interacting with event manager */
    bool notifyKeyPressed(int key);
    int getTimerInterval();
    static void timerCallback(void *data);

    /* control methods subclasses may want to override */
    virtual bool keyPressed(int key) = 0;
    virtual void timerFired();

private:
    int timerInterval;
};

#endif /* CONTROLLER_H */
