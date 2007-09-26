#ifndef VDC_H
#define VDC_H

#include "globals.h"

#include <iostream>
#include <vector>

#include "util.h"
#include "video.h"

using namespace std;

class Cpu;

class Vdc
{
    private:
        static const int VDC_SIZE = 256;

        static const int VDC_CONTROL_REGISTER = 0xa0;
        static const int VDC_STATUS_REGISTER = 0xa1;
        static const int VDC_COLLISION_REGISTER = 0xa2;
        static const int VDC_COLOR_REGISTER = 0xa3;
        static const int VDC_Y_REGISTER = 0xa4;
        static const int VDC_X_REGISTER = 0xa5;

        static const int VDC_SPRITE_CONTROL_START = 0x00;
        static const int VDC_CHARS_START = 0x10;
        static const int VDC_QUADS_START = 0x40;
        static const int VDC_SPRITE_SHAPE_START = 0x80;
        static const int VDC_HORIZONTAL_GRID_START = 0xc0;
        static const int VDC_HORIZONTAL_GRID9_START = 0xd0;
        static const int VDC_VERTICAL_GRID_START = 0xe0;

        static const int VDC_COLLISION_SPRITE0 = 0;
        static const int VDC_COLLISION_SPRITE1 = 1;
        static const int VDC_COLLISION_SPRITE2 = 2;
        static const int VDC_COLLISION_SPRITE3 = 3;
        static const int VDC_COLLISION_VGRID = 4;
        static const int VDC_COLLISION_HGRID = 5;
        static const int VDC_COLLISION_EXTERNAL = 6;
        static const int VDC_COLLISION_CHAR = 7;

        static const int VDC_SCREEN_WIDTH = 320;
        static const int VDC_SCREEN_HEIGHT = 240;

        static const int GRID_HORIZONTAL_OFFSET = 13;
        static const int FOREGROUND_HORIZONTAL_OFFSET = -3;

        vector<uint8_t> mem_;
        Cpu *cpu_;
        Video &video_;

        int clock_;
        int scanlines_, curline_;

        const int first_drawing_scanline_;
        static const int cycles_per_scanline_ = 152;     // 15.2 cycles per scanline
        static const int cycles_per_scanline_unit_ = 10; // it's pseudo-fixed-point math

        bool entered_vblank_;
        int hblank_timer_;

        bool foreground_enabled() const;
        bool grid_enabled() const;

        bool is_drawable(int x) const;

        void clear_line();
        void draw_grid();
        void draw_chars();
        void draw_quads();
        void draw_sprites();

        void plot(int x, int color);
        void plot(int x, int color, int coll_index);

        mutable vector<uint8_t> collision_;
        mutable vector<uint8_t> collision_table_;
        uint8_t calculate_collisions();
        void clear_collisions();

        uint8_t latched_x_, latched_y_;

    public:
        Vdc(Video &video);

        void init(Cpu *cpu);

        void reset();
        void step();

        bool entered_vblank();

        void debug_dump(ostream &out) const { dump_memory(out, mem_, VDC_SIZE); }

        uint8_t read(uint8_t offset);
        void write(uint8_t offset, uint8_t value);
};

inline void Vdc::init(Cpu *cpu)
{
    cpu_ = cpu;
}

inline bool Vdc::entered_vblank()
{
    if (entered_vblank_) {
        entered_vblank_ = false;
        return true;
    }
    else {
        return false;
    }
}

#endif
