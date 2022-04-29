#include <stdio.h>
#include <stdlib.h>
#include "anim.h"

#ifdef CONFIG_ANIM_RANDOM
// This function provided by the user returns an integer between
// 0 (inclusive) and range (exclusive).
extern int anim_random(int range);
#endif

enum AnimType {
    ANIM_CYCLE_I,
    ANIM_CYCLE_RANDOM_I,
    ANIM_LINEAR_F2,
};

typedef struct {
    uint16_t animType;
    uint16_t state;
    uint16_t loops;
    uint16_t _pad;
    uint32_t finishId;
    float duration;
    float ctime;
    union {
        struct {
            int start;
            int end;
            int current;
            int chance;
        } i;
        struct {
            float start[2];
            float end[2];
            float current[2];
        } f2;
    } var;
}
AnimatedValue;

#define BANK(an)        ((AnimatedValue*) an->bank)
#define ANIM_FINISHED   (ANIM_PAUSED+1)

#define FREE_TERM   0xffff
#define NEXT_FREE   finishId

static void anim_nopFinish(void* fdata, uint32_t fid)
{
    (void) fdata;
    (void) fid;
}

/*
 * Allocate memory for the specified maximum number of animated values.
 *
 * Return non-zero if memory allocation is successful.
 */
int anim_init(Animator* an, int count, void (*finishFunc)(void*, uint32_t),
              void* fdata)
{
    an->finish = finishFunc ? finishFunc : anim_nopFinish;
    an->finishData = fdata;
    an->bank = malloc(sizeof(AnimatedValue) * count);
    an->avail = an->bank ? count : 0;
    anim_clear(an);
    return an->avail;
}

/*
 * Free memory for all animated values.
 */
void anim_free(Animator* an)
{
    free(an->bank);
    an->bank = NULL;
    an->avail = an->used = 0;
}

/*
 * Stop all animations and return them to the free pool.
 */
void anim_clear(Animator* an)
{
    an->used = an->firstFree = 0;
    if (an->avail) {
        AnimatedValue* it = BANK(an);
        int end = an->avail - 1;
        int i;

        for (i = 1; i < end; ++i) {
            it->NEXT_FREE = i;      // Point to next free value.
            it->state = ANIM_FREE;
            ++it;
        }
        it->NEXT_FREE = FREE_TERM;
        it->state = ANIM_FREE;
    }
}

static AnimId anim_alloc(Animator* an)
{
    AnimId id = an->firstFree;
    if (id != FREE_TERM) {
        an->firstFree = BANK(an)[id].NEXT_FREE;
        if (id >= an->used)
            an->used = id + 1;
    }
    return id;
}

static void anim_release(Animator* an, AnimatedValue* it)
{
    AnimatedValue* bank = BANK(an);
    int itPos, last;

    it->state = ANIM_FREE;

    // Link into the free list.
    it->NEXT_FREE = an->firstFree;
    an->firstFree = itPos = it - bank;

    // Adjust used downward to the next active value.
    last = an->used - 1;
    if (itPos == last) {
        do {
            --last;
        } while (last >= 0 && bank[last].state == ANIM_FREE);
        an->used = last + 1;
    }
}

static float lerp(float start, float end, float frac)
{
    return start + (end - start) * frac;
}

/*
 * Advance all playing animations by the given time.
 */
