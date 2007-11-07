#include "globals.h"

#include <cstring>

#include "vdc.h"

#include "charset.h"
#include "cpu.h"
#include "options.h"

Vdc::Vdc()
    : mem_(VDC_SIZE),
      first_drawing_scanline_(g_options.pal_emulation ? 71 : 21),
      entered_vblank_(false),
      collision_(8),
      collision_table_(VDC_SCREEN_WIDTH * VDC_SCREEN_HEIGHT)
{
}

inline bool Vdc::foreground_enabled() const
{
    return mem_[VDC_CONTROL_REGISTER] & 1 << 5;
}

inline bool Vdc::grid_enabled() const
{
    return mem_[VDC_CONTROL_REGISTER] & 1 << 3;
}

inline bool Vdc::is_drawable(int x) const
{
    return x / 2 == cycles_;
}

inline bool Vdc::should_check_collisions(int x) const
{
    return x >= 0 && x <= VDC_SCREEN_WIDTH;
}

inline void Vdc::plot(int x, int color)
{
    if (is_drawable(x))
        framebuffer_->plot(x, curline_, color);
}

inline void Vdc::plot(int x, int color, int coll_index, bool check_bounds)
{
    plot(x, color);

    if (!check_bounds || should_check_collisions(x)) {
        uint8_t coll = collision_table_[curline_ * VDC_SCREEN_WIDTH + x];

        for (int i = 0; i < 8; ++i) {
            if (coll & 1 << i) {
                collision_[i] |= 1 << coll_index;
                collision_[coll_index] |= 1 << i;
            }
        }

        collision_table_[curline_ * VDC_SCREEN_WIDTH + x] |= 1 << coll_index;
    }
}

inline void Vdc::draw_background()
{
    int color = (mem_[VDC_COLOR_REGISTER] & (1 << 3 | 1 << 4 | 1 << 5)) >> 3;
    if (g_p1 & (1 << 7))
        color += 8; // luminescence bit

    framebuffer_->plot(cycles_ * 2, curline_, color);
    framebuffer_->plot(cycles_ * 2 + 1, curline_, color);
}

inline void Vdc::draw_chars()
{
    for (uint8_t *ptr = &mem_[VDC_CHARS_START]; ptr != &mem_[VDC_CHARS_START + 48]; ptr += 4) {
        uint8_t y = ptr[0];
        int y_off = curline_ - y;
        if (y_off < 0 || y_off >= 8 * 2) // char pixels are 2px tall
            continue;

        int x = ptr[1];
        int x_off = cycles_ - x + FOREGROUND_HORIZONTAL_OFFSET / 2;
        if (x_off < 0 || x_off >= 8 * 2) // char pixels are 2px wide
            continue;

        uint8_t control = ptr[3];
        int bitfield_index = ((ptr[2] | (control & 1 << 0) << 8) + curline_ / 2);
        if (y_off / 2 > bitfield_index % 8)
            continue; // cut-off lines
        bitfield_index -= y % 2;

        int color = ((control & (1 << 1 | 1 << 2 | 1 << 3)) >> 1) + 16;
        uint8_t bitfield = charset[bitfield_index & CHARSET_SIZE - 1];
        for (int i = 0; i < 8; ++i) {
            if (bitfield & 1 << 7 - i) {
                int plot_x = (x + i) * 2 + FOREGROUND_HORIZONTAL_OFFSET;
                plot(plot_x++, color, VDC_COLLISION_CHAR);
                plot(plot_x, color, VDC_COLLISION_CHAR);
            }
        }
    }
}

