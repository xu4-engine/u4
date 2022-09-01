/*
 * controller.cpp
 */

#include <cstdio>

#include "controller.h"
#include "event.h"
#include "xu4.h"

/**
 * \param timerInterval     The timerFired method will be called if this value
 *                          is non-zero.
 */
Controller::Controller(short timerInterval) : autoDelete(false) {
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
    bool processed = EventHandler::globalKeyHandler(key);
    if (!processed)
        processed = keyPressed(key);
    return processed;
}

/**
 * Perform any subclass setup before a controller becomes active.
 *
 * Return true if the controller should be run.
 */
bool Controller::present() {
    return true;
}

/**
 * Perform any subclass cleanup after a controller has run.
 *
 * The conclude method is invoked only if present returned true.
 */
void Controller::conclude() {}

/**
 * The keyboard input handler for a controller.
 * The base keyPressed method returns true (consumes all key events).
 */
bool Controller::keyPressed(int key) {
    return true;
}

/**
 * The mouse/joystick input handler for a controller.
 * The base inputEvent method returns true (consumes all input events).
 */
bool Controller::inputEvent(const InputEvent*) {
    return true;
}

/**
 * The default timerFired handler for a controller.  By default,
 * timers are ignored, but subclasses can override this method and it
 * will be called every <interval> 1/4 seconds.
 */
void Controller::timerFired() {
#ifdef DEBUG
    printf("Unused Controller::timerFired! %p\n", this);
#endif
}

/**
 * A simple adapter to make a timer callback into a controller method
 * call.
 */
void Controller::timerCallback(void *data) {
    Controller *controller = static_cast<Controller *>(data);
    controller->timerFired();
}

void Controller_startWait() {
    xu4.eventHandler->run();
    xu4.eventHandler->setControllerDone(false);
    xu4.eventHandler->popController();
}

void Controller_endWait() {
    xu4.eventHandler->setControllerDone();
}
