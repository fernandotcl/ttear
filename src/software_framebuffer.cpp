#include "common.h"

#include "software_framebuffer.h"

#include "options.h"

void SoftwareFramebuffer::init()
{
    // We only want a hardware surface when we can directly blit to it, because pixel manipualtion is too
    // slow on hardware surfaces
    Uint32 surface_type = scale_is_one_ ? SDL_HWSURFACE : SDL_SWSURFACE;

    Uint32 flags = surface_type;
    if (g_options.fullscreen)
        flags |= SDL_FULLSCREEN;
    if (g_options.double_buffering)
        flags |= SDL_DOUBLEBUF;

    screen_ = SDL_SetVideoMode(g_options.x_res, g_options.y_res, 32, flags);
    if (!screen_)
        throw runtime_error(SDL_GetError());

    // Create a buffer to which we'll plot
    buffer_ = SDL_CreateRGBSurface(surface_type, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);
    if (!buffer_)
        throw runtime_error(SDL_GetError());
    else if (buffer_->format->BitsPerPixel != 32)
        throw runtime_error("Unable to create a 32bpp surface");

    // Set up the colormap
    for (int i = 0; i < COLORTABLE_SIZE; ++i)
        colormap_[i] = SDL_MapRGB(buffer_->format, colortable_[i][0], colortable_[i][1], colortable_[i][2]);

    // Generate the scaling table
    unsigned int width = window_size_.x_end - window_size_.x;
    unsigned int height = window_size_.y_end - window_size_.y;
    unsigned int buffer_pitch = buffer_->pitch / 4;
    scaling_table_.resize(width * height);
    for (unsigned int y = 0; y < height; ++y)
        for (unsigned int x = 0; x < width; ++x) {
            scaling_table_[y * width + x]
                = (int)(y / window_size_.y_scale) * buffer_pitch + (int)(x / window_size_.x_scale);
    }
}

void SoftwareFramebuffer::blit()
{
    if (scale_is_one_) {
        SDL_BlitSurface(buffer_, NULL, screen_, NULL);
    }
    else {
        Uint32 *src = (Uint32 *)buffer_->pixels;
        Uint32 *dst = (Uint32 *)screen_->pixels;
        unsigned int dst_pitch = screen_->pitch / 4;

        if (SDL_MUSTLOCK(screen_))
            SDL_LockSurface(screen_);

        unsigned int width = window_size_.x_end - window_size_.x;
        unsigned int &x_start = window_size_.x;
        unsigned int &y_start = window_size_.y;
        for (unsigned int y = window_size_.y; y < window_size_.y_end; ++y) {
            for (unsigned int x = window_size_.x; x < window_size_.x_end; ++x)
                dst[y * dst_pitch + x] = src[scaling_table_[(y - y_start) * width + x - x_start]];
        }

        if (SDL_MUSTLOCK(screen_))
            SDL_UnlockSurface(screen_);
    }

    if (g_options.double_buffering)
        SDL_Flip(screen_);
    else
        SDL_UpdateRect(screen_, window_size_.x, window_size_.y, window_size_.x_end, window_size_.y_end);
}
