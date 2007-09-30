#ifndef UTIL_H
#define UTIL_H

#include "globals.h"

#include <iostream>
#include <iomanip>
#include <string>

using namespace std;

#ifndef HAVE_BZERO
static inline void bzero(void *dst, size_t len)
{
    memset(dst, 0, len * sizeof(char));
}
#endif

template<typename T> void dump_memory(ostream &out, const T mem, size_t len)
{
    out << setfill('0') << hex;

    bool first = true;
    for (size_t i = 0; i < len; ++i) {
        if (first) {
            out << "0x" << setw(2) << i << ": ";
            first = false;
        }

        out << setw(2) << (int)mem[i];
        if ((i + 1) % 16 == 0) {
            out << "\n"; first = true;
        }
        else if ((i + 1) % 8 == 0) {
            out << "  ";
        }
        else {
            out << ' ';
        }
    }
    out.flush();
}

static inline string trim_left(const string &str, const string &delims = " \t\r\n")
{
    string::size_type start = str.find_first_not_of(delims);
    if (start == string::npos)
        return "";
    else
        return str.substr(start);
}

static inline string trim_right(const string &str, const string &delims = " \t\r\n")
{
    // Note that we assumes a left-trimmed string
    string::size_type end = str.find_last_not_of(delims);
    return str.substr(0, end + 1);
}

static inline string trim(const string &str, const string &delims = " \t\r\n")
{
    return trim_right(trim_left(str, delims), delims);
}

#endif
