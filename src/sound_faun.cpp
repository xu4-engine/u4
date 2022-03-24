/*
 * sound_faun.cpp
 */

#include <faun.h>
#include <string.h>
#include "sound.h"

#include "config.h"
#include "context.h"
#include "debug.h"
#include "error.h"
#include "event.h"
#include "settings.h"
#include "xu4.h"

#define config_soundFile(id)    xu4.config->soundFile(id)
#define config_musicFile(id)    xu4.config->musicFile(id)
#define config_voiceParts(id)   xu4.config->voiceParts(id)
#define BUFFER_LIMIT    32
#define SOURCE_LIMIT    8
#define SID_END         7
#define SID_MUSIC   SOURCE_LIMIT
#define SID_SPEECH  SOURCE_LIMIT+1
#define BUFFER_MS_FAILED    1

static int currentTrack;
static int currentDialog;
static int musicEnabled;
static int nextSource;              // Use sources in round-robin order.
static int musicFadeMs;             // Minimizes calls to faun_setParameter.
static float soundVolume = 0.0f;
static float musicVolume = 0.0f;
static uint16_t bufferMs[BUFFER_LIMIT];

/*
 * Initialize sound & music service.
 */
int soundInit()
{
    const char* error;

    currentTrack = MUSIC_NONE;
    currentDialog = 0;
    nextSource = 0;
    musicFadeMs = 0;
    memset(bufferMs, 0, sizeof(bufferMs));

    error = faun_startup(BUFFER_LIMIT, SOURCE_LIMIT, 2, 0, "xu4");
    if (error) {
        errorWarning("Faun: %s", error);
        soundVolume = musicVolume = 0.0f;
        return 0;
    }

    musicEnabled = 1;
    musicSetVolume(xu4.settings->musicVol);
    soundSetVolume(xu4.settings->soundVol);
    return 1;
}

void soundDelete()
{
    faun_shutdown();
}

static int loadSoundBuffer(int sound)
{
    float duration = 0.0f;
    uint16_t ms;

#ifdef CONF_MODULE
    const CDIEntry* ent = config_soundFile(sound);
    if (ent)
        duration = faun_loadBuffer(sound, xu4.config->modulePath(ent),
                                   ent->offset, ent->bytes);
#else
    const char* pathname = config_soundFile(sound);
    if (pathname)
        duration = faun_loadBuffer(sound, pathname, 0, 0);
#endif

    if (duration)
        ms = (uint16_t) (duration * 1000.0f);
    else {
        // Mark buffer as loaded even upon failure so we don't keep trying
        // to load bad or nonexistent data.
        ms = BUFFER_MS_FAILED;
    }

    bufferMs[sound] = ms;
    return ms;
}

void soundPlay(Sound sound, bool onlyOnce, int limitMSec)
{
    (void) onlyOnce;
    ASSERT(sound < SOUND_MAX, "Invalid soundPlay() id");

    // Do nothing if muted or soundInit failed.
    if (soundVolume <= 0.0f)
        return;

    if (bufferMs[sound] == 0)
        loadSoundBuffer(sound);

    // The source SID_END is reserved for when limitMSec is used so that
    // FAUN_END_TIME doesn't need to be reset.  This assumes that limitMSec
    // is rarely used.

    if (limitMSec > 0) {
        faun_playSource(SID_END, sound, FAUN_PLAY_ONCE);
        faun_setParameter(SID_END, 1, FAUN_END_TIME,
                          (float) limitMSec / 1000.0f);
    } else {
        faun_playSource(nextSource, sound, FAUN_PLAY_ONCE);
        if (++nextSource >= SID_END)
            nextSource = 0;
    }
}

/*
 * Play a line of spoken dialogue.
 */
