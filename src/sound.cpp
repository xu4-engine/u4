/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise
#include "sound.h"

#include "config.h"
#include "debug.h"
#include "error.h"
#include "music.h"
#include "settings.h"
#include "u4file.h"

#include "sound_p.h"

using std::string;
using std::vector;

Music* musicMgr = NULL;
static SoundManager* soundMgr = NULL;

int soundInit(void)
{
    // Music sets up the platform audio system while SoundManager only holds
    // the sample data for sound effects.

    ASSERT(! musicMgr, "soundInit called more than once");
    musicMgr = new Music;
    soundMgr = new SoundManager;
    return soundMgr->init();
}

void soundDelete(void)
{
    delete soundMgr;
    delete musicMgr;
}

void soundPlay(Sound sound, bool onlyOnce, int specificDurationInTicks) {
    soundMgr->play(sound, onlyOnce, specificDurationInTicks);
}

void soundStop(int channel) {
    soundMgr->stop(channel);
}

SoundManager::SoundManager() {
}

SoundManager::~SoundManager() {
    del_sys();
}

int SoundManager::init() {
    soundChunk.resize(SOUND_MAX, NULL);
    return init_sys();
}

bool SoundManager::load(Sound sound) {
    ASSERT(sound < SOUND_MAX, "Attempted to load an invalid sound in soundLoad()");

    // If music didn't initialize correctly, then we can't play it anyway
    if (!Music::functional || !settings.soundVol)
        return false;

    if (soundChunk[sound] == NULL) {
        const char* pathname = configService->soundFile(sound);
        if( pathname )
            return load_sys(sound, pathname);
    }
    return true;
}

void SoundManager::play(Sound sound, bool onlyOnce, int specificDurationInTicks) {
    ASSERT(sound < SOUND_MAX, "Attempted to play an invalid sound in soundPlay()");

    // If music didn't initialize correctly, then we can't play it anyway
    if (!Music::functional || !settings.soundVol)
        return;

    if (soundChunk[sound] == NULL)
    {
        if (!load(sound)) {
            return;
        }
    }

    play_sys(sound, onlyOnce, specificDurationInTicks);
}