inline void Vdc::draw_quads()
{
    for (uint8_t *ptr = &mem_[VDC_QUADS_START]; ptr != &mem_[VDC_QUADS_START + 64]; ptr += 16) {
        uint8_t y = ptr[12];
        int y_off = curline_ - y;
        if (y_off < 0 || y_off >= 8 * 2) // quad pixels are also 2px tall
            continue;

        {
            int bitfield_index = ((ptr[14] | (ptr[15] & 1 << 0) << 8) + curline_ / 2) & CHARSET_SIZE - 1;
            if (y_off / 2 > bitfield_index % 8)
                continue; // cut-off lines based on the fourth char in the quad
        }

        int x = ptr[1];
        int bitfield_base = curline_ / 2;

        for (uint8_t *chr_ptr = ptr; chr_ptr != ptr + 16; chr_ptr += 4) {
            uint8_t control = chr_ptr[3];

            int x_off = cycles_ - x + FOREGROUND_HORIZONTAL_OFFSET / 2;
            if (x_off < 0 || x_off >= 8 * 2) { // char pixels are 2px wide
                x += 16;
                continue;
            }

            int color = ((control & (1 << 1 | 1 << 2 | 1 << 3)) >> 1) + 16;

            int bitfield_index = ((chr_ptr[2] | (control & 1 << 0) << 8) + bitfield_base);
            if (y_off / 2 > bitfield_index % 8) { // bottom cut-off (KTAA)
                x += 16;
                continue;
            }
            bitfield_index -= y % 2;
            uint8_t bitfield = charset[bitfield_index & CHARSET_SIZE - 1];

            for (int i = 0; i < 8; ++i) {
                if (bitfield & 1 << 7 - i) {
                    int plot_x = (x + i) * 2 + FOREGROUND_HORIZONTAL_OFFSET;
                    plot(plot_x++, color, VDC_COLLISION_CHAR);
                    plot(plot_x, color, VDC_COLLISION_CHAR);
                }
            }
            x += 16;
        }
    }
}

inline void Vdc::draw_sprites()
{
    for (int i = 3; i >= 0; --i) { // draw in reverse order, sprite 0 should always appear on top
        uint8_t *data = &mem_[VDC_SPRITE_CONTROL_START + i * 4];

        int control = data[2];
        int multiplier = control & (1 << 2) ? 4 : 2;

        int y = data[0];
        int y_off = curline_ - y;
        if (y_off < 0 || y_off >= 8 * multiplier)
            continue;

        int x = data[1];
        int x_off = cycles_ - x + FOREGROUND_HORIZONTAL_OFFSET / multiplier;
        if (x_off < 0 || x_off >= 8 * multiplier)
            continue;

        int color = ((control & (1 << 3 | 1 << 4 | 1 << 5)) >> 3) + 16;
        uint8_t *shape = &mem_[VDC_SPRITE_SHAPE_START + i * 8];

        int shift = 0;
        if (control & 1 << 0)
            ++shift;
        if (control & 1 << 1 && y_off / multiplier % 2)
            ++shift;

        uint8_t bitfield = shape[y_off / multiplier];
        for (int k = 0; k < 8; ++k) {
            if (bitfield & 1 << k) {
                for (int j = 0; j < multiplier; ++j) {
                    int plot_x = x * 2 + k * multiplier + shift + j + FOREGROUND_HORIZONTAL_OFFSET;
                    plot(plot_x, color, i);
                }
            }
        }
    }
}

inline void Vdc::draw_grid()
{
    int cur = curline_ - 24; // the grid starts on line 24

    if (cur < 0 || cur >= 8 * 24 + 4) // vertical bars are 24 lines tall
        return;

    int color = mem_[VDC_COLOR_REGISTER] & (1 << 0 | 1 << 1 | 1 << 2);
    if (!(mem_[VDC_COLOR_REGISTER] & (1 << 6)))
        color += 8;

    // TODO Implement dot grid mode

    // The horizontal grid lines for the nineth row
    if (cur >= 8 * 24 && cur < 8 * 24 + 3) {
        for (int i = 0; i < 9; ++i) {
            if (mem_[VDC_HORIZONTAL_GRID9_START + i] & (1 << 0)) {
                for (int j = 0; j < 36; ++j) // 36 pixels wide (32 + 4)
                    plot(i * 32 + j + GRID_HORIZONTAL_OFFSET, color, VDC_COLLISION_HGRID, false);
            }
        }
    }
    if (cur > 8 * 24)
        return;

    // The horizontal grid lines
    if (cur % 24 < 3) {
        for (int i = 0; i < 9; ++i) {
            uint8_t bitfield = mem_[VDC_HORIZONTAL_GRID_START + i];
            if (bitfield & 1 << (cur / 24)) {
                for (int j = 0; j < 36; ++j) // 36 pixels wide (32 + 4)
                    plot(i * 32 + j + GRID_HORIZONTAL_OFFSET, color, VDC_COLLISION_HGRID, false);
            }
        }
    }

    // Fill mode
    int vert_width = mem_[VDC_CONTROL_REGISTER] & 1 << 7 ? 32 : 4;

    // The vertical grid lines
    for (int i = 0; i < 10; ++i) {
        uint8_t bitfield = mem_[VDC_VERTICAL_GRID_START + i];
        if (bitfield & 1 << (cur / 24)) {
            for (int j = 0; j < vert_width; ++j)
                plot(i * 32 + GRID_HORIZONTAL_OFFSET + j, color, VDC_COLLISION_VGRID, false);
        }
    }
}