void anim_advance(Animator* an, float seconds)
{
    AnimatedValue* it  = BANK(an);
    AnimatedValue* end = it + an->used;
    AnimatedValue* firstDone = NULL;
    AnimatedValue* endDone = NULL;

    for ( ; it != end; ++it) {
        if (it->state == ANIM_PLAYING) {
            // Advance time.
            it->ctime += seconds;
            if (it->ctime >= it->duration) {
                // Cycle complete.

                if (it->loops == ANIM_FOREVER) {
                    it->ctime -= it->duration;
                } else {
                    --it->loops;
                    if (it->loops) {
                        it->ctime -= it->duration;
                    } else {
                        it->state = ANIM_FINISHED;
                        if (! firstDone)
                            firstDone = it;
                        endDone = it + 1;
                    }
                }

#ifdef CONFIG_ANIM_RANDOM
                if (it->animType == ANIM_CYCLE_RANDOM_I) {
                    if (it->var.i.chance > anim_random(100)) {
                        int n = it->var.i.current + 1;
                        it->var.i.current =
                            (n < it->var.i.end) ? n : it->var.i.start;
                    }
                    continue;
                }
#endif
            }

            // Update value.
            switch (it->animType) {
                case ANIM_LINEAR_F2:
                    if (it->state == ANIM_FINISHED) {
                        it->var.f2.current[0] = it->var.f2.end[0];
                        it->var.f2.current[1] = it->var.f2.end[1];
                    } else {
                        float frac = it->ctime / it->duration;
                        it->var.f2.current[0] = lerp(it->var.f2.start[0],
                                                     it->var.f2.end[0], frac);
                        it->var.f2.current[1] = lerp(it->var.f2.start[1],
                                                     it->var.f2.end[1], frac);
                    }
                    break;
            }
        }
    }

    // Invoke the finish handler and release the slot for completed animations.
    while (firstDone != endDone) {
        // This iterates backwards so that the last released slot has the
        // lowest firstFree index of the batch.  This sets up the next
        // anim_alloc to produce a lower Animator::used value.

        --endDone;
        if (endDone->state == ANIM_FINISHED) {
            if (endDone->finishId)
                an->finish(an->finishData, endDone->finishId);
            anim_release(an, endDone);
        }
    }
}

static void anim_stdInit(AnimatedValue* it, float dur, int loops, uint32_t fid)
{
    it->state    = ANIM_PLAYING;
    it->loops    = loops;
    it->_pad     = 0;
    it->finishId = fid;
    it->duration = dur;
    it->ctime    = 0.0f;
}

#ifdef CONFIG_ANIM_RANDOM
AnimId anim_startCycleRandomI(Animator* an, float dur, int loops, uint32_t fid,
                              int start, int end, int chance)
{
    AnimId id = anim_alloc(an);
    if (id != FREE_TERM) {
        AnimatedValue* it = BANK(an) + id;
        it->animType = ANIM_CYCLE_RANDOM_I;
        anim_stdInit(it, dur, loops, fid);
        it->var.i.start  = start;
        it->var.i.end    = end;
        it->var.i.current = start;
        it->var.i.chance = chance;

        //printf("anim_start %d dur:%f chance:%d\n", id, dur, chance);
    }
    return id;
}
#endif

AnimId anim_startLinearF2(Animator* an, float dur, uint32_t fid,
                          float* start, float* end)
{
    AnimId id = anim_alloc(an);
    if (id != FREE_TERM) {
        AnimatedValue* it = BANK(an) + id;
        it->animType = ANIM_LINEAR_F2;
        anim_stdInit(it, dur, 1, fid);
        it->var.f2.start[0] = it->var.f2.current[0] = start[0];
        it->var.f2.start[1] = it->var.f2.current[1] = start[1];
        it->var.f2.end[0]   = end[0];
        it->var.f2.end[1]   = end[1];
    }
    return id;
}

/*
 * Pause, unpause, or stop an animation.
 */
void anim_setState(Animator* an, AnimId id, int animState)
{
    AnimatedValue* it = BANK(an) + id;
    if (animState == ANIM_FREE) {
        if (it->state != ANIM_FREE)
            anim_release(an, it);
    } else
        it->state = animState;
}

int anim_valueI(const Animator* an, AnimId id)
{
    return BANK(an)[id].var.i.current;
}

float* anim_valueF2(const Animator* an, AnimId id)
{
    return BANK(an)[id].var.f2.current;
}
