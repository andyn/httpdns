#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "socket.h"
#include "util.h"

int socket_close(int *fd_ptr) {
	int r = -1;
	while (fd_ptr) {
		int fd = *fd_ptr;
		if (fd < 0) {
			break;
		}

		while (-1 == (r = close(fd))) {
			if (errno == EINTR) {
				continue;
			}
			else {
				break;
			}
		}
		*fd_ptr = -1;
		break;
	}
	return r;
}

// Common functionality to all protocols
static int socket_connect(const char *hostname, const char *port, int socktype) {
	if (!hostname || !port) {
		return -1;
	}

	// Prepare the hints struct
	struct addrinfo *address, hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = socktype;
	hints.ai_flags = AI_PASSIVE | AI_ALL | AI_V4MAPPED;

	// Get a list of addresses to try
	int r = -1;	
	if (0 != (r = getaddrinfo(hostname, port, &hints, &address))) {
		return -1;
	}

	// Iterate over the list of addresses
	int fd = -1;
	for (struct addrinfo *iter = address; iter; iter = iter->ai_next) {
		if (-1 == (fd = socket(iter->ai_family, iter->ai_socktype, address->ai_protocol))) {
			perror("socket");
			continue;
		}
		if (-1 == connect(fd, address->ai_addr, address->ai_addrlen)) {
			perror("connect");
			socket_close(&fd);
			fd = -1;
		}
		// Success
		break;
	}
	freeaddrinfo(address);

	return fd;
}

int socket_udp_connect(const char *hostname, const char *port) {
	return socket_connect(hostname, port, SOCK_DGRAM);
}

int socket_tcp_connect(const char *hostname, const char *port) {
	return socket_connect(hostname, port, SOCK_STREAM);
}

int socket_tcp_listen(const char *hostname, const char *port) {
	// NULL port / service name is not accepted.
	// NULL hostname is OK. (== bind to any interface)
	if (!port) {
		return -1;
	}

	struct addrinfo *address, hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE | AI_ALL | AI_V4MAPPED;

	int r = -1;	
	if (0 != (r = getaddrinfo(hostname, port, &hints, &address))) {
		return -1;
	}
	int fd = -1;
	for (struct addrinfo *iter = address; iter; iter = iter->ai_next) {
		if (-1 == (fd = socket(iter->ai_family, iter->ai_socktype, address->ai_protocol))) {
			perror("socket");
			continue;
		}
		int yes = 1;
		if (-1 == setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))) {
			perror("setsockopt");
			socket_close(&fd);
			continue;
		}
		if (-1 == bind(fd, iter->ai_addr, iter->ai_addrlen)) {
			perror("bind");
			socket_close(&fd);
			continue;
		}
		if (-1 == listen(fd, 128)) {
			perror("listen");
			socket_close(&fd);
		}
		// Success
		break;
	}
	freeaddrinfo(address);

	return fd;
}

ssize_t socket_read(int fd, void *buf, size_t count) {
	// Prevent UB
	if (fd < 0 || buf == NULL || count > SSIZE_MAX) {
		return -1;
	}

	char *buffer = buf;
	// TODO better return value checking
	return read(fd, buffer, count);
}

ssize_t socket_write(int fd, const void *buf, size_t count) {
	// Void cast to char because I want prototype compatibility with write(2)
	const char *buffer = buf;
	size_t written_total = 0;
	while (count > 0) {
		ssize_t written_now = write(fd, buffer, count);
		if (written_now == -1) {
			if (errno == EINTR) {
				continue;
			} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return written_total;
			}
			else {
				return -1;
			}
		}
		buffer += written_now;
		count -= written_now;
		written_total += written_now;
	}
	return written_total;
}