inline uint8_t Vdc::calculate_collisions()
{
    uint8_t val = 0;

    uint8_t bits = mem_[VDC_COLLISION_REGISTER];

    for (int i = 0; i < 8; ++i) {
        if (bits & 1 << i)
            val |= collision_[i];
    }

    return val;
}

inline void Vdc::clear_collisions()
{
    bzero(&collision_[0], 8);
    bzero(&collision_table_[0], VDC_SCREEN_WIDTH * VDC_SCREEN_HEIGHT);
}

uint8_t Vdc::read(uint8_t offset)
{
    uint8_t val;

    switch (offset)
    {
        case VDC_STATUS_REGISTER:
            cpu_->clear_external_irq();
            val = mem_[VDC_STATUS_REGISTER];
            mem_[VDC_STATUS_REGISTER] &= ~(1 << 3);
            break;
        case VDC_COLLISION_REGISTER:
            return calculate_collisions();
            break;
        case VDC_Y_REGISTER:
            val = mem_[VDC_CONTROL_REGISTER] & 1 << 1 ? latched_y_ : curline_;
            break;
        case VDC_X_REGISTER:
            val = mem_[VDC_CONTROL_REGISTER] & 1 << 1 ? latched_x_ : cycles_;
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
    if (foreground_enabled() && !(offset & 1 << 7))
        return;
    if (grid_enabled()
            && ((offset >= VDC_HORIZONTAL_GRID_START && offset <= VDC_HORIZONTAL_GRID_START + 8)
                || (offset >= VDC_HORIZONTAL_GRID9_START && offset <= VDC_HORIZONTAL_GRID9_START + 9)
                || (offset >= VDC_VERTICAL_GRID_START && offset <= VDC_VERTICAL_GRID_START + 8)))
        return;

    if (offset & 1 << 6 && !(offset & 1 << 7) && (offset % 4 == 0 || offset % 4 == 1)) {
        // We got the first or second char of a quad, let's mirror it across the quad
        assert(offset >= 0x40 && offset < 0x7f);
        offset &= ~(1 << 1 | 1 << 2 | 1 << 3);
        for (int i = 0; i < 4; ++i)
            mem_[offset + i * 4] = value;
    }
    else {
        if (offset == VDC_CONTROL_REGISTER) {
            if (value & 1 << 1) {
                latched_x_ = cycles_;
                latched_y_ = curline_;
            }
        }

        mem_[offset] = value;
    }
}

void Vdc::reset()
{
    cycles_ = 0;
    scanlines_ = 0;
    curline_ = scanlines_ - first_drawing_scanline_;

    bzero(&mem_[0], VDC_SIZE);
    clear_collisions();
}

inline bool Vdc::in_hblank() const
{
    return cycles_ < HBLANK_LENGTH;
}

void Vdc::step()
{
    if (cycles_ == CYCLES_PER_SCANLINE) {
        cycles_ = 0;

        if (curline_ >= 0) {
            // Signal HBLANK
            mem_[VDC_STATUS_REGISTER] &= ~(1 << 0);

            if (mem_[VDC_CONTROL_REGISTER] & 1 << 0)
                cpu_->external_irq();
        }

        if (curline_ == Framebuffer::SCREEN_HEIGHT) {
            // Entered VBLANK
            entered_vblank_ = true;

            // Back to the first line
            scanlines_ = 0;
            curline_ = scanlines_ - first_drawing_scanline_;

            // Do the blitting
            framebuffer_->blit();

            mem_[VDC_STATUS_REGISTER] |= 1 << 3;
            g_t1 = true;

            // Fire an external IRQ
            cpu_->external_irq();
        }
        else if (curline_ == 0) {
            // Out of VBLANK
            g_t1 = false;

            clear_collisions();
        }
        else if (g_options.pal_emulation && scanlines_ == 21) { // clear external IRQ on line 21 for PAL systems
            cpu_->clear_external_irq();
        }

        ++scanlines_;
        curline_ = scanlines_ - first_drawing_scanline_;
    }
    else if (curline_ >= 0) {
        draw_background();

        if (grid_enabled())
            draw_grid();

        if (!in_hblank() && foreground_enabled()) {
            draw_chars();
            draw_quads();
            draw_sprites();
        }
    }

    ++cycles_;
    if (cycles_ == HBLANK_LENGTH) {
        // Out of HBLANK
        mem_[VDC_STATUS_REGISTER] |= 1 << 0;
        cpu_->counter_increment();
    }
}
