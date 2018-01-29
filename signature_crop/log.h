#ifndef LOG_H
#define LOG_H

enum {
    LOG_INFO = 0,
    LOG_WARN,
    LOG_ERROR,
    LOG_DEBUG
} LOG_LEVELS;

extern int LOG_LEVEL;

void log_info(const char* message, ...);
void log_warn(const char* message, ...);
void log_error(const char* message, ...);
void log_debug(const char* message, ...);

#endif
