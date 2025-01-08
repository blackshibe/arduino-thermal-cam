#include "spiffs.h"

esp_err_t spiffs_controller::init() {
	
	return ESP_OK;
}

spiffs_controller *spiffs;
spiffs_controller get_spiffs() {
	if (spiffs == nullptr)
		spiffs = new spiffs_controller();

	return *spiffs;
}
