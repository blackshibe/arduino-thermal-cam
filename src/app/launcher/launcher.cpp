#include "launcher.h"

#include "module/tft.h"
#include "config/options.h"

launcher_app launcher_app::instance;

#define ENTRY_SIZE 64

int selection = 1;

void launcher_app::update() {	
	int button_up_pressed = digitalRead(BUTTON_UP) == 0;
	int button_down_pressed = digitalRead(BUTTON_DOWN) == 0;

	if (app_manifest::instance.is_first_update) {
		tft.fillScreen(TFT_BLACK);
		tft.setTextColor(TFT_WHITE);
		tft.setRotation(2);
		tft.setCursor(8, 302);

		tft.setTextSize(1);
		tft.printf("UP to select, DOWN to change selection");

		tft.setTextSize(3);
	}

	// skip first app (launcher itself)
	for (int i = 1; i < NUM_APPS; i++) {
		bool selected = selection == i;
		int offset = (i - 1) * ENTRY_SIZE;

		tft.fillRect(0, offset, 240, ENTRY_SIZE, selection == i ? TFT_WHITE : TFT_BLACK);
		tft.setCursor(8, offset + 8);
		tft.setTextColor(selected ? TFT_BLACK : TFT_WHITE);
		tft.printf(app_manifest::names[i]);
	}

	if (button_down_pressed) {
		selection++;
		if (selection >= NUM_APPS) {
			selection = 1;
		}
	}
	
	if (button_up_pressed) {
		app_manifest::instance.transition_app(selection);
	}

	while (button_down_pressed || button_up_pressed) {
		button_up_pressed = digitalRead(BUTTON_UP) == 0;
		button_down_pressed = digitalRead(BUTTON_DOWN) == 0;
		delay(16);
	}

}