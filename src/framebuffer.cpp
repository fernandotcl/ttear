#include "globals.h"

#include "framebuffer.h"

#include "options.h"

#include <iostream>
using namespace std;

const uint8_t Framebuffer::colortable_[24][3] = {
    // Light background or light grid colors
    { 95, 110, 107}, // dark gray
    {106, 161, 255}, // blue
    { 61, 240, 122}, // green
    { 49, 255, 255}, // cyan
    {255,  66,  85}, // red
    {255, 152, 255}, // violet
    {217, 173,  93}, // yellow
    {255, 255, 255}, // white

    // Dark background or dark grid colors
    {  0,   0,   0}, // black
    { 14,  61, 212}, // dark blue
    {  0, 152,  27}, // dark green
    {  0, 187, 217}, // cyan
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

Framebuffer::Framebuffer()
{
    window_size_.x_scale = (float)g_options.x_res / SCREEN_WIDTH;
    window_size_.y_scale = (float)g_options.y_res / SCREEN_HEIGHT;

    if (g_options.keep_aspect) {
        float scale = window_size_.x_scale < window_size_.y_scale ? window_size_.x_scale : window_size_.y_scale;

        window_size_.x = (g_options.x_res - scale * SCREEN_WIDTH) / 2;
        window_size_.y = (g_options.y_res - scale * SCREEN_HEIGHT) / 2;
        window_size_.x_end = window_size_.x + scale * SCREEN_WIDTH;
        window_size_.y_end = window_size_.y + scale * SCREEN_HEIGHT;
        window_size_.x_scale = window_size_.y_scale = scale;
    }
    else {
        window_size_.x = 0;
        window_size_.y = 0;
        window_size_.x_end = g_options.x_res;
        window_size_.y_end = g_options.y_res;
    }
}