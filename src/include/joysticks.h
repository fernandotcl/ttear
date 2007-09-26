#ifndef JOYSTICKS_H
#define JOYSTICKS_H

#include "globals.h"

class Joysticks
{
    private:
        enum {
            JOYSTICK_UP = 0,
            JOYSTICK_RIGHT = 1,
            JOYSTICK_DOWN = 2,
            JOYSTICK_LEFT = 3,
            JOYSTICK_ACTION = 4
        };
        uint8_t buses_[2];

    public:
        struct controls_t
        {
            bool enabled;
            SDLKey up, down, left, right, action;

            void operator=(const controls_t &rhs)
            {
                enabled = rhs.enabled;
                up = rhs.up;
                down = rhs.down;
                left = rhs.left;
                right = rhs.right;
                action = rhs.action;
            }
        };

        Joysticks();

        bool handle_key_down(const SDL_keysym &keysym);
        bool handle_key_up(const SDL_keysym &keysym);

        uint8_t get_bus();
};

inline Joysticks::Joysticks()
{
    buses_[0] = (1 << JOYSTICK_ACTION + 1) - 1;
    buses_[1] = (1 << JOYSTICK_ACTION + 1) - 1;
}

#endif
