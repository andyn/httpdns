#ifndef UTIL_H_
#define UTIL_H_

#include "common.h"

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "string.h"

/**
 * For debugging purposes. Use like printf.
 * All caps for legacy reasons
 */
static inline void VERBOSE(const char *format, ...) {
	// Use an internal buffer to allow atomic writes to stderr

	// Timestamp
	String *s = string_new("[");
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts); \
	char ts_string[32]; \
	snprintf(ts_string, sizeof ts_string, "%ld.%ld", (long) ts.tv_sec, (long) ts.tv_nsec);
	string_append_c(s, ts_string);
	string_append_c(s, "] ");

	// Message
	char message[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(message, sizeof(message), format, args);
	va_end(args);
	string_append_c(s, message);

	// Newline
	string_append_c(s, "\n");

	// One hopefully atomic write. Retry in any case.
	size_t written = 0;
	size_t left = s->size;
	while (left > 0) {
		ssize_t written_last = write(STDERR_FILENO, s->c_str + written, left);
		if (written_last == -1 && errno != EINTR) {
			break;
		}
		written += written_last;
		left -= written_last;
	}
	string_delete(s);
}

// Return whether a file is directory
static inline bool is_directory(int fd) {
	bool is_directory = false;

	struct stat file_stats;
	if (fstat(fd, &file_stats) != -1) {
		if (S_ISDIR(file_stats.st_mode)) {
			is_directory = true;
		}
	}

	return is_directory;
}

// Return whether a file is a regular file
static inline bool is_regular_file(int fd) {
	bool is_regular_file = false;

	struct stat file_stats;
	if (fstat(fd, &file_stats) != -1) {
		if (S_ISREG(file_stats.st_mode)) {
			is_regular_file = true;
		}
	}

	return is_regular_file;
}

#endif