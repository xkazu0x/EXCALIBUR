#ifndef EXCALIBUR_LOG_H
#define EXCALIBUR_LOG_H

#define LOG_ENABLE_WARN 1
#define LOG_ENABLE_INFO 1

#if EXCALIBUR_DEBUG
#define LOG_ENABLE_DEBUG 1
#define LOG_ENABLE_TRACE 1
#endif

enum Log_Level {
    LogLevel_Fatal,
    LogLevel_Error,
    LogLevel_Warn,
    LogLevel_Info,
    LogLevel_Debug,
    LogLevel_Trace,
    LogLevel_Count,
};

internal void _log(Log_Level level, char *message, ...);

#define log_fatal(message, ...) _log(LogLevel_Fatal, message, ##__VA_ARGS__);
#define log_error(message, ...) _log(LogLevel_Error, message, ##__VA_ARGS__);

#if LOG_ENABLE_WARN == 1
#define log_warn(message, ...) _log(LogLevel_Warn, message, ##__VA_ARGS__);
#else
#define log_warn(message, ...)
#endif

#if LOG_ENABLE_INFO == 1
#define log_info(message, ...) _log(LogLevel_Info, message, ##__VA_ARGS__);
#else
#define log_info(message, ...)
#endif

#if LOG_ENABLE_DEBUG == 1
#define log_debug(message, ...) _log(LogLevel_Debug, message, ##__VA_ARGS__);
#else
#define log_debug(message, ...)
#endif

#if LOG_ENABLE_TRACE == 1
#define log_trace(message, ...) _log(LogLevel_Trace, message, ##__VA_ARGS__);
#else
#define log_trace(message, ...)
#endif

#endif // EXCALIBUR_LOG_H
