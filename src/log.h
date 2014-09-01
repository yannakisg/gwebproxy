#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>

#define LOG_INFO(...) ((void) fprintf(stdout, __VA_ARGS__))
#define LOG_ERROR(...) ((void) fprintf(stderr, __VA_ARGS__))

#endif
