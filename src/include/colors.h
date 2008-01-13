#ifndef COLORS_H
#define COLORS_H

#include "common.h"

static const uint8_t colortable[24][3] = {
    // Sprite and char colors
    { 95, 110, 107}, // dark gray
    {255,  66,  85}, // red
    { 61, 240, 122}, // green
    {217, 173,  93}, // yellow
    {106, 161, 255}, // blue
    {255, 152, 255}, // violet
    { 49, 255, 255}, // cyan
    {255, 255, 255}, // white

    // Light background and grid colors
    { 95, 110, 107}, // dark gray
    {106, 161, 255}, // blue
    { 61, 240, 122}, // green
    { 49, 255, 255}, // cyan
    {255,  66,  85}, // red
    {255, 152, 255}, // violet
    {217, 173,  93}, // yellow
    {255, 255, 255}, // white

    // Dark background and grid colors
    {  0,   0,   0}, // black
    { 14,  61, 212}, // dark blue
    {  0, 152,  27}, // dark green
    {  0, 187, 217}, // cyan
    {199,   0,   8}, // red
    {204,  22, 179}, // violet
    {157, 135,  16}, // orange
    {225, 209, 225}  // light gray
};

static const int COLORTABLE_CHAR_OFFSET = 0;
static const int COLORTABLE_SPRITE_OFFSET = 0;
static const int COLORTABLE_BACKGROUND_OFFSET = 8;
static const int COLORTABLE_GRID_OFFSET = 8;
static const int COLORTABLE_LIGHT_BACKGROUND_OFFSET = 16;
static const int COLORTABLE_LIGHT_GRID_OFFSET = 16;

#endif
