#include "app/manifest.h"

struct camera_app : app_definition {
	static camera_app instance;

	int id = 1;
	void update();
};