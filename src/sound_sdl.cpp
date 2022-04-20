/*
 * sound_sdl.cpp
 */

#include <SDL.h>
#include <SDL_mixer.h>

#include "sound.h"
#include "config.h"
#include "context.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "settings.h"
#include "xu4.h"

extern int u4_SDL_InitSubSystem(Uint32 flags);
extern void u4_SDL_QuitSubSystem(Uint32 flags);


// Use Channel 1 for sound effects
#define FX_CHANNEL  1
#define NLOOPS -1

static bool audioFunctional = false;
static bool musicEnabled = false;
static int currentTrack;
static struct _Mix_Music* playing = NULL;
static std::vector<Mix_Chunk *> soundChunk;

/*
 * Initialize sound & music service.
 */
int soundInit(void)
{
    ASSERT(! audioFunctional, "soundInit called more than once");

    //logger(new Debug("debug/music.txt", "Music"))
    //TRACE_LOCAL(*logger, "Initializing SDL audio");

    int audio_rate = 22050;
    Uint16 audio_format = AUDIO_S16LSB; /* 16-bit stereo */
    int audio_channels = 2;
    int audio_buffers = 1024;

    if (u4_SDL_InitSubSystem(SDL_INIT_AUDIO) == -1) {
        errorWarning("unable to init SDL audio subsystem: %s", SDL_GetError());
        audioFunctional = false;
        return 0;
    }

    if (Mix_OpenAudio(audio_rate, audio_format, audio_channels,
            audio_buffers)) {
        fprintf(stderr, "Unable to open audio!\n");
        audioFunctional = false;
        return 0;
    }
    audioFunctional = true;

    Mix_AllocateChannels(16);

    // Set up the volume.
    musicEnabled = xu4.settings->musicVol;
    musicSetVolume(xu4.settings->musicVol);
    soundSetVolume(xu4.settings->soundVol);
    //TRACE(*logger, string("Music initialized: volume is ") + (musicEnabled ? "on" : "off"));

    soundChunk.resize(SOUND_MAX, NULL);

    // Initialize the music
    currentTrack = MUSIC_NONE;
    playing = NULL;

    return 1;
}

/**
 * Ensures that the music is playing if it is supposed to be, or off
 * if it is supposed to be turned off.
 */
static void music_callback(void *data) {
    xu4.eventHandler->getTimer()->remove(&music_callback);

    bool mplaying = Mix_PlayingMusic();
    if (musicEnabled) {
        if (!mplaying)
            musicPlayLocale();
    } else {
        if (mplaying)
            Mix_HaltMusic();
    }
}

void soundDelete(void)
{
    if (! audioFunctional)
        return;

    //TRACE(*logger, "Uninitializing sound");
    xu4.eventHandler->getTimer()->remove(&music_callback);

    if (playing) {
        Mix_FreeMusic(playing);
        playing = NULL;
    }

    Mix_CloseAudio();
    u4_SDL_QuitSubSystem(SDL_INIT_AUDIO);
    audioFunctional = false;

    //TRACE(*logger, "Sound uninitialized");
    //delete logger;
}

static bool sound_load(Sound sound) {
    if (soundChunk[sound] == NULL) {
        const char* pathname = xu4.config->soundFile(sound);
        if (pathname) {
            soundChunk[sound] = Mix_LoadWAV(pathname);
            if (!soundChunk[sound]) {
                errorWarning("Unable to load sound file %s: %s",
                             pathname, Mix_GetError());
                return false;
            }
        }
    }
    return true;
}

void soundPlay(Sound sound, bool onlyOnce, int specificDurationInTicks) {
    ASSERT(sound < SOUND_MAX, "Attempted to play an invalid sound in soundPlay()");

    // If music didn't initialize correctly, then we can't play it anyway
    if (!audioFunctional || !xu4.settings->soundVol)
        return;

    if (soundChunk[sound] == NULL)
    {
        if (!sound_load(sound))
            return;
    }

    if (!onlyOnce || !Mix_Playing(FX_CHANNEL)) {
        if (Mix_PlayChannelTimed(FX_CHANNEL, soundChunk[sound],
                    specificDurationInTicks == -1 ? 0 : -1,
                    specificDurationInTicks) == -1)
            fprintf(stderr, "Error playing sound %d: %s\n",
                    sound, Mix_GetError());
    }
}

/*
 * Return duration in milliseconds.
 */
int soundDuration(Sound sound) {
    (void) sound;
    if (!audioFunctional)
        return 0;
    return 200;     // TODO: Implement this.
}

