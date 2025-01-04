#include "ex_logger.h"

#include <stdio.h>
#include <stdarg.h>

void
_logger_output(EX_LOG_LEVEL level, const char *message, ...) {
    const char *levels[6] {
        "[FATAL]: ",
        "[ERROR]: ",
        "[WARN]: ",
        "[INFO]: ",
        "[DEBUG]: ",
        "[TRACE]: ",
    };

    static char buffer[1024];
    va_list arg_list;
    va_start(arg_list, message);
    vsprintf_s(buffer, sizeof(buffer), message, arg_list);
    va_end(arg_list);

    static char buffer1[1024];
    sprintf_s(buffer1, "%s%s\n", levels[level], buffer);
    printf(buffer1);
}
