/*
 * InputRecorder v0.5
 * Copyright (C) 2024  Karl Robillard
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef IRECORD_H
#define IRECORD_H

#include <stdbool.h>
#include <stdint.h>

struct InputRecorder {
    int fd;
    int mode;
    uint32_t replayKey;
    uint32_t clock;
    uint32_t last;
};

typedef struct InputRecorder InputRecorder;

void     irec_init(InputRecorder*);
bool     irec_beginRecording(InputRecorder*, const char* file, uint32_t seed);
void     irec_endRecording(InputRecorder*);
void     irec_recordKey(InputRecorder*, uint16_t key, uint8_t mod);
uint32_t irec_recordedKey(InputRecorder*);
uint32_t irec_replay(InputRecorder*, const char* file);

#define irec_recordTick(rec)    ++(rec)->clock

#define IREC_KEY(rkey)  (rkey & 0xffff)
#define IREC_MOD(rkey)  (rkey >> 16)

#endif
