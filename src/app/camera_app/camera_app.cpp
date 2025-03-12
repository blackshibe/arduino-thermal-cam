#include "camera_app.h"

#include "config/options.h"
#include "module/camera.h"
#include "module/printer.h"
#include "module/tft.h"
#include "utils.h"
#include "SD_MMC.h"

uint8_t printer_buffer[PRINTER_BUFFER_WIDTH * (PRINTER_BUFFER_HEIGHT / 8)];

camera_app camera_app::instance;

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
	tft.pushImage(x, y, w, h, bitmap);

	return true;
}

// This code is from deepseek. The memory constraints of the ESP32 make this far more challenging 
// and time consuming than I can handle.
bool printer_jpg_callback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
    // 8x8 Bayer matrix for finer dithering (64 entries)
    static const uint8_t bayer_matrix[8][8] = {
        {  0, 32,  8, 40,  2, 34, 10, 42 },
        { 48, 16, 56, 24, 50, 18, 58, 26 },
        { 12, 44,  4, 36, 14, 46,  6, 38 },
        { 60, 28, 52, 20, 62, 30, 54, 22 },
        {  3, 35, 11, 43,  1, 33,  9, 41 },
        { 51, 19, 59, 27, 49, 17, 57, 25 },
        { 15, 47,  7, 39, 13, 45,  5, 37 },
        { 63, 31, 55, 23, 61, 29, 53, 21 }
    };

    // Gamma correction LUT (2.2 gamma)
    static uint8_t gamma_LUT[256];
    static bool lut_initialized = false;
    if (!lut_initialized) {
        for (int i = 0; i < 256; i++) {
            gamma_LUT[i] = static_cast<uint8_t>(pow(i / 255.0f, 2.2f) * 255.0f);
        }
        lut_initialized = true;
    }

    for (uint16_t off_y = 0; off_y < h; ++off_y) {
        for (uint16_t off_x = 0; off_x < w; ++off_x) {
            uint16_t pixel_idx = off_x + off_y * w;
            uint16_t pixel_x = x + off_x;
            if (pixel_x >= PRINTER_OUTPUT_WIDTH) continue;

            uint16_t pixel_y = y + off_y;

            // Convert RGB565 to linear RGB888
            uint8_t red, green, blue;
            convert_rgb565_to_rgb888(bitmap[pixel_idx], red, green, blue);

            // Apply gamma correction to lightness
            uint8_t linear_red = gamma_LUT[red];
            uint8_t linear_green = gamma_LUT[green];
            uint8_t linear_blue = gamma_LUT[blue];
            
            // Calculate linear lightness
            uint8_t lightness = static_cast<uint8_t>(
                0.2126f * linear_red + 
                0.7152f * linear_green + 
                0.0722f * linear_blue
            );

            // Get Bayer threshold (0-255)
            uint8_t threshold = bayer_matrix[off_y % 8][off_x % 8] * 4;

            // Apply dithering
            bool is_black = lightness <= threshold;
            uint16_t column_idx = get_pos58_buffer_idx(pixel_x, pixel_y);

            if (is_black) {
                printer_buffer[column_idx] |= (1 << get_pos58_buffer_pixel_offset(pixel_y));
            }

            tft.drawPixel(pixel_x, pixel_y, is_black ? TFT_BLACK : TFT_WHITE);
        }
    }
    return true;
}

void display_frame(camera_fb_t *fb, bool return_buffer = true) {
	if (!fb) {
		tft.fillScreen(TFT_BLACK);
		tft.printf("failed to get fb\n");
		delay(100);
		esp_camera_fb_return(fb);

		return;
	}

	TJpgDec.drawJpg(0, 0, (const uint8_t *)fb->buf, fb->len);
	if (return_buffer)
		esp_camera_fb_return(fb);
}

