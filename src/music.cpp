/*
 * NOTE: This is a legacy file to keep iOS code working until a developer
 *       for that system can update sound_ios.mm & music_ios.mm.
 */

#include <string>

#include "music.h"
#include "sound.h"

#include "config.h"
#include "context.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "location.h"
#include "settings.h"

/*
 * Static variables
 */
bool Music::musicEnabled = false;
bool Music::functional = true;

/*
 * Constructors/Destructors
 */

/**
 * Initiliaze the music
 */
Music::Music() : current(MUSIC_NONE), playing(NULL), logger(new Debug("debug/music.txt", "Music")) {
    TRACE(*logger, "Initializing music");

    create_sys(); // Call the Sound System specific creation file.

    // Set up the volume.
    musicEnabled = settings.musicVol;
    setMusicVolume(settings.musicVol);
    setSoundVolume(settings.soundVol);
    TRACE(*logger, string("Music initialized: volume is ") + (musicEnabled ? "on" : "off"));
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

    const char* pathname = configService->musicFile(music);
    if (pathname) {
        if (load_sys(pathname)) {
            current = music;
            return true;
        }
    }
    return false;
}

/**
 * Ensures that the music is playing if it is supposed to be, or off
 * if it is supposed to be turned off.
 */
void Music::callback(void *data) {
    eventHandler->getTimer()->remove(&Music::callback);

    bool mplaying = musicMgr->isPlaying();
    if (musicMgr->musicEnabled) {
        if (!mplaying)
            musicMgr->playMid(c->location->map->music);
    } else {
        if (mplaying)
            musicMgr->stopMid();
    }
}

/**
 * Toggle the music on/off (usually by pressing 'v')
 */
bool Music::toggle() {
    eventHandler->getTimer()->remove(&Music::callback);

    musicEnabled = !musicEnabled;
    if (musicEnabled)
        fadeIn(1000, true);
    else
        fadeOut(1000);

    eventHandler->getTimer()->add(&Music::callback, settings.gameCyclesPerSecond);
    return musicEnabled;
}

/**
 * Fade out the music
 */
void Music::fadeOut(int msecs) {
    // fade the music out even if 'musicEnabled' is false
    if (!functional)
        return;

    if (isPlaying()) {
        if (settings.volumeFades)
            fadeOut_sys(msecs);
        else
            stopMid();
    }
}

/**
 * Fade in the music
 */
void Music::fadeIn(int msecs, bool loadFromMap) {
    if (!functional || !musicEnabled)
        return;

    if (!isPlaying()) {
        /* make sure we've got something loaded to play */
        if (loadFromMap || !playing)
            load(c->location->map->music);

        if (settings.volumeFades)
            fadeIn_sys(msecs, loadFromMap);
        else
            playMid(c->location->map->music);
    }
}

// Game Interface

void musicPlay(int track)
{
/*  Was done in Music::intro (Now musicPlay(introMusic)).
#ifdef IOS
    musicEnabled = true; // Force iOS to turn this back on from going in the background.
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
    musicMgr->stopMid();
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
