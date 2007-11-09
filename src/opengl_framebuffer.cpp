#include "common.h"

#include <SDL_opengl.h>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include "opengl_framebuffer.h"

#include "options.h"

#ifndef GLAPIENTRY
# define GLAPIENTRY
#endif

typedef const GLubyte *(GLAPIENTRY *glGetString_t)(GLenum);
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
typedef void (GLAPIENTRY *glDeleteTextures_t)(GLsizei, const GLuint*);
typedef void (GLAPIENTRY *glBegin_t)(GLenum);
typedef void (GLAPIENTRY *glEnd_t)();
typedef void (GLAPIENTRY *glTexCoord2f_t)(GLfloat, GLfloat);
typedef void (GLAPIENTRY *glVertex2i_t)(GLint, GLint);

static glGetString_t s_glGetString;
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
static glDeleteTextures_t s_glDeleteTextures;
static glBegin_t s_glBegin;
static glEnd_t s_glEnd;
static glTexCoord2f_t s_glTexCoord2f;
static glVertex2i_t s_glVertex2i;

OpenGLFramebuffer::~OpenGLFramebuffer()
{
    SDL_FreeSurface(buffer_);
    s_glDeleteTextures(1, &texture_);
}

template<typename T> void OpenGLFramebuffer::load_proc(T &var, const char *procname)
{
    var = (T)SDL_GL_GetProcAddress(procname);
    if (!var)
        throw runtime_error(string("Unable to load ") + procname);
}

void OpenGLFramebuffer::init()
{
    if (SDL_GL_LoadLibrary(NULL))
        throw runtime_error("Unable to load OpenGL library");

    Uint32 flags = SDL_OPENGL;
    if (g_options.fullscreen)
        flags |= SDL_FULLSCREEN;
    if (g_options.double_buffering)
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    screen_ = SDL_SetVideoMode(g_options.x_res, g_options.y_res, 32, flags);
    if (!screen_)
        throw runtime_error(SDL_GetError());

    load_proc(s_glGetString, "glGetString");
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
    load_proc(s_glDeleteTextures, "glDeleteTextures");
    load_proc(s_glBegin, "glBegin");
    load_proc(s_glEnd, "glEnd");
    load_proc(s_glTexCoord2f, "glTexCoord2f");
    load_proc(s_glVertex2i, "glVertex2i");

    {
        const char *extensions = (const char *)s_glGetString(GL_EXTENSIONS);
        if (strstr(extensions, "ARB_texture_rectangle")) {
            cout << "ARB_texture_rectangle extension detected" << endl;
            arbrect_support_ = true;
            texture_target_ = GL_TEXTURE_RECTANGLE_ARB;
            texture_x_ = SCREEN_WIDTH;
            texture_y_ = SCREEN_HEIGHT;
            texture_width_ = SCREEN_WIDTH;
            texture_height_ = SCREEN_HEIGHT;
        }
        else {
            arbrect_support_ = false;
            texture_target_ = GL_TEXTURE_2D;
            texture_x_ = (float)SCREEN_WIDTH / SCREEN_WIDTH_POWER2;
            texture_y_ = (float)SCREEN_HEIGHT / SCREEN_HEIGHT_POWER2;
            texture_width_ = SCREEN_WIDTH_POWER2;
            texture_height_ = SCREEN_HEIGHT_POWER2;
        }
    }

    buffer_ = SDL_CreateRGBSurface(SDL_HWSURFACE, texture_width_, texture_height_, 32, 0, 0, 0, 0);
    if (!buffer_)
        throw runtime_error(SDL_GetError());

    for (int i = 0; i < COLORTABLE_SIZE; ++i)
        colormap_[i] = SDL_MapRGB(buffer_->format, colortable_[i][0], colortable_[i][1], colortable_[i][2]);
 
    s_glShadeModel(GL_FLAT);
    s_glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    s_glDisable(GL_CULL_FACE);
    s_glDisable(GL_DEPTH_TEST);
    s_glDisable(GL_ALPHA_TEST);
    s_glDisable(GL_LIGHTING);

    s_glEnable(texture_target_);

    s_glViewport(0, 0, g_options.x_res, g_options.y_res);
    s_glMatrixMode(GL_PROJECTION);
    s_glLoadIdentity();
    s_glOrtho(0, g_options.x_res, g_options.y_res, 0, -1, 1);
    s_glMatrixMode(GL_MODELVIEW);
    s_glLoadIdentity();

    s_glGenTextures(1, &texture_);
    s_glBindTexture(texture_target_, texture_);
    {
        GLenum mode = g_options.scaling_mode == Options::SCALING_MODE_NEAREST ? GL_NEAREST : GL_LINEAR;
        s_glTexParameteri(texture_target_, GL_TEXTURE_MIN_FILTER, mode);
        s_glTexParameteri(texture_target_, GL_TEXTURE_MAG_FILTER, mode);
    }
    s_glTexImage2D(texture_target_, 0, GL_RGB, texture_width_, texture_height_,
            0, GL_BGRA, GL_UNSIGNED_BYTE, buffer_->pixels);

    s_glClear(GL_COLOR_BUFFER_BIT);
    if (g_options.double_buffering) {
        SDL_GL_SwapBuffers();
        s_glClear(GL_COLOR_BUFFER_BIT);
    }
}

void OpenGLFramebuffer::blit()
{
    if (!snapshot_.empty()) {
        SDL_SaveBMP(buffer_, snapshot_.c_str());
        snapshot_.clear();
    }

    s_glTexSubImage2D(texture_target_, 0, 0, 0, texture_width_, texture_height_,
            GL_BGRA, GL_UNSIGNED_BYTE, buffer_->pixels);

    const GLfloat tex_x = texture_x_;
    const GLfloat tex_y = texture_y_;
    const GLint x_size = window_size_.x;
    const GLint y_size = window_size_.y;
    const GLint x_size_end = window_size_.x_end;
    const GLint y_size_end = window_size_.y_end;
    s_glBegin(GL_QUADS);
        s_glTexCoord2f(0, 0);
        s_glVertex2i(x_size, y_size);
        s_glTexCoord2f(tex_x, 0);
        s_glVertex2i(x_size_end, y_size);
        s_glTexCoord2f(tex_x, tex_y);
        s_glVertex2i(x_size_end, y_size_end);
        s_glTexCoord2f(0, tex_y);
        s_glVertex2i(x_size, y_size_end);
    s_glEnd();

    SDL_GL_SwapBuffers();
}
