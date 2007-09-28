#include "globals.h"

#include <stdexcept>

#include "software_framebuffer.h"

#include "options.h"

void SoftwareFramebuffer::init()
{
    Uint32 flags = SDL_SWSURFACE;
    if (g_options.fullscreen)
        flags |= SDL_FULLSCREEN;
    if (g_options.double_buffering)
        flags |= SDL_DOUBLEBUF;

    screen_ = SDL_SetVideoMode(g_options.x_res, g_options.y_res, 32, flags);
    if (!screen_)
        throw(runtime_error(SDL_GetError()));

    // If we don't get a 32bpp screen, we'll have to scale to a 32bpp surface and then do the real blitting
    if (!scale_is_one_ && screen_->format->BitsPerPixel != 32) {
        LOGWARNING << "Unable to set a 32bpp video mode, blitting will be slower" << endl;
        temp_buffer_ = SDL_CreateRGBSurface(SDL_SWSURFACE, g_options.x_res, g_options.y_res, 32, 0, 0, 0, 0);
        if (!temp_buffer_)
            throw(runtime_error(SDL_GetError()));
        using_temp_surface_ = true;
    }

    // Create a buffer to which we'll plot
    buffer_ = SDL_CreateRGBSurface(SDL_SWSURFACE, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);
    if (!buffer_)
        throw(runtime_error(SDL_GetError()));
    buffer_data_ = (Uint32 *)buffer_->pixels;

    // Set up the colormap
    for (int i = 0; i < COLORTABLE_SIZE; ++i)
        colormap_[i] = SDL_MapRGB(buffer_->format, colortable_[i][0], colortable_[i][1], colortable_[i][2]);

    // Generate the scaling table
    const unsigned int width = window_size_.x_end - window_size_.x;
    const unsigned int height = window_size_.y_end - window_size_.y;
    scaling_table_.resize(width * height);
    for (unsigned int y = 0; y < height; ++y)
        for (unsigned int x = 0; x < width; ++x) {
            scaling_table_[y * width + x]
                = (int)(y / window_size_.y_scale) * SCREEN_WIDTH + (int)(x / window_size_.x_scale);
    }
}

void SoftwareFramebuffer::blit()
{
    if (scale_is_one_) {
        SDL_BlitSurface(buffer_, NULL, screen_, NULL);
    }
    else {
        Uint32 *src, *dst;
        src = (Uint32 *)buffer_->pixels;

        if (using_temp_surface_) {
            dst = (Uint32 *)temp_buffer_->pixels;
        }
        else {
            dst = (Uint32 *)screen_->pixels;
            if (SDL_MUSTLOCK(screen_) && SDL_LockSurface(screen_))
                throw(runtime_error("Unable to lock screen surface"));
        }

        const unsigned int x_res = g_options.x_res;
        const unsigned int width = window_size_.x_end - window_size_.x;
        const unsigned int x_start = window_size_.x;
        const unsigned int y_start = window_size_.y;
        for (unsigned int y = window_size_.y; y < window_size_.y_end; ++y) {
            for (unsigned int x = window_size_.x; x < window_size_.x_end; ++x)
                dst[y * x_res + x] = src[scaling_table_[(y - y_start) * width + x - x_start]];
        }

        if (using_temp_surface_)
            SDL_BlitSurface(temp_buffer_, NULL, screen_, NULL);
        else if (SDL_MUSTLOCK(screen_))
            SDL_UnlockSurface(screen_);
    }

    if (g_options.double_buffering)
        SDL_Flip(screen_);
    else
        SDL_UpdateRect(screen_, window_size_.x, window_size_.y, window_size_.x_end, window_size_.y_end);
}
