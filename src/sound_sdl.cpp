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

extern int u4_SDL_InitSubSystem(Uint32 flags);
extern void u4_SDL_QuitSubSystem(Uint32 flags);


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
    musicEnabled = settings.musicVol;
    musicSetVolume(settings.musicVol);
    soundSetVolume(settings.soundVol);
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
    eventHandler->getTimer()->remove(&music_callback);

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
    //TRACE(*logger, "Uninitializing sound");
    eventHandler->getTimer()->remove(&music_callback);

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
        const char* pathname = configService->soundFile(sound);
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
    if (!audioFunctional || !settings.soundVol)
        return;

    if (soundChunk[sound] == NULL)
    {
        if (!sound_load(sound))
            return;
    }

    /**
     * Use Channel 1 for sound effects
     */
    if (!onlyOnce || !Mix_Playing(1)) {
        if (Mix_PlayChannelTimed(1, soundChunk[sound],
                    specificDurationInTicks == -1 ? 0 : -1,
                    specificDurationInTicks) == -1)
            fprintf(stderr, "Error playing sound %d: %s\n",
                    sound, Mix_GetError());
    }
}

void soundStop(int channel) {
    // If music didn't initialize correctly, then we shouldn't try to stop it
    if (!audioFunctional || !settings.soundVol)
        return;

    if (Mix_Playing(channel))
        Mix_HaltChannel(channel);
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

    const char* pathname = configService->musicFile(music);
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
    Mix_HaltMusic();
}

void musicFadeOut(int msec)
{
    // fade the music out even if 'musicEnabled' is false
    if (!audioFunctional)
        return;

    if (Mix_PlayingMusic()) {
        if (settings.volumeFades) {
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

        if (settings.volumeFades) {
            if (Mix_FadeInMusic(playing, NLOOPS, msec) == -1)
                errorWarning("Mix_FadeInMusic: %s\n", Mix_GetError());
        } else
            musicPlayLocale();
    }
}

void musicSetVolume(int volume)
{
    Mix_VolumeMusic(int((float)MIX_MAX_VOLUME / MAX_VOLUME * volume));
}

int musicVolumeDec()
{
    if (settings.musicVol > 0)
        musicSetVolume(--settings.musicVol);
    return (settings.musicVol * 100 / MAX_VOLUME);  // percentage
}

int musicVolumeInc()
{
    if (settings.musicVol < MAX_VOLUME)
        musicSetVolume(++settings.musicVol);
    return (settings.musicVol * 100 / MAX_VOLUME);  // percentage
}

/**
 * Toggle the music on/off.
 */
bool musicToggle()
{
    eventHandler->getTimer()->remove(&music_callback);

    musicEnabled = !musicEnabled;
    if (musicEnabled)
        musicFadeIn(1000, true);
    else
        musicFadeOut(1000);

    eventHandler->getTimer()->add(&music_callback, settings.gameCyclesPerSecond);
    return musicEnabled;
}

void soundSetVolume(int volume) {
    // Use Channel 1 for sound effects
    Mix_Volume(1, int((float)MIX_MAX_VOLUME / MAX_VOLUME * volume));
}

int soundVolumeDec()
{
    if (settings.soundVol > 0)
        soundSetVolume(--settings.soundVol);
    return (settings.soundVol * 100 / MAX_VOLUME);  // percentage
}

int soundVolumeInc()
{
    if (settings.soundVol < MAX_VOLUME)
        soundSetVolume(++settings.soundVol);
    return (settings.soundVol * 100 / MAX_VOLUME);  // percentage
}
