/*
 * performance.h
 */

#ifdef ENABLE_PERF
#define PERF_OBJ(V,N)       Performance V(N);
#define PERF_START(V)       V.start();
#define PERF_END(V,F)       V.end(F);
#define PERF_REPORT(V,P)    V.report(P);
#define PERF_RESET(V)       V.reset();
#else
#define PERF_OBJ(V,N)
#define PERF_START(V)
#define PERF_END(V,F)
#define PERF_REPORT(V,P)
#define PERF_RESET(V)
#endif

class Performance {
    typedef std::map<string, clock_t> TimeMap;
public:
    Performance(const string &s) {
        init(s);
    }

    void init(const string &s) {
        Path path(s);
        FileSystem::createDirectory(path);

        filename = path.getPath();
        log = fopen(filename.c_str(), "wt");
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
