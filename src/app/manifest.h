#pragma once

struct app_definition {
	virtual void update();
	const char* name;
};

#define NUM_APPS 2

struct app_manifest {
	static app_definition* apps[];
	static const char* names[];
	static app_manifest instance;

	int open_app = 0;
	bool is_first_update;
	const void transition_app(int new_app) {
		open_app = new_app;
		is_first_update = true;
	}
};
