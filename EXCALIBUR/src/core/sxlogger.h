#pragma once

#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1

#if defined(_DEBUG)
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 1
#else
#define LOG_DEBUG_ENABLED 0
#define LOG_TRACE_ENABLED 0
#endif

enum log_level {
	LOG_LEVEL_FATAL = 0,
	LOG_LEVEL_ERROR = 1,
	LOG_LEVEL_WARN = 2,
	LOG_LEVEL_INFO = 3,
	LOG_LEVEL_DEBUG = 4,
	LOG_LEVEL_TRACE = 5
};

namespace sxlogger {
	bool initialize();
	void shutdown();
}

void _log_output(log_level level, const char *message, ...);

#define SXFATAL(message, ...) _log_output(LOG_LEVEL_FATAL, message, ##__VA_ARGS__);

#ifndef SXERROR
#define SXERROR(message, ...) _log_output(LOG_LEVEL_ERROR, message, ##__VA_ARGS__);
#endif

#if LOG_WARN_ENABLED == 1
#define SXWARN(message, ...) _log_output(LOG_LEVEL_WARN, message, ##__VA_ARGS__);
#else
#define SXWARN(message, ...)
#endif

#if LOG_INFO_ENABLED == 1
#define SXINFO(message, ...) _log_output(LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#else
#define SXINFO(message, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
#define SXDEBUG(message, ...) _log_output(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#else
#define SXDEBUG(message, ...)
#endif

#if LOG_TRACE_ENABLED == 1
#define SXTRACE(message, ...) _log_output(LOG_LEVEL_TRACE, message, ##__VA_ARGS__);
#else
#define SXTRACE(message, ...)
#endif