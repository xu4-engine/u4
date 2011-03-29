/*
 * SoundMgr_SDL.cpp
 *
 *  Created on: 2011-01-27
 *      Author: Darren Janeczek
 */

#include "sound_sdl.h"
#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_mixer.h>

#include "sound.h"

#include "config.h"
#include "debug.h"
#include "error.h"
#include "music.h"
#include "settings.h"
#include "u4file.h"
#include "music_sdl.h"

int SoundMgr_SDL::init() {
	int r = (SoundMgr::init());
	soundChunk.resize(this->soundFilenames.size(), NULL);
	return r;

}


bool SoundMgr_SDL::load(Sound sound) {
    ASSERT(sound < SOUND_MAX, "Attempted to load an invalid sound in soundLoad()");


    // If music didn't initialize correctly, then we can't play it anyway
    if (!musicMgr->functional || !settings.soundVol)
        return false;

    if (soundChunk[sound] == NULL) {
        string pathname(u4find_sound(soundFilenames[sound]));
        string basename = pathname.substr(pathname.find_last_of("/") + 1);
        if (!basename.empty()) {
            soundChunk[sound] = Mix_LoadWAV(pathname.c_str());
            if (!soundChunk[sound]) {
                errorWarning("Unable to load sound effect file %s: %s", soundFilenames[sound].c_str(), Mix_GetError());
                return false;
            }
        }
    }
    return true;
}

void SoundMgr_SDL::play(Sound sound, bool onlyOnce, int specificDurationInTicks) {

    ASSERT(sound < SOUND_MAX, "Attempted to play an invalid sound in soundPlay()");

    // If music didn't initialize correctly, then we can't play it anyway
    if (!musicMgr->functional || !settings.soundVol)
        return;

    if (soundChunk[sound] == NULL)
    {
        if (!soundLoad(sound))
        {
            errorWarning("Unable to load sound effect file %s: %s", soundFilenames[sound].c_str(), Mix_GetError());
            return;
        }
    }

    /**
     * Use Channel 1 for sound effects
     */
    if (!onlyOnce || !Mix_Playing(1)) {
        if (Mix_PlayChannelTimed(1, soundChunk[sound], specificDurationInTicks == -1 ? 0 : -1, specificDurationInTicks) == -1)
            fprintf(stderr, "Error playing sound %d: %s\n", sound, Mix_GetError());
    }
}

void SoundMgr_SDL::stop(int channel)
{
    // If music didn't initialize correctly, then we shouldn't try to stop it
    if (!musicMgr->functional || !settings.soundVol)
        return;

    if (Mix_Playing(channel))
        Mix_HaltChannel(channel);
}

SoundMgr *SoundMgr_SDL::getSDLInstance() {
    if (!instance)
        instance = new SoundMgr_SDL();
    return instance;
}

void SoundMgr_SDL::replaceSoundMgrInstance()
{
	SoundMgr::GET_SOUND_MGR_INSTANCE = &SoundMgr_SDL::getSDLInstance;
}

