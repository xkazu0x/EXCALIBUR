#ifndef EXCALIBUR_LOG_H
#define EXCALIBUR_LOG_H

#define LOG_ENABLE_INFO 1

#if EXCALIBUR_DEBUG
#define LOG_ENABLE_DEBUG 1
#endif

enum Log_Level {
    LogLevel_Fatal,
    LogLevel_Error,
    LogLevel_Info,
    LogLevel_Debug,
    LogLevel_Count,
};

internal void log_level(Log_Level level, char *message, ...);

#define log_fatal(message, ...) log_level(LogLevel_Fatal, message, ##__VA_ARGS__);
#define log_error(message, ...) log_level(LogLevel_Error, message, ##__VA_ARGS__);

#if LOG_ENABLE_INFO == 1
#define log_info(message, ...) log_level(LogLevel_Info, message, ##__VA_ARGS__);
#else
#define log_info(message, ...)
#endif

#if LOG_ENABLE_DEBUG == 1
#define log_debug(message, ...) log_level(LogLevel_Debug, message, ##__VA_ARGS__);
#else
#define log_debug(message, ...)
#endif

#endif // EXCALIBUR_LOG_H
