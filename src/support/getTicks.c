/*
 * getTicks - A cross platform timer function.
 */

#ifdef _WIN32
#include <sys/types.h>
#include <sys/timeb.h>
#include <windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

static uint32_t getTicks_start = 0;

// Return milliseconds elapsed since first call to getTicks().
uint32_t getTicks()
{
    uint32_t now;

#ifdef _WIN32
    struct _timeb tb;
    _ftime( &tb );
    now = (uint32_t) (tb.time*1000 + tb.millitm);
#else
    // Android, Linux, iOS, macOS.
    struct timeval ts;
    gettimeofday(&ts, NULL);
    now = (uint32_t) (ts.tv_sec*1000 + ts.tv_usec/1000);
#endif

    if (getTicks_start)
        return now - getTicks_start;
    getTicks_start = now;
    return 0;
}

void msecSleep(uint32_t ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
   struct timespec stime;
   stime.tv_sec  = ms / 1000;
   stime.tv_nsec = (ms - stime.tv_sec*1000) * 1000000;
   nanosleep(&stime, 0);
#endif
}
