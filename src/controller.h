/*
 * controller.h
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdint.h>

enum ControllerInputEvent {
    CIE_MOUSE_MOVE,
    CIE_MOUSE_PRESS,
    CIE_MOUSE_RELEASE,
    CIE_MOUSE_WHEEL
};

enum ControllerMouseButton {
    CMOUSE_LEFT = 1,
    CMOUSE_MIDDLE,
    CMOUSE_RIGHT
};

struct InputEvent {
    uint16_t type;      // ControllerInputEvent
    uint16_t n;         // Button id
    uint16_t state;     // Button mask
    int16_t  x, y;      // Axis value
};

/**
 * A generic controller base class.  Controllers are classes that
 * contain the logic for responding to external events (e.g. keyboard,
 * mouse, timers).
 */
class Controller {
public:
    Controller(short timerInterval = 0);
    virtual ~Controller();

    /* methods for interacting with event manager */
    bool notifyKeyPressed(int key);
    int getTimerInterval() const { return timerInterval; }
    bool deleteOnPop() const { return autoDelete; }
    static void timerCallback(void *data);

    /* control methods subclasses may want to override */
    virtual bool present();
    virtual void conclude();
    virtual bool keyPressed(int key);
    virtual bool inputEvent(const InputEvent*);
    virtual void timerFired();
    virtual bool isCombatController() const { return false; }

protected:
    void setDeleteOnPop(bool enable = true) { autoDelete = enable; }

    short timerInterval;
    bool autoDelete;
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
