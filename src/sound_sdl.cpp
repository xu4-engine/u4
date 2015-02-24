/*
 * SoundMgr_SDL.cpp
 *
 *  Created on: 2011-01-27
 *      Author: Darren Janeczek
 */

#include "sound_p.h"

#include <SDL.h>
#include <SDL_mixer.h>

#include "sound.h"

#include "config.h"
#include "debug.h"
#include "error.h"
#include "music.h"
#include "settings.h"
#include "u4file.h"


bool SoundManager::load_sys(Sound sound, const string &pathname) {
    soundChunk[sound] = Mix_LoadWAV(pathname.c_str());
    if (!soundChunk[sound]) {
        errorWarning("Unable to load sound effect file %s: %s", soundFilenames[sound].c_str(), Mix_GetError());
        return false;
    }
    return true;
}

void SoundManager::play_sys(Sound sound, bool onlyOnce, int specificDurationInTicks) {

    /**
     * Use Channel 1 for sound effects
     */
    if (!onlyOnce || !Mix_Playing(1)) {
        if (Mix_PlayChannelTimed(1, soundChunk[sound], specificDurationInTicks == -1 ? 0 : -1, specificDurationInTicks) == -1)
            fprintf(stderr, "Error playing sound %d: %s\n", sound, Mix_GetError());
    }
}

void SoundManager::stop_sys(int channel)
{
    // If music didn't initialize correctly, then we shouldn't try to stop it
    if (!musicMgr->functional || !settings.soundVol)
        return;

    if (Mix_Playing(channel))
        Mix_HaltChannel(channel);
}

int SoundManager::init_sys()
{
    return 1;
}

void SoundManager::del_sys()
{
}
