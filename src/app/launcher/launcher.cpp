#include "launcher.h"

#include "module/tft.h"

launcher_app launcher_app::instance;

void launcher_app::update() {
	tft.fillScreen(TFT_BLACK);
	tft.println("Hello world!");
}