#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <stdarg.h>
#include <stdio.h>

/** Logging levels of severity. */
typedef enum {
    LOG_ERROR, /**< Errors. */
    LOG_WARN,  /**< Warnings. */
    LOG_INFO,  /**< Information. */
    LOG_DEBUG, /**< Debugging. */
} log_level_e;

void _log_print(FILE *stream, log_level_e lvl, const char *file, const char *func, int line, const char *fmt_string,
                ...);

#define log_print(stream, lvl, fmt_string, ...)                                                                        \
    _log_print((stream), (lvl), __FILE__, __FUNCTION__, __LINE__, (fmt_string), ##__VA_ARGS__)

#endif // _LOGGING_H_
