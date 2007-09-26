#ifndef OPTIONS_H
#define OPTIONS_H

#include "globals.h"

#include <map>
#include <stdexcept>
#include <string>

#include "iniparser.h"
#include "joysticks.h"

using namespace std;

class Options
{
    private:
        map <string, SDLKey> keymap_;

        SDLKey parse_key(IniParser &parser, const string &name, const string &section);

    public:
        string bios, rom;
        bool pal_emulation;
        unsigned int speed_limit;

        bool debug, debug_on_ill;

        bool opengl, fullscreen;
        unsigned int x_res, y_res;

        Joysticks::controls_t controls[2];

        Options();
        void show_usage(const char *progname, ostream &out);
        bool parse(int argc, char **argv);
};

extern Options g_options;

#endif
