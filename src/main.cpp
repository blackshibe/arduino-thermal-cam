#include "SPI.h"
#include "TFT_eSPI.h"

#include "FS.h"
#include "SD_MMC.h"

#include "esp_camera.h"
#include "camera_pins.h"

TFT_eSPI tft = TFT_eSPI();

#define GPIO_BUILTIN_FLASH 4
#define GPIO_BUILTIN_STATUS 33

#define BUTTON_UP 13
#define BUTTON_DOWN 16 

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

	// if (!SD_MMC.begin("/sdcard", true)) blink_forever(50);

	tft.init();
	tft.setRotation(2);
	tft.fillScreen(TFT_BLACK);
	tft.println("Mounting MicroSD Card");

	uint8_t cardType = SD_MMC.cardType();
	if (cardType == CARD_NONE) {
		tft.println("No SD card attached");
		return;
	}

	tft.print("SD Card Type: ");
	if (cardType == CARD_MMC) tft.println("MMC");
	else if (cardType == CARD_SD) tft.println("SDSC");
	else if (cardType == CARD_SDHC) tft.println("SDHC");
	else tft.println("UNKNOWN");

	uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
	tft.printf("SD Card Size: %lluMB\n", cardSize);

	delay(2000);

	camera_config_t config;
	config.ledc_channel = LEDC_CHANNEL_0;
	config.ledc_timer = LEDC_TIMER_0;
	config.pin_d0 = Y2_GPIO_NUM;
	config.pin_d1 = Y3_GPIO_NUM;
	config.pin_d2 = Y4_GPIO_NUM;
	config.pin_d3 = Y5_GPIO_NUM;
	config.pin_d4 = Y6_GPIO_NUM;
	config.pin_d5 = Y7_GPIO_NUM;
	config.pin_d6 = Y8_GPIO_NUM;
	config.pin_d7 = Y9_GPIO_NUM;
	config.pin_xclk = XCLK_GPIO_NUM;
	config.pin_pclk = PCLK_GPIO_NUM;
	config.pin_vsync = VSYNC_GPIO_NUM;
	config.pin_href = HREF_GPIO_NUM;
	config.pin_sccb_sda = SIOD_GPIO_NUM;
	config.pin_sccb_scl = SIOC_GPIO_NUM;
	config.pin_pwdn = PWDN_GPIO_NUM;
	config.pin_reset = RESET_GPIO_NUM;
	config.xclk_freq_hz = 20000000;
	config.frame_size = FRAMESIZE_QQVGA;
	config.pixel_format = PIXFORMAT_RGB565;
	config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
	config.fb_location = CAMERA_FB_IN_PSRAM;
	config.jpeg_quality = 12;
	config.fb_count = 1;

	esp_err_t err = esp_camera_init(&config);
	if (err != ESP_OK) {
		tft.printf("Camera init failed with error 0x%x", err);
		return;
	}
	
	tft.printf("camera initialized\n");
	delay(2000);
}


void loop() {
	tft.fillScreen(TFT_BLACK);
	tft.setCursor(0, 0);
	tft.setTextColor(TFT_WHITE, TFT_BLACK);
	tft.setTextSize(2);

	// tft.println(digitalRead(BUTTON_UP));
	// tft.println(digitalRead(BUTTON_DOWN));

	delay(128);

	if (digitalRead(BUTTON_UP) == 0) {

		digitalWrite(GPIO_BUILTIN_FLASH, HIGH);
		delay(16);

		camera_fb_t * fb = NULL;

		fb = esp_camera_fb_get();  
		digitalWrite(GPIO_BUILTIN_FLASH, LOW);

		if(!fb) {
			tft.println("Camera capture failed");
			delay(1000);

			return;
		}



		uint8_t *data = fb->buf;
		size_t size = fb->len;
		
		tft.setRotation(1);
		tft.setAddrWindow(0, 0, 160, 120);
		tft.pushColors(data, size);
		tft.setRotation(2);
		

		esp_camera_fb_return(fb); 
		delay(5000);
	}
}
