/*
 * $Id$
 */

#ifndef UTILS_C
#define UTILS_C

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The following was taken from the Apache httpd core module source code
 * and defines the XU4_OFFSETOF() macro.
 *
 * {{{ Modified Apache source
 */ 
#if defined(CRAY) || (defined(__arm) && !defined(LINUX))
    #ifdef __STDC__
        #define XU4_OFFSET(p_type,field) _Offsetof(p_type,field)
    #else
        #ifdef CRAY2
            #define XU4_OFFSET(p_type,field) \
                    (sizeof(int)*((unsigned int)&(((p_type)NULL)->field)))
        #else /* !CRAY2 */
            #define XU4_OFFSET(p_type,field) ((unsigned int)&(((p_type)NULL)->field))
        #endif /* !CRAY2 */
    #endif /* __STDC__ */
#else /* ! (CRAY || __arm) */
    #define XU4_OFFSET(p_type,field) \
            ((long) (((char *) (&(((p_type)NULL)->field))) - ((char *) NULL)))
#endif /* !CRAY */

/**
 * Finding offsets of elements within structures.
 * @param s_type structure type name
 * @param field  data field within the structure
 * @return offset
 */
#if defined(offsetof) && !defined(__cplusplus)
    #define XU4_OFFSETOF(s_type,field) offsetof(s_type,field)
#else
    #define XU4_OFFSETOF(s_type,field) XU4_OFFSET(s_type*,field)
#endif
/**
 * }}} End
 */

#define AdjustValueMax(var, val, max) ((var) += (val)); if ((var) > (max)) (var) = (max)
#define AdjustValueMin(var, val, min) ((var) += (val)); if ((var) < (min)) (var) = (min);
#define AdjustValue(var, val, max, min) ((var) += (val)); if ((var) > (max)) (var) = (max); if ((var) < (min)) (var) = (min);

char *concat(const char *str, ...);
int strcmp_i(const char *str1, const char *str2);
void xu4_srandom(void);
int xu4_random(int upperval);

#ifdef __cplusplus
}
#endif

#endif
