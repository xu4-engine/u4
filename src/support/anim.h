#ifndef ANIM_H
#define ANIM_H

#include <stdint.h>

enum AnimState {
    ANIM_FREE,
    ANIM_PLAYING,
    ANIM_PAUSED
};

#define ANIM_FOREVER    0xffff

typedef uint16_t AnimId;
#define ANIM_UNUSED     0xffff

typedef struct {
    void* bank;
    void (*finish)(void*, uint32_t);
    void* finishData;
    uint32_t avail;
    uint32_t used;
    uint32_t firstFree;
}
Animator;

#ifdef __cplusplus
extern "C" {
#endif

int    anim_init(Animator*, int count, void (*finishFunc)(void*, uint32_t),
                 void* fdata);
void   anim_free(Animator*);
void   anim_clear(Animator*);
void   anim_advance(Animator*, float seconds);
AnimId anim_startCycleRandomI(Animator* an, float dur, int loops, uint32_t fid,
                              int start, int end, int chance);
AnimId anim_startLinearF2(Animator* an, float dur, uint32_t fid,
                          float* start, float* end);
void   anim_setState(Animator*, AnimId, int animState);
int    anim_valueI(const Animator*, AnimId);
float* anim_valueF2(const Animator*, AnimId);

#ifdef __cplusplus
}
#endif

#endif // ANIM_H
