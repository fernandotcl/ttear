#ifndef OPENGL_FRAMEBUFFER_H
#define OPENGL_FRAMEBUFFER_H

#include "globals.h"

extern "C" {
#include <SDL_opengl.h>
}
#include <cstring>
#include <vector>

#include "framebuffer.h"

using namespace std;

class OpenGLFramebuffer : public Framebuffer
{
    private:
        static const int SCREEN_WIDTH_POWER2 = 512;
        static const int SCREEN_HEIGHT_POWER2 = 256;

        SDL_Surface *screen_, *buffer_;
        Uint32 *buffer_data_;
        const int buffer_width_;

        Uint32 colormap_[COLORTABLE_SIZE];

        const GLfloat texture_x_proportion_, texture_y_proportion_;

        template<typename T> void load_proc(T &var, const char *procname);

    public:
        OpenGLFramebuffer();
        ~OpenGLFramebuffer();

        void init();

        void plot(int x, int y, int color);
        void setline(int y, int color);

        void blit();
};

inline OpenGLFramebuffer::OpenGLFramebuffer()
    : screen_(NULL), buffer_(NULL),
      buffer_width_(SCREEN_WIDTH_POWER2),
      texture_x_proportion_((GLfloat)SCREEN_WIDTH / SCREEN_WIDTH_POWER2),
      texture_y_proportion_((GLfloat)SCREEN_HEIGHT / SCREEN_HEIGHT_POWER2)
{
}

inline OpenGLFramebuffer::~OpenGLFramebuffer()
{
    //SDL_FreeSurface(buffer_);
    //SDL_FreeSurface(temp_buffer_);
    //TODO
}

inline void OpenGLFramebuffer::plot(int x, int y, int color)
{
    buffer_data_[y * buffer_width_ + x] = colormap_[color];
}

inline void OpenGLFramebuffer::setline(int y, int color)
{
    memset(buffer_data_ + y * SCREEN_WIDTH, colormap_[color], SCREEN_WIDTH * 4);
}

#endif
