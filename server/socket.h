#ifndef SOCKET_H_
#define SOCKET_H_
/**
 * Socket module
 * Contains socket and file descriptor related helper and wrapper functionality
 */

#include <stddef.h>
#include <unistd.h>
 
/**
 * Close a socket, retrying on EINTR. See man 2 close.
 * Sets file descriptor to -1 to help avoid multiple closes on same data.
 * @param fd Pointer to file descriptor to close & set to -1.
 * @return Zero on success, -1 on failure (EBADF, EIO)
 */
int socket_close(int *fd);

/**
 * Connect to a TCP IPv4/IPv6 host.
 * @param hostname Host name to connect to.
 * @param port Port or service name to connect to.
 * @return A file descriptor on success, -1 on failure (with appropriate errno set).
 */
int socket_tcp_connect(const char *hostname, const char *port);

/**
 * Listen on a TCP IPv4/IPv6 socket.
 * @param hostname Host name to bind to. NULL to bind to all interfaces.
 * @param port Port or service name to listen on.
 * @return A file descriptor on success, -1 on failure (with appropriate errno set).
 */
int socket_tcp_listen(const char *hostname, const char *port);

/**
 * Write to a socket, retrying on EINTR. See man 2 write.
 * @param fd File descriptor to read from.
 * @param buf Buffer whose contents to write.
 * @param count Number of bytes to write.
 */
ssize_t socket_read(int fd, void *buf, size_t count);

/**
 * Write to a socket, retrying on EINTR. See man 2 write.
 * @param fd File descriptor to write to.
 * @param buf Buffer whose contents to write.
 * @param count Number of bytes to write.
 */
ssize_t socket_write(int fd, const void *buf, size_t count);

#endif
