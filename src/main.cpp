#include "FS.h"
#include "SD_MMC.h"
#include "app/camera_app/post_processing.h"
#include "app/manifest.h"
#include "config/options.h"
#include "utils.h"
#include <HardwareSerial.h>

#include "config/camera_pins.h"
#include "module/camera.h"
#include "module/tft.h"

#include "esp_camera.h"
#include <WiFi.h>

// Make sure to add this file
#include "config/wifi.h"

void startCameraServer();
void setupLedFlash(int pin);

bool listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
	tft.printf("Listing directory: %s\n", dirname);

	File root = fs.open(dirname);
	if (!root) {
		tft.println("Failed to open directory");
		return false;
	}
	if (!root.isDirectory()) {
		tft.println("Not a directory");
		return false;
	}

	File file = root.openNextFile();
	while (file) {
		if (file.isDirectory()) {
			tft.print("  DIR : ");
			tft.println(file.name());
			if (levels) {
				listDir(fs, file.name(), levels - 1);
			}
		} else {
			tft.print("  FILE: ");
			tft.print(file.name());
			tft.print("  SIZE: ");
			tft.println(file.size());
		}
		file = root.openNextFile();
	}

	return true;
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
	tft.pushImage(x, y, w, h, bitmap);

	return true;
}

void blink_forever(int delay_ms) {
	while (true) {
		digitalWrite(GPIO_BUILTIN_STATUS, HIGH);
		delay(delay_ms);
		digitalWrite(GPIO_BUILTIN_STATUS, LOW);
		delay(delay_ms);
	}
}

// move from fs::FS one to another
bool move_file(fs::FS &fs_start, fs::FS &fs_finish, const char *path1, const char *path2) {
	File file = fs_start.open(path1, FILE_READ);
	if (!file) {
		return false;
	}

	// create a new file
	File newFile = fs_finish.open(path2, FILE_WRITE);
	if (!newFile) {
		return false;
	}

	while (file.available()) {
		newFile.write(file.read());
	}

	file.close();
	newFile.close();

	return true;
}

void setup() {
	pinMode(GPIO_BUILTIN_FLASH, OUTPUT);
	pinMode(GPIO_BUILTIN_STATUS, OUTPUT);

	psramInit();
	micros();

	// EARLY BOOT: SD CARD DOWNLOAD MODE
	bool spiffs_status = SPIFFS.begin(true);

	fs::File status = SPIFFS.open("/.camstatus", FILE_READ);
	String str_status = status.readString();
	String message = "No message when syncing SPIFFS";
	bool fast_init = str_status.length() > 0;

	// free up space by not initializing the sd card at all when not used
	if (!spiffs_status)
		message = "SPIFFS failed to initialize, cannot use SD card";
	if (spiffs_status && fast_init) {
		// has to be done first so CS is set
		// the sd card is only available during boot
		bool sd_status = SD_MMC.begin("/sdcard", true);

		message = "Saved photo to SD";
		move_file(SPIFFS, SD_MMC, "/.transport", str_status.c_str());

		// clear camstatus
		status.close();

		status = SPIFFS.open("/.camstatus", FILE_WRITE);
		status.print("");
		status.close();
	}

	// LATE BOOT: TFT, CAMERA, PRINTER
	pinMode(BUTTON_UP, INPUT_PULLUP);
	// pinMode(BUTTON_DOWN, INPUT_PULLUP);

	TJpgDec.setJpgScale(1);
	TJpgDec.setSwapBytes(true);
	TJpgDec.setCallback(tft_output);

	// tft is already initialized
	tft.init();
	tft.setCursor(0, 0);
	tft.setTextSize(1);
	tft.setRotation(2);
	tft.setTextColor(TFT_BLACK);

	if (!fast_init) {
		tft.fillScreen(TFT_WHITE);
	} else {
		tft.fillRect(0, 0, 160, 60, TFT_WHITE);
	}

	tft.println(message);
	tft.println(str_status);

	esp_err_t camera_status = camera_controller::instance.init();

	if (!fast_init) {
		if (camera_status == ESP_FAIL) {
			tft.printf("camera init failed");
			blink_forever(50);
		} else {
			tft.printf("camera ready\n");
		}

		if (psramFound()) {
			tft.printf("PSRAM detected\n");
		}

		tft.println("UP to continue");
	}

	while (digitalRead(BUTTON_UP) == 1 && !fast_init) {
		delay(10);
	}

	while (digitalRead(BUTTON_UP) == 0 && !fast_init) {
		delay(10);
	}

	if (digitalRead(BUTTON_DOWN) == 0) {
		WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
		WiFi.setSleep(false);

		while (WiFi.status() != WL_CONNECTED) {
			delay(500);
			tft.print(".");
		}
		tft.println("");
		tft.println("WiFi connected");

		startCameraServer();

		tft.print("Camera Ready! Use 'http://");
		tft.print(WiFi.localIP());
		tft.println("' to connect");
	}

	esp_err_t __status = camera_controller::instance.set_mode(cameraControlMode::preview);

	delay(300);

	tft.fillScreen(TFT_BLACK);
	tft.setRotation(1);
}

unsigned long next_frame_time;
void loop() {
	if (millis() < next_frame_time) {
		delay(16);
	}

	for (int i = 0; i < NUM_APPS; i++) {
		app_definition app = app_manifest::apps[i];
		if (app_manifest::open_app != i)
			continue;

		app.update();
	}

	// 30fps target
	next_frame_time = millis() + 32;
}
