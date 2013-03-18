#ifndef THREAD_H_
#define THREAD_H_

/**
 * Create a detached thread
 * @param thread_main The function to run in thread. NULL for no-op.
 * @param arg Argument to pass to the thread.
 * @return 0 if the thread was created. -1 if creating the thread failed.
 */
int thread_create_detached(void *(*thread_main)(void *), void *arg);

#endif
