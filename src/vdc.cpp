#include "common.h"

#include <algorithm>
#include <iostream>

#include "vdc.h"

#include "chars.h"
#include "cpu.h"
#include "framebuffer.h"
#include "sprites.h"

Vdc g_vdc;

bool Vdc::entered_vblank_;

Vdc::Vdc()
    : mem_(MEMORY_SIZE),
      first_drawing_scanline_(g_options.pal_emulation ? 70 : 21)
{
}

void Vdc::reset()
{
    fill(mem_.begin(), mem_.end(), 0);
    cycles_ = 0;
    scanlines_ = 0;
    cur_frame_ = 0;
}

void Vdc::draw_background(SDL_Rect &clip_r)
{
    int color = (mem_[COLOR_REGISTER] & (1 << 3 | 1 << 4 | 1 << 5)) >> 3;
    if (g_p1 & (1 << 7))
        color += 8; // luminescence bit

    g_framebuffer->fill_rect(clip_r, color);
}

void Vdc::draw_grid(SDL_Rect &clip_r)
{
    // TODO Implement shape caching

    int color = mem_[COLOR_REGISTER] & (1 << 0 | 1 << 1 | 1 << 2);
    if (!(mem_[COLOR_REGISTER] & (1 << 6)))
        color += 8;

    SDL_Rect r;

    // The horizontal grid lines
    for (int i = 0; i < 9; ++i) {
        uint8_t bitfield = mem_[HORIZONTAL_GRID_START + i];
        for (int j = 0; j < 8; ++j) {
            if (bitfield & 1 << j) {
                r.x = (i * 16 + 12) * Framebuffer::SCREEN_WIDTH_MULTIPLIER;
                r.y = j * 24 + 24;
                r.w = 18 * Framebuffer::SCREEN_WIDTH_MULTIPLIER;
                r.h = 4;
                g_framebuffer->fill_rect(r, color);
            }
        }
    }

    // The horizontal grid lines for the nineth row
    for (int i = 0; i < 9; ++i) {
        if (mem_[HORIZONTAL_GRID9_START + i] & 1 << 0) {
            r.x = (i * 16 + 12) * Framebuffer::SCREEN_WIDTH_MULTIPLIER;
            r.y = 9 * 24;
            r.w = 18 * Framebuffer::SCREEN_WIDTH_MULTIPLIER;
            r.h = 4;
            g_framebuffer->fill_rect(r, color);
        }
    }

    // The vertical grid lines
    uint16_t vert_height = mem_[CONTROL_REGISTER] & 1 << 6 ? 4 : 24;
    uint16_t vert_width = mem_[CONTROL_REGISTER] & 1 << 7 ? 18 : 2;
    for (int i = 0; i < 10; ++i) {
        uint8_t bitfield = mem_[VERTICAL_GRID_START + i];
        for (int j = 0; j < 8; ++j) {
            if (bitfield & 1 << j) {
                r.x = (i * 16 + 12) * Framebuffer::SCREEN_WIDTH_MULTIPLIER;
                r.y = j * 24 + 24;
                r.w = vert_width * Framebuffer::SCREEN_WIDTH_MULTIPLIER;
                r.h = vert_height;
                g_framebuffer->fill_rect(r, color);
            }
        }
    }
}

inline void Vdc::draw_rect(SDL_Rect &clip_r)
{
#ifdef DEBUG
    if (clip_r.x != 0 || clip_r.y != 0 || clip_r.w != Framebuffer::SCREEN_WIDTH
            || clip_r.h != Framebuffer::SCREEN_HEIGHT)
        cout << "draw_rect(): going to draw {"
             << dec << (clip_r.x / Framebuffer::SCREEN_WIDTH_MULTIPLIER) << ", " << clip_r.y
             << ", " << (clip_r.w / Framebuffer::SCREEN_WIDTH_MULTIPLIER) << ", " << clip_r.h << "}" << endl;
#endif

    draw_background(clip_r);

    if (grid_enabled())
        draw_grid(clip_r);

    if (foreground_enabled()) {
        g_chars.draw(&*mem_.begin(), clip_r);
        g_sprites.draw(&*mem_.begin(), clip_r);
    }
}

inline void Vdc::draw_screen()
{
    static SDL_Rect whole_screen = {0, 0, Framebuffer::SCREEN_WIDTH, Framebuffer::SCREEN_HEIGHT};
    draw_rect(whole_screen);

    screen_drawn_ = true;
}

inline void Vdc::update_screen()
{
    int curline = scanlines_ - first_drawing_scanline_;
    if (cycles_ == 0) {
        SDL_Rect r = {0, curline, Framebuffer::SCREEN_WIDTH, Framebuffer::SCREEN_HEIGHT - curline};
        g_framebuffer->set_clip_rect(r);
        draw_rect(r);
    }
    else {
        if (scanlines_ + 1 != Framebuffer::SCREEN_HEIGHT) {
            SDL_Rect r = {0, curline + 1, cycles_, Framebuffer::SCREEN_HEIGHT - curline - 1};
            g_framebuffer->set_clip_rect(r);
            draw_rect(r);
        }
        SDL_Rect r = {cycles_, curline, Framebuffer::SCREEN_WIDTH - cycles_,
            Framebuffer::SCREEN_HEIGHT - curline};
        g_framebuffer->set_clip_rect(r);
        draw_rect(r);
    }
    g_framebuffer->clear_clip_rect();
}

