#ifndef SPRITES_H
#define SPRITES_H

#include "common.h"

class Sprites
{
    private:
        static const int SPRITE_CONTROL_START = 0x00;
        static const int SPRITE_SHAPE_START = 0x80;

        SDL_Surface *surface_;
        uint32_t colormap_[8];

        void draw_sprite(uint8_t *ptr, uint8_t *shape, SDL_Rect &clip_r);

    public:
        void init();

        void draw(uint8_t *mem, SDL_Rect &clip_r);
};

extern Sprites g_sprites;

#endif
