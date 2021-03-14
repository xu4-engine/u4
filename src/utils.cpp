/*
 * $Id$
 */

#include "vc6.h" // Fixes things if you're using VC6, does nothing if otherwise

#if (defined(__unix__) || defined(unix)) && !defined(USG)
#include <sys/param.h>
#endif

#include "utils.h"
#include <cctype>
#include <cstdlib>
#include <ctime>

/**
 * Seed the random number generator.
 */
void xu4_srandom() {
#if (defined(BSD) && (BSD >= 199103)) || (defined (MACOSX) || defined (IOS))
    srandom(time(NULL));
#else
    srand((unsigned int)time(NULL));
#endif
}

/**
 * Generate a random number between 0 and (upperRange - 1).  This
 * routine uses the upper bits of the random number provided by rand()
 * to compensate for older generators that have low entropy in the
 * lower bits (e.g. MacOS X).
 */
int xu4_random(int upperRange) {
#if (defined(BSD) && (BSD >= 199103)) || (defined (MACOSX) || defined (IOS))
    int r = random();
#else
    int r = rand();
#endif
    return (int) ((((double)upperRange) * r) / (RAND_MAX+1.0));
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
 * Converts an integer value to a string
 */
string xu4_to_string(int val) {
    char buffer[16];
    sprintf(buffer, "%d", val);
    return buffer;
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
