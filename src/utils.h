#pragma once

#include "options.h"
#include <cstdint>

inline void convert_rgb565_to_rgb888(uint16_t value, uint8_t &r, uint8_t &g, uint8_t &b) {
    r = (value >> 8) & 0xF8; r |= (r >> 5);
    g = (value >> 3) & 0xFC; g |= (g >> 6);
    b = (value << 3) & 0xF8; b |= (b >> 5);
}

inline uint16_t get_pos58_buffer_idx(uint16_t x, uint16_t y) {
    uint16_t raw_idx = y / 8;
    return PRINTER_OUTPUT_WIDTH * raw_idx + x;
}

inline uint16_t get_pos58_buffer_pixel_offset(uint16_t y) {
    return y % 8;
}