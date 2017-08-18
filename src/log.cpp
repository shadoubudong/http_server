#include "log.h"
#include <stdio.h>
#include <stdarg.h>


void log::logFun(const char* file, int line, const int level, const char* format, ...)
{
    char buf[1024] = {0};
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    va_end(ap);
    printf("%s[%s:%d] %s\n", get_level(level).c_str(), file, line, buf);
}

std::string log::get_level(int level)
{
    switch(level)
    {
        case 0:
            return std::string(PRINT_COLOR_GREED "[NORMAL]" PRINT_COLOR_NONE);
        case 1:
            return std::string(PRINT_COLOR_BLUE "[DEBUG]" PRINT_COLOR_NONE);
        case 2:
            return std::string(PRINT_COLOR_YELLOW "[WARN]" PRINT_COLOR_NONE);
        case 3:
            return std::string(PRINT_COLOR_RED "[ERROR]" PRINT_COLOR_NONE);
        default:
            return std::string(PRINT_COLOR_RED "[UNKOWN]" PRINT_COLOR_NONE);
    }
}

void LogAssertError(const char* str)
{
    ERROR("ASSERT(%s)", str);
}


