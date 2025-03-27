#ifndef EXCALIBUR_LOGGER_H
#define EXCALIBUR_LOGGER_H

#define EX_ENABLE_WARN 1
#define EX_ENABLE_INFO 1

#if EXCALIBUR_DEBUG
#define EX_ENABLE_DEBUG 1
#define EX_ENABLE_TRACE 1
#endif

enum EX_LOG_LEVEL {
    EX_LOG_LEVEL_FATAL,
    EX_LOG_LEVEL_ERROR,
    EX_LOG_LEVEL_WARN,
    EX_LOG_LEVEL_INFO,
    EX_LOG_LEVEL_DEBUG,
    EX_LOG_LEVEL_TRACE,
    EX_LOG_LEVEL_MAX,
};

internal void _logger_output(EX_LOG_LEVEL level, char *message, ...);

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

#endif // EXCALIBUR_LOGGER_H
