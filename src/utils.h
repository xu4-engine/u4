/*
 * $Id$
 */

#ifndef UTILS_C
#define UTILS_C

#include <cstdio>
#include <ctime>
#include <map>
#include <string>

using std::string;

#define AdjustValueMax(var, val, max) ((var) += (val)); if ((var) > (max)) (var) = (max)
#define AdjustValueMin(var, val, min) ((var) += (val)); if ((var) < (min)) (var) = (min)
#define AdjustValue(var, val, max, min) ((var) += (val)); if ((var) > (max)) (var) = (max); if ((var) < (min)) (var) = (min)

void xu4_srandom(void);
int xu4_random(int upperval);
void trim(string *val);

class Performance {
    typedef std::map<string, clock_t> TimeMap;
public:
    Performance(const string &s) {
        init(s);
    }

    void init(const string &s) {
        filename = s;
        log = fopen(s.c_str(), "wt");
        if (!log)
            // FIXME: throw exception
            return;
    }

    void reset() {
        if (!log) {
            log = fopen(filename.c_str(), "at");
            if (!log)
                // FIXME: throw exception
                return;
        }
    }

    void start() {
        s = clock();
    }

    void end(const string &funcName) {        
        e = clock();
        times[funcName] = e - s;
    }

    void report(const char *pre = NULL) {
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
    }

private:
    FILE *log;
    string filename;
    clock_t s, e;
    TimeMap times;
};

#endif
