#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

class Framebuffer
{
    protected:
        struct {
            unsigned int x, y;
            unsigned int x_end, y_end;
            float x_scale, y_scale;
        } window_size_;

        static const uint8_t colortable_[24][3];
        static const int COLORTABLE_SIZE = 24;

    public:
        static const int SCREEN_WIDTH = 340;
        static const int SCREEN_HEIGHT = 240;

        Framebuffer();
        virtual ~Framebuffer() {}

        virtual void init() = 0;

        virtual void plot(int x, int y, int color) = 0;
        virtual void setline(int x1, int x2, int y, int color) = 0;

        virtual void blit() = 0;
};

#endif
