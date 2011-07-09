/*
 *  event_ios.mm
 *  xu4
 *
 * Copyright 2011 Trenton Schulz. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY TRENTON SCHULZ ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Trenton Schulz.
 *
 */



/*
 * $Id$
 */

#include "u4.h"

#include "event.h"

#include "context.h"
#include "debug.h"
#include "error.h"
#include "screen.h"
#include "settings.h"
#include <UIKit/UIKit.h>
#include <limits>
#include "ios_helpers.h"
#include "combat.h"

extern bool verbose, quit;
extern int eventTimerGranularity;

KeyHandler::KeyHandler(Callback func, void *d, bool asyncronous) :
handler(func),
async(asyncronous),
data(d)
{}

/**
 * Sets the key-repeat characteristics of the keyboard.
 */
int KeyHandler::setKeyRepeat(int /*delay*/, int interval) {
    return 0;
}

/**
 * Handles any and all keystrokes.
 * Generally used to exit the application, switch applications,
 * minimize, maximize, etc.
 */
bool KeyHandler::globalHandler(int key) {
    switch(key) {
#if defined(MACOSX)
        case U4_META + 'q': /* Cmd+q */
        case U4_META + 'x': /* Cmd+x */
        case U4_META + 'X': /* Cmd+q */
        case U4_META + 'Q': /* Cmd+x */
#endif
        case U4_ALT + 'x': /* Alt+x */
        case U4_ALT + 'X': /* Alt+x */
#if defined(WIN32)
        case U4_ALT + U4_FKEY + 3:
#endif
            quit = true;
            EventHandler::end();
            return true;
        default: return false;
    }
}

/**
 * A default key handler that should be valid everywhere
 */
bool KeyHandler::defaultHandler(int key, void *) {
    bool valid = true;
    
    switch (key) {
        case '`':
            if (c && c->location)
                printf("x = %d, y = %d, level = %d, tile = %d (%s)\n", c->location->coords.x, c->location->coords.y, c->location->coords.z, c->location->map->translateToRawTileIndex(*c->location->map->tileAt(c->location->coords, WITH_OBJECTS)), c->location->map->tileTypeAt(c->location->coords, WITH_OBJECTS)->getName().c_str());
            break;
        default:
            valid = false;
            break;
    }
    
    return valid;
}

/**
 * A key handler that ignores keypresses
 */
bool KeyHandler::ignoreKeys(int , void *) {
    return true;
}

/**
 * Handles a keypress.
 * First it makes sure the key combination is not ignored
 * by the current key handler. Then, it passes the keypress
 * through the global key handler. If the global handler
 * does not process the keystroke, then the key handler
 * handles it itself by calling its handler callback function.
 */
bool KeyHandler::handle(int key) {
    bool processed = false;
    if (!isKeyIgnored(key)) {
        processed = globalHandler(key);
        if (!processed)
            processed = handler(key, data);
    }
    
    return processed;
}

/**
 * Returns true if the key or key combination is always ignored by xu4
 */
bool KeyHandler::isKeyIgnored(int key) {
    switch(key) {
        case U4_RIGHT_SHIFT:
        case U4_LEFT_SHIFT:
        case U4_RIGHT_CTRL:
        case U4_LEFT_CTRL:
        case U4_RIGHT_ALT:
        case U4_LEFT_ALT:
        case U4_RIGHT_META:
        case U4_LEFT_META:
        case U4_TAB:
            return true;
        default: return false;
    }
}

bool KeyHandler::operator==(Callback cb) const {
    return (handler == cb) ? true : false;
}

KeyHandlerController::KeyHandlerController(KeyHandler *handler) {
    this->handler = handler;
}

KeyHandlerController::~KeyHandlerController() {
    delete handler;
}

bool KeyHandlerController::keyPressed(int key) {
    ASSERT(handler != NULL, "key handler must be initialized");
    return handler->handle(key);
}

KeyHandler *KeyHandlerController::getKeyHandler() {
    return handler;
}


@interface TimedManagerHelper : NSObject {
    TimedEventMgr *m_manager;
    NSTimer *m_timer;
    NSTimeInterval m_interval;
}
- (id)initWithTimedEventMgr:(TimedEventMgr *)manager;
- (void)timerFired:(NSTimer *)timer;
- (void)setInterval:(int)baseInterval;
- (void)startTimer;
- (void)stopTimer;
- (BOOL)isValid;
@end

@implementation TimedManagerHelper

- (id)initWithTimedEventMgr:(TimedEventMgr *)manager
{
    self = [self init];
    if (self) {
        m_manager = manager;
    }
    return self;
}

- (void)dealloc {
    [m_timer release];
    [super dealloc];
}

- (void)timerFired:(NSTimer *)timer {
    if (eventHandler->screenCallback() != 0)
        (*eventHandler->screenCallback())();
    screenRedrawScreen();
    m_manager->tick();
}

- (void)setInterval:(int)baseInterval {
    m_interval = baseInterval / 1000.0;
}

