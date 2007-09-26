#ifndef VIDEO_H
#define VIDEO_H

#include "globals.h"

class Video
{
    private:
        bool using_opengl_;
        bool using_temp_surface_;

        int scale_;

        SDL_Surface *real_screen_, *screen_, *tmp_screen_;
        Uint32 colormap_[24];

        void load_opengl_procs();
        void init_opengl();

        void fix_resolution();

        void set_video_mode();
        void initialize_colormap();

        void blit_software();
        void blit_opengl();

    public:
        static const int SCREEN_WIDTH = 320;
        static const int SCREEN_HEIGHT = 240;

        Video();

        void init();

        void plot(int x, int y, int color);
        void blit();
};

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
