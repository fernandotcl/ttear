#ifndef OPENGL_FRAMEBUFFER_H
#define OPENGL_FRAMEBUFFER_H

#include "common.h"

#include <SDL_opengl.h>
#include <algorithm>
#include <stdexcept>

#include "framebuffer.h"

class OpenGLFramebuffer : public Framebuffer
{
    private:
        static const int SCREEN_WIDTH_POWER2 = 512;
        static const int SCREEN_HEIGHT_POWER2 = 256;

        SDL_Surface *screen_, *buffer_;

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

        void set_clip_rect(SDL_Rect &r);
        void clear_clip_rect();
        void fill_rect(SDL_Rect &r, int color);

        void paste_surface(int x, int y, SDL_Surface *surface);

        void blit();
};

inline OpenGLFramebuffer::OpenGLFramebuffer()
    : screen_(NULL), buffer_(NULL)
{
}

inline void OpenGLFramebuffer::set_clip_rect(SDL_Rect &r)
{
    SDL_SetClipRect(buffer_, &r);
}

inline void OpenGLFramebuffer::clear_clip_rect()
{
    SDL_SetClipRect(buffer_, NULL);
}

inline void OpenGLFramebuffer::fill_rect(SDL_Rect &r, int color)
{
    SDL_FillRect(buffer_, &r, colormap_[color]);
}

inline void OpenGLFramebuffer::paste_surface(int x, int y, SDL_Surface *surface)
{
    SDL_Rect r = {x, y, 0, 0};
    SDL_BlitSurface(surface, NULL, buffer_, &r);
}

#endif
