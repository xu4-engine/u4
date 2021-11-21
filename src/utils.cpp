/*
 * $Id$
 */

#if (defined(__unix__) || defined(unix)) && !defined(USG)
#include <sys/param.h>
#endif

#include "utils.h"
#include <cctype>
#include <cstdlib>

#ifdef USE_BORON
#include <boron/boron.h>
#include "config.h"
#include "xu4.h"
#endif

/**
 * Seed the random number generator.
 */
void xu4_srandom(uint32_t seed) {
#ifdef USE_BORON
    boron_randomSeed(xu4.config->boronThread(), seed);
#elif (defined(BSD) && (BSD >= 199103)) || (defined (MACOSX) || defined (IOS))
    srandom(seed);
#else
    srand(seed);
#endif
}

#ifdef REPORT_RNG
char rpos = '-';
#endif

/**
 * Generate a random number between 0 and (upperRange - 1).  This
 * routine uses the upper bits of the random number provided by rand()
 * to compensate for older generators that have low entropy in the
 * lower bits (e.g. MacOS X).
 */
extern "C" int xu4_random(int upperRange) {
#ifdef USE_BORON
#ifdef REPORT_RNG
    uint32_t r = boron_random(xu4.config->boronThread());
    uint32_t n = r % upperRange;
    printf( "KR rn %d %d %c\n", r, n, rpos);
    return n;
#else
    return boron_random(xu4.config->boronThread()) % upperRange;
#endif
#else
#if (defined(BSD) && (BSD >= 199103)) || (defined (MACOSX) || defined (IOS))
    int r = random();
#else
    int r = rand();
#endif
    return (int) ((((double)upperRange) * r) / (RAND_MAX+1.0));
#endif
}

/**
 * Trims whitespace from a std::string
 * @param val The string you are trimming
 * @param chars_to_trim A list of characters that will be trimmed
 */
string& trim(string &val, const string &chars_to_trim) {
    using namespace std;
    string::iterator i;
    if (val.size()) {
        string::size_type pos;
        for (i = val.begin(); (i != val.end()) && (pos = chars_to_trim.find(*i)) != string::npos; )
            i = val.erase(i);
        for (i = val.end()-1; (i != val.begin()) && (pos = chars_to_trim.find(*i)) != string::npos; )
            i = val.erase(i)-1;
    }
    return val;
}

/**
 * Converts the string to lowercase
 */
string& lowercase(string &val) {
    using namespace std;
    string::iterator i;
    for (i = val.begin(); i != val.end(); i++)
        *i = tolower(*i);
    return val;
}

/**
 * Converts the string to uppercase
 */
string& uppercase(string &val) {
    using namespace std;
    string::iterator i;
    for (i = val.begin(); i != val.end(); i++)
        *i = toupper(*i);
    return val;
}

/**
 * Splits a string into substrings, divided by the charactars in
 * separators.  Multiple adjacent seperators are treated as one.
 */
std::vector<string> split(const string &s, const string &separators) {
    std::vector<string> result;
    string current;

    for (unsigned i = 0; i < s.length(); i++) {
        if (separators.find(s[i]) != string::npos) {
            if (current.length() > 0)
                result.push_back(current);
            current.erase();
        } else
            current += s[i];
    }

    if (current.length() > 0)
        result.push_back(current);

    return result;
}
