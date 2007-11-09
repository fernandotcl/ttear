#ifndef SOFTWARE_FRAMEBUFFER_H
#define SOFTWARE_FRAMEBUFFER_H

#include "common.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

#include "framebuffer.h"

class SoftwareFramebuffer : public Framebuffer
{
    private:
        SDL_Surface *screen_, *buffer_;

        bool scale_is_one_;
        vector<int> scaling_table_;

        Uint32 colormap_[COLORTABLE_SIZE];

    public:
        SoftwareFramebuffer();
        ~SoftwareFramebuffer();

        void init();

        void set_clip_rect(SDL_Rect &r);
        void clear_clip_rect();
        void fill_rect(SDL_Rect &r, int color);

        void paste_surface(int x, int y, SDL_Surface *surface);

        void blit();
};

inline SoftwareFramebuffer::SoftwareFramebuffer()
    : screen_(NULL), buffer_(NULL),
      scale_is_one_(window_size_.x_scale == 1 && window_size_.y_scale == 1)
{
}

inline SoftwareFramebuffer::~SoftwareFramebuffer()
{
    SDL_FreeSurface(buffer_);
}

inline void SoftwareFramebuffer::set_clip_rect(SDL_Rect &r)
{
    SDL_SetClipRect(buffer_, &r);
}

inline void SoftwareFramebuffer::clear_clip_rect()
{
    SDL_SetClipRect(buffer_, NULL);
}

inline void SoftwareFramebuffer::fill_rect(SDL_Rect &r, int color)
{
    SDL_FillRect(buffer_, &r, colormap_[color]);
}

inline void SoftwareFramebuffer::paste_surface(int x, int y, SDL_Surface *surface)
{
    SDL_Rect r = {x, y, 0, 0};
    SDL_BlitSurface(surface, NULL, buffer_, &r);
}

#endif
