#pragma once

#include "defines.h"

struct platform_info {
	int screen_width;
	int screen_height;
	const char *title;
};

struct platform_state {
	void *internal_state;
	bool quit;
};

namespace platform {
	bool initialize(platform_state *platform, platform_info *info);
	void shutdown(platform_state *platform);
	void pump_messages(platform_state *platform);
	bool should_stop(platform_state *platform);
	void set_should_stop(platform_state *platform, bool value);
}