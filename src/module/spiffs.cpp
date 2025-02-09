#include "spiffs.h"

spiffs_controller spiffs_controller::instance;

esp_err_t spiffs_controller::init() {
	return ESP_OK;
}
