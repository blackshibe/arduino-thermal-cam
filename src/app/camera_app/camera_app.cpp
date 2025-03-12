#include "camera_app.h"

#include "config/options.h"
#include "module/camera.h"
#include "module/printer.h"
#include "module/tft.h"
#include "utils.h"

uint8_t printer_buffer[PRINTER_BUFFER_WIDTH * (PRINTER_BUFFER_HEIGHT / 8)];

camera_app camera_app::instance;

bool printer_jpg_callback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
	// constexpr int16_t CROP_OFFSET = (PRINTER_OUTPUT_WIDTH - PRINTER_BUFFER_WIDTH) / 2;
	int16_t error_buffer[2][PRINTER_OUTPUT_WIDTH] = {0};

	for (uint16_t off_y = 0; off_y < h; ++off_y) {
		for (uint16_t off_x = 0; off_x < w; ++off_x) {
			uint16_t pixel_idx = off_x + off_y * w;
			uint16_t pixel_x = x + off_x;

			if (pixel_x >= PRINTER_OUTPUT_WIDTH) {
				continue;
			}

			uint8_t red, green, blue;
			convert_rgb565_to_rgb888(bitmap[pixel_idx], red, green, blue);
			int16_t gray = static_cast<uint16_t>(0.299f * red + 0.587f * green + 0.114f * blue);

			gray += error_buffer[0][off_x];
			int16_t new_pixel = (gray > 100) ? 255 : 0;
			int16_t error = gray - new_pixel;

			uint16_t pixel_y = y + off_y;
			uint16_t column_idx = get_pos58_buffer_idx(pixel_x, pixel_y);
			bool is_black = !new_pixel;

			if (is_black) {
				printer_buffer[column_idx] |= (1 << get_pos58_buffer_pixel_offset(pixel_y));
			}

			tft.drawPixel(pixel_x, pixel_y, is_black ? TFT_BLACK : TFT_WHITE);
		}

		for (uint16_t i = 0; i < w; ++i) {
			error_buffer[0][i] = error_buffer[1][i];
			error_buffer[1][i] = 0;
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

	if (taking_photo) {
		digitalWrite(GPIO_BUILTIN_FLASH, true);
		esp_err_t status = camera_controller::instance.set_mode(cameraControlMode::photo);

		// wait for framebuffer to get populated
		delay(500);

		camera_fb_t *buffer = esp_camera_fb_get();

		// the buffer gets overwritten before it's read
		TJpgDec.setJpgScale(PRINTER_DOWNSCALE);
		digitalWrite(GPIO_BUILTIN_FLASH, false);
		display_frame(buffer, false);

		delay(1000);

		tft.fillScreen(TFT_BLACK);
		tft.setRotation(2);
		tft.printf("Preparing to upload\n");

		memset(printer_buffer, 0, sizeof(printer_buffer));
		TJpgDec.setJpgScale(PRINTER_DOWNSCALE);
		TJpgDec.setCallback(printer_jpg_callback);
		display_frame(buffer, false);

		delay(1000);

		printer_controller::instance.printer.printPos58Bitmap(PRINTER_OUTPUT_WIDTH, PRINTER_BUFFER_HEIGHT, printer_buffer, false);
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

		// The SPIFFS filesystem is not big enough to store more information than a single image
		SPIFFS.remove(".transport");

		String name = "/IMG_" + photo_number + ".jpg";
		fs::File spiff_file = SPIFFS.open("/.transport", FILE_WRITE);
		fs::File cam_status = SPIFFS.open("/.camstatus", FILE_WRITE);

		cam_status.print(name);
		cam_status.close();

		esp_camera_deinit();

		// write
		tft.fillScreen(TFT_WHITE);
		tft.setTextColor(TFT_BLACK);
		int next = 1000;
		bool is_cooked = false; // NOTE: temporary - going back a byte didn't work in testing, so for now the entire buffer is rewritten just so the photos actually save correctly
		do {
			is_cooked = false;
			spiff_file.flush();
			spiff_file.seek(0);

			for (size_t i = 0; i < buffer->len; i++) {
				if (next == 0) {
					tft.fillRect(0, 0, 80, 20, TFT_WHITE);
					tft.setCursor(0, 0);
					tft.printf("%d/%dKB\n", (int)(i * 0.001f), (int)(buffer->len * 0.001f));
					next = 1000;
				}

				// Sometimes the byte doesn't get written, so we retry
				if (!spiff_file.write(buffer->buf[i])) {
					tft.fillScreen(TFT_RED);
					tft.setCursor(0, 0);
					tft.printf("Failed to write at offset: %d.\nRetrying to write the photo.", i);
					delay(2000);
					tft.fillScreen(TFT_WHITE);

					is_cooked = true;
					break;
				}

				next -= 1;
			};
		} while (is_cooked);

		spiff_file.close();

		tft.fillScreen(TFT_WHITE);
		tft.printf("Uploading to SDCard\n.");
		tft.setCursor(0, 1000);

		esp_camera_fb_return(buffer);
		// time to restart
		// TO TEST: see if doing this will free up SPI for writing to the SD card correctly
		tft.getSPIinstance().end();
		SPIFFS.end();
		ESP.restart();
	}

	camera_fb_t *fb = esp_camera_fb_get();
	display_frame(fb);
}