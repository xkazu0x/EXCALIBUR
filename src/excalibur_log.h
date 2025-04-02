#ifndef EXCALIBUR_LOG_H
#define EXCALIBUR_LOG_H

#define EX_ENABLE_WARN 1
#define EX_ENABLE_INFO 1

#if EXCALIBUR_DEBUG
#define EX_ENABLE_DEBUG 1
#define EX_ENABLE_TRACE 1
#endif

enum log_level_t {
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_TRACE,
    LOG_LEVEL_MAX,
};

internal void log_output(log_level_t level, char *message, ...);

#define EXFATAL(message, ...) log_output(LOG_LEVEL_FATAL, message, ##__VA_ARGS__);
#define EXERROR(message, ...) log_output(LOG_LEVEL_ERROR, message, ##__VA_ARGS__);

#if EX_ENABLE_WARN == 1
#define EXWARN(message, ...) log_output(LOG_LEVEL_WARN, message, ##__VA_ARGS__);
#else
#define EXWARN(message, ...)
#endif

#if EX_ENABLE_INFO == 1
#define EXINFO(message, ...) log_output(LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#else
#define EXINFO(message, ...)
#endif

#if EX_ENABLE_DEBUG == 1
#define EXDEBUG(message, ...) log_output(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#else
#define EXDEBUG(message, ...)
#endif

#if EX_ENABLE_TRACE == 1
#define EXTRACE(message, ...) log_output(LOG_LEVEL_TRACE, message, ##__VA_ARGS__);
#else
#define EXTRACE(message, ...)
#endif

#endif // EXCALIBUR_LOG_H