- (void)startTimer {
    m_timer = [NSTimer scheduledTimerWithTimeInterval:m_interval target:self
                                             selector:@selector(timerFired:) userInfo:nil repeats:YES];
    [m_timer retain];
}

- (void)stopTimer {
    [m_timer invalidate];
    [m_timer release];
    m_timer = nil;
}

- (BOOL)isValid {
    return [m_timer isValid];
}
@end

/**
 * Constructs a timed event manager object.
 * Adds a timer callback to the SDL subsystem, which
 * will drive all of the timed events that this object
 * controls.
 */
TimedEventMgr::TimedEventMgr(int i) : baseInterval(i) {
    m_helper = [[TimedManagerHelper alloc] initWithTimedEventMgr:this];
    [m_helper setInterval:baseInterval];
    [m_helper startTimer];
    instances++;
}

/**
 * Destructs a timed event manager object.
 * It removes the callback timer and un-initializes the
 * SDL subsystem if there are no other active TimedEventMgr
 * objects.
 */
TimedEventMgr::~TimedEventMgr() {
    [m_helper stopTimer];
    [m_helper release];
    if (instances > 0)
        instances--;
}


unsigned int TimedEventMgr::callback(unsigned int, void *) {
    return 0;
}

/**
 * Re-initializes the timer manager to a new timer granularity
 */
void TimedEventMgr::reset(unsigned int interval) {
    baseInterval = interval;
    stop();
    [m_helper setInterval:baseInterval];
    start();
}

void TimedEventMgr::stop() {
    [m_helper stopTimer];
}

void TimedEventMgr::start() {
    [m_helper startTimer];
}

bool TimedEventMgr::hasActiveTimer() const
{
    return [m_helper isValid];
}

/**
 * Constructs an event handler object.
 */
EventHandler::EventHandler() : timer(eventTimerGranularity), updateScreen(NULL) {
}

class DisableButtonHelper {
public:
    DisableButtonHelper() {
        U4IOS::disableGameButtons();
    }
    ~DisableButtonHelper() {
        U4IOS::enableGameButtons();
    }
};


/**
 * Delays program execution for the specified number of milliseconds.
 */
void EventHandler::sleep(unsigned int usec) {
    DisableButtonHelper disableButtons;
    NSRunLoop *runloop = [NSRunLoop mainRunLoop];
    NSDate *date = [NSDate dateWithTimeIntervalSinceNow:usec / 1000.0];
    [runloop runUntilDate:date];
}

void EventHandler::run() {
    static const CFTimeInterval DistantFuture = std::numeric_limits<CFTimeInterval>::max();
    while (!ended && !controllerDone) {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, DistantFuture, true);
    }
    if (ended) {
        int unused; // Quit somehow...
        (void)unused;
    }
}

void EventHandler::controllerStopped_helper() {
    CFRunLoopRef cfrunloop = [[NSRunLoop mainRunLoop] getCFRunLoop];
    CFRunLoopStop(cfrunloop);
}

void EventHandler::handleEvent(UIEvent *)  {
}

void EventHandler::setScreenUpdate(void (*updateScreen)(void)) {
    this->updateScreen = updateScreen;
}

/**
 * Returns true if the queue is empty of events that match 'mask'.
 */
bool EventHandler::timerQueueEmpty() {
    return !eventHandler->getTimer()->hasActiveTimer();
}

/**
 * Adds a key handler to the stack.
 */
void EventHandler::pushKeyHandler(KeyHandler kh) {
    KeyHandler *new_kh = new KeyHandler(kh);
    KeyHandlerController *khc = new KeyHandlerController(new_kh);
    pushController(khc);
}

/**
 * Pops a key handler off the stack.
 * Returns a pointer to the resulting key handler after
 * the current handler is popped.
 */
void EventHandler::popKeyHandler() {
    if (controllers.empty())
        return;
    
    popController();
}

/**
 * Returns a pointer to the current key handler.
 * Returns NULL if there is no key handler.
 */
KeyHandler *EventHandler::getKeyHandler() const {
    if (controllers.empty())
        return NULL;
    
    KeyHandlerController *khc = dynamic_cast<KeyHandlerController *>(controllers.back());
    ASSERT(khc != NULL, "EventHandler::getKeyHandler called when controller wasn't a keyhandler");
    if (khc == NULL)
        return NULL;
    
    return khc->getKeyHandler();
}

/**
 * Eliminates all key handlers and begins stack with new handler.
 * This pops all key handlers off the stack and adds
 * the key handler provided to the stack, making it the
 * only key handler left. Use this function only if you
 * are sure the key handlers in the stack are disposable.
 */
void EventHandler::setKeyHandler(KeyHandler kh) {
    while (popController() != NULL) {}
    pushKeyHandler(kh);
}

MouseArea* EventHandler::mouseAreaForPoint(int x, int y) {
    int i;
    MouseArea *areas = getMouseAreaSet();
    
    if (!areas)
        return NULL;
    
    for (i = 0; areas[i].npoints != 0; i++) {
        if (screenPointInMouseArea(x, y, &(areas[i]))) {
            return &(areas[i]);
        }
    }
    return NULL;
}
