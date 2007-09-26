#include "globals.h"

extern "C" {
#include <SDL_opengl.h>
}
#include <stdexcept>

#include "video.h"

#include "options.h"

typedef void (GLAPIENTRY *glDisable_t)(GLenum cap);
typedef void (GLAPIENTRY *glViewport_t)(GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (GLAPIENTRY *glMatrixMode_t)(GLenum mode);
typedef void (GLAPIENTRY *glLoadIdentity_t)();
typedef void (GLAPIENTRY *glOrtho_t)(GLdouble left, GLdouble right,
        GLdouble bottom, GLdouble top, GLdouble near_val, GLdouble far_val);
typedef void (GLAPIENTRY *glBegin_t)(GLenum mode);
typedef void (GLAPIENTRY *glEnd_t)();
typedef void (GLAPIENTRY *glVertex3f_t)(GLfloat x, GLfloat y, GLfloat z);
//typedef void (GLAPIENTRY *glBindTexture_t)(GLenum target, GLuint texture);
//typedef void (GLAPIENTRY *glTexParameteri_t)(GLenum target, GLenum pname, GLint param);

static glDisable_t s_glDisable;
static glViewport_t s_glViewport;
static glMatrixMode_t s_glMatrixMode;
static glLoadIdentity_t s_glLoadIdentity;
static glOrtho_t s_glOrtho;
static glBegin_t s_glBegin;
static glEnd_t s_glEnd;
static glVertex3f_t s_glVertex3f;
//static glBindTexture_t s_glBindTexture;
//static glTexParameteri_t s_glTexParameteri;

Video::Video()
    : using_opengl_(false),
      using_temp_surface_(false)
{
    screen_ = SDL_CreateRGBSurface(SDL_SWSURFACE, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);
    if (!screen_)
        throw(runtime_error(SDL_GetError()));
}

void Video::load_opengl_procs()
{
    s_glDisable = (glDisable_t)SDL_GL_GetProcAddress("glDisable");
    s_glViewport = (glViewport_t)SDL_GL_GetProcAddress("glViewport");
    s_glMatrixMode = (glMatrixMode_t)SDL_GL_GetProcAddress("glMatrixMode");
    s_glLoadIdentity = (glLoadIdentity_t)SDL_GL_GetProcAddress("glLoadIdentity");
    s_glOrtho = (glOrtho_t)SDL_GL_GetProcAddress("glOrtho");
    s_glBegin = (glBegin_t)SDL_GL_GetProcAddress("glBegin");
    s_glEnd = (glEnd_t)SDL_GL_GetProcAddress("glEnd");
    s_glVertex3f = (glVertex3f_t)SDL_GL_GetProcAddress("glVertex3f");
    //s_glBindTexture = (glBindTexture_t)SDL_GL_GetProcAddress("glBindTexture");
    //s_glTexParameteri = (glTexParameteri_t)SDL_GL_GetProcAddress("glTexParameteri");
}

inline void Video::initialize_colormap()
{
    static Uint8 table[24][3] = {
        // Light (bright background or grid) colors
        { 95, 110, 107}, // dark gray
        {106, 161, 255}, // blue
        { 61, 240, 122}, // green
        { 49, 255, 255}, // cyan
        {255,  66,  85}, // red
        {255, 152, 255}, // violet
        {217, 173,  93}, // yellow
        {255, 255, 255},  // white

        // Dark (dark background or grid) colors
        {  0,   0,   0}, // black
        { 14,  61, 212}, // dark blue
        {  0, 152,  27}, // dark green
        {  0, 187, 217}, // dark green
        {199,   0,   8}, // red
        {204,  22, 179}, // violet
        {157, 135,  16}, // orange
        {225, 209, 225}, // light gray

        // Sprite or char colors
        { 95, 110, 107}, // dark gray
        {255,  66,  85}, // red
        { 61, 240, 122}, // green
        {217, 173,  93}, // yellow
        {106, 161, 255}, // blue
        {255, 152, 255}, // violet
        { 49, 255, 255}, // cyan
        {255, 255, 255}  // white
    };

    for (int i = 0; i < 24; ++i)
        colormap_[i] = SDL_MapRGB(screen_->format, table[i][0], table[i][1], table[i][2]);
}

void Video::fix_resolution()
{
    fix_resolution();
    if (g_options.x_res % SCREEN_WIDTH) {
        LOGWARNING << "X resolution is not a multiple of " << SCREEN_WIDTH << ", tweaking it" << endl;
        g_options.x_res = g_options.x_res / SCREEN_WIDTH;
    }
    if (g_options.y_res % SCREEN_HEIGHT) {
        LOGWARNING << "Y resolution is not a multiple of " << SCREEN_HEIGHT << ", tweaking it" << endl;
        g_options.y_res = g_options.y_res / SCREEN_HEIGHT;
    }

    unsigned int x_scale, y_scale;
    scale_ = x_scale = g_options.x_res / SCREEN_WIDTH;
    y_scale = g_options.y_res / SCREEN_HEIGHT;
    if (x_scale != y_scale) {
        LOGWARNING << "The specified resolution does not have a 4:3 aspect ratio, tweaking it" << endl;
        scale_ = x_scale < y_scale ? x_scale : y_scale;
    }
    if (scale_ == 0)
        scale_ = 1;
    else if (scale_ > 3)
        scale_ = 3;

    g_options.x_res = SCREEN_WIDTH * scale_;
    g_options.y_res = SCREEN_HEIGHT * scale_;
}

void Video::set_video_mode()
{
    Uint32 flags = g_options.opengl ? SDL_OPENGL : SDL_SWSURFACE;
    if (g_options.fullscreen)
        flags |= SDL_FULLSCREEN;

    real_screen_ = SDL_SetVideoMode(g_options.x_res, g_options.y_res, 32, flags);
    if (!real_screen_)
        throw(runtime_error(SDL_GetError()));

    if (!using_opengl_ && scale_ != 1 && real_screen_->format->BitsPerPixel != 32) {
        LOGWARNING << "Unable to set a 32bpp video mode, blitting will be slower" << endl;
        tmp_screen_ = SDL_CreateRGBSurface(SDL_SWSURFACE, SCREEN_WIDTH * scale_, SCREEN_HEIGHT * scale_,
                32, 0, 0, 0, 0);
        if (!tmp_screen_)
            throw(runtime_error(SDL_GetError()));
        using_temp_surface_ = true;
    }
}

void Video::init_opengl()
{
    s_glViewport(0, 0, g_options.x_res, g_options.y_res);

    s_glMatrixMode(GL_PROJECTION);
    s_glLoadIdentity();
    s_glOrtho(0, g_options.x_res, g_options.y_res, 0, -1, 1);

    s_glMatrixMode(GL_MODELVIEW);
    s_glLoadIdentity();

    s_glDisable(GL_DEPTH_TEST);

    s_glBegin(GL_QUADS);
        s_glVertex3f(0, 0, 0);
        s_glVertex3f(g_options.x_res, 0, 0);
        s_glVertex3f(g_options.x_res, g_options.y_res, 0);
        s_glVertex3f(0.0, g_options.y_res, 0);
    s_glEnd();
}

void Video::init()
{
    if (g_options.opengl) {
        if (SDL_GL_LoadLibrary(NULL))
            LOGWARNING << "Unable to load OpenGL library, disabling OpenGL support" << endl;
        else
            using_opengl_ = true;
    }

    if (!using_opengl_)
        fix_resolution();
    set_video_mode();

    if (using_opengl_) {
        load_opengl_procs();
        init_opengl();
    }

    SDL_ShowCursor(SDL_DISABLE);
    SDL_WM_SetCaption(PACKAGE_NAME " " PACKAGE_VERSION, PACKAGE_NAME);
    if (g_options.debug)
        SDL_WM_IconifyWindow();

    initialize_colormap();
}

void Video::blit_software()
{
    if (scale_ == 1) {
        SDL_BlitSurface(screen_, NULL, real_screen_, NULL);
    }
    else {
        Uint32 *src, *dst;
        src = (Uint32 *)screen_->pixels;

        if (using_temp_surface_) {
            dst = (Uint32 *)tmp_screen_->pixels;
        }
        else {
            dst = (Uint32 *)real_screen_->pixels;
            SDL_LockSurface(screen_);
        }

        switch (scale_) {
            case 2:
                for (int x = 0; x < SCREEN_WIDTH; ++x) {
                    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
                        dst[(y * 2 + 0) * 2 * SCREEN_WIDTH + x * 2 + 0] = src[y * SCREEN_WIDTH + x];
                        dst[(y * 2 + 1) * 2 * SCREEN_WIDTH + x * 2 + 0] = src[y * SCREEN_WIDTH + x];
                        dst[(y * 2 + 0) * 2 * SCREEN_WIDTH + x * 2 + 1] = src[y * SCREEN_WIDTH + x];
                        dst[(y * 2 + 1) * 2 * SCREEN_WIDTH + x * 2 + 1] = src[y * SCREEN_WIDTH + x];
                    }
                }
                break;
            case 3:
                for (int x = 0; x < SCREEN_WIDTH; ++x) {
                    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
                        dst[(y * 3 + 0) * 3 * SCREEN_WIDTH + x * 3 + 0] = src[y * SCREEN_WIDTH + x];
                        dst[(y * 3 + 1) * 3 * SCREEN_WIDTH + x * 3 + 0] = src[y * SCREEN_WIDTH + x];
                        dst[(y * 3 + 2) * 3 * SCREEN_WIDTH + x * 3 + 0] = src[y * SCREEN_WIDTH + x];
                        dst[(y * 3 + 0) * 3 * SCREEN_WIDTH + x * 3 + 1] = src[y * SCREEN_WIDTH + x];
                        dst[(y * 3 + 1) * 3 * SCREEN_WIDTH + x * 3 + 1] = src[y * SCREEN_WIDTH + x];
                        dst[(y * 3 + 2) * 3 * SCREEN_WIDTH + x * 3 + 1] = src[y * SCREEN_WIDTH + x];
                        dst[(y * 3 + 0) * 3 * SCREEN_WIDTH + x * 3 + 2] = src[y * SCREEN_WIDTH + x];
                        dst[(y * 3 + 1) * 3 * SCREEN_WIDTH + x * 3 + 2] = src[y * SCREEN_WIDTH + x];
                        dst[(y * 3 + 2) * 3 * SCREEN_WIDTH + x * 3 + 2] = src[y * SCREEN_WIDTH + x];
                    }
                }
                break;
        }

        if (using_temp_surface_)
            SDL_BlitSurface(tmp_screen_, NULL, real_screen_, NULL);
        else
            SDL_UnlockSurface(screen_);
    }
    SDL_UpdateRect(real_screen_, 0, 0, SCREEN_WIDTH * scale_, SCREEN_HEIGHT * scale_);
}

void Video::blit_opengl()
{
    SDL_GL_SwapBuffers();
}
