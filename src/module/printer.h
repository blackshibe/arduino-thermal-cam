#pragma once

#include "Adafruit_Thermal.h"
#include "esp_err.h"
#include "SPI.h"

struct printer_controller {
	esp_err_t init();

	HardwareSerial serial;
	Adafruit_Thermal printer;

	static printer_controller instance;

	printer_controller(): serial(1), printer(&serial) {}
};
