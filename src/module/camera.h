#pragma once

#include "../config/camera_pins.h"
#include "esp_camera.h"
#include "tft.h"

enum cameraControlMode {
	preview,
	photo,
	unset
};

struct camera_controller {
	cameraControlMode mode;

	esp_err_t init();
	esp_err_t print_dbg_info(TFT_eSPI tft);
	esp_err_t set_mode(cameraControlMode new_mode);

	static camera_controller instance;
};
