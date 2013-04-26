#ifndef TCPCLIENTSOCKET_HH
#define TCPCLIENTSOCKET_HH

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "tcpsocket.hh"

/**
 * @short A wrapper for TCP client sockets.
 * Is constructed from an addrinfo list.
 */
class TcpClientSocket : public TcpSocket {
public:
    TcpClientSocket(Addrinfo const &addresses)
        : TcpSocket(0) {
        struct addrinfo const *a = addresses.ai();
        while (a != NULL) {
            // We DO want to force SOCK_STREAM, so if unavailable,
            // try the next one
            int sock = socket(a->ai_family, SOCK_STREAM, a->ai_protocol);
            if (sock == -1) {
                a = a->ai_next;
            }
            else {
                while (true) {
                    int success = connect(sock, a->ai_addr, a->ai_addrlen);
                    if (success != 0) {
                        if (errno == EINTR)
                            continue;
                        else
                            throw ConnectError(strerror(errno));
                    }
                    else { // success!
                        m_fd = sock;
                        break;
                    }
                }
                break;
            }
        }
    }
};

#endif // TCPSOCKET_HH

