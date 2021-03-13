/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

/* FIXME: should this file have all SDL-related stuff extracted and put in music_sdl.c? */
// Yes! :)


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

/*
 * Static variables
 */
bool Music::fading = false;
bool Music::on = false;
bool Music::functional = true;

/*
 * Constructors/Destructors
 */

/**
 * Initiliaze the music
 */
Music::Music() : current(MUSIC_NONE), playing(NULL), logger(new Debug("debug/music.txt", "Music")) {
    filenames.reserve(MUSIC_MAX);
    filenames.push_back("");    // filename for MUSIC_NONE;

    TRACE(*logger, "Initializing music");

    /*
     * load music track filenames from xml config file
     */
    const Config *config = Config::getInstance();

    TRACE_LOCAL(*logger, "Loading music tracks");

    vector<ConfigElement> musicConfs = config->getElement("music").getChildren();
    vector<ConfigElement>::const_iterator i = musicConfs.begin();
    vector<ConfigElement>::const_iterator theEnd = musicConfs.end();
    for (; i != theEnd; ++i) {
        if (i->getName() != "track")
            continue;

        filenames.push_back(i->getString("file"));
        TRACE_LOCAL(*logger, string("\tTrack file: ") + filenames.back());
    }

	create_sys(); // Call the Sound System specific creation file.

	// Set up the volume.
    on = settings.musicVol;
    setMusicVolume(settings.musicVol);
    setSoundVolume(settings.soundVol);
    TRACE(*logger, string("Music initialized: volume is ") + (on ? "on" : "off"));
}

/**
 * Stop playing the music and cleanup
 */
Music::~Music() {
    TRACE(*logger, "Uninitializing music");
    eventHandler->getTimer()->remove(&Music::callback);
	destroy_sys(); // Call the Sound System specific destruction file.

    TRACE(*logger, "Music uninitialized");
    delete logger;
}


bool Music::load(int music) {
    ASSERT(music < MUSIC_MAX, "Attempted to load an invalid piece of music in Music::load()");

    /* music already loaded */
    if (music == current) {
        /* tell calling function it didn't load correctly (because it's already playing) */
        if (isPlaying())
            return false;
        /* it loaded correctly */
        else 
            return true;
    }

    string pathname(u4find_music(filenames[music]));
    if (!pathname.empty()) {
		bool status = load_sys(pathname);
		if (status)
			current = music;
		return status;
    }
    return false;
}

/**
 * Ensures that the music is playing if it is supposed to be, or off
 * if it is supposed to be turned off.
 */
void Music::callback(void *data) {    
    eventHandler->getTimer()->remove(&Music::callback);

    if (musicMgr->on && !musicMgr->isPlaying())
        musicMgr->playMid(c->location->map->music);
    else if (!musicMgr->on && musicMgr->isPlaying())
        musicMgr->stop();
}
    
/**
 * Toggle the music on/off (usually by pressing 'v')
 */
bool Music::toggle() {
    eventHandler->getTimer()->remove(&Music::callback);

    on = !on;
    if (!on)
        fadeOut(1000);
    else
        fadeIn(1000, true);

    eventHandler->getTimer()->add(&Music::callback, settings.gameCyclesPerSecond);
    return on;    
}

/**
 * Fade out the music
 */
void Music::fadeOut(int msecs) {
	// fade the music out even if 'on' is false
	if (!functional)
		return;

	if (isPlaying()) {
		if (!settings.volumeFades)
			stop();
		else
			fadeOut_sys(msecs);
	}
}

/**
 * Fade in the music
 */
void Music::fadeIn(int msecs, bool loadFromMap) {
	if (!functional || !on)
		return;

	if (!isPlaying()) {
		/* make sure we've got something loaded to play */
		if (loadFromMap || !playing)
			load(c->location->map->music);

		if (!settings.volumeFades)
			playMid(c->location->map->music);
		else {
			fadeIn_sys(msecs, loadFromMap);
		}
	}
}

// Game Interface

void musicPlay(int track)
{
/*  Was done in Music::intro (Now musicPlay(introMusic)).
#ifdef IOS
    on = true; // Force iOS to turn this back on from going in the background.
#endif
*/
    musicMgr->playMid(track);
}

void musicPlayLocale()
{
    musicMgr->playMid( c->location->map->music );
}

void musicStop()
{
    musicMgr->stop();
}

void musicFadeOut(int msec)
{
    musicMgr->fadeOut(msec);
}

void musicFadeIn(int msec, bool loadFromMap)
{
    musicMgr->fadeIn(msec, loadFromMap);
}

void musicSetVolume(int vol)
{
    musicMgr->setMusicVolume(vol);
}

int musicVolumeDec()
{
    if (settings.musicVol > 0)
        musicMgr->setMusicVolume(--settings.musicVol);
    return (settings.musicVol * 100 / MAX_VOLUME);  // percentage
}

int musicVolumeInc()
{
    if (settings.musicVol < MAX_VOLUME)
        musicMgr->setMusicVolume(++settings.musicVol);
    return (settings.musicVol * 100 / MAX_VOLUME);  // percentage
}

bool musicToggle()
{
    return musicMgr->toggle();
}

void soundSetVolume(int vol) {
    musicMgr->setSoundVolume(vol);
}

int soundVolumeDec()
{
    if (settings.soundVol > 0)
        musicMgr->setSoundVolume(--settings.soundVol);
    return (settings.soundVol * 100 / MAX_VOLUME);  // percentage
}

int soundVolumeInc()
{
    if (settings.soundVol < MAX_VOLUME)
        musicMgr->setSoundVolume(++settings.soundVol);
    return (settings.soundVol * 100 / MAX_VOLUME);  // percentage
}
