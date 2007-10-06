#ifndef OPENGL_FRAMEBUFFER_H
#define OPENGL_FRAMEBUFFER_H

#include "globals.h"

extern "C" {
#include <SDL_opengl.h>
}
#include <algorithm>

#include "framebuffer.h"

using namespace std;

class OpenGLFramebuffer : public Framebuffer
{
    private:
        static const int SCREEN_WIDTH_POWER2 = 512;
        static const int SCREEN_HEIGHT_POWER2 = 256;

        SDL_Surface *screen_, *buffer_;
        Uint32 *buffer_data_;
        int buffer_width_;

        bool arbrect_support_;

        Uint32 colormap_[COLORTABLE_SIZE];

        GLuint texture_;
        GLenum texture_target_;
        GLfloat texture_x_, texture_y_;
        GLsizei texture_width_, texture_height_;

        template<typename T> void load_proc(T &var, const char *procname);

    public:
        OpenGLFramebuffer();
        ~OpenGLFramebuffer();

        void init();

        void plot(int x, int y, int color);
        void setline(int x1, int x2, int y, int color);

        void blit();
};

inline OpenGLFramebuffer::OpenGLFramebuffer()
    : screen_(NULL), buffer_(NULL)
{
}

inline void OpenGLFramebuffer::plot(int x, int y, int color)
{
    buffer_data_[y * buffer_width_ + x] = colormap_[color];
}

inline void OpenGLFramebuffer::setline(int x1, int x2, int y, int color)
{
    fill(buffer_data_ + y * SCREEN_WIDTH + x1, buffer_data_ + y * SCREEN_WIDTH + x2, colormap_[color]);
}

#endif
