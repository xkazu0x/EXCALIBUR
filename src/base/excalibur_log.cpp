#include <stdio.h>
#include <stdarg.h>

internal void
log_level(Log_Level level, char *message, ...) {
    char *levels[LogLevel_Count] = {
        "[FATAL]: ",
        "[ERROR]: ",
        "[INFO]: ",
        "[DEBUG]: ",
    };
    
    char message_buffer[1024];
    char out_message_buffer[2048];
    
    va_list arg_list;
    va_start(arg_list, message);
    vsprintf_s(message_buffer, sizeof(message_buffer), message, arg_list);
    va_end(arg_list);

    sprintf(out_message_buffer, "%s%s\n", levels[level], message_buffer);
    printf(out_message_buffer);
}
