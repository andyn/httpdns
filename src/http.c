#include <stdlib.h>

#include "http.h"
#include "socket.h"
#include "string.h"
#include "url.h"
#include "util.h"

/**
 * HTTP PUT buffer contents to a remote URL
 * @param url URL to PUT the contents to
 * @param buf Buffer whose contents to send
 * @param count Size of buffer (number of bytes to send)
 * @return HTTP status code, 200 for OK. See the HTTP RFC for details.
 */
int http_put_buf(const char *url, const void *buf, size_t count) {
	char *host = url_get_field(url, URL_HOST);
	char *port = url_get_field(url, URL_PORT);
	char *path = url_get_field(url, URL_PATH);

	int response_code = -1;

	if (host && port && path) {
		String *buffer = string_new("");

		string_append_c(buffer, "PUT ");
		string_append_c(buffer, path);
		string_append_c(buffer, " HTTP/1.1\r\n");

		string_append_c(buffer, "Host: ");
		string_append_c(buffer, host);
		string_append_c(buffer, "\r\n");

		string_append_c(buffer, "Content-type: text/plain\r\n");

		if (count > 0) {
			char length_str[16];
			snprintf(length_str, sizeof length_str, "%zu", count);
			string_append_c(buffer, "Content-length: ");
			string_append_c(buffer, length_str);
			string_append_c(buffer, "\r\n");
		}

		string_append_c(buffer, "Connection: close\r\n");
		string_append_c(buffer, "Iam: anilakar\r\n");

		string_append_c(buffer, "\r\n");

		int http_socket = socket_tcp_connect(host, port);
		if (http_socket >= 0) {
			if (-1 == socket_write(http_socket, buffer->c_str, buffer->size) ||
				-1 == socket_write(http_socket, buf, count)) {
				VERBOSE("Could not write to HTTP server.");
			} else {
				char response[512]; // Need only the first line
				int r = socket_read(http_socket, response, sizeof response - 1);
				response[r] = '\0';
				sscanf(response, "HTTP/1.1 %d", &response_code);				
			}
		}
		socket_close(http_socket);
		string_delete(buffer);

	}
	else {
		VERBOSE("Malformed URL: %s", url);
	}

	free(path);
	free(port);
	free(host);

	VERBOSE("HTTP Response: %d", response_code);
	return response_code;
}

