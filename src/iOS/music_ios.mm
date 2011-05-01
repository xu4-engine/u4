/*
 *  music_ios.mm
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

/* FIXME: should this file have all SDL-related stuff extracted and put in music_sdl.c? */
#import "U4AudioController.h"

#include <memory>
#include <string>
#include <vector>

#include "music.h"
#include "sound.h"

#include "config.h"
#include "context.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "location.h"
#include "settings.h"
#include "u4.h"
#include "u4file.h"

using std::string;
using std::vector;

/*
 * Constructors/Destructors
 */

/**
 * Initiliaze the music
 */
void Music::create_sys() {
	// Seems like we don't need any OS specific stuff here.
}

/**
 * Stop playing the music and cleanup
 */
void Music::destroy_sys() {
    if (playing) {
        TRACE_LOCAL(*logger, "Stopping currently playing music");
        [playing stop];
        [playing release];
    }

    TRACE_LOCAL(*logger, "Closing audio");    

    TRACE_LOCAL(*logger, "Quitting iOS audio subsystem");
    [[AVAudioSession sharedInstance] setActive:NO error:nil];
}

/**
 * Play a midi file
 */
void Music::playMid(Type music) {
    if (!functional || !on)
        return;
    
    /* loaded a new piece of music */
    if (load(music)) {
        [playing setNumberOfLoops:NLOOPS];
        [playing play];
    }
}

bool Music::load_sys(const string &pathname) {
    CGFloat volume = settings.musicVol / CGFloat(MAX_VOLUME);
	if (playing) {
        volume = playing.volume;
	    [playing stop];
	    [playing release];
	    playing = nil;
	}
	playing = [[U4AudioController alloc] initWithFile:
	            [NSString stringWithUTF8String:pathname.c_str()]];
    playing.volume = volume;
	return true;
}

/**
 * Returns true if the mixer is playing any audio
 */
bool Music::isPlaying_sys() {
    return playing != nil ? [playing isPlaying] : false;
}

/**
 * Stop playing music
 */
void Music::stopMid() {
    if (playing != nil)
        [playing stop];
}

/**
 * Set, increase, and decrease music volume
 */
void Music::setMusicVolume_sys(int volume) {
    [playing setVolume:(float)volume / MAX_VOLUME];
}

/**
 * Set, increase, and decrease sound volume
 */
void Music::setSoundVolume_sys(int volume) {
    /**
     * Use Channel 1 for sound effects
     */ 
//    Mix_Volume(1, int((float)MIX_MAX_VOLUME / MAX_VOLUME * volume));
}

void Music::fadeIn_sys(int msecs, bool loadFromMap) {
    [playing fadeIn:NSTimeInterval(msecs / 1000.)];
}

void Music::fadeOut_sys(int msecs) {
    [playing fadeOut:NSTimeInterval(msecs / 1000.)];
}
