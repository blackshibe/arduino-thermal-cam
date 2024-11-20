#include "camera.h"
#include "tft.h"

esp_err_t camera_controller::print_dbg_info(TFT_eSPI tft) {
	sensor_t *sensor = esp_camera_sensor_get();
	if (!sensor) {
		tft.printf("failed to get sensor\n");
		return ESP_FAIL;
	}

	sensor_id_t id;
	id.MIDH = sensor->id.MIDH;
	id.MIDL = sensor->id.MIDL;
	id.PID = sensor->id.PID;
	id.VER = sensor->id.VER;

	camera_sensor_info_t *info = esp_camera_sensor_get_info(&id);
	if (info == nullptr) {
		tft.printf("failed to get sensor info\n");
		return ESP_FAIL;
	}

	tft.printf("name: %s\n", info->name);

	return ESP_OK;
}

esp_err_t camera_controller::set_mode(cameraControlMode new_mode) {
	sensor_t *sensor = esp_camera_sensor_get();
	if (!sensor)
		return ESP_FAIL;

	this->mode = new_mode;
	if (new_mode == cameraControlMode::photo) {
		sensor->set_quality(sensor, 10);
	} else {
		sensor->set_quality(sensor, 60);
	}

	return ESP_OK;
};

esp_err_t camera_controller::init() {
	camera_config_t config;
	config.ledc_channel = LEDC_CHANNEL_0;
	config.ledc_timer = LEDC_TIMER_0;
	config.pin_d0 = Y2_GPIO_NUM;
	config.pin_d1 = Y3_GPIO_NUM;
	config.pin_d2 = Y4_GPIO_NUM;
	config.pin_d3 = Y5_GPIO_NUM;
	config.pin_d4 = Y6_GPIO_NUM;
	config.pin_d5 = Y7_GPIO_NUM;
	config.pin_d6 = Y8_GPIO_NUM;
	config.pin_d7 = Y9_GPIO_NUM;
	config.pin_xclk = XCLK_GPIO_NUM;
	config.pin_pclk = PCLK_GPIO_NUM;
	config.pin_vsync = VSYNC_GPIO_NUM;
	config.pin_href = HREF_GPIO_NUM;
	config.pin_sccb_sda = SIOD_GPIO_NUM;
	config.pin_sccb_scl = SIOC_GPIO_NUM;
	config.pin_pwdn = PWDN_GPIO_NUM;
	config.pin_reset = RESET_GPIO_NUM;

	config.xclk_freq_hz = 20000000;
	config.pixel_format = PIXFORMAT_JPEG;
	config.grab_mode = CAMERA_GRAB_LATEST;
	config.fb_location = CAMERA_FB_IN_DRAM;

	config.frame_size = FRAMESIZE_240X240;
	config.jpeg_quality = 60;
	config.fb_count = 2;

	this->mode = cameraControlMode::unset;

	esp_err_t err = esp_camera_init(&config);

	return err;
}

camera_controller *camera;
camera_controller get_camera() {
	if (camera == nullptr)
		camera = new camera_controller();

	return *camera;
}
