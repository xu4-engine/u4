/*
 * $Id$
 */

#ifndef UTILS_H
#define UTILS_H

#include <cstdio>
#include <ctime>
#include <map>
#include <string>
#include <vector>

#include "filesystem.h"

using std::string;

/* The AdjustValue functions used to be #define'd macros, but these are
 * evil for several reasons, *especially* when they contain multiple
 * statements, and have if statements in them. The macros did both.
 * See http://www.parashift.com/c++-faq-lite/inline-functions.html#faq-9.5
 * for more information.
 */
inline void AdjustValueMax(int &v, int val, int max) { v += val; if (v > max) v = max; }
inline void AdjustValueMin(int &v, int val, int min) { v += val; if (v < min) v = min; }
inline void AdjustValue(int &v, int val, int max, int min) { v += val; if (v > max) v = max; if (v < min) v = min; }

inline void AdjustValueMax(short &v, int val, int max) { v += val; if (v > max) v = max; }
inline void AdjustValueMin(short &v, int val, int min) { v += val; if (v < min) v = min; }
inline void AdjustValue(short &v, int val, int max, int min) { v += val; if (v > max) v = max; if (v < min) v = min; }

inline void AdjustValueMax(unsigned short &v, int val, int max) { v += val; if (v > max) v = max; }
inline void AdjustValueMin(unsigned short &v, int val, int min) { v += val; if (v < min) v = min; }
inline void AdjustValue(unsigned short &v, int val, int max, int min) { v += val; if (v > max) v = max; if (v < min) v = min; }

void xu4_srandom(void);
int xu4_random(int upperval);
string& trim(string &val, const string &chars_to_trim = "\t\013\014 \n\r");
string& lowercase(string &val);
string& uppercase(string &val);
string  xu4_to_string(int val);
std::vector<string> split(const string &s, const string &separators);

class Performance {
    typedef std::map<string, clock_t> TimeMap;
public:
    Performance(const string &s) {
#ifndef NPERF
        init(s);
#endif
    }

    void init(const string &s) {
#ifndef NPERF
        Path path(s);
        FileSystem::createDirectory(path);

        filename = path.getPath();
        log = fopen(filename.c_str(), "wt");
        if (!log)
            // FIXME: throw exception
            return;
#endif
    }

    void reset() {
#ifndef NPERF
        if (!log) {
            log = fopen(filename.c_str(), "at");
            if (!log)
                // FIXME: throw exception
                return;
        }
#endif
    }

    void start() {
#ifndef NPERF
        s = clock();
#endif
    }

    void end(const string &funcName) {
#ifndef NPERF
        e = clock();
        times[funcName] = e - s;
#endif
    }

    void report(const char *pre = NULL) {
#ifndef NPERF
        static const double msec = double(CLOCKS_PER_SEC) / double(1000);
        TimeMap::const_iterator i;
        clock_t total = 0;
        std::map<double, string> percentages;
        std::map<double, string>::iterator perc;

        if (pre)
            fprintf(log, "%s", pre);

        for (i = times.begin(); i != times.end(); i++) {
            fprintf(log, "%s [%0.2f msecs]\n", i->first.c_str(), double(i->second) / msec);
            total += i->second;
        }

        for (i = times.begin(); i != times.end(); i++) {
            double perc = 100.0 * double(i->second) / total;
            percentages[perc] = i->first;
        }

        fprintf(log, "\n");
        for (perc = percentages.begin(); perc != percentages.end(); perc++)
            fprintf(log, "%0.1f%% - %s\n", perc->first, perc->second.c_str());

        fprintf(log, "\nTotal [%0.2f msecs]\n", double(total) / msec);

        fclose(log);
        log = NULL;
        times.clear();
#endif
    }

private:
    FILE *log;
    string filename;
    clock_t s, e;
    TimeMap times;
};

#endif
