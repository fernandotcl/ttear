// TODO Implement the GL_TEXTURE_RECTANGLE_ARB extension, if supported
// TODO Implement GL_LINEAR
// TODO Implement keep_aspect = false

#include "globals.h"

extern "C" {
#include <SDL_opengl.h>
}
#include <stdexcept>

#include "video.h"

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

template<typename T> void Video::load_opengl_proc(T &var, const char *procname)
{
    var = (T)SDL_GL_GetProcAddress(procname);
    if (!var)
        throw(runtime_error(string("Unable to load ") + procname));
}

void Video::load_opengl_procs()
{
    load_opengl_proc(s_glEnable, "glEnable");
    load_opengl_proc(s_glDisable, "glDisable");
    load_opengl_proc(s_glShadeModel, "glShadeModel");
    load_opengl_proc(s_glHint, "glHint");
    load_opengl_proc(s_glTexParameteri, "glTexParameteri");
    load_opengl_proc(s_glViewport, "glViewport");
    load_opengl_proc(s_glMatrixMode, "glMatrixMode");
    load_opengl_proc(s_glLoadIdentity, "glLoadIdentity");
    load_opengl_proc(s_glOrtho, "glOrtho");
    load_opengl_proc(s_glClear, "glClear");
    load_opengl_proc(s_glGenTextures, "glGenTextures");
    load_opengl_proc(s_glBindTexture, "glBindTexture");
    load_opengl_proc(s_glTexImage2D, "glTexImage2D");
    load_opengl_proc(s_glTexSubImage2D, "glTexSubImage2D");
    load_opengl_proc(s_glBegin, "glBegin");
    load_opengl_proc(s_glEnd, "glEnd");
    load_opengl_proc(s_glTexCoord2f, "glTexCoord2f");
    load_opengl_proc(s_glVertex2i, "glVertex2i");
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
        {157, 1.0,  16}, // orange
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

    if (using_opengl_) {
        for (int i = 0; i < 24; ++i)
            colormap_[i] = SDL_MapRGB(screen_->format, table[i][2], table[i][1], table[i][0]);
    }
    else {
        for (int i = 0; i < 24; ++i)
            colormap_[i] = SDL_MapRGB(screen_->format, table[i][0], table[i][1], table[i][2]);
    }
}

void Video::set_video_mode()
{
    Uint32 flags = g_options.opengl ? SDL_OPENGL : SDL_SWSURFACE;
    if (g_options.fullscreen)
        flags |= SDL_FULLSCREEN;
    if (g_options.double_buffering) {
        if (g_options.opengl)
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        else
            flags |= SDL_DOUBLEBUF;
    }

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

void Video::init_software()
{
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

void Video::init_opengl()
{
    x_proportion_ = SCREEN_WIDTH / (float)SCREEN_WIDTH_POWER2;
    y_proportion_ = SCREEN_HEIGHT / (float)SCREEN_HEIGHT_POWER2;

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
    s_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    s_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    s_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCREEN_WIDTH_POWER2, SCREEN_HEIGHT_POWER2,
            0, GL_RGBA, GL_UNSIGNED_BYTE, screen_->pixels);

    s_glClear(GL_COLOR_BUFFER_BIT);
    if (g_options.double_buffering) {
        SDL_GL_SwapBuffers();
        s_glClear(GL_COLOR_BUFFER_BIT);
    }
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
        init_software();
    set_video_mode();

    if (using_opengl_)
        screen_ = SDL_CreateRGBSurface(SDL_SWSURFACE, SCREEN_WIDTH_POWER2, SCREEN_HEIGHT_POWER2, 32, 0, 0, 0, 0);
    else
        screen_ = SDL_CreateRGBSurface(SDL_SWSURFACE, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);
    if (!screen_)
        throw(runtime_error(SDL_GetError()));
    else if (screen_->format->BytesPerPixel != 4)
        throw(runtime_error("Asked for a 32bpp surface, but received something else"));

    if (using_opengl_) {
        load_opengl_procs();
        init_opengl(); // OpenGL can only be initialized after set_video_mode
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

    if (g_options.double_buffering)
        SDL_Flip(real_screen_);
    else
        SDL_UpdateRect(real_screen_, 0, 0, SCREEN_WIDTH * scale_, SCREEN_HEIGHT * scale_);
}

void Video::blit_opengl()
{
    s_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SCREEN_WIDTH_POWER2, SCREEN_HEIGHT_POWER2,
            GL_RGBA, GL_UNSIGNED_BYTE, screen_->pixels);

    s_glBegin(GL_QUADS);
        s_glTexCoord2f(0, 0);
        s_glVertex2i(0, 0);
        s_glTexCoord2f(x_proportion_, 0);
        s_glVertex2i(g_options.x_res, 0);
        s_glTexCoord2f(x_proportion_, y_proportion_);
        s_glVertex2i(g_options.x_res, g_options.y_res);
        s_glTexCoord2f(0, y_proportion_);
        s_glVertex2i(0, g_options.y_res);
    s_glEnd();

    SDL_GL_SwapBuffers();
}
