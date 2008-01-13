#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "common.h"

#include <map>
#include <set>

class Keyboard
{
    private:
        static const SDLKey keymap_[6][8];
        set<SDLKey> possible_keys_;
        map<SDLKey, SDLKey> aliases_;
        SDLKey pressed_;

    public:
        Keyboard();

        SDLKey translate_key(SDLKey key) const;

        void handle_key_down(const SDL_keysym &keysym);
        void handle_key_up(const SDL_keysym &keysym);

        void calculate_p2();
};

extern Keyboard g_keyboard;

inline SDLKey Keyboard::translate_key(SDLKey key) const
{
    set<SDLKey>::const_iterator it = possible_keys_.find(key);
    if (it == possible_keys_.end()) {
        map<SDLKey, SDLKey>::const_iterator it = aliases_.find(key);
        if (it == aliases_.end())
            return SDLK_UNKNOWN;
        else
            key = it->second;
    }

    return key;
}

inline void Keyboard::handle_key_down(const SDL_keysym &keysym)
{
    if (translate_key(keysym.sym) != SDLK_UNKNOWN)
        pressed_ = keysym.sym;
}

inline void Keyboard::handle_key_up(const SDL_keysym &keysym)
{
    if (translate_key(keysym.sym) != SDLK_UNKNOWN)
        pressed_ = SDLK_UNKNOWN;
}

inline void Keyboard::calculate_p2()
{
    if (pressed_ == SDLK_UNKNOWN || g_p1 & (1 << 2)) {
        g_p2 |= 0xf0; // no key pressed or keyboard scan disabled
        return;
    }

    int row = g_p2 & (1 << 0 | 1 << 1 | 1 << 2);
    if (row > 5)
        return;

    int pressed_col = -1;
    for (int col = 0; col < 8; ++col) {
        if (pressed_ == keymap_[row][col]) {
            pressed_col = col ^ (1 << 0 | 1 << 1 | 1 << 2);
            break;
        }
    }

    if (pressed_col == -1)
        g_p2 |= 0xf0;
    else
        g_p2 = (g_p2 & 0x0f) | pressed_col << 5;
}

#endif
