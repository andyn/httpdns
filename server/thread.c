#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

int thread_create_detached(void *(*thread_main)(void *), void *arg) {
	// Spawning a NULL is not an error (== want a no-op)
	if (!thread_main) {
		return 0;
	}
	pthread_attr_t attributes;
	pthread_attr_init(&attributes);
	pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);

	pthread_t thread;
	int r = pthread_create(&thread, &attributes, thread_main, arg);

	if (!r) {
		return 0;
	}
	else {
		// Don't leak the error code out -- a failure is a failure
		// that should be contained.
		VERBOSE("Error spawning thread: %s", strerror(r));
		return -1;
	}
}