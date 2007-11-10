#include "common.h"

#include <cstring>

#include "vdc.h"

#include "charset.h"
#include "cpu.h"

uint32_t *Vdc::object_data_;
int Vdc::object_pitch_;

const uint8_t Vdc::object_colortable_[8][3] = {
    // Sprite and char colors
    { 95, 110, 107}, // dark gray
    {255,  66,  85}, // red
    { 61, 240, 122}, // green
    {217, 173,  93}, // yellow
    {106, 161, 255}, // blue
    {255, 152, 255}, // violet
    { 49, 255, 255}, // cyan
    {255, 255, 255}  // white
};

Vdc::Vdc()
    : mem_(MEMORY_SIZE),
      first_drawing_scanlines_(g_options.pal_emulation ? 71 : 21),
      object_surface_(NULL)
{
}

Vdc::~Vdc()
{
    SDL_FreeSurface(object_surface_);
}

void Vdc::init(Framebuffer *framebuffer, Cpu *cpu)
{
    cpu_ = cpu;
    framebuffer_ = framebuffer;

    {
        // Create the surface that's going to be used to draw objects (chars, quads and sprites)
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
        static const uint32_t rmask = 0xff000000;
        static const uint32_t gmask = 0x00ff0000;
        static const uint32_t bmask = 0x0000ff00;
        static const uint32_t amask = 0x000000ff;
#else
        static const uint32_t rmask = 0x000000ff;
        static const uint32_t gmask = 0x0000ff00;
        static const uint32_t bmask = 0x00ff0000;
        static const uint32_t amask = 0xff000000;
#endif
        object_surface_ = SDL_CreateRGBSurface(SDL_HWSURFACE, 80, 32, 32, rmask, gmask, bmask, amask);
    }
    if (!object_surface_)
        throw runtime_error(SDL_GetError());
    else if (object_surface_->format->BitsPerPixel != 32)
        throw runtime_error("Unable to create a 32bpp surface");

    // We use those as an optimization, since they're used in several loops
    object_data_ = (uint32_t *)object_surface_->pixels;
    object_pitch_ = object_surface_->pitch / 4;

    for (int i = 0; i < 8; ++i)
        object_colormap_[i] = SDL_MapRGB(object_surface_->format, object_colortable_[i][0],
                object_colortable_[i][1], object_colortable_[i][2]);
}

void Vdc::reset()
{
    cycles_ = CYCLES_PER_SCANLINE;
    scanlines_ = Framebuffer::SCREEN_HEIGHT + first_drawing_scanlines_;
}

void Vdc::draw_background(SDL_Rect &clip_r)
{
    int color = (mem_[COLOR_REGISTER] & (1 << 3 | 1 << 4 | 1 << 5)) >> 3;
    if (g_p1 & (1 << 7))
        color += 8; // luminescence bit

    framebuffer_->fill_rect(clip_r, color);
}

void Vdc::draw_grid(SDL_Rect &clip_r)
{
    int color = mem_[COLOR_REGISTER] & (1 << 0 | 1 << 1 | 1 << 2);
    if (!(mem_[COLOR_REGISTER] & (1 << 6)))
        color += 8;

    SDL_Rect r;

    // The horizontal grid lines
    for (int i = 0; i < 9; ++i) {
        uint8_t bitfield = mem_[HORIZONTAL_GRID_START + i];
        for (int j = 0; j < 8; ++j) {
            if (bitfield & 1 << j) {
                r.x = i * 80 + 60;
                r.y = j * 24 + 24;
                r.w = 90;
                r.h = 4;
                framebuffer_->fill_rect(r, color);
            }
        }
    }

    // The horizontal grid lines for the nineth row
    for (int i = 0; i < 9; ++i) {
        if (mem_[HORIZONTAL_GRID9_START + i] & 1 << 0) {
            r.x = i * 80 + 60;
            r.y = 9 * 24;
            r.w = 90;
            r.h = 4;
            framebuffer_->fill_rect(r, color);
        }
    }

    // The vertical grid lines
    uint16_t vert_height = mem_[CONTROL_REGISTER] & 1 << 6 ? 4 : 24;
    uint16_t vert_width = mem_[CONTROL_REGISTER] & 1 << 7 ? 90 : 10;
    for (int i = 0; i < 10; ++i) {
        uint8_t bitfield = mem_[VERTICAL_GRID_START + i];
        for (int j = 0; j < 8; ++j) {
            if (bitfield & 1 << j) {
                r.x = i * 80 + 60;
                r.y = j * 24 + 24;
                r.w = vert_width;
                r.h = vert_height;
                framebuffer_->fill_rect(r, color);
            }
        }
    }
}

