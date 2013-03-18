#ifndef UTIL_H_
#define UTIL_H_

#include <stdio.h>

/**
 * For debugging purposes. Use like printf.
 */
#define VERBOSE(...) \
	do { \
		fprintf(stderr, __VA_ARGS__); \
		fputc('\n', stderr); \
	} while (0)

#endif
