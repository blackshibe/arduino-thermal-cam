#include "../manifest.h"

struct launcher_app : app_definition {
	static launcher_app instance;
	int id = 0;
	void update();
};