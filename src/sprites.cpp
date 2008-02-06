#include "common.h"

#include <stdexcept>

#include "sprites.h"

#include "colors.h"
#include "framebuffer.h"

// TODO Implement shape caching

Sprites g_sprites;

void Sprites::init()
{
    // Create the surface
    surface_ = SDL_CreateRGBSurface(SDL_HWSURFACE,
            16 * Framebuffer::SCREEN_WIDTH_MULTIPLIER, 32, 32,
            TRANSPARENT_RMASK, TRANSPARENT_GMASK, TRANSPARENT_BMASK, TRANSPARENT_AMASK);
    if (!surface_)
        throw runtime_error(SDL_GetError());
    else if (surface_->format->BitsPerPixel != 32)
        throw runtime_error("Unable to create a 32bpp surface");

    // Initialize the colormap
    for (int i = 0; i < 8; ++i)
        colormap_[i] = SDL_MapRGB(surface_->format, colortable[i + COLORTABLE_SPRITE_OFFSET][0],
                colortable[i + COLORTABLE_SPRITE_OFFSET][1], colortable[i + COLORTABLE_SPRITE_OFFSET][2]);
}

inline void Sprites::draw_sprite(uint8_t *ptr, uint8_t *shape, SDL_Rect &clip_r)
{
    int y = ptr[0];
    int x = ptr[1] % 228 + 4;

    // TODO Optimize using clipping rect

    memset(surface_->pixels, SDL_ALPHA_TRANSPARENT, surface_->pitch * 32);

    int control = ptr[2];

    static const int shift_table[4][2] = {
        {0,                                    0},
        {Framebuffer::SCREEN_WIDTH_MULTIPLIER, Framebuffer::SCREEN_WIDTH_MULTIPLIER},
        {Framebuffer::SCREEN_WIDTH_MULTIPLIER, 0},
        {0,                                    Framebuffer::SCREEN_WIDTH_MULTIPLIER},
    };
    int shift_index = control & (1 << 0 | 1 << 1);
    int shift_even = shift_table[shift_index][0];
    int shift_odd = shift_table[shift_index][1];

    int color = colormap_[(control & (1 << 3 | 1 << 4 | 1 << 5)) >> 3];
    int multiplier = control & 1 << 2 ? 2 : 1;

    for (int i = 0; i < 8; ++i) {
        int shift = i % 2 ? shift_odd : shift_even;

        uint8_t bitfield = shape[i];

        if (bitfield & 1 << 0) {
            // The first column of every sprite is 1/Framebuffer::SCREEN_WIDTH_MULTIPLIER shorter
            SDL_Rect r = {shift + 1, i * multiplier * 2,
                multiplier * Framebuffer::SCREEN_WIDTH_MULTIPLIER - 1, multiplier * 2};
            SDL_FillRect(surface_, &r, color);
        }
        for (int j = 1; j < 8; ++j) {
            if (bitfield & 1 << j) {
                SDL_Rect r = {j * multiplier * Framebuffer::SCREEN_WIDTH_MULTIPLIER + shift,
                    i * multiplier * 2, multiplier * Framebuffer::SCREEN_WIDTH_MULTIPLIER,
                    multiplier * 2};
                SDL_FillRect(surface_, &r, color);
            }
        }
    }

    g_framebuffer->paste_surface(x * Framebuffer::SCREEN_WIDTH_MULTIPLIER, y, surface_);
}

void Sprites::draw(uint8_t *mem, SDL_Rect &clip_r)
{
    if (SDL_MUSTLOCK(surface_)) {
        if (SDL_LockSurface(surface_))
            throw runtime_error(SDL_GetError());
    }
    for (int i = 3; i >= 0; --i)
        draw_sprite(&mem[SPRITE_CONTROL_START + i * 4], &mem[SPRITE_SHAPE_START + i * 8], clip_r);
    if (SDL_MUSTLOCK(surface_))
        SDL_UnlockSurface(surface_);
}
