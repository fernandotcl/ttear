#include "globals.h"

extern "C" {
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif
}
#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>

#include "options.h"

#include "iniparser.h"
#include "joysticks.h"

#ifdef SVN_REVISION
# define VERSION_STRING "revision " SVN_REVISION
# define SHORT_VERSION_STRING "r" SVN_REVISION
#else
# define VERSION_STRING "version " PACKAGE_VERSION
# define SHORT_VERSION_STRNG "v" PACKAGE_VERSION
#endif

using namespace std;

Options g_options;

Options::Options()
    : pal_emulation(false),
      speed_limit(100),
      debug(false), debug_on_ill(true),
      opengl(true), x_res(640), y_res(480),
      fullscreen(false), double_buffering(true),
      keep_aspect(true), scaling_mode(SCALING_MODE_NEAREST)
{
    controls[0].enabled = true;
    controls[0].left = SDLK_LEFT;
    controls[0].right = SDLK_RIGHT;
    controls[0].up = SDLK_UP;
    controls[0].down = SDLK_DOWN;
    controls[0].action = SDLK_RSHIFT;
    controls[1].enabled = true;
    controls[1].left = SDLK_a;
    controls[1].right = SDLK_d;
    controls[1].up = SDLK_w;
    controls[1].down = SDLK_s;
    controls[1].action = SDLK_SPACE;

    keymap_["left_arrow"] = SDLK_LEFT;
    keymap_["right_arrow"] = SDLK_RIGHT;
    keymap_["up_arrow"] = SDLK_UP;
    keymap_["down_arrow"] = SDLK_DOWN;
    keymap_["spacebar"] = SDLK_SPACE;
    keymap_["left_control"] = SDLK_LCTRL;
    keymap_["right_control"] = SDLK_RCTRL;
    keymap_["left_shift"] = SDLK_LSHIFT;
    keymap_["right_shift"] = SDLK_RSHIFT;
    keymap_["left_alt"] = SDLK_LALT;
    keymap_["right_alt"] = SDLK_RALT;
    keymap_["left_super"] = SDLK_LSUPER;
    keymap_["right_super"] = SDLK_RSUPER;
    keymap_["letter_a"] = SDLK_a;
    keymap_["letter_b"] = SDLK_b;
    keymap_["letter_c"] = SDLK_c;
    keymap_["letter_d"] = SDLK_d;
    keymap_["letter_e"] = SDLK_e;
    keymap_["letter_f"] = SDLK_f;
    keymap_["letter_g"] = SDLK_g;
    keymap_["letter_h"] = SDLK_h;
    keymap_["letter_i"] = SDLK_i;
    keymap_["letter_j"] = SDLK_j;
    keymap_["letter_k"] = SDLK_k;
    keymap_["letter_l"] = SDLK_l;
    keymap_["letter_m"] = SDLK_m;
    keymap_["letter_n"] = SDLK_n;
    keymap_["letter_o"] = SDLK_o;
    keymap_["letter_p"] = SDLK_p;
    keymap_["letter_q"] = SDLK_q;
    keymap_["letter_r"] = SDLK_r;
    keymap_["letter_s"] = SDLK_s;
    keymap_["letter_t"] = SDLK_t;
    keymap_["letter_u"] = SDLK_u;
    keymap_["letter_v"] = SDLK_v;
    keymap_["letter_w"] = SDLK_w;
    keymap_["letter_y"] = SDLK_y;
    keymap_["letter_x"] = SDLK_x;
    keymap_["letter_z"] = SDLK_z;
    keymap_["keypad_0"] = SDLK_KP0;
    keymap_["keypad_1"] = SDLK_KP1;
    keymap_["keypad_2"] = SDLK_KP2;
    keymap_["keypad_3"] = SDLK_KP3;
    keymap_["keypad_4"] = SDLK_KP4;
    keymap_["keypad_5"] = SDLK_KP5;
    keymap_["keypad_6"] = SDLK_KP6;
    keymap_["keypad_7"] = SDLK_KP7;
    keymap_["keypad_8"] = SDLK_KP8;
    keymap_["keypad_9"] = SDLK_KP9;
}

void Options::show_usage(const char *progname, ostream &out)
{
    out << PACKAGE_NAME " " VERSION_STRING " - Odyssey^2/G7000 emulator\n"
           "(C) 2007 - Fernando T. C. Lemos\n"
           "Distributed under the terms of the zlib/libpng license\n"
           "Check the LICENSE file in the source distribution root for details\n"
           "\n"
           "Usage:\n"
           "  " << progname << " [-b <file>] [-c <file>] [-dip] <ROM image>\n"
           "  " << progname << " [-h]\n"
           "  " << progname << " [-V]\n"
           "\n"
           "  The following command line switches are recognized:\n"
           "\n"
#ifdef HAVE_GETOPT_LONG
           "    (-V|--version)       Display version information and exit\n"
           "    (-b|--bios) <file>   Specify what BIOS image file to use\n"
           "    (-c|--config) <file> Read defaults from config file\n"
           "    (-d|--debug)         Start in debug mode\n"
           "    (-i|--invert)        Invert the joystick controls\n"
           "    (-p|--pal)           Use PAL timing instead of NTSC\n"
           "    (-h|--help)          Display this usage information and exit\n"
#else
           "    -V        Display version information and exit\n"
           "    -b <file> Specify what BIOS image file to use\n"
           "    -c <file> Read defaults from config file\n"
           "    -d        Start in debug mode\n"
           "    -i        Invert the joystick controls\n"
           "    -p        Use PAL timing instead of NTSC\n"
           "    -h        Display this usage information and exit\n"
#endif
        << endl;
}

