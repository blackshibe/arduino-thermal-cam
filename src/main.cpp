#include "FS.h"
#include "SD_MMC.h"
#include "SPI.h"

#include "module/camera.h"
#include "module/tft.h"

#define GPIO_BUILTIN_FLASH 4
#define GPIO_BUILTIN_STATUS 33

#define BUTTON_UP 13
#define BUTTON_DOWN 16

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
	TFT_eSPI tft = get_tft();

	tft.pushImage(x, y, w, h, bitmap);

	return 1;
}

void blink_forever(int delay_ms) {
	while (true) {
		digitalWrite(GPIO_BUILTIN_STATUS, HIGH);
		delay(delay_ms);
		digitalWrite(GPIO_BUILTIN_STATUS, LOW);
		delay(delay_ms);
	}
}

void setup() {
	pinMode(GPIO_BUILTIN_FLASH, OUTPUT);
	pinMode(GPIO_BUILTIN_STATUS, OUTPUT);

	pinMode(BUTTON_UP, INPUT_PULLUP);
	pinMode(BUTTON_DOWN, INPUT_PULLUP);

	psramInit();

	TJpgDec.setJpgScale(1);
	TJpgDec.setSwapBytes(true);
	TJpgDec.setCallback(tft_output);

	// has to be done first so CS is set
	bool sd_status = SD_MMC.begin("/sdcard", true);

	TFT_eSPI tft = get_tft();
	camera_controller camera = get_camera();

	tft.init();
	tft.setOrigin(8, 0);
	tft.setRotation(0);
	tft.fillScreen(TFT_WHITE);
	tft.setTextColor(TFT_BLACK);
	tft.setTextPadding(8);

	esp_err_t camera_status = camera.init();

	if (camera_status == ESP_FAIL) {
		tft.printf("camera init failed");
		blink_forever(50);
	} else {
		tft.printf("display ready\n");
	}

	if (psramFound()) {
		tft.printf("PSRAM detected\n");
	}

	if (sd_status) {
		tft.printf("sd available\n");

		uint8_t cardType = SD_MMC.cardType();
		if (cardType == CARD_NONE) {
			tft.printf("no sd attached");

			return;
		}

		tft.print("sd card type: ");
		if (cardType == CARD_MMC)
			tft.printf("MMC\n");
		else if (cardType == CARD_SD)
			tft.printf("SDSC\n");
		else if (cardType == CARD_SDHC)
			tft.printf("SDHC\n");
		else
			tft.printf("UNKNOWN\n");

		uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
		tft.printf("sd space: %lluMB\n", cardSize);
	} else {
		tft.printf("no sd card inserted\n");
	}

	delay(1000);
	tft.fillScreen(TFT_BLACK);
}

void display_frame() {
	camera_fb_t *fb = esp_camera_fb_get();
	TFT_eSPI tft = get_tft();

	if (!fb) {
		tft.fillScreen(TFT_BLACK);
		tft.printf("failed to get fb\n");
		delay(100);
		esp_camera_fb_return(fb);

		return;
	}

	TJpgDec.drawJpg(0, 0, (const uint8_t *)fb->buf, fb->len);
	esp_camera_fb_return(fb);
}

void loop() {
	camera_controller camera = get_camera();
	TFT_eSPI tft = get_tft();

	int taking_photo = digitalRead(BUTTON_UP) == 0;
	int flash_on = digitalRead(BUTTON_DOWN) == 0;

	digitalWrite(GPIO_BUILTIN_FLASH, flash_on);

	esp_err_t status = camera.set_mode(taking_photo ? cameraControlMode::photo : cameraControlMode::preview);
	if (status != ESP_OK) {
		tft.fillScreen(TFT_BLACK);
		tft.printf("failed to set mode\n");
		delay(100);

		return;
	}

	display_frame();

	tft.setCursor(0, 240);
	tft.printf("total heap: %i\n", ESP.getHeapSize());
	tft.printf("free heap: %i\n", ESP.getFreeHeap());
}
