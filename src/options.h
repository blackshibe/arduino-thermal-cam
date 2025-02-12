#pragma once

#include <cstdint>

constexpr uint16_t PRINTER_DOWNSCALE = 4;
constexpr uint16_t PRINTER_BUFFER_WIDTH = 1920 / PRINTER_DOWNSCALE;
constexpr uint16_t PRINTER_BUFFER_HEIGHT = 1080 / PRINTER_DOWNSCALE;

constexpr uint16_t PRINTER_OUTPUT_WIDTH = 384;