inline void Vdc::draw_char(int x, uint8_t *ptr, SDL_Rect &clip_r)
{
    // XXX Otimize this using clip_r

    memset(object_data_, SDL_ALPHA_TRANSPARENT, 80 * 32 * 4);

    x = x % 228 + 4;
    int y = ptr[0];
    uint8_t &control = ptr[3];

    uint32_t color = object_colormap_[(control & (1 << 1 | 1 << 2 | 1 << 3)) >> 1];
    int charset_ptr = ptr[2] | (control & 1 << 0) << 8;

    for (int i = 0; i < 16; ++i) {
        int charset_index = charset_ptr + (y + i) / 2;
        if (i / 2 > charset_index % 8)
            continue; // cut-off

        uint8_t bitfield = charset[charset_index & CHARSET_SIZE - 1];
        for (int j = 0; j < 8; ++j) {
            if (bitfield & 1 << 7 - j) {
                int plot_x = i * object_pitch_ + 5 * j;
                object_data_[plot_x++] = color;
                object_data_[plot_x++] = color;
                object_data_[plot_x++] = color;
                object_data_[plot_x++] = color;
                object_data_[plot_x] = color;
            }
        }
    }

    framebuffer_->paste_surface(x * 5 - 1, y, object_surface_);
}

void Vdc::draw_chars(SDL_Rect &clip_r)
{
    for (uint8_t *ptr = &mem_[CHARS_START]; ptr != &mem_[CHARS_START + 48]; ptr += 4)
        draw_char(ptr[1], ptr, clip_r);
}

inline void Vdc::draw_quad(uint8_t *ptr, SDL_Rect &clip_r)
{
    // TODO Implement quad cutting

    int x = ptr[1];
    for (int i = 0; i < 16; i += 4) {
        draw_char(x, &ptr[i], clip_r);
        x += 16;
    }
}

void Vdc::draw_quads(SDL_Rect &clip_r)
{
    for (uint8_t *ptr = &mem_[QUADS_START]; ptr != &mem_[QUADS_START + 64]; ptr += 16)
        draw_quad(ptr, clip_r);
}

inline void Vdc::draw_sprite(uint8_t *ptr, uint8_t *shape, SDL_Rect &clip_r)
{
    // TODO Optimize this using clip_r

    memset(object_data_, SDL_ALPHA_TRANSPARENT, 80 * 32 * 4);

    int y = ptr[0];
    int x = ptr[1] % 228 + 4;
    int control = ptr[2];

    int color = object_colormap_[(control & (1 << 3 | 1 << 4 | 1 << 5)) >> 3];
    int multiplier = control & 1 << 2 ? 4 : 2;
    int shift = control & 1 << 0 ? 1 : 0;

    for (int i = 0; i < 8; ++i) {
        uint8_t bitfield = shape[i];
        int shift_even = control & 1 << 1 && i % 2 ? 1 : 0;

        if (bitfield & 1 << 0) {
            SDL_Rect r = {shift_even, i * multiplier, multiplier * 5 / 2 - 1, multiplier};
            SDL_FillRect(object_surface_, &r, color);
        }
        for (int j = 0; j < 8; ++j) {
            if (bitfield & 1 << j) {
                SDL_Rect r = {j * multiplier * 5 / 2 + shift_even, i * multiplier,
                    multiplier * 5 / 2, multiplier};
                SDL_FillRect(object_surface_, &r, color);
            }
        }
    }

    framebuffer_->paste_surface(x * 5 + shift, y, object_surface_);
}

void Vdc::draw_sprites(SDL_Rect &clip_r)
{
    for (int i = 3; i >= 0; --i)
        draw_sprite(&mem_[SPRITE_CONTROL_START + i * 4], &mem_[SPRITE_SHAPE_START + i * 8], clip_r);
}

void Vdc::draw_screen()
{
    static SDL_Rect whole_screen = {0, 0, Framebuffer::SCREEN_WIDTH, Framebuffer::SCREEN_HEIGHT};

    draw_background(whole_screen);

    if (grid_enabled())
        draw_grid(whole_screen);

    if (foreground_enabled()) {
        if (SDL_MUSTLOCK(object_surface_))
            SDL_LockSurface(object_surface_);

        draw_chars(whole_screen);
        draw_quads(whole_screen);
        draw_sprites(whole_screen);

        if (SDL_MUSTLOCK(object_surface_))
            SDL_UnlockSurface(object_surface_);
    }
}

void Vdc::step()
{
    if (cycles_ == CYCLES_PER_SCANLINE) {
        cycles_ = 0;

        if (scanlines_ == Framebuffer::SCREEN_HEIGHT + first_drawing_scanlines_) {
            // Entered VBLANK
            entered_vblank_ = true;
            scanlines_ = 0;

            // Let the running program know
            mem_[STATUS_REGISTER] |= 1 << 3;
            g_t1 = true;
            cpu_->external_irq();

            // Do the blitting, set the screen as not drawn yet
            framebuffer_->blit();
            screen_drawn_ = false;
        }

        else if (scanlines_ == first_drawing_scanlines_) {
            // Out of VBLANK
            g_t1 = false;
            // TODO clear collisions

            // If we haven't drawn the screen yet, drawn it (will overwrite everything on screen)
            if (!screen_drawn_) {
                draw_screen();
                screen_drawn_ = true;
            }
        }

        else if (g_options.pal_emulation && scanlines_ == -50) { // clear external IRQ on line 21 for PAL
            cpu_->clear_external_irq();
        }

        ++scanlines_;
    }

    if (cycles_ == HBLANK_START) {
        // Entered HBLANK, let the running program know
        mem_[STATUS_REGISTER] &= ~(1 << 0);
        if (mem_[CONTROL_REGISTER] & 1 << 0)
            cpu_->external_irq();
    }

    if (cycles_ == HBLANK_END) {
        // Out of HBLANK, let the running program know
        mem_[STATUS_REGISTER] |= 1 << 0;
        cpu_->counter_increment();
    }

    ++cycles_;
}

uint8_t Vdc::read(uint8_t offset)
{
    uint8_t val;

    switch (offset)
    {
        case STATUS_REGISTER:
            cpu_->clear_external_irq();
            val = mem_[STATUS_REGISTER];
            mem_[STATUS_REGISTER] &= ~(1 << 3);
            break;
        case COLLISION_REGISTER:
            //return calculate_collisions();
            return 0; // TODO
            break;
        case Y_REGISTER:
            val = mem_[CONTROL_REGISTER] & 1 << 1 ? latched_y_ : (uint8_t)scanlines_;
            break;
        case X_REGISTER:
            val = mem_[CONTROL_REGISTER] & 1 << 1 ? latched_x_ : (uint8_t)cycles_;
            break;
        default:
            return mem_[offset];
            break;
    }

    // Write back so that the right value shows up in the debugger
    return mem_[offset] = val;
}

void Vdc::write(uint8_t offset, uint8_t value)
{
    // TODO Call the draw_* functions whenever necessary

    // Don't allow writes to those registers when they're set to be displayed
    if (foreground_enabled() && !(offset & 1 << 7))
        return;
    if (grid_enabled()
            && ((offset >= HORIZONTAL_GRID_START && offset <= HORIZONTAL_GRID_START + 8)
                || (offset >= HORIZONTAL_GRID9_START && offset <= HORIZONTAL_GRID9_START + 9)
                || (offset >= VERTICAL_GRID_START && offset <= VERTICAL_GRID_START + 8)))
        return;

    if (offset & 1 << 6 && !(offset & 1 << 7) && (offset % 4 == 0 || offset % 4 == 1)) {
        // We got the first or second char of a quad, let's mirror it across the quad
        assert(offset >= 0x40 && offset < 0x7f);
        offset &= ~(1 << 1 | 1 << 2 | 1 << 3);
        for (int i = 0; i < 4; ++i)
            mem_[offset + i * 4] = value;
    }

    else {
        if (offset == CONTROL_REGISTER) {
            if (value & 1 << 1) {
                latched_x_ = (uint8_t)cycles_;
                latched_y_ = (uint8_t)scanlines_;
            }
        }

        mem_[offset] = value;
    }
}
