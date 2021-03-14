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
