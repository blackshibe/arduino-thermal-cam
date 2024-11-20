// See SetupX_Template.h for all options available
#define USER_SETUP_ID 1

#define ILI9341_DRIVER

#define TFT_MISO 13
#define TFT_MOSI 12
#define TFT_SCLK 14
#define TFT_CS 15 // Chip select control pin
#define TFT_DC 2  // Data Command control pin
// #define TFT_RST 16  // Reset pin (Connected to 3V3)

#define LOAD_GLCD // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH

#define SMOOTH_FONT

#define SPI_FREQUENCY 60000000