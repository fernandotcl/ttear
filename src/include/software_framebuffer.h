#ifndef SOFTWARE_FRAMEBUFFER_H
#define SOFTWARE_FRAMEBUFFER_H

#include "globals.h"

#include <algorithm>
#include <vector>

#include "framebuffer.h"

using namespace std;

class SoftwareFramebuffer : public Framebuffer
{
    private:
        bool using_temp_surface_;
        SDL_Surface *screen_, *buffer_, *temp_buffer_;
        Uint32 *buffer_data_;

        bool scale_is_one_;
        vector<int> scaling_table_;

        Uint32 colormap_[COLORTABLE_SIZE];

    public:
        SoftwareFramebuffer();
        ~SoftwareFramebuffer();

        void init();

        void plot(int x, int y, int color);
        void setline(int x1, int x2, int y, int color);

        void blit();
};

inline SoftwareFramebuffer::SoftwareFramebuffer()
    : using_temp_surface_(false),
      screen_(NULL), buffer_(NULL), temp_buffer_(NULL),
      scale_is_one_(window_size_.x_scale == 1 && window_size_.y_scale == 1)
{
}

inline SoftwareFramebuffer::~SoftwareFramebuffer()
{
    SDL_FreeSurface(buffer_);
    SDL_FreeSurface(temp_buffer_);
}

inline void SoftwareFramebuffer::plot(int x, int y, int color)
{
    buffer_data_[y * SCREEN_WIDTH + x] = colormap_[color];
}

inline void SoftwareFramebuffer::setline(int x1, int x2, int y, int color)
{
    fill(buffer_data_ + y * SCREEN_WIDTH + x1, buffer_data_ + y * SCREEN_WIDTH + x2, colormap_[color]);
}

#endif
