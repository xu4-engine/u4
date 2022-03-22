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

static int64_t getTicks_start = 0;

// Return milliseconds elapsed since first call to getTicks().
uint32_t getTicks()
{
    int64_t now;

#ifdef _WIN32
    struct _timeb tb;
    _ftime( &tb );
    now = (int64_t) tb.time*1000 + tb.millitm;
#elif defined(CLOCK_MONOTONIC)
    // Android, Linux, macOS 10.12
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    now = (int64_t) ts.tv_sec*1000 + ts.tv_nsec/1000000;
#else
    // Older POSIX systems.  POSIX.1-2008 marks gettimeofday() as obsolete.
    struct timeval ts;
    gettimeofday(&ts, NULL);
    now = (int64_t) ts.tv_sec*1000 + ts.tv_usec/1000;
#endif

    if (getTicks_start)
        return (uint32_t) (now - getTicks_start);
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
