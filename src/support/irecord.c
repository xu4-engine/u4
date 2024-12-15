/*
 * InputRecorder v0.5
 * Copyright (C) 2024  Karl Robillard
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/*
To replay from inside main loop:

    uint32_t mkey;
    while ((mkey = irec_recordedKey(rec)))
        myApplication_keyPressed(IREC_KEY(mkey), IREC_MOD(mkey));
    irec_recordTick(rec);
*/

#include "irecord.h"

void irec_init(InputRecorder* rec)
{
    rec->fd = -1;
    rec->mode = 0;
}

#include <fcntl.h>

#ifdef _WIN32
#include <io.h>
#include <sys/stat.h>
#define close   _close
#define read    _read
#define write   _write
#else
#include <unistd.h>
#endif

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define RECORD_CDI  0xDA7A4FC0
#else
#define RECORD_CDI  0xC04F7ADA
#endif

#define HDR_SIZE    8

enum RecordMode {
    MODE_DISABLED,
    MODE_RECORD,
    MODE_REPLAY,
};

enum RecordCommand {
    RECORD_NOP,
    RECORD_KEY,
    RECORD_END = 0xff
};

typedef struct {
    uint8_t op, mod;
    uint16_t key;
    uint16_t delay;
} RecordKey;

bool irec_beginRecording(InputRecorder* rec, const char* file, uint32_t seed)
{
    uint32_t head[2];

    rec->clock = rec->last = 0;
    rec->mode = MODE_DISABLED;

    if (rec->fd >= 0)
        close(rec->fd);
#ifdef _WIN32
    rec->fd = _open(file, _O_WRONLY | _O_CREAT, _S_IWRITE);
#else
    rec->fd = open(file, O_WRONLY | O_CREAT,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
#endif
    if (rec->fd < 0)
        return false;

    head[0] = RECORD_CDI;
    head[1] = seed;
    if (write(rec->fd, head, HDR_SIZE) != HDR_SIZE)
        return false;
    rec->mode = MODE_RECORD;
    return true;
}

/**
 * Stop either recording or playback.
 */
void irec_endRecording(InputRecorder* rec) {
    if (rec->fd >= 0) {
        if (rec->mode == MODE_RECORD) {
            char op = RECORD_END;
            write(rec->fd, &op, 1);
        }
        close(rec->fd);
        rec->fd = -1;
        rec->mode = MODE_DISABLED;
    }
}

//void irec_recordMouse(InputRecorder*, int16_t x, int16_t y, uint8_t button)

/*
 * \param key   Key code
 * \param mod   Modifier flags
 */
void irec_recordKey(InputRecorder* rec, uint16_t key, uint8_t mod) {
    if (rec->mode == MODE_RECORD) {
        RecordKey event;
        event.op    = RECORD_KEY;
        event.mod   = mod;
        event.key   = key;
        event.delay = rec->clock - rec->last;

        rec->last = rec->clock;
        write(rec->fd, &event, 6);
    }
}

/**
 * Check for a recorded key press.
 *
 * This can be called in both recording and playback modes.
 * When recording, a zero is always returned.
 *
 * \return Key code & modifiers or zero if no key was pressed.
 *         Use the IREC_KEY & IREC_MOD macros to get the values.
 */
uint32_t irec_recordedKey(InputRecorder* rec) {
    uint32_t key = 0;
    if (rec->mode == MODE_REPLAY) {
        if (rec->replayKey) {
            if (rec->clock >= rec->last) {
                key = rec->replayKey;
                rec->replayKey = 0;
            }
        } else {
            RecordKey event;
            if (read(rec->fd, &event, 6) == 6 && (event.op == RECORD_KEY)) {
                uint32_t fkey = ((uint32_t) event.mod << 16) | event.key;
                if (event.delay)
                    rec->replayKey = fkey;
                else
                    key = fkey;
                rec->last = rec->clock + event.delay;
            } else {
                irec_endRecording(rec);
            }
        }
    }

    return key;
}

/**
 * Begin playback from recorded input file.
 *
 * \return Random seed or zero if the recording file could not be opened.
 */
uint32_t irec_replay(InputRecorder* rec, const char* file) {
    uint32_t head[2];

    rec->clock = rec->last = 0;
    rec->mode = MODE_DISABLED;
    rec->replayKey = 0;

    if (rec->fd >= 0)
        close(rec->fd);
#ifdef _WIN32
    rec->fd = _open(file, _O_RDONLY);
#else
    rec->fd = open(file, O_RDONLY);
#endif
    if (rec->fd < 0)
        return 0;
    if (read(rec->fd, head, HDR_SIZE) != HDR_SIZE || head[0] != RECORD_CDI)
        return 0;

    rec->mode = MODE_REPLAY;
    return head[1];
}
