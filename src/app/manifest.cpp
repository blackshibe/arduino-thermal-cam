
#include "manifest.h"
#include "camera_app/camera_app.h"
#include "launcher/launcher.h"

// change NUM_APPS when adding an app
app_definition apps[] = {
	launcher_app::instance,
	camera_app::instance};
