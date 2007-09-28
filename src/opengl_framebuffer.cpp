#include "globals.h"

extern "C" {
#include <SDL_opengl.h>
}
#include <stdexcept>

#include "opengl_framebuffer.h"

#include "options.h"

typedef void (GLAPIENTRY *glEnable_t)(GLenum);
typedef void (GLAPIENTRY *glDisable_t)(GLenum);
typedef void (GLAPIENTRY *glShadeModel_t)(GLenum);
typedef void (GLAPIENTRY *glHint_t)(GLenum, GLenum);
typedef void (GLAPIENTRY *glTexParameteri_t)(GLenum, GLenum, GLint);
typedef void (GLAPIENTRY *glViewport_t)(GLint, GLint, GLsizei, GLsizei);
typedef void (GLAPIENTRY *glMatrixMode_t)(GLenum);
typedef void (GLAPIENTRY *glLoadIdentity_t)();
typedef void (GLAPIENTRY *glOrtho_t)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
typedef void (GLAPIENTRY *glClear_t)(GLbitfield);
typedef void (GLAPIENTRY *glGenTextures_t)(GLsizei, GLuint *);
typedef void (GLAPIENTRY *glBindTexture_t)(GLenum, GLuint);
typedef void (GLAPIENTRY *glTexImage2D_t)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*);
typedef void (GLAPIENTRY *glTexSubImage2D_t)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei,
        GLenum, GLenum, const GLvoid*);
typedef void (GLAPIENTRY *glBegin_t)(GLenum);
typedef void (GLAPIENTRY *glEnd_t)();
typedef void (GLAPIENTRY *glTexCoord2f_t)(GLfloat, GLfloat);
typedef void (GLAPIENTRY *glVertex2i_t)(GLint, GLint);

static glEnable_t s_glEnable;
static glDisable_t s_glDisable;
static glShadeModel_t s_glShadeModel;
static glHint_t s_glHint;
static glTexParameteri_t s_glTexParameteri;
static glViewport_t s_glViewport;
static glMatrixMode_t s_glMatrixMode;
static glLoadIdentity_t s_glLoadIdentity;
static glOrtho_t s_glOrtho;
static glClear_t s_glClear;
static glGenTextures_t s_glGenTextures;
static glBindTexture_t s_glBindTexture;
static glTexImage2D_t s_glTexImage2D;
static glTexSubImage2D_t s_glTexSubImage2D;
static glBegin_t s_glBegin;
static glEnd_t s_glEnd;
static glTexCoord2f_t s_glTexCoord2f;
static glVertex2i_t s_glVertex2i;

template<typename T> void OpenGLFramebuffer::load_proc(T &var, const char *procname)
{
    var = (T)SDL_GL_GetProcAddress(procname);
    if (!var)
        throw(runtime_error(string("Unable to load ") + procname));
}

void OpenGLFramebuffer::init()
{
    if (SDL_GL_LoadLibrary(NULL))
        throw(runtime_error("Unable to load OpenGL library"));

    Uint32 flags = SDL_OPENGL;
    if (g_options.fullscreen)
        flags |= SDL_FULLSCREEN;
    if (g_options.double_buffering)
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    screen_ = SDL_SetVideoMode(g_options.x_res, g_options.y_res, 32, flags);
    if (!screen_)
        throw(runtime_error(SDL_GetError()));

    buffer_ = SDL_CreateRGBSurface(SDL_SWSURFACE, SCREEN_WIDTH_POWER2, SCREEN_HEIGHT_POWER2, 32, 0, 0, 0, 0);
    if (!buffer_)
        throw(runtime_error(SDL_GetError()));
    buffer_data_ = (Uint32 *)buffer_->pixels;

    for (int i = 0; i < COLORTABLE_SIZE; ++i)
        colormap_[i] = SDL_MapRGB(buffer_->format, colortable_[i][0], colortable_[i][1], colortable_[i][2]);

    load_proc(s_glEnable, "glEnable");
    load_proc(s_glDisable, "glDisable");
    load_proc(s_glShadeModel, "glShadeModel");
    load_proc(s_glHint, "glHint");
    load_proc(s_glTexParameteri, "glTexParameteri");
    load_proc(s_glViewport, "glViewport");
    load_proc(s_glMatrixMode, "glMatrixMode");
    load_proc(s_glLoadIdentity, "glLoadIdentity");
    load_proc(s_glOrtho, "glOrtho");
    load_proc(s_glClear, "glClear");
    load_proc(s_glGenTextures, "glGenTextures");
    load_proc(s_glBindTexture, "glBindTexture");
    load_proc(s_glTexImage2D, "glTexImage2D");
    load_proc(s_glTexSubImage2D, "glTexSubImage2D");
    load_proc(s_glBegin, "glBegin");
    load_proc(s_glEnd, "glEnd");
    load_proc(s_glTexCoord2f, "glTexCoord2f");
    load_proc(s_glVertex2i, "glVertex2i");

    s_glShadeModel(GL_FLAT);
    s_glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    s_glDisable(GL_CULL_FACE);
    s_glDisable(GL_DEPTH_TEST);
    s_glDisable(GL_ALPHA_TEST);
    s_glDisable(GL_LIGHTING);

    s_glEnable(GL_TEXTURE_2D);

    s_glViewport(0, 0, g_options.x_res, g_options.y_res);
    s_glMatrixMode(GL_PROJECTION);
    s_glLoadIdentity();
    s_glOrtho(0, g_options.x_res, g_options.y_res, 0, -1, 1);
    s_glMatrixMode(GL_MODELVIEW);
    s_glLoadIdentity();

    GLuint texture;
    s_glGenTextures(1, &texture);
    s_glBindTexture(GL_TEXTURE_2D, texture);
    {
        GLenum mode = g_options.scaling_mode == Options::SCALING_MODE_NEAREST ? GL_NEAREST : GL_LINEAR;
        s_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mode);
        s_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mode);
    }
    s_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCREEN_WIDTH_POWER2, SCREEN_HEIGHT_POWER2,
            0, GL_BGRA, GL_UNSIGNED_BYTE, buffer_->pixels);

    s_glClear(GL_COLOR_BUFFER_BIT);
    if (g_options.double_buffering) {
        SDL_GL_SwapBuffers();
        s_glClear(GL_COLOR_BUFFER_BIT);
    }
}

void OpenGLFramebuffer::blit()
{
    s_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCREEN_WIDTH_POWER2, SCREEN_HEIGHT_POWER2,
            GL_BGRA, GL_UNSIGNED_BYTE, buffer_->pixels);

    const GLint x_size = window_size_.x;
    const GLint y_size = window_size_.y;
    const GLint x_size_end = window_size_.x_end;
    const GLint y_size_end = window_size_.y_end;
    s_glBegin(GL_QUADS);
        s_glTexCoord2f(0, 0);
        s_glVertex2i(x_size, y_size);
        s_glTexCoord2f(texture_x_proportion_, 0);
        s_glVertex2i(x_size_end, y_size);
        s_glTexCoord2f(texture_x_proportion_, texture_y_proportion_);
        s_glVertex2i(x_size_end, y_size_end);
        s_glTexCoord2f(0, texture_y_proportion_);
        s_glVertex2i(x_size, y_size_end);
    s_glEnd();

    SDL_GL_SwapBuffers();
}
