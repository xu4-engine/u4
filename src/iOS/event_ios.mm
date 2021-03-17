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
 * Sets the key-repeat characteristics of the keyboard.
 */
int EventHandler::setKeyRepeat(int /*delay*/, int interval) {
    return 0;
}
