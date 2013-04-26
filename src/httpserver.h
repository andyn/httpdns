#ifndef HTTPSERVER_H_
#define HTTPSERVER_H_

/**
 * Run a listening HTTP server on given port. Exit on a signal.
 * @param port Service name or numeric port to listen on.
 * @return 0 if server was run (and shutdown) correctly. -1 if the server could not
 * bind to given port for one reason or another.
 */
int httpserver_run(const char *port);

#endif
