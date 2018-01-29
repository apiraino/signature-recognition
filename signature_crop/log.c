#include "common.h"
#include "log.h"

int LOG_LEVEL;

void log_format(const char* tag, const char* message, va_list args)
{
    /* time_t now; */
    /* time(&now); */
    /* char *date = ctime(&now); date[strlen(date) - 1] = '\0'; */
    printf("[%s] ", tag);
    vprintf(message, args);
}

void log_error(const char* message, ...)
{
    va_list args;
    va_start(args, message);
    log_format("ERROR", message, args);
    va_end(args);
}

void log_info(const char* message, ...)
{
    va_list args;
    va_start(args, message);
    log_format("INFO", message, args);
    va_end(args);
}

void log_warn(const char* message, ...)
{
    va_list args;
    va_start(args, message);
    log_format("WARN", message, args);
    va_end(args);
}

void log_debug(const char* message, ...)
{
    va_list args;
    va_start(args, message);
    log_format("DEBUG", message, args);
    va_end(args);
}
