#ifndef VDC_H
#define VDC_H

#include "common.h"

#include <vector>

#include "framebuffer.h"
#include "options.h"

class Cpu;

class Vdc
{
    private:
        static const int MEMORY_SIZE = 256;

        static const int CONTROL_REGISTER = 0xa0;
        static const int STATUS_REGISTER = 0xa1;
        static const int COLLISION_REGISTER = 0xa2;
        static const int COLOR_REGISTER = 0xa3;
        static const int Y_REGISTER = 0xa4;
        static const int X_REGISTER = 0xa5;

        static const int SPRITE_CONTROL_START = 0x00;
        static const int CHARS_START = 0x10;
        static const int QUADS_START = 0x40;
        static const int SPRITE_SHAPE_START = 0x80;
        static const int HORIZONTAL_GRID_START = 0xc0;
        static const int HORIZONTAL_GRID9_START = 0xd0;
        static const int VERTICAL_GRID_START = 0xe0;

        static const int COLLISION_SPRITE0 = 0;
        static const int COLLISION_SPRITE1 = 1;
        static const int COLLISION_SPRITE2 = 2;
        static const int COLLISION_SPRITE3 = 3;
        static const int COLLISION_VGRID = 4;
        static const int COLLISION_HGRID = 5;
        static const int COLLISION_EXTERNAL = 6;
        static const int COLLISION_CHAR = 7;

        static const int CYCLES_PER_SCANLINE = 228;
        static const int HBLANK_START = 178;
        static const int HBLANK_END = 222;

        Cpu *cpu_;
        Framebuffer *framebuffer_;

        vector<uint8_t> mem_;

        int cycles_, scanlines_;
        const int first_drawing_scanline_;

        static bool entered_vblank_;

        bool grid_enabled() { return mem_[CONTROL_REGISTER] & 1 << 3; }
        bool foreground_enabled() { return mem_[CONTROL_REGISTER] & 1 << 5; }

        SDL_Surface *object_surface_;
        static uint32_t *object_data_;
        static int object_pitch_;
        static const uint8_t object_colortable_[8][3];
        uint32_t object_colormap_[8];

        void draw_background(SDL_Rect &clip_r);
        void draw_grid(SDL_Rect &clip_r);
        void draw_char(int x, uint8_t *ptr, SDL_Rect &clip_r);
        void draw_chars(SDL_Rect &clip_r);
        void draw_quad(uint8_t *ptr, SDL_Rect &clip_r);
        void draw_quads(SDL_Rect &clip_r);
        void draw_sprite(uint8_t *ptr, uint8_t *shape, SDL_Rect &clip_r);
        void draw_sprites(SDL_Rect &clip_r);
        void draw_rect(SDL_Rect &clip_r);

        bool screen_drawn_;
        void draw_screen();
        void update_screen();

        uint8_t latched_x_, latched_y_;

    public:
        Vdc();
        ~Vdc();

        void init(Framebuffer *framebuffer, Cpu *cpu);

        void reset();
        void step();

        uint8_t read(uint8_t offset);
        void write(uint8_t offset, uint8_t value);

        bool entered_vblank();

        void debug_dump(ostream &out) const { dump_memory(out, mem_, MEMORY_SIZE); }
};

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
