/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <string>
#include <vector>

#include "sound.h"

#include "config.h"
#include "debug.h"
#include "error.h"
#include "music.h"
#include "settings.h"
#include "u4file.h"

using std::string;
using std::vector;

/*
 * Static variables
 */
SoundMgr *SoundMgr::instance = NULL;
SoundMgr * (*SoundMgr::GET_SOUND_MGR_INSTANCE)(void) = &SoundMgr::getInstance;



int SoundMgr::init() {
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

    return 1;
}

SoundMgr *SoundMgr::getInstance() {
    if (!instance)
        instance = new SoundMgr();
    return instance;
}

