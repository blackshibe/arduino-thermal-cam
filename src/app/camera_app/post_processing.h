#pragma once

#include <cstdint>

namespace post_processing {
    void apply_steinberg_filter(uint16_t *bitmap, uint8_t *output_bitmap, uint16_t w, uint16_t h);
}