#include "sxlogger.h"
#include "platform/platform.h"

#include <stdio.h>
#include <stdarg.h>

bool sxlogger::initialize() {
	return true;
}

void sxlogger::shutdown() {
}

void _log_output(log_level level, const char *message, ...) {
	const char *level_string[6] = { "[FATAL]: ", "[ERROR]: ", "[WARN]: ", "[INFO]: ", "[DEBUG]: ", "[TRACE]: " };
	bool is_error = level < LOG_LEVEL_WARN;

	va_list arg_ptr;
	char text[1024];

	va_start(arg_ptr, message);
	vsnprintf(text, sizeof(text), message, arg_ptr);
	va_end(arg_ptr);

	char out_message[1024];
	sprintf_s(out_message, "%s%s\n", level_string[level], text);

	//fputs(out_message, stdout);
	if (is_error) platform::console_write_error(out_message, level);
	else platform::console_write(out_message, level);
}