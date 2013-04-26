#include <cstdarg>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

#include "filedescriptor.hh"

FileDescriptor::FileDescriptor(int fd)  {
    if (fd == -1)
        throw std::runtime_error("Cannot wrap an invalid file descriptor");
    m_fd = fd;
}

FileDescriptor::~FileDescriptor() {
    // Try to free the file descriptor. Repeat if interrupted by a signal.
    do {
        int success = close(m_fd);
        if (success == -1) {
            if (errno == EINTR) {
                continue;
            }
            else {
                std::cerr << "Error closing file descriptor in destructor."
                          << std::endl;
            }
        }
    } while (false);
}

ssize_t FileDescriptor::read(vector<char> &buffer) {
    size_t bytes_remaining = buffer.size();
    size_t bytes_read = 0;
    while (bytes_remaining > 0) {
        ssize_t result = ::read(m_fd, &buffer[bytes_read], bytes_remaining);
        if (result == -1) {
            if (errno == EINTR)
                continue;
            else
                throw (std::runtime_error(strerror(errno)));
        }
        else if (result == 0)
            break;
        bytes_read += result;
        bytes_remaining -= result;
    }
    
    return bytes_read;
}

ssize_t FileDescriptor::writeCharBuffer(char const *buffer,
                                        size_t bytes_remaining) {
    size_t bytes_written = 0;
    while (bytes_remaining > 0) {
        ssize_t result = ::write(m_fd, &buffer[bytes_written], bytes_remaining);
        if (result == -1) {
            if (errno == EINTR)
                continue;
            else
                throw std::runtime_error(strerror(errno));
        }
        
        bytes_written += result;
        bytes_remaining -= result;
    }
    
    return bytes_written;
}

ssize_t FileDescriptor::write(string const &buffer) {
    return writeCharBuffer(buffer.c_str(),
                           buffer.size());
}

ssize_t FileDescriptor::write(vector<char> const &buffer) {
    return writeCharBuffer(&buffer[0], buffer.size());
}

int FileDescriptor::fcntl(int cmd, ...) {
    va_list ellipsis;
    va_start(ellipsis, cmd);
    return ::fcntl(m_fd, cmd, ellipsis);
    va_end(ellipsis);
}


