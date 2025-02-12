#include "post_processing.h"
#include "utils.h"
#include <cstdlib>

void post_processing::apply_steinberg_filter(uint16_t *bitmap, uint8_t *output_bitmap, uint16_t w, uint16_t h) {
    /*
    uint8_t luminance_buffer[w * h];

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint16_t idx = x + y * w;
            uint8_t r,g,b;
            convert_rgb565_to_rgb888(bitmap[idx], r, g, b);

            float luminance = r * 0.299 + g * 0.587 + b * 0.114;
            luminance_buffer[idx] = luminance * 255;
        }
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint16_t idx = x + y * w;
            uint8_t new_pixel = luminance_buffer[idx] < 129 ? 0 : 255;
            uint8_t err = (luminance_buffer[idx] - new_pixel) / 16;

            output_bitmap[idx] = new_pixel;

            output_bitmap[idx + 4] += err * 7;
            output_bitmap[idx + 4 * w - 4] += err * 3;
            output_bitmap[idx + 4 * w] += err * 5;
            output_bitmap[idx + 4 * w + 4] += err * 1;
        }
    }
    */
}