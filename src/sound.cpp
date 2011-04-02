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

int soundInit(void) { return SoundManager::getInstance()->init(); }
void soundDelete(void) { delete SoundManager::getInstance(); }
void soundPlay(Sound sound, bool onlyOnce, int specificDurationInTicks) {
    SoundManager::getInstance()->play(sound, onlyOnce, specificDurationInTicks);
}
void soundStop(int channel) {
    SoundManager::getInstance()->stop(channel);
}

SoundManager *SoundManager::instance = 0;

SoundManager::SoundManager() {
}

SoundManager::~SoundManager() {
    del();
    instance = 0;
}

SoundManager *SoundManager::getInstance() {
    if (!instance)
        instance = new SoundManager();
    return instance;
}

int SoundManager::init() {
    /*
     * load sound track filenames from xml config file
     */
    const Config *config = Config::getInstance();
    soundFilenames.reserve(SOUND_MAX);
    soundChunk.resize(SOUND_MAX, NULL);
    
    vector<ConfigElement> soundConfs = config->getElement("sound").getChildren();
    vector<ConfigElement>::const_iterator i = soundConfs.begin();
    vector<ConfigElement>::const_iterator theEnd = soundConfs.end();
    for (; i != theEnd; ++i) {
        if (i->getName() != "track")
            continue;
        
        soundFilenames.push_back(i->getString("file"));
    }
    return init_sys();
}

void SoundManager::del() {
    del_sys();
}

bool SoundManager::load(Sound sound) {
    ASSERT(sound < SOUND_MAX, "Attempted to load an invalid sound in soundLoad()");
    
    // If music didn't initialize correctly, then we can't play it anyway
    if (!Music::functional || !settings.soundVol)
        return false;
    
    if (soundChunk[sound] == NULL) {
        string pathname(u4find_sound(soundFilenames[sound]));
        string basename = pathname.substr(pathname.find_last_of("/") + 1);
        if (!basename.empty())
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

void SoundManager::stop(int channel) {
    stop_sys(channel);
}
