/*
 * sound_allegro.cpp
 *
 * See https://liballeg.org/a5docs/trunk/audio.html
 *
 * The pulseaudio process uses 100% CPU with allegro5 5.2.4.
 * This is fixed in 5.2.7 (See https://liballeg.org/changes-5.2.html).
 */

#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

#include "sound.h"

#ifdef UNIT_TEST
#include <vector>
#include <cstdio>
#include "support/getTicks.c"
#define ASSERT(exp,msg)
#define errorWarning(...)   printf(__VA_ARGS__); printf("\n");
#define MAX_VOLUME 10
struct Settings {
    int musicVol, soundVol;
    bool volumeFades;
};
Settings ts = {5, 10, true};
struct {
    Settings* settings = &ts;
} xu4;
const char* config_soundFile(int id) {
    return NULL;
}
const char* config_musicFile(int id) {
    const char* fn = NULL;
    switch (id) {
        case 1: fn = "../module/Ultima-IV/music/minstrel/castles.ogg"; break;
        case 2: fn = "../mid/Castles.mp3"; break;
        case 3: fn = "../mid/dungeon.it";  break;
        case 4: fn = "../mid/Castles.mid"; break;
    }
    printf("  musicFile: %s\n", fn ? fn : "none");
    return fn;
}
float screenFrameDuration() { return 1.0f/24.0f; }

#else

#include "config.h"
#include "context.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "settings.h"
#include "xu4.h"

extern uint32_t getTicks();
extern float screenFrameDuration();

#define config_soundFile(id)    xu4.config->soundFile(id)
#define config_musicFile(id)    xu4.config->musicFile(id)
#define config_voiceParts(id)   xu4.config->voiceParts(id)
#endif

#define FX_CONTROL_SLOTS    2
#define FX_CONTROL_BITS     ((1<<FX_CONTROL_SLOTS) - 1)

static bool audioFunctional = false;
static bool musicEnabled = false;
static int currentTrack;
static int controlUsed;                 // Mask of active fxControl ids.
static float musicVolume = 1.0;         // Final level desired by user.
static float musicGain = 1.0;           // Current fade level.
static float musicFade = 0.0;           // musicGain delta per frame.
static ALLEGRO_VOICE* voice = NULL;
static ALLEGRO_MIXER* finalMix = NULL;
static ALLEGRO_MIXER* fxMixer = NULL;
static ALLEGRO_AUDIO_STREAM* musicStream = NULL;
static ALLEGRO_SAMPLE_ID fxControl[FX_CONTROL_SLOTS];
static uint32_t fxDuration[FX_CONTROL_SLOTS];
static std::vector<ALLEGRO_SAMPLE *> sa_samples;

#ifdef CONF_MODULE
static ALLEGRO_FILE* moduleFile = NULL;

#define CONF_SPEAK
#ifdef CONF_SPEAK
static ALLEGRO_AUDIO_STREAM* dialogStream = NULL;
static ALLEGRO_FILE* moduleDialog = NULL;
static double dialogEnd;
static int currentDialog;
#endif
#endif

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

    // Initialize the music
    currentTrack = MUSIC_NONE;
    musicStream = NULL;

#ifdef CONF_SPEAK
    // Initialize the dialog
    currentDialog = 0;
    dialogStream = NULL;
    dialogEnd = 0.0;
#endif

    // Set up the volume.
    musicEnabled = xu4.settings->musicVol;
    musicSetVolume(xu4.settings->musicVol);
    soundSetVolume(xu4.settings->soundVol);
    //TRACE(*logger, string("Music initialized: volume is ") + (musicEnabled ? "on" : "off"));

    sa_samples.resize(SOUND_MAX, NULL);
    controlUsed = 0;

    return 1;
}

