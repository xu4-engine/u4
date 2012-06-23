/*
 *  sound_ios.mm
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

#include <string>
#include <vector>

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import "U4AudioController.h"

#include "sound_p.h"

#include "config.h"
#include "debug.h"
#include "error.h"
#include "music.h"
#include "settings.h"
#include "u4file.h"

using std::string;
using std::vector;


@interface SoundStopper : NSObject {
    NSTimer *m_timer;
    U4AudioController *m_controller;
    NSTimeInterval m_interval;
}
- (id)init;
- (void)setAudioController:(U4AudioController *)controller;
- (void)timerFired:(NSTimer *)timer;
- (void)setInterval:(int)baseInterval;
- (void)startTimer;
- (void)stopTimer;
- (BOOL)isValid;
@end

@implementation SoundStopper

- (id)init
{
    self = [super init];
    if (self) {
        ;
    }
    return self;
}

- (void)dealloc {
    [m_timer release];
    [m_controller release];
    [super dealloc];
}

- (void)timerFired:(NSTimer *)timer {
    [m_controller stop];
    [self autorelease];
}

- (void)setAudioController:(U4AudioController *)controller {
    U4AudioController *old = m_controller;
    m_controller = controller;
    [m_controller retain];
    [old release];
}

- (void)setInterval:(int)baseInterval {
    m_interval = baseInterval / 1000.0;
}

- (void)startTimer {
    m_timer = [NSTimer scheduledTimerWithTimeInterval:m_interval target:self
                                             selector:@selector(timerFired:) userInfo:nil repeats:NO];
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


static void initAudioSession() {
    UInt32 otherAudioIsPlaying;
    UInt32 propertySize = sizeof(otherAudioIsPlaying);
    
    // The Sonud stuff is called first in the RootController, therefore all the audio session
    // stuff needs to happen here.
    AudioSessionGetProperty(kAudioSessionProperty_OtherAudioIsPlaying,  &propertySize, 
                            &otherAudioIsPlaying);
    NSError *setCategoryError = nil;
    if (otherAudioIsPlaying) {
        if ([[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategoryAmbient
                                                   error:&setCategoryError] == NO) {
            NSLog(@"Audio Session Set Category Error: %@", [setCategoryError localizedDescription]);
        }
        
    } else {
        if ([[AVAudioSession sharedInstance] setCategory:AVAudioSessionCategorySoloAmbient
                                                   error:&setCategoryError] == NO) {
            NSLog(@"Audio Session Set Category Error: %@", [setCategoryError localizedDescription]);
        }
    }
    NSError *setActiveError = nil;
    if ([[AVAudioSession sharedInstance] setActive:YES error:&setActiveError] == NO) {
        NSLog(@"Audio Activate Error: %@", [setCategoryError localizedDescription]);
    }    
}

int SoundManager::init_sys() {
    initAudioSession();
    return 1;
}


void SoundManager::del_sys() {
    vector<U4AudioController *>::iterator i = soundChunk.begin();
    vector<U4AudioController *>::iterator theEnd = soundChunk.end();
    while (i != theEnd) {
        [*i release];
        *i = nil;
        ++i;
    }
    soundChunk.clear();
}

bool SoundManager::load_sys(Sound sound, const std::string &soundPathName) {
    U4AudioController *player = [[U4AudioController alloc] 
                                 initWithFile:[NSString stringWithUTF8String:soundPathName.c_str()]];
    soundChunk[sound] = player; // Released in soundDelete.
    return true;
}




void SoundManager::play_sys(Sound sound, bool onlyOnce, int specificDurationInTicks) {
    U4AudioController *player = soundChunk.at(sound);
    if (!onlyOnce || ![player isPlaying]) {
        [player play];
    }
    if (specificDurationInTicks > 0) {
        SoundStopper *stopper = [[SoundStopper alloc] init];
        [stopper setAudioController:player];
        [stopper setInterval:specificDurationInTicks];
        [stopper startTimer];
        // Autoreleased in the stop timer.
    }
}

void SoundManager::stop_sys(int /*channel*/)
{
    // This is not excatly what this function wants to do.
    // It wants to stop all sound on one channel, I suspect one channel
    // is sound and the other is music. Oh well.
    std::vector<U4AudioController *>::const_iterator i = soundChunk.begin();
    std::vector<U4AudioController *>::const_iterator theEnd = soundChunk.end();
    while (i != theEnd) {
        [*i stop];
        ++i;
    }
}


