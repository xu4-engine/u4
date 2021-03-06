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
    virtual bool keyPressed(int key);
    virtual void timerFired();
    virtual bool isCombatController() const { return false; }

private:
    int timerInterval;
};

// helper functions for the waitable controller; they just avoid
// having eventhandler dependencies in this header file
void Controller_startWait();
void Controller_endWait();

/**
 * Class template for controllers that can be "waited for".
 * Subclasses should set the value variable and call doneWaiting when
 * the controller has completed.
 */
template<class T>
class WaitableController : public Controller {
public:
    WaitableController() : exitWhenDone(false) {}

    virtual T getValue() {
        return value;
    }

    virtual T waitFor() {
        exitWhenDone = true;
        Controller_startWait();
        return getValue();
    }

protected:
    T value;
    void doneWaiting() {
        if (exitWhenDone)
            Controller_endWait();
    }

private:
    bool exitWhenDone;
};

#endif /* CONTROLLER_H */
