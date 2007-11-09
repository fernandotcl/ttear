#include "common.h"

#include "keyboard.h"

const SDLKey Keyboard::keymap_[6][8] = {
    {SDLK_0, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7},
    {SDLK_8, SDLK_9, SDLK_UNKNOWN, SDLK_UNKNOWN, SDLK_SPACE, SDLK_QUESTION, SDLK_l, SDLK_p},
    {SDLK_PLUS, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_u, SDLK_i, SDLK_o},
    {SDLK_q, SDLK_s, SDLK_d, SDLK_f, SDLK_g, SDLK_h, SDLK_j, SDLK_k},
    {SDLK_a, SDLK_z, SDLK_x, SDLK_c, SDLK_v, SDLK_b, SDLK_m, SDLK_PERIOD},
    {SDLK_KP_MINUS, SDLK_KP_MULTIPLY, SDLK_KP_DIVIDE, SDLK_EQUALS, SDLK_y, SDLK_n, SDLK_BACKSPACE, SDLK_KP_ENTER}
};

Keyboard::Keyboard()
    : pressed_(SDLK_UNKNOWN)
{
    // Fill the set of possible keys
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (keymap_[i][j] != SDLK_UNKNOWN)
                possible_keys_.insert(keymap_[i][j]);
        }
    }

    aliases_[SDLK_SLASH] = SDLK_QUESTION; // the slash is generally below the question mark
    aliases_[SDLK_PLUS] = SDLK_KP_PLUS;
    aliases_[SDLK_PERIOD] = SDLK_KP_PERIOD;
    aliases_[SDLK_MINUS] = SDLK_KP_MINUS;
    aliases_[SDLK_ASTERISK] = SDLK_KP_MULTIPLY;
    aliases_[SDLK_DELETE] = SDLK_BACKSPACE;
    aliases_[SDLK_RETURN] = SDLK_KP_ENTER;
}
