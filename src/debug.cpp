/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "debug.h"
#include "settings.h"
#include "utils.h"

#if HAVE_BACKTRACE
#include <execinfo.h>

using std::vector;

/**
 * Get a backtrace and print it to the file.  Note that gcc requires
 * the -rdynamic flag to have access to the actual backtrace symbols;
 * otherwise they will be simple hex offsets.
 */
void print_trace(FILE *file) {
    /* Code Taken from GNU C Library manual */
    void *array[10];
    size_t size;
    char **strings;
    size_t i;

    size = backtrace(array, 10);
    strings = backtrace_symbols(array, size);

    fprintf(file, "Stack trace:\n");

    /* start at one to omit print_trace */
    for (i = 1; i < size; i++) {
        fprintf(file, "%s\n", strings[i]);
    }
    free(strings);
}

#else

/**
 * Stub for systems without access to the stack backtrace.
 */
void print_trace(FILE *file) {
    fprintf(file, "Stack trace not available\n");
}

#endif

#if !HAVE_VARIADIC_MACROS

#include <cstdarg>

/**
 * Stub for systems without variadic macros.  Unfortunately, this
 * assert won't be very useful.
 */
void ASSERT(int exp, const char *desc, ...) {
#ifndef NDEBUG
    va_list args;
    va_start(args, desc);

    if (!exp) {
        fprintf(stderr, "Assertion failed: ");
        vfprintf(stderr, desc, args);
        fprintf(stderr, "\n");
        abort();
    }

    va_end(args);
#endif
}

#endif

/**
 * FIXME: this should go somewhere else later -- probably in a class
 */ 
string getFilename(const string &path) {
    unsigned int pos = path.find_last_of("/");
    if (pos >= path.size())
        pos = path.find_last_of("\\");
    if (pos >= path.size())
        return path;
    else
        return path.substr(pos+1);    
}

FILE *Debug::global = NULL;

Debug::Debug(const string &fn, const string &nm, bool append) : disabled(false), filename(fn), name(nm) {
    if (!loggingEnabled(name)) {
        disabled = true;
        return;
    }

    if (append)
        file = fopen(filename.c_str(), "at");
    else file = fopen(filename.c_str(), "wt");        

    if (!file) {} // FIXME: throw exception here
    else if (!name.empty())
        fprintf(file, "=== %s ===\n", name.c_str());
}

void Debug::initGlobal(const string &filename) {
    if (global)
        fclose(global);
    global = fopen(filename.c_str(), "wt");    
    if (!global) {} // FIXME: throw exception here
}

void Debug::trace(const string &msg, const string &filename, const string &func, const int line, bool glbl) {
    if (disabled)
        return;

    bool brackets = false;
    string message;

    if (!file)
        return;
    
    if (!msg.empty())
        message += msg;        
    
    if (!filename.empty() || line > 0) {
        brackets = true;
        message += " [";        
    }

    if ((l_filename == filename) && (l_func == func) && (l_line == line))
        message += "...";
    else {
        if (!func.empty()) {
            l_func = func;
            message += func + "() - ";
        }
        else l_func.empty();

        if (!filename.empty()) {
            l_filename = filename;
            message += filename + ": ";
        }
        else l_filename.empty();

        if (line > 0) {
            l_line = line;
            char ln[8];
            sprintf(ln, "%d", line);
            message += "line ";
            message += ln;        
        }
        else l_line = -1;
    }

    if (brackets)
        message += "]";
    message += "\n";
    
    fprintf(file, message.c_str());
    if (global && glbl)
        fprintf(global, "%12s: %s", name.c_str(), message.c_str());
}

bool Debug::loggingEnabled(const string &name) {
    if (settings.logging == "all")
        return true;

    vector<string> enabledLogs = split(settings.logging, ", ");
    if (find(enabledLogs.begin(), enabledLogs.end(), name) != enabledLogs.end())
        return true;

    return false;
}
