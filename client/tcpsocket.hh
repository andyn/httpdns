#ifndef TCPSOCKET_HH
#define TCPSOCKET_HH

#include "socket.hh"

/**
 * @short Exception type for class TcpSocket
 */
struct ConnectError : public std::runtime_error {
    ConnectError(std::string const &str) : std::runtime_error(str) {}
};

/**
 * @short A wrapper for TCP sockets.
 * Defaults to IPv6 and any protocol
 */
class TcpSocket : public Socket {
public:
    TcpSocket(int domain, int protocol) :
        Socket(domain, SOCK_STREAM, protocol) {}

    TcpSocket(int fd)
        : Socket(fd) {}

    // virtual ~TcpSocket();
};

#endif // TCPSOCKET_HH