inline SDLKey Options::parse_key(IniParser &parser, const string &name, const string &section)
{
    string keyname;
    if (!parser.get(keyname, name, section))
        return SDLK_UNKNOWN;

    map<string, SDLKey>::const_iterator it = keymap_.find(keyname);
    if (it == keymap_.end()) {
        ostringstream oss;
        oss << "Unknown key \"" << keyname << '"';
        throw(runtime_error(oss.str().c_str()));
    }

    return it->second;
}

bool Options::parse(int argc, char **argv)
{
    string config_file;
    bool bios_touched = false, debug_touched = false, pal_touched = false;
    bool swap_controls = false;

    int c;
#ifdef HAVE_GETOPT_LONG
    static option options[] = {
        { "version", no_argument,       NULL, 'V' },
        { "bios",    required_argument, NULL, 'b' },
        { "config",  required_argument, NULL, 'c' },
        { "debug",   no_argument,       NULL, 'd' },
        { "invert",  no_argument,       NULL, 'i' },
        { "help",    no_argument,       NULL, 'h' },
        { "pal",     no_argument,       NULL, 'p' },
        { NULL,      no_argument,       NULL,  0  }
    };
    while ((c = getopt_long(argc, argv, "b:c:dihpV", options, NULL)) != -1) {
#else
    while ((c = getopt(argc, argv, "b:c:dihpV")) != -1) {
#endif
        switch (c) {
            case 'V':
                cout << SHORT_VERSION_STRING << endl;
                return false;
                break;
            case 'b':
                bios = optarg;
                bios_touched = true;
                break;
            case 'c':
                config_file = optarg;
                break;
            case 'd':
                debug = true;
                debug_touched = true;
                break;
            case 'i':
                swap_controls = true;
                break;
            case 'h':
                show_usage(argv[0], cout);
                return false;
                break;
            case 'p':
                pal_touched = true;
                pal_emulation = true;
                break;
            default:
                show_usage(argv[0], cerr);
                throw(runtime_error("Unknown command line switch"));
                break;
        }
    }

    if (argc - optind == 1) {
        rom = argv[optind];
    }
    else {
        show_usage(argv[0], cerr);
        throw(runtime_error("ROM image file not specified"));
    }

    if (bios.empty()) {
        show_usage(argv[0], cerr);
        throw(runtime_error("BIOS image file not specified"));
    }

    if (!config_file.empty()) {
        IniParser parser(config_file.c_str());
        parser.load();

        // system
        if (!bios_touched)
            parser.get(bios, "bios", "system");
        if (!pal_touched)
            parser.get(pal_emulation, "pal_emulation", "system");
        parser.get(speed_limit, "speed_limit", "system");

        // video
        parser.get(opengl, "opengl", "video");
        {
            string res;
            parser.get(res, "resolution", "video");

            if (!res.empty()) {
                string::size_type x = res.find('x');
                if (x == string::npos || x == 0 || x == res.size() - 1)
                    throw(runtime_error("Invalid resolution setting"));

                string x_str = trim(res.substr(0, x));
                string y_str = trim(res.substr(x + 1));
                istringstream iss;
                iss.str(x_str);
                iss >> x_res;
                if (iss.fail())
                    throw(runtime_error("Invalid X resolution setting"));
                iss.clear();
                iss.str(y_str);
                iss >> y_res;
                if (iss.fail())
                    throw(runtime_error("Invalid Y resolution setting"));
            }
        }
        parser.get(fullscreen, "fullscreen", "video");
        parser.get(double_buffering, "double_buffering", "video");
        parser.get(keep_aspect, "keep_aspect", "video");
        {
            string mode;
            parser.get(mode, "scaling_mode", "video");
            if (!mode.empty()) {
                if (mode == "nearest")
                    scaling_mode = SCALING_MODE_NEAREST;
                else if (mode == "linear")
                    scaling_mode = SCALING_MODE_LINEAR;
                else
                    throw(runtime_error("Invalid scaling mode"));
            }
        }

        // debugger
        if (!debug_touched)
            parser.get(debug, "debug_mode", "debugger");
        parser.get(debug_on_ill, "debug_on_ill", "debugger");

        // controls/playerX
        for (int i = 0; i < 2; ++i) {
            ostringstream oss;
            oss << "controls/player" << (i + 1);
            string section = oss.str();
            parser.get(controls[i].enabled, "enabled", section);

            if (controls[i].enabled) {
                controls[i].left = parse_key(parser, "left", section);
                controls[i].right = parse_key(parser, "right", section);
                controls[i].up = parse_key(parser, "up", section);
                controls[i].down = parse_key(parser, "down", section);
                controls[i].action = parse_key(parser, "action", section);
            }
        }
    }

    if (swap_controls) {
        Joysticks::controls_t temp = controls[0];
        controls[0] = controls[1];
        controls[1] = temp;
    }

    return true;
}
