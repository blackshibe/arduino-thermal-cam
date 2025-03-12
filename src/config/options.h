#pragma once

#include <cstdint>

constexpr uint16_t PRINTER_DOWNSCALE = 4;
constexpr uint16_t PRINTER_BUFFER_WIDTH = 1920 / PRINTER_DOWNSCALE;
constexpr uint16_t PRINTER_BUFFER_HEIGHT = 1080 / PRINTER_DOWNSCALE;

constexpr uint16_t PRINTER_OUTPUT_WIDTH = 384;

constexpr uint8_t GPIO_BUILTIN_FLASH = 4;
constexpr uint8_t GPIO_BUILTIN_STATUS = 33;

constexpr uint8_t BUTTON_UP = 13;
constexpr uint8_t BUTTON_DOWN = 16;