/*
 * Stop all sound effects.  Use musicStop() to halt music playback.
 */
void soundStop() {
    // If music didn't initialize correctly, then we shouldn't try to stop it
    if (!audioFunctional || !xu4.settings->soundVol)
        return;

    if (Mix_Playing(FX_CHANNEL))
        Mix_HaltChannel(FX_CHANNEL);
}

static bool music_load(int music) {
    ASSERT(music < MUSIC_MAX, "Attempted to load an invalid piece of music in music_load()");

    /* music already loaded */
    if (music == currentTrack) {
        /* tell calling function it didn't load correctly (because it's already playing) */
        if (Mix_PlayingMusic())
            return false;
        /* it loaded correctly */
        else
            return true;
    }

    const char* pathname = xu4.config->musicFile(music);
    if (! pathname)
        return false;

    if (playing)
        Mix_FreeMusic(playing);
    playing = Mix_LoadMUS(pathname);
    if (!playing) {
        errorWarning("unable to load music file %s: %s", pathname, Mix_GetError());
        return false;
    }

    currentTrack = music;
    return true;
}

void musicPlay(int track)
{
    if (!audioFunctional || !musicEnabled)
        return;

    /* loaded a new piece of music */
    if (music_load(track)) {
        Mix_PlayMusic(playing, NLOOPS);
        //Mix_SetMusicPosition(0.0);  //Could be useful if music was stored on different 'it/mod' patterns
    }
}

void musicPlayLocale()
{
    musicPlay( c->location->map->music );
}

void musicStop()
{
    if (audioFunctional)
        Mix_HaltMusic();
}

void musicFadeOut(int msec)
{
    // fade the music out even if 'musicEnabled' is false
    if (!audioFunctional)
        return;

    if (Mix_PlayingMusic()) {
        if (xu4.settings->volumeFades) {
            if (Mix_FadeOutMusic(msec) == -1)
                errorWarning("Mix_FadeOutMusic: %s\n", Mix_GetError());
        } else
            Mix_HaltMusic();
    }
}

void musicFadeIn(int msec, bool loadFromMap)
{
    if (!audioFunctional || !musicEnabled)
        return;

    if (!Mix_PlayingMusic()) {
        /* make sure we've got something loaded to play */
        if (loadFromMap || !playing)
            music_load(c->location->map->music);

        if (xu4.settings->volumeFades) {
            if (Mix_FadeInMusic(playing, NLOOPS, msec) == -1)
                errorWarning("Mix_FadeInMusic: %s\n", Mix_GetError());
        } else
            musicPlayLocale();
    }
}

void musicSetVolume(int volume)
{
    if (audioFunctional)
        Mix_VolumeMusic(int((float)MIX_MAX_VOLUME / MAX_VOLUME * volume));
}

int musicVolumeDec()
{
    if (xu4.settings->musicVol > 0)
        musicSetVolume(--xu4.settings->musicVol);
    return (xu4.settings->musicVol * 100 / MAX_VOLUME);  // percentage
}

int musicVolumeInc()
{
    if (xu4.settings->musicVol < MAX_VOLUME)
        musicSetVolume(++xu4.settings->musicVol);
    return (xu4.settings->musicVol * 100 / MAX_VOLUME);  // percentage
}

/**
 * Toggle the music on/off.
 */
bool musicToggle()
{
    if (! audioFunctional)
        return false;

    xu4.eventHandler->getTimer()->remove(&music_callback);

    musicEnabled = !musicEnabled;
    if (musicEnabled)
        musicFadeIn(1000, true);
    else
        musicFadeOut(1000);

    xu4.eventHandler->getTimer()->add(&music_callback, xu4.settings->gameCyclesPerSecond);
    return musicEnabled;
}

void soundSetVolume(int volume) {
    if (audioFunctional)
        Mix_Volume(FX_CHANNEL, int((float)MIX_MAX_VOLUME / MAX_VOLUME * volume));
}

int soundVolumeDec()
{
    if (xu4.settings->soundVol > 0)
        soundSetVolume(--xu4.settings->soundVol);
    return (xu4.settings->soundVol * 100 / MAX_VOLUME);  // percentage
}

int soundVolumeInc()
{
    if (xu4.settings->soundVol < MAX_VOLUME)
        soundSetVolume(++xu4.settings->soundVol);
    return (xu4.settings->soundVol * 100 / MAX_VOLUME);  // percentage
}
