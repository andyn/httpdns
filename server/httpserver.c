#include "common.h"

#include <errno.h>
#include <dirent.h>
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

static void httpserver_reply_bad_request(int fd) {
	String *reply = string_new("");
	string_append_c(reply, "HTTP/1.1 400 Bad Request\r\n");
	string_append_c(reply, "Iam: anilakar\r\n");
	string_append_c(reply, "Connection: close\r\n\r\n");
	string_append_c(reply, "400 Bad Request");
	socket_write(fd, reply->c_str, reply->size);
	string_delete(reply);
}

static void httpserver_reply_forbidden(int fd) {
	String *reply = string_new("");
	string_append_c(reply, "HTTP/1.1 403 Forbidden\r\n");
	string_append_c(reply, "Iam: anilakar\r\n");
	string_append_c(reply, "Connection: close\r\n\r\n");
	string_append_c(reply, "403 Forbidden");
	socket_write(fd, reply->c_str, reply->size);
	string_delete(reply);
}

static void httpserver_reply_not_found(int fd) {
	String *reply = string_new("");
	string_append_c(reply, "HTTP/1.1 404 Not Found\r\n");
	string_append_c(reply, "Iam: anilakar\r\n");
	string_append_c(reply, "Connection: close\r\n\r\n");
	string_append_c(reply, "404 Not Found");
	socket_write(fd, reply->c_str, reply->size);
	string_delete(reply);
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

static void httpserver_reply_internal_server_error(int fd) {
	String *reply = string_new("");
	string_append_c(reply, "HTTP/1.1 503 Internal Server Error\r\n");
	string_append_c(reply, "Iam: anilakar\r\n");
	string_append_c(reply, "Connection: close\r\n\r\n");
	string_append_c(reply, "503 Internal Server Error");
	socket_write(fd, reply->c_str, reply->size);
	string_delete(reply);
}

static void httpserver_reply_get_file(int network_socket, int local_file) {
	// Find out file size and rewind
	size_t file_size = lseek(local_file, 0, SEEK_END);
	char content_length[16];
	snprintf(content_length, sizeof(content_length), "%zu", file_size);
	lseek(local_file, 0, SEEK_SET);

	// Generate and send header
	String *reply = string_new("");
	string_append_c(reply, "HTTP/1.1 200 OK\r\n");
	string_append_c(reply, "Iam: anilakar\r\n");
	string_append_c(reply, "Content-Type: text/plain\r\n");
	string_append_c(reply, "Content-Length: ");
	string_append_c(reply, content_length);
	string_append_c(reply, "\r\n");
	string_append_c(reply, "Connection: close\r\n\r\n");
	socket_write(network_socket, reply->c_str, reply->size);
	string_delete(reply);

	// Send content/payload
	size_t bytes_remaining = file_size;
	while (bytes_remaining > 0) {
		char buffer[8192];
		ssize_t bytes_read = socket_read(local_file, buffer, sizeof buffer);
		if (bytes_read < 0) {
			break; // Abort, there's really nothing helpful to do after the headers are on the wire
		}
		ssize_t bytes_written = socket_write(network_socket, buffer, bytes_read);
		if (bytes_written < 0) {
			break; // As above
		}
		bytes_remaining -= bytes_written;
	}
}

static inline String *httpserver_get_directory_contents_dir(DIR *directory) {
	struct dirent entry;
	struct dirent *iter;
	String *s = string_new("");
	if (s) {
		while (readdir_r(directory, &entry, &iter), iter) {
			if (!string_append_c(s, entry.d_name) || !string_append_c(s, "\r\n")) {
				string_delete(s); 
				s = NULL;
				break;
			}
		}
	}
	if (!s) {
		VERBOSE("Error allocating memory");
	}
	return s;
}

static inline String *httpserver_get_directory_contents(int directory) {
	String *s = NULL;
	DIR *dir = fdopendir(directory);
	if (dir) {
		s = httpserver_get_directory_contents_dir(dir);
	}
	closedir(dir);
	return s;
}

// Print directory contents
static void httpserver_reply_get_directory(int network_socket, int directory) {

	String *directory_contents = httpserver_get_directory_contents(directory);
	if (directory_contents) {
		char content_length[16];
		snprintf(content_length, sizeof(content_length), "%zu", directory_contents->size);

		String *reply = string_new("");
		string_append_c(reply, "HTTP/1.1 200 OK\r\n");
		string_append_c(reply, "Iam: anilakar\r\n");
		string_append_c(reply, "Content-Type: text/plain\r\n");
		string_append_c(reply, "Content-Length: ");
		string_append_c(reply, content_length);
		string_append_c(reply, "\r\n");
		string_append_c(reply, "Connection: close\r\n\r\n");
		string_append(reply, directory_contents);
		socket_write(network_socket, reply->c_str, reply->size);
		string_delete(reply);
	}
	else {
		httpserver_reply_internal_server_error(network_socket);
	}
	string_delete(directory_contents);
}

static void httpserver_handle_get(int fd, const char *path) {
	int local_file = -1;

	// This allows getting directory contents of document root.
	if (strcmp(path, "/") == 0) {
		path = "/.";
	}
	if (strlen(path) > 1 && (local_file = open(path + 1, O_RDONLY)) != -1) {
		if (is_regular_file(local_file)) {
			// Serve file contents
			httpserver_reply_get_file(fd, local_file);
		} else if (is_directory(local_file)) {
			// Serve directory listing
			httpserver_reply_get_directory(fd, local_file);
		} else {
			// Other than regular files or directories are not served
			httpserver_reply_forbidden(fd);
		}
	} else {
		httpserver_reply_not_found(fd);
	}

	socket_close(local_file);
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
	// Read in the header...
	ssize_t bytes_read = socket_read(thread_data->client_fd, buffer, buffer_size);
	// ... and split it to lines. The first line will be the
	// request line; subsequent lines will be the rest of the header. The last field
	// will hold the start of the payload; this needs to be sent to the file before
	String *incoming_data = string_new_from_range(buffer, buffer + bytes_read);
	String **header = string_split(incoming_data, "\r\n");
	string_delete(incoming_data);
	// Calculate number of lines in header
	size_t header_size = 0;
	for (; header[header_size]; ++header_size);

	if (header_size > 2) {
		// At least request \r\n \r\n payload
		char action[10], path[512], version[10];
		sscanf(header[0]->c_str, "%9s %511s %9s", action, path, version);
		VERBOSE("HTTP Action: %s %s %s", action, path, version);

		if (0 == strcmp(action, "GET")) {
			httpserver_handle_get(thread_data->client_fd, path);
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

	// Ignore some common signals
	struct sigaction handler;
	handler.sa_handler = signal_handler;
	handler.sa_flags = 0;
	sigemptyset(&handler.sa_mask);
	if (sigaction(SIGINT, &handler, NULL) ||
		sigaction(SIGTERM, &handler, NULL) ||
		sigaction(SIGPIPE, &handler, NULL)) {
		VERBOSE("Error setting signal handlers");
		return -1;
	}

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
			if (caught_signal) {
				VERBOSE("Caught signal, shutting down.");
				shutdown(listen_socket, SHUT_RDWR);
				running = false;
				continue;
			}

			int incoming_socket = accept(listen_socket, NULL, NULL);
			if (incoming_socket != -1) {
				VERBOSE("Incoming connection.");
				httpserver_handle_connection(incoming_socket);
			} 
			else if (errno == EINTR) {
				// Signal? Try again and handle it
				continue;
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
