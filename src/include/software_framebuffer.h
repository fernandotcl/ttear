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
        void paste_surface(int x, int y, SDL_Surface *surface, SDL_Rect &src_r);

        void blit();

        void take_snapshot(const string &str);
};

inline SoftwareFramebuffer::SoftwareFramebuffer()
    : screen_(NULL), buffer_(NULL)
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

inline void SoftwareFramebuffer::paste_surface(int x, int y, SDL_Surface *surface, SDL_Rect &src_r)
{
    SDL_Rect r = {x, y, 0, 0};
    SDL_BlitSurface(surface, &src_r, buffer_, &r);
}

inline void SoftwareFramebuffer::take_snapshot(const string &str)
{
    SDL_SaveBMP(screen_, str.c_str());
}

#endif
