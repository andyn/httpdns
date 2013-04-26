// HttpDnsD
// A simple HTTP daemon supporting GET and PUT

#include "common.h"

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "httpserver.h"
#include "util.h"

/**
 * Daemonize the process.
 * Exits on failure.
 */
static void daemonize(void) {
	// If already a daemon, do nothing.
	if (getppid() == 1) {
		return;
	}

	// Fork to background
	pid_t pid = fork();
	if (pid < 0) {
		VERBOSE("Error daemonizing: could not fork the first time.");
		exit(EXIT_FAILURE);
	// Parent should quit now.
	} else if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	// Get a new session so as not to be dependant on shell.
	pid_t sid = setsid();
	if (sid < 0) {
		VERBOSE("Error daemonizing: could not create new session.");
		exit(EXIT_FAILURE);
	}

	// Fork again to prevent a terminal from attaching to the daemon.
	pid = fork();
	if (pid < 0) {
		VERBOSE("Error daemonizing: could not fork the second time.");
		exit(EXIT_FAILURE);
	// Parent should quit now.
	} else if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	// Allow creating files with all perms.
	umask(0);

	// Chdir to root so as not to keep open paths.
	/*
	if (chdir("/") < 0) {
		VERBOSE("Error daemonizing: could not chdir to /.");
		exit(EXIT_FAILURE);
	}
	*/

	// Redirect standard streams
	freopen("/dev/null", "r", stdin);
	freopen("/dev/null", "w", stdout);
	freopen("/dev/null", "w", stderr);
}

static void print_usage_and_exit(const char *program_name) {
	printf(
		"Usage: %s [-f] [-v] PORT\n"
		"    -f    Stay on foreground\n"
		"    -v    Print verbose output\n"
		"    PORT  Port or service name to listen on\n", program_name);
	exit(0);
}

int main(int argc, char *argv[]) {
	struct {
		bool daemonize;
		bool verbose;
		char *port;
	} options = {
		.daemonize = true,
		.verbose = false,
		.port = NULL,
	};

	// Accept -v for verbose mode. Disable getopt internal warnings.
	extern int opterr, optopt, optind;
	opterr = 0;
	int optchar;
	while (-1 != (optchar = getopt(argc, argv, "fv"))) {
		if (optchar == 'f'){
			options.daemonize = false;
		}
		else if (optchar == 'v') {
			options.verbose = true;
		} else {
			printf("Unknown option '%c'\n", optopt);
			print_usage_and_exit(argv[0]);
		}
	}
	// Accepts just one parameter - the port
	if (argc - optind != 1) {
		print_usage_and_exit(argv[0]);
	}
	options.port = argv[optind + 0];

	// Go to background
	if (options.daemonize) {
		daemonize();
	}

	// If not verbose, close stderr
	if (!options.verbose) {
		freopen("/dev/null", "w", stderr);
	}

	httpserver_run(options.port);

	return EXIT_SUCCESS;
}