void soundDelete(void)
{
    if (! audioFunctional)
        return;

    if (musicStream) {
        al_destroy_audio_stream(musicStream);
        musicStream = NULL;
    }
#ifdef CONF_MODULE
    if (moduleFile) {
        al_fclose(moduleFile);
        moduleFile = NULL;
    }
#ifdef CONF_SPEAK
    if (dialogStream) {
        al_destroy_audio_stream(dialogStream);
        dialogStream = NULL;
    }
    if (moduleDialog) {
        al_fclose(moduleDialog);
        moduleDialog = NULL;
    }
#endif
#endif
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

void soundSuspend(int halt)
{
    if (audioFunctional) {
#if 0
        // Causes 100% CPU usage on Linux with 5.2.7 & 5.2.8.
        al_set_mixer_playing(finalMix, ! halt);
#else
        // al_set_voice_playing doesn't work with mixers, so the mixer is
        // detached instead (which doesn't actually suspend the backend).
        if (halt)
            al_detach_mixer(finalMix);
        else
            al_attach_mixer_to_voice(finalMix, voice);
#endif
    }
}

#ifdef CONF_MODULE
static const char* audioExt(const CDIEntry* entry) {
    // NOTE: Using CDI_MASK_FORMAT since mod_addLayer() changes the 0xDA byte.

    switch (entry->cdi & CDI_MASK_FORMAT) {
        case DA7A_AUDIO_WAVE & CDI_MASK_FORMAT:
            return ".wav";
        case DA7A_AUDIO_MP3 & CDI_MASK_FORMAT:
            return ".mp3";
        case DA7A_AUDIO_OGG_VORBIS & CDI_MASK_FORMAT:
            return ".ogg";
    }
    return NULL;
}
#endif

static bool sound_load(Sound sound) {
    if (sa_samples[sound] == NULL) {
#ifdef CONF_MODULE
        const CDIEntry* ent = config_soundFile(sound);
        if (ent) {
            ALLEGRO_FILE* slice;
            ALLEGRO_FILE* af = al_fopen(xu4.config->modulePath(ent), "rb");
            if (af) {
                al_fseek(af, ent->offset, ALLEGRO_SEEK_SET);
                slice = al_fopen_slice(af, ent->bytes, "r");
                sa_samples[sound] = al_load_sample_f(slice, audioExt(ent));

                // NOTE: al_fclose does an unwanted seek to the slice end.
                //       Allegro 5.2.8 will have a 'n' mode to prevent this.
                al_fclose(slice);
                al_fclose(af);
            }
        }
#else
        const char* pathname = config_soundFile(sound);
        if (pathname)
            sa_samples[sound] = al_load_sample(pathname);
#endif
        if (! sa_samples[sound]) {
            errorWarning("Unable to load sound %d", (int) sound);
            return false;
        }
    }
    return true;
}

void soundPlay(Sound sound, bool onlyOnce, int durationLimitMSec) {
    (void) onlyOnce;
    ASSERT(sound < SOUND_MAX, "Attempted to play an invalid sound in soundPlay()");

    // If audio didn't initialize correctly, then we can't play it anyway
    if (!audioFunctional || !xu4.settings->soundVol)
        return;

    if (sa_samples[sound] == NULL)
    {
        if (!sound_load(sound))
            return;
    }

    if (durationLimitMSec > 0 && controlUsed != FX_CONTROL_BITS) {
        int bit, i;
        // The controlUsed check above tells us there is a free control slot
        // so find it and set its duration.
        for (i = 0, bit = 1; i < FX_CONTROL_SLOTS; bit <<= 1, ++i) {
            if ((controlUsed & bit) == 0) {
                if (al_play_sample(sa_samples[sound], 1.0, 0.0, 1.0,
                                   ALLEGRO_PLAYMODE_ONCE, fxControl + i)) {
                    controlUsed |= bit;
                    fxDuration[i] = getTicks() + durationLimitMSec;
                } else
                    fprintf(stderr, "Error playing sound %d\n", sound);
                break;
            }
        }
    } else {
        if (! al_play_sample(sa_samples[sound], 1.0, 0.0, 1.0,
                             ALLEGRO_PLAYMODE_ONCE, NULL))
            fprintf(stderr, "Error playing sound %d\n", sound);
    }
}

static ALLEGRO_AUDIO_STREAM* moduleAudioStream(const CDIEntry* ent,
                                               ALLEGRO_FILE** fileHandlePtr) {
    ALLEGRO_FILE* fh = *fileHandlePtr;
    if (! fh)
        *fileHandlePtr = fh = al_fopen(xu4.config->modulePath(ent), "rb");

    if (fh) {
        ALLEGRO_FILE* slice;
        al_fseek(fh, ent->offset, ALLEGRO_SEEK_SET);
        slice = al_fopen_slice(fh, ent->bytes, "r");
        return al_load_audio_stream_f(slice, audioExt(ent), 4, 2048);

        // NOTE: Stream takes ownership of ALLEGRO_FILE.
        // Since we pass a slice, we must still close moduleFile ourselves.
    }
    return NULL;
}

#ifdef CONF_SPEAK
static void speakPart(ALLEGRO_AUDIO_STREAM* stream, const float* segment,
                      bool wait) {
    double start = segment[1];
#if ALLEGRO_VERSION_INT < 0x05020800
    dialogEnd = start + segment[0];
#else
    // Allegro 5.2.8 will internally handle ending stream segments.
    al_set_audio_stream_loop_secs(stream, start, start + segment[0]);
#endif
    if (al_seek_audio_stream_secs(stream, start)) {
        al_set_audio_stream_playing(stream, 1);
        if (wait)
            EventHandler::wait_msecs(int(1000.0f * segment[0]));
    }
}
#endif

void soundSpeakLine(int streamId, int line, bool wait) {
#ifdef CONF_SPEAK
    if (!audioFunctional || !xu4.settings->soundVol || streamId < 1)
        return;

    const float* streamPart = config_voiceParts(streamId);
    if (! streamPart)
        return;
    streamPart += line * 2;
    if (streamPart[0] < 0.3f)           // Ignore NUL entries.
        return;

    // Dialog already loaded
    if (streamId == currentDialog && dialogStream) {
        speakPart(dialogStream, streamPart, wait);
        return;
    }

    if (dialogStream) {
        al_destroy_audio_stream(dialogStream);
        dialogStream = NULL;
        currentDialog = 0;
        dialogEnd = 0.0;
    }

    const CDIEntry* ent = config_musicFile(streamId);
    if (ent)
        dialogStream = moduleAudioStream(ent, &moduleDialog);

    if (! dialogStream) {
        errorWarning("Unable to load dialogue audio stream %d", streamId);
    } else {
        al_attach_audio_stream_to_mixer(dialogStream, fxMixer);
        currentDialog = streamId;
        speakPart(dialogStream, streamPart, wait);
    }
#else
    (void) streamId;
    (void) line;
#endif
}

/*
 * Return duration in milliseconds.
 */
int soundDuration(Sound sound) {
    if (! audioFunctional)
        return 0;
    if (sa_samples[sound] == NULL) {
        if (! sound_load(sound))
            return 0;
    }
    const ALLEGRO_SAMPLE* spl = sa_samples[sound];
    return al_get_sample_length(spl) * 1000 / al_get_sample_frequency(spl);
}

/*
 * Stop all sound effects.  Use musicStop() to halt music playback.
 */
void soundStop() {
    // If audio didn't initialize correctly, then we shouldn't try to stop it
    if (!audioFunctional || !xu4.settings->soundVol)
        return;

    al_stop_samples();
}

/*
 * Start playing a music track.
 *
 * \param newGain   Gain to use if playing from the start.
 *
 * Return true if the stream begins playing from the start.  If the stream
 * is already playing or it cannot be loaded then false is returned.
 */
static bool music_load(int music, float newGain) {
    ASSERT(music < MUSIC_MAX, "Invalid music_load() track id");

    // Track already loaded
    if (music == currentTrack) {
        if (! musicStream)
            return false;       // Handles MUSIC_NONE; nothing to load.

        if (al_get_audio_stream_playing(musicStream))
            return false;       // Already playing; nothing to load.

        // Restart streaming.
        musicGain = newGain;
        al_set_audio_stream_gain(musicStream, musicVolume * musicGain);
        al_rewind_audio_stream(musicStream);
        al_set_audio_stream_playing(musicStream, 1);
        return true;
    }

    if (musicStream) {
        al_destroy_audio_stream(musicStream);
        musicStream = NULL;
    }

#ifdef CONF_MODULE
    const CDIEntry* ent = config_musicFile(music);
    if (ent)
        musicStream = moduleAudioStream(ent, &moduleFile);
#else
    const char* pathname = config_musicFile(music);
    if (! pathname)
        return false;

    musicStream = al_load_audio_stream(pathname, 4, 2048);
#endif

    if (! musicStream) {
        errorWarning("Unable to load music %d", music);
        return false;
    }

    musicGain = newGain;
    al_set_audio_stream_gain(musicStream, musicVolume * musicGain);
    al_set_audio_stream_playmode(musicStream, ALLEGRO_PLAYMODE_LOOP);
    al_attach_audio_stream_to_mixer(musicStream, finalMix);
    currentTrack = music;
    return true;
}

void musicPlay(int track)
{
    if (!audioFunctional || !musicEnabled)
        return;

    if (music_load(track, 1.0))
        musicFade = 0.0;
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

// Private function for Allegro backend to control fading.
void musicUpdate()
{
    if (controlUsed) {
        int bit, i;
        uint32_t now = getTicks();

        // Stop any sounds with a duration limit.
        for (i = 0, bit = 1; i < FX_CONTROL_SLOTS; bit <<= 1, ++i) {
            if ((controlUsed & bit) && (now >= fxDuration[i])) {
                controlUsed &= ~bit;
                al_stop_sample(fxControl + i);
            }
        }
    }

    if (musicStream && musicFade) {
        musicGain += musicFade;
        if (musicGain >= 1.0) {
            musicGain = 1.0;
            musicFade = 0.0;
        } else if (musicGain <= 0.0) {
            musicGain = 0.0;
            musicFade = 0.0;
            al_set_audio_stream_playing(musicStream, 0);
            return;
        }
        al_set_audio_stream_gain(musicStream, musicVolume * musicGain);
    }

#if defined(CONF_SPEAK) && (ALLEGRO_VERSION_INT < 0x05020800)
    // Allegro 5.2.8 will internally handle ending stream segments.
    if (dialogStream && dialogEnd > 0.0) {
        double pos = al_get_audio_stream_position_secs(dialogStream);
        //printf("KR dialog %f %f\n", pos, dialogEnd);

        // al_get_audio_stream_position_secs() reports the stream read
        // position, not the play position, so we must move it back a bit or
        // the segment will get clipped at the end.
        pos -= 0.2;

        if (pos >= dialogEnd) {
            dialogEnd = 0.0;
            al_set_audio_stream_playing(dialogStream, 0);
        }
    }
#endif
}

#define FADE_DELTA(msec)    (1000.0f / msec * screenFrameDuration())

void musicFadeOut(int msec)
{
    // fade the music out even if 'musicEnabled' is false
    if (!audioFunctional)
        return;

    if (musicStream && al_get_audio_stream_playing(musicStream)) {
        if (xu4.settings->volumeFades)
            musicFade = -FADE_DELTA(msec);
        else
            musicStop();
    }
}

void musicFadeIn(int msec, bool loadFromMap)
{
    if (!audioFunctional || !musicEnabled)
        return;

    if (xu4.settings->volumeFades && msec > 0)
        musicFade = FADE_DELTA(msec);
    else
        musicFade = 0.0f;

    if (loadFromMap || ! musicStream) {
#ifdef UNIT_TEST
        int track = 1;
#else
        int track = c->location->map->music;
#endif
        music_load(track, musicFade ? 0.0f : 1.0f);
    } else {
        // If fading is disabled use full volume, otherwise we don't touch
        // the gain on a playing stream.
        if (! musicFade) {
            musicGain = 1.0f;
            al_set_audio_stream_gain(musicStream, musicVolume);
        }
        al_set_audio_stream_playing(musicStream, 1);
    }
}

void musicSetVolume(int volume)
{
    musicVolume = float(volume) / MAX_VOLUME;
    if (musicStream)
        al_set_audio_stream_gain(musicStream, musicVolume);
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

    musicEnabled = ! musicEnabled;
    if (musicEnabled)
        musicFadeIn(1000, true);
    else
        musicFadeOut(1000);

    return musicEnabled;
}

void soundSetVolume(int volume) {
    if (audioFunctional)
        al_set_mixer_gain(fxMixer, float(volume) / MAX_VOLUME);
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


#ifdef UNIT_TEST
// c++ -DUNIT_TEST -g -DDEBUG -I. sound_allegro.cpp -lallegro_acodec -lallegro_audio -lallegro
int main() {
    printf( "al_init: %d\n", al_init());
    printf( "soundInit: %d\n", soundInit());

    for (int i = 1; i < 5; ++i) {
        printf("musicPlay(%d)\n", i);
        musicPlay(i);
        al_rest(4.0);
    }

    soundDelete();
    printf( "Test done.\n" );
}
#endif
