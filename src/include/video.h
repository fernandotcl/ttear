#ifndef VIDEO_H
#define VIDEO_H

#include "globals.h"

extern "C" {
#include <SDL_opengl.h>
}
#include <vector>

using namespace std;

class Video
{
    private:
        bool using_opengl_;
        bool using_temp_surface_;

        union {
            int scale_;
            struct {
                GLfloat x_proportion_;
                GLfloat y_proportion_;
            };
        };

        SDL_Surface *real_screen_, *screen_, *tmp_screen_;
        vector<Uint32> colormap_;

        template<typename T> void load_opengl_proc(T &var, const char *name);
        void load_opengl_procs();
        void init_opengl();

        void init_software();

        void set_video_mode();
        void initialize_colormap();

        void blit_software();
        void blit_opengl();

    public:
        static const int SCREEN_WIDTH = 320;
        static const int SCREEN_WIDTH_POWER2 = 512;
        static const int SCREEN_HEIGHT = 240;
        static const int SCREEN_HEIGHT_POWER2 = 256;

        Video();

        void init();

        void plot(int x, int y, int color);
        void blit();
};

inline Video::Video()
    : using_opengl_(false),
      using_temp_surface_(false),
      colormap_(24)
{
}

inline void Video::plot(int x, int y, int color)
{
    *((Uint32 *)screen_->pixels + y * screen_->pitch / 4 + x) = colormap_[color];
}

inline void Video::blit()
{
    if (using_opengl_)
        blit_opengl();
    else
        blit_software();
}

#endif
