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
        typedef enum {
            SCALING_MODE_NEAREST,
            SCALING_MODE_LINEAR
        } scaling_mode_t;

        string bios, rom;
        bool pal_emulation;
        unsigned int speed_limit;

        bool debug, debug_on_ill;

        bool opengl;
        unsigned int x_res, y_res;
        bool fullscreen, double_buffering;
        bool keep_aspect;
        scaling_mode_t scaling_mode;

        Joysticks::controls_t controls[2];

        Options();
        void show_usage(const char *progname, ostream &out);
        bool parse(int argc, char **argv);
};

extern Options g_options;

#endif
