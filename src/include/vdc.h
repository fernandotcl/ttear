#ifndef VDC_H
#define VDC_H

#include "globals.h"

#include <vector>

#include "util.h"
#include "framebuffer.h"

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

        static const int GRID_HORIZONTAL_OFFSET = 24;
        static const int FOREGROUND_HORIZONTAL_OFFSET = 8;

        static const int COLLISION_TABLE_WIDTH = 368;
        static const int COLLISION_TABLE_HEIGHT = Framebuffer::SCREEN_HEIGHT;

        static const int CYCLES_PER_SCANLINE = 152;     // 15.2 cycles per scanline
        static const int CYCLES_PER_SCANLINE_UNIT = 10; // it's pseudo-fixed-point math

        static const int HBLANK_TIMER_INITIAL_VALUE = 3; // 3 cycles, 12.6us, pretty close to 12.4us

        vector<uint8_t> mem_;
        Cpu *cpu_;
        Framebuffer *framebuffer_;

        int clock_;
        int scanlines_, curline_;

        const int first_drawing_scanline_;

        bool entered_vblank_;
        int hblank_timer_;

        bool foreground_enabled() const;
        bool grid_enabled() const;

        bool is_drawable(int x) const;
        bool should_check_collisions(int x) const;

        bool in_hblank() const;

        void clear_line();
        void draw_grid();
        void draw_chars();
        void draw_quads();
        void draw_sprites();

        void plot(int x, int color);
        void plot(int x, int color, int coll_index, bool check_bounds = true);

        mutable vector<uint8_t> collision_;
        mutable vector<uint8_t> collision_table_;
        uint8_t calculate_collisions();
        void clear_collisions();

        uint8_t latched_x_, latched_y_;

        vector<int> visible_pixels_;

    public:
        Vdc();

        void init(Framebuffer *framebuffer, Cpu *cpu);

        void reset();
        void step();

        bool entered_vblank();

        void debug_dump(ostream &out) const { dump_memory(out, mem_, VDC_SIZE); }

        uint8_t read(uint8_t offset);
        void write(uint8_t offset, uint8_t value);
};

inline void Vdc::init(Framebuffer *framebuffer, Cpu *cpu)
{
    framebuffer_ = framebuffer;
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
