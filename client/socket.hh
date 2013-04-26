#ifndef SOCKET_HH
#define SOCKET_HH

/**
 * @short Wrapper functions for UNIX sockets
 * @author Antti Nilakari 66648T <antti.nilakari@aalto.fi>
 */

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/types.h>
#include <sys/socket.h> 

class Socket : public FileDescriptor{
public:
    /**
     * @short Create a new socket.
     */
    Socket(int domain, int type, int protocol)
        : FileDescriptor(socket(domain, type, protocol)) {}

    /**
     * @short Construct a socket by assuming ownership of a preexisting socket
     */
    Socket(int fd)
        : FileDescriptor(fd) {}
        
    /**
     * @short Send the contents of a buffer into a file descriptor.
     * This is the C style version of the function. See man 2 send for details.
     */
    ssize_t send(void const *buf, size_t len, int flags = 0) {
        do {
            ssize_t bytes_written = ::send(m_fd, buf, len, flags);
            if (bytes_written == -1) {
                if (errno == EINTR) // Retry if caught a signal;
                    continue;
                else
                    throw std::runtime_error(strerror(errno));
            } else {
                return bytes_written;
            }
        } while (true);            
    }
    
    /**
     * @ Send the contents of a string into a file descriptor.
     * The C++ style version. See man 2 send for details
     */
    ssize_t send(std::string const &buf, int flags = 0) {
        // Temp variables are needed in order to cast params.
        void const *voidptr = buf.c_str();
        size_t bufsize = buf.size();
        return Socket::send(voidptr, bufsize, flags);
    }
    
private:
    Socket(Socket const &); // Noncopyable, RO3
    Socket &operator=(Socket const &); // Noncopyable, RO3
    
};

#endif // SOCKET_HH

