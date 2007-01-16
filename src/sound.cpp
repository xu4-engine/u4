/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

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

using std::string;
using std::vector;

vector<string> soundFilenames;
vector<Mix_Chunk *> soundChunk;

int soundInit() {
    /*
     * load sound track filenames from xml config file
     */
    const Config *config = Config::getInstance();

    vector<ConfigElement> soundConfs = config->getElement("sound").getChildren();
    for (std::vector<ConfigElement>::iterator i = soundConfs.begin(); i != soundConfs.end(); i++) {

        if (i->getName() != "track")
            continue;

        soundFilenames.push_back(i->getString("file"));
    }

    soundFilenames.resize(SOUND_MAX, "");
    soundChunk.resize(SOUND_MAX, NULL);

    return 1;
}

void soundDelete() {
}

bool soundLoad(Sound sound) {
    ASSERT(sound < SOUND_MAX, "Attempted to load an invalid sound in soundLoad()");
    
    // If music didn't initialize correctly, then we can't play it anyway
    if (!Music::functional || !settings.soundVol)
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

void soundPlay(Sound sound, bool onlyOnce) {

    ASSERT(sound < SOUND_MAX, "Attempted to play an invalid sound in soundPlay()");
    
    // If music didn't initialize correctly, then we can't play it anyway
    if (!Music::functional || !settings.soundVol)
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
        if (Mix_PlayChannel(1, soundChunk[sound], 0) == -1)
            fprintf(stderr, "Error playing sound %d: %s\n", sound, Mix_GetError());
    }
}

void soundStop(int channel)
{
    if (Mix_Playing(channel))
        Mix_HaltChannel(channel);
}
