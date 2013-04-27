#include "common.h"

#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "dns.h"
#include "http.h"
#include "httpserver.h"
#include "socket.h"
#include "string.h"
#include "thread.h"
#include "util.h"

#define CRLF "\r\n"
#define I_AM "anilakar"
#define DEFAULT_DNS_SERVER "8.8.8.8" // Google's open DNS resolver
#define REGISTRATION_URL "http://nwprog1.netlab.hut.fi:3000/servers-" I_AM ".txt"

volatile int caught_signal = 0;

void signal_handler(int signal) {
	(void) signal;
	caught_signal = 1;
}

// Thread parameters
typedef struct {
	int client_fd;
} ThreadData;

static void httpserver_reply_full(int fd, const char *code_and_status) {
	char content_length[16];
	snprintf(content_length, sizeof(content_length), "%zu", strlen(code_and_status));

	String *reply = string_new("");
	string_append_c(reply, "HTTP/1.1 ");
	string_append_c(reply, code_and_status);
	string_append_c(reply, CRLF);
	string_append_c(reply, "Iam: " I_AM CRLF);
	string_append_c(reply, "Content-Type: text/plain" CRLF);
	string_append_c(reply, "Content-Length: ");
	string_append_c(reply, content_length);
	string_append_c(reply, CRLF);
	string_append_c(reply, "Connection: close" CRLF CRLF);
	string_append_c(reply, code_and_status);
	socket_write(fd, reply->c_str, reply->size);
	string_delete(reply);
}

static void httpserver_reply_header(int fd, const char *code_and_status) {
	String *reply = string_new("");
	string_append_c(reply, "HTTP/1.1 ");
	string_append_c(reply, code_and_status);
	string_append_c(reply, CRLF);
	string_append_c(reply, "Iam: " I_AM CRLF);
	string_append_c(reply, "Connection: close" CRLF CRLF);
	socket_write(fd, reply->c_str, reply->size);
	string_delete(reply);
}

static void httpserver_reply_ok(int fd, String **payload_lines) {
	char content_length[16];
	size_t content_length_bytes = 0;
	for (String **it = payload_lines; *it; ++it) {
		content_length_bytes += (*it)->size + 2; // + 2 CRLF
	}
	snprintf(content_length, sizeof content_length, "%zu", content_length_bytes);

	String *reply = string_new("");
	string_append_c(reply, "HTTP/1.1 200 OK" CRLF);
	string_append_c(reply, "Iam: " I_AM CRLF);
	string_append_c(reply, "Content-Type: text/plain" CRLF);
	string_append_c(reply, "Content-Length: ");
	string_append_c(reply, content_length);
	string_append_c(reply, CRLF);
	string_append_c(reply, "Connection: close" CRLF CRLF);
	socket_write(fd, reply->c_str, reply->size);
	for (String **it = payload_lines; *it; ++it) {
		socket_write(fd, (*it)->c_str, (*it)->size);
		socket_write(fd, CRLF, strlen(CRLF));
	}
	string_delete(reply);
}

static void httpserver_reply_bad_request(int fd) {
	httpserver_reply_full(fd, "400 Bad Request");
}

static void httpserver_reply_forbidden(int fd) {
	httpserver_reply_full(fd, "403 Forbidden");
}

static void httpserver_reply_not_found(int fd) {
	httpserver_reply_full(fd, "404 Not Found");
}

static void httpserver_reply_method_not_allowed(int fd) {
	httpserver_reply_full(fd, "405 Method Not Allowed");
}

static void httpserver_reply_internal_server_error(int fd) {
	httpserver_reply_full(fd, "503 Internal Server Error");
}

