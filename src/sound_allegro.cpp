/*
 * sound_allegro.cpp
 *
 * See https://liballeg.org/a5docs/trunk/audio.html
 *
 * The pulseaudio process uses 100% CPU with allegro5 5.2.4.
 * This may be fixed in 5.2.7 (See https://liballeg.org/changes-5.2.html).
 */

#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

#include "sound.h"

#ifdef UNIT_TEST
#include <vector>
#include <cstdio>
#define ASSERT(exp,msg)
#define errorWarning(...)   printf(__VA_ARGS__); printf("\n");
#define MAX_VOLUME 10
struct {
    int musicVol = 5;
    int soundVol = 10;
} settings;
const char* config_soundFile(int id) {
    return NULL;
}
const char* config_musicFile(int id) {
    const char* fn = NULL;
    switch (id) {
        case 1: fn = "../mid/minstrel/U4song1.ogg"; break;
        case 2: fn = "../mid/Castles.mp3"; break;
        case 3: fn = "../mid/castles.it";  break;
        case 4: fn = "../mid/Castles.mid"; break;
    }
    printf("  musicFile: %s\n", fn ? fn : "none");
    return fn;
}

#else

#include "config.h"
#include "context.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "settings.h"

#define config_soundFile(id)    configService->soundFile(id)
#define config_musicFile(id)    configService->musicFile(id)
#endif


static bool audioFunctional = false;
static bool musicEnabled = false;
static int currentTrack;
static float musicVolume = 1.0;
static ALLEGRO_VOICE* voice = NULL;
static ALLEGRO_MIXER* finalMix = NULL;
static ALLEGRO_MIXER* fxMixer = NULL;
static ALLEGRO_AUDIO_STREAM* musicStream = NULL;
static std::vector<ALLEGRO_SAMPLE *> sa_samples;

/*
 * Initialize sound & music service.
 */
int soundInit(void)
{
    ASSERT(! audioFunctional, "soundInit called more than once");

    if (! al_install_audio())
        return 0;
    if (! al_init_acodec_addon())
        return 0;

#if 0
    int audio_rate = 22050;
    Uint16 audio_format = AUDIO_S16LSB; /* 16-bit stereo */
    int audio_channels = 2;
    int audio_buffers = 1024;

    if (al_create_mixer(audio_rate, audio_format, audio_channels,
            audio_buffers)) {
        fprintf(stderr, "Unable to open audio!\n");
        audioFunctional = false;
        return 0;
    }
#endif

    voice = al_create_voice(44100, ALLEGRO_AUDIO_DEPTH_INT16,
                            ALLEGRO_CHANNEL_CONF_2);
    finalMix = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32,
                               ALLEGRO_CHANNEL_CONF_2);
    fxMixer  = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32,
                               ALLEGRO_CHANNEL_CONF_2);

    al_attach_mixer_to_voice(finalMix, voice);
    al_attach_mixer_to_mixer(fxMixer, finalMix);

    // Reserve sample instances on fxMixer.
    al_set_default_mixer(fxMixer);
    if (! al_reserve_samples(12))
        return 0;
    audioFunctional = true;

    // Set up the volume.
    musicEnabled = settings.musicVol;
    musicSetVolume(settings.musicVol);
    soundSetVolume(settings.soundVol);
    //TRACE(*logger, string("Music initialized: volume is ") + (musicEnabled ? "on" : "off"));

    sa_samples.resize(SOUND_MAX, NULL);

    // Initialize the music
    currentTrack = MUSIC_NONE;
    musicStream = NULL;

    return 1;
}

#ifndef UNIT_TEST
/**
 * Ensures that the music is playing if it is supposed to be, or off
 * if it is supposed to be turned off.
 */
static void music_callback(void *data) {
    eventHandler->getTimer()->remove(&music_callback);

    bool mplaying = al_get_audio_stream_playing(musicStream);
    if (musicEnabled) {
        if (!mplaying)
        {
            printf( "KR mc play\n" );
            musicPlayLocale();
        }
    } else {
        if (mplaying)
        {
            printf( "KR mc stop\n" );
            musicStop();
        }
    }
}
#endif

void soundDelete(void)
{
    if (! audioFunctional)
        return;

#ifndef UNIT_TEST
    eventHandler->getTimer()->remove(&music_callback);
#endif

    if (musicStream) {
        al_destroy_audio_stream(musicStream);
        musicStream = NULL;
    }
    if (fxMixer) {
        al_destroy_mixer(fxMixer);
        fxMixer = NULL;
    }
    if (finalMix) {
        al_destroy_mixer(finalMix);
        finalMix = NULL;
    }
    if (voice) {
        al_destroy_voice(voice);
        voice = NULL;
    }

    std::vector<ALLEGRO_SAMPLE *>::iterator si;
    for( si = sa_samples.begin(); si != sa_samples.end(); ++si )
        al_destroy_sample( *si );

    al_uninstall_audio();
    audioFunctional = false;
}

