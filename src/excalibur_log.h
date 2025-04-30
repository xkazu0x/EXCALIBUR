#ifndef EXCALIBUR_LOG_H
#define EXCALIBUR_LOG_H

#define LOG_ENABLE_WARN 1
#define LOG_ENABLE_INFO 1

#if EXCALIBUR_DEBUG
#define LOG_ENABLE_DEBUG 1
#define LOG_ENABLE_TRACE 1
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

internal void _log(log_level_t level, char *message, ...);

#define log_fatal(message, ...) _log(LOG_LEVEL_FATAL, message, ##__VA_ARGS__);
#define log_error(message, ...) _log(LOG_LEVEL_ERROR, message, ##__VA_ARGS__);

#if LOG_ENABLE_WARN == 1
#define log_warn(message, ...) _log(LOG_LEVEL_WARN, message, ##__VA_ARGS__);
#else
#define log_warn(message, ...)
#endif

#if LOG_ENABLE_INFO == 1
#define log_info(message, ...) _log(LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#else
#define log_info(message, ...)
#endif

#if LOG_ENABLE_DEBUG == 1
#define log_debug(message, ...) _log(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#else
#define log_debug(message, ...)
#endif

#if LOG_ENABLE_TRACE == 1
#define log_trace(message, ...) _log(LOG_LEVEL_TRACE, message, ##__VA_ARGS__);
#else
#define log_trace(message, ...)
#endif

#endif // EXCALIBUR_LOG_H