void camera_app::update() {
	int taking_photo = digitalRead(BUTTON_UP) == 0;
	int flash_on = digitalRead(BUTTON_DOWN) == 0;

	if (app_manifest::instance.is_first_update) {
		tft.setRotation(1);
		TJpgDec.setCallback(tft_output);
	}

	if (taking_photo) {
		digitalWrite(GPIO_BUILTIN_FLASH, true);
		esp_err_t status = camera_controller::instance.set_mode(cameraControlMode::photo);

		// wait for framebuffer to get populated
		delay(500);

		camera_fb_t *buffer = esp_camera_fb_get();
		digitalWrite(GPIO_BUILTIN_FLASH, false);

		TJpgDec.setJpgScale(8);
		tft.fillScreen(TFT_BLACK);
		display_frame(buffer, false);

		while (!flash_on) {
			flash_on = digitalRead(BUTTON_DOWN) == 0;
			delay(100);
		}

		tft.setRotation(2);
		tft.setTextColor(TFT_WHITE);

		tft.fillScreen(TFT_BLACK);
		tft.setCursor(0, 0);
		tft.printf("Preparing to upload\n");

		memset(printer_buffer, 0, sizeof(printer_buffer));
		TJpgDec.setJpgScale(PRINTER_DOWNSCALE);
		TJpgDec.setCallback(printer_jpg_callback);

		tft.setRotation(1);
		display_frame(buffer, false);

		delay(1000);
		while (!flash_on) {
			flash_on = digitalRead(BUTTON_DOWN) == 0;
			delay(100);
		}
		
		tft.fillScreen(TFT_BLACK);
		tft.setCursor(0, 0);
		tft.printf("Uploading\n");

		printer_controller::instance.printer.printPos58Bitmap(PRINTER_OUTPUT_WIDTH, PRINTER_BUFFER_HEIGHT, printer_buffer, false);
		printer_controller::instance.printer.println(); // leaves space to tear the page off
		printer_controller::instance.printer.println();
		printer_controller::instance.printer.println();

		// read camera photo count
		String photo_number;
		{
			fs::File cam_status = SPIFFS.open("/.camcount", FILE_READ);
			String str_photo_number = cam_status.readString();
			int next_photo_number = str_photo_number.toInt() + 1;
			String str_next_photo_number = String(next_photo_number);
			while (str_next_photo_number.length() < 3)
				str_next_photo_number = "0" + str_next_photo_number;
			photo_number = str_next_photo_number;

			cam_status.close();

			cam_status = SPIFFS.open("/.camcount", FILE_WRITE);
			cam_status.print(str_next_photo_number);
			cam_status.close();
		}

		String name = "/IMG_" + photo_number + ".jpg";

		tft.setRotation(2);
		tft.setTextSize(1);
		tft.fillScreen(TFT_WHITE);
		tft.setTextColor(TFT_BLACK);
		tft.printf("Saving photo %s\n", name);

		// SD is unusable while tft spi is active
		tft.getSPIinstance().end();
		esp_camera_deinit();

		delay(1000);
		
		// fs::File status = SPIFFS.open("/.camstatus", FILE_READ);
		bool sd_status = SD_MMC.begin("/sdcard", true);

		fs::File sd_image = SD_MMC.open(name, FILE_WRITE);
		fs::File cam_status = SPIFFS.open("/.camstatus", FILE_WRITE);

		cam_status.print(name);
		cam_status.close();

		sd_image.write(buffer->buf, buffer->len);
		sd_image.close();

		SD_MMC.end();
		ESP.restart();
	}

	camera_fb_t *fb = esp_camera_fb_get();
	display_frame(fb);
}

// String str_status = status.readString();
// String message = "No message when syncing SPIFFS";
// bool fast_init = str_status.length() > 0;

// free up space by not initializing the sd card at all when not used
// if (!spiffs_status)
// 	message = "SPIFFS failed to initialize, cannot use SD card";
// if (spiffs_status && fast_init) {
// 	// has to be done first so CS is set
// 	// the sd card is only available during boot

// 	message = "Saved photo to SD";
// 	move_file(SPIFFS, SD_MMC, "/.transport", str_status.c_str());

// 	// clear camstatus
// 	status.close();

// 	status = SPIFFS.open("/.camstatus", FILE_WRITE);
// 	status.print("");
// 	status.close();
// }

// write

// int next = 1000;
// bool is_cooked = false; // NOTE: temporary - going back a byte didn't work in testing, so for now the entire buffer is rewritten just so the photos actually save correctly
// do {
// 	is_cooked = false;
// 	sd_image.flush();
// 	sd_image.seek(0);

// 	for (size_t i = 0; i < buffer->len; i++) {
// 		if (next == 0) {
// 			tft.fillRect(0, 0, 80, 20, TFT_WHITE);
// 			tft.setCursor(0, 0);
// 			tft.printf("%d/%dKB\n", (int)(i * 0.001f), (int)(buffer->len * 0.001f));
// 			next = 1000;
// 		}

// 		// Sometimes the byte doesn't get written, so we retry
// 		if (!sd_image.write(buffer->buf[i])) {
// 			tft.fillScreen(TFT_RED);
// 			tft.setCursor(0, 0);
// 			tft.printf("Failed to write at offset: %d.\nRetrying to write the photo.", i);
// 			delay(2000);
// 			tft.fillScreen(TFT_WHITE);

// 			is_cooked = true;
// 			break;
// 		}

// 		next -= 1;
// 	};
// } while (is_cooked);