static void httpserver_reply_get_file(int *network_socket, int *local_file) {
	// Find out file size and rewind
	size_t file_size = lseek(*local_file, 0, SEEK_END);
	char content_length[16];
	snprintf(content_length, sizeof(content_length), "%zu", file_size);
	lseek(*local_file, 0, SEEK_SET);

	// Generate and send header
	String *reply = string_new("");
	string_append_c(reply, "HTTP/1.1 200 OK" CRLF);
	string_append_c(reply, "Iam: " I_AM CRLF);
	string_append_c(reply, "Content-Type: text/plain" CRLF);
	string_append_c(reply, "Content-Length: ");
	string_append_c(reply, content_length);
	string_append_c(reply, CRLF);
	string_append_c(reply, "Connection: close" CRLF CRLF);
	socket_write(*network_socket, reply->c_str, reply->size);
	string_delete(reply);

	// Send content/payload
	size_t bytes_remaining = file_size;
	while (bytes_remaining > 0) {
		char buffer[8192];
		ssize_t bytes_read = socket_read(*local_file, buffer, sizeof buffer);
		if (bytes_read < 0) {
			break; // Abort, there's really nothing helpful to do after the headers are on the wire
		}
		ssize_t bytes_written = socket_write(*network_socket, buffer, bytes_read);
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

static inline String *httpserver_get_directory_contents(int *directory) {
	String *s = NULL;
	DIR *dir = fdopendir(*directory);
	*directory = -1; // DIR *dir takes ownership of the file descriptor
	if (dir) {
		s = httpserver_get_directory_contents_dir(dir);
	}
	closedir(dir);

	return s;
}

// Print directory contents
static void httpserver_reply_get_directory(int *network_socket, int *directory) {

	String *directory_contents = httpserver_get_directory_contents(directory);
	if (directory_contents) {
		char content_length[16];
		snprintf(content_length, sizeof(content_length), "%zu", directory_contents->size);

		String *reply = string_new("");
		string_append_c(reply, "HTTP/1.1 200 OK" CRLF);
		string_append_c(reply, "Iam: " I_AM CRLF);
		string_append_c(reply, "Content-Type: text/plain" CRLF);
		string_append_c(reply, "Content-Length: ");
		string_append_c(reply, content_length);
		string_append_c(reply, CRLF);
		string_append_c(reply, "Connection: close" CRLF CRLF);
		string_append(reply, directory_contents);
		socket_write(*network_socket, reply->c_str, reply->size);
		string_delete(reply);
	}
	else {
		httpserver_reply_internal_server_error(*network_socket);
	}
	string_delete(directory_contents);
}

static void httpserver_handle_get(int *fd, const char *path) {
	int local_file = -1;

	// This allows getting directory contents of document root.
	if (strcmp(path, "/") == 0) {
		path = "/.";
	}
	if (strlen(path) > 1 && (local_file = open(path + 1, O_RDONLY)) != -1) {
		if (is_regular_file(local_file)) {
			// Serve file contents
			httpserver_reply_get_file(fd, &local_file);
		} else if (is_directory(local_file)) {
			// Serve directory listing
			httpserver_reply_get_directory(fd, &local_file);
		} else {
			// Other than regular files or directories are not served
			httpserver_reply_forbidden(*fd);
		}
	} else {
		httpserver_reply_not_found(*fd);
	}

	socket_close(&local_file);
}

static void httpserver_reply_put(int *fd, int local_file, size_t content_length, String *header) {
	ssize_t r = -1;

	// Write initial header
	if (header) {
		r = socket_write(local_file, header->c_str, header->size);
		if (r == -1) {
			httpserver_reply_internal_server_error(*fd);
			return;
		}
		content_length -= header->size;
	}

	// Loop read and write
	while (content_length > 0) {
		char buf[8192];
		ssize_t bytes_read;
		bytes_read = socket_read(*fd, buf, sizeof(buf));
		if (bytes_read > 0) {
			content_length -= bytes_read;
			while (bytes_read > 0) {
				ssize_t bytes_written = socket_write(local_file, buf, bytes_read);
				if (bytes_written == -1) {
					// Could not write to disk -- internal error
					httpserver_reply_internal_server_error(*fd);
					return;
				}
				bytes_read -= bytes_written;
			}
		}
		else {
			// Could not read while content still remaining -- bad request
			httpserver_reply_bad_request(*fd);
			return;
		}
	}
	httpserver_reply_full(*fd, "201 Created");
}

static void httpserver_handle_put(int *fd, const char *path, String **header) {
	// Find content length and allocate a new file with enough space.
	ssize_t content_length = 0;
	bool header_ok = false;
	bool expect_100 = false;
	++header; // Skip the first HTTP action line
	while (header[0]) {
		if (strcmp(header[0]->c_str, "") == 0) {
			++header; // header contains the payload start now.
			header_ok = true;
			break;
		}
		// Find the content-length parameter
		if (strncasecmp(header[0]->c_str, "content-length:", strlen("content-length:")) == 0) {
			String **line = string_split(header[0], ":");
			sscanf(line[1]->c_str, "%zd", &content_length);
			string_delete_array(line);
		}
			if (strncasecmp(header[0]->c_str, "Expect: 100-continue", strlen("Expect: 100-continue")) == 0) {
			expect_100 = true;
		}
		++header;
	}
	if (header_ok && content_length >= 0) {
		if (expect_100) {
			// Happily accept anything
			httpserver_reply_header(*fd, "100 Continue");
		}
		// Create a new file for writing
		int local_file = open(path + 1, O_WRONLY | O_CREAT | O_TRUNC);
		if (local_file != -1) {
			httpserver_reply_put(fd, local_file, content_length, header[0]);
		}
		socket_close(&local_file);
	} else {
		httpserver_reply_bad_request(*fd);
	}
}

static void httpserver_reply_dns(int *fd, int dns_socket) {
	String **reply = dns_receive_answer(dns_socket);
	httpserver_reply_ok(*fd, reply);
	string_delete_array(reply);
}

static void httpserver_handle_dns_request(int *fd, int dns_type, const char *dns_name, const char *dns_server) {
	if (!dns_server) {
		dns_server = DEFAULT_DNS_SERVER;
	}
	int dns_socket = socket_udp_connect(dns_server, "53");
	if (dns_socket != -1) {
		dns_send_query(dns_socket, dns_type, dns_name);
		fd_set set;
		FD_ZERO(&set);
		FD_SET(dns_socket, &set);
		struct timeval timeout = { 2, 0 };
		select(dns_socket + 1, &set, NULL, NULL, &timeout);
		if FD_ISSET(dns_socket, &set) {
			httpserver_reply_dns(fd, dns_socket);
		} else {
			httpserver_reply_not_found(*fd);
		}
	}
	socket_close(&dns_socket);
}

// Find the payload string in a string array and return a pointer to it
// Return NULL if not found
static String *httpserver_find_payload(String **header) {
	while (*header) {
		// If found empty line delimiter AND there is a next field
		// then return that next field
		if (!strcmp("", header[0]->c_str) && header[1] != NULL) {
			return header[1];
		}
		++header;
	}
	return NULL;
}

static void httpserver_handle_post(int *fd, String **header) {
	// Find the payload
	String *payload = httpserver_find_payload(header);
	if (!payload) {
		httpserver_reply_bad_request(*fd);
		return;
	}

	// Get DNS request type and query string
	int dns_type = DNS_TYPE_A;
	char *dns_name = NULL;
	char *dns_server = NULL;
	String **fields = string_split(payload, "&");
	if (!fields) {
		httpserver_reply_internal_server_error(*fd);
		return;
	}
	for (String **data_iter = fields; *data_iter; ++data_iter) {
		String **key_value = string_split(*data_iter, "=");
		if (!strcasecmp("name", key_value[0]->c_str)) {
			dns_name = strdup(key_value[1]->c_str);
		}
		if (!strcasecmp("type", key_value[0]->c_str)) {
			if (!strcmp("A", key_value[1]->c_str)) {
				dns_type = DNS_TYPE_A;
			} else if (!strcmp("AAAA", key_value[1]->c_str)) {
				dns_type = DNS_TYPE_AAAA;
			}
		}
		if (!strcasecmp("server", key_value[0]->c_str)) {
			dns_server = strdup(key_value[1]->c_str);
		}
		string_delete_array(key_value);
	}
	if (dns_type && dns_name) {
		// Have both type and name, do the request
		VERBOSE("[%d] DNS query %s", *fd, dns_name);
		httpserver_handle_dns_request(fd, dns_type, dns_name, dns_server);
	} else {
		httpserver_reply_bad_request(*fd);
	}
	free(dns_server);
	free(dns_name);
	string_delete_array(fields);
}

/**
 * Worker thread. Handles incoming connections. Assumes ownership of argument struct.
 */
static void *httpserver_worker_thread(void *args) {
	ThreadData *thread_data = args;

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
	if (!header) {
		VERBOSE("[%d] Bad data: %d read from %d", thread_data->client_fd, bytes_read, thread_data->client_fd);
		perror("socket_read");
	}
	string_delete(incoming_data);
	if (header) {
		// Calculate number of lines in header
		size_t header_size = 0;
		for (; header[header_size]; ++header_size);

		if (header_size > 2) {
			// At least request \r\n \r\n payload
			char action[10], path[512], version[10];
			sscanf(header[0]->c_str, "%9s %511s %9s", action, path, version);
			VERBOSE("[%d] %s %s %s", thread_data->client_fd, action, path, version);

			if (!strcmp(action, "GET")) {
				httpserver_handle_get(&thread_data->client_fd, path);
			}
			else if (!strcmp(action, "PUT")) {
				httpserver_handle_put(&thread_data->client_fd, path, header);
			}
			else if (!strcmp(action, "POST") &&
				!strcasecmp(path, "/dns-query")) {
				httpserver_handle_post(&thread_data->client_fd, header);
			}
			else {
				httpserver_reply_method_not_allowed(thread_data->client_fd);
			}
		}
		else {
			httpserver_reply_bad_request(thread_data->client_fd);
		}
	}
	string_delete_array(header);

	socket_close(&thread_data->client_fd);
	free(buffer);
	free(thread_data);

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

			struct sockaddr_in6 peer;
			socklen_t peer_length = sizeof(peer);
			int incoming_socket = accept(listen_socket, &peer, &peer_length);
			if (incoming_socket != -1) {
				char peer_hostname[128];
				peer_hostname[0] = '\0';
				char peer_port[16];
				peer_port[0] = '\0';
				getnameinfo((struct sockaddr *) &peer, peer_length,
					peer_hostname, sizeof peer_hostname,
					peer_port, sizeof peer_port,
					NI_NUMERICHOST | NI_NUMERICSERV);
				VERBOSE("[%d] Incoming connection from %s:%s", incoming_socket, peer_hostname, peer_port);
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
	VERBOSE("Closing listening socket", listen_socket);
	socket_close(&listen_socket);

	return 0;
}
