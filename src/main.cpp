#include "FS.h"
#include "SD_MMC.h"
#include "SPI.h"
#include "Adafruit_Thermal.h"
#include <HardwareSerial.h> 

#include "camera_pins.h"
#include "module/camera.h"
#include "module/tft.h"

#include "esp_camera.h"
#include <WiFi.h>


// ===========================
// Enter your WiFi credentials
// ===========================
const char *ssid = "Aha";
const char *password = "VD08RZ17U6MJ9MPR";

void startCameraServer();
void setupLedFlash(int pin);

#define GPIO_BUILTIN_FLASH 4
#define GPIO_BUILTIN_STATUS 33

#define BUTTON_UP 13
#define BUTTON_DOWN 16 

const int printerBaudrate = 9600;  // or 19200
const byte rxPin = -1;   // useless after printer initialization
const byte txPin = 3;   

HardwareSerial mySerial(1);
Adafruit_Thermal myPrinter(&mySerial);

bool listDir(fs::FS &fs, const char * dirname, uint8_t levels){
	TFT_eSPI tft = get_tft();
    tft.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        tft.println("Failed to open directory");
        return false;
    }
    if(!root.isDirectory()){
        tft.println("Not a directory");
        return  false;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            tft.print("  DIR : ");
            tft.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
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

// move from fs::FS one to another
bool move_file(fs::FS &fs_start, fs::FS &fs_finish, const char * path1, const char * path2) {
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
    if (!spiffs_status) message = "SPIFFS failed to initialize, cannot use SD card";
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
	pinMode(txPin, OUTPUT);

	TJpgDec.setJpgScale(1);
	TJpgDec.setSwapBytes(true);
	TJpgDec.setCallback(tft_output);

	TFT_eSPI tft = get_tft();
	camera_controller camera = get_camera();

	tft.init();
	tft.setCursor(0, 0);
    tft.setTextSize(1);
	tft.setRotation(2);
	tft.setTextColor(TFT_BLACK);
	tft.fillScreen(TFT_WHITE);
	tft.println(message);
    tft.println(str_status);

	mySerial.begin(printerBaudrate, SERIAL_8N1, rxPin, txPin);  // must be 8N1 mode
	myPrinter.begin();

	esp_err_t camera_status = camera.init();

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
        WiFi.begin(ssid, password);
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

	esp_err_t __status = camera.set_mode(cameraControlMode::preview);

    delay(300);

	tft.fillScreen(TFT_BLACK);
	tft.setRotation(1);

}

void display_frame(camera_fb_t *fb, bool return_buffer = true) {
	TFT_eSPI tft = get_tft();

	if (!fb) {
		tft.fillScreen(TFT_BLACK);
		tft.printf("failed to get fb\n");
		delay(100);
		esp_camera_fb_return(fb);

		return;
	}

	TJpgDec.drawJpg(0, 0, (const uint8_t *)fb->buf, fb->len);
	if (return_buffer) esp_camera_fb_return(fb);
}

void loop() {
	camera_controller camera = get_camera();
	TFT_eSPI tft = get_tft();

	int taking_photo = digitalRead(BUTTON_UP) == 0;
	int flash_on = digitalRead(BUTTON_DOWN) == 0;

	// if (taking_photo) {
	// 	fs::File map = SPIFFS.open("/FOX_BMP.bin", FILE_READ);

	// 	if (!map.available()) {
	// 		tft.fillScreen(TFT_BLACK);
	// 		tft.printf("failed to open file\n");
	// 		delay(2000);

	// 		return;
	// 	}

	// 	tft.fillScreen(TFT_BLACK);
	// 	tft.printf("printing bitmap\n");

	// 	int size = map.size();
	// 	tft.println(size);

	// 	delay(2000);

	// 	// http://www.easovation.com/wp-content/uploads/2018/06/HOPE58.pdf
	// 	// The default Adafruit driver sends commands not mentioned in the datasheet at all
	// 	// POS58 also draws each byte as a column, 8 pixels high making things even more annoying
	// 	// The datasheet and driver mentions the name "POS58", which i believe means "Piece of Shit",
	// 	// but the datasheet says HOPE58, so maybe there's some funny innuendo around this
	// 	tft.printf("starting\n");
	// 	void *buffer = malloc(map.size());
	// 	tft.printf("allocated buffer\n");
	// 	map.readBytes((char*)buffer, map.size());
	// 	tft.printf("loaded buffer\n");

	// 	delay(1000);

	// 	myPrinter.printPos58Bitmap(384, 384, (const uint8_t *)buffer, false);
	// 	// free(buffer);

	// 	delay(10000);
	// 	abort();
	// }
	
	if (taking_photo) {

		digitalWrite(GPIO_BUILTIN_FLASH, true);
		esp_err_t status = camera.set_mode(cameraControlMode::photo);

        // wait for framebuffer to get populated
        delay(500);

		camera_fb_t* buffer = esp_camera_fb_get();

        // the buffer gets overwritten before it's read
        TJpgDec.setJpgScale(8);
		digitalWrite(GPIO_BUILTIN_FLASH, false);
		display_frame(buffer, false);

        delay(1000);

        tft.fillScreen(TFT_BLACK);
        tft.setRotation(2);
        tft.printf("Preparing to upload\n");

        // read camera photo count
        String photo_number;
        {
            fs::File cam_status = SPIFFS.open("/.camcount", FILE_READ);
            String str_photo_number = cam_status.readString();
            int next_photo_number = str_photo_number.toInt() + 1;
            String str_next_photo_number = String(next_photo_number);
            while (str_next_photo_number.length() < 3) str_next_photo_number = "0" + str_next_photo_number;
            photo_number = str_next_photo_number;

            cam_status.close();

            cam_status = SPIFFS.open("/.camcount", FILE_WRITE);
            cam_status.print(str_next_photo_number);
            cam_status.close();
        }

        // The SPIFFS filesystem is not big enough to store more information than a single image
        SPIFFS.remove(".transport");
        
        String name = "/IMG_" + photo_number + ".jpg";
		fs::File file = SPIFFS.open("/.transport", FILE_WRITE);
		fs::File cam_status = SPIFFS.open("/.camstatus", FILE_WRITE);

		cam_status.print(name);
		cam_status.close();

        esp_camera_deinit();

        // write   
        tft.fillScreen(TFT_BLACK);
        int next = 1000;
        for (size_t i = 0; i < buffer->len; i++) {
            if (next == 0) {
                tft.fillRect(0, 0, 80, 20, TFT_BLACK);
                tft.setCursor(0, 0);
                tft.printf("%d/%dKB\n", (int)(i * 0.001f), (int)(buffer->len * 0.001f));
                next = 1000;
            }

            file.write(buffer->buf[i]);
            next -= 1;
        };

		file.close();

        tft.fillScreen(TFT_BLACK);
        tft.printf("Uploading to SDCard\n.");
        tft.setCursor(0, 100000);

        esp_camera_fb_return(buffer);
		// time to restart
        SPIFFS.end();
		ESP.restart();


	}

	camera_fb_t *fb = esp_camera_fb_get();
	display_frame(fb);
}