void soundSpeakLine(int streamId, int line, bool wait) {
#ifdef CONF_MODULE
    if (soundVolume <= 0.0f || streamId < 1)
        return;

    const float* streamPart = config_voiceParts(streamId);
    if (! streamPart)
        return;
    streamPart += line * 2;
    if (streamPart[0] < 0.3f)           // Ignore NUL entries.
        return;

    //printf("KR soundSpeakLine %d %d %f,%f\n",
    //       streamId, line, streamPart[0], streamPart[1]);

    if (streamId == currentDialog)
        goto speak;     // Dialogue already loaded

    currentDialog = 0;
    {
    const CDIEntry* ent = config_musicFile(streamId);
    if (ent) {
        faun_playStream(SID_SPEECH, xu4.config->modulePath(ent),
                        ent->offset, ent->bytes, 0);
        currentDialog = streamId;
        goto speak;
    }
    }

    errorWarning("Dialogue audio stream %d not found", streamId);
    return;

speak:
    faun_playStreamPart(SID_SPEECH, streamPart[1], streamPart[0],
                        FAUN_PLAY_ONCE);
    if (wait)
        EventHandler::wait_msecs(int(1000.0f * streamPart[0]));
#else
    (void) streamId;
    (void) line;
#endif
}

/*
 * Return duration in milliseconds.
 */
int soundDuration(Sound sound)
{
    int ms = bufferMs[sound];
    if (ms == 0)
        ms = loadSoundBuffer(sound);
    if (ms == BUFFER_MS_FAILED)
        return 0;
    return ms;
}

/*
 * Stop all sound effects.  Use musicStop() to halt music playback.
 */
void soundStop() {
    faun_control(0, SOURCE_LIMIT, FC_STOP);
}

/*
 * Start playing a music track.
 *
 * Return true if the stream begins playing from the start.  If the stream
 * is already playing or it cannot be loaded then false is returned.
 */
static bool music_start(int music, int mode) {
    ASSERT(music < MUSIC_MAX, "Invalid music_start() track id");

    // Track already loaded
    if (music == currentTrack)
        return false;
#if 0
    {
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
#endif

#ifdef CONF_MODULE
    const CDIEntry* ent = config_musicFile(music);
    if (ent)
        faun_playStream(SID_MUSIC, xu4.config->modulePath(ent),
                        ent->offset, ent->bytes, mode);
#else
    const char* pathname = config_musicFile(music);
    if (! pathname)
        return false;
    faun_playStream(SID_MUSIC, pathname, 0, 0, mode);
#endif

    currentTrack = music;
    return true;
}

void musicPlay(int track)
{
    if (musicEnabled && musicVolume > 0.0f)
        music_start(track, FAUN_PLAY_LOOP);
}

void musicPlayLocale()
{
    musicPlay(c->location->map->music);
}

void musicStop()
{
    faun_control(SID_MUSIC, 1, FC_STOP);
}

static void music_setFadePeriod(int msec)
{
    if (musicFadeMs != msec) {
        musicFadeMs = msec;
        faun_setParameter(SID_MUSIC, 1, FAUN_FADE_PERIOD, msec/1000.0f);
    }
}

void musicFadeOut(int msec)
{
    if (currentTrack != MUSIC_NONE) {
        currentTrack = MUSIC_NONE;
        if (xu4.settings->volumeFades) {
            music_setFadePeriod(msec);
            faun_control(SID_MUSIC, 1, FC_FADE_OUT);
        } else
            musicStop();
    }
}

void musicFadeIn(int msec, bool loadFromMap)
{
    int mode = FAUN_PLAY_LOOP;

    if (xu4.settings->volumeFades && msec > 0) {
        music_setFadePeriod(msec);
        mode |= FAUN_PLAY_FADE_IN;
    }

    if (loadFromMap || currentTrack == MUSIC_NONE)
        music_start(c->location->map->music, mode);
    /*
    else {
        // FC_FADE_IN doesn't exist.
        faun_control(SID_MUSIC, 1, FC_FADE_IN);
    }
    */
}

void musicUpdate() {}

void musicSetVolume(int volume)
{
    musicVolume = float(volume) / MAX_VOLUME;
    faun_setParameter(SID_MUSIC, 1, FAUN_VOLUME, musicVolume);
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
    musicEnabled = ! musicEnabled;
    if (musicEnabled)
        musicFadeIn(1000, true);
    else
        musicFadeOut(1000);

    return musicEnabled;
}

void soundSetVolume(int volume) {
    soundVolume = float(volume) / MAX_VOLUME;
    faun_setParameter(0, SOURCE_LIMIT, FAUN_VOLUME, soundVolume);
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