static bool sound_load(Sound sound) {
    if (sa_samples[sound] == NULL) {
        const char* pathname = config_soundFile(sound);
        if (pathname) {
            sa_samples[sound] = al_load_sample(pathname);
            if (!sa_samples[sound]) {
                errorWarning("Unable to load sound file %s", pathname);
                return false;
            }
        }
    }
    return true;
}

void soundPlay(Sound sound, bool onlyOnce, int specificDurationInTicks) {
    ASSERT(sound < SOUND_MAX, "Attempted to play an invalid sound in soundPlay()");

    // If audio didn't initialize correctly, then we can't play it anyway
    if (!audioFunctional || !settings.soundVol)
        return;

    if (sa_samples[sound] == NULL)
    {
        if (!sound_load(sound))
            return;
    }

    //if (!onlyOnce || !Mix_Playing(1)) {
        // TODO: Handle specificDurationInTicks.
        if (! al_play_sample(sa_samples[sound], 1.0, 0.0, 1.0,
                             ALLEGRO_PLAYMODE_ONCE, NULL))
            fprintf(stderr, "Error playing sound %d\n", sound);
    //}
}

/*
 * Stop all sound effects.  Use musicStop() to halt music playback.
 */
void soundStop() {
    // If audio didn't initialize correctly, then we shouldn't try to stop it
    if (!audioFunctional || !settings.soundVol)
        return;

    al_stop_samples();
}

static bool music_load(int music) {
    ASSERT(music < MUSIC_MAX, "Attempted to load an invalid piece of music in music_load()");

    /* music already loaded */
    if (music == currentTrack) {
        if (! musicStream)
            return false;       // Handle MUSIC_NONE.
        /* tell calling function it didn't load correctly (because it's already playing) */
        if (al_get_audio_stream_playing(musicStream))
            return false;
        /* it loaded correctly */
        else
            return true;
    }

    const char* pathname = config_musicFile(music);
    if (! pathname)
        return false;

    if (musicStream)
        al_destroy_audio_stream(musicStream);
    musicStream = al_load_audio_stream(pathname, 4, 2048);
    if (!musicStream) {
        errorWarning("unable to load music file %s", pathname );
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
        al_set_audio_stream_gain(musicStream, musicVolume);
        al_attach_audio_stream_to_mixer(musicStream, finalMix);
    }
}

void musicPlayLocale()
{
#ifdef UNIT_TEST
    musicPlay(1);
#else
    musicPlay( c->location->map->music );
#endif
}

void musicStop()
{
    if (musicStream)
        al_set_audio_stream_playing(musicStream, 0);
}

#if 0
void fader() {
    al_set_audio_stream_gain(musicStream, 0.0);
}
#endif

void musicFadeOut(int msec)
{
    // fade the music out even if 'musicEnabled' is false
    if (!audioFunctional)
        return;

    if (musicStream && al_get_audio_stream_playing(musicStream)) {
        /*
        if (settings.volumeFades) {
            if (Mix_FadeOutMusic(msec) == -1)
                errorWarning("Mix_FadeOutMusic");
        } else
        */
            musicStop();
    }
}

void musicFadeIn(int msec, bool loadFromMap)
{
    if (!audioFunctional || !musicEnabled)
        return;

    if (! al_get_audio_stream_playing(musicStream)) {
        /* make sure we've got something loaded to play */
        if (loadFromMap || !musicStream) {
#ifdef UNIT_TEST
            music_load(0);
#else
            music_load(c->location->map->music);
#endif
        }

        /*
        if (settings.volumeFades) {
            if (Mix_FadeInMusic(musicStream, NLOOPS, msec) == -1)
                errorWarning("Mix_FadeInMusic");
        } else
        */
            musicPlayLocale();
    }
}

void musicSetVolume(int volume)
{
    musicVolume = float(volume) / MAX_VOLUME;
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
    if (! audioFunctional)
        return false;

#ifndef UNIT_TEST
    eventHandler->getTimer()->remove(&music_callback);
#endif

    musicEnabled = !musicEnabled;
    if (musicEnabled)
        musicFadeIn(1000, true);
    else
        musicFadeOut(1000);

#ifndef UNIT_TEST
    eventHandler->getTimer()->add(&music_callback, settings.gameCyclesPerSecond);
#endif
    return musicEnabled;
}

void soundSetVolume(int volume) {
    if (audioFunctional)
        al_set_mixer_gain(fxMixer, float(volume) / MAX_VOLUME);
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


#ifdef UNIT_TEST
// c++ -DUNIT_TEST -g -DDEBUG -I. sound_allegro.cpp -lallegro_acodec -lallegro_audio -lallegro
int main() {
    printf( "al_init: %d\n", al_init());
    printf( "soundInit: %d\n", soundInit());

    for (int i = 1; i < 4; ++i) {
        printf("musicPlay(%d)\n", i);
        musicPlay(i);
        al_rest(4.0);
    }

    soundDelete();
    printf( "Test done.\n" );
}
#endif
