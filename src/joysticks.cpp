#include "common.h"

#include "joysticks.h"

#include "options.h"

bool Joysticks::handle_key_down(const SDL_keysym &keysym)
{
    for (int i = 0; i < 2; ++i) {
        if (!g_options.controls[i].enabled)
            continue;

        if (keysym.sym == g_options.controls[i].up) {
            buses_[i] &= ~(1 << JOYSTICK_UP);
            return true;
        }
        else if (keysym.sym == g_options.controls[i].down) {
            buses_[i] &= ~(1 << JOYSTICK_DOWN);
            return true;
        }
        else if (keysym.sym == g_options.controls[i].left) {
            buses_[i] &= ~(1 << JOYSTICK_LEFT);
            return true;
        }
        else if (keysym.sym == g_options.controls[i].right) {
            buses_[i] &= ~(1 << JOYSTICK_RIGHT);
            return true;
        }
        else if (keysym.sym == g_options.controls[i].action) {
            buses_[i] &= ~(1 << JOYSTICK_ACTION);
            return true;
        }
    }

    return false;
}

bool Joysticks::handle_key_up(const SDL_keysym &keysym)
{
    for (int i = 0; i < 2; ++i) {
        if (!g_options.controls[i].enabled)
            continue;

        if (keysym.sym == g_options.controls[i].up) {
            buses_[i] |= 1 << JOYSTICK_UP;
            return true;
        }
        else if (keysym.sym == g_options.controls[i].down) {
            buses_[i] |= 1 << JOYSTICK_DOWN;
            return true;
        }
        else if (keysym.sym == g_options.controls[i].left) {
            buses_[i] |= 1 << JOYSTICK_LEFT;
            return true;
        }
        else if (keysym.sym == g_options.controls[i].right) {
            buses_[i] |= 1 << JOYSTICK_RIGHT;
            return true;
        }
        else if (keysym.sym == g_options.controls[i].action) {
            buses_[i] |= 1 << JOYSTICK_ACTION;
            return true;
        }
    }

    return false;
}

uint8_t Joysticks::get_bus()
{
    if (!(g_p1 & 1 << 3 && g_p2 & 1 << 4))
        return 0;

    int index = g_p2 & (1 << 0 | 1 << 1 | 1 << 2);
    if (index == 0)
        return buses_[0];
    else if (index == 1)
        return buses_[1];
    else
        return 0;
}
