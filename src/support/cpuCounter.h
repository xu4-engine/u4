#ifndef CPUCOUNTER_H
#define CPUCOUNTER_H


#include <stdint.h>


#if defined(__GNUC__)
#if defined(__x86_64__)
#define HAVE_CPU_COUNTER
static __inline__ uint64_t cpuCounter()
{
    uint32_t lo, hi;
    /* We cannot use "=A", since this would use %rax on x86_64 */
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return (uint64_t) hi << 32 | lo;
}
#elif defined(__i386__)
#define HAVE_CPU_COUNTER
static __inline__ uint64_t cpuCounter()
{
    uint64_t c;
    __asm__ __volatile__ (".byte 0x0f, 0x31" : "=A" (c));
    return c;
}
#elif defined(__powerpc__)
#define HAVE_CPU_COUNTER
static __inline__ uint64_t cpuCounter()
{
    uint32_t lo, hi, t;
    __asm__ __volatile__ (
        "0:\n"
        "\tmftbu  %0\n"
        "\tmftb   %1\n"
        "\tmftbu  %2\n"
        "\tcmpw   %2,%0\n"
        "\tbne    0b\n"
        : "=r" (hi), "=r" (lo), "=r" (t)
    );
    return (uint64_t) hi << 32 | lo;
}
#endif
#endif


#ifdef CPU_TEST
#define CPU_START()    uint64_t pc0 = cpuCounter();
#define CPU_END(msg)   printf(msg " %ld\n", cpuCounter() - pc0);
#else
#define CPU_START()
#define CPU_END(msg)
#endif


#endif /*CPUCOUNTER_H*/
