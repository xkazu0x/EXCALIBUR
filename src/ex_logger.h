#pragma once

#include "ex_base.h"

#define EX_ENABLE_WARN 1
#define EX_ENABLE_INFO 1

#ifdef EX_DEBUG_MODE
#define EX_ENABLE_DEBUG 1
#define EX_ENABLE_TRACE 1
#endif

enum EX_LOG_LEVEL {
    EX_LOG_LEVEL_FATAL = 0,
    EX_LOG_LEVEL_ERROR = 1,
    EX_LOG_LEVEL_WARN = 2,
    EX_LOG_LEVEL_INFO = 3,
    EX_LOG_LEVEL_DEBUG = 4,
    EX_LOG_LEVEL_TRACE = 5,
};

void _logger_output(EX_LOG_LEVEL level, const char *message, ...);

#define EXFATAL(message, ...) _logger_output(EX_LOG_LEVEL_FATAL, message, ##__VA_ARGS__);
#define EXERROR(message, ...) _logger_output(EX_LOG_LEVEL_ERROR, message, ##__VA_ARGS__);
#if EX_ENABLE_WARN == 1
#define EXWARN(message, ...) _logger_output(EX_LOG_LEVEL_WARN, message, ##__VA_ARGS__);
#else
#define EXWARN(message, ...)
#endif
#if EX_ENABLE_INFO == 1
#define EXINFO(message, ...) _logger_output(EX_LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#else
#define EXINFO(message, ...)
#endif
#if EX_ENABLE_DEBUG == 1
#define EXDEBUG(message, ...) _logger_output(EX_LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#else
#define EXDEBUG(message, ...)
#endif
#if EX_ENABLE_TRACE == 1
#define EXTRACE(message, ...) _logger_output(EX_LOG_LEVEL_TRACE, message, ##__VA_ARGS__);
#else
#define EXTRACE(message, ...)
#endif
