#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "http.h"
#include "httpserver.h"
#include "socket.h"
#include "string.h"
#include "thread.h"
#include "util.h"

#define REGISTRATION_URL "http://nwprog1.netlab.hut.fi:3000/servers-anilakar.txt"

volatile int caught_signal = 0;

void signal_handler(int signal) {
	(void) signal;
	caught_signal = 1;
}

// Thread parameters
typedef struct {
	int client_fd;
} ThreadData;

static void httpserver_reply_get(int fd, const char *path) {
	int local_file = -1;

	if (strlen(path) > 1 &&
		-1 != (local_file = open(path + 1, O_RDONLY))) {

		// Find out file size and rewind
		size_t file_size = lseek(local_file, 0, SEEK_END);
		char content_length[16];
		snprintf(content_length, sizeof content_length, "%zu", file_size);
		lseek(local_file, 0, SEEK_SET);

		// Generate and send header
		String *reply = string_new("");
		string_append_c(reply, "HTTP/1.1 200 OK\r\n");
		string_append_c(reply, "Iam: anilakar\r\n");
		string_append_c(reply, "Content-Length: ");
		string_append_c(reply, content_length);
		string_append_c(reply, "\r\n");
		string_append_c(reply, "Connection: close\r\n\r\n");
		socket_write(fd, reply->c_str, reply->size);
		string_delete(reply);

		// Send content/payload
		size_t bytes_remaining = file_size;
		while (bytes_remaining > 0) {
			char buffer[8192];
			ssize_t bytes_read = socket_read(local_file, buffer, sizeof buffer);
			if (bytes_read < 0) {
				break; // Abort, there's really nothing helpful to do after the headers are on the wire
			}
			ssize_t bytes_written = socket_write(fd, buffer, bytes_read);
			if (bytes_written < 0) {
				break; // As above
			}
			bytes_remaining -= bytes_written;
		}

	} else {
		// Four-oh-four
		const char *reply_not_found = "404 Not Found";
		char content_length[16];
		snprintf(content_length, sizeof content_length, "%zu", strlen(reply_not_found));

		String *reply = string_new("");
		string_append_c(reply, "HTTP/1.1 200 OK\r\n");
		string_append_c(reply, "Iam: anilakar\r\n");
		string_append_c(reply, "Content-Length: ");
		string_append_c(reply, content_length);
		string_append_c(reply, "\r\n");
		string_append_c(reply, "Connection: close\r\n\r\n");
		string_append_c(reply, reply_not_found);
		socket_write(fd, reply->c_str, reply->size);
		string_delete(reply);
	}

	socket_close(local_file);
}

static void httpserver_reply_method_not_allowed(int fd) {
	String *reply = string_new("");
	string_append_c(reply, "HTTP/1.1 405 Method Not Allowed\r\n");
	string_append_c(reply, "Iam: anilakar\r\n");
	string_append_c(reply, "Connection: close\r\n\r\n");
	string_append_c(reply, "405 Method Not Allowed");
	socket_write(fd, reply->c_str, reply->size);
	string_delete(reply);
}

static void httpserver_reply_bad_request(int fd) {
	String *reply = string_new("");
	string_append_c(reply, "HTTP/1.1 400 Bad Request\r\n");
	string_append_c(reply, "Iam: anilakar\r\n");
	string_append_c(reply, "Connection: close\r\n\r\n");
	string_append_c(reply, "400 Bad Request");
	socket_write(fd, reply->c_str, reply->size);
	string_delete(reply);
}

/**
 * Worker thread. Handles incoming connections. Assumes ownership of argument struct.
 */
