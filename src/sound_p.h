/*
 * NOTE: This is a legacy file to keep iOS code working until a developer
 *       for that system can update sound_ios.mm & music_ios.mm.
 *
 *  sound_p.h
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

#ifndef SOUND_P_H
#define SOUND_P_H

#ifdef IOS
# if __OBJC__
@class U4AudioController;
# else
typedef void U4AudioController;
# endif
typedef U4AudioController OSSoundChunk;
#endif

#include <string>
#include <vector>
#include "sound.h"

class SoundManager {
public:
    SoundManager();
    ~SoundManager();
    int init(void);
    void play(Sound sound, bool onlyOnce = true, int specificDurationInTicks = -1);
    void stop(int channel = 1);
private:
    bool load(Sound sound);
    int init_sys();
    void del_sys();
    void play_sys(Sound sound, bool onlyOnce, int specificDurationInTicks);
    bool load_sys(Sound sound, const char* pathname);
    std::vector<OSSoundChunk *> soundChunk;
    static SoundManager *instance;
};

#endif // SOUND_P_H
