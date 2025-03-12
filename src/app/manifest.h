#pragma once

struct app_definition {
	virtual void update() {};
};

#define NUM_APPS 2

struct app_manifest {
	static int open_app;
	static app_definition apps[];
	static void transition_app(int new_app) {
		app_manifest::open_app = new_app;
	}

	app_manifest() {
		open_app = 0;
	}
};