static void *httpserver_worker_thread(void *args) {
	ThreadData *thread_data = args;

	VERBOSE("Worker thread started.");

	// Avoid wasting stack space on individual threads -- use the heap instead
	const int buffer_size = 8192;
	char *buffer = malloc(sizeof(*buffer) * buffer_size);
	// Read in the header and split it to lines. The first line will be the
	// request line; subsequent lines will be the rest of the header. The last field
	// will hold the start of the payload; this needs to be sent to the file before
	ssize_t bytes_read = socket_read(thread_data->client_fd, buffer, buffer_size);
	String *incoming_data = string_new_from_range(buffer, buffer + bytes_read);
	String **header = string_split(incoming_data, "\r\n");
	string_delete(incoming_data);
	size_t header_size = 0;
	for (; header[header_size]; ++header_size);
	if (header_size > 2) {
		// At least request \r\n \r\n payload
		char action[10], path[512], version[10];
		sscanf(header[0]->c_str, "%9s %511s %9s", action, path, version);
		VERBOSE("HTTP Action: %s %s %s", action, path, version);

		if (0 == strcmp(action, "GET")) {
			// httpserver_reply_method_not_allowed(thread_data->client_fd);
			httpserver_reply_get(thread_data->client_fd, path);
		}
		else {
			httpserver_reply_method_not_allowed(thread_data->client_fd);
		}
	}
	else {
		httpserver_reply_bad_request(thread_data->client_fd);
	}
	string_delete_array(header);

	socket_close(thread_data->client_fd);
	free(buffer);
	free(thread_data);

	VERBOSE("Worker thread ended");
	return NULL;
}

/**
 * Handle an incoming connection. Assumes ownership of the socket.
 * @param connected_fd Socket with an incoming connection. Ownership is assumed
 */
static void httpserver_handle_connection(int connected_fd) {
	ThreadData *thread_data = malloc(sizeof(*thread_data));
	thread_data->client_fd = connected_fd;
	if (0 != thread_create_detached(httpserver_worker_thread, thread_data)) {
		VERBOSE("Could not spawn a worker thread");
	}
}

/**
 * Register to the central bookkeeping server
 */
static void httpserver_register(const char *port) {
	VERBOSE("Registering to central server...");

	char registration_string[64];
	snprintf(registration_string, sizeof registration_string,
		"nwprog1.netlab.hut.fi:%s\n", port);

	for (int max_retries = 3; max_retries > 0; --max_retries) {
		int status_code = http_put_buf(REGISTRATION_URL, registration_string, strlen(registration_string));
		if (200 <= status_code && status_code < 300) {
			VERBOSE("Registered to central server.");
			break;
		} else if (max_retries == 1) {
			VERBOSE("Could not register to central server. Starting up regardless.");
		} else {
			VERBOSE("Could not register to central server. Retrying...");
			sleep(5);
		}
	}
}

/**
 * Deregister from the central bookkeeping servert
 */
static void httpserver_deregister(void) {
	VERBOSE("Deregistering from central server...");
	for (int max_retries = 3; max_retries > 0; --max_retries) {
		int status_code = http_put_buf(REGISTRATION_URL, "", 0);
		if (200 <= status_code  && status_code < 300) {
			VERBOSE("Deregistered from central server.");
			break;
		}
		else if (max_retries == 1) {
			VERBOSE("Could not deregister from central server, giving up.");
		}
		else {
			VERBOSE("Could not deregister from central server, retrying...");
			sleep(5);
		}
	}
}

int httpserver_run(const char *port) {

	// Terminal and default termination signals are handled
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	// Bind to a port
	VERBOSE("Opening a listening socket on localhost:tcp/%s.", port);
	int listen_socket = socket_tcp_listen(NULL, port);
	if (listen_socket >= 0) {
		VERBOSE("Listening socket open.");

		// Register to central server
		httpserver_register(port);

		// Listen for incoming connections and pass them to the handler
		bool running = true;
		while (running) {
			int incoming_socket = accept(listen_socket, NULL, NULL);
			if (incoming_socket >= 0) {
				VERBOSE("Incoming connection.");
				httpserver_handle_connection(incoming_socket);
			} 
			else if (errno == EINTR || caught_signal) {
				VERBOSE("Caught signal, shutting down.");
				shutdown(listen_socket, SHUT_RDWR);
				running = false;
			}
			else {
				VERBOSE("Error accepting connection: %s", strerror(errno));
			}
		}

		// Deregister from central server
		httpserver_deregister();

	} else {
		VERBOSE("Error opening listening socket: %s", strerror(errno));
	}
	socket_close(listen_socket);

	return 0;
}
