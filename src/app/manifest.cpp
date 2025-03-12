
#include "manifest.h"
#include "camera_app/camera_app.h"
#include "launcher/launcher.h"

app_manifest app_manifest::instance;

// change NUM_APPS when adding an app
app_definition* app_manifest::apps[] = {
	&launcher_app::instance,
	&camera_app::instance};

const char* app_manifest::names[] = {
	"Launcher",
	"Camera"};