/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include "controller.h"
#include "event.h"

Controller::Controller(int timerInterval) {
    this->timerInterval = timerInterval;
}

Controller::~Controller() {
}

/**
 * The event manager will call this method to notify the active
 * controller that a key has been pressed.  The key will be passed on
 * to the virtual keyPressed method.
 */
bool Controller::notifyKeyPressed(int key) {
    bool processed = KeyHandler::globalHandler(key);
    if (!processed)
        processed = keyPressed(key);
    
    return processed;
}

int Controller::getTimerInterval() {
    return timerInterval;
}

/**
 * The default timerFired handler for a controller.  By default,
 * timers are ignored, but subclasses can override this method and it
 * will be called every <interval> 1/4 seconds.
 */
void Controller::timerFired() {
}

/**
 * A simple adapter to make a timer callback into a controller method
 * call.
 */
void Controller::timerCallback(void *data) {
    Controller *controller = static_cast<Controller *>(data);
    controller->timerFired();
}
