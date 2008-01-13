#ifndef CHARS_H
#define CHARS_H

#include <cstring>

#include "common.h"

class Chars
{
    private:
        static const int NUM_CHARS = 64;
        static const int CHARS_START = 0x10;
        static const int QUADS_START = 0x40;

        SDL_Surface *surfaces_[8];
        static const uint8_t charset_[NUM_CHARS * 8];

        void create_chars(SDL_Surface *surface, uint32_t color);
        SDL_Rect get_rect(int index, int cut_top, int cut_bottom);
        void draw_char(int x, int y, uint8_t *ptr, SDL_Rect &clip_r);

    public:
        void init();

        void draw(uint8_t *mem, SDL_Rect &clip_r);
};

extern Chars g_chars;

#endif
