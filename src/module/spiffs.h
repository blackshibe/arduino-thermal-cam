#pragma once

#include "esp_err.h"

struct spiffs_controller {
	esp_err_t init();
	static spiffs_controller instance;
};
