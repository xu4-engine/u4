/*
 * $Id$
 */

#ifndef DEBUG_H
#define DEBUG_H

/*
 * Define XU4_FUNCTION as the function name.  Most compilers define
 * __FUNCTION__.  GCC provides __FUNCTION__ as a variable, not as a
 * macro, so detecting with #if __FUNCTION__ doesn't work.
 */
#if __GNUC__ || __FUNCTION__
# define XU4_FUNCTION __FUNCTION__
#else
# define XU4_FUNCTION ""
#endif

#undef TRACE
#define TRACE(dbg, msg) (dbg).trace(msg, getFilename(__FILE__), XU4_FUNCTION, __LINE__)
#define TRACE_LOCAL(dbg, msg) (dbg).trace(msg, getFilename(__FILE__), XU4_FUNCTION, __LINE__, false);

#include <string>
#include <cstdio>

using std::string;

/*
 * Derived from XINE_ASSERT in the xine project.  I've updated it to
 * be C99 compliant, to use stderr rather than stdout, and to compile
 * out when NDEBUG is set, like a regular assert.  Finally, an
 * alternate ASSERT stub is provided for pre C99 systems.
 */

void print_trace(FILE *file);

#if HAVE_VARIADIC_MACROS
#   ifdef NDEBUG        
#       define ASSERT(exp, desc, ...)  /* nothing */
#   else
#       define ASSERT(exp, ...)                                             \
            do {                                                            \
                if (!(exp)) {                                               \
                    fprintf(stderr, "%s:%s:%d: assertion `%s' failed. ",    \
                           __FILE__, XU4_FUNCTION, __LINE__, #exp);         \
                    fprintf(stderr, __VA_ARGS__);                           \
                    fprintf(stderr, "\n\n");                                \
                    print_trace(stderr);                                    \
                    abort();                                                \
                }                                                           \
            } while(0)
#   endif /* ifdef NDEBUG */
#else

void ASSERT(int exp, const char *desc, ...);

#endif /* if HAVE_VARIADIC_MACROS */



string getFilename(const string &path);

/**
 * Provides trace functionality to debug apps.
 */
class Debug {
public:
    Debug(const string &filename, const string &name = "", bool append = false);

    static void initGlobal(const string &filename);
    void trace(const string &msg, const string &file = "", const string &func = "", const int line = -1, bool glbl = true);

private:        
    static bool loggingEnabled(const string &name);

    bool disabled;
    string filename, name;
    FILE *file;
    static FILE *global; 

    string l_filename, l_func;
    int l_line;
};

#endif /* ifndef DEBUG_H */


