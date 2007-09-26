#ifndef INIPARSER_H
#define INIPARSER_H

#include "globals.h"
#include "util.h"

#include <cstring>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <sstream>

using namespace std;

static inline void lowercase(string &str)
{
    for (string::iterator it = str.begin(); it != str.end(); ++it) {
        if (*it >= 'A' && *it <= 'Z')
            *it += 32;
    }
}

static inline void throw_error_at_line(size_t linecnt, const string &msg = "Syntax error")
{
    ostringstream oss;
    oss << msg << " at line " << linecnt << endl;
    throw(runtime_error(oss.str().c_str()));
}

class IniParser
{
    private:
        fstream f_;

        map<string, map<string, string> > sections_;
        char *filename_;

    public:
        IniParser(const char *filename = NULL);
        ~IniParser();

        void set_filename(const char *filename);
        void load();

        template<typename T> bool get(T &out, const string &name, const string &section = "");
};

inline IniParser::IniParser(const char *filename)
    : filename_(NULL)
{
    if (filename)
        filename_ = strdup(filename);
}

inline IniParser::~IniParser()
{
    if (filename_)
        free(filename_);
}

inline void IniParser::set_filename(const char *filename)
{
    assert(filename);
    filename_ = strdup(filename);
}

inline void IniParser::load()
{
    assert(filename_);
    f_.open(filename_, fstream::in);
    if (f_.fail())
        throw(runtime_error("Unable to open file for reading"));

    string line;
    size_t linecnt = 0;
    string section;
    while (getline(f_, line)) {
        ++linecnt;

        line = trim_left(line);
        if (line[0] == ';') {
            continue;
        }
        else if (line[0] == '[') {
            string::size_type pos = line.find(']');
            if (pos == string::npos)
                throw_error_at_line(linecnt);

            section = trim(line.substr(1, pos - 1));
            lowercase(section);
        }
        else {
            line = trim_right(line);
            if (line.empty())
                continue;

            string::size_type separator = line.find('=');
            if (separator == string::npos || separator == 0 || separator == line.size() - 1)
                throw_error_at_line(linecnt);

            string key = trim(line.substr(0, separator));
            string value = trim(line.substr(separator + 1));
            if (key.empty() || value.empty())
                throw_error_at_line(linecnt);

            lowercase(key);
            lowercase(value);
            sections_[section][key] = value;
        }
    }

    f_.close();
}

template<typename T> inline bool IniParser::get(T &out, const string &name, const string &section)
{
    map<string, map<string, string> >::const_iterator it = sections_.find(section);
    if (it == sections_.end())
        return false;

    map<string, string>::const_iterator it2 = it->second.find(name);
    if (it2 == it->second.end())
        return false;

    istringstream iss(it2->second);
    iss.exceptions(istringstream::failbit | istringstream::badbit);
    iss >> out;
    return true;
}

template<> inline bool IniParser::get<bool>(bool &out, const string &name, const string &section)
{
    map<string, map<string, string> >::const_iterator it = sections_.find(section);
    if (it == sections_.end())
        return false;

    map<string, string>::const_iterator it2 = it->second.find(name);
    if (it2 == it->second.end())
        return false;

    out = it2->second == "true" || it2->second == "yes" || it2->second == "on" || it2->second  == "1";
    return true;
}

#endif
