#pragma once

#include "Adafruit_Thermal.h"
#include "esp_err.h"

struct printer_controller {
	esp_err_t init();

	Adafruit_Thermal printer;

	static printer_controller instance;
};