void Vdc::step()
{
    int scanlines_end = g_options.pal_emulation ?
        CYCLES_PER_SCANLINE : CYCLES_PER_SCANLINE - scanlines_ % 2;
    if (cycles_ >= scanlines_end) {
        cycles_ -= scanlines_end;

        if (scanlines_ == Framebuffer::SCREEN_HEIGHT + first_drawing_scanline_ + cur_frame_ % 2) {
#ifdef DEBUG
            cout << "entered vblank " << endl;
#endif
            // Entered VBLANK
            entered_vblank_ = true;
            scanlines_ = 0;
            ++cur_frame_;

            // Let the running program know
            mem_[STATUS_REGISTER] |= 1 << 3;
            g_t1 = true;
            g_cpu.external_irq();

            // Do the blitting, set the screen as not drawn yet
            g_framebuffer->blit();
            screen_drawn_ = false;
        }

        else if (scanlines_ == first_drawing_scanline_) {
            // Out of VBLANK
            g_t1 = false;

            // If we haven't drawn the screen yet, drawn it (will overwrite everything on screen)
            if (!screen_drawn_)
                draw_screen();
        }

        else if (g_options.pal_emulation && scanlines_ == 21) {
            // Clear external IRQ on line 21 for PAL
            g_cpu.clear_external_irq();
        }

        ++scanlines_;
    }

    if (cycles_ == HBLANK_START) {
        // Entered HBLANK, let the running program know
        mem_[STATUS_REGISTER] &= ~(1 << 0);
        if (mem_[CONTROL_REGISTER] & 1 << 0)
            g_cpu.external_irq();
    }

    else if (cycles_ == HBLANK_END) {
        // Out of HBLANK, let the running program know
        mem_[STATUS_REGISTER] |= 1 << 0;
        if (scanlines_ >= first_drawing_scanline_)
            g_cpu.counter_increment();
    }

    ++cycles_;
}

uint8_t Vdc::read(uint8_t offset)
{
    uint8_t val;

    switch (offset)
    {
        case STATUS_REGISTER:
            g_cpu.clear_external_irq();
            val = mem_[STATUS_REGISTER];
            mem_[STATUS_REGISTER] &= ~(1 << 3);
            break;
        case COLLISION_REGISTER:
            //return calculate_collisions();
            return 0; // TODO
            break;
        case Y_REGISTER:
            val = mem_[CONTROL_REGISTER] & 1 << 1 ? latched_y_ : (uint8_t)(scanlines_ - first_drawing_scanline_);
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
        uint8_t diff = mem_[offset] ^ value;
        mem_[offset] = value;

        if (offset == CONTROL_REGISTER) {
            if (value & 1 << 1) {
                latched_x_ = (uint8_t)cycles_;
                latched_y_ = (uint8_t)(scanlines_ - first_drawing_scanline_);
            }

            if (diff & 1 << 1) {
                // Change the position strobe status accordinagly
                if (value & 1 << 1)
                    mem_[STATUS_REGISTER] |= 1 << 1;
                else
                    mem_[STATUS_REGISTER] &= ~(1 << 1);
            }

            // The screen needs to be redrawn if the graphics have been changed
            if (screen_drawn_) {
                if (diff & ~(1 << 0 | 1 << 1 | 1 << 2)) {
#ifdef DEBUG
                    cout << "updating screen because of CONTROL_REGISTER change (diff: 0x"
                         << setw(2) << setfill('0') << hex << (int)(diff & ~(1 << 0 | 1 << 1 | 1 << 2))
                         << " value: 0x" << setw(2) << setfill('0') << hex << (int)value
                         << " scanline: " << dec << scanlines_
                         << " x: " << (cycles_ / Framebuffer::SCREEN_WIDTH_MULTIPLIER) << ')' << endl;
#endif
                    update_screen();
                }
            }
        }

        else {
            if (!diff)
                return;

            if (offset == COLLISION_REGISTER) {
                // A change to the collision register indicates what collisions
                // we will check for in the next frame
                // TODO
            }

            else {
                // The screen needs to be redrawn if foreground objects, the grid
                // or the color register have been changed
                if (screen_drawn_ && (!(offset & 1 << 7) || offset == COLOR_REGISTER)) {
#ifdef DEBUG
                    cout << "updating screen because of other change (offset: 0x"
                         << setw(2) << setfill('0') << hex << (int)offset
                         << " scanline: " << dec << scanlines_
                         << " x: " << (cycles_ / Framebuffer::SCREEN_WIDTH_MULTIPLIER) << ')' << endl;
#endif
                    update_screen();
                }
            }
        }

        mem_[offset] = value;
    }
}

void Vdc::debug_print_timing(ostream &out)
{
    out << "Scanline: " << dec << scanlines_ << " (0x" << hex << scanlines_
        << ") Beam: " << dec << cycles_ << " (0x" << hex << cycles_ << ')' << endl;
}
