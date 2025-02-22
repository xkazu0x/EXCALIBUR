#include <stdio.h>
#include <stdarg.h>

internal void
_logger_output(EX_LOG_LEVEL level, char *message, ...) {
    const char *levels[EX_LOG_LEVEL_MAX] {
        "[FATAL]: ",
        "[ERROR]: ",
        "[WARN]: ",
        "[INFO]: ",
        "[DEBUG]: ",
        "[TRACE]: ",
    };

    local char message_buffer[1024];
    va_list arg_list;
    va_start(arg_list, message);
    vsprintf_s(message_buffer, sizeof(message_buffer), message, arg_list);
    va_end(arg_list);

    local char out_message_buffer[2048];
    sprintf(out_message_buffer, "%s%s\n", levels[level], message_buffer);
    printf(out_message_buffer);
